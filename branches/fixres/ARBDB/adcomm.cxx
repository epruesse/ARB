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
#include <cerrno>

#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include "gb_key.h"
#include "gb_comm.h"
#include "gb_localdata.h"

#include <SigHandler.h>
#include <arb_signal.h>
#include <arb_file.h>

static GBCM_ServerResult gbcms_talking(int con, long *hs, void *sin);


#define FD_SET_TYPE

#define debug_printf(a, b)

#define GBCMS_TRANSACTION_TIMEOUT 60*60             // one hour timeout
#define MAX_QUEUE_LEN             5

#define GBCM_COMMAND_UNFOLD             (GBTUM_MAGIC_NUMBER)
#define GBCM_COMMAND_GET_UPDATA         (GBTUM_MAGIC_NUMBER+1)
#define GBCM_COMMAND_PUT_UPDATE         (GBTUM_MAGIC_NUMBER+2)
#define GBCM_COMMAND_UPDATED            (GBTUM_MAGIC_NUMBER+3)
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
#define GBCM_COMMAND_SETDEEP            (GBTUM_MAGIC_NUMBER+0x3000)
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

// --------------------------
//      error generators

inline GB_ERROR clientserver_error(const char *clientserver, const char *what_failed, int sourceLine) {
    const char *rev_tag = GB_get_arb_revision_tag();
    return GBS_global_string("ARBDB %s error: %s (errcode=%s#%i)", clientserver, what_failed, rev_tag, sourceLine);
}

#define CLIENT_ERROR(reason)        clientserver_error("client", reason, __LINE__)
#define SERVER_ERROR(reason)        clientserver_error("server", reason, __LINE__)
#define COMM_ERROR(reason)          clientserver_error("communication", reason, __LINE__)

#define CLIENT_RECEIVE_ERROR() CLIENT_ERROR("receive failed")
#define SERVER_RECEIVE_ERROR() SERVER_ERROR("receive failed")
#define COMM_RECEIVE_ERROR()   COMM_ERROR("receive failed")

#define CLIENT_SEND_ERROR() CLIENT_ERROR("send failed")
#define SERVER_SEND_ERROR() SERVER_ERROR("send failed")
#define COMM_SEND_ERROR()   COMM_ERROR("send failed")

#define CLIENT_SEND_ERROR_AT_ITEM(gb_item) CLIENT_ERROR(GBS_global_string("send failed (entry='%s')", GB_KEY(gb_item)))

static void dumpError(GB_ERROR error) {
    fputc('\n', stderr);
    fputs(error, stderr);
    fputc('\n', stderr);
    fflush(stderr);
}

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

static void g_bcms_delete_Socinf(Socinf *THIS) {
    freenull(THIS->username);
    THIS->next = 0;
    free(THIS);
}

struct gb_server_data {
    int     hso;
    char   *unix_name;
    Socinf *soci;
    long    nsoc;
    long    timeout;
    GBDATA *gb_main;
    int     wait_for_new_request;
    bool    inside_remote_action;

    gbcms_delete_list *del_first; // All deleted items, that are yet unknown to at least one client
    gbcms_delete_list *del_last;
};



struct gbcms_create {
    gbcms_create *next;
    GBDATA       *server_id;
    GBDATA       *client_id;
};


// --------------------
//      Panic save

static GBCONTAINER *gbcms_gb_main;

static void gbcms_sighup(int) {
    char *panic_file = 0;                      // hang-up trigger file
    char *db_panic   = 0;                      // file to save DB to
    {
        const char *ap = GB_getenv("ARB_PID");
        if (!ap) ap    = "";

        FILE *in = GB_fopen_tempfile(GBS_global_string("arb_panic_%s_%s", GB_getenvUSER(), ap), "rt", &panic_file);

        fprintf(stderr,
                "**** ARB DATABASE SERVER received a HANGUP SIGNAL ****\n"
                "- Looking for file '%s'\n",
                panic_file);

        db_panic = GB_read_fp(in);
        fclose(in);
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

        fprintf(stderr, "- Trying to save DATABASE in ASCII mode into file '%s'\n", db_panic);

        GB_ERROR error = GBCONTAINER_MAIN(gbcms_gb_main)->panic_save(db_panic);

        if (error) fprintf(stderr, "Error while saving '%s': %s\n", db_panic, error);
        else fprintf(stderr, "- DATABASE saved into '%s' (ASCII)\n", db_panic);

        unlink(panic_file);
        free(db_panic);
    }
}

