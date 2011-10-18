// =============================================================== //
//                                                                 //
//   File      : adcomm.cxx                                        //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <unistd.h>

#include <csignal>
#include <ctime>
#include <cerrno>

#include <sys/socket.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include "gb_storage.h"
#include "gb_comm.h"
#include "gb_localdata.h"

#include <arb_signal.h>
#include <arb_file.h>
#include <static_assert.h>

__ATTR__USERESULT static GBCM_ServerResult gbcms_talking(int con, long *hs, void *sin);

#define debug_printf(a, b)

#define GBCMS_TRANSACTION_TIMEOUT 60*60             // one hour timeout
#define MAX_QUEUE_LEN             5

#define GBCM_COMMAND_UNFOLD             (GBTUM_MAGIC_NUMBER)
#define GBCM_COMMAND_PUT_UPDATE         (GBTUM_MAGIC_NUMBER+2)
#define GBCM_COMMAND_BEGIN_TRANSACTION  (GBTUM_MAGIC_NUMBER+4)
#define GBCM_COMMAND_COMMIT_TRANSACTION (GBTUM_MAGIC_NUMBER+5)
#define GBCM_COMMAND_ABORT_TRANSACTION  (GBTUM_MAGIC_NUMBER+6)
#define GBCM_COMMAND_INIT_TRANSACTION   (GBTUM_MAGIC_NUMBER+7)
#define GBCM_COMMAND_FIND               (GBTUM_MAGIC_NUMBER+8)
#define GBCM_COMMAND_CLOSE              (GBTUM_MAGIC_NUMBER+9)
#define GBCM_COMMAND_KEY_ALLOC          (GBTUM_MAGIC_NUMBER+11)
#define GBCM_COMMAND_UNDO               (GBTUM_MAGIC_NUMBER+12)
#define GBCM_COMMAND_DONT_WAIT          (GBTUM_MAGIC_NUMBER+13)

#define GBCM_COMMAND_SEND               (GBTUM_MAGIC_NUMBER+0x1000)
#define GBCM_COMMAND_SEND_COUNT         (GBTUM_MAGIC_NUMBER+0x2000)
#define GBCM_COMMAND_SETDEPTH           (GBTUM_MAGIC_NUMBER+0x3000)
#define GBCM_COMMAND_SETINDEX           (GBTUM_MAGIC_NUMBER+0x4000)
#define GBCM_COMMAND_PUT_UPDATE_KEYS    (GBTUM_MAGIC_NUMBER+0x5000)
#define GBCM_COMMAND_PUT_UPDATE_CREATE  (GBTUM_MAGIC_NUMBER+0x6000)
#define GBCM_COMMAND_PUT_UPDATE_DELETE  (GBTUM_MAGIC_NUMBER+0x7000)
#define GBCM_COMMAND_PUT_UPDATE_UPDATE  (GBTUM_MAGIC_NUMBER+0x8000)
#define GBCM_COMMAND_PUT_UPDATE_END     (GBTUM_MAGIC_NUMBER+0x9000)
#define GBCM_COMMAND_TRANSACTION_RETURN (GBTUM_MAGIC_NUMBER+0x100000)
#define GBCM_COMMAND_FIND_ERG           (GBTUM_MAGIC_NUMBER+0x108000)
#define GBCM_COMMAND_KEY_ALLOC_RES      (GBTUM_MAGIC_NUMBER+0x10b000)
#define GBCM_COMMAND_UNDO_CMD           (GBTUM_MAGIC_NUMBER+0x10a0001)

// ------------------------
//      some structures

struct gbcms_delete_list {                          // Store all deleted items in a list
    gbcms_delete_list *next;
    long               creation_date;
    long               update_date;
    GBDATA            *gbd;
};

struct Socinf {
    Socinf            *next;
    int                socket;
    gbcms_delete_list *dl;                          // point to last deleted item that is sent to this client
    char              *username;
};

void g_bcms_delete_Socinf(Socinf *THIS) {
    freenull(THIS->username);
    THIS->next = 0;
    free(THIS);
}

struct gb_server_data {
    int                hso;
    char              *unix_name;
    Socinf            *soci;
    long               nsoc;
    long               timeout;
    GBDATA            *gb_main;
    int                wait_for_new_request;
    gbcms_delete_list *del_first; // All deleted items, that are yet unknown to at least one client
    gbcms_delete_list *del_last;

    SigHandler old_SIGPIPE_handler;
    SigHandler old_SIGHUP_handler;
};



struct gbcms_create {
    gbcms_create *next;
    GBDATA       *server_id;
    GBDATA       *client_id;
};


// --------------------
//      Panic save

static GBCONTAINER *gbcms_gb_main;

static const char *panic_file_name() {
    const char *ap = GB_getenv("ARB_PID");
    if (!ap) ap    = "";
    return GBS_global_string("arb_panic_%s_%s", GB_getenvUSER(), ap);
}

static void gbcms_sighup(int) {
    char *panic_file = 0;                      // hang-up trigger file
    char *db_panic   = 0;                      // file to save DB to
    {
        FILE *in = GB_fopen_tempfile(panic_file_name(), "rt", &panic_file);

        fprintf(stderr,
                "**** ARB DATABASE SERVER received a HANGUP SIGNAL ****\n"
                "- Looking for file '%s'\n",
                panic_file);

        if (in) {
            db_panic = GB_read_fp(in);
            fclose(in);
        }
    }

    if (!db_panic) {
        fprintf(stderr,
                "- Could not read '%s' (Reason: %s)\n"
                "[maybe retry]\n",
                panic_file, GB_await_error());
    }
    else {
        char *newline           = strchr(db_panic, '\n');
        if (newline) newline[0] = 0;

        GB_MAIN_TYPE *Main       = GBCONTAINER_MAIN(gbcms_gb_main);
        int           translevel = Main->transaction;

        fprintf(stderr, "- Trying to save DATABASE in ASCII mode into file '%s'\n", db_panic);

        Main->transaction = 0;
        GB_ERROR error    = GB_save_as((GBDATA *) gbcms_gb_main, db_panic, "a");

        if (error) fprintf(stderr, "Error while saving '%s': %s\n", db_panic, error);
        else fprintf(stderr, "- DATABASE saved into '%s' (ASCII)\n", db_panic);

        unlink(panic_file);
        Main->transaction = translevel;

        free(db_panic);
    }
    free(panic_file);
}

GB_ERROR GBCMS_open(const char *path, long timeout, GBDATA *gb_main) {
    // server open

    GB_MAIN_TYPE *Main  = GB_MAIN(gb_main);
    GB_ERROR      error = 0;

    if (Main->server_data) {
        error = "reopen of server not allowed";
    }
    else {
        gbcmc_comm *comm = gbcmc_open(path);
        if (comm) {
            error = GBS_global_string("Socket '%s' already in use", path);
            IGNORE_RESULT(gbcmc_close(comm));
        }
        else {
            int   socket;
            char *unix_name;

            error = gbcm_open_socket(path, TCP_NODELAY, 0, &socket, &unix_name);
            if (!error) {

                SigHandler old_SIGPIPE_handler = INSTALL_SIGHANDLER(SIGPIPE, gbcms_sigpipe, "GBCMS_open");
                SigHandler old_SIGHUP_handler  = INSTALL_SIGHANDLER(SIGHUP, gbcms_sighup, "GBCMS_open");

                gb_assert(is_default_or_ignore_sighandler(old_SIGPIPE_handler));
                gb_assert(old_SIGHUP_handler == SIG_DFL);

                gbcms_gb_main = (GBCONTAINER *)gb_main;

                if (listen(socket, MAX_QUEUE_LEN) < 0) {
                    error = GBS_global_string("could not listen (server; errno=%i)", errno);
                }
                else {
                    gb_server_data *hs = (gb_server_data *)GB_calloc(sizeof(gb_server_data), 1);

                    hs->timeout   = timeout;
                    hs->gb_main   = gb_main;
                    hs->hso       = socket;
                    hs->unix_name = unix_name;

                    hs->old_SIGPIPE_handler = old_SIGPIPE_handler;
                    hs->old_SIGHUP_handler  = old_SIGHUP_handler;

                    Main->server_data = hs;
                }
            }
        }
    }

    if (error) {
        error = GBS_global_string("ARB_DB_SERVER_ERROR: %s", error);
        fprintf(stderr, "%s\n", error);
    }
    return error;
}

void GBCMS_shutdown(GBDATA *gbd) {
    // server close

    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    if (Main->server_data) {
        gb_server_data *hs = Main->server_data;
        for (Socinf *si = hs->soci; si; si=si->next) {
            shutdown(si->socket, SHUT_RDWR);
            close(si->socket);
        }
        shutdown(hs->hso, SHUT_RDWR);

        if (hs->unix_name) {
            unlink(hs->unix_name);
            freenull(hs->unix_name);
        }
        close(hs->hso);

        ASSERT_RESULT(SigHandler, gbcms_sigpipe, INSTALL_SIGHANDLER(SIGPIPE, hs->old_SIGPIPE_handler, "GBCMS_shutdown"));
        ASSERT_RESULT(SigHandler, gbcms_sighup,  INSTALL_SIGHANDLER(SIGHUP,  hs->old_SIGHUP_handler,  "GBCMS_shutdown"));
        
        freenull(Main->server_data);
    }
}

#define MAXBUF 8 // max buffersize needed

class WriteBuffer {
    bool   sendSize;
    long   buf[MAXBUF];
    size_t pos;
public:
    WriteBuffer(long command, long firstData)
        : sendSize(false), pos(2)
    {
        buf[0] = command;
        buf[1] = firstData;
    }
    explicit WriteBuffer(long command) // automatically sends size as 2nd data
        : sendSize(true), pos(2)
    {
        buf[0] = command;
        buf[1] = 0; // reserve for size
    }

    void put(long data) { gb_assert(pos<MAXBUF); buf[pos++] = data; }

    template <typename T> void putSmall(const T& val) {
        COMPILE_ASSERT(sizeof(T)<sizeof(long));
        long  L     = 0; // otherwise uninitialized bytes will be sent
        *((T*)(void*)(&L)) = val;
        put(L);
    }

    GBCM_ServerResult send_to(int socket) {
        if (sendSize) buf[1] = pos;
        return gbcm_write(socket, (const char*)buf, pos*sizeof(*buf));
    }
};

