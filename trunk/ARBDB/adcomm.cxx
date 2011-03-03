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

#include <SigHandler.h>
#include <arb_signal.h>

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
#define GBCM_COMMAND_SYSTEM             (GBTUM_MAGIC_NUMBER+10)
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
#define GBCM_COMMAND_SYSTEM_RETURN      (GBTUM_MAGIC_NUMBER+0x10a0000)
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
            gbcmc_close(comm);
        }
        else {
            int   socket;
            char *unix_name;

            error = gbcm_open_socket(path, TCP_NODELAY, 0, &socket, &unix_name);
            if (!error) {
                ASSERT_RESULT_PREDICATE(is_default_or_ignore_sighandler, INSTALL_SIGHANDLER(SIGPIPE, gbcms_sigpipe, "GBCMS_open"));
                ASSERT_RESULT(SigHandler, SIG_DFL,                       INSTALL_SIGHANDLER(SIGHUP, gbcms_sighup, "GBCMS_open"));

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

#if defined(DEVEL_RALF)
#warning rewrite gbcm_write_bin (error handling - do not export)
#endif // DEVEL_RALF

static GB_ERROR gbcm_write_bin(int socket, GBDATA *gbd, long *buffer, long mode, long deep, int send_headera) {
     /* send a database item to client/server
      *
      * mode    =1 server
      *         =0 client
      * buffer = buffer
      * deep = 0 -> just send one item   >0 send sub entries too
      * send_headera = 1 -> if type = GB_DB send flag and key_quark array
     */

    GBCONTAINER *gbc;
    long         i;

    buffer[0] = GBCM_COMMAND_SEND;
    i = 2;
    buffer[i++] = (long)gbd;
    buffer[i++] = gbd->index;
    *(gb_flag_types *)(&buffer[i++]) = gbd->flags;
    if (GB_TYPE(gbd) == GB_DB) {
        int end;
        int index;
        gbc = (GBCONTAINER *)gbd;
        end = gbc->d.nheader;
        *(gb_flag_types3 *)(&buffer[i++]) = gbc->flags3;

        buffer[i++] = send_headera ? end : -1;
        buffer[i++] = deep ? gbc->d.size : -1;
        buffer[1]   = i;

        if (gbcm_write(socket, (const char *)buffer, i* sizeof(long))) {
            return GB_export_error("ARB_DB WRITE TO SOCKET FAILED");
        }

        if (send_headera) {
            gb_header_list  *hdl  = GB_DATA_LIST_HEADER(gbc->d);
            gb_header_flags *buf2 = (gb_header_flags *)GB_give_buffer2(gbc->d.nheader * sizeof(gb_header_flags));

            for (index = 0; index < end; index++) {
                buf2[index] = hdl[index].flags;
            }
            if (gbcm_write(socket, (const char *)buf2, end * sizeof(gb_header_flags))) {
                return GB_export_error("ARB_DB WRITE TO SOCKET FAILED");
            }
        }

        if (deep) {
            GBDATA *gb2;
            GB_ERROR error;
            debug_printf("%i    ", gbc->d.size);

            for (index = 0; index < end; index++) {
                if ((gb2 = GBCONTAINER_ELEM(gbc, index))) {
                    debug_printf("%i ", index);
                    error = gbcm_write_bin(socket, gb2,
                                           (long *)buffer, mode, deep-1, send_headera);
                    if (error) return error;
                }
            }
            debug_printf("\n", 0);
        }

    }
    else if ((unsigned int)GB_TYPE(gbd) < (unsigned int)GB_BITS) {
        buffer[i++] = gbd->info.i;
        buffer[1] = i;
        if (gbcm_write(socket, (const char *)buffer, i*sizeof(long))) {
            return GB_export_error("ARB_DB WRITE TO SOCKET FAILED");
        }
    }
    else {
        long memsize;
        buffer[i++] = GB_GETSIZE(gbd);
        memsize = buffer[i++] = GB_GETMEMSIZE(gbd);
        buffer[1] = i;
        if (gbcm_write(socket, (const char *)buffer, i* sizeof(long))) {
            return GB_export_error("ARB_DB WRITE TO SOCKET FAILED");
        }
        if (gbcm_write(socket, GB_GETDATA(gbd), memsize)) {
            return GB_export_error("ARB_DB WRITE TO SOCKET FAILED");
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

static GBCM_ServerResult gbcm_read_bin(int socket, GBCONTAINER *gbd, long *buffer, long mode, GBDATA *gb_source, void *cs_main) {
     /* read an entry into gbd
      * mode ==  1  server reads data
      * mode ==  0  client read all data
      * mode == -1  client read but do not read subobjects -> folded cont
      * mode == -2  client dummy read
      */
    GBDATA *gb2;
    long    index_pos;
    long    size;
    long    id;
    long    i;
    int     type;

    gb_flag_types  flags;
    gb_flag_types3 flags3;

    size = gbcm_read(socket, (char *)buffer, sizeof(long) * 3);
    if (size != sizeof(long) * 3) {
        fprintf(stderr, "receive failed header size\n");
        return GBCM_SERVER_FAULT;
    }
    if (buffer[0] != GBCM_COMMAND_SEND) {
        fprintf(stderr, "receive failed wrong command\n");
        return GBCM_SERVER_FAULT;
    }
    id = buffer[2];
    i = buffer[1];
    i = sizeof(long) * (i - 3);

    size = gbcm_read(socket, (char *)buffer, i);
    if (size != i) {
        GB_internal_error("receive failed DB_NODE\n");
        return GBCM_SERVER_FAULT;
    }

    i = 0;
    index_pos = buffer[i++];
    if (!gb_source && gbd && index_pos<gbd->d.nheader) {
        gb_source = GBCONTAINER_ELEM(gbd, index_pos);
    }
    flags = *(gb_flag_types *)(&buffer[i++]);
    type = flags.type;

    if (mode >= -1) {   // real read data
        if (gb_source) {
            int types = GB_TYPE(gb_source);
            gb2 = gb_source;
            if (types != type) {
                GB_internal_error("Type changed in client: Connection aborted\n");
                return GBCM_SERVER_FAULT;
            }
            if (mode>0) {   // transactions only in server
                RETURN_SERVER_FAULT_ON_BAD_ADDRESS(gb2);
            }
            if (types != GB_DB) {
                gb_save_extern_data_in_ts(gb2);
            }
            gb_touch_entry(gb2, GB_NORMAL_CHANGE);
        }
        else {
            if (mode==-1) goto dont_create_in_a_folded_container;
            if (type == GB_DB) {
                gb2 = (GBDATA *)gb_make_container(gbd, 0, index_pos,
                                                  GB_DATA_LIST_HEADER(gbd->d)[index_pos].flags.key_quark);
            }
            else {  // @@@ Header Transaction stimmt nicht
                gb2 = gb_make_entry(gbd, 0, index_pos,
                                    GB_DATA_LIST_HEADER(gbd->d)[index_pos].flags.key_quark, (GB_TYPES)type);
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
                cs = (gbcms_create *) GB_calloc(sizeof(gbcms_create), 1);
                cs->next = *((gbcms_create **) cs_main);
                *((gbcms_create **) cs_main) = cs;
                cs->server_id = gb2;
                cs->client_id = (GBDATA *) id;
            }
        }
        gb2->flags = flags;
        if (type == GB_DB) {
            ((GBCONTAINER *)gb2)->flags3 = *((gb_flag_types3 *)&(buffer[i++]));
        }
    }
    else {
    dont_create_in_a_folded_container :
        if (type == GB_DB) {
            flags3 = *((gb_flag_types3 *)&(buffer[i++]));
        }
        gb2 = 0;
    }

    if (type == GB_DB) {
        long nitems;
        long nheader;
        long item, irror;

        nheader = buffer[i++];
        nitems  = buffer[i++];

        if (nheader > 0) {
            long             realsize = nheader* sizeof(gb_header_flags);
            gb_header_flags *buffer2  = (gb_header_flags *)GB_give_buffer2(realsize);

            size = gbcm_read(socket, (char *)buffer2, realsize);
            if (size != realsize) {
                GB_internal_error("receive failed data\n");
                return GBCM_SERVER_FAULT;
            }
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
                for (item = 0; item < nheader; item++) {
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
            for (item = 0; item < nitems; item++) {
                debug_printf("  Client reading %i\n", item);
                irror = gbcm_read_bin(socket, (GBCONTAINER *)gb2, buffer,
                                      newmod, 0, cs_main);
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
    else if (type < GB_BITS) {
        if (mode >= 0)   gb2->info.i = buffer[i++];
    }
    else {
        if (mode >= 0) {
            long  realsize = buffer[i++];
            long  memsize  = buffer[i++];
            char *data;

            GB_INDEX_CHECK_OUT(gb2);

            assert_or_exit(!(gb2->flags2.extern_data && GB_EXTERN_DATA_DATA(gb2->info.ex)));

            if (GB_CHECKINTERN(realsize, memsize)) {
                GB_SETINTERN(gb2);
                data = GB_GETDATA(gb2);
            }
            else {
                GB_SETEXTERN(gb2);
                data = (char*)gbm_get_mem((size_t)memsize, GB_GBM_INDEX(gb2));
            }
            size = gbcm_read(socket, data, memsize);
            if (size != memsize) {
                fprintf(stderr, "receive failed data\n");
                return GBCM_SERVER_FAULT;
            }

            GB_SETSMD(gb2, realsize, memsize, data);

        }
        else {            // dummy read (e.g. updata in server && not cached in client
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

static GBCM_ServerResult gbcms_write_deleted(int socket, GBDATA *gbd, long hsin, long client_clock, long *buffer) {
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
    gbd = gbd;
    return GBCM_SERVER_OK;
}

static GBCM_ServerResult gbcms_write_updated(int socket, GBDATA *gbd, long hsin, long client_clock, long *buffer) {
    gb_server_data *hs;
    int             send_header = 0;

    if (GB_GET_EXT_UPDATE_DATE(gbd)<=client_clock) return GBCM_SERVER_OK;
    hs = (gb_server_data *)hsin;
    if (GB_GET_EXT_CREATION_DATE(gbd) > client_clock) {
        buffer[0] = GBCM_COMMAND_PUT_UPDATE_CREATE;
        buffer[1] = (long)GB_FATHER(gbd);
        if (gbcm_write(socket, (const char *)buffer, sizeof(long)*2)) return GBCM_SERVER_FAULT;
        gbcm_write_bin(socket, gbd, buffer, 1, 0, 1);
    }
    else {                                          // send clients first
        if (GB_TYPE(gbd) == GB_DB)
        {
            GBDATA      *gb2;
            GBCONTAINER *gbc = ((GBCONTAINER *)gbd);
            int          index, end;

            end = (int)gbc->d.nheader;
            if (gbc->header_update_date > client_clock) send_header = 1;

            buffer[0] = GBCM_COMMAND_PUT_UPDATE_UPDATE;
            buffer[1] = (long)gbd;
            if (gbcm_write(socket, (const char *)buffer, sizeof(long)*2)) return GBCM_SERVER_FAULT;
            gbcm_write_bin(socket, gbd, buffer, 1, 0, send_header);

            for (index = 0; index < end; index++) {
                if ((gb2 = GBCONTAINER_ELEM(gbc, index))) {
                    if (gbcms_write_updated(socket, gb2, hsin, client_clock, buffer))
                        return GBCM_SERVER_FAULT;
                }
            }
        }
        else {
            buffer[0] = GBCM_COMMAND_PUT_UPDATE_UPDATE;
            buffer[1] = (long)gbd;
            if (gbcm_write(socket, (const char *)buffer, sizeof(long)*2)) return GBCM_SERVER_FAULT;
            gbcm_write_bin(socket, gbd, buffer, 1, 0, send_header);
        }
    }

    return GBCM_SERVER_OK;
}

static GBCM_ServerResult gbcms_write_keys(int socket, GBDATA *gbd) {
    int i;
    long buffer[4];
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);

    buffer[0] = GBCM_COMMAND_PUT_UPDATE_KEYS;
    buffer[1] = (long)gbd;
    buffer[2] = Main->keycnt;
    buffer[3] = Main->first_free_key;
    if (gbcm_write(socket, (const char *)buffer, 4*sizeof(long))) return GBCM_SERVER_FAULT;

    for (i=1; i<Main->keycnt; i++) {
        buffer[0] = Main->keys[i].nref;
        buffer[1] = Main->keys[i].next_free_key;
        if (gbcm_write(socket, (const char *)buffer, sizeof(long)*2)) return GBCM_SERVER_FAULT;
        if (gbcm_write_string(socket, Main->keys[i].key)) return GBCM_SERVER_FAULT;
    }
    return GBCM_SERVER_OK;
}

static GBCM_ServerResult gbcms_talking_unfold(int socket, long */*hsin*/, void */*sin*/, GBDATA *gb_in) {
    // command: GBCM_COMMAND_UNFOLD

    GBCONTAINER *gbc = (GBCONTAINER *)gb_in;
    GBDATA      *gb2;
    char        *buffer;
    long         deep[1];
    long         index_pos[1];
    int          index, start, end;

    RETURN_SERVER_FAULT_ON_BAD_ADDRESS(gbc);
    if (GB_TYPE(gbc) != GB_DB) return GBCM_SERVER_FAULT;
    if (gbcm_read_two(socket, GBCM_COMMAND_SETDEEP, 0, deep)) {
        return GBCM_SERVER_FAULT;
    }
    if (gbcm_read_two(socket, GBCM_COMMAND_SETINDEX, 0, index_pos)) {
        return GBCM_SERVER_FAULT;
    }

    gbcm_read_flush();
    buffer = GB_give_buffer(1014);

    if (index_pos[0]==-2) {
        GB_ERROR error = gbcm_write_bin(socket, gb_in, (long *)buffer, 1, deep[0]+1, 1);
        if (error) {
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
            GB_ERROR error = gbcm_write_bin(socket, gb2, (long *)buffer, 1, deep[0], 1);
            if (error) {
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
                irror = gbcm_read_bin(socket, (GBCONTAINER *)gbd, buffer, 1, 0, (void *)cs_main);
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

static GBCM_ServerResult gbcms_talking_init_transaction(int socket, long *hsin, void *sin, GBDATA *gb_dummy) {
    /* begin client transaction
     * sends clock
     *
     * command: GBCM_COMMAND_INIT_TRANSACTION
     */
    GBDATA         *gb_main;
    GBDATA         *gbd;
    gb_server_data *hs = (gb_server_data *)hsin;
    Socinf         *si = (Socinf *)sin;
    long            anz;
    long           *buffer;
    char           *user;
    fd_set          set;
    struct timeval  timeout;
    GB_MAIN_TYPE   *Main;

    gb_dummy = gb_dummy;
    gb_main = hs->gb_main;
    Main = GB_MAIN(gb_main);
    gbd = gb_main;
    user = gbcm_read_string(socket);
    gbcm_read_flush();
    if (gbcm_login((GBCONTAINER *)gbd, user)) {
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
    buffer = (long *)GB_give_buffer(1024);

    GB_begin_transaction(gbd);
    while (gb_local->running_client_transaction == ARB_TRANS) {

        FD_ZERO(&set);
        FD_SET(socket, &set);

        timeout.tv_sec  = GBCMS_TRANSACTION_TIMEOUT;
        timeout.tv_usec = 100000;

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
    if (gbcms_write_deleted(socket, gbd, (long)hs, client_clock, buffer)) return GBCM_SERVER_FAULT;
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

static GBCM_ServerResult gbcms_talking_system(int socket, long */*hsin*/, void */*sin*/, GBDATA */*gbd*/) {
    // command: GBCM_COMMAND_SYSTEM
    char *comm = gbcm_read_string(socket);

    gbcm_read_flush();

    GB_ERROR error = GB_system(comm);
    if (error) {
        GB_warning(error);
        return GBCM_SERVER_FAULT;
    }

    if (gbcm_write_two(socket, GBCM_COMMAND_SYSTEM_RETURN, 0)) {
        return GBCM_SERVER_FAULT;
    }
    return gbcm_write_flush(socket);
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
            gbd = (GBDATA *)GB_FATHER(gbd);
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
    gbcms_talking_system,                           // GBCM_COMMAND_SYSTEM
    gbcms_talking_key_alloc,                        // GBCM_COMMAND_KEY_ALLOC
    gbcms_talking_undo,                             // GBCM_COMMAND_UNDO
    gbcms_talking_disable_wait_for_new_request      // GBCM_COMMAND_DONT_WAIT
};

static GBCM_ServerResult gbcms_talking(int con, long *hs, void *sin) {
    long              buf[3];
    long              len;
    GBCM_ServerResult error;
    long              magic_number;

    gbcm_read_flush();
 next_command :
    len = gbcm_read(con, (char *)buf, sizeof(long) * 3);
    if (len == sizeof(long) * 3) {
        magic_number = buf[0];
        if ((magic_number & GBTUM_MAGIC_NUMBER_FILTER) != GBTUM_MAGIC_NUMBER) {
            gbcm_read_flush();
            fprintf(stderr, "Illegal Access\n");
            return GBCM_SERVER_FAULT;
        }
        magic_number &= ~GBTUM_MAGIC_NUMBER_FILTER;
        error = (aisc_talking_functions[magic_number])(con, hs, sin, (GBDATA *)buf[2]);
        if (error == GBCM_SERVER_OK_WAIT) {
            goto next_command;
        }
        gbcm_read_flush();
        if (!error) {
            buf[0] = GBCM_SERVER_OK;
            return GBCM_SERVER_OK;
        }
        else {
            buf[0] = GBCM_SERVER_FAULT;
            return error;
        }
    }
    else {
        return GBCM_SERVER_FAULT;
    }
}

bool GBCMS_accept_calls(GBDATA *gbd, bool wait_extra_time) {
    // returns true if served

    gb_server_data    *hs;
    int                con;
    long               anz, i;
    GBCM_ServerResult  error    = GBCM_SERVER_OK;
    Socinf            *si, *si_last, *sinext, *sptr;
    fd_set             set, setex;
    timeval            timeout;
    GB_MAIN_TYPE      *Main     = GB_MAIN(gbd);
    long               in_trans = GB_read_transaction(gbd);

    if (!Main->server_data) return false;
    if (in_trans)           return false;

    hs = Main->server_data;


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
        FD_ZERO(&set);
        FD_ZERO(&setex);
        FD_SET(hs->hso, &set);
        FD_SET(hs->hso, &setex);

        for (si=hs->soci, i=1; si; si=si->next, i++)
        {
            FD_SET(si->socket, &set);
            FD_SET(si->socket, &setex);
        }

        if (hs->timeout>=0) {
            anz = select(FD_SETSIZE, FD_SET_TYPE &set, NULL, FD_SET_TYPE &setex, &timeout);
        }
        else {
            anz = select(FD_SETSIZE, FD_SET_TYPE &set, NULL, FD_SET_TYPE &setex, 0);
        }

        if (anz==-1) {
            return false;
        }
        if (!anz) { // timed out
            return false;
        }


        if (FD_ISSET(hs->hso, &set)) {
            con = accept(hs->hso, NULL, 0);
            if (con>0) {
                long optval[1];
                sptr = (Socinf *)GB_calloc(sizeof(Socinf), 1);
                if (!sptr) return 0;
                sptr->next = hs->soci;
                sptr->socket = con;
                hs->soci=sptr;
                hs->nsoc++;
                optval[0] = 1;
                setsockopt(con, IPPROTO_TCP, TCP_NODELAY, (char *)optval, 4);
            }
        }
        else {
            si_last = 0;

            for (si=hs->soci; si; si_last=si, si=sinext) {
                sinext = si->next;

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


#define SEND_ERROR() GBS_global_string("cannot send data to server (errcode=%i)", __LINE__)

GB_ERROR gbcm_unfold_client(GBCONTAINER *gbd, long deep, long index_pos) {
    // goes to header: __ATTR__USERESULT

    /* read data from a server
     * deep       = -1   read whole data
     * deep       = 0...n    read to deep
     * index_pos == -1 read all clients
     * index_pos == -2 read all clients + header array
     */

    int  socket;
    long buffer[256];
    long nitems[1];
    long item;
    long irror = 0;

    GB_ERROR error = NULL;

    socket = GBCONTAINER_MAIN(gbd)->c_link->socket;
    gbcm_read_flush();

    if      (gbcm_write_two  (socket, GBCM_COMMAND_UNFOLD, gbd->server_id)) error = SEND_ERROR();
    else if (gbcm_write_two  (socket, GBCM_COMMAND_SETDEEP, deep)) error = SEND_ERROR();
    else if (gbcm_write_two  (socket, GBCM_COMMAND_SETINDEX, index_pos)) error = SEND_ERROR();
    else if (gbcm_write_flush(socket)) error = SEND_ERROR();
    else {
        if (index_pos == -2) {
            irror = gbcm_read_bin(socket, 0, buffer, 0, (GBDATA*)gbd, 0);
        }
        else {
            if (gbcm_read_two(socket, GBCM_COMMAND_SEND_COUNT, 0, nitems)) irror = 1;
            else {
                for (item=0; !irror && item<nitems[0]; item++) {
                    irror = gbcm_read_bin(socket, gbd, buffer, 0, 0, 0);
                }
            }
        }

        if (irror) {
            error = GB_export_errorf("GB_unfold (%s) read error", GB_read_key_pntr((GBDATA*)gbd));
        }
        else {
            gbcm_read_flush();
            if (index_pos < 0) {
                gbd->flags2.folded_container = 0;
            }
        }
    }

    return error;
}

// -------------------------
//      Client functions

#if defined(DEVEL_RALF)
#warning rewrite gbcmc_... (error handling - do not export)
#endif // DEVEL_RALF

GB_ERROR gbcmc_begin_sendupdate(GBDATA *gbd)
{
    if (gbcm_write_two(GB_MAIN(gbd)->c_link->socket, GBCM_COMMAND_PUT_UPDATE, gbd->server_id)) {
        return GB_export_errorf("Cannot send '%s' to server", GB_KEY(gbd));
    }
    return 0;
}

GB_ERROR gbcmc_end_sendupdate(GBDATA *gbd)
{
    long    buffer[2];
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    int socket = Main->c_link->socket;
    if (gbcm_write_two(socket, GBCM_COMMAND_PUT_UPDATE_END, gbd->server_id)) {
        return GB_export_errorf("Cannot send '%s' to server", GB_KEY(gbd));
    }
    gbcm_write_flush(socket);
    while (1) {
        if (gbcm_read(socket, (char *)&(buffer[0]), sizeof(long)*2) != sizeof(long)*2) {
            return GB_export_error("ARB_DB READ ON SOCKET FAILED");
        }
        gbd = (GBDATA *)buffer[0];
        if (!gbd) break;
        gbd->server_id = buffer[1];
        GBS_write_numhash(Main->remote_hash, gbd->server_id, (long)gbd);
    }
    gbcm_read_flush();
    return 0;
}

GB_ERROR gbcmc_sendupdate_create(GBDATA *gbd)
{
    long    *buffer;
    GB_ERROR error;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    int socket = Main->c_link->socket;
    GBCONTAINER *father = GB_FATHER(gbd);

    if (!father) return GB_export_errorf("internal error #2453:%s", GB_KEY(gbd));
    if (gbcm_write_two(socket, GBCM_COMMAND_PUT_UPDATE_CREATE, father->server_id)) {
        return GB_export_errorf("Cannot send '%s' to server", GB_KEY(gbd));
    }
    buffer = (long *)GB_give_buffer(1014);
    error = gbcm_write_bin(socket, gbd, buffer, 0, -1, 1);
    if (error) return error;
    return 0;
}

GB_ERROR gbcmc_sendupdate_delete(GBDATA *gbd)
{
    if (gbcm_write_two(GB_MAIN(gbd)->c_link->socket,
                        GBCM_COMMAND_PUT_UPDATE_DELETE,
                        gbd->server_id)) {
        return GB_export_errorf("Cannot send '%s' to server", GB_KEY(gbd));
    }
    else {
        return 0;
    }
}

GB_ERROR gbcmc_sendupdate_update(GBDATA *gbd, int send_headera)
{
    long    *buffer;
    GB_ERROR error;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);

    if (!GB_FATHER(gbd)) return GB_export_errorf("internal error #2453 %s", GB_KEY(gbd));
    if (gbcm_write_two(Main->c_link->socket, GBCM_COMMAND_PUT_UPDATE_UPDATE, gbd->server_id)) {
        return GB_export_errorf("Cannot send '%s' to server", GB_KEY(gbd));
    }
    buffer = (long *)GB_give_buffer(1016);
    error = gbcm_write_bin(Main->c_link->socket, gbd, buffer, 0, 0, send_headera);
    if (error) return error;
    return 0;
}

static GB_ERROR gbcmc_read_keys(int socket, GBDATA *gbd) {
    long size;
    int i;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    char *key;
    long buffer[2];

    if (gbcm_read(socket, (char *)buffer, sizeof(long)*2) != sizeof(long)*2) {
        return GB_export_error("ARB_DB CLIENT ERROR receive failed 6336");
    }
    size = buffer[0];
    Main->first_free_key = buffer[1];
    gb_create_key_array(Main, (int)size);
    for (i=1; i<size; i++) {
        if (gbcm_read(socket, (char *)buffer, sizeof(long)*2) != sizeof(long)*2) {
            return GB_export_error("ARB_DB CLIENT ERROR receive failed 6253");
        }
        Main->keys[i].nref          = buffer[0];    // to control malloc_index
        Main->keys[i].next_free_key = buffer[1];    // to control malloc_index
        key                         = gbcm_read_string(socket);
        if (key) {
            GBS_write_hash(Main->key_2_index_hash, key, i);
            if (Main->keys[i].key) free (Main->keys[i].key);
            Main->keys[i].key = key;
        }
    }
    Main->keycnt = (int)size;
    return 0;
}

GB_ERROR gbcmc_begin_transaction(GBDATA *gbd)
{
    long    *buffer;
    long clock[1];
    GBDATA  *gb2;
    long    d;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    int socket = Main->c_link->socket;
    long    mode;
    GB_ERROR error;

    buffer = (long *)GB_give_buffer(1026);
    if (gbcm_write_two(Main->c_link->socket, GBCM_COMMAND_BEGIN_TRANSACTION, Main->clock)) {
        return GB_export_errorf("Cannot send '%s' to server", GB_KEY(gbd));
    }
    if (gbcm_write_flush(socket)) {
        return GB_export_error("ARB_DB CLIENT ERROR send failed 1626");
    }
    if (gbcm_read_two(socket, GBCM_COMMAND_TRANSACTION_RETURN, 0, clock)) {
        return GB_export_error("ARB_DB CLIENT ERROR receive failed 3656");
    };
    Main->clock = clock[0];
    while (1) {
        if (gbcm_read(socket, (char *)buffer, sizeof(long)*2) != sizeof(long)*2) {
            return GB_export_error("ARB_DB CLIENT ERROR receive failed 6435");
        }
        d = buffer[1];
        gb2 = (GBDATA *)GBS_read_numhash(Main->remote_hash, d);
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
                    return GB_export_error("ARB_DB CLIENT ERROR receive failed 2456");
                }
                if (gb2) {
                    GB_CREATE_EXT(gb2);
                    gb2->ext->update_date = clock[0];
                }
                break;
            case GBCM_COMMAND_PUT_UPDATE_CREATE:
                if (gbcm_read_bin(socket, (GBCONTAINER *)gb2, buffer, mode, 0, 0)) {
                    return GB_export_error("ARB_DB CLIENT ERROR receive failed 4236");
                }
                if (gb2) {
                    GB_CREATE_EXT(gb2);
                    gb2->ext->creation_date = gb2->ext->update_date = clock[0];
                }
                break;
            case GBCM_COMMAND_PUT_UPDATE_DELETE:
                if (gb2) gb_delete_entry(&gb2);
                break;
            case GBCM_COMMAND_PUT_UPDATE_KEYS:
                error = gbcmc_read_keys(socket, gbd);
                if (error) return error;
                break;
            case GBCM_COMMAND_PUT_UPDATE_END:
                goto endof_gbcmc_begin_transaction;
            default:
                return GB_export_error("ARB_DB CLIENT ERROR receive failed 6574");
        }
    }
 endof_gbcmc_begin_transaction :
    gbcm_read_flush();
    return 0;
}

GB_ERROR gbcmc_init_transaction(GBCONTAINER *gbd)
{
    long clock[1];
    long buffer[4];
    GB_ERROR error = 0;
    GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(gbd);
    int socket = Main->c_link->socket;

    if (gbcm_write_two(socket, GBCM_COMMAND_INIT_TRANSACTION, Main->clock)) {
        return GB_export_errorf("Cannot send '%s' to server", GB_KEY((GBDATA*)gbd));
    }
    gbcm_write_string(socket, Main->this_user->username);
    if (gbcm_write_flush(socket)) {
        return GB_export_error("ARB_DB CLIENT ERROR send failed 1426");
    }

    if (gbcm_read_two(socket, GBCM_COMMAND_TRANSACTION_RETURN, 0, clock)) {
        return GB_export_error("ARB_DB CLIENT ERROR receive failed 3456");
    };
    Main->clock = clock[0];

    if (gbcm_read_two(socket, GBCM_COMMAND_TRANSACTION_RETURN, 0, buffer)) {
        return GB_export_error("ARB_DB CLIENT ERROR receive failed 3654");
    };
    gbd->server_id = buffer[0];

    if (gbcm_read_two(socket, GBCM_COMMAND_TRANSACTION_RETURN, 0, buffer)) {
        return GB_export_error("ARB_DB CLIENT ERROR receive failed 3654");
    };
    Main->this_user->userid = (int)buffer[0];
    Main->this_user->userbit = 1<<((int)buffer[0]);

    GBS_write_numhash(Main->remote_hash, gbd->server_id, (long)gbd);

    if (gbcm_read(socket, (char *)buffer, 2 * sizeof(long)) != 2 * sizeof(long)) {
        return GB_export_error("ARB_DB CLIENT ERROR receive failed 2336");
    }
    error = gbcmc_read_keys(socket, (GBDATA *)gbd);
    if (error) return error;

    gbcm_read_flush();
    return 0;
}

GB_ERROR gbcmc_commit_transaction(GBDATA *gbd)
{
    long dummy[1];
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    int socket = Main->c_link->socket;

    if (gbcm_write_two(socket, GBCM_COMMAND_COMMIT_TRANSACTION, gbd->server_id)) {
        return GB_export_errorf("Cannot send '%s' to server", GB_KEY(gbd));
    }
    if (gbcm_write_flush(socket)) {
        return GB_export_error("ARB_DB CLIENT ERROR send failed");
    }
    gbcm_read_two(socket, GBCM_COMMAND_TRANSACTION_RETURN, 0, dummy);
    gbcm_read_flush();
    return 0;
}
GB_ERROR gbcmc_abort_transaction(GBDATA *gbd)
{
    long dummy[1];
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    int socket = Main->c_link->socket;
    if (gbcm_write_two(socket, GBCM_COMMAND_ABORT_TRANSACTION, gbd->server_id)) {
        return GB_export_errorf("Cannot send '%s' to server", GB_KEY(gbd));
    }
    if (gbcm_write_flush(socket)) {
        return GB_export_error("ARB_DB CLIENT ERROR send failed");
    }
    gbcm_read_two(socket, GBCM_COMMAND_TRANSACTION_RETURN, 0, dummy);
    gbcm_read_flush();
    return 0;
}

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

static GB_ERROR gbcmc_unfold_list(int socket, GBDATA * gbd)
{
    long      readvar[2];
    GBCONTAINER         *gb_client;
    GB_ERROR error;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    if (!gbcm_read(socket, (char *) readvar, sizeof(long) * 2)) {
        return GB_export_error("receive failed");
    }
    gb_client = (GBCONTAINER *) readvar[1];
    if (gb_client) {
        error = gbcmc_unfold_list(socket, gbd);
        if (error)
            return error;
        gb_client = (GBCONTAINER *) GBS_read_numhash(Main->remote_hash, (long) gb_client);
        gb_unfold(gb_client, 0, (int)readvar[0]);
    }
    return 0;
}

GBDATA *GBCMC_find(GBDATA *gbd, const char *key, GB_TYPES type, const char *str, GB_CASE case_sens, GB_SEARCH_TYPE gbs) {
    // perform search in DB server (from DB client)
    union {
        GBDATA *gbd;
        long    l;
    } result;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);

    int socket;
    if (Main->local_mode) {
        gb_assert(0); // GBCMC_find may only be used in DB clients
        return (GBDATA *)-1;
    }

    socket = Main->c_link->socket;

    if (gbcm_write_two(socket, GBCM_COMMAND_FIND, gbd->server_id)) {
        GB_export_error(SEND_ERROR());
        GB_print_error();
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
            GB_print_error();
            return 0;
    }

    gbcm_write_long(socket, gbs);

    if (gbcm_write_flush(socket)) {
        GB_export_error("ARB_DB CLIENT ERROR send failed");
        GB_print_error();
        return 0;
    }
    gbcm_read_two(socket, GBCM_COMMAND_FIND_ERG, 0, &result.l);
    if (result.gbd) {
        gbcmc_unfold_list(socket, gbd);
        result.l = GBS_read_numhash(Main->remote_hash, result.l);
    }
    gbcm_read_flush();
    return result.gbd;
}


long gbcmc_key_alloc(GBDATA *gbd, const char *key) {
    long gb_result[1];
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);

    int socket;
    if (Main->local_mode) return 0;
    socket = Main->c_link->socket;

    if (gbcm_write_two(socket, GBCM_COMMAND_KEY_ALLOC, gbd->server_id)) {
        GB_export_error(SEND_ERROR());
        GB_print_error();
        return 0;
    }

    gbcm_write_string(socket, key);

    if (gbcm_write_flush(socket)) {
        GB_export_error("ARB_DB CLIENT ERROR send failed");
        GB_print_error();
        return 0;
    }
    gbcm_read_two(socket, GBCM_COMMAND_KEY_ALLOC_RES, 0, gb_result);
    gbcm_read_flush();
    return gb_result[0];
}

#if defined(DEVEL_RALF)
#warning GBCMC_system should return GB_ERROR!
#endif // DEVEL_RALF

int GBCMC_system(GBDATA *gbd, const char *ss) { // goes to header: __ATTR__USERESULT
    int           socket;
    long          gb_result[2];
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);

    if (Main->local_mode) {
        GB_ERROR error = GB_system(ss);
        if (error) GB_export_error(error);
        return error != 0;
    }
    socket = Main->c_link->socket;

    if (gbcm_write_two(socket, GBCM_COMMAND_SYSTEM, gbd->server_id)) {
        GB_export_error(SEND_ERROR());
        GB_print_error();
        return -1;
    }

    gbcm_write_string(socket, ss);
    if (gbcm_write_flush(socket)) {
        GB_export_error("ARB_DB CLIENT ERROR send failed");
        GB_print_error();
        return -1;
    }
    gbcm_read_two(socket, GBCM_COMMAND_SYSTEM_RETURN, 0, (long *)gb_result);
    gbcm_read_flush();
    return 0;
}

GB_ERROR gbcmc_send_undo_commands(GBDATA *gbd, enum gb_undo_commands command) { // goes to header: __ATTR__USERESULT
    // send an undo command

    GB_ERROR      error = NULL;
    GB_MAIN_TYPE *Main  = GB_MAIN(gbd);

    if (Main->local_mode) {
        GB_internal_error("gbcmc_send_undo_commands: cannot call a server in a server");
    }
    else {
        int socket = Main->c_link->socket;

        if      (gbcm_write_two  (socket, GBCM_COMMAND_UNDO, gbd->server_id)) error = SEND_ERROR();
        else if (gbcm_write_two  (socket, GBCM_COMMAND_UNDO_CMD, command)) error = SEND_ERROR();
        else if (gbcm_write_flush(socket)) error = SEND_ERROR();
        else {
            error = gbcm_read_string(socket);
            gbcm_read_flush();
        }
    }
    return error;
}

char *gbcmc_send_undo_info_commands(GBDATA *gbd, enum gb_undo_commands command) {
    int socket;
    char *result;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    if (Main->local_mode) {
        GB_internal_error("gbcmc_send_undo_commands: cannot call a server in a server");
        return 0;
    }
    socket = Main->c_link->socket;

    if (gbcm_write_two(socket, GBCM_COMMAND_UNDO, gbd->server_id)) {
        GB_export_error("Cannot send data to Server 456");
        return 0;
    }
    if (gbcm_write_two(socket, GBCM_COMMAND_UNDO_CMD, command)) {
        GB_export_error("Cannot send data to Server 96f");
        return 0;
    }
    if (gbcm_write_flush(socket)) {
        GB_export_error("Cannot send data to Server 536");
        return 0;
    }
    result = gbcm_read_string(socket);
    gbcm_read_flush();
    return result;
}

GB_ERROR GB_tell_server_dont_wait(GBDATA *gbd) {
    int socket;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);

    if (Main->local_mode) {
        return 0;
    }
    socket = Main->c_link->socket;
    if (gbcm_write_two(socket, GBCM_COMMAND_DONT_WAIT, gbd->server_id)) {
        GB_export_error("Cannot send data to Server 456");
        return 0;
    }

    return 0;
}

// ---------------------
//      Login/Logout


GB_ERROR gbcm_login(GBCONTAINER *gb_main, const char *loginname) {
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
            user = (gb_user *) GB_calloc(sizeof(gb_user), 1);
            
            user->username = strdup(loginname);
            user->userid   = i;
            user->userbit  = 1<<i;
            user->nusers   = 1;

            Main->this_user = user;

            return 0;
        }
    }
    return GB_export_errorf("Too many users in this database: User '%s' ", loginname);
}

GBCM_ServerResult gbcmc_close(gbcmc_comm * link) {
    if (link->socket) {
        if (gbcm_write_two(link->socket, GBCM_COMMAND_CLOSE, 0)) {
            GB_export_error("Cannot send data to server");
            GB_print_error();
            return GBCM_SERVER_FAULT;
        }
        if (gbcm_write_flush(link->socket)) {
            GB_export_error("ARB_DB CLIENT ERROR send failed");
            GB_print_error();
            return GBCM_SERVER_FAULT;
        }
        close(link->socket);
        link->socket = 0;
    }
    if (link->unix_name) free(link->unix_name); // @@@
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


GB_CSTR GB_get_hostname() {
    static char *hn = 0;
    if (!hn) {
        char buffer[4096];
        gethostname(buffer, 4095);
        hn = strdup(buffer);
    }
    return hn;
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
    tm      *p;

    gettimeofday(&date, 0);

#if defined(DARWIN)
    struct timespec local;
    TIMEVAL_TO_TIMESPEC(&date, &local); // not avail in time.h of Linux gcc 2.95.3
    p = localtime(&local.tv_sec);
#else
    p = localtime(&date.tv_sec);
#endif // DARWIN

    char *readable = asctime(p); // points to a static buffer
    char *cr       = strchr(readable, '\n');
    gb_assert(cr);
    cr[0]          = 0;         // cut of \n

    return readable;
}

