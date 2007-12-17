#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>
#include <errno.h>
/*#include "arbdb.h"*/
#include "adlocal.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <sys/time.h>

#if defined(SUN4) || defined(SUN5)
# ifndef __cplusplus
#  define SIG_PF void (*)()
# else
#  include <sysent.h>   /* c++ only for sun */
# endif
#else
# define SIG_PF void (*)(int )
#endif

#define FD_SET_TYPE

#define debug_printf(a,b)

#define GBCMS_TRANSACTION_TIMEOUT 60*60
/* one hour timeout */
#define MAX_QUEUE_LEN 5
#define GBCM_COMMAND_UNFOLD     (GBTUM_MAGIC_NUMBER)
#define GBCM_COMMAND_GET_UPDATA     (GBTUM_MAGIC_NUMBER+1)
#define GBCM_COMMAND_PUT_UPDATE     (GBTUM_MAGIC_NUMBER+2)
#define GBCM_COMMAND_UPDATED        (GBTUM_MAGIC_NUMBER+3)
#define GBCM_COMMAND_BEGIN_TRANSACTION  (GBTUM_MAGIC_NUMBER+4)
#define GBCM_COMMAND_COMMIT_TRANSACTION (GBTUM_MAGIC_NUMBER+5)
#define GBCM_COMMAND_ABORT_TRANSACTION  (GBTUM_MAGIC_NUMBER+6)
#define GBCM_COMMAND_INIT_TRANSACTION   (GBTUM_MAGIC_NUMBER+7)
#define GBCM_COMMAND_FIND       (GBTUM_MAGIC_NUMBER+8)
#define GBCM_COMMAND_CLOSE      (GBTUM_MAGIC_NUMBER+9)
#define GBCM_COMMAND_SYSTEM     (GBTUM_MAGIC_NUMBER+10)
#define GBCM_COMMAND_KEY_ALLOC      (GBTUM_MAGIC_NUMBER+11)
#define GBCM_COMMAND_UNDO       (GBTUM_MAGIC_NUMBER+12)
#define GBCM_COMMAND_DONT_WAIT      (GBTUM_MAGIC_NUMBER+13)



#define GBCM_COMMAND_SEND       (GBTUM_MAGIC_NUMBER+0x1000)
#define GBCM_COMMAND_SEND_COUNT     (GBTUM_MAGIC_NUMBER+0x2000)
#define GBCM_COMMAND_SETDEEP        (GBTUM_MAGIC_NUMBER+0x3000)
#define GBCM_COMMAND_SETINDEX       (GBTUM_MAGIC_NUMBER+0x4000)

#define GBCM_COMMAND_PUT_UPDATE_KEYS    (GBTUM_MAGIC_NUMBER+0x5000)
#define GBCM_COMMAND_PUT_UPDATE_CREATE  (GBTUM_MAGIC_NUMBER+0x6000)
#define GBCM_COMMAND_PUT_UPDATE_DELETE  (GBTUM_MAGIC_NUMBER+0x7000)
#define GBCM_COMMAND_PUT_UPDATE_UPDATE  (GBTUM_MAGIC_NUMBER+0x8000)
#define GBCM_COMMAND_PUT_UPDATE_END (GBTUM_MAGIC_NUMBER+0x9000)

#define GBCM_COMMAND_TRANSACTION_RETURN (GBTUM_MAGIC_NUMBER+0x100000)
#define GBCM_COMMAND_FIND_ERG       (GBTUM_MAGIC_NUMBER+0x108000)
#define GBCM_COMMAND_KEY_ALLOC_RES  (GBTUM_MAGIC_NUMBER+0x10b000)

#define GBCM_COMMAND_SYSTEM_RETURN  (GBTUM_MAGIC_NUMBER+0x10a0000)
#define GBCM_COMMAND_UNDO_CMD       (GBTUM_MAGIC_NUMBER+0x10a0001)
/*********************************** some structures *************************************/
/** Store all deleted items in a list */
struct gbcms_delete_list {
    struct gbcms_delete_list *next;
    long            creation_date;
    long            update_date;
    GBDATA          *gbd;
};

struct Socinf {
    struct Socinf *next;
    int socket;
    struct  gbcms_delete_list   *dl;    /* point to last deleted item that is sent to
                                           this client */
    char    *username;
};

void g_bcms_delete_Socinf(struct Socinf *THIS){
    if (THIS->username) free(THIS->username);
    THIS->username = NULL;
    THIS->next = 0;
    free ((char *)THIS);
}

struct Hs_struct {
    int hso;
    char    *unix_name;
    struct Socinf *soci;
    long nsoc;
    long timeout;
    GBDATA *gb_main;
    int     wait_for_new_request;
    struct gbcms_delete_list    *del_first; /* All deleted items, that are yet
                                               unknown to at least one client */
    struct gbcms_delete_list    *del_last;
};



struct gbcms_create_struct {
    struct gbcms_create_struct *next;
    GBDATA  *server_id;
    GBDATA  *client_id;
};


/**************************************************************************************
********************************    Panic save *************************
***************************************************************************************/
GBCONTAINER *gbcms_gb_main;
void *gbcms_sighup(void){
    char          buffer[1024];
    char         *fname;
    GB_ERROR      error;
    int           translevel;
    GB_MAIN_TYPE *Main;

    {
        const char *ap = GB_getenv("ARB_PID");
        if (!ap ) ap = "";
        sprintf(buffer,"/tmp/arb_panic_%s_%s",GB_getenvUSER(),ap);
    }
    fprintf(stderr,"**** ARB DATABASE SERVER GOT a HANGUP SIGNAL ****\n");
    fprintf(stderr,"- Looking for file '%s'\n",buffer);
    fname = GB_read_file(buffer);
    if (!fname) {
        fprintf(stderr,"- File '%s' not found - exiting!\n",buffer);
        exit(EXIT_FAILURE);
    }
    if (fname[strlen(fname)-1] == '\n') fname[strlen(fname)-1] = 0;
    if (strcmp(fname,"core") == 0) { GB_CORE; }

    fprintf(stderr,"- Trying to save DATABASE in ASCII Mode into file '%s'\n", fname);
    Main              = GBCONTAINER_MAIN(gbcms_gb_main);
    translevel        = Main->transaction;
    Main->transaction = 0;
    error             = GB_save_as((GBDATA*)gbcms_gb_main, fname, "a");
    
    if (error) fprintf(stderr,"Error while  saving '%s': %s\n", fname, error);
    else fprintf(stderr,"- DATABASE saved into '%s'\n", fname);

    unlink(buffer);
    Main->transaction = translevel;
    
    free(fname);

    return 0;
}


/**************************************************************************************
                server open
***************************************************************************************/

GB_ERROR GBCMS_open(const char *path,long timeout,GBDATA *gb_main)
{
    struct Hs_struct *hs;
    int      so[1];
    char    *unix_name[1];
    GB_ERROR err;
    struct gbcmc_comm *comm;
    GB_MAIN_TYPE   *Main = GB_MAIN(gb_main);
    if (Main->server_data) {
        return GB_export_error("ARB_DB_SERVER_ERROR reopen of server not allowed");
    }
    if ( (comm = gbcmc_open(path)) ) {
        GB_export_error("Socket '%s' already in use",path);
        GB_print_error();
        gbcmc_close(comm);
        comm = 0;
        return GB_get_error();
    }
    hs = (struct Hs_struct *)GB_calloc(sizeof(struct Hs_struct),1);

    hs->timeout = timeout;
    hs->gb_main = gb_main;
    err = gbcm_open_socket(path,TCP_NODELAY, 0, so,unix_name);
    if (err) {
        printf("%s\n",err);
        return err;
    }

    /*  signal(SIGSEGV,(SIG_PF)gbcm_sig_violation); */
    signal(SIGPIPE,(SIG_PF)gbcms_sigpipe);
    signal(SIGHUP,(SIG_PF)gbcms_sighup);

    gbcms_gb_main = (GBCONTAINER *)gb_main;

    if (listen((int)(so[0]), MAX_QUEUE_LEN) < 0) {
        return GB_export_error("ARB_DB SERVER ERROR could not listen (server) %i",errno);
    }
    hs->hso = so[0];
    hs->unix_name = unix_name[0];
    Main->server_data = (void *)hs;
    return 0;
}
/**************************************************************************************
                server close
***************************************************************************************/
GB_ERROR GBCMS_shutdown(GBDATA *gbd){
    struct Hs_struct *hs;
    struct Socinf *si;
    GB_MAIN_TYPE   *Main = GB_MAIN(gbd);
    if (!Main->server_data) return 0;
    hs = (struct Hs_struct *)Main->server_data;
    for(si=hs->soci; si; si=si->next){
        shutdown(si->socket, 2);  /* 2 = both dir */
        close(si->socket);
    }
    shutdown(hs->hso, 2);
    if (hs->unix_name){
        unlink(hs->unix_name);
        free(hs->unix_name);
        hs->unix_name = 0;
    }
    close(hs->hso);
    free((char *)Main->server_data);
    Main->server_data = 0;
    return 0;
}