GB_ERROR GB_MAIN_TYPE::panic_save(const char *db_panic) {
    const int org_transaction_level = transaction_level;

    transaction_level = 0;
    GB_ERROR error    = save_as(db_panic, "az"); // attempt zipped first
    if (error) error  = save_as(db_panic, "a");  // fallback to plain ascii on failure
    transaction_level = org_transaction_level;

    return error;
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
            gbcmc_close(comm); // ignore result
        }
        else {
            int   socket;
            char *unix_name = NULL;

            error = gbcm_open_socket(path, false, &socket, &unix_name);
            if (!error) {
                ASSERT_RESULT(SigHandler, SIG_DFL,                       INSTALL_SIGHANDLER(SIGHUP, gbcms_sighup, "GBCMS_open"));

                gbcms_gb_main = gb_main->as_container();

                if (listen(socket, MAX_QUEUE_LEN) < 0) {
                    error = GBS_global_string("could not listen (server; errno=%i)", errno);
                }
                else {
                    gb_server_data *hs = (gb_server_data *)ARB_calloc(sizeof(gb_server_data), 1);

                    hs->timeout   = timeout;
                    hs->gb_main   = gb_main;
                    hs->hso       = socket;
                    hs->unix_name = unix_name;

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
        Socinf         *si;

        for (si=hs->soci; si; si=si->next) {
            shutdown(si->socket, SHUT_RDWR);
            close(si->socket);
        }
        shutdown(hs->hso, SHUT_RDWR);

        if (hs->unix_name) {
            unlink(hs->unix_name);
            freenull(hs->unix_name);
        }
        close(hs->hso);
        freenull(Main->server_data);
    }
}

template<typename T>
inline void write_into_comm_buffer(long& buffer, T& t) {
    STATIC_ASSERT(sizeof(t) <= sizeof(long));

    buffer         = 0; // initialize (avoid to write uninitialized byte to avoid valgrind errors)
    *(T*)(&buffer) = t;
}

static __ATTR__USERESULT GB_ERROR gbcm_write_bin(int socket, GBDATA *gbd, long *buffer, long deep, int send_headera) {
    /* send a database item to client/server
      *
      * mode    =1 server
      *         =0 client
      * buffer = buffer
      * deep = 0 -> just send one item   >0 send sub entries too
      * send_headera = 1 -> if type = GB_DB send flag and key_quark array
      */

    buffer[0] = GBCM_COMMAND_SEND;

    long i      = 2;
    buffer[i++] = (long)gbd;
    buffer[i++] = gbd->index;

    write_into_comm_buffer(buffer[i++], gbd->flags);

    if (gbd->is_container()) {
        GBCONTAINER *gbc = gbd->as_container();
        int          end = gbc->d.nheader;

        write_into_comm_buffer(buffer[i++], gbc->flags3);

        buffer[i++] = send_headera ? end : -1;
        buffer[i++] = deep ? gbc->d.size : -1;
        buffer[1]   = i;

        if (gbcm_write(socket, (const char *)buffer, i* sizeof(long))) {
            return COMM_SEND_ERROR();
        }

        if (send_headera) {
            gb_header_list  *hdl  = GB_DATA_LIST_HEADER(gbc->d);
            gb_header_flags *buf2 = (gb_header_flags *)GB_give_buffer2(gbc->d.nheader * sizeof(gb_header_flags));

            for (int index = 0; index < end; index++) {
                buf2[index] = hdl[index].flags;
            }
            if (gbcm_write(socket, (const char *)buf2, end * sizeof(gb_header_flags))) {
                return COMM_SEND_ERROR();
            }
        }

        if (deep) {
            debug_printf("%i    ", gbc->d.size);

            for (int index = 0; index < end; index++) {
                GBDATA *gb2 = GBCONTAINER_ELEM(gbc, index);
                if (gb2) {
                    debug_printf("%i ", index);
                    GB_ERROR error = gbcm_write_bin(socket, gb2, (long *)buffer, deep-1, send_headera);
                    if (error) return error;
                }
            }
            debug_printf("\n", 0);
        }
    }
    else {
        GBENTRY *gbe = gbd->as_entry();
        if (gbe->type() < GB_BITS) {
            buffer[i++] = gbe->info.i;
            buffer[1] = i;
            if (gbcm_write(socket, (const char *)buffer, i*sizeof(long))) {
                return COMM_SEND_ERROR();
            }
        }
        else {
            long memsize;
            buffer[i++] = gbe->size();
            memsize = buffer[i++] = gbe->memsize();
            buffer[1] = i;
            if (gbcm_write(socket, (const char *)buffer, i* sizeof(long))) {
                return COMM_SEND_ERROR();
            }
            if (gbcm_write(socket, gbe->data(), memsize)) {
                return COMM_SEND_ERROR();
            }
        }
    }
    return 0;
}

#define RETURN_SERVER_FAULT_ON_BAD_ADDRESS(ptr)                         \
    do {                                                                \
        GB_ERROR error = GBK_test_address((long*)(ptr), GBTUM_MAGIC_NUMBER); \
        if (error) {                                                    \
            GB_warningf("%s (%s, #%i)", error, __FILE__, __LINE__);     \
            return GBCM_SERVER_FAULT;                                   \
        }                                                               \
    } while (0)

static GBCM_ServerResult gbcm_read_bin(int socket, GBCONTAINER *gbc, long *buffer, long mode, GBDATA *gb_source, void *cs_main) {
    /* read an entry into gbc
     * mode == 1  server reads data
     * mode == 0  client read all data
     * mode == -1  client read but do not read subobjects -> folded cont
     * mode == -2  client dummy read
     */

    long size = gbcm_read(socket, (char *)buffer, sizeof(long) * 3);
    if (size != sizeof(long) * 3) {
        fprintf(stderr, "receive failed header size\n");
        return GBCM_SERVER_FAULT;
    }
    if (buffer[0] != GBCM_COMMAND_SEND) {
        fprintf(stderr, "receive failed wrong command\n");
        return GBCM_SERVER_FAULT;
    }

    long id = buffer[2];
    long i  = buffer[1];
    i       = sizeof(long) * (i - 3);

    size = gbcm_read(socket, (char *)buffer, i);
    if (size != i) {
        GB_internal_error("receive failed DB_NODE\n");
        return GBCM_SERVER_FAULT;
    }

    i              = 0;
    long index_pos = buffer[i++];
    if (!gb_source && gbc && index_pos<gbc->d.nheader) {
        gb_source = GBCONTAINER_ELEM(gbc, index_pos);
    }

    gb_flag_types flags = *(gb_flag_types *)(&buffer[i++]);
    GB_TYPES      type  = GB_TYPES(flags.type);

    GBDATA *gb2;
    if (mode >= -1) {   // real read data
        if (gb_source) {
            GB_TYPES stype = gb_source->type();
            gb2 = gb_source;
            if (stype != type) {
                GB_internal_error("Type changed in client: Connection aborted\n");
                return GBCM_SERVER_FAULT;
            }
            if (mode>0) {   // transactions only in server
                RETURN_SERVER_FAULT_ON_BAD_ADDRESS(gb2);
            }
            if (stype != GB_DB) {
                gb_save_extern_data_in_ts(gb2->as_entry());
            }
            gb_touch_entry(gb2, GB_NORMAL_CHANGE);
        }
        else {
            if (mode==-1) goto dont_create_in_a_folded_container;
            if (type == GB_DB) {
                gb2 = gb_make_container(gbc, 0, index_pos, GB_DATA_LIST_HEADER(gbc->d)[index_pos].flags.key_quark);
            }
            else {  // @@@ Header Transaction stimmt nicht
                gb2 = gb_make_entry(gbc, 0, index_pos, GB_DATA_LIST_HEADER(gbc->d)[index_pos].flags.key_quark, (GB_TYPES)type);
            }
            if (mode>0) {   // transaction only in server
                gb_touch_entry(gb2, GB_CREATED);
            }
            else {
                gb2->server_id = id;
                GBS_write_numhash(GB_MAIN(gb2)->remote_hash, id, (long) gb2);
            }
            if (cs_main) {
                gbcms_create *cs;
                cs = (gbcms_create *)ARB_calloc(sizeof(gbcms_create), 1);
                cs->next = *((gbcms_create **) cs_main);
                *((gbcms_create **) cs_main) = cs;
                cs->server_id = gb2;
                cs->client_id = (GBDATA *)id;
            }
        }
        gb2->flags = flags;
        if (type == GB_DB) {
            gb2->as_container()->flags3 = *((gb_flag_types3 *)&(buffer[i++]));
        }
    }
    else {
      dont_create_in_a_folded_container :
        if (type == GB_DB) {
            // gb_flag_types3 flags3 = *((gb_flag_types3 *)&(buffer[i++]));
            ++i;
        }
        gb2 = 0;
    }

    if (type == GB_DB) {
        long nheader = buffer[i++];
        long nitems  = buffer[i++];

        if (nheader > 0) {
            long             realsize = nheader* sizeof(gb_header_flags);
            gb_header_flags *buffer2  = (gb_header_flags *)GB_give_buffer2(realsize);

            size = gbcm_read(socket, (char *)buffer2, realsize);
            if (size != realsize) {
                GB_internal_error("receive failed data\n");
                return GBCM_SERVER_FAULT;
            }
            if (gb2 && mode >= -1) {
                GBCONTAINER  *gbc2 = gb2->as_container();
                GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(gbc2);

                gb_create_header_array(gbc2, (int)nheader);
                if (nheader < gbc2->d.nheader) {
                    GB_internal_error("Inconsistency Client-Server Cache");
                }
                gbc2->d.nheader = (int)nheader;
                gb_header_list *hdl = GB_DATA_LIST_HEADER(gbc2->d);
                for (long item = 0; item < nheader; item++) {
                    GBQUARK old_index = hdl->flags.key_quark;
                    GBQUARK new_index = buffer2->key_quark;
                    if (new_index && !old_index) {  // a rename ...
                        gb_write_index_key(gbc2, item, new_index);
                    }
                    if (mode>0) {   // server read data


                    }
                    else {
                        if (buffer2->changed >= GB_DELETED) {
                            hdl->flags.set_change(GB_DELETED);
                        }
                    }
                    hdl->flags.flags = buffer2->flags;
                    hdl++; buffer2++;
                }
                if (mode>0) {   // transaction only in server
                    gb_touch_header(gbc2);
                }
                else {
                    gbc2->header_update_date = Main->clock;
                }
            }
        }

        if (nitems >= 0) {
            long newmod = mode;
            if (mode>=0) {
                if (mode==0 && nitems<=1) {         // only a partial send
                    gb2->flags2.folded_container = 1;
                }
            }
            else {
                newmod = -2;
            }
            debug_printf("Client %i \n", nheader);
            for (long item = 0; item < nitems; item++) {
                debug_printf("  Client reading %i\n", item);
                long irror = gbcm_read_bin(socket, gb2->as_container(), buffer, newmod, 0, cs_main);
                if (irror) {
                    return GBCM_SERVER_FAULT;
                }
            }
            debug_printf("Client done\n", 0);
        }
        else {
            if ((mode==0) && !gb_source) {      // created GBDATA at client
                gb2->flags2.folded_container = 1;
            }
        }
    }
    else {
        if (mode >= 0) {
            GBENTRY *ge2 = gb2->as_entry();
            if (type < GB_BITS) {
                ge2->info.i = buffer[i++];
            }
            else {
                long  realsize = buffer[i++];
                long  memsize  = buffer[i++];

                ge2->index_check_out();
                assert_or_exit(!(ge2->stored_external() && ge2->info.ex.get_data()));

                GBENTRY_memory storage(ge2, realsize, memsize);
                size = gbcm_read(socket, storage, memsize);
                if (size != memsize) {
                    fprintf(stderr, "receive failed data\n");
                    return GBCM_SERVER_FAULT;
                }
            }
        }
        else {
            if (type >= GB_BITS) { // dummy read (e.g. updata in server && not cached in client
                long memsize;
                char *buffer2;
                i++;
                memsize = buffer[i++];
                buffer2 = GB_give_buffer2(memsize);

                size = gbcm_read(socket, buffer2, memsize);
                if (size != memsize) {
                    GB_internal_error("receive failed data\n");
                    return GBCM_SERVER_FAULT;
                }
            }
        }
    }

    return GBCM_SERVER_OK;
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

static GBCM_ServerResult gbcms_write_deleted(int socket, long hsin, long client_clock, long *buffer) {
    Socinf            *socinf;
    gb_server_data    *hs;
    gbcms_delete_list *dl;

    hs = (gb_server_data *)hsin;
    for (socinf = hs->soci; socinf; socinf=socinf->next) {
        if (socinf->socket == socket) break;
    }
    if (!socinf) return GBCM_SERVER_OK;
    if (!hs->del_first) return GBCM_SERVER_OK;
    while (!socinf->dl || (socinf->dl->next)) {
        if (!socinf->dl) socinf->dl = hs->del_first;
        else    socinf->dl = socinf->dl->next;
        if (socinf->dl->creation_date>client_clock) continue;
        // created and deleted object
        buffer[0] = GBCM_COMMAND_PUT_UPDATE_DELETE;
        buffer[1] = (long)socinf->dl->gbd;
        if (gbcm_write(socket, (const char *)buffer, sizeof(long)*2)) return GBCM_SERVER_FAULT;
    }
    for (socinf = hs->soci; socinf; socinf=socinf->next) {
        if (!socinf->dl) return GBCM_SERVER_OK;
    }
    while ((dl = hs->del_first)) {
        for (socinf = hs->soci; socinf; socinf=socinf->next) {
            if (socinf->dl == dl) return GBCM_SERVER_OK;
        }
        hs->del_first = dl->next;
        gbm_free_mem(dl, sizeof(gbcms_delete_list), GBM_CB_INDEX);
    }
    return GBCM_SERVER_OK;
}

static GBCM_ServerResult gbcms_write_updated(int socket, GBDATA *gbd, long hsin, long client_clock, long *buffer) {
    if (gbd->update_date()<=client_clock) return GBCM_SERVER_OK;
    if (gbd->creation_date() > client_clock) {
        buffer[0] = GBCM_COMMAND_PUT_UPDATE_CREATE;
        buffer[1] = (long)GB_FATHER(gbd);
        if (gbcm_write(socket, (const char *)buffer, sizeof(long)*2)) return GBCM_SERVER_FAULT;

        GB_ERROR error = gbcm_write_bin(socket, gbd, buffer, 0, 1);
        if (error) {
            dumpError(error);
            return GBCM_SERVER_FAULT;
        }
    }
    else { // send clients first
        if (gbd->is_container()) {
            GBCONTAINER *gbc         = gbd->as_container();
            int          end         = (int)gbc->d.nheader;
            int          send_header = (gbc->header_update_date > client_clock) ? 1 : 0;

            buffer[0] = GBCM_COMMAND_PUT_UPDATE_UPDATE;
            buffer[1] = (long)gbd;
            if (gbcm_write(socket, (const char *)buffer, sizeof(long)*2)) return GBCM_SERVER_FAULT;

            GB_ERROR error = gbcm_write_bin(socket, gbd, buffer, 0, send_header);
            if (error) {
                dumpError(error);
                return GBCM_SERVER_FAULT;
            }

            for (int index = 0; index < end; index++) {
                GBDATA *gb2 = GBCONTAINER_ELEM(gbc, index);
                if (gb2 && gbcms_write_updated(socket, gb2, hsin, client_clock, buffer)) {
                    return GBCM_SERVER_FAULT;
                }
            }
        }
        else {
            buffer[0]             = GBCM_COMMAND_PUT_UPDATE_UPDATE;
            buffer[1]             = (long)gbd;
            if (gbcm_write(socket, (const char *)buffer, sizeof(long)*2)) return GBCM_SERVER_FAULT;

            const int SEND_HEADER = 0;
            GB_ERROR  error       = gbcm_write_bin(socket, gbd, buffer, 0, SEND_HEADER);
            if (error) {
                dumpError(error);
                return GBCM_SERVER_FAULT;
            }
        }
    }

    return GBCM_SERVER_OK;
}

static GBCM_ServerResult gbcms_write_keys(int socket, GBDATA *gbd) { // @@@ move into GB_MAIN_TYPE?
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);

    long buffer[4];
    buffer[0] = GBCM_COMMAND_PUT_UPDATE_KEYS;
    buffer[1] = (long)gbd;
    buffer[2] = Main->keycnt;
    buffer[3] = Main->first_free_key;

    if (gbcm_write(socket, (const char *)buffer, 4*sizeof(long))) return GBCM_SERVER_FAULT;

    for (int i=1; i<Main->keycnt; i++) {
        gb_Key &key = Main->keys[i];
        buffer[0] = key.nref;
        buffer[1] = key.next_free_key;
        if (gbcm_write(socket, (const char *)buffer, sizeof(long)*2)) return GBCM_SERVER_FAULT;
        if (gbcm_write_string(socket, key.key)) return GBCM_SERVER_FAULT;
    }
    return GBCM_SERVER_OK;
}

static GBCM_ServerResult gbcms_talking_unfold(int socket, long */*hsin*/, void */*sin*/, GBDATA *gb_in) {
    // command: GBCM_COMMAND_UNFOLD

    GBCONTAINER *gbc = gb_in->expect_container();
    GBDATA      *gb2;
    char        *buffer;
    long         deep[1];
    long         index_pos[1];
    int          index, start, end;

    RETURN_SERVER_FAULT_ON_BAD_ADDRESS(gbc);
    if (gbc->type() != GB_DB) return GBCM_SERVER_FAULT;
    if (gbcm_read_two(socket, GBCM_COMMAND_SETDEEP, 0, deep)) {
        return GBCM_SERVER_FAULT;
    }
    if (gbcm_read_two(socket, GBCM_COMMAND_SETINDEX, 0, index_pos)) {
        return GBCM_SERVER_FAULT;
    }

    gbcm_read_flush();
    buffer = GB_give_buffer(1014);

    if (index_pos[0]==-2) {
        GB_ERROR error = gbcm_write_bin(socket, gbc, (long *)buffer, deep[0]+1, 1);
        if (error) {
            dumpError(error);
            return GBCM_SERVER_FAULT;
        }
        gbcm_write_flush(socket);
        return GBCM_SERVER_OK;
    }

    if (index_pos[0] >= 0) {
        start  = (int)index_pos[0];
        end = start + 1;
        if (gbcm_write_two(socket, GBCM_COMMAND_SEND_COUNT, 1)) {
            return GBCM_SERVER_FAULT;
        }
    }
    else {
        start = 0;
        end = gbc->d.nheader;
        if (gbcm_write_two(socket, GBCM_COMMAND_SEND_COUNT, gbc->d.size)) {
            return GBCM_SERVER_FAULT;
        }
    }
    for (index = start; index < end; index++) {
        if ((gb2 = GBCONTAINER_ELEM(gbc, index))) {
            GB_ERROR error = gbcm_write_bin(socket, gb2, (long *)buffer, deep[0], 1);
            if (error) {
                dumpError(error);
                return GBCM_SERVER_FAULT;
            }
        }
    }

    gbcm_write_flush(socket);
    return GBCM_SERVER_OK;
}

static GBCM_ServerResult gbcms_talking_get_update(int /*socket*/, long */*hsin*/, void */*sin*/, GBDATA */*gbd*/) {
    return GBCM_SERVER_OK;
}

static GBCM_ServerResult gbcms_talking_put_update(int socket, long */*hsin*/, void */*sin*/, GBDATA */*gbd*/) {
    /* Reads
     * - the date
     * - and all changed data
     * from client.
     *
     * command: GBCM_COMMAND_PUT_UPDATE
     */
    long          irror;
    GBDATA       *gbd;
    gbcms_create *cs[1], *cs_main[1];
    long         *buffer;
    bool          end;

    cs_main[0] = 0;
    buffer     = (long *) GB_give_buffer(1024);
    end        = false;
    while (!end) {
        if (gbcm_read(socket, (char *) buffer, sizeof(long) * 3) != sizeof(long) * 3) {
            return GBCM_SERVER_FAULT;
        }
        gbd = (GBDATA *) buffer[2];
        RETURN_SERVER_FAULT_ON_BAD_ADDRESS(gbd);
        switch (buffer[0]) {
            case GBCM_COMMAND_PUT_UPDATE_CREATE:
                irror = gbcm_read_bin(socket, gbd->as_container(), buffer, 1, 0, (void *)cs_main);
                if (irror) return GBCM_SERVER_FAULT;
                break;
            case GBCM_COMMAND_PUT_UPDATE_DELETE:
                gb_delete_force(gbd);
                break;
            case GBCM_COMMAND_PUT_UPDATE_UPDATE:
                irror = gbcm_read_bin(socket, 0, buffer, 1, gbd, 0);
                if (irror) return GBCM_SERVER_FAULT;
                break;
            case GBCM_COMMAND_PUT_UPDATE_END:
                end = true;
                break;
            default:
                return GBCM_SERVER_FAULT;
        }
    }
    gbcm_read_flush();                        // send all id's of newly created objects
    for (cs[0] = cs_main[0]; cs[0]; cs[0]=cs_main[0]) {
        cs_main[0] = cs[0]->next;
        buffer[0] = (long)cs[0]->client_id;
        buffer[1] = (long)cs[0]->server_id;
        if (gbcm_write(socket, (const char *)buffer, sizeof(long)*2)) return GBCM_SERVER_FAULT;
        free(cs[0]);
    }
    buffer[0] = 0;
    if (gbcm_write(socket, (const char *)buffer, sizeof(long)*2)) return GBCM_SERVER_FAULT;
    gbcm_write_flush(socket);
    return GBCM_SERVER_OK;
}

static GBCM_ServerResult gbcms_talking_updated(int /*socket*/, long */*hsin*/, void */*sin*/, GBDATA */*gbd*/) {
    return GBCM_SERVER_OK;
}

static GBCM_ServerResult gbcms_talking_init_transaction(int socket, long *hsin, void *sin, GBDATA */*gbd*/) {
    /* begin client transaction
     * sends clock
     *
     * command: GBCM_COMMAND_INIT_TRANSACTION
     */

    gb_server_data *hs = (gb_server_data *)hsin;
    Socinf         *si = (Socinf *)sin;

    GBDATA       *gb_main = hs->gb_main;
    GB_MAIN_TYPE *Main    = GB_MAIN(gb_main);
    GBDATA       *gbd     = gb_main;
    char         *user    = gbcm_read_string(socket);

    gbcm_read_flush();
    if (gbcm_login(gbd->as_container(), user)) {
        return GBCM_SERVER_FAULT;
    }
    si->username = user;

    gb_local->running_client_transaction = ARB_TRANS;

    if (gbcm_write_two(socket, GBCM_COMMAND_TRANSACTION_RETURN, Main->clock)) {
        return GBCM_SERVER_FAULT;
    }
    if (gbcm_write_two(socket, GBCM_COMMAND_TRANSACTION_RETURN, (long)gbd)) {
        return GBCM_SERVER_FAULT;
    }
    if (gbcm_write_two(socket, GBCM_COMMAND_TRANSACTION_RETURN, (long)Main->this_user->userid)) {
        return GBCM_SERVER_FAULT;
    }
    gbcms_write_keys(socket, gbd);

    gbcm_write_flush(socket);
    // send modified data to client

    GB_begin_transaction(gbd);
    while (gb_local->running_client_transaction == ARB_TRANS) {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(socket, &set);

        struct timeval timeout;
        timeout.tv_sec  = GBCMS_TRANSACTION_TIMEOUT;
        timeout.tv_usec = 100000;

        long anz = select(FD_SETSIZE, FD_SET_TYPE &set, NULL, NULL, &timeout);

        if (anz<0) continue;
        if (anz==0) {
            GB_export_errorf("ARB_DB ERROR CLIENT TRANSACTION TIMEOUT, CLIENT DISCONNECTED (I waited %lu seconds)", timeout.tv_sec);
            GB_print_error();
            gb_local->running_client_transaction = ARB_ABORT;
            GB_abort_transaction(gbd);
            return GBCM_SERVER_FAULT;
        }
        if (GBCM_SERVER_OK == gbcms_talking(socket, hsin, sin)) continue;
        gb_local->running_client_transaction = ARB_ABORT;
        GB_abort_transaction(gbd);
        return GBCM_SERVER_FAULT;
    }
    if (gb_local->running_client_transaction == ARB_COMMIT) {
        GB_commit_transaction(gbd);
        gbcms_shift_delete_list(hsin, sin);
    }
    else {
        GB_abort_transaction(gbd);
    }
    return GBCM_SERVER_OK;
}

static GBCM_ServerResult gbcms_talking_begin_transaction(int socket, long *hsin, void *sin, GBDATA *long_client_clock) {
    /* begin client transaction
     * sends clock
     * deleted
     * created+updated
     *
     * command: GBCM_COMMAND_BEGIN_TRANSACTION
     */
    long            client_clock = (long)long_client_clock;
    GBDATA         *gb_main;
    GBDATA         *gbd;
    gb_server_data *hs           = (gb_server_data *)hsin;
    long            anz;
    long           *buffer;
    fd_set          set;
    timeval         timeout;

    gb_main = hs->gb_main;
    gbd = gb_main;
    gbcm_read_flush();
    gb_local->running_client_transaction = ARB_TRANS;

    if (gbcm_write_two(socket, GBCM_COMMAND_TRANSACTION_RETURN, GB_MAIN(gbd)->clock)) {
        return GBCM_SERVER_FAULT;
    }

    // send modified data to client
    buffer = (long *)GB_give_buffer(1024);
    if (GB_MAIN(gb_main)->key_clock > client_clock) {
        if (gbcms_write_keys(socket, gbd)) return GBCM_SERVER_FAULT;
    }
    if (gbcms_write_deleted(socket,      (long)hs, client_clock, buffer)) return GBCM_SERVER_FAULT;
    if (gbcms_write_updated(socket, gbd, (long)hs, client_clock, buffer)) return GBCM_SERVER_FAULT;
    buffer[0] = GBCM_COMMAND_PUT_UPDATE_END;
    buffer[1] = 0;
    if (gbcm_write(socket, (const char *)buffer, sizeof(long)*2)) return GBCM_SERVER_FAULT;
    if (gbcm_write_flush(socket))       return GBCM_SERVER_FAULT;

    GB_begin_transaction(gbd);
    while (gb_local->running_client_transaction == ARB_TRANS) {
        FD_ZERO(&set);
        FD_SET(socket, &set);

        timeout.tv_sec  = GBCMS_TRANSACTION_TIMEOUT;
        timeout.tv_usec = 0;

        anz = select(FD_SETSIZE, FD_SET_TYPE &set, NULL, NULL, &timeout);

        if (anz<0) continue;
        if (anz==0) {
            GB_export_errorf("ARB_DB ERROR CLIENT TRANSACTION TIMEOUT, CLIENT DISCONNECTED (I waited %lu seconds)", timeout.tv_sec);
            GB_print_error();
            gb_local->running_client_transaction = ARB_ABORT;
            GB_abort_transaction(gbd);
            return GBCM_SERVER_FAULT;
        }
        if (GBCM_SERVER_OK == gbcms_talking(socket, hsin, sin)) continue;
        gb_local->running_client_transaction = ARB_ABORT;
        GB_abort_transaction(gbd);
        return GBCM_SERVER_FAULT;
    }
    if (gb_local->running_client_transaction == ARB_COMMIT) {
        GB_commit_transaction(gbd);
        gbcms_shift_delete_list(hsin, sin);
    }
    else {
        GB_abort_transaction(gbd);
    }
    return GBCM_SERVER_OK;
}

static GBCM_ServerResult commit_or_abort_transaction(int socket, GBDATA *gbd, ARB_TRANS_TYPE commit_or_abort) {
    RETURN_SERVER_FAULT_ON_BAD_ADDRESS(gbd);
    
    gb_local->running_client_transaction = commit_or_abort;
    gbcm_read_flush();

    if (gbcm_write_two(socket, GBCM_COMMAND_TRANSACTION_RETURN, 0)) return GBCM_SERVER_FAULT;
    return gbcm_write_flush(socket);
}
static GBCM_ServerResult gbcms_talking_commit_transaction(int socket, long */*hsin*/, void */*sin*/, GBDATA *gbd) {
    // command: GBCM_COMMAND_COMMIT_TRANSACTION
    return commit_or_abort_transaction(socket, gbd, ARB_COMMIT);
}

static GBCM_ServerResult gbcms_talking_abort_transaction(int socket, long */*hsin*/, void */*sin*/, GBDATA *gbd) {
    // command: GBCM_COMMAND_ABORT_TRANSACTION
    return commit_or_abort_transaction(socket, gbd, ARB_ABORT);
}

static GBCM_ServerResult gbcms_talking_close(int /*socket*/, long */*hsin*/, void */*sin*/, GBDATA */*gbd*/) {
    // command: GBCM_COMMAND_CLOSE
    return GBCM_SERVER_ABORTED;
}

static GBCM_ServerResult gbcms_talking_undo(int socket, long */*hsin*/, void */*sin*/, GBDATA *gbd) {
    // command: GBCM_COMMAND_UNDO
    long cmd;
    GB_ERROR result = 0;
    char *to_free = 0;
    if (gbcm_read_two(socket, GBCM_COMMAND_UNDO_CMD, 0, &cmd)) {
        return GBCM_SERVER_FAULT;
    }
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
        default:    result = GB_set_undo_mem(gbd, cmd);
    }
    if (gbcm_write_string(socket, result)) {
        if (to_free) free(to_free);
        return GBCM_SERVER_FAULT;
    }
    if (to_free) free(to_free);
    return gbcm_write_flush(socket);
}