__ATTR__USERESULT static GBCM_ServerResult gbcm_write_GBDATA(int socket, GBDATA *gbd, long depth, bool send_headera) {
    /* send a database item to client/server
     *
     * depth         = 0 -> just send one item, != 0 -> send sub entries too (use -1 for "full" recursion)
     * send_headera = 1 -> if type = GB_DB send flag and key_quark array
     */

    WriteBuffer buf(GBCM_COMMAND_SEND);
    buf.put((long)gbd);
    buf.put(gbd->index);
    buf.putSmall(gbd->flags);

    GBCM_ServerResult result = GBCM_ServerResult::OK();
    if (GB_TYPE(gbd) == GB_DB) {
        GBCONTAINER *gbc = (GBCONTAINER *)gbd;
        int          end = gbc->d.nheader;
        
        buf.putSmall(gbc->flags3);
        buf.put(send_headera ? end : -1);
        buf.put(depth ? gbc->d.size : -1);

        result = buf.send_to(socket);

        if (result.ok() && send_headera) {
            gb_header_list  *hdl  = GB_DATA_LIST_HEADER(gbc->d);
            gb_header_flags *buf2 = (gb_header_flags *)GB_give_buffer2(gbc->d.nheader * sizeof(gb_header_flags));

            for (int index = 0; index < end; index++) {
                buf2[index] = hdl[index].flags;
            }
            result = gbcm_write(socket, (const char *)buf2, end * sizeof(gb_header_flags));
        }
        if (depth && result.ok()) {
            debug_printf("%i    ", gbc->d.size);

            for (int index = 0; index < end && result.ok(); index++) {
                GBDATA *gb2 = GBCONTAINER_ELEM(gbc, index);
                if (gb2) {
                    debug_printf("%i ", index);
                    result = gbcm_write_GBDATA(socket, gb2, depth-1, send_headera);
                }
            }
            debug_printf("\n", 0);
        }
    }
    else if ((unsigned int)GB_TYPE(gbd) < (unsigned int)GB_BITS) {
        buf.put(gbd->info.i);
        result = buf.send_to(socket);
    }
    else {
        long memsize = GB_GETMEMSIZE(gbd);
        buf.put(GB_GETSIZE(gbd));
        buf.put(memsize);
        
        result = buf.send_to(socket);
        if (result.ok()) result = gbcm_write(socket, GB_GETDATA(gbd), memsize);
    }
    return result;
}

__ATTR__USERESULT static GBCM_ServerResult gbcm_check_address(void *ptr) {
    GB_ERROR error = GBK_test_address((long*)(ptr), GBTUM_MAGIC_NUMBER);
    return error ? GBCM_ServerResult::FAULT(error) : GBCM_ServerResult::OK();
}

__ATTR__USERESULT static GBCM_ServerResult gbcm_read_GBDATA(int socket, GBCONTAINER *gbd, long *buffer, long mode, GBDATA *gb_source, void *cs_main) {
    /* read an entry into gbd
     * mode ==  1  server reads data
     * mode ==  0  client read all data
     * mode == -1  client read but do not read subobjects -> folded cont
     * mode == -2  client dummy read
     */
    GBCM_ServerResult result = gbcm_read_expect_size(socket, (char *)buffer, sizeof(long) * 3);

    if (result.ok() && buffer[0] != GBCM_COMMAND_SEND) {
        result = GBCM_ServerResult::FAULT(GBS_global_string("receive expected command %i, got %i", GBCM_COMMAND_SEND, int(buffer[0])));
    }

    if (result.ok()) {
        long id = buffer[2];
        long i  = buffer[1];
        i       = sizeof(long) * (i - 3);

        result = gbcm_read_expect_size(socket, (char *)buffer, i);
        if (result.failed()) result.annotate("failed to receive DB-node");
        else {
            i = 0;
            long index_pos = buffer[i++];
            if (!gb_source && gbd && index_pos<gbd->d.nheader) {
                gb_source = GBCONTAINER_ELEM(gbd, index_pos);
            }

            gb_flag_types flags = *(gb_flag_types *)(&buffer[i++]);
            int           type  = flags.type;

            GBDATA *gb2;
            if (mode >= -1) {   // real read data
                if (gb_source) {
                    int types = GB_TYPE(gb_source);
                    gb2 = gb_source;
                    if (types != type) {
                        result = GBCM_ServerResult::FAULT(GBS_global_string("data type mismatch (local=%i, remote=%i)", types, type));
                    }
                    else {
                        if (mode>0) {   // transactions only in server
                            result = gbcm_check_address(gb2);
                        }
                        if (result.ok()) {
                            if (types != GB_DB) {
                                gb_save_extern_data_in_ts(gb2);
                            }
                            gb_touch_entry(gb2, GB_NORMAL_CHANGE);
                        }
                    }
                }
                else {
                    if (mode==-1) goto dont_create_in_a_folded_container;
                    if (type == GB_DB) {
                        gb2 = (GBDATA *)gb_make_container(gbd, 0, index_pos, GB_DATA_LIST_HEADER(gbd->d)[index_pos].flags.key_quark);
                    }
                    else {  // @@@ Header Transaction stimmt nicht
                        gb2 = gb_make_entry(gbd, 0, index_pos, GB_DATA_LIST_HEADER(gbd->d)[index_pos].flags.key_quark, (GB_TYPES)type);
                    }
                    if (mode>0) {   // transaction only in server
                        gb_touch_entry(gb2, GB_CREATED);
                    }
                    else {
                        gb2->server_id = id;
                        GBS_write_numhash(GB_MAIN(gb2)->remote_hash, id, (long) gb2);
                    }
                    if (cs_main) {
                        gbcms_create *cs = (gbcms_create *) GB_calloc(sizeof(gbcms_create), 1);
                        
                        cs->server_id = gb2;
                        cs->client_id = (GBDATA *) id;
                        cs->next      = *((gbcms_create **) cs_main);

                        *((gbcms_create **) cs_main) = cs;
                    }
                }
                if (result.ok()) {
                    gb2->flags = flags;
                    if (type == GB_DB) {
                        ((GBCONTAINER *)gb2)->flags3 = *((gb_flag_types3 *)&(buffer[i++]));
                    }
                }
            }
            else {
              dont_create_in_a_folded_container :
                if (type == GB_DB) i++; // skip over gb_flag_types3
                gb2 = 0;
            }

            if (result.ok()) {
                if (type == GB_DB) {
                    long nheader = buffer[i++];
                    long nitems  = buffer[i++];

                    if (nheader > 0) {
                        long             realsize = nheader* sizeof(gb_header_flags);
                        gb_header_flags *buffer2  = (gb_header_flags *)GB_give_buffer2(realsize);

                        result = gbcm_read_expect_size(socket, (char *)buffer2, realsize);
                        if (result.failed()) result.annotate("failed to receive data");
                        else {
                            if (gb2 && mode >= -1) {
                                GBCONTAINER    *gbc  = (GBCONTAINER*)gb2;
                                gb_header_list *hdl;
                                GB_MAIN_TYPE   *Main = GBCONTAINER_MAIN(gbc);

                                gb_create_header_array(gbc, (int)nheader);
                                if (nheader < gbc->d.nheader) {
                                    GB_internal_error("Inconsistency Client-Server Cache");
                                }
                                gbc->d.nheader = (int)nheader;
                                hdl = GB_DATA_LIST_HEADER(gbc->d);
                                for (long item = 0; item < nheader; item++) {
                                    GBQUARK old_index = hdl->flags.key_quark;
                                    GBQUARK new_index = buffer2->key_quark;
                                    if (new_index && !old_index) {  // a rename ...
                                        gb_write_index_key(gbc, item, new_index);
                                    }
                                    if (mode>0) {   // server read data


                                    }
                                    else {
                                        if (buffer2->changed >= GB_DELETED) {
                                            hdl->flags.changed = GB_DELETED;
                                            hdl->flags.ever_changed = 1;
                                        }
                                    }
                                    hdl->flags.flags = buffer2->flags;
                                    hdl++; buffer2++;
                                }
                                if (mode>0) {   // transaction only in server
                                    gb_touch_header(gbc);
                                }
                                else {
                                    gbc->header_update_date = Main->clock;
                                }
                            }
                        }
                    }

                    if (result.ok()) {
                        if (nitems >= 0) {
                            long newmod = mode;
                            if (mode>=0) {
                                if (mode==0 && nitems<=1) {         // only a partial send
                                    UNCOVERED();
                                    gb2->flags2.folded_container = 1;
                                }
                            }
                            else {
                                UNCOVERED();
                                newmod = -2;
                            }
                            debug_printf("Client %i \n", nheader);
                            for (long item = 0; item < nitems && result.ok(); item++) {
                                debug_printf("  Client reading %i\n", item);
                                result = gbcm_read_GBDATA(socket, (GBCONTAINER *)gb2, buffer, newmod, 0, cs_main);
                            }
                            debug_printf("Client done\n", 0);
                        }
                        else {
                            if ((mode==0) && !gb_source) {      // created GBDATA at client
                                gb2->flags2.folded_container = 1;
                            }
                        }
                    }
                }
                else if (type < GB_BITS) {
                    if (mode >= 0)   gb2->info.i = buffer[i++];
                }
                else {
                    if (mode >= 0) {
                        long  realsize = buffer[i++];
                        long  memsize  = buffer[i++];

                        GB_INDEX_CHECK_OUT(gb2);

                        assert_or_exit(!(gb2->flags2.extern_data && GB_EXTERN_DATA_DATA(gb2->info.ex)));

                        char *data;
                        if (GB_CHECKINTERN(realsize, memsize)) {
                            GB_SETINTERN(gb2);
                            data = GB_GETDATA(gb2);
                        }
                        else {
                            GB_SETEXTERN(gb2);
                            data = (char*)gbm_get_mem((size_t)memsize, GB_GBM_INDEX(gb2));
                        }
                        result = gbcm_read_expect_size(socket, data, memsize);
                        if (result.ok()) GB_SETSMD(gb2, realsize, memsize, data);

                    }
                    else {            // dummy read (e.g. updata in server && not cached in client
                        i++;
                        long  memsize = buffer[i++];
                        char *buffer2 = GB_give_buffer2(memsize);
                        result        = gbcm_read_expect_size(socket, buffer2, memsize);
                    }
                }
            }
        }
    }
    return result;
}


static void gbcms_shift_delete_list(void *hsi, void *soi) {
    gb_server_data *hs     = (gb_server_data *)hsi;
    Socinf         *socinf = (Socinf *)soi;

    if (!hs->del_first) return;
    while ((!socinf->dl) || (socinf->dl->next)) {
        if (!socinf->dl) socinf->dl = hs->del_first;
        else    socinf->dl = socinf->dl->next;
    }
}

__ATTR__USERESULT inline GBCM_ServerResult gbcm_write_two(int socket, long L1, long L2) {
    return WriteBuffer(L1, L2).send_to(socket);
}