/**************************************************************************************
                server send deleted and updated
***************************************************************************************/
void gbcms_shift_delete_list(void *hsi,void *soi)
{
    struct Hs_struct *hs = (struct Hs_struct *)hsi;
    struct Socinf *socinf   = (struct Socinf *)soi;
    if (!hs->del_first) return;
    while ( (!socinf->dl) || (socinf->dl->next) ) {
        if (!socinf->dl) socinf->dl = hs->del_first;
        else    socinf->dl = socinf->dl->next;
    }
}

/**************************************************************************************
                server send deleted and updated
***************************************************************************************/
int gbcms_write_deleted(int socket,GBDATA *gbd,long hsin,long client_clock, long *buffer)
{
    struct Socinf *socinf;
    struct Hs_struct *hs;
    struct gbcms_delete_list *dl;

    hs = (struct Hs_struct *)hsin;
    for (socinf = hs->soci;socinf;socinf=socinf->next){
        if (socinf->socket == socket) break;
    }
    if (!socinf) return 0;
    if (!hs->del_first) return 0;
    while (!socinf->dl || (socinf->dl->next) ) {
        if (!socinf->dl) socinf->dl = hs->del_first;
        else    socinf->dl = socinf->dl->next;
        if (socinf->dl->creation_date>client_clock) continue;
        /* created and deleted object */
        buffer[0] = GBCM_COMMAND_PUT_UPDATE_DELETE;
        buffer[1] = (long)socinf->dl->gbd;
        if (gbcm_write(socket,(const char *)buffer,sizeof(long)*2) ) return GBCM_SERVER_FAULT;
    }
    for (socinf = hs->soci;socinf;socinf=socinf->next){
        if (!socinf->dl) return 0;
    }
    while ( (dl = hs->del_first) ) {
        for (socinf = hs->soci;socinf;socinf=socinf->next){
            if (socinf->dl == dl) return 0;
        }
        hs->del_first = dl->next;
        gbm_free_mem((char *)dl,sizeof(struct gbcms_delete_list),GBM_CB_INDEX);
    }
    gbd = gbd;
    return 0;
}



int gbcms_write_updated(int socket,GBDATA *gbd,long hsin,long client_clock, long *buffer)
{
    struct Hs_struct *hs;
    int send_header = 0;

    if (GB_GET_EXT_UPDATE_DATE(gbd)<=client_clock) return 0;
    hs = (struct Hs_struct *)hsin;
    if ( GB_GET_EXT_CREATION_DATE(gbd) > client_clock) {
        buffer[0] = GBCM_COMMAND_PUT_UPDATE_CREATE;
        buffer[1] = (long)GB_FATHER(gbd);
        if (gbcm_write(socket,(const char *)buffer,sizeof(long)*2) ) return GBCM_SERVER_FAULT;
        gbcm_write_bin(socket,gbd,buffer,1,0,1);
    }else{      /* send clients first */
        if (GB_TYPE(gbd) == GB_DB)
        {
            GBDATA *gb2;
            GBCONTAINER *gbc = ((GBCONTAINER *)gbd);
            register int index,end;

            end = (int)gbc->d.nheader;
            if ( gbc->header_update_date > client_clock) send_header = 1;

            buffer[0] = GBCM_COMMAND_PUT_UPDATE_UPDATE;
            buffer[1] = (long)gbd;
            if (gbcm_write(socket,(const char *)buffer,sizeof(long)*2) ) return GBCM_SERVER_FAULT;
            gbcm_write_bin(socket,gbd,buffer,1,0,send_header);

            for (index = 0; index < end; index++) {
                if ( (gb2 = GBCONTAINER_ELEM(gbc,index)) ) {
                    if (gbcms_write_updated(socket,gb2,hsin,client_clock,buffer))
                        return GBCM_SERVER_FAULT;
                }
            }
        }else{
            buffer[0] = GBCM_COMMAND_PUT_UPDATE_UPDATE;
            buffer[1] = (long)gbd;
            if (gbcm_write(socket,(const char *)buffer,sizeof(long)*2) ) return GBCM_SERVER_FAULT;
            gbcm_write_bin(socket,gbd,buffer,1,0,send_header);
        }
    }

    return 0;
}

int gbcms_write_keys(int socket,GBDATA *gbd)
{
    int i;
    long buffer[4];
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);

    buffer[0] = GBCM_COMMAND_PUT_UPDATE_KEYS;
    buffer[1] = (long)gbd;
    buffer[2] = Main->keycnt;
    buffer[3] = Main->first_free_key;
    if (gbcm_write(socket,(const char *)buffer,4*sizeof(long)) ) return GBCM_SERVER_FAULT;

    for (i=1;i<Main->keycnt;i++) {
        buffer[0] = Main->keys[i].nref;
        buffer[1] = Main->keys[i].next_free_key;
        if (gbcm_write(socket,(const char *)buffer,sizeof(long)*2) ) return GBCM_SERVER_FAULT;
        if (gbcm_write_string(socket,Main->keys[i].key )) return GBCM_SERVER_FAULT;
    }
    return 0;
}

/**************************************************************************************
                server gbcms_talking_unfold
                GBCM_COMMAND_UNFOLD
***************************************************************************************/
int gbcms_talking_unfold(int socket,long *hsin,void *sin, GBDATA *gb_in)
{
    register GBCONTAINER *gbc = (GBCONTAINER *)gb_in;
    GB_ERROR error;
    GBDATA *gb2;
    char    *buffer;
    long deep[1];
    long index_pos[1];
    int index,start,end;
    GBUSE(hsin);GBUSE(sin);
    if ( (error = gbcm_test_address((long *)gbc,GBTUM_MAGIC_NUMBER))) {
        return GBCM_SERVER_FAULT;
    }
    if (GB_TYPE(gbc) != GB_DB) return 1;
    if (gbcm_read_two(socket,GBCM_COMMAND_SETDEEP,0,deep)){
        return GBCM_SERVER_FAULT;
    }
    if (gbcm_read_two(socket,GBCM_COMMAND_SETINDEX,0,index_pos)){
        return GBCM_SERVER_FAULT;
    }

    gbcm_read_flush(socket);
    buffer = GB_give_buffer(1014);

    if (index_pos[0]==-2) {
        error = gbcm_write_bin(socket,gb_in,(long *)buffer,1,deep[0]+1,1);
        if (error) {
            return GBCM_SERVER_FAULT;
        }
        gbcm_write_flush(socket);
        return 0;
    }

    if (index_pos[0] >= 0) {
        start  = (int)index_pos[0];
        end = start + 1;
        if (gbcm_write_two(socket,GBCM_COMMAND_SEND_COUNT, 1)){
            return GBCM_SERVER_FAULT;
        }
    }else{
        start = 0;
        end = gbc->d.nheader;
        if (gbcm_write_two(socket,GBCM_COMMAND_SEND_COUNT, gbc->d.size)){
            return GBCM_SERVER_FAULT;
        }
    }
    for (index = start; index < end; index++) {
        if ( (gb2 = GBCONTAINER_ELEM(gbc,index)) ) {
            error = gbcm_write_bin(socket,gb2,(long *)buffer,1,deep[0],1);
            if (error) {
                return GBCM_SERVER_FAULT;
            }
        }
    }

    gbcm_write_flush(socket);
    return 0;
}
/**************************************************************************************
                server gbcms_talking_get_update
***************************************************************************************/
int gbcms_talking_get_update(int socket,long *hsin,void *sin,GBDATA *gbd)
{
    struct Hs_struct *hs = (struct Hs_struct *)hsin;
    GBUSE(hs);
    socket = socket;
    gbd = gbd;
    sin = sin;
    return 0;
}
/**************************************************************************************
                server gbcms_talking_put_update
                GBCM_COMMAND_PUT_UPDATE
***************************************************************************************/
int
gbcms_talking_put_update(int socket, long *hsin, void *sin,GBDATA * gbd_dummy)
{
    /* reads the date from a client
       read all changed data of the client */
    GB_ERROR       error;
    long        irror;
    GBDATA         *gbd;
    struct gbcms_create_struct *cs[1], *cs_main[1];
    long            *buffer;
    GB_BOOL     end;
    struct Hs_struct *hs = (struct Hs_struct *) hsin;
    GBUSE(hs); GBUSE(sin);
    sin = sin;
    gbd_dummy = gbd_dummy;
    cs_main[0] = 0;
    buffer = (long *) GB_give_buffer(1024);
    end = GB_FALSE;
    while (!end) {
        if (gbcm_read(socket, (char *) buffer, sizeof(long) * 3) != sizeof(long) * 3)
            return GBCM_SERVER_FAULT;
        gbd = (GBDATA *) buffer[2];
        if ( (error = gbcm_test_address((long *)gbd,GBTUM_MAGIC_NUMBER))) {
            GB_export_error("address %p not valid 3712",gbd);
            GB_print_error();
            return GBCM_SERVER_FAULT;
        }
        switch (buffer[0]) {
            case GBCM_COMMAND_PUT_UPDATE_CREATE:
                irror = gbcm_read_bin(socket,
                                      (GBCONTAINER *)gbd, buffer, 1, 0,(void *)cs_main);
                if (irror) return GBCM_SERVER_FAULT;
                break;
            case GBCM_COMMAND_PUT_UPDATE_DELETE:
                gb_delete_force(gbd);
                break;
            case GBCM_COMMAND_PUT_UPDATE_UPDATE:
                irror = gbcm_read_bin(socket,0,buffer, 1,gbd,0);
                if (irror) return GBCM_SERVER_FAULT;
                break;
            case GBCM_COMMAND_PUT_UPDATE_END:
                end = GB_TRUE;
                break;
            default:
                return GBCM_SERVER_FAULT;
        }
    }
    gbcm_read_flush(socket);            /* send all id's of newly created
                            objects */
    for (cs[0] = cs_main[0];cs[0];cs[0]=cs_main[0]) {
        cs_main[0] = cs[0]->next;
        buffer[0] = (long)cs[0]->client_id;
        buffer[1] = (long)cs[0]->server_id;
        if (gbcm_write(socket,(const char *)buffer, sizeof(long)*2)) return GBCM_SERVER_FAULT;
        free((char *)cs[0]);
    }
    buffer[0] = 0;
    if (gbcm_write(socket,(const char *)buffer,sizeof(long)*2)) return GBCM_SERVER_FAULT;
    gbcm_write_flush(socket);
    return 0;
}
/**************************************************************************************
                server gbcms_talking_updated
***************************************************************************************/
int gbcms_talking_updated(int socket,long *hsin,void *sin, GBDATA *gbd)
{
    struct Hs_struct *hs = (struct Hs_struct *)hsin;
    GBUSE(hs);
    socket = socket;
    gbd = gbd;
    sin = sin;
    return 0;
}