static GBCM_ServerResult gbcms_talking_find(int socket, long */*hsin*/, void */*sin*/, GBDATA * gbd) {
    // command: GBCM_COMMAND_FIND
    
    char     *key;
    char     *val1      = 0;
    GB_CASE   case_sens = GB_CASE_UNDEFINED;
    long      val2      = 0;
    GB_TYPES  type;
    void     *buffer[2];

    RETURN_SERVER_FAULT_ON_BAD_ADDRESS(gbd);

    key  = gbcm_read_string(socket);
    type = GB_TYPES(gbcm_read_long(socket));

    switch (type) {
        case GB_NONE:
            break;

        case GB_STRING:
            val1      = gbcm_read_string(socket);
            case_sens = GB_CASE(gbcm_read_long(socket));
            break;

        case GB_INT:
            val2 = gbcm_read_long(socket);
            break;

        default:
            gb_assert(0);
            GB_export_errorf("gbcms_talking_find: illegal data type (%i)", type);
            GB_print_error();
            return GBCM_SERVER_FAULT;
    }

    {
        GB_SEARCH_TYPE gbs = GB_SEARCH_TYPE(gbcm_read_long(socket));
        gbcm_read_flush();

        if (type == GB_FIND) {
            gbd = GB_find(gbd, key, gbs);
        }
        else if (type == GB_STRING) {
            gbd = GB_find_string(gbd, key, val1, case_sens, gbs);
            free(val1);
        }
        else if (type == GB_INT) {
            gbd = GB_find_int(gbd, key, val2, gbs);
        }
        else {
            GB_internal_errorf("Searching DBtype %i not implemented", type);
        }
    }

    free(key);

    if (gbcm_write_two(socket, GBCM_COMMAND_FIND_ERG, (long) gbd)) {
        return GBCM_SERVER_FAULT;
    }
    if (gbd) {
        while (GB_GRANDPA(gbd)) {
            buffer[0] = (void *)gbd->index;
            buffer[1] = (void *)GB_FATHER(gbd);
            gbcm_write(socket, (const char *) buffer, sizeof(long) * 2);
            gbd = GB_FATHER(gbd);
        }
    }
    buffer[0] = NULL;
    buffer[1] = NULL;
    gbcm_write(socket, (const char *) buffer, sizeof(long) * 2);

    return gbcm_write_flush(socket);
}