__ATTR__USERESULT static GBCM_ServerResult gbcms_write_deleted(int socket, long hsin, long client_clock) {
    gb_server_data *hs = (gb_server_data *)hsin;
    Socinf         *socinf;
    for (socinf = hs->soci; socinf; socinf=socinf->next) {
        if (socinf->socket == socket) break;
    }
    GBCM_ServerResult result = GBCM_ServerResult::OK();
    if (socinf && hs->del_first) {
        while (result.ok() && (!socinf->dl || (socinf->dl->next))) {
            if (!socinf->dl) socinf->dl = hs->del_first;
            else             socinf->dl = socinf->dl->next;

            if (socinf->dl->creation_date <= client_clock) {
                result = gbcm_write_two(socket, GBCM_COMMAND_PUT_UPDATE_DELETE, (long)socinf->dl->gbd);
            }
        }
        if (result.ok()) {
            for (socinf = hs->soci; socinf; socinf=socinf->next) {
                if (!socinf->dl) return result;
            }
            gbcms_delete_list *dl;
            while ((dl = hs->del_first)) {
                for (socinf = hs->soci; socinf; socinf=socinf->next) {
                    if (socinf->dl == dl) return result;
                }
                hs->del_first = dl->next;
                gbm_free_mem(dl, sizeof(gbcms_delete_list), GBM_CB_INDEX);
            }
        }
    }
    return result;
}

__ATTR__USERESULT static GBCM_ServerResult gbcms_write_updated(int socket, GBDATA *gbd, long hsin, long client_clock) {
    GBCM_ServerResult result = GBCM_ServerResult::OK();
    if (GB_GET_EXT_UPDATE_DATE(gbd)>client_clock) {
        if (GB_GET_EXT_CREATION_DATE(gbd) > client_clock) {
            result                  = gbcm_write_two(socket, GBCM_COMMAND_PUT_UPDATE_CREATE, (long)GB_FATHER(gbd));
            if (result.ok()) result = gbcm_write_GBDATA(socket, gbd, 0, 1);
        }
        else { // send clients first
            result = gbcm_write_two(socket, GBCM_COMMAND_PUT_UPDATE_UPDATE, (long)gbd);

            if (result.ok()) {
                if (GB_TYPE(gbd) == GB_DB) {
                    GBCONTAINER *gbc         = ((GBCONTAINER *)gbd);
                    int          end         = (int)gbc->d.nheader;
                    bool         send_header = (gbc->header_update_date > client_clock);

                    result = gbcm_write_GBDATA(socket, gbd, 0, send_header);

                    for (int index = 0; index < end && result.ok(); index++) {
                        GBDATA *gb2 = GBCONTAINER_ELEM(gbc, index);
                        if (gb2) result = gbcms_write_updated(socket, gb2, hsin, client_clock);
                    }
                }
                else {
                    result = gbcm_write_GBDATA(socket, gbd, 0, 0);
                }
            }
        }
    }
    return result;
}

__ATTR__USERESULT static GBCM_ServerResult gbcms_write_keys(int socket, GBDATA *gbd) {
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);

    WriteBuffer buf(GBCM_COMMAND_PUT_UPDATE_KEYS, (long)gbd);
    buf.put(Main->keycnt);
    buf.put(Main->first_free_key);

    GBCM_ServerResult result = buf.send_to(socket);

    for (int i=1; i<Main->keycnt && result.ok(); i++) {
        result                  = gbcm_write_two(socket, Main->keys[i].nref, Main->keys[i].next_free_key);
        if (result.ok()) result = gbcm_write_string(socket, Main->keys[i].key);
    }
    return result;
}

__ATTR__USERESULT inline GBCM_ServerResult gbcm_write_tagged(int socket, long tag, long value) {
    /*! write a 'tag'ged 'value' to 'socket'
     * value should be read with gbcm_read_tagged(.., tag, ..)
     */
    WriteBuffer buf(tag);
    buf.put(value);
    return buf.send_to(socket);
}

__ATTR__USERESULT static GBCM_ServerResult gbcm_read_tagged(int socket, long tag, long *value) {
    /*! read a 'tag'ged 'value' from 'socket'
     *  data has to be send by using gbcm_write_tagged(.., tag, ..)
     *  'value' may be a NULL ptr to ignore the read value
     */

    long buf[3];
    GBCM_ServerResult result = gbcm_read_expect_size(socket, (char *)&(buf[0]), sizeof(long)*3);
    if (result.ok()) {
        if (buf[0] != tag || buf[1] != 3) {
            result = GBCM_ServerResult::FAULT(GBS_global_string("received unexpected tag (expected=%lx/3, got=%lx/%lx)", tag, buf[0], buf[1]));
        }
        else if (value) {
            *value = buf[2];
        }
    }
    return result;
}

__ATTR__USERESULT static GBCM_ServerResult gbcms_talking_unfold(int socket, long */*hsin*/, void */*sin*/, GBDATA *gb_in) {
    // command: GBCM_COMMAND_UNFOLD
    GBCONTAINER       *gbc    = (GBCONTAINER *)gb_in;
    GBCM_ServerResult  result = gbcm_check_address(gbc);
    if (result.ok()) {
        if (GB_TYPE(gbc) != GB_DB) {
            result = GBCM_ServerResult::FAULT("can only unfold containers");
        }
    }

    long depth;
    long index_pos;
    if (result.ok()) result = gbcm_read_tagged(socket, GBCM_COMMAND_SETDEPTH, &depth);
    if (result.ok()) result = gbcm_read_tagged(socket, GBCM_COMMAND_SETINDEX, &index_pos);

    if (result.ok()) {
        gbcm_read_flush();
        if (index_pos==-2) {
            result                  = gbcm_write_GBDATA(socket, gb_in, depth+1, 1);
            if (result.ok()) result = gbcm_write_flush(socket);
        }
        else {
            int start;
            int end;
            if (index_pos >= 0) {
                start  = (int)index_pos;
                end    = start + 1;
                result = gbcm_write_tagged(socket, GBCM_COMMAND_SEND_COUNT, 1);
            }
            else {
                start  = 0;
                end    = gbc->d.nheader;
                result = gbcm_write_tagged(socket, GBCM_COMMAND_SEND_COUNT, gbc->d.size);
            }
            for (int index = start; index < end && result.ok(); index++) {
                GBDATA *gb2 = GBCONTAINER_ELEM(gbc, index);
                if (gb2) result = gbcm_write_GBDATA(socket, gb2, depth, 1);
            }

            if (result.ok()) result = gbcm_write_flush(socket);
        }
    }
    return result;
}

__ATTR__USERESULT static GBCM_ServerResult gbcms_talking_put_update(int socket, long */*hsin*/, void */*sin*/, GBDATA */*gbd*/) {
    /* Reads
     * - the date
     * - and all changed data
     * from client.
     *
     * command: GBCM_COMMAND_PUT_UPDATE
     */
    gbcms_create      *cs_main = 0;
    long              *buffer  = (long *)GB_give_buffer(1024);
    GBCM_ServerResult  result  = GBCM_ServerResult::OK();
    bool               end     = false;

    while (!end) {
        result = gbcm_read_expect_size(socket, (char *) buffer, sizeof(long) * 3);
        GBDATA *gbd;
        if (result.ok()) {
            gbd    = (GBDATA *) buffer[2];
            result = gbcm_check_address(gbd);
        }
        if (result.ok()) {
            switch (buffer[0]) {
                case GBCM_COMMAND_PUT_UPDATE_CREATE:
                    result = gbcm_read_GBDATA(socket, (GBCONTAINER *)gbd, buffer, 1, 0, &cs_main);
                    break;

                case GBCM_COMMAND_PUT_UPDATE_DELETE:
                    gb_delete_force(gbd);
                    break;

                case GBCM_COMMAND_PUT_UPDATE_UPDATE:
                    result = gbcm_read_GBDATA(socket, 0, buffer, 1, gbd, 0);
                    break;

                case GBCM_COMMAND_PUT_UPDATE_END:
                    end = true;
                    break;

                default:
                    result = GBCM_ServerResult::FAULT(GBS_global_string("invalid command byte %i", int(buffer[0])));
                    break;
            }
        }
    }


    if (result.ok()) {
        gbcm_read_flush();
        gbcms_create *cs = cs_main;
        while (cs && result.ok()) { // send all id's of newly created objects
            result = gbcm_write_two(socket, (long)cs->client_id, (long)cs->server_id);
            freeset(cs, cs->next);
        }
        if (result.ok()) result = gbcm_write_two(socket, 0, 0);
        if (result.ok()) result = gbcm_write_flush(socket);
    }
    return result;
}

__ATTR__USERESULT static GBCM_ServerResult gbcms_send_modified_data_to_client(GBDATA *gbd, int socket, long *hsin, void *sin) {
    GBCM_ServerResult result = GBCM_ServerResult::OK();
    GB_begin_transaction(gbd);
    while (gb_local->running_client_transaction == ARB_TRANS) {
        fd_set fset;
        FD_ZERO(&fset);
        FD_SET(socket, &fset);

        timeval timeout;
        timeout.tv_sec  = GBCMS_TRANSACTION_TIMEOUT;
        timeout.tv_usec = 0;

        long anz = select(FD_SETSIZE, &fset, NULL, NULL, &timeout);

        if (anz<0) continue;
        if (anz==0) {
            gb_local->running_client_transaction = ARB_ABORT;
            result = GBCM_ServerResult::FAULT(GBS_global_string("client transaction timeout (waited %lu seconds)", timeout.tv_sec));
        }
        else {
            result = gbcms_talking(socket, hsin, sin);
            if (result.failed()) {
                gb_local->running_client_transaction = ARB_ABORT;
            }
        }
    }
    if (gb_local->running_client_transaction == ARB_COMMIT) {
        GB_commit_transaction(gbd);
        gbcms_shift_delete_list(hsin, sin);
    }
    else {
        GB_abort_transaction(gbd);
    }
    return result;
}