/**************************************************************************************
                server gbcms_talking_begin_transaction
                GBCM_COMMAND_INIT_TRANSACTION
***************************************************************************************/
int gbcms_talking_init_transaction(int socket,long *hsin,void *sin,GBDATA *gb_dummy)
     /* begin client transaction
        send        clock
    */
{
    GBDATA *gb_main;
    GBDATA *gbd;
    struct Hs_struct *hs = (struct Hs_struct *)hsin;
    struct Socinf   *si  = (struct Socinf *)sin;
    long anz;
    long    *buffer;
    char    *user;
    fd_set set;
    struct timeval timeout;
    GB_MAIN_TYPE *Main;

    gb_dummy = gb_dummy;
    gb_main = hs->gb_main;
    Main = GB_MAIN(gb_main);
    gbd = gb_main;
    user = gbcm_read_string(socket);
    gbcm_read_flush(socket);
    if (gbcm_login((GBCONTAINER *)gbd,user)) {
        return GBCM_SERVER_FAULT;
    }
    si->username = user;

    gb_local->running_client_transaction = ARB_TRANS;

    if (gbcm_write_two(socket,GBCM_COMMAND_TRANSACTION_RETURN,Main->clock)){
        return GBCM_SERVER_FAULT;
    }
    if (gbcm_write_two(socket,GBCM_COMMAND_TRANSACTION_RETURN,(long)gbd)){
        return GBCM_SERVER_FAULT;
    }
    if (gbcm_write_two(socket,GBCM_COMMAND_TRANSACTION_RETURN,
                       (long)Main->this_user->userid)){
        return GBCM_SERVER_FAULT;
    }
    gbcms_write_keys(socket,gbd);

    gbcm_write_flush(socket);
    /* send modified data to client */
    buffer = (long *)GB_give_buffer(1024);

    GB_begin_transaction(gbd);
    while (gb_local->running_client_transaction == ARB_TRANS){

        FD_ZERO(&set);
        FD_SET(socket,&set);

        timeout.tv_sec  = GBCMS_TRANSACTION_TIMEOUT;
        timeout.tv_usec = 100000;

        anz = select(FD_SETSIZE,FD_SET_TYPE &set,NULL,NULL,&timeout);

        if (anz<0) continue;
        if (anz==0) {
            GB_export_error("ARB_DB ERROR CLIENT TRANSACTION TIMEOUT, CLIENT DISCONNECTED (I waited %lu seconds)",timeout.tv_sec);
            GB_print_error();
            gb_local->running_client_transaction = ARB_ABORT;
            GB_abort_transaction(gbd);
            return GBCM_SERVER_FAULT;
        }
        if( GBCM_SERVER_OK == gbcms_talking(socket,hsin,sin )) continue;
        gb_local->running_client_transaction = ARB_ABORT;
        GB_abort_transaction(gbd);
        return GBCM_SERVER_FAULT;
    }
    if (gb_local->running_client_transaction == ARB_COMMIT) {
        GB_commit_transaction(gbd);
        gbcms_shift_delete_list(hsin,sin);
    }else{
        GB_abort_transaction(gbd);
    }
    return 0;
}
/**************************************************************************************
                server gbcms_talking_begin_transaction
                GBCM_COMMAND_BEGIN_TRANSACTION
***************************************************************************************/
int gbcms_talking_begin_transaction(int socket,long *hsin,void *sin, long client_clock)
     /* begin client transaction
        send        clock
                deleted
                created+updated
    */
{
    GBDATA *gb_main;
    GBDATA *gbd;
    struct Hs_struct *hs = (struct Hs_struct *)hsin;
    long anz;
    long    *buffer;
    fd_set set;
    struct timeval timeout;

    gb_main = hs->gb_main;
    gbd = gb_main;
    gbcm_read_flush(socket);
    gb_local->running_client_transaction = ARB_TRANS;

    if (gbcm_write_two(socket,GBCM_COMMAND_TRANSACTION_RETURN,GB_MAIN(gbd)->clock)){
        return GBCM_SERVER_FAULT;
    }

    /* send modified data to client */
    buffer = (long *)GB_give_buffer(1024);
    if (GB_MAIN(gb_main)->key_clock > client_clock) {
        if (gbcms_write_keys(socket,gbd)) return GBCM_SERVER_FAULT;
    }
    if (gbcms_write_deleted(socket,gbd,(long)hs,client_clock,buffer)) return GBCM_SERVER_FAULT;
    if (gbcms_write_updated(socket,gbd,(long)hs,client_clock,buffer)) return GBCM_SERVER_FAULT;
    buffer[0] = GBCM_COMMAND_PUT_UPDATE_END;
    buffer[1] = 0;
    if (gbcm_write(socket,(const char *)buffer,sizeof(long)*2)) return GBCM_SERVER_FAULT;
    if (gbcm_write_flush(socket))       return GBCM_SERVER_FAULT;

    GB_begin_transaction(gbd);
    while (gb_local->running_client_transaction == ARB_TRANS){
        FD_ZERO(&set);
        FD_SET(socket,&set);

        timeout.tv_sec  = GBCMS_TRANSACTION_TIMEOUT;
        timeout.tv_usec = 0;

        anz = select(FD_SETSIZE,FD_SET_TYPE &set,NULL,NULL,&timeout);

        if (anz<0) continue;
        if (anz==0) {
            GB_export_error("ARB_DB ERROR CLIENT TRANSACTION TIMEOUT, CLIENT DISCONNECTED (I waited %lu seconds)",timeout.tv_sec);
            GB_print_error();
            gb_local->running_client_transaction = ARB_ABORT;
            GB_abort_transaction(gbd);
            return GBCM_SERVER_FAULT;
        }
        if( GBCM_SERVER_OK == gbcms_talking(socket,hsin,sin )) continue;
        gb_local->running_client_transaction = ARB_ABORT;
        GB_abort_transaction(gbd);
        return GBCM_SERVER_FAULT;
    }
    if (gb_local->running_client_transaction == ARB_COMMIT) {
        GB_commit_transaction(gbd);
        gbcms_shift_delete_list(hsin,sin);
    }else{
        GB_abort_transaction(gbd);
    }
    return 0;
}
/**************************************************************************************
                server gbcms_talking_commit_transaction
                GBCM_COMMAND_COMMIT_TRANSACTION
***************************************************************************************/
int gbcms_talking_commit_transaction(int socket,long *hsin,void *sin, GBDATA *gbd)
{
    GB_ERROR error = 0;
    struct Hs_struct *hs = (struct Hs_struct *)hsin;
    GBUSE(hs);
    sin = sin;
    if ( (error = gbcm_test_address((long *)gbd,GBTUM_MAGIC_NUMBER)) ) {
        GB_export_error("address %p not valid 4783",gbd);
        GB_print_error();
        if (error) return GBCM_SERVER_FAULT;
        return GBCM_SERVER_OK;
    }
    gb_local->running_client_transaction = ARB_COMMIT;
    gbcm_read_flush(socket);
    if (gbcm_write_two(socket,GBCM_COMMAND_TRANSACTION_RETURN,0)){
        return GBCM_SERVER_FAULT;
    }
    if (gbcm_write_flush(socket)){
        return GBCM_SERVER_FAULT;
    }
    return GBCM_SERVER_OK;
}
/**************************************************************************************
                server gbcms_talking_abort_transaction
                GBCM_COMMAND_ABORT_TRANSACTION
***************************************************************************************/
int gbcms_talking_abort_transaction(int socket,long *hsin,void *sin, GBDATA *gbd)
{
    GB_ERROR error;
    struct Hs_struct *hs = (struct Hs_struct *)hsin;
    GBUSE(hs);
    sin = sin;
    if ( (error = gbcm_test_address((long *)gbd,GBTUM_MAGIC_NUMBER)) ) {
        GB_export_error("address %p not valid 4356",gbd);
        GB_print_error();
        return GBCM_SERVER_FAULT;
    }
    gb_local->running_client_transaction = ARB_ABORT;
    gbcm_read_flush(socket);
    if (gbcm_write_two(socket,GBCM_COMMAND_TRANSACTION_RETURN,0)){
        return GBCM_SERVER_FAULT;
    }
    if (gbcm_write_flush(socket)){
        return GBCM_SERVER_FAULT;
    }
    return 0;
}
/**************************************************************************************
                gbcms_talking_close
                GBCM_COMMAND_CLOSE
***************************************************************************************/
int gbcms_talking_close(int socket,long *hsin,void *sin,GBDATA *gbd)
{
    gbd = gbd;
    socket = socket;
    hsin = hsin;
    sin = sin;
    return GBCM_SERVER_ABORTED;
}