static GBCM_ServerResult gbcms_talking_key_alloc(int socket, long */*hsin*/, void */*sin*/, GBDATA * gbd) {
    // command: GBCM_COMMAND_KEY_ALLOC
    // (old maybe wrong comment: "do a query in the server")

    char *key;
    long  index;

    RETURN_SERVER_FAULT_ON_BAD_ADDRESS(gbd);
    key = gbcm_read_string(socket);
    gbcm_read_flush();

    if (key)
        index = gb_create_key(GB_MAIN(gbd), key, false);
    else
        index = 0;

    if (key)
        free(key);

    if (gbcm_write_two(socket, GBCM_COMMAND_KEY_ALLOC_RES, index)) {
        return GBCM_SERVER_FAULT;
    }
    return gbcm_write_flush(socket);
}

static GBCM_ServerResult gbcms_talking_disable_wait_for_new_request(int /*socket*/, long *hsin, void */*sin*/, GBDATA */*gbd*/) {
    gb_server_data *hs = (gb_server_data *) hsin;
    hs->wait_for_new_request--;
    return GBCM_SERVER_OK_WAIT;
}

static GBCM_ServerResult gbcms_talking_obsolete(int /*socket*/, long */*hsin*/, void */*sin*/, GBDATA */*gbd*/) {
    fputs("Obsolete server function called\n", stderr);
    return GBCM_SERVER_FAULT;
}