__ATTR__USERESULT static GBCM_ServerResult gbcms_talking_init_transaction(int socket, long *hsin, void *sin, GBDATA */*gb_dummy*/) {
    /* begin client transaction
     * sends clock
     *
     * command: GBCM_COMMAND_INIT_TRANSACTION
     */

    Socinf         *si = (Socinf *)sin;
    gb_server_data *hs = (gb_server_data *)hsin;

    GBDATA       *gb_main = hs->gb_main;
    GB_MAIN_TYPE *Main    = GB_MAIN(gb_main);
    GBDATA       *gbd     = gb_main;

    GBCM_ServerResult  result = GBCM_ServerResult::OK();
    char              *user   = gbcm_read_string(socket, result);

    gbcm_read_flush();

    if (result.ok()) result = gbcm_login((GBCONTAINER *)gbd, user);
    if (result.ok()) {
        si->username = user;
        gb_local->running_client_transaction = ARB_TRANS;

        result                  = gbcm_write_tagged(socket, GBCM_COMMAND_TRANSACTION_RETURN, Main->clock);
        if (result.ok()) result = gbcm_write_tagged(socket, GBCM_COMMAND_TRANSACTION_RETURN, (long)gbd);
        if (result.ok()) result = gbcm_write_tagged(socket, GBCM_COMMAND_TRANSACTION_RETURN, (long)Main->this_user->userid);
        if (result.ok()) result = gbcms_write_keys(socket, gbd);
        if (result.ok()) result = gbcm_write_flush(socket);
    }

    if (result.ok()) result = gbcms_send_modified_data_to_client(gbd, socket, hsin, sin);
    return result;
}

__ATTR__USERESULT static GBCM_ServerResult gbcms_talking_begin_transaction(int socket, long *hsin, void *sin, GBDATA *long_client_clock) {
    /* begin client transaction
     * sends clock
     * deleted
     * created+updated
     *
     * command: GBCM_COMMAND_BEGIN_TRANSACTION
     */

    gb_server_data *hs      = (gb_server_data *)hsin;
    GBDATA         *gb_main = hs->gb_main;
    GBDATA         *gbd     = gb_main;

    gbcm_read_flush();
    gb_local->running_client_transaction = ARB_TRANS;

    GBCM_ServerResult result = gbcm_write_tagged(socket, GBCM_COMMAND_TRANSACTION_RETURN, GB_MAIN(gbd)->clock);

    if (result.ok()) {
        // send modified data to client
        long  client_clock = (long)long_client_clock;
        if (GB_MAIN(gb_main)->key_clock > client_clock) result = gbcms_write_keys(socket, gbd);

        if (result.ok()) result = gbcms_write_deleted(socket, (long)hs, client_clock);
        if (result.ok()) result = gbcms_write_updated(socket, gbd, (long)hs, client_clock);
        if (result.ok()) result = gbcm_write_two(socket, GBCM_COMMAND_PUT_UPDATE_END, 0);
        if (result.ok()) result = gbcm_write_flush(socket);
        if (result.ok()) result = gbcms_send_modified_data_to_client(gbd, socket, hsin, sin);
    }
    return result;
}

__ATTR__USERESULT static GBCM_ServerResult commit_or_abort_transaction(int socket, GBDATA *gbd, ARB_TRANS_TYPE commit_or_abort) {
    GBCM_ServerResult result = gbcm_check_address(gbd);
    if (result.ok()) {
        gb_local->running_client_transaction = commit_or_abort;
        gbcm_read_flush();

        result                  = gbcm_write_tagged(socket, GBCM_COMMAND_TRANSACTION_RETURN, 0);
        if (result.ok()) result = gbcm_write_flush(socket);
    }
    return result;
}
__ATTR__USERESULT static GBCM_ServerResult gbcms_talking_commit_transaction(int socket, long */*hsin*/, void */*sin*/, GBDATA *gbd) {
    // command: GBCM_COMMAND_COMMIT_TRANSACTION
    return commit_or_abort_transaction(socket, gbd, ARB_COMMIT);
}

__ATTR__USERESULT static GBCM_ServerResult gbcms_talking_abort_transaction(int socket, long */*hsin*/, void */*sin*/, GBDATA *gbd) {
    // command: GBCM_COMMAND_ABORT_TRANSACTION
    return commit_or_abort_transaction(socket, gbd, ARB_ABORT);
}

__ATTR__USERESULT static GBCM_ServerResult gbcms_talking_close(int /*socket*/, long */*hsin*/, void */*sin*/, GBDATA */*gbd*/) {
    // command: GBCM_COMMAND_CLOSE
    return GBCM_ServerResult::ABORTED();
}

__ATTR__USERESULT static GBCM_ServerResult gbcms_talking_undo(int socket, long */*hsin*/, void */*sin*/, GBDATA *gbd) {
    // command: GBCM_COMMAND_UNDO
    long cmd;
    GBCM_ServerResult sresult = gbcm_read_tagged(socket, GBCM_COMMAND_UNDO_CMD, &cmd);

    if (sresult.ok()) {
        GB_ERROR  result  = 0;
        char     *to_free = 0;

        gbcm_read_flush();
        switch (cmd) {
            case _GBCMC_UNDOCOM_REQUEST_NOUNDO:
                result = GB_request_undo_type(gbd, GB_UNDO_NONE);
                break;
            case _GBCMC_UNDOCOM_REQUEST_NOUNDO_KILL:
                result = GB_request_undo_type(gbd, GB_UNDO_KILL);
                break;
            case _GBCMC_UNDOCOM_REQUEST_UNDO:
                result = GB_request_undo_type(gbd, GB_UNDO_UNDO);
                break;
            case _GBCMC_UNDOCOM_INFO_UNDO:
                result = to_free = GB_undo_info(gbd, GB_UNDO_UNDO);
                break;
            case _GBCMC_UNDOCOM_INFO_REDO:
                result = to_free = GB_undo_info(gbd, GB_UNDO_REDO);
                break;
            case _GBCMC_UNDOCOM_UNDO:
                result = GB_undo(gbd, GB_UNDO_UNDO);
                break;
            case _GBCMC_UNDOCOM_REDO:
                result = GB_undo(gbd, GB_UNDO_REDO);
                break;
            default:
                result = GB_set_undo_mem(gbd, cmd);
                break;
        }
        
        sresult                   = gbcm_write_string(socket, result);
        if (sresult.ok()) sresult = gbcm_write_flush(socket);

        free(to_free);
    }
    return sresult;
}

__ATTR__USERESULT static GBCM_ServerResult gbcms_talking_find(int socket, long */*hsin*/, void */*sin*/, GBDATA * gbd) {
    // command: GBCM_COMMAND_FIND
    GBCM_ServerResult result = gbcm_check_address(gbd);
    if (result.ok()) {
        char           *key       = gbcm_read_string(socket, result);
        const GB_TYPES  type      = GB_TYPES(gbcm_read_long(socket, result));
        char           *str_val   = 0;
        GB_CASE         case_sens = GB_CASE_UNDEFINED;
        long            int_val   = 0;

        if (result.ok()) {
            switch (type) {
                case GB_NONE:
                    break;

                case GB_STRING:
                    str_val      = gbcm_read_string(socket, result);
                    case_sens = GB_CASE(gbcm_read_long(socket, result));
                    break;

                case GB_INT:
                    int_val = gbcm_read_long(socket, result);
                    break;

                default:
                    result = GBCM_ServerResult::FAULT(GBS_global_string("illegal data type (%i)", type));
                    break;
            }
        }

        if (result.ok()) {
            GB_SEARCH_TYPE gbs = GB_SEARCH_TYPE(gbcm_read_long(socket, result));
            gbcm_read_flush();

            if (result.ok()) {
                if (type == GB_NONE) {
                    gbd = GB_find(gbd, key, gbs);
                }
                else if (type == GB_STRING) {
                    gbd = GB_find_string(gbd, key, str_val, case_sens, gbs);
                    free(str_val);
                }
                else if (type == GB_INT) {
                    gbd = GB_find_int(gbd, key, int_val, gbs);
                }
                else {
                    result = GBCM_ServerResult::FAULT(GBS_global_string("Searching DBtype %i not implemented", type));
                }
            }
        }

        free(key);
    }
    if (result.ok()) {
        result = gbcm_write_tagged(socket, GBCM_COMMAND_FIND_ERG, (long) gbd);
        if (result.ok() && gbd) {
            while (result.ok() && GB_GRANDPA(gbd)) {
                result = gbcm_write_two(socket, gbd->index, (long)GB_FATHER(gbd));
                gbd    = (GBDATA *)GB_FATHER(gbd);
            }
        }
        if (result.ok()) result = gbcm_write_two(socket, 0, 0);
        if (result.ok()) result = gbcm_write_flush(socket);
    }
    return result;
}

__ATTR__USERESULT static GBCM_ServerResult gbcms_talking_key_alloc(int socket, long */*hsin*/, void */*sin*/, GBDATA * gbd) {
    // command: GBCM_COMMAND_KEY_ALLOC
    // (old maybe wrong comment: "do a query in the server")

    GBCM_ServerResult result = gbcm_check_address(gbd);
    if (result.ok()) {
        long index = 0;
        {
            char *key = gbcm_read_string(socket, result);
            gbcm_read_flush();

            if (key) {
                index = gb_create_key(GB_MAIN(gbd), key, false);
                free(key);
            }
        }

        if (result.ok()) result = gbcm_write_tagged(socket, GBCM_COMMAND_KEY_ALLOC_RES, index);
        if (result.ok()) result = gbcm_write_flush(socket);
    }
    return result;
}

__ATTR__USERESULT static GBCM_ServerResult gbcms_talking_disable_wait_for_new_request(int /*socket*/, long *hsin, void */*sin*/, GBDATA */*gbd*/) {
    // command GBCM_COMMAND_DONT_WAIT
    gb_server_data *hs = (gb_server_data *) hsin;
    hs->wait_for_new_request--;
    return GBCM_ServerResult::OK_WAIT();
}

__ATTR__USERESULT static GBCM_ServerResult gbcms_OBSOLETE_talking(int /*socket*/, long */*hsin*/, void */*sin*/, GBDATA */*gbd*/) {
    fputs("Obsolete server function called\n", stderr);
    return GBCM_ServerResult::FAULT("Obsolete server function called");
}

// -----------------------
//      server talking

typedef GBCM_ServerResult (*TalkingFunction)(int socket, long *hsin, void *sin, GBDATA *gbd);

static TalkingFunction aisc_talking_functions[] = {
    gbcms_talking_unfold,                           // GBCM_COMMAND_UNFOLD
    gbcms_OBSOLETE_talking,
    gbcms_talking_put_update,                       // GBCM_COMMAND_PUT_UPDATE
    gbcms_OBSOLETE_talking,
    gbcms_talking_begin_transaction,                // GBCM_COMMAND_BEGIN_TRANSACTION
    gbcms_talking_commit_transaction,               // GBCM_COMMAND_COMMIT_TRANSACTION
    gbcms_talking_abort_transaction,                // GBCM_COMMAND_ABORT_TRANSACTION
    gbcms_talking_init_transaction,                 // GBCM_COMMAND_INIT_TRANSACTION
    gbcms_talking_find,                             // GBCM_COMMAND_FIND
    gbcms_talking_close,                            // GBCM_COMMAND_CLOSE
    gbcms_OBSOLETE_talking,
    gbcms_talking_key_alloc,                        // GBCM_COMMAND_KEY_ALLOC
    gbcms_talking_undo,                             // GBCM_COMMAND_UNDO
    gbcms_talking_disable_wait_for_new_request      // GBCM_COMMAND_DONT_WAIT
};