/**************************************************************************************
                gbcms_talking_system
                GBCM_COMMAND_SYSTEM
***************************************************************************************/
int gbcms_talking_system(int socket,long *hsin,void *sin,GBDATA *gbd)
{
    char    *comm = gbcm_read_string(socket);
    gbcm_read_flush(socket);
    gbd = gbd;
    socket = socket;
    hsin = hsin;
    sin = sin;
    printf("Action: '%s'\n",comm);
    system(comm);
    if (gbcm_write_two(socket,GBCM_COMMAND_SYSTEM_RETURN,0)){
        return GBCM_SERVER_FAULT;
    }
    if (gbcm_write_flush(socket)){
        return GBCM_SERVER_FAULT;
    }
    return GBCM_SERVER_OK;
}

/**************************************************************************************
                gbcms_talking_undo
                GBCM_COMMAND_UNDO
***************************************************************************************/
int gbcms_talking_undo(int socket,long *hsin,void *sin,GBDATA *gbd)
{
    long cmd;
    GB_ERROR result = 0;
    char *to_free = 0;
    if (gbcm_read_two(socket, GBCM_COMMAND_UNDO_CMD,0,&cmd)){
        return GBCM_SERVER_FAULT;
    }
    gbcm_read_flush(socket);
    hsin = hsin;
    sin = sin;
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
            result = GB_undo(gbd,GB_UNDO_UNDO);
            break;
        case _GBCMC_UNDOCOM_REDO:
            result = GB_undo(gbd,GB_UNDO_REDO);
            break;
        default:    result = GB_set_undo_mem(gbd,cmd);
    }
    if (gbcm_write_string(socket,result)){
        if (to_free) free(to_free);
        return GBCM_SERVER_FAULT;
    }
    if (to_free) free(to_free);
    if (gbcm_write_flush(socket)){
        return GBCM_SERVER_FAULT;
    }
    return GBCM_SERVER_OK;
}

/**************************************************************************************
                do a query in the server
                GBCM_COMMAND_FIND
***************************************************************************************/
int
gbcms_talking_find(int socket, long *hsin, void *sin, GBDATA * gbd)
{
    GB_ERROR  error;
    char     *key;
    char     *val1 = 0;
    long      val2 = 0;
    GB_TYPES  type;
    void     *buffer[2];

    struct Hs_struct *hs = (struct Hs_struct *) hsin;
    GBUSE(hs);GBUSE(sin);

    if ( (error = gbcm_test_address((long *) gbd, GBTUM_MAGIC_NUMBER)) ) {
        GB_export_error("address %p not valid 8734", gbd);
        GB_print_error();
        return GBCM_SERVER_FAULT;
    }

    key  = gbcm_read_string(socket);
    type = gbcm_read_long(socket);

    switch (type) {
        case GB_STRING:
            val1  = gbcm_read_string(socket);
            break;
        case GB_INT:
            val2 = gbcm_read_long(socket);
            break;
        default:
            gb_assert(0);
            GB_export_error(GBS_global_string("gbcms_talking_find: illegal data type (%i)", type));
            GB_print_error();
            return GBCM_SERVER_FAULT;
    }

    {
        enum gb_search_types gbs = gbcm_read_long(socket);
        gbcm_read_flush(socket);

        if (type == GB_STRING) {
            gbd = GB_find(gbd, key, val1, gbs);
            free(val1);
        }
        else if (type == GB_INT) {
            gbd = GB_find_int(gbd, key, val2, gbs);
        }
        else {
            GB_internal_error(GBS_global_string("Searching DBtype %i not implemented", type));
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
            gbcm_write(socket, (const char *) buffer, sizeof(long) * 2 );
            gbd = (GBDATA *)GB_FATHER(gbd);
        }
    }
    buffer[0] = NULL;
    buffer[1] = NULL;
    gbcm_write(socket, (const char *) buffer, sizeof(long) * 2);

    if (gbcm_write_flush(socket)) {
        return GBCM_SERVER_FAULT;
    }
    return 0;
}

/**************************************************************************************
                do a query in the server
                GBCM_COMMAND_KEY_ALLOC
***************************************************************************************/
int
gbcms_talking_key_alloc(int socket, long *hsin, void *sin, GBDATA * gbd)
{
    GB_ERROR    error;
    char           *key;
    long            index;
    struct Hs_struct *hs = (struct Hs_struct *) hsin;
    GBUSE(hs);GBUSE(sin);

    if ( (error = gbcm_test_address((long *) gbd, GBTUM_MAGIC_NUMBER))) {
        GB_export_error("address %p not valid 8734", gbd);
        GB_print_error();
        return GBCM_SERVER_FAULT;
    }
    key = gbcm_read_string(socket);
    gbcm_read_flush(socket);

    if (key)
        index = gb_create_key(GB_MAIN(gbd),key,GB_FALSE);
    else
        index = 0;

    if (key)
        free(key);

    if (gbcm_write_two(socket, GBCM_COMMAND_KEY_ALLOC_RES, index)) {
        return GBCM_SERVER_FAULT;
    }
    if (gbcm_write_flush(socket)) {
        return GBCM_SERVER_FAULT;
    }
    return GBCM_SERVER_OK;
}

int gbcms_talking_disable_wait_for_new_request(int socket, long *hsin, void *sin, GBDATA *gbd){
    struct Hs_struct *hs = (struct Hs_struct *) hsin;
    GBUSE(socket);
    GBUSE(sin);
    GBUSE(gbd);
    hs->wait_for_new_request--;
    return GBCM_SERVER_OK_WAIT;
}
/**************************************************************************************
                server talking
***************************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
    static int (*(aisc_talking_functions[]))(int,long *,void *,GBDATA *) = {
        gbcms_talking_unfold,   /* GBCM_COMMAND_UNFOLD */
        gbcms_talking_get_update, /* GBCM_COMMAND_GET_UPDATA */
        gbcms_talking_put_update, /* GBCM_COMMAND_PUT_UPDATE */
        gbcms_talking_updated,  /* GBCM_COMMAND_UPDATED */
        ( int (*)(int , long *, void *,  GBDATA*) )gbcms_talking_begin_transaction, /* GBCM_COMMAND_BEGIN_TRANSACTION */
        gbcms_talking_commit_transaction, /* GBCM_COMMAND_COMMIT_TRANSACTION */
        gbcms_talking_abort_transaction, /* GBCM_COMMAND_ABORT_TRANSACTION */
        gbcms_talking_init_transaction, /* GBCM_COMMAND_INIT_TRANSACTION */
        gbcms_talking_find,     /* GBCM_COMMAND_FIND */
        gbcms_talking_close,    /* GBCM_COMMAND_CLOSE */
        gbcms_talking_system,   /* GBCM_COMMAND_SYSTEM */
        gbcms_talking_key_alloc, /* GBCM_COMMAND_KEY_ALLOC */
        gbcms_talking_undo,     /* GBCM_COMMAND_UNDO */
        gbcms_talking_disable_wait_for_new_request /* GBCM_COMMAND_DONT_WAIT */
    };
#ifdef __cplusplus
}
#endif

int gbcms_talking(int con,long *hs, void *sin)
{
    long      buf[3];
    long             len,error;
    long             magic_number;
    gbcm_read_flush(con);
 next_command:
    len = gbcm_read(con, (char *)buf, sizeof(long) * 3);
    if (len == sizeof(long) * 3) {
        magic_number = buf[0];
        if ((magic_number & GBTUM_MAGIC_NUMBER_FILTER) != GBTUM_MAGIC_NUMBER) {
            gbcm_read_flush(con);
            fprintf(stderr,"Illegal Access\n");
            return GBCM_SERVER_FAULT;
        }
        magic_number &= ~GBTUM_MAGIC_NUMBER_FILTER;
        error = (aisc_talking_functions[magic_number])(con,hs,sin,(GBDATA *)buf[2]);
        if (error == GBCM_SERVER_OK_WAIT){
            goto next_command;
        }
        gbcm_read_flush(con);
        if (!error) {
            buf[0] = GBCM_SERVER_OK;
            return GBCM_SERVER_OK;
        } else {
            buf[0] = GBCM_SERVER_FAULT;
            return error;
        }
    } else {
        return GBCM_SERVER_FAULT;
    }
}