// -----------------------
//      server talking

typedef GBCM_ServerResult (*TalkingFunction)(int socket, long *hsin, void *sin, GBDATA *gbd);

static TalkingFunction aisc_talking_functions[] = {
    gbcms_talking_unfold,                           // GBCM_COMMAND_UNFOLD
    gbcms_talking_get_update,                       // GBCM_COMMAND_GET_UPDATA
    gbcms_talking_put_update,                       // GBCM_COMMAND_PUT_UPDATE
    gbcms_talking_updated,                          // GBCM_COMMAND_UPDATED
    gbcms_talking_begin_transaction,                // GBCM_COMMAND_BEGIN_TRANSACTION
    gbcms_talking_commit_transaction,               // GBCM_COMMAND_COMMIT_TRANSACTION
    gbcms_talking_abort_transaction,                // GBCM_COMMAND_ABORT_TRANSACTION
    gbcms_talking_init_transaction,                 // GBCM_COMMAND_INIT_TRANSACTION
    gbcms_talking_find,                             // GBCM_COMMAND_FIND
    gbcms_talking_close,                            // GBCM_COMMAND_CLOSE
    gbcms_talking_obsolete,
    gbcms_talking_key_alloc,                        // GBCM_COMMAND_KEY_ALLOC
    gbcms_talking_undo,                             // GBCM_COMMAND_UNDO
    gbcms_talking_disable_wait_for_new_request      // GBCM_COMMAND_DONT_WAIT
};

static GBCM_ServerResult gbcms_talking(int con, long *hs, void *sin) {
    gbcm_read_flush();

  next_command :

    long buf[3];
    long len = gbcm_read(con, (char *)buf, sizeof(long) * 3);
    if (len == sizeof(long) * 3) {
        long magic_number = buf[0];
        if ((magic_number & GBTUM_MAGIC_NUMBER_FILTER) != GBTUM_MAGIC_NUMBER) {
            gbcm_read_flush();
            fprintf(stderr, "Illegal Access\n");
            return GBCM_SERVER_FAULT;
        }
        magic_number &= ~GBTUM_MAGIC_NUMBER_FILTER;
        GBCM_ServerResult error = (aisc_talking_functions[magic_number])(con, hs, sin, (GBDATA *)buf[2]);
        if (error == GBCM_SERVER_OK_WAIT) {
            goto next_command;
        }
        gbcm_read_flush();
        return error ? error : GBCM_SERVER_OK;
    }
    else {
        return GBCM_SERVER_FAULT;
    }
}