__ATTR__USERESULT static GBCM_ServerResult gbcms_talking(int con, long *hs, void *sin) {
    GBCM_ServerResult result = GBCM_ServerResult::OK();

    gbcm_read_flush();

    do {
        long buf[3];
        result = gbcm_read_expect_size(con, (char *)buf, sizeof(long)*3);
        if (result.ok()) {
            long magic_number = buf[0];
            if ((magic_number & GBTUM_MAGIC_NUMBER_FILTER) != GBTUM_MAGIC_NUMBER) {
                gbcm_read_flush();
                result = GBCM_ServerResult::FAULT("illegal access");
            }
            else {
                magic_number &= ~GBTUM_MAGIC_NUMBER_FILTER;
                result        = (aisc_talking_functions[magic_number])(con, hs, sin, (GBDATA *)buf[2]);
            }
        }
    }
    while (result.get_code() == GBCM_SERVER_OK_WAIT);

    return result;
}

bool GBCMS_accept_calls(GBDATA *gbd, bool wait_extra_time) { // @@@ shall return GB_ERROR and ensure no error is exported
    // returns true if served

    GB_MAIN_TYPE   *Main     = GB_MAIN(gbd);
    long            in_trans = GB_read_transaction(gbd);

    if (!Main->server_data) return false;
    if (in_trans)           return false;

    gb_server_data *hs = Main->server_data;

    timeval timeout;
    if (wait_extra_time) {
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;
    }
    else {
        timeout.tv_sec = (int)(hs->timeout / 1000);
        timeout.tv_usec = (hs->timeout % 1000) * 1000;
    }
    
    if (wait_extra_time) {
        hs->wait_for_new_request = 1;
    }
    else {
        hs->wait_for_new_request = 0;
    }

    {
        fd_set fset_read, fset_except;

        FD_ZERO(&fset_read);
        FD_ZERO(&fset_except);
        FD_SET(hs->hso, &fset_read);
        FD_SET(hs->hso, &fset_except);

        for (Socinf *si = hs->soci; si; si=si->next) {
            FD_SET(si->socket, &fset_read);
            FD_SET(si->socket, &fset_except);
        }

        {
            long anz;
            if (hs->timeout>=0) {
                anz = select(FD_SETSIZE, &fset_read, NULL, &fset_except, &timeout);
            }
            else {
                anz = select(FD_SETSIZE, &fset_read, NULL, &fset_except, 0);
            }

            if (anz==-1) {
                return false;
            }
            if (!anz) { // timed out
                return false;
            }
        }

        if (FD_ISSET(hs->hso, &fset_read)) {
            int con = accept(hs->hso, NULL, 0);
            if (con>0) {
                Socinf *sptr = (Socinf *)GB_calloc(sizeof(Socinf), 1);
                if (!sptr) return 0;

                sptr->next   = hs->soci;
                sptr->socket = con;
                hs->soci     = sptr;
                hs->nsoc++;

                long optval = 1;
                setsockopt(con, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, 4);
            }
        }
        else {
            Socinf *si_last = 0;
            Socinf *sinext;
            Socinf *si;
            for (si=hs->soci; si; si_last=si, si=sinext) {
                sinext = si->next;

                GBCM_ServerResult result = GBCM_ServerResult::OK();
                if (FD_ISSET(si->socket, &fset_read)) {
                    result = gbcms_talking(si->socket, (long *)hs, (void *)si);
                    if (result.ok()) {
                        hs->wait_for_new_request++;
                        continue;
                    }
                }
                else if (!FD_ISSET(si->socket, &fset_except)) {
                    continue;
                }

                // kill socket
                if (close(si->socket)) {
                    printf("aisc_accept_calls: ");
                    printf("couldn't close socket errno = %i!\n", errno);
                }

                hs->nsoc--;
                if (si==hs->soci) { // first one
                    hs->soci = si->next;
                }
                else {
                    si_last->next = si->next;
                }
                if (si->username) {
                    gbcm_logout(Main, si->username);
                }
                g_bcms_delete_Socinf(si);
                si = 0;

                if (result.get_code() != GBCM_SERVER_ABORTED) {
                    fprintf(stdout, "ARB_DB_SERVER: a client died abnormally (Reason: %s)\n", result.get_error());
                }
                break;
            }
        }

    }
    if (hs->wait_for_new_request>0) {
        return true;
    }
    return false;
}

__ATTR__USERESULT inline GBCM_ServerResult cannot_use_in_server(GB_MAIN_TYPE *Main) {
    if (Main->local_mode) return GBCM_ServerResult::FAULT("invalid use in server");
    return GBCM_ServerResult::OK();
}

__ATTR__USERESULT inline GBCM_ServerResult gbcmc_get_server_socket(GBDATA *gbd, int& socket)  {
    GB_MAIN_TYPE      *Main   = GB_MAIN(gbd);
    GBCM_ServerResult  result = cannot_use_in_server(Main);

    socket = result.ok() ? Main->c_link->socket : -1;
    return result;
}

GB_ERROR gbcm_unfold_client(GBCONTAINER *gbd, long depth, long index_pos) {
    // goes to header: __ATTR__USERESULT

    /* read data from a server
     * depth       = -1   read whole data
     * depth       = 0...n    read to depth
     * index_pos == -1 read all clients
     * index_pos == -2 read all clients + header array
     */

    GB_ERROR error  = NULL;
    gbcm_read_flush();

    int socket; GBCM_ServerResult result = gbcmc_get_server_socket((GBDATA*)gbd, socket);

    if (result.ok()) result = gbcm_write_tagged(socket, GBCM_COMMAND_UNFOLD, gbd->server_id);
    if (result.ok()) result = gbcm_write_tagged(socket, GBCM_COMMAND_SETDEPTH, depth);
    if (result.ok()) result = gbcm_write_tagged(socket, GBCM_COMMAND_SETINDEX, index_pos);
    if (result.ok()) result = gbcm_write_flush(socket);

    if (result.ok()) {
        long buffer[256];
        if (index_pos == -2) {
            result = gbcm_read_GBDATA(socket, 0, buffer, 0, (GBDATA*)gbd, 0);
        }
        else {
            long nitems;
            result = gbcm_read_tagged(socket, GBCM_COMMAND_SEND_COUNT, &nitems);
            for (long item = 0; item<nitems && result.ok(); item++) {
                result = gbcm_read_GBDATA(socket, gbd, buffer, 0, 0, 0);
            }
        }
        gbcm_read_flush();
        if (result.ok() && index_pos < 0) gbd->flags2.folded_container = 0;
    }

    if (result.failed()) {
        error = GB_export_errorf("GB_unfold(%s): %s", GB_read_key_pntr((GBDATA*)gbd), result.get_error());
    }
    return error;
}

// -------------------------
//      Client functions

#if defined(WARN_TODO)
#warning rewrite gbcmc_... (error handling - do not export)
#endif

inline void sendupdate_annotate_error(GBCM_ServerResult& result, GBDATA *gbd, const char *where) {
    if (result.failed()) result.annotate(GBS_global_string("Error sending '%s' to server [%s]", GB_KEY(gbd), where));
}

GB_ERROR gbcmc_begin_sendupdate(GBDATA *gbd) {
    int socket; GBCM_ServerResult result = gbcmc_get_server_socket(gbd, socket);
    if (result.ok()) result = gbcm_write_tagged(socket, GBCM_COMMAND_PUT_UPDATE, gbd->server_id);
    sendupdate_annotate_error(result, gbd, "gbcmc_begin_sendupdate");
    return result.get_error();
}

GB_ERROR gbcmc_end_sendupdate(GBDATA *gbd) {
    int socket; GBCM_ServerResult result = gbcmc_get_server_socket(gbd, socket);
    if (result.ok()) result = gbcm_write_tagged(socket, GBCM_COMMAND_PUT_UPDATE_END, gbd->server_id);
    if (result.ok()) result = gbcm_write_flush(socket);

    while (result.ok()) {
        long buffer[2];
        result = gbcm_read_expect_size(socket, (char *)&(buffer[0]), sizeof(long)*2);
        if (result.ok()) {
            gbd = (GBDATA *)buffer[0];
            if (!gbd) break;

            gbd->server_id = buffer[1];
            GBS_write_numhash(GB_MAIN(gbd)->remote_hash, gbd->server_id, (long)gbd);
        }
    }
    gbcm_read_flush();
    sendupdate_annotate_error(result, gbd, "gbcmc_end_sendupdate");
    return result.get_error();
}

GB_ERROR gbcmc_sendupdate_create(GBDATA *gbd) {
    int socket; GBCM_ServerResult result = gbcmc_get_server_socket(gbd, socket);
    if (result.ok()) {
        GBCONTAINER *father = GB_FATHER(gbd);
        if (!father) {
            result = GBCM_ServerResult::FAULT("entry without father");
        }
        else {
            result                  = gbcm_write_tagged(socket, GBCM_COMMAND_PUT_UPDATE_CREATE, father->server_id);
            if (result.ok()) result = gbcm_write_GBDATA(socket, gbd, -1, 1);
        }
    }

    sendupdate_annotate_error(result, gbd, "gbcmc_sendupdate_create");
    return result.get_error();
}

GB_ERROR gbcmc_sendupdate_delete(GBDATA *gbd) {
    int socket; GBCM_ServerResult result = gbcmc_get_server_socket(gbd, socket);
    if (result.ok()) result = gbcm_write_tagged(socket, GBCM_COMMAND_PUT_UPDATE_DELETE, gbd->server_id);
    sendupdate_annotate_error(result, gbd, "gbcmc_sendupdate_delete");
    return result.get_error();
}

GB_ERROR gbcmc_sendupdate_update(GBDATA *gbd, int send_headera) {
    int socket; GBCM_ServerResult result       = gbcmc_get_server_socket(gbd, socket);
    if (result.ok() && !GB_FATHER(gbd)) result = GBCM_ServerResult::FAULT("entry without father");
    if (result.ok())  result                   = gbcm_write_tagged(socket, GBCM_COMMAND_PUT_UPDATE_UPDATE, gbd->server_id);
    if (result.ok()) result                    = gbcm_write_GBDATA(socket, gbd, 0, send_headera);
    sendupdate_annotate_error(result, gbd, "gbcmc_sendupdate_update");
    return result.get_error();
}