GB_BOOL GBCMS_accept_calls(GBDATA *gbd,GB_BOOL wait_extra_time) /* returns GB_TRUE if served */
{
    struct Hs_struct *hs;
    int  con;
    long anz, i, error = 0;
    struct Socinf *si, *si_last, *sinext, *sptr;
    fd_set set,setex;
    struct timeval timeout;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    long    in_trans = GB_read_transaction(gbd);
    if (!Main->server_data) return GB_FALSE;
    if (in_trans)       return GB_FALSE;
    if (!Main->server_data) return GB_FALSE;

    hs = (struct Hs_struct *)Main->server_data;


    if (wait_extra_time){
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;
    }else{
        timeout.tv_sec = (int)(hs->timeout / 1000);
        timeout.tv_usec = (hs->timeout % 1000) * 1000;
    }
    if (wait_extra_time){
        hs->wait_for_new_request = 1;
    }else{
        hs->wait_for_new_request = 0;
    }
    {
        FD_ZERO(&set);
        FD_ZERO(&setex);
        FD_SET(hs->hso,&set);
        FD_SET(hs->hso,&setex);

        for(si=hs->soci, i=1; si; si=si->next, i++)
        {
            FD_SET(si->socket,&set);
            FD_SET(si->socket,&setex);
        }

        if ( hs->timeout>=0) {
            anz = select(FD_SETSIZE,FD_SET_TYPE &set,NULL,FD_SET_TYPE &setex,&timeout);
        }else{
            anz = select(FD_SETSIZE,FD_SET_TYPE &set,NULL,FD_SET_TYPE &setex,0);
        }

        if(anz==-1){
            /*printf("ERROR: poll in aisc_accept_calls %i\n",errno);*/
            return GB_FALSE;
        }
        if(!anz){ /* timed out */
            return GB_FALSE;
        }


        if(FD_ISSET(hs->hso,&set)){
            con = accept(hs->hso,NULL,0);
            if(con>0){
                long optval[1];
                sptr = (struct Socinf *)GB_calloc(sizeof(struct Socinf),1);
                if(!sptr) return 0;
                sptr->next = hs->soci;
                sptr->socket = con;
                hs->soci=sptr;
                hs->nsoc++;
                optval[0] = 1;
                setsockopt(con,IPPROTO_TCP,TCP_NODELAY,(char *)optval,4);
            }
        }else{
            si_last = 0;

            for(si=hs->soci; si; si_last=si, si=sinext){
                sinext = si->next;

                if (FD_ISSET(si->socket,&set)){
                    error = gbcms_talking(si->socket,(long *)hs,(void *)si);
                    if( GBCM_SERVER_OK == error){
                        hs->wait_for_new_request ++;
                        continue;
                    }
                }else if (!FD_ISSET(si->socket,&setex)) continue;

                /** kill socket **/

                if(close(si->socket)){
                    printf("aisc_accept_calls: ");
                    printf("couldn't close socket errno = %i!\n",errno);
                }

                hs->nsoc--;
                if(si==hs->soci){ /* first one */
                    hs->soci = si->next;
                }else{
                    si_last->next = si->next;
                }
                if (si->username){
                    gbcm_logout((GBCONTAINER *)hs->gb_main,si->username);
                }
                g_bcms_delete_Socinf(si);
                si = 0;

                if (error != GBCM_SERVER_ABORTED) {
                    fprintf(stdout,"ARB_DB_SERVER: a client died abnormally\n");
                }
                break;
            }
        }

    } /* while main loop */
    if (hs->wait_for_new_request>0){
        return GB_TRUE;
    }
    return GB_FALSE;
}