bool GBCMS_accept_calls(GBDATA *gbd, bool wait_extra_time) {
    // returns true if served

    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    if (!Main->server_data) return false;
    if (Main->get_transaction_level()) return false;

    gb_server_data *hs = Main->server_data;
    timeval         timeout;

    if (wait_extra_time) {
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; // 100 ms
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
        fd_set set;
        fd_set setex;

        FD_ZERO(&set);
        FD_ZERO(&setex);
        FD_SET(hs->hso, &set);
        FD_SET(hs->hso, &setex);

        for (Socinf *si=hs->soci; si; si=si->next) {
            FD_SET(si->socket, &set);
            FD_SET(si->socket, &setex);
        }

        {
            long anz;
            if (hs->timeout>=0) {
                anz = select(FD_SETSIZE, FD_SET_TYPE &set, NULL, FD_SET_TYPE &setex, &timeout);
            }
            else {
                anz = select(FD_SETSIZE, FD_SET_TYPE &set, NULL, FD_SET_TYPE &setex, 0);
            }

            if (anz==-1) return false;
            if (!anz) return false; // timed out
        }


        if (FD_ISSET(hs->hso, &set)) {
            int con = accept(hs->hso, NULL, 0);
            if (con>0) {
                long optval[1];
                Socinf *sptr = (Socinf *)ARB_calloc(sizeof(Socinf), 1);
                sptr->next = hs->soci;
                sptr->socket = con;
                hs->soci=sptr;
                hs->nsoc++;
                optval[0] = 1;
                setsockopt(con, IPPROTO_TCP, TCP_NODELAY, (char *)optval, 4);
            }
        }
        else {
            Socinf *si_last = 0;
            Socinf *si_next;

            for (Socinf *si=hs->soci; si; si_last=si, si=si_next) {
                si_next = si->next;

                GBCM_ServerResult error = GBCM_SERVER_OK;
                if (FD_ISSET(si->socket, &set)) {
                    error = gbcms_talking(si->socket, (long *)hs, (void *)si);
                    if (GBCM_SERVER_OK == error) {
                        hs->wait_for_new_request ++;
                        continue;
                    }
                } else if (!FD_ISSET(si->socket, &setex)) continue;

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

                if (error != GBCM_SERVER_ABORTED) {
                    fprintf(stdout, "ARB_DB_SERVER: a client died abnormally\n");
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


GB_ERROR gbcm_unfold_client(GBCONTAINER *gbc, long deep, long index_pos) {
    // goes to header: __ATTR__USERESULT

    /* read data from a server
     * deep       = -1   read whole data
     * deep       = 0...n    read to deep
     * index_pos == -1 read all clients
     * index_pos == -2 read all clients + header array
     */

    GB_ERROR error  = NULL;
    int      socket = GBCONTAINER_MAIN(gbc)->c_link->socket;
    gbcm_read_flush();

    if      (gbcm_write_two  (socket, GBCM_COMMAND_UNFOLD, gbc->server_id)) error = CLIENT_SEND_ERROR();
    else if (gbcm_write_two  (socket, GBCM_COMMAND_SETDEEP, deep))          error = CLIENT_SEND_ERROR();
    else if (gbcm_write_two  (socket, GBCM_COMMAND_SETINDEX, index_pos))    error = CLIENT_SEND_ERROR();
    else if (gbcm_write_flush(socket))                                      error = CLIENT_SEND_ERROR();
    else {
        long buffer[256];
        long irror = 0;
        if (index_pos == -2) {
            irror = gbcm_read_bin(socket, 0, buffer, 0, gbc, 0);
        }
        else {
            long nitems;
            if (gbcm_read_two(socket, GBCM_COMMAND_SEND_COUNT, 0, &nitems)) irror = 1;
            else {
                for (long item = 0; !irror && item<nitems; item++) {
                    irror = gbcm_read_bin(socket, gbc, buffer, 0, 0, 0);
                }
            }
        }

        if (irror) {
            error = CLIENT_ERROR(GBS_global_string("receive error while unfolding '%s'", GB_read_key_pntr(gbc)));
        }
        else {
            gbcm_read_flush();
            if (index_pos < 0) {
                gbc->flags2.folded_container = 0;
            }
        }
    }

    return error;
}

// -------------------------
//      Client functions

GB_ERROR gbcmc_begin_sendupdate(GBDATA *gbd) {
    // goes to header: __ATTR__USERESULT

    if (gbcm_write_two(GB_MAIN(gbd)->c_link->socket, GBCM_COMMAND_PUT_UPDATE, gbd->server_id)) {
        return CLIENT_SEND_ERROR_AT_ITEM(gbd);
    }
    return 0;
}

GB_ERROR gbcmc_end_sendupdate(GBDATA *gbd) {
    // goes to header: __ATTR__USERESULT

    GB_MAIN_TYPE *Main   = GB_MAIN(gbd);
    int           socket = Main->c_link->socket;
    if (gbcm_write_two(socket, GBCM_COMMAND_PUT_UPDATE_END, gbd->server_id)) {
        return CLIENT_SEND_ERROR_AT_ITEM(gbd);
    }

    gbcm_write_flush(socket);
    while (1) {
        long buffer[2];
        if (gbcm_read(socket, (char *)&(buffer[0]), sizeof(long)*2) != sizeof(long)*2) {
            return CLIENT_RECEIVE_ERROR();
        }
        gbd = (GBDATA *)buffer[0];
        if (!gbd) break;
        gbd->server_id = buffer[1];
        GBS_write_numhash(Main->remote_hash, gbd->server_id, (long)gbd);
    }
    gbcm_read_flush();
    return 0;
}

GB_ERROR gbcmc_sendupdate_create(GBDATA *gbd) {
    // goes to header: __ATTR__USERESULT

    GBCONTAINER *father = GB_FATHER(gbd);
    if (!father) {
        return CLIENT_ERROR(GBS_global_string("entry '%s' has no father", GB_KEY(gbd)));
    }

    int socket = GB_MAIN(father)->c_link->socket;
    if (gbcm_write_two(socket, GBCM_COMMAND_PUT_UPDATE_CREATE, father->server_id)) {
        return CLIENT_SEND_ERROR_AT_ITEM(gbd);
    }

    long *buffer = (long *)GB_give_buffer(1014);
    return gbcm_write_bin(socket, gbd, buffer, -1, 1);
}

GB_ERROR gbcmc_sendupdate_delete(GBDATA *gbd) {
    // goes to header: __ATTR__USERESULT

    if (gbcm_write_two(GB_MAIN(gbd)->c_link->socket, GBCM_COMMAND_PUT_UPDATE_DELETE, gbd->server_id)) {
        return CLIENT_SEND_ERROR_AT_ITEM(gbd);
    }
    return 0;
}

GB_ERROR gbcmc_sendupdate_update(GBDATA *gbd, int send_headera) { // @@@ DRY vs gbcmc_sendupdate_create
    // goes to header: __ATTR__USERESULT

    GBCONTAINER *father = GB_FATHER(gbd);
    if (!father) {
        return CLIENT_ERROR(GBS_global_string("entry '%s' has no father", GB_KEY(gbd)));
    }

    int socket = GB_MAIN(father)->c_link->socket;
    if (gbcm_write_two(socket, GBCM_COMMAND_PUT_UPDATE_UPDATE, gbd->server_id)) {
        return CLIENT_SEND_ERROR_AT_ITEM(gbd);
    }

    long *buffer = (long *)GB_give_buffer(1016);
    return gbcm_write_bin(socket, gbd, buffer, 0, send_headera);
}

static __ATTR__USERESULT GB_ERROR gbcmc_read_keys(int socket, GBDATA *gbd) { // @@@ move into GB_MAIN_TYPE?
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    long          buffer[2];

    if (gbcm_read(socket, (char *)buffer, sizeof(long)*2) != sizeof(long)*2) {
        return CLIENT_RECEIVE_ERROR();
    }

    long size            = buffer[0];
    Main->first_free_key = buffer[1];
    gb_create_key_array(Main, (int)size);

    for (int i=1; i<size; i++) {
        if (gbcm_read(socket, (char *)buffer, sizeof(long)*2) != sizeof(long)*2) {
            return CLIENT_RECEIVE_ERROR();
        }

        gb_Key &KEY = Main->keys[i];

        KEY.nref          = buffer[0];    // to control malloc_index
        KEY.next_free_key = buffer[1];    // to control malloc_index

        char *key = gbcm_read_string(socket);
        if (key) {
            if (!key[0]) { // empty key
                free(key);
                return CLIENT_ERROR("invalid empty key received");
            }

            GBS_write_hash(Main->key_2_index_hash, key, i);
            freeset(KEY.key, key);
        }
    }
    Main->keycnt = (int)size;
    return 0;
}

GB_ERROR gbcmc_begin_transaction(GBDATA *gbd) {
    // goes to header: __ATTR__USERESULT

    GB_MAIN_TYPE *Main   = GB_MAIN(gbd);
    int           socket = Main->c_link->socket;
    long         *buffer = (long *)GB_give_buffer(1026);

    if (gbcm_write_two(socket, GBCM_COMMAND_BEGIN_TRANSACTION, Main->clock)) {
        return CLIENT_SEND_ERROR_AT_ITEM(gbd);
    }

    if (gbcm_write_flush(socket)) {
        return CLIENT_SEND_ERROR();
    }

    {
        long server_clock;
        if (gbcm_read_two(socket, GBCM_COMMAND_TRANSACTION_RETURN, 0, &server_clock)) {
            return CLIENT_RECEIVE_ERROR();
        }
        Main->clock = server_clock;
    }

    while (1) {
        if (gbcm_read(socket, (char *)buffer, sizeof(long)*2) != sizeof(long)*2) {
            return CLIENT_RECEIVE_ERROR();
        }

        long    d   = buffer[1];
        GBDATA *gb2 = (GBDATA *)GBS_read_numhash(Main->remote_hash, d);
        long    mode;
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
                if (gbcm_read_bin(socket, 0, buffer, mode, gb2, 0)) {
                    return CLIENT_RECEIVE_ERROR();
                }
                if (gb2) {
                    gb2->create_extended();
                    gb2->touch_update(Main->clock);
                }
                break;
            case GBCM_COMMAND_PUT_UPDATE_CREATE:
                if (gbcm_read_bin(socket, GBDATA::as_container(gb2), buffer, mode, 0, 0)) {
                    return CLIENT_RECEIVE_ERROR();
                }
                if (gb2) {
                    gb2->create_extended();
                    gb2->touch_creation_and_update(Main->clock);
                }
                break;
            case GBCM_COMMAND_PUT_UPDATE_DELETE:
                if (gb2) gb_delete_entry(gb2);
                break;
            case GBCM_COMMAND_PUT_UPDATE_KEYS: {
                GB_ERROR error = gbcmc_read_keys(socket, gbd);
                if (error) return error;
                break;
            }
            case GBCM_COMMAND_PUT_UPDATE_END:
                goto endof_gbcmc_begin_transaction;

            default:
                return CLIENT_RECEIVE_ERROR();
        }
    }

  endof_gbcmc_begin_transaction :
    gbcm_read_flush();
    return 0;
}

GB_ERROR gbcmc_init_transaction(GBCONTAINER *gbc) {
    // goes to header: __ATTR__USERESULT

    GB_ERROR      error  = 0;
    GB_MAIN_TYPE *Main   = GBCONTAINER_MAIN(gbc);
    int           socket = Main->c_link->socket;

    if (gbcm_write_two(socket, GBCM_COMMAND_INIT_TRANSACTION, Main->clock)) {
        return CLIENT_SEND_ERROR_AT_ITEM(gbc);
    }
    gbcm_write_string(socket, Main->this_user->username);
    if (gbcm_write_flush(socket)) {
        return CLIENT_SEND_ERROR();
    }

    {
        long server_clock;
        if (gbcm_read_two(socket, GBCM_COMMAND_TRANSACTION_RETURN, 0, &server_clock)) {
            return CLIENT_RECEIVE_ERROR();
        }
        Main->clock = server_clock;
    }

    long buffer[4];
    if (gbcm_read_two(socket, GBCM_COMMAND_TRANSACTION_RETURN, 0, buffer)) {
        return CLIENT_RECEIVE_ERROR();
    }
    gbc->server_id = buffer[0];

    if (gbcm_read_two(socket, GBCM_COMMAND_TRANSACTION_RETURN, 0, buffer)) {
        return CLIENT_RECEIVE_ERROR();
    }
    Main->this_user->userid = (int)buffer[0];
    Main->this_user->userbit = 1<<((int)buffer[0]);

    GBS_write_numhash(Main->remote_hash, gbc->server_id, (long)gbc);

    if (gbcm_read(socket, (char *)buffer, 2 * sizeof(long)) != 2 * sizeof(long)) {
        return CLIENT_RECEIVE_ERROR();
    }
    error = gbcmc_read_keys(socket, gbc);
    if (error) return error;

    gbcm_read_flush();
    return 0;
}

GB_ERROR gbcmc_commit_transaction(GBDATA *gbd) {
    // goes to header: __ATTR__USERESULT

    int socket = GB_MAIN(gbd)->c_link->socket;
    if (gbcm_write_two(socket, GBCM_COMMAND_COMMIT_TRANSACTION, gbd->server_id)) {
        return CLIENT_SEND_ERROR_AT_ITEM(gbd);
    }

    if (gbcm_write_flush(socket)) {
        return CLIENT_SEND_ERROR();
    }

    long dummy;
    if (gbcm_read_two(socket, GBCM_COMMAND_TRANSACTION_RETURN, 0, &dummy)) {
        return CLIENT_RECEIVE_ERROR();
    }
    gbcm_read_flush();
    return NULL;
}
GB_ERROR gbcmc_abort_transaction(GBDATA *gbd) { // @@@ DRY vs gbcmc_commit_transaction
    // goes to header: __ATTR__USERESULT

    int socket = GB_MAIN(gbd)->c_link->socket;
    if (gbcm_write_two(socket, GBCM_COMMAND_ABORT_TRANSACTION, gbd->server_id)) {
        return CLIENT_SEND_ERROR_AT_ITEM(gbd);
    }

    if (gbcm_write_flush(socket)) {
        return CLIENT_SEND_ERROR();
    }

    long dummy;
    if (gbcm_read_two(socket, GBCM_COMMAND_TRANSACTION_RETURN, 0, &dummy)) {
        return CLIENT_RECEIVE_ERROR();
    }
    gbcm_read_flush();
    return 0;
}

GB_ERROR gbcms_add_to_delete_list(GBDATA *gbd) {
    gb_server_data *hs = GB_MAIN(gbd)->server_data;

    if (hs && hs->soci) {
        gbcms_delete_list *dl = (gbcms_delete_list *)gbm_get_mem(sizeof(gbcms_delete_list), GBM_CB_INDEX);

        dl->creation_date = gbd->creation_date();
        dl->update_date   = gbd->update_date();
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

void GB_set_remote_action(GBDATA *gbd, bool in_action) {
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);

    if (Main->is_server()) { // GB_set_remote_action has no effect in clients
        gb_server_data *hs = Main->server_data;
        gb_assert(hs); // have no server data (program logic error)
        if (hs) {
            gb_assert(hs->inside_remote_action != in_action);
            hs->inside_remote_action = in_action;
        }
    }
}
bool GB_inside_remote_action(GBDATA *gbd) {
    GB_MAIN_TYPE *Main   = GB_MAIN(gbd);
    bool          inside = false;

    gb_assert(Main->is_server()); // GB_inside_remote_action not allowed from clients
    if (Main->is_server()) {
        gb_server_data *hs = Main->server_data;
        if (hs) {
            inside = hs->inside_remote_action;
        }
    }
    return inside;
}

long GB_read_clients(GBDATA *gbd) {
    // returns number of clients or
    // -1 if not called from server

    GB_MAIN_TYPE *Main    = GB_MAIN(gbd);
    long          clients = -1;

    if (Main->is_server()) {
        gb_server_data *hs = Main->server_data;
        clients = hs ? hs->nsoc : 0;
    }

    return clients;
}

bool GB_is_server(GBDATA *gbd) {
    return GB_MAIN(gbd)->is_server();
}

static __ATTR__USERESULT GB_ERROR gbcmc_unfold_list(int socket, GBDATA * gbd) {
    GB_ERROR error = NULL;

    long readvar[2];
    if (!gbcm_read(socket, (char *) readvar, sizeof(long) * 2)) {
        error = CLIENT_RECEIVE_ERROR();
    }
    else {
        GBCONTAINER *gb_client = (GBCONTAINER*)readvar[1];
        if (gb_client) {
            error = gbcmc_unfold_list(socket, gbd);
            if (!error) {
                gb_client = (GBCONTAINER *)GBS_read_numhash(GB_MAIN(gbd)->remote_hash, (long) gb_client);
                gb_unfold(gb_client, 0, (int)readvar[0]);
            }
        }
    }
    return error;
}

static void invalid_use_in_server(const char *function) {
    GB_internal_errorf("ARBDB fatal error: function '%s' may not be called in server", function);
}

GBDATA *GBCMC_find(GBDATA *gbd, const char *key, GB_TYPES type, const char *str, GB_CASE case_sens, GB_SEARCH_TYPE gbs) {
    // perform search in DB server (from DB client)
    // returns NULL if not found OR error occurred (use GB_have_error to test)

    union {
        GBDATA *gbd;
        long    l;
    } result;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);

    int socket;
    if (Main->is_server()) {
        invalid_use_in_server("GBCMC_find");
    }

    socket = Main->c_link->socket;

    if (gbcm_write_two(socket, GBCM_COMMAND_FIND, gbd->server_id)) {
        GB_export_error(CLIENT_SEND_ERROR());
        return 0;
    }

    gbcm_write_string(socket, key);
    gbcm_write_long(socket, type);
    switch (type) {
        case GB_NONE:
            break;
        case GB_STRING:
            gbcm_write_string(socket, str);
            gbcm_write_long(socket, case_sens);
            break;
        case GB_INT:
            gbcm_write_long(socket, *(long*)str);
            break;
        default:
            gb_assert(0);
            GB_export_errorf("GBCMC_find: Illegal data type (%i)", type);
            return 0;
    }

    gbcm_write_long(socket, gbs);

    if (gbcm_write_flush(socket)) {
        GB_export_error(CLIENT_SEND_ERROR());
        return 0;
    }
    if (gbcm_read_two(socket, GBCM_COMMAND_FIND_ERG, 0, &result.l)) {
        GB_export_error(CLIENT_RECEIVE_ERROR());
        return 0;
    }
    if (result.gbd) {
        GB_ERROR error = gbcmc_unfold_list(socket, gbd);
        if (error) {
            GB_export_error(error);
            return 0;
        }
        result.l = GBS_read_numhash(Main->remote_hash, result.l);
    }
    gbcm_read_flush();
    return result.gbd;
}


long gbcmc_key_alloc(GBDATA *gbd, const char *key) {
    /*! allocate a new key quark from client
     * returns new key quark for 'key' or 0 (error is exported in that case)
     */
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    if (Main->is_server()) {
        invalid_use_in_server("gbcmc_key_alloc");
    }

    int      socket = Main->c_link->socket;
    GB_ERROR error  = NULL;
    long     result = 0;
    if (gbcm_write_two(socket, GBCM_COMMAND_KEY_ALLOC, gbd->server_id)) {
        error = CLIENT_SEND_ERROR();
    }
    else {
        gbcm_write_string(socket, key);

        if (gbcm_write_flush(socket)) {
            error = CLIENT_SEND_ERROR();
        }
        else {
            if (gbcm_read_two(socket, GBCM_COMMAND_KEY_ALLOC_RES, 0, &result)) {
                error  = CLIENT_RECEIVE_ERROR();
                result = 0;
            }
            gbcm_read_flush();
        }
    }
    if (error) {
        gb_assert(result == 0); // indicates error
        GB_export_error(error);
    }
    return result;
}

GB_ERROR gbcmc_send_undo_commands(GBDATA *gbd, enum gb_undo_commands command) { // goes to header: __ATTR__USERESULT
    // send an undo command

    GB_ERROR      error = NULL;
    GB_MAIN_TYPE *Main  = GB_MAIN(gbd);

    if (Main->is_server()) {
        invalid_use_in_server("gbcmc_send_undo_commands");
    }
    else {
        int socket = Main->c_link->socket;

        if      (gbcm_write_two  (socket, GBCM_COMMAND_UNDO, gbd->server_id)) error = CLIENT_SEND_ERROR();
        else if (gbcm_write_two  (socket, GBCM_COMMAND_UNDO_CMD, command))    error = CLIENT_SEND_ERROR();
        else if (gbcm_write_flush(socket))                                    error = CLIENT_SEND_ERROR();
        else {
            error = gbcm_read_string(socket);
            gbcm_read_flush();
        }
    }
    return error;
}

char *gbcmc_send_undo_info_commands(GBDATA *gbd, enum gb_undo_commands command) {
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    if (Main->is_server()) {
        invalid_use_in_server("gbcmc_send_undo_info_commands");
    }

    int socket = Main->c_link->socket;
    if (gbcm_write_two(socket, GBCM_COMMAND_UNDO, gbd->server_id) ||
        gbcm_write_two(socket, GBCM_COMMAND_UNDO_CMD, command) ||
        gbcm_write_flush(socket))
    {
        GB_export_error(CLIENT_SEND_ERROR());
        return 0;
    }

    char *result = gbcm_read_string(socket);
    gbcm_read_flush();
    return result;
}

GB_ERROR GB_tell_server_dont_wait(GBDATA *gbd) {
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    if (Main->is_server()) return 0;

    int socket = Main->c_link->socket;
    if (gbcm_write_two(socket, GBCM_COMMAND_DONT_WAIT, gbd->server_id)) {
        return CLIENT_SEND_ERROR();
    }

    return 0;
}

// ---------------------
//      Login/Logout


GB_ERROR gbcm_login(GBCONTAINER *gb_main, const char *loginname) {
    // goes to header: __ATTR__USERESULT

     // look for any free user and set this_user
    int i;
    GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(gb_main);

    for (i = 0; i<GB_MAX_USERS; i++) {
        gb_user *user = Main->users[i];
        if (user && strcmp(loginname, user->username) == 0) {
            Main->this_user = user;
            user->nusers++;
            return 0;
        }
    }
    for (i = 0; i<GB_MAX_USERS; i++) {
        gb_user*& user = Main->users[i];
        if (!user) {
            user = (gb_user *)ARB_calloc(sizeof(gb_user), 1);
            
            user->username = strdup(loginname);
            user->userid   = i;
            user->userbit  = 1<<i;
            user->nusers   = 1;

            Main->this_user = user;

            return 0;
        }
    }
    return GBS_global_string("Too many users in this database (user='%s')", loginname);
}

GBCM_ServerResult gbcmc_close(gbcmc_comm * link) {
    if (link->socket) {
        if (gbcm_write_two(link->socket, GBCM_COMMAND_CLOSE, 0)) {
            GB_export_error(CLIENT_SEND_ERROR());
            return GBCM_SERVER_FAULT;
        }
        if (gbcm_write_flush(link->socket)) {
            GB_export_error(CLIENT_SEND_ERROR());
            return GBCM_SERVER_FAULT;
        }
        close(link->socket);
        link->socket = 0;
    }
    free(link->unix_name);
    free(link);
    return GBCM_SERVER_OK;
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