__ATTR__USERESULT static GBCM_ServerResult gbcmc_read_keys(int socket, GBDATA *gbd) {
    long buffer[2];
    GBCM_ServerResult result = gbcm_read_expect_size(socket, (char *)buffer, sizeof(long)*2);
    if (result.ok()) {
        GB_MAIN_TYPE *Main   = GB_MAIN(gbd);
        long          size   = buffer[0];
        Main->first_free_key = buffer[1];
        gb_create_key_array(Main, (int)size);

        for (int i=1; i<size && result.ok(); i++) {
            result = gbcm_read_expect_size(socket, (char *)buffer, sizeof(long)*2);
            if (result.ok()) {
                Main->keys[i].nref          = buffer[0];    // to control malloc_index
                Main->keys[i].next_free_key = buffer[1];    // to control malloc_index

                char *key = gbcm_read_string(socket, result);
                if (key) {
                    GBS_write_hash(Main->key_2_index_hash, key, i);
                    if (Main->keys[i].key) free (Main->keys[i].key);
                    Main->keys[i].key = key;
                }
            }
        }
        if (result.ok()) Main->keycnt = (int)size;
    }
    return result;
}

GB_ERROR gbcmc_begin_transaction(GBDATA *gbd) {
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    int socket; GBCM_ServerResult result = gbcmc_get_server_socket(gbd, socket);
    long clock;

    if (result.ok()) result = gbcm_write_tagged(socket, GBCM_COMMAND_BEGIN_TRANSACTION, Main->clock);
    if (result.ok()) result = gbcm_write_flush(socket);
    if (result.ok()) result = gbcm_read_tagged(socket, GBCM_COMMAND_TRANSACTION_RETURN, &clock);

    if (result.ok()) {
        Main->clock = clock;
        while (result.ok()) {
            long *buffer = (long *)GB_give_buffer(1026);

            result = gbcm_read_expect_size(socket, (char *)buffer, sizeof(long)*2);
            if (result.ok()) {
                GBDATA *gb2 = (GBDATA *)GBS_read_numhash(Main->remote_hash, buffer[1]);

                long mode;
                if (gb2) {
                    mode = gb2->flags2.folded_container
                        ? -1                                // read container
                        : 0;                                // read all
                }
                else {
                    mode = -2;                              // read nothing
                }

                switch (buffer[0]) {
                    case GBCM_COMMAND_PUT_UPDATE_UPDATE:
                        result = gbcm_read_GBDATA(socket, 0, buffer, mode, gb2, 0);
                        if (result.ok() && gb2) {
                            GB_CREATE_EXT(gb2);
                            gb2->ext->update_date = clock;
                        }
                        break;

                    case GBCM_COMMAND_PUT_UPDATE_CREATE:
                        result = gbcm_read_GBDATA(socket, (GBCONTAINER *)gb2, buffer, mode, 0, 0);
                        if (result.ok() && gb2) {
                            GB_CREATE_EXT(gb2);
                            gb2->ext->creation_date = gb2->ext->update_date = clock;
                        }
                        break;

                    case GBCM_COMMAND_PUT_UPDATE_DELETE:
                        if (gb2) gb_delete_entry(&gb2);
                        break;

                    case GBCM_COMMAND_PUT_UPDATE_KEYS:
                        result = gbcmc_read_keys(socket, gbd);
                        break;

                    case GBCM_COMMAND_PUT_UPDATE_END:
                        goto endof_gbcmc_begin_transaction;

                    default:
                        result = GBCM_ServerResult::FAULT(GBS_global_string("invalid command-code %i", int(buffer[0])));
                        break;
                }
            }
        }
    }
  endof_gbcmc_begin_transaction :
    gbcm_read_flush();
    sendupdate_annotate_error(result, gbd, "gbcmc_begin_transaction");
    return result.get_error();
}

GB_ERROR gbcmc_init_transaction(GBCONTAINER *gbd) {
    GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(gbd);
    int socket; GBCM_ServerResult result = gbcmc_get_server_socket((GBDATA*)gbd, socket);

    long clock;
    if (result.ok()) result = gbcm_write_tagged(socket, GBCM_COMMAND_INIT_TRANSACTION, Main->clock);
    if (result.ok()) result = gbcm_write_string(socket, Main->this_user->username);
    if (result.ok()) result = gbcm_write_flush(socket);
    if (result.ok()) result = gbcm_read_tagged(socket, GBCM_COMMAND_TRANSACTION_RETURN, &clock);
    if (result.ok()) {
        long buffer[4];
        Main->clock = clock;
        result      = gbcm_read_tagged(socket, GBCM_COMMAND_TRANSACTION_RETURN, buffer);
        if (result.ok()) {
            gbd->server_id = buffer[0];
            result         = gbcm_read_tagged(socket, GBCM_COMMAND_TRANSACTION_RETURN, buffer);
        }
        if (result.ok()) {
            Main->this_user->userid  = (int)buffer[0];
            Main->this_user->userbit = 1<<((int)buffer[0]);
            GBS_write_numhash(Main->remote_hash, gbd->server_id, (long)gbd);

            result = gbcm_read_expect_size(socket, (char *)buffer, 2*sizeof(long));
            if (result.ok()) result = gbcmc_read_keys(socket, (GBDATA *)gbd);
        }
    }

    gbcm_read_flush();
    sendupdate_annotate_error(result, (GBDATA*)gbd, "gbcmc_init_transaction");
    return result.get_error();
}

inline GB_ERROR gbcmc_end_transaction(GBDATA *gbd, long command) {
    int socket;
    GBCM_ServerResult result = gbcmc_get_server_socket(gbd, socket);
    if (result.ok()) result  = gbcm_write_tagged(socket, command, gbd->server_id);
    if (result.ok()) result  = gbcm_write_flush(socket);
    if (result.ok()) result  = gbcm_read_tagged(socket, GBCM_COMMAND_TRANSACTION_RETURN, NULL);

    gbcm_read_flush();
    sendupdate_annotate_error(result, gbd, "gbcmc_end_transaction");
    return result.get_error();
}
GB_ERROR gbcmc_commit_transaction(GBDATA *gbd) { return gbcmc_end_transaction(gbd, GBCM_COMMAND_COMMIT_TRANSACTION); }
GB_ERROR gbcmc_abort_transaction(GBDATA *gbd) { return gbcmc_end_transaction(gbd, GBCM_COMMAND_ABORT_TRANSACTION); }

GB_ERROR gbcms_add_to_delete_list(GBDATA *gbd) {
    GB_MAIN_TYPE   *Main = GB_MAIN(gbd);
    gb_server_data *hs   = Main->server_data;

    if (hs && hs->soci) {
        gbcms_delete_list *dl = (gbcms_delete_list *)gbm_get_mem(sizeof(gbcms_delete_list), GBM_CB_INDEX);

        dl->creation_date = GB_GET_EXT_CREATION_DATE(gbd);
        dl->update_date   = GB_GET_EXT_UPDATE_DATE(gbd);
        dl->gbd           = gbd;

        if (!hs->del_first) {
            hs->del_first = hs->del_last = dl;
        }
        else {
            hs->del_last->next = dl;
            hs->del_last = dl;
        }
    }
    return 0;
}

long GB_read_clients(GBDATA *gbd) {
    // returns number of clients or
    // -1 if not called from server

    GB_MAIN_TYPE *Main    = GB_MAIN(gbd);
    long          clients = -1;

    if (Main->local_mode) { // i am the server
        gb_server_data *hs = Main->server_data;
        clients = hs ? hs->nsoc : 0;
    }

    return clients;
}

bool GB_is_server(GBDATA *gbd) {
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    return Main->local_mode;
}
bool GB_is_client(GBDATA *gbd) {
    return !GB_is_server(gbd);
}

__ATTR__USERESULT static GBCM_ServerResult gbcmc_unfold_list(int socket, GBDATA * gbd) {
    GB_MAIN_TYPE      *Main   = GB_MAIN(gbd);
    long               readvar[2];
    GBCM_ServerResult  result = gbcm_read_expect_size(socket, (char *)readvar, sizeof(long)*2);
    if (result.ok()) {
        GBCONTAINER *gb_client = (GBCONTAINER *) readvar[1];
        if (gb_client) {
            result = gbcmc_unfold_list(socket, gbd);
            if (result.ok()) {
                gb_client      = (GBCONTAINER *) GBS_read_numhash(Main->remote_hash, (long) gb_client);
                GB_ERROR error = gb_unfold(gb_client, 0, (int)readvar[0]);
                if (error) result = GBCM_ServerResult::FAULT(error);
            }
        }
    }
    return result;
}

GBDATA *GBCMC_find(GBDATA *gbd, const char *key, GB_TYPES type, const char *str, GB_CASE case_sens, GB_SEARCH_TYPE gbs) {
    // perform search in DB server (from DB client)
    union {
        GBDATA *gbd;
        long    l;
    } found;

    int socket; GBCM_ServerResult result = gbcmc_get_server_socket(gbd, socket);
    if (result.ok()) {
        result                  = gbcm_write_tagged(socket, GBCM_COMMAND_FIND, gbd->server_id);
        if (result.ok()) result = gbcm_write_string(socket, key);
        if (result.ok()) result = gbcm_write_long(socket, type);
        if (result.ok()) {
            switch (type) {
                case GB_NONE:
                    break;

                case GB_STRING:
                    result                  = gbcm_write_string(socket, str);
                    if (result.ok()) result = gbcm_write_long(socket, case_sens);
                    break;

                case GB_INT:
                    result = gbcm_write_long(socket, *(long*)str);
                    break;

                default:
                    result = GBCM_ServerResult::FAULT(GBS_global_string("Illegal data type (%i)", type));
                    gb_assert(0);
                    break;
            }

            if (result.ok()) result = gbcm_write_long(socket, gbs);
            if (result.ok()) result = gbcm_write_flush(socket);
            if (result.ok()) result = gbcm_read_tagged(socket, GBCM_COMMAND_FIND_ERG, &found.l);

            if (found.gbd) {
                result  = gbcmc_unfold_list(socket, gbd);
                found.l = GBS_read_numhash(GB_MAIN(gbd)->remote_hash, found.l);
            }
        }
        gbcm_read_flush();
    }
    
    if (result.failed()) {
        sendupdate_annotate_error(result, gbd, "GBCMC_find");
        GB_export_error(result.get_error());
        GB_print_error();
        return NULL;
    }
    return found.gbd;
}


long gbcmc_key_alloc(GBDATA *gbd, const char *key) {
    int socket;
    GBCM_ServerResult result = gbcmc_get_server_socket(gbd, socket);
    if (result.ok()) result  = gbcm_write_tagged(socket, GBCM_COMMAND_KEY_ALLOC, gbd->server_id);
    if (result.ok()) result  = gbcm_write_string(socket, key);
    if (result.ok()) {
        result  = gbcm_write_flush(socket);
        long gb_result;
        if (result.ok()) result = gbcm_read_tagged(socket, GBCM_COMMAND_KEY_ALLOC_RES, &gb_result);
        gbcm_read_flush();
        if (result.ok()) return gb_result;
    }
    result.annotate("gbcmc_key_alloc");
    GB_export_error(result.get_error());
    GB_print_error();
    return 0;
}