/**************************************************************************************
                write an entry
***************************************************************************************/
GB_ERROR gbcm_write_bin(int socket,GBDATA *gbd, long *buffer, long mode, long deep, int send_headera)
     /* mode =1 server  =0 client   */
     /* send a database item to client/server
        buffer = buffer
        deep = 0 -> just send one item   >0 send sub entries too
        send_headera = 1 -> if type = GB_DB send flag and key_quark array
     */
{
    register GBCONTAINER *gbc;
    long    i;

    buffer[0] = GBCM_COMMAND_SEND;
    i = 2;
    buffer[i++] = (long)gbd;
    buffer[i++] = gbd->index;
    *(struct gb_flag_types *)(&buffer[i++]) = gbd->flags;
    if (GB_TYPE(gbd) == GB_DB) {
        int end;
        int index;
        gbc = (GBCONTAINER *)gbd;
        end = gbc->d.nheader;
        *(struct gb_flag_types3 *)(&buffer[i++]) = gbc->flags3;

        if (send_headera) {
            buffer[i++] = end;
        }else{
            buffer[i++] = -1;
        }

        if (deep){
            buffer[i++] = gbc->d.size;
        }else{
            buffer[i++] = -1;
        }
        buffer[1] = i;
        if (gbcm_write(socket,(const char *)buffer,i* sizeof(long))) {
            return GB_export_error("ARB_DB WRITE TO SOCKET FAILED");
        }

        if (send_headera) {
            register struct gb_header_list_struct *hdl = GB_DATA_LIST_HEADER(gbc->d);
            struct gb_header_flags *buf2= (struct gb_header_flags *)GB_give_buffer2(gbc->d.nheader * sizeof(struct gb_header_flags));
            for (index = 0; index < end; index++) {
                buf2[index] = hdl[index].flags;
            }
            if (gbcm_write(socket,(const char *)buf2,end* sizeof(struct gb_header_flags))) {
                return GB_export_error("ARB_DB WRITE TO SOCKET FAILED");
            }
        }

        if (deep) {
            GBDATA *gb2;
            GB_ERROR error;
            debug_printf("%i    ",gbc->d.size);

            for (index = 0; index < end; index++) {
                if (( gb2 = GBCONTAINER_ELEM(gbc,index) )) {
                    debug_printf("%i ",index);
                    error = gbcm_write_bin(socket,gb2,
                                           (long *)buffer,mode,deep-1,send_headera);
                    if (error) return error;
                }
            }
            debug_printf("\n",0);
        }

    }else if ((unsigned int)GB_TYPE(gbd) < (unsigned int)GB_BITS) {
        buffer[i++] = gbd->info.i;
        buffer[1] = i;
        if (gbcm_write(socket,(const char *)buffer,i*sizeof(long)))  {
            return GB_export_error("ARB_DB WRITE TO SOCKET FAILED");
        }
    }else{
        long memsize;
        buffer[i++] = GB_GETSIZE(gbd);
        memsize = buffer[i++] = GB_GETMEMSIZE(gbd);
        buffer[1] = i;
        if (gbcm_write(socket,(const char *)buffer,i* sizeof(long)))  {
            return GB_export_error("ARB_DB WRITE TO SOCKET FAILED");
        }
        if (gbcm_write(socket,GB_GETDATA(gbd), memsize)){
            return GB_export_error("ARB_DB WRITE TO SOCKET FAILED");
        }
    }
    return 0;
}
/**************************************************************************************
                read an entry into gbd
***************************************************************************************/
long gbcm_read_bin(int socket,GBCONTAINER *gbd, long *buffer, long mode, GBDATA *gb_source,void *cs_main)
     /* mode == 1   server reads data       */
     /* mode ==  0  client read all data        */
     /* mode == -1  client read but do not read subobjects -> folded cont   */
     /* mode == -2  client dummy read       */
{
    register GBDATA *gb2;
    long        index_pos;
    long             size;
    long             id;
    long             i;
    struct gb_flag_types flags;
    int     type;
    struct gb_flag_types3 flags3;

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
    if (!gb_source && gbd && index_pos<gbd->d.nheader ) {
        gb_source = GBCONTAINER_ELEM(gbd,index_pos);
    }
    flags = *(struct gb_flag_types *)(&buffer[i++]);
    type = flags.type;

    if (mode >= -1) {   /* real read data */
        if (gb_source) {
            int types = GB_TYPE(gb_source);
            gb2 = gb_source;
            if (types != type) {
                GB_internal_error("Type changed in client: Connection abortet\n");
                return GBCM_SERVER_FAULT;
            }
            if (mode>0) {   /* transactions only in server */
                if (gbcm_test_address((long *) gb2, GBTUM_MAGIC_NUMBER)) {
                    return GBCM_SERVER_FAULT;
                }
            }
            if (types != GB_DB) {
                gb_save_extern_data_in_ts(gb2);
            }
            gb_touch_entry(gb2, gb_changed);
        } else {
            if (mode==-1) goto dont_create_in_a_folded_container;
            if (type == GB_DB) {
                gb2 = (GBDATA *)gb_make_container(gbd, 0, index_pos,
                                                  GB_DATA_LIST_HEADER(gbd->d)[index_pos].flags.key_quark);
            }else{  /* @@@ Header Transaction stimmt nicht */
                gb2 = gb_make_entry(gbd, 0, index_pos,
                                    GB_DATA_LIST_HEADER(gbd->d)[index_pos].flags.key_quark,(GB_TYPES)type);
            }
            if (mode>0){    /* transaction only in server */
                gb_touch_entry(gb2,gb_created);
            }else{
                gb2->server_id = id;
                GBS_write_hashi(GB_MAIN(gb2)->remote_hash, id, (long) gb2);
            }
            if (cs_main) {
                struct gbcms_create_struct *cs;
                cs = (struct gbcms_create_struct *) GB_calloc(sizeof(struct gbcms_create_struct), 1);
                cs->next = *((struct gbcms_create_struct **) cs_main);
                *((struct gbcms_create_struct **) cs_main) = cs;
                cs->server_id = gb2;
                cs->client_id = (GBDATA *) id;
            }
        }
        gb2->flags = flags;
        if (type == GB_DB) {
            ((GBCONTAINER *)gb2)->flags3 =
                *((struct gb_flag_types3 *)&(buffer[i++]));
        }
    } else {
    dont_create_in_a_folded_container:
        if (type == GB_DB) {
            flags3 = *((struct gb_flag_types3 *)&(buffer[i++]));
        }
        gb2 = 0;
    }

    if (type == GB_DB) {
        long             nitems;
        long         nheader;
        long             item,irror;
        nheader = buffer[i++];
        nitems = buffer[i++];

        if (nheader > 0) {
            long realsize = nheader* sizeof(struct gb_header_flags);
            register struct gb_header_flags *buffer2;
            buffer2 = (struct gb_header_flags *)GB_give_buffer2(realsize);
            size = gbcm_read(socket, (char *)buffer2, realsize);
            if (size != realsize) {
                GB_internal_error("receive failed data\n");
                return GBCM_SERVER_FAULT;
            }
            if (gb2 && mode >= -1) {
                GBCONTAINER *gbc = (GBCONTAINER*)gb2;
                register struct gb_header_list_struct *hdl;
                GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(gbc);

                gb_create_header_array(gbc,(int)nheader);
                if (nheader < gbc->d.nheader) {
                    GB_internal_error("Inconsistency Client-Server Cache");
                }
                gbc->d.nheader = (int)nheader;
                hdl = GB_DATA_LIST_HEADER(gbc->d);
                for (item = 0; item < nheader; item++) {
                    int old_index = hdl->flags.key_quark;
                    int new_index = buffer2->key_quark;
                    if (new_index && !old_index ) { /* a rename ... */
                        gb_write_index_key(gbc,item,new_index);
                    }
                    if (mode>0) {   /* server read data */


                    }else{
                        if ((int)buffer2->changed >= gb_deleted) {
                            hdl->flags.changed = gb_deleted;
                            hdl->flags.ever_changed = 1;
                        }
                    }
                    hdl->flags.flags = buffer2->flags;
                    hdl++; buffer2++;
                }
                if (mode>0){    /* transaction only in server */
                    gb_touch_header(gbc);
                }else{
                    gbc->header_update_date = Main->clock;
                }
            }
        }

        if (nitems >= 0) {
            long newmod = mode;
            if (mode>=0){
                if (mode==0 && nitems<=1 ) {        /* only a partial send */
                    gb2->flags2.folded_container = 1;
                }
            }else{
                newmod = -2;
            }
            debug_printf("Client %i \n",nheader);
            for (item = 0; item < nitems; item++) {
                debug_printf("  Client reading %i\n",item);
                irror = gbcm_read_bin(socket, (GBCONTAINER *)gb2, buffer,
                                      newmod,0,cs_main);
                if (irror) {
                    return GBCM_SERVER_FAULT;
                }
            }
            debug_printf("Client done\n",0);
        } else {
            if ((mode==0) && !gb_source){       /* created GBDATA at client */
                gb2->flags2.folded_container = 1;
            }
        }
    } else if (type < GB_BITS) {
        if (mode >=0)   gb2->info.i = buffer[i++];
    } else {
        if (mode >=0) {
            long realsize = buffer[i++];
            long memsize = buffer[i++];
            char *data;
            /* GB_FREEDATA(gb2); */
            GB_INDEX_CHECK_OUT(gb2);                        \
            if (gb2->flags2.extern_data && GB_EXTERN_DATA_DATA(gb2->info.ex)) GB_CORE;

            if (GB_CHECKINTERN(realsize,memsize)) {
                GB_SETINTERN(gb2);
                data = GB_GETDATA(gb2);
            }else{
                GB_SETEXTERN(gb2);
                data = gbm_get_mem((size_t)memsize,GB_GBM_INDEX(gb2));
            }
            size = gbcm_read(socket, data, memsize);
            if (size != memsize) {
                fprintf(stderr, "receive failed data\n");
                return GBCM_SERVER_FAULT;
            }

            GB_SETSMD(gb2,realsize,memsize,data);

        } else {            /* dummy read (e.g. updata in server && not cached in client */
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
/**************************************************************************************
                unfold client
***************************************************************************************/
GB_ERROR gbcm_unfold_client(GBCONTAINER *gbd, long deep, long index_pos)
     /* read data from a server
        deep = -1   read whole data
        deep = 0...n    read to deep
        index_pos  == -1 read all clients
        index_pos == -2 read all clients + header array
    */
{
    int socket;
    long    buffer[256];
    long    nitems[1];
    long    item;
    long    irror=0;

    socket = GBCONTAINER_MAIN(gbd)->c_link->socket;
    gbcm_read_flush(socket);
    if (gbcm_write_two(socket,GBCM_COMMAND_UNFOLD,gbd->server_id)){
        return GB_export_error("Cannot send data to Server");
    }
    if (gbcm_write_two(socket,GBCM_COMMAND_SETDEEP,deep)){
        return GB_export_error("Cannot send data to Server");
    }
    if (gbcm_write_two(socket,GBCM_COMMAND_SETINDEX,index_pos)){
        return GB_export_error("Cannot send data to Server");
    }
    if (gbcm_write_flush(socket)){
        return GB_export_error("Cannot send data to Server");
    }

    if (index_pos == -2){
        irror = gbcm_read_bin(socket,0,buffer,0,(GBDATA*)gbd,0);
    }else{
        if (gbcm_read_two(socket,GBCM_COMMAND_SEND_COUNT,0,nitems)) {
            irror = 1;
        }else{
            for (item=0;item<nitems[0];item++){
                irror = gbcm_read_bin(socket,gbd,buffer,0,0,0);
                if (irror) break;
            }
        }
    }
    if (irror) {
        GB_ERROR error;
        error = GB_export_error("GB_unfold (%s) read error",GB_read_key_pntr((GBDATA*)gbd));
        return error;
    }

    gbcm_read_flush(socket);
    if (index_pos < 0){
        gbd->flags2.folded_container = 0;
    }
    return 0;
}

/**************************************************************************************
                send update to server
                CLIENT  FUNCTIONS
***************************************************************************************/
GB_ERROR gbcmc_begin_sendupdate(GBDATA *gbd)
{
    if (gbcm_write_two(GB_MAIN(gbd)->c_link->socket,GBCM_COMMAND_PUT_UPDATE,gbd->server_id) ) {
        return GB_export_error("Cannot send '%s' to server",GB_KEY(gbd));
    }
    return 0;
}

GB_ERROR gbcmc_end_sendupdate(GBDATA *gbd)
{
    long    buffer[2];
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    int socket = Main->c_link->socket;
    if (gbcm_write_two(socket,GBCM_COMMAND_PUT_UPDATE_END,gbd->server_id) ) {
        return GB_export_error("Cannot send '%s' to server",GB_KEY(gbd));
    }
    gbcm_write_flush(socket);
    while (1) {
        if (gbcm_read(socket,(char *)&(buffer[0]),sizeof(long)*2) != sizeof(long)*2){
            return GB_export_error("ARB_DB READ ON SOCKET FAILED");
        }
        gbd = (GBDATA *)buffer[0];
        if (!gbd) break;
        gbd->server_id = buffer[1];
        GBS_write_hashi(Main->remote_hash,gbd->server_id,(long)gbd);
    }
    gbcm_read_flush(socket);
    return 0;
}

GB_ERROR gbcmc_sendupdate_create(GBDATA *gbd)
{
    long    *buffer;
    GB_ERROR error;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    int socket = Main->c_link->socket;
    GBCONTAINER *father = GB_FATHER(gbd);

    if (!father) return GB_export_error("internal error #2453:%s",GB_KEY(gbd));
    if (gbcm_write_two(socket,GBCM_COMMAND_PUT_UPDATE_CREATE,father->server_id) ) {
        return GB_export_error("Cannot send '%s' to server",GB_KEY(gbd));
    }
    buffer = (long *)GB_give_buffer(1014);
    error = gbcm_write_bin(socket,gbd,buffer,0,-1,1);
    if (error) return error;
    return 0;
}

GB_ERROR gbcmc_sendupdate_delete(GBDATA *gbd)
{
    if (gbcm_write_two( GB_MAIN(gbd)->c_link->socket,
                        GBCM_COMMAND_PUT_UPDATE_DELETE,
                        gbd->server_id) ) {
        return GB_export_error("Cannot send '%s' to server",GB_KEY(gbd));
    }else{
        return 0;
    }
}

GB_ERROR gbcmc_sendupdate_update(GBDATA *gbd, int send_headera)
{
    long    *buffer;
    GB_ERROR error;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);

    if (!GB_FATHER(gbd)) return GB_export_error("internal error #2453 %s",GB_KEY(gbd));
    if (gbcm_write_two(Main->c_link->socket,GBCM_COMMAND_PUT_UPDATE_UPDATE,gbd->server_id) ) {
        return GB_export_error("Cannot send '%s' to server",GB_KEY(gbd));
    }
    buffer = (long *)GB_give_buffer(1016);
    error = gbcm_write_bin(Main->c_link->socket,gbd,buffer,0,0,send_headera);
    if (error) return error;
    return 0;
}

GB_ERROR gbcmc_read_keys(int socket,GBDATA *gbd){
    long size;
    int i;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    char *key;
    long buffer[2];

    if (gbcm_read(socket,(char *)buffer,sizeof(long)*2) != sizeof(long)*2) {
        return GB_export_error("ARB_DB CLIENT ERROR receive failed 6336");
    }
    size = buffer[0];
    Main->first_free_key = buffer[1];
    gb_create_key_array(Main,(int)size);
    for (i=1;i<size;i++) {
        if (gbcm_read(socket,(char *)buffer,sizeof(long)*2) != sizeof(long)*2) {
            return GB_export_error("ARB_DB CLIENT ERROR receive failed 6253");
        }
        Main->keys[i].nref = buffer[0];         /* to control malloc_index */
        Main->keys[i].next_free_key = buffer[1];    /* to control malloc_index */
        key = gbcm_read_string(socket);
        if (key) {
            GBS_write_hash(Main->key_2_index_hash,key,i);
            if (Main->keys[i].key) free ( Main->keys[i].key );
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
    if (gbcm_write_two(Main->c_link->socket,GBCM_COMMAND_BEGIN_TRANSACTION,Main->clock) ) {
        return GB_export_error("Cannot send '%s' to server",GB_KEY(gbd));
    }
    if (gbcm_write_flush(socket)) {
        return GB_export_error("ARB_DB CLIENT ERROR send failed 1626");
    }
    if (gbcm_read_two(socket,GBCM_COMMAND_TRANSACTION_RETURN,0,clock)){
        return GB_export_error("ARB_DB CLIENT ERROR receive failed 3656");
    };
    Main->clock = clock[0];
    while (1){
        if (gbcm_read(socket,(char *)buffer,sizeof(long)*2) != sizeof(long)*2) {
            return GB_export_error("ARB_DB CLIENT ERROR receive failed 6435");
        }
        d = buffer[1];
        gb2 = (GBDATA *)GBS_read_hashi(Main->remote_hash,d);
        if (gb2){
            if (gb2->flags2.folded_container) {
                mode =-1;   /* read container */
            }else{
                mode = 0;   /* read all */
            }
        }else{
            mode = -2;      /* read nothing */
        }
        switch(buffer[0]) {
            case GBCM_COMMAND_PUT_UPDATE_UPDATE:
                if (gbcm_read_bin(socket,0,buffer, mode,gb2,0)){
                    return GB_export_error("ARB_DB CLIENT ERROR receive failed 2456");
                }
                if (gb2 ){
                    GB_CREATE_EXT(gb2);
                    gb2->ext->update_date = clock[0];
                }
                break;
            case GBCM_COMMAND_PUT_UPDATE_CREATE:
                if (gbcm_read_bin(socket, (GBCONTAINER *)gb2, buffer, mode, 0, 0)) {
                    return GB_export_error("ARB_DB CLIENT ERROR receive failed 4236");
                }
                if (gb2 ) {
                    GB_CREATE_EXT(gb2);
                    gb2->ext->creation_date = gb2->ext->update_date = clock[0];
                }
                break;
            case GBCM_COMMAND_PUT_UPDATE_DELETE:
                if (gb2)    gb_delete_entry(gb2);
                break;
            case GBCM_COMMAND_PUT_UPDATE_KEYS:
                error = gbcmc_read_keys(socket,gbd);
                if (error) return error;
                break;
            case GBCM_COMMAND_PUT_UPDATE_END:
                goto endof_gbcmc_begin_transaction;
            default:
                return GB_export_error("ARB_DB CLIENT ERROR receive failed 6574");
        }
    }
 endof_gbcmc_begin_transaction:
    gbcm_read_flush(socket);
    return 0;
}

GB_ERROR gbcmc_init_transaction(GBCONTAINER *gbd)
{
    long clock[1];
    long buffer[4];
    GB_ERROR error = 0;
    GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(gbd);
    int socket = Main->c_link->socket;

    if (gbcm_write_two(socket,GBCM_COMMAND_INIT_TRANSACTION,Main->clock) ) {
        return GB_export_error("Cannot send '%s' to server",GB_KEY((GBDATA*)gbd));
    }
    gbcm_write_string(socket,Main->this_user->username);
    if (gbcm_write_flush(socket)) {
        return GB_export_error("ARB_DB CLIENT ERROR send failed 1426");
    }

    if (gbcm_read_two(socket,GBCM_COMMAND_TRANSACTION_RETURN,0,clock)){
        return GB_export_error("ARB_DB CLIENT ERROR receive failed 3456");
    };
    Main->clock = clock[0];

    if (gbcm_read_two(socket,GBCM_COMMAND_TRANSACTION_RETURN,0,buffer)){
        return GB_export_error("ARB_DB CLIENT ERROR receive failed 3654");
    };
    gbd->server_id = buffer[0];

    if (gbcm_read_two(socket,GBCM_COMMAND_TRANSACTION_RETURN,0,buffer)){
        return GB_export_error("ARB_DB CLIENT ERROR receive failed 3654");
    };
    Main->this_user->userid = (int)buffer[0];
    Main->this_user->userbit = 1<<((int)buffer[0]);

    GBS_write_hashi(Main->remote_hash,gbd->server_id,(long)gbd);

    if (gbcm_read(socket,(char *)buffer, 2 * sizeof(long)) !=  2 * sizeof(long)) {
        return GB_export_error("ARB_DB CLIENT ERROR receive failed 2336");
    }
    error = gbcmc_read_keys(socket,(GBDATA *)gbd);
    if (error) return error;

    gbcm_read_flush(socket);
    return 0;
}

GB_ERROR gbcmc_commit_transaction(GBDATA *gbd)
{
    long dummy[1];
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    int socket = Main->c_link->socket;

    if (gbcm_write_two(socket,GBCM_COMMAND_COMMIT_TRANSACTION,gbd->server_id) ) {
        return GB_export_error("Cannot send '%s' to server",GB_KEY(gbd));
    }
    if (gbcm_write_flush(socket)) {
        return GB_export_error("ARB_DB CLIENT ERROR send failed");
    }
    gbcm_read_two(socket,GBCM_COMMAND_TRANSACTION_RETURN,0,dummy);
    gbcm_read_flush(socket);
    return 0;
}
GB_ERROR gbcmc_abort_transaction(GBDATA *gbd)
{
    long dummy[1];
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    int socket = Main->c_link->socket;
    if (gbcm_write_two(socket,GBCM_COMMAND_ABORT_TRANSACTION,gbd->server_id) ) {
        return GB_export_error("Cannot send '%s' to server",GB_KEY(gbd));
    }
    if (gbcm_write_flush(socket)) {
        return GB_export_error("ARB_DB CLIENT ERROR send failed");
    }
    gbcm_read_two(socket,GBCM_COMMAND_TRANSACTION_RETURN,0,dummy);
    gbcm_read_flush(socket);
    return 0;
}

/**************************************************************************************
                send update to client
***************************************************************************************/

GB_ERROR gbcms_add_to_delete_list(GBDATA *gbd)
{
    struct Hs_struct *hs;
    struct gbcms_delete_list    *dl;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    hs = (struct Hs_struct *)Main->server_data;
    if (!hs) return 0;
    if (!hs->soci) return 0;
    dl = (struct gbcms_delete_list *)gbm_get_mem(sizeof(struct gbcms_delete_list),GBM_CB_INDEX);
    dl->creation_date = GB_GET_EXT_CREATION_DATE(gbd);
    dl->update_date = GB_GET_EXT_UPDATE_DATE(gbd);
    dl->gbd = gbd;
    if (!hs->del_first) {
        hs->del_first = hs->del_last = dl;
    }else{
        hs->del_last->next = dl;
        hs->del_last = dl;
    }
    return 0;
}

/**************************************************************************************
                How many clients, returns -1 if not server
***************************************************************************************/
long GB_read_clients(GBDATA *gbd)
{
    GB_MAIN_TYPE *Main    = GB_MAIN(gbd);
    long          clients = -1;

    if (Main->local_mode) { /* i am the server */
        struct Hs_struct *hs = (struct Hs_struct *)Main->server_data;
        clients = hs ? hs->nsoc : 0;
    }

    return clients;
}

GB_BOOL GB_is_server(GBDATA *gbd) {
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    return Main->local_mode;
}
GB_BOOL GB_is_client(GBDATA *gbd) {
    return !GB_is_server(gbd);
}

/**************************************************************************************
                Query in the server
***************************************************************************************/
GB_ERROR
gbcmc_unfold_list(int socket, GBDATA * gbd)
{
    long      readvar[2];
    GBCONTAINER         *gb_client;
    GB_ERROR error;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    if (!gbcm_read(socket, (char *) readvar, sizeof(long) * 2 )) {
        return GB_export_error("receive failed");
    }
    gb_client = (GBCONTAINER *) readvar[1];
    if (gb_client) {
        error = gbcmc_unfold_list(socket, gbd);
        if (error)
            return error;
        gb_client = (GBCONTAINER *) GBS_read_hashi(Main->remote_hash, (long) gb_client);
        gb_unfold(gb_client, 0, (int)readvar[0]);
    }
    return 0;
}

GBDATA *GBCMC_find(GBDATA *gbd, const char *key, GB_TYPES type, const char *str, enum gb_search_types gbs) {
    /* perform search in DB server (from DB client) */
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

    if (gbcm_write_two(socket,GBCM_COMMAND_FIND,gbd->server_id)){
        GB_export_error("Cannot send data to Server");
        GB_print_error();
        return 0;
    }

    gbcm_write_string(socket,key);
    gbcm_write_long(socket, type);
    switch (type) {
        case GB_STRING:
            gbcm_write_string(socket,str);
            break;
        case GB_INT:
            gbcm_write_long(socket, *(long*)str);
            break;
        default :
            gb_assert(0);
            GB_export_error(GBS_global_string("GBCMC_find: Illegal data type (%i)", type));
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
    if (result.gbd){
        gbcmc_unfold_list(socket,gbd);
        result.l = GBS_read_hashi(Main->remote_hash, result.l);
    }
    gbcm_read_flush(socket);
    return result.gbd;
}


long gbcmc_key_alloc(GBDATA *gbd,const char *key)   {
    long gb_result[1];
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);

    int socket;
    if (Main->local_mode) return 0;
    socket = Main->c_link->socket;

    if (gbcm_write_two(socket,GBCM_COMMAND_KEY_ALLOC,gbd->server_id)){
        GB_export_error("Cannot send data to Server");
        GB_print_error();
        return 0;
    }

    gbcm_write_string(socket,key);

    if (gbcm_write_flush(socket)) {
        GB_export_error("ARB_DB CLIENT ERROR send failed");
        GB_print_error();
        return 0;
    }
    gbcm_read_two(socket,GBCM_COMMAND_KEY_ALLOC_RES,0,gb_result);
    gbcm_read_flush(socket);
    return gb_result[0];
}

int GBCMC_system(GBDATA *gbd,const char *ss) {

    int socket;
    long    gb_result[2];
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    if (Main->local_mode){
        printf("Action: '%s'\n",ss);
        if (system(ss)){
            if (strlen(ss) < 1000){
                GB_export_error("Cannot run '%s'",ss);
            }
            return 1;
        }
        return 0;
    };
    socket = Main->c_link->socket;

    if (gbcm_write_two(socket,GBCM_COMMAND_SYSTEM,gbd->server_id)){
        GB_export_error("Cannot send data to Server");
        GB_print_error();
        return -1;
    }

    gbcm_write_string(socket,ss);
    if (gbcm_write_flush(socket)) {
        GB_export_error("ARB_DB CLIENT ERROR send failed");
        GB_print_error();
        return -1;
    }
    gbcm_read_two(socket,GBCM_COMMAND_SYSTEM_RETURN,0,(long *)gb_result);
    gbcm_read_flush(socket);
    return 0;
}

/** send an undo command !!! */
GB_ERROR gbcmc_send_undo_commands(GBDATA *gbd, enum gb_undo_commands command){
    int socket;
    GB_ERROR result;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    if (Main->local_mode)
        GB_internal_error("gbcmc_send_undo_commands: cannot call a server in a server");
    socket = Main->c_link->socket;

    if (gbcm_write_two(socket,GBCM_COMMAND_UNDO,gbd->server_id)){
        return GB_export_error("Cannot send data to Server 456");
    }
    if (gbcm_write_two(socket,GBCM_COMMAND_UNDO_CMD,command)){
        return GB_export_error("Cannot send data to Server 96f");
    }
    if (gbcm_write_flush(socket)) {
        return GB_export_error("Cannot send data to Server 536");
    }
    result = gbcm_read_string(socket);
    gbcm_read_flush(socket);
    if (result) GB_export_error("%s",result);
    return result;
}

char *gbcmc_send_undo_info_commands(GBDATA *gbd, enum gb_undo_commands command){
    int socket;
    char *result;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    if (Main->local_mode){
        GB_internal_error("gbcmc_send_undo_commands: cannot call a server in a server");
        return 0;
    }
    socket = Main->c_link->socket;

    if (gbcm_write_two(socket,GBCM_COMMAND_UNDO,gbd->server_id)){
        GB_export_error("Cannot send data to Server 456");
        return 0;
    }
    if (gbcm_write_two(socket,GBCM_COMMAND_UNDO_CMD,command)){
        GB_export_error("Cannot send data to Server 96f");
        return 0;
    }
    if (gbcm_write_flush(socket)) {
        GB_export_error("Cannot send data to Server 536");
        return 0;
    }
    result = gbcm_read_string(socket);
    gbcm_read_flush(socket);
    return result;
}

GB_ERROR GB_tell_server_dont_wait(GBDATA *gbd){
    int socket;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);

    if (Main->local_mode){
        return 0;
    }
    socket = Main->c_link->socket;
    if (gbcm_write_two(socket,GBCM_COMMAND_DONT_WAIT,gbd->server_id)){
        GB_export_error("Cannot send data to Server 456");
        return 0;
    }

    return 0;
}

/**************************************************************************************
                Login logout functions
***************************************************************************************/

GB_ERROR gbcm_login(GBCONTAINER *gb_main,const char *user)
     /* look for any free user and set this_user */
{
    int i;
    GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(gb_main);

    for (i = 0; i<GB_MAX_USERS; i++) {
        if (!Main->users[i]) continue;
        if (!strcmp(user,Main->users[i]->username) ) {
            Main->this_user = Main->users[i];
            Main->users[i]->nusers++;
            return 0;
        }
    }
    for (i = 0; i<GB_MAX_USERS; i++) {
        if (Main->users[i]) continue;
        Main->users[i] = (struct gb_user_struct *) GB_calloc(sizeof(struct gb_user_struct),1);
        Main->users[i]->username = GB_STRDUP(user);
        Main->users[i]->userid = i;
        Main->users[i]->userbit = 1<<i;
        Main->users[i]->nusers = 1;
        Main->this_user = Main->users[i];
        return 0;
    }
    return GB_export_error("Too many users in this database: User '%s' ",user);
}

long gbcmc_close(struct gbcmc_comm * link)
{
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
    if (link->unix_name) free(link->unix_name); /* @@@ */
    free((char *)link);
    return 0;
}

GB_ERROR gbcm_logout(GBCONTAINER *gb_main,char *user)
{
    long i;
    GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(gb_main);

    for (i = 0; i<GB_MAX_USERS; i++) {
        if (!Main->users[i]) continue;
        if (!strcmp(user,Main->users[i]->username) ) {
            Main->users[i]->nusers--;
            if (Main->users[i]->nusers<=0) {/* kill user and his projects */
                free(Main->users[i]->username);
                free((char *)Main->users[i]);
                Main->users[i] = 0;
                fprintf(stdout,"The User %s has logged out\n",user);
            }
            return 0;
        }
    }
    return GB_export_error("User '%s' not logged in",user);
}

GB_CSTR GBC_get_hostname(void){
    static char *hn = 0;
    if (!hn){
        char buffer[4096];
        gethostname(buffer,4095);
        hn = strdup(buffer);
    }
    return hn;
}

GB_ERROR GB_install_pid(int mode)   /* tell the arb_clean software what
                                       programs are running !!!
                                       mode == 1 install
                                       mode == 0 never install */
{
    static long lastpid = 0;
    long pid = getpid();
    FILE *pidfile;
    char filename[1000];
    const char *user = GB_getenvUSER();
    const char *arb_pid = GB_getenv("ARB_PID");
    if (!user) user = "UNKNOWN";
    if (!arb_pid) arb_pid = "";
    if (mode ==0) lastpid = -25;
    if (lastpid == pid) return 0;
    if (lastpid == -25) return 0;   /* never install */
    lastpid = pid;
    sprintf(filename,"/tmp/arb_pids_%s_%s",user,arb_pid);
    pidfile = fopen(filename,"a");
    if (!pidfile) return GB_export_error("Cannot open pid file '%s'",filename);
    fprintf(pidfile,"%li ",pid);
    fclose(pidfile);
    return 0;
}