__ATTR__USERESULT static GBCM_ServerResult gbcmc_send_undo(GBDATA *gbd, enum gb_undo_commands command, char *& answer) {
    answer = 0;
    int socket;
    GBCM_ServerResult result = gbcmc_get_server_socket(gbd, socket);
    if (result.ok()) result  = gbcm_write_tagged(socket, GBCM_COMMAND_UNDO, gbd->server_id);
    if (result.ok()) result  = gbcm_write_tagged(socket, GBCM_COMMAND_UNDO_CMD, command);
    if (result.ok()) result  = gbcm_write_flush(socket);
    if (result.ok()) answer  = gbcm_read_string(socket, result);
    gbcm_read_flush();
    return result;
}

GB_ERROR gbcmc_send_undo_commands(GBDATA *gbd, enum gb_undo_commands command) { // goes to header: __ATTR__USERESULT
    char              *answer;
    GBCM_ServerResult  result = gbcmc_send_undo(gbd, command, answer);
    if (result.ok() && answer) {
        result = GBCM_ServerResult::FAULT(GBS_global_string("server-error: %s", answer));
    }
    if (result.failed()) result.annotate("gbcmc_send_undo_commands");
    return result.get_error();
}

char *gbcmc_send_undo_info_commands(GBDATA *gbd, enum gb_undo_commands command) {
    char              *info;
    GBCM_ServerResult  result = gbcmc_send_undo(gbd, command, info);
    if (result.failed()) {
        result.annotate("gbcmc_send_undo_commands");
        GB_export_error(result.get_error());
    }
    return info;
}

GB_ERROR GB_tell_server_dont_wait(GBDATA *gbd) {
    // @@@ this is used in EDIT4 and DIST, but not in other clients
    int socket; GBCM_ServerResult result = gbcmc_get_server_socket(gbd, socket);
    if (result.ok()) result              = gbcm_write_tagged(socket, GBCM_COMMAND_DONT_WAIT, gbd->server_id);

    if (result.failed()) result.annotate("GB_tell_server_dont_wait");
    return result.get_error();
}

// ---------------------
//      Login/Logout


GBCM_ServerResult gbcm_login(GBCONTAINER *gb_main, const char *loginname) { // goes to header: __ATTR__USERESULT
    // look for any free user and set this_user
    GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(gb_main);

    for (int i = 0; i<GB_MAX_USERS; i++) {
        gb_user *user = Main->users[i];
        if (user && strcmp(loginname, user->username) == 0) {
            Main->this_user = user;
            user->nusers++;
            return GBCM_ServerResult::OK();
        }
    }
    for (int i = 0; i<GB_MAX_USERS; i++) {
        gb_user*& user = Main->users[i];
        if (!user) {
            user = (gb_user *) GB_calloc(sizeof(gb_user), 1);
            
            user->username = strdup(loginname);
            user->userid   = i;
            user->userbit  = 1<<i;
            user->nusers   = 1;

            Main->this_user = user;

            return GBCM_ServerResult::OK();
        }
    }
    return GBCM_ServerResult::FAULT(GBS_global_string("Too many users in this database: User '%s' ", loginname));
}

GBCM_ServerResult gbcmc_close(gbcmc_comm * link) { // goes to header: __ATTR__USERESULT
    GBCM_ServerResult result = GBCM_ServerResult::OK();
    if (link->socket) {
        result                  = gbcm_write_tagged(link->socket, GBCM_COMMAND_CLOSE, 0);
        if (result.ok()) result = gbcm_write_flush(link->socket);

        close(link->socket);
        link->socket = 0;
    }
    free(link->unix_name); 
    gbcmc_restore_sighandlers(link);
    free(link);
    if (result.failed()) result.annotate("gbcmc_close");
    return result;
}

GB_ERROR gbcm_logout(GB_MAIN_TYPE *Main, const char *loginname) {
    // if 'loginname' is NULL, the first logged-in user will be logged out

    if (!loginname) {
        loginname = Main->users[0]->username;
        gb_assert(loginname);
    }

    for (long i = 0; i<GB_MAX_USERS; i++) {
        gb_user*& user = Main->users[i];
        if (user && strcmp(loginname, user->username) == 0) {
            user->nusers--;
            if (user->nusers<=0) { // kill user and his projects
                if (i) fprintf(stdout, "User '%s' has logged out\n", loginname);
                free(user->username);
                freenull(user);
            }
            return 0;
        }
    }
    return GB_export_errorf("User '%s' not logged in", loginname);
}

GB_ERROR GB_install_pid(int mode) {
    /* tell the arb_clean script what programs are running.
     * mode == 1 -> install
     * mode == 0 -> never install
     */

    static long lastpid = -1;
    GB_ERROR    error   = 0;

    if (mode == 0) {
        gb_assert(lastpid == -1); // you have to call GB_install_pid(0) before opening any database!
        lastpid = -25;            // mark as "never install"
    }

    if (lastpid != -25) {
        long pid = getpid();

        if (pid != lastpid) {   // don't install pid multiple times
            char *pidfile_name;
            {
                const char *user    = GB_getenvUSER();
                const char *arb_pid = GB_getenv("ARB_PID"); // normally the pid of the 'arb' shell script

                gb_assert(user);
                if (!arb_pid) arb_pid = "";

                pidfile_name = GBS_global_string_copy("arb_pids_%s_%s", user, arb_pid);
            }

            char *pid_fullname;
            FILE *pidfile = GB_fopen_tempfile(pidfile_name, "at", &pid_fullname);

            if (!pidfile) {
                error = GBS_global_string("GB_install_pid: %s", GB_await_error());
            }
            else {
                fprintf(pidfile, "%li ", pid);
                lastpid = pid; // remember installed pid
                fclose(pidfile);
            }

            // ensure pid file is private, otherwise someone could inject PIDs which will be killed later
            gb_assert(GB_is_privatefile(pid_fullname, false));

            free(pid_fullname);
            free(pidfile_name);
        }
    }

    return error;
}

const char *GB_date_string() {
    timeval  date;

    gettimeofday(&date, 0);

    tm *p;
    {
#if defined(DARWIN)
        struct timespec local;
        TIMEVAL_TO_TIMESPEC(&date, &local); // not avail in time.h of Linux gcc 2.95.3
        p = localtime(&local.tv_sec);
#else
        p = localtime(&date.tv_sec);
#endif
    }

    char *readable = asctime(p); // points to a static buffer
    char *cr       = strchr(readable, '\n');
    gb_assert(cr);
    if (cr) cr[0]  = 0;          // cut of \n

    return readable;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

#include <sys/wait.h>
#include <arbdbt.h>

// test_com_server and test_com_server run in separate processes
struct test_com : virtual Noncopyable {
    char *socket;
    long  timeout;

    test_com() : socket(0), timeout(50) {}
    ~test_com() { free(socket); }

};

inline void sleep_ms(int ms) { GB_usleep(ms*1000); }

inline bool served(GBDATA *gb_main, bool wait) {
    GB_begin_transaction(gb_main);
    GB_commit_transaction(gb_main);
    return GBCMS_accept_calls(gb_main, wait);
}

inline void serve_all(GBDATA *gb_main) {
#if 1 // @@@ 
    // slow, but more coverage
    if (served(gb_main, false)) {
        while (served(gb_main, true)) {}
    }
#else
    // fast, but less coverage
    while (served(gb_main, false)) {}
#endif
}

inline int test_sync(GBDATA *gb_main, bool is_server) {
    int  val    = *GBT_read_int(gb_main, "sync");
    bool is_odd = val%2;

    if (is_server == is_odd) { // server handles odd, client even values
        if (is_server) {
            if (val) {
                GBT_write_int(gb_main, "trigger_create_update", 7); // triggers some special updates
            }
        }
        GBT_write_int(gb_main, "sync", ++val);
        if (!is_server) { // wait for server doing his job
            TEST_ASSERT_NO_ERROR(GB_tell_server_dont_wait(gb_main));
            while (*GBT_read_int(gb_main, "sync") == val) {
                TEST_ASSERT_NO_ERROR(GB_tell_server_dont_wait(gb_main));
                sleep_ms(50);
                TEST_ASSERT_NO_ERROR(GB_tell_server_dont_wait(gb_main));
            }
            val++;
        }
    }
    else {
        val = 0;
    }
    return val;
}

#define CONTNUMBER      "cont/number"
#define NOTUSEDBYCLIENT "serveronly/value"
#define DELETEDBYCLIENT "cont/clientdel"

#define TEST_ASSERT_HAVE_INT(gb_main,path,value) do {           \
        long *fieldExists = GBT_read_int(gb_main, path);        \
        TEST_ASSERT(fieldExists);                               \
        TEST_ASSERT_EQUAL(*fieldExists, value);                 \
    } while(0)

static void test_com_server(const test_com& tcom, bool wait_for_HUP) {
    GB_shell shell;
    GBDATA   *gb_main = GB_open("nosuch.arb", "crw");
    GB_ERROR  error   = NULL;

    if (!gb_main) {
        error = GB_await_error();
    }
    else {
        error = GBCMS_open(tcom.socket, tcom.timeout, gb_main);

        if (!error) { // prepare some data
            TEST_ASSERT(GB_is_server(gb_main));

            TEST_ASSERT_CONTAINS(GBCMS_open(tcom.socket, tcom.timeout, gb_main),
                                 "reopen of server not allowed"); // 2nd open on socket shall fail

            GB_transaction ta(gb_main);
            TEST_ASSERT_NO_ERROR(GBT_write_string(gb_main, "text", "bla"));
            TEST_ASSERT_NO_ERROR(GBT_write_string(gb_main, NOTUSEDBYCLIENT, "yes"));
            TEST_ASSERT_NO_ERROR(GBT_write_string(gb_main, DELETEDBYCLIENT, "deleteme"));
            TEST_ASSERT_NO_ERROR(GBT_write_int(gb_main, CONTNUMBER, 0x4711));

            GBDATA *gb_cont = GB_entry(gb_main, "cont");
            TEST_ASSERT(gb_cont);

            {
                GBDATA *gb_entry = GB_create(gb_cont, "entry", GB_STRING); TEST_ASSERT_NO_ERROR(GB_write_string(gb_entry, "super"));
                gb_entry         = GB_create(gb_cont, "entry", GB_STRING); TEST_ASSERT_NO_ERROR(GB_write_string(gb_entry, "hyper"));
            }

            GBDATA *gb_unsaved = GB_create_container(gb_main, "unsaved");
            TEST_ASSERT(gb_unsaved);

            for (int i = 0; i<GB_MAX_LOCAL_SEARCH+10; ++i) { // need more than GB_MAX_LOCAL_SEARCH entries to force serverside search
                GBDATA *gb_sub   = GB_create_container(gb_unsaved, "sub");
                TEST_ASSERT(gb_sub);

                GBDATA *gb_id  = GB_create(gb_sub, "id", GB_STRING);
                GBDATA *gb_nid = GB_create(gb_sub, "nid", GB_INT);
                TEST_ASSERT(gb_id && gb_nid);
                TEST_ASSERT_NO_ERROR(GB_write_string(gb_id, GBS_global_string("<%i>", i)));
                TEST_ASSERT_NO_ERROR(GB_write_int(gb_nid, i));

                if (i == 99) TEST_ASSERT_NO_ERROR(GBT_write_string(gb_sub, "one", "unique"));
            }
        }

        if (!error) {
            // serve the client
            int clients = 0;

            GBT_write_int(gb_main, "sync", 0);

            while (!clients) { served(gb_main, false); clients = GB_read_clients(gb_main); } // wait for client
            while (clients) { // serve client until he disconnects
                serve_all(gb_main);
                int sync_pos = test_sync(gb_main, true);
                switch (sync_pos) { // sync to server state (see SYNC_TO_SERVER_STATE)
                    case 2: {
                        TEST_ASSERT_NO_ERROR(GBT_write_int(gb_main, CONTNUMBER, 0x4712));
                        TEST_ASSERT_HAVE_INT(gb_main, CONTNUMBER, 0x4712);
                        break;
                    }
                    case 4: {
                        TEST_ASSERT_HAVE_INT(gb_main, CONTNUMBER, 0x4713);
                        TEST_ASSERT_NO_ERROR(GBT_write_int(gb_main, CONTNUMBER, 0x4714));
                        TEST_ASSERT_HAVE_INT(gb_main, CONTNUMBER, 0x4714);
                        TEST_ASSERT_NO_ERROR(GBT_write_string(gb_main, NOTUSEDBYCLIENT, "no"));
                        break;
                    }
                    case 6: {
                        GB_transaction ta(gb_main);
                        TEST_ASSERT_NO_ERROR(GB_delete(GB_search(gb_main, CONTNUMBER, GB_FIND)));
                        break;
                    }
                    case 8: { // final sync (clean up to keep panix.arb small)
                        GB_transaction ta(gb_main);
                        TEST_ASSERT_NO_ERROR(GB_delete(GB_search(gb_main, "unsaved", GB_FIND)));
                        break;
                    }
                    default: TEST_ASSERT_EQUAL(0, sync_pos);
                }
                clients = GB_read_clients(gb_main);
            }

            // check data created by client
            TEST_ASSERT_HAVE_INT(gb_main, "container/value", 0x0815);
            {
                GB_transaction ta(gb_main);
                TEST_ASSERT(!GB_search(gb_main, "container/undone", GB_FIND));
            }
        }

        if (wait_for_HUP) {
            while (!GB_is_regularfile("panix.arb")) { 
                sleep_ms(100);
            }
        }

        GBCMS_shutdown(gb_main);
        GB_close(gb_main);
    }

    TEST_ASSERT_NO_ERROR(error);
}


static GBDATA *connect_to_server(const test_com& tcom) {
    GBDATA *gb_main = NULL;
    int     maxwait = 3*1000;
    int     mywait  = 0;
    while (!gb_main && mywait<maxwait) {
        gb_main  = GB_open(tcom.socket, "rw");;
        if (!gb_main) {
            // not reached if server was faster
            TEST_ASSERT_CONTAINS(GB_await_error(), "There is no ARBDB server");
            int w    = 30;
            sleep_ms(w);
            mywait  += w;
        }
    }
    return gb_main;
}

#define SYNC_TO_SERVER_STATE(val) TEST_ASSERT_EQUAL(test_sync(gb_main, false), val)

static void test_com_client(const test_com& tcom) {
    GB_shell  shell;
    GBDATA   *gb_main = connect_to_server(tcom);
    GB_ERROR  error   = NULL;

    if (!gb_main) {
        error = GB_await_error();
    }
    else {
        TEST_ASSERT(GB_is_client(gb_main));

        // check data created by server
        {
            GB_transaction  ta(gb_main);
            GBDATA         *gb_cont = GB_entry(gb_main, "cont");
            TEST_ASSERT(gb_cont);

            GBDATA *gb_found = GB_find_string(gb_cont,  "entry", "super", GB_MIND_CASE, SEARCH_CHILD);   TEST_ASSERT(gb_found);
            gb_found         = GB_find_string(gb_found, "entry", "hyper", GB_MIND_CASE, SEARCH_BROTHER); TEST_ASSERT(gb_found);

            GBDATA *gb_unsaved = GB_entry(gb_main, "unsaved");
            TEST_ASSERT(gb_unsaved);

            gb_found = GB_find_string(gb_unsaved, "id",  "<141>", GB_MIND_CASE, SEARCH_GRANDCHILD); TEST_ASSERT(gb_found);
            gb_found = GB_find_int   (gb_unsaved, "nid", 201,                   SEARCH_GRANDCHILD); TEST_ASSERT(gb_found);
            gb_found = GB_find       (gb_unsaved, "one",                        SEARCH_GRANDCHILD); TEST_ASSERT(gb_found);
        }


        TEST_ASSERT_EQUAL(GBT_read_char_pntr(gb_main, "text"), "bla");
        TEST_ASSERT_EQUAL(GBT_read_char_pntr(gb_main, "cont/entry"), "super");
        TEST_ASSERT_HAVE_INT(gb_main, CONTNUMBER, 0x4711);

        SYNC_TO_SERVER_STATE(2);

        TEST_ASSERT_HAVE_INT(gb_main, CONTNUMBER, 0x4712); // updated to new value by server
        { // start and abort transaction
            TEST_ASSERT_NO_ERROR(GB_begin_transaction(gb_main));
            TEST_ASSERT_RESULT__NOERROREXPORTED(GB_create(gb_main, "aborted_string", GB_STRING));
            TEST_ASSERT_RESULT__NOERROREXPORTED(GB_create(gb_main, "aborted_int", GB_INT));
            TEST_ASSERT_NO_ERROR(GB_abort_transaction(gb_main));
        }
        TEST_ASSERT_NO_ERROR(GBT_write_int(gb_main, CONTNUMBER, 0x4713));

        { // delete data from server
            GB_transaction ta(gb_main);
            TEST_ASSERT_NO_ERROR(GB_delete(GB_search(gb_main, DELETEDBYCLIENT, GB_FIND)));
        }

        SYNC_TO_SERVER_STATE(4);

        TEST_ASSERT_HAVE_INT(gb_main, CONTNUMBER, 0x4714); // updated to new value by server

        // create some new data, then undo it
        TEST_ASSERT_NO_ERROR(GBT_write_int(gb_main, "container/value", 0x0815));
        TEST_ASSERT_NO_ERROR(GB_request_undo_type(gb_main, GB_UNDO_UNDO));
        TEST_ASSERT_NO_ERROR(GBT_write_string(gb_main, "container/undone", "17+4"));
        {
            char *uinfo = GB_undo_info(gb_main, GB_UNDO_UNDO);
            TEST_ASSERT_EQUAL(uinfo, "Delete new entry: undone\nUndo modified entry: container\nDelete new entry: @key\nUndo modified entry: @key_data\n");
            free(uinfo);
        }
        TEST_ASSERT_NO_ERROR(GB_undo(gb_main, GB_UNDO_UNDO));

        SYNC_TO_SERVER_STATE(6);

        TEST_ASSERT(GBT_read_int(gb_main, CONTNUMBER) == NULL); // deleted by server 

        TEST_ASSERT_CONTAINS(GBCMS_open(tcom.socket, tcom.timeout, gb_main), // open a server on used socket
                             "TEST_DB_com_interface.socket' already in use");

        SYNC_TO_SERVER_STATE(8);
        
        GB_close(gb_main);
    }
    TEST_ASSERT_NO_ERROR(error);
}

inline void test_com_interface(bool as_client, bool as_child, const test_com& tcom) {
    as_client
        ? test_com_client(tcom)
        : test_com_server(tcom, as_child); // as_child wait_for_HUP
}

void TEST_SLOW_DB_com_interface() {
    int  parent_as_client_in_loop = GB_random(2); // should not matter

    test_com tcom;
    tcom.socket = GBS_global_string_copy(":%s/UNIT_TESTER/sockets/TEST_DB_com_interface.socket", GB_getenvARBHOME());

    for (int loop = 0; loop <= 1; ++loop) {   // test once as client, once as server (to get full coverage of both sides)
        bool parent_as_client = parent_as_client_in_loop == loop;
        bool child_as_client  = !parent_as_client;

        pid_t child_pid = TEST_FORK();

        if (child_pid) { // parent
            TEST_ANNOTATE_ASSERT(GBS_global_string("parent(%s) in loop %i", parent_as_client ? "client" : "server", loop));
            test_com_interface(parent_as_client, false, tcom);

            const char *panic_arb          = "panix.arb";
            const char *panic_arb_expected = "panix.arb.expected";
            if (parent_as_client) {
#if 1
                kill(child_pid, SIGHUP); // HUP w/o panic save
                sleep_ms(200);           // let server interrupt
#endif

                // send HUP (server should panic-save)
                FILE *panic_fp = GB_fopen_tempfile(panic_file_name(), "wt", NULL);
                fprintf(panic_fp, "%s\n", panic_arb);
                fclose(panic_fp);

                kill(child_pid, SIGHUP);
            }

            while (child_pid != wait(NULL)) {}

            if (parent_as_client) { // test server has panic-saved database
// #define TEST_AUTO_UPDATE
#if defined(TEST_AUTO_UPDATE)
                TEST_COPY_FILE(panic_arb, panic_arb_expected);
#else // !defined(TEST_AUTO_UPDATE)
                TEST_ASSERT_TEXTFILES_EQUAL(panic_arb, panic_arb_expected);
                TEST_ASSERT_ZERO_OR_SHOW_ERRNO(GB_unlink(panic_arb));
#endif
            }
        }
        else { // child
            TEST_ANNOTATE_ASSERT(GBS_global_string("child(%s) in loop %i", child_as_client ? "client" : "server", loop));
            test_com_interface(child_as_client, true, tcom);
            EXIT_CHILD();
        }
    }
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
