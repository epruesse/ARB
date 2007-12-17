#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>
#include <errno.h>
/* #include <malloc.h> */
#include <string.h>
#include <memory.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <signal.h>
#include <time.h>

#include "adlocal.h"
/*#include "arbdb.h"*/

#if defined(__cplusplus)
extern "C" {
#   include <sys/mman.h>
}
#else
#   include <sys/mman.h>
#endif

#if defined(SUN4) || defined(SUN5)
# ifndef __cplusplus
#  define SIG_PF void (*)()
# endif
#else
# define SIG_PF void (*)(int )
#endif


/************************************ signal handling ****************************************/
/************************************ valid memory tester ************************************/
int gbcm_sig_violation_flag;
int gbcm_pipe_violation_flag;

# if defined(SUN5)
void GB_usleep(long usec){
    struct timespec timeout,time2;
    timeout.tv_sec  = 0;
    timeout.tv_nsec = usec*1000;
    nanosleep(&timeout,&time2);
}
# else
#  if defined(DIGITAL)
#   if defined(__cplusplus)
extern "C" {    extern void usleep(unsigned int );  }
#   else
extern void usleep(unsigned int);
#   endif
#  endif // DIGITAL
void GB_usleep(long usec){
    usleep(usec);
}
# endif

long gbcm_sig_violation_end();

GB_ERROR gbcm_test_address(long *address,long key)
{
    /* tested ob die Addresse address erlaubt ist,
       falls ja, dann return NULL, sonst Fehlerstring */
    /* Falls key != NULL, tested ob *address == key */
    long i;
    gbcm_sig_violation_flag = 0;
    i = *address;
    if (gbcm_sig_violation_flag) {
        fprintf(stderr,"MEMORY MANAGER ERROR: SIGNAL SEGV;      ADDRESS 0x%lx",(long)address);
        return "MEMORY MANAGER ERROR";
    }
    if (key){
        if (i!=key) {
            fprintf(stderr,"MEMORY MANAGER ERROR: OBJECT KEY (0x%lx) IS NOT OF TYPE 0x%lx",i,key);
            return "MEMORY MANAGER ERROR";
        }
    }
    return NULL;
}


long gbcm_test_address_end()
{
    return 1;
}


void *gbcm_sig_violation(long sig,long code,long *scpin,char *addr)
{
    struct sigcontext *scp= (struct sigcontext *)scpin;
#ifdef SUN4
    const long a = (const long)gbcm_test_address;
    const long e = (const long)gbcm_test_address_end;
    gbcm_sig_violation_flag =1;

    if ( (scp->sc_pc<a) || (scp->sc_pc>e) ){
        signal(SIGSEGV,SIG_DFL);        /* make core */
        return 0;
    }
    scp->sc_pc = scp->sc_npc;
#else
    GBUSE(scp);
#endif
    sig = sig;
    code = code;
    addr = addr;
    return 0;
}

/**************************************************************************************
********************************    valid memory tester (end) *************************
***************************************************************************************/

void *gbcms_sigpipe()
{
    gbcm_pipe_violation_flag = 1;
    return 0;
}
/**************************************************************************************
                private read and write socket functions
***************************************************************************************/
void gbcm_read_flush(int socket)
{gb_local->write_ptr = gb_local->write_buffer;
 gb_local->write_free = gb_local->write_bufsize;
 socket = socket;}

long gbcm_read_buffered(int socket,char *ptr, long size)
{
    /* write_ptr ptr to not read data
       write_free   = write_bufsize-size of non read data;
    */
    long holding;
    holding = gb_local->write_bufsize - gb_local->write_free;
    if (holding <= 0) {
        holding = read(socket,gb_local->write_buffer,(size_t)gb_local->write_bufsize);

        if (holding < 0)
        {
            fprintf(stderr,"Cannot read data from client: len=%li (%s, errno %i)\n",
                    holding, strerror(errno), errno);
            return 0;
        }
        gbcm_read_flush(socket);
        gb_local->write_free-=holding;
    }
    if (size>holding) size = holding;
    GB_MEMCPY(ptr,gb_local->write_ptr,(int)size);
    gb_local->write_ptr += size;
    gb_local->write_free+= size;
    return size;
}

long gbcm_read(int socket,char *ptr,long size)
{
    long    leftsize,readsize;
    leftsize = size;
    readsize = 0;

    while (leftsize) {
        readsize = gbcm_read_buffered(socket,ptr,leftsize);
        if (readsize<=0) return 0;
        ptr += readsize;
        leftsize -= readsize;
    }

    return size;
}

int
gbcm_write_flush(int socket)
{
    char *ptr;
    long    leftsize,writesize;
    ptr = gb_local->write_buffer;
    leftsize = gb_local->write_ptr - ptr;
    gb_local->write_free = gb_local->write_bufsize;
    if (!leftsize) return GBCM_SERVER_OK;

    gb_local->write_ptr = ptr;
    gbcm_pipe_violation_flag = 0;

    writesize = write(socket,ptr,(size_t)leftsize);

    if (gbcm_pipe_violation_flag || writesize<0) {
        if (  gb_local->iamclient ) {
            fprintf(stderr,"DB_Server is killed, Now I kill myself\n");
            exit(-1);
        }
        fprintf(stderr,"writesize: %li ppid %i\n",writesize,getppid());
        return GBCM_SERVER_FAULT;
    }
    ptr += writesize;
    leftsize = leftsize - writesize;

    while (leftsize) {

        GB_usleep(10000);

        writesize = write(socket,ptr,(size_t)leftsize);
        if (gbcm_pipe_violation_flag || writesize<0) {
            if ( (int)getppid() <=1 ) {
                fprintf(stderr,"DB_Server is killed, Now I kill myself\n");
                exit(-1);
            }
            fprintf(stderr,"write error\n");
            return GBCM_SERVER_FAULT;
        }
        ptr += writesize;
        leftsize -= writesize;
    }
    return GBCM_SERVER_OK;
}

int gbcm_write(int socket,const char *ptr,long size) {

    while  (size >= gb_local->write_free){
        GB_MEMCPY(gb_local->write_ptr, ptr, (int)gb_local->write_free);
        gb_local->write_ptr += gb_local->write_free;
        size -= gb_local->write_free;
        ptr += gb_local->write_free;

        gb_local->write_free = 0;
        if (gbcm_write_flush(socket)) return GBCM_SERVER_FAULT;
    }
    GB_MEMCPY(gb_local->write_ptr, ptr, (int)size);
    gb_local->write_ptr += size;
    gb_local->write_free -= size;
    return GBCM_SERVER_OK;
}



void *gbcm_sigio()
{
    return 0;
}

/************************************* find the mach name and id *************************************/

GB_ERROR gbcm_get_m_id(const char *path, char **m_name, long *id)
{
    const char *p;
    char       *mn;
    long        i;

    if (!path) {
        return "OPEN_ARB_DB_CLIENT ERROR: missing hostname:socketid";
    }
    if (strcmp(path,":") == 0) {
        path = GBS_read_arb_tcp("ARB_DB_SERVER");
        if (!path) return GB_get_error();
    }
    p = strchr(path, ':');
    if (path[0] == '*' || path[0] == ':'){  /* UNIX MODE */
        if (!p) {
            return GB_export_error("OPEN_ARB_DB_CLIENT ERROR: missing ':' in %s",path);
        }
        *m_name = GB_STRDUP(p+1);
        *id = -1;
        return 0;
    }
    if (!p) {
        return GB_export_error("OPEN_ARB_DB_CLIENT ERROR: missing ':' in %s",path);
    }
    mn = (char *)GB_calloc(sizeof(char), p - path + 1);
    strncpy(mn, path, p - path);
    *m_name = mn;
    i = atoi(p + 1);
    if ((i < 1) || (i > 4096)) {
        return GB_export_error("OPEN_ARB_DB_CLIENT ERROR: socketnumber %li not in [1024..4095]",i);
    }
    *id = i;
    return 0;
}

GB_ERROR
gbcm_open_socket(const char *path, long delay2, long do_connect, int *psocket, char **unix_name)
{
    long      socket_id[1];
    char    *mach_name[1];
    struct in_addr  addr;   /* union -> u_long  */
    struct hostent *he;
    GB_ERROR err;

    mach_name[0] = 0;
    err = gbcm_get_m_id(path, mach_name, socket_id);
    if (err) {
        if (mach_name[0]) free((char *)mach_name[0]);
        return err;
    }
    if (socket_id[0] >= 0) {    /* TCP */
        struct sockaddr_in so_ad;
        memset((char *)&so_ad,0,sizeof(struct sockaddr_in));
        *psocket = socket(PF_INET, SOCK_STREAM, 0);
        if (*psocket <= 0) {
            return "CANNOT CREATE SOCKET";
        }
        he = gethostbyname(mach_name[0]);

        if (!he) {
            return "Unknown host";
        }

        /** simply take first address **/
        addr.s_addr = *(long *) (he->h_addr);
        so_ad.sin_addr = addr;
        so_ad.sin_family = AF_INET;
        so_ad.sin_port = htons((unsigned short)(socket_id[0])); /* @@@ = pb_socket  */
        if (do_connect){
            /*printf("Connecting to %X:%i\n",addr.s_addr,socket_id[0]);*/
            if (connect((int)*psocket,(struct sockaddr *)(&so_ad), sizeof(so_ad))) {
                GB_warning("Cannot connect to %s:%li   errno %i",mach_name[0],socket_id[0],errno);
                return "";
            }
        }else{
            int one = 1;
            setsockopt((int)(*psocket),SOL_SOCKET,SO_REUSEADDR,(const char *)&one,sizeof(one));
            if (bind((int)(*psocket),(struct sockaddr *)(&so_ad),sizeof(so_ad))){
                return "Could not open socket on Server";
            }
        }
        if (mach_name[0]) free((char *)mach_name[0]);
        if (delay2 == TCP_NODELAY) {
            GB_UINT4      optval;
            optval = 1;
            setsockopt((int)(*psocket), IPPROTO_TCP, TCP_NODELAY, (char *)&optval, sizeof (GB_UINT4));
        }
        *unix_name = 0;
        return 0;
    } else {        /* UNIX */
        struct sockaddr_un so_ad; memset((char *)&so_ad,0,sizeof(so_ad));
        *psocket = socket(PF_UNIX, SOCK_STREAM, 0);
        if (*psocket <= 0) {
            return "CANNOT CREATE SOCKET";
        }
        so_ad.sun_family = AF_UNIX;
        sprintf(so_ad.sun_path,mach_name[0]);

        if (do_connect){
            if (connect((int)(*psocket), (struct sockaddr*)(&so_ad), strlen(so_ad.sun_path)+2)) {
                if (mach_name[0]) free((char *)mach_name[0]);
                return "";
            }
        }else{
#if 0
            FILE    *test;
            test = fopen(mach_name[0],"r");
            if (test) {
                fclose(test);
                if (GB_is_regularfile(mach_name[0])){
                    GB_ERROR error = 0;
                    error = GB_export_error("Socket '%s' already exists as a file",mach_name[0]);
                    free((char *)mach_name[0]);
                    return error;
                }
            }
#endif
            if (unlink(mach_name[0])) {
                ;
            }else{
                printf("old socket found\n");
            }
            if (bind(*psocket,(struct sockaddr*)(&so_ad),strlen(mach_name[0])+2)){
                if (mach_name[0]) free((char *)mach_name[0]);
                return "Could not open socket on Server";
            }
            if (chmod(mach_name[0],0777)) return GB_export_error("Cannot change mode of socket '%s'",mach_name[0]);
        }
        *unix_name = mach_name[0];
        return 0;
    }
}


long    gbcms_close(struct gbcmc_comm *link)
{
    if (link->socket) {
        close(link->socket);
        link->socket = 0;
        if (link->unix_name) {
            unlink(link->unix_name);
        }
    }
    return 0;
}

struct gbl_param *ppara;
struct gbcmc_comm *gbcmc_open(const char *path)
{
    struct gbcmc_comm *link;
    GB_ERROR err;

    link = (struct gbcmc_comm *)GB_calloc(sizeof(struct gbcmc_comm), 1);
    err = gbcm_open_socket(path, TCP_NODELAY, 1, &link->socket, &link->unix_name);
    if (err) {
        if (link->unix_name) free(link->unix_name); /* @@@ */
        free((char *)link);
        if(*err){
            GB_internal_error(GBS_global_string("ARB_DB_CLIENT_OPEN\n(Reason: %s)", err));
        }
        return 0;
    }
    /*  signal(SIGSEGV, (SIG_PF) gbcm_sig_violation); */
    signal(SIGPIPE, (SIG_PF) gbcm_sigio);
    gb_local->iamclient = 1;
    return link;
}

/**************************************************************************************
                write and read triples
***************************************************************************************/

long gbcm_write_two(int socket, long a, long c)
{
    long    ia[3];
    ia[0] = a;
    ia[1] = 3;
    ia[2] = c;
    if (!socket) return 1;
    return  gbcm_write(socket, (const char *)ia, sizeof(long)*3);
}


/** read two values:    length and any user long
 *  if data are send be gbcm_write_two than b should be zero
 *  and is not used !!! */

long gbcm_read_two(int socket, long a, long *b, long *c)
{
    long    ia[3];
    long    size;
    size = gbcm_read(socket,(char *)&(ia[0]),sizeof(long)*3);
    if (size != sizeof(long) * 3) {
        GB_internal_error("receive failed: %i bytes expected, %li got, keyword %lX",
                          sizeof(long) * 3,size, a );
        return GBCM_SERVER_FAULT;
    }
    if (ia[0] != a) {
        GB_internal_error("received keyword failed %lx != %lx\n",ia[0],a);
        return GBCM_SERVER_FAULT;
    }
    if (b) {
        *b = ia[1];
    }else{
        if (ia[1]!=3) {
            GB_internal_error("receive failed: size not 3\n");
            return GBCM_SERVER_FAULT;
        }
    }
    *c = ia[2];
    return GBCM_SERVER_OK;
}

long gbcm_write_string(int socket, const char *key)
{
    if (key) {
        size_t len = strlen(key);
        gbcm_write_long(socket, len);
        if (len) gbcm_write(socket,key,len);
    }
    else {
        gbcm_write_long(socket, -1);
    }
    return GBCM_SERVER_OK;
}

char *gbcm_read_string(int socket)
{
    char *key;
    long  strlen = gbcm_read_long(socket);

    if (strlen) {
        if (strlen>0) {
            key = (char *)GB_calloc(sizeof(char), (size_t)strlen+1);
            gbcm_read(socket, key, strlen);
        }
        else {
            key = 0;
        }
    }
    else {
        key = GB_strdup("");
    }

    return key;
}

long gbcm_write_long(int socket, long data) {
    gbcm_write(socket, (char*)&data, sizeof(data));
    return GBCM_SERVER_OK;
}

long gbcm_read_long(int socket) {
    long data;
    gbcm_read(socket, (char*)&data, sizeof(data));
    return data;
}


struct stat gb_global_stt;

GB_ULONG GB_time_of_file(const char *path)
{
    if (path){
        char *path2 = GBS_eval_env(path);
        if (stat(path2, &gb_global_stt)){
            free(path2);
            return 0;
        }
        free(path2);
    }
    return gb_global_stt.st_mtime;
}

long GB_size_of_file(const char *path)
{
    if (path) if (stat(path, &gb_global_stt)) return -1;
    return gb_global_stt.st_size;
}

long GB_mode_of_file(const char *path)
{
    if (path) if (stat(path, &gb_global_stt)) return -1;
    return gb_global_stt.st_mode;
}

long GB_mode_of_link(const char *path)
{
    if (path) if (lstat(path, &gb_global_stt)) return -1;
    return gb_global_stt.st_mode;
}

int GB_is_regularfile(const char *path){
    struct stat stt;
    if (stat(path, &stt)) return 0;
    if (S_ISREG(stt.st_mode)) return 1;
    return 0;
}

int GB_is_executablefile(const char *path) {
    struct stat stt;    
    if (stat(path, &stt)) return 0;

    {
        uid_t my_userid = geteuid(); // effective user id
        if (stt.st_uid == my_userid) { // I am the owner of the file
            return !!(stt.st_mode&S_IXUSR); // owner execution permission
        }
    }
    {
        gid_t my_groupid = getegid(); // effective group id
        if (stt.st_gid == my_groupid) { // I am member of the file's group
            return !!(stt.st_mode&S_IXGRP); // group execution permission
        }
    }

    return !!(stt.st_mode&S_IXOTH); // others execution permission
}

int GB_is_directory(const char *path){
    struct stat stt;
    if (stat(path, &stt)) return 0;
    if (S_ISDIR(stt.st_mode)) return 1;
    return 0;
}


long GB_getuid(){
    return getuid();
}

long GB_getpid(){
    return getpid();
}

long GB_getuid_of_file(char *path){
    struct stat stt;
    if (stat(path, &stt)) return -1;
    return stt.st_uid;
}

int GB_unlink(const char *path)
{   /* returns
       0    success
       1    File does not exist
       -1   Error
    */
    if (unlink(path)) {
        if (errno == ENOENT) return 1;
        perror(0);
        GB_export_error("Cannot remove %s",path);
        return -1;
    }
    return 0;
}

char *GB_follow_unix_link(const char *path){    /* returns the real path of a file */
    char buffer[1000];
    char *path2;
    char *pos;
    char *res;
    int len = readlink(path,buffer,999);
    if (len<0) return 0;
    buffer[len] = 0;
    if (path[0] == '/') return GB_STRDUP(buffer);

    path2 = GB_STRDUP(path);
    pos = strrchr(path2,'/');
    if (!pos){
        free(path2);
        return GB_STRDUP(buffer);
    }
    *pos = 0;
    res =  GB_STRDUP(GBS_global_string("%s/%s",path2,buffer));
    free(path2);
    return res;
}

GB_ERROR GB_symlink(const char *name1, const char *name2){  /* name1 is the existing file !!! */
    if (symlink(name1,name2)<0){
        return GB_export_error("Cannot create symlink '%s' to file '%s'",name2,name1);
    }
    return 0;
}

GB_ERROR GB_set_mode_of_file(const char *path,long mode)
{
    if (chmod(path, (int)mode)) return GB_export_error("Cannot change mode of '%s'",path);
    return 0;
}

GB_ERROR GB_rename_file(const char *oldpath,const char *newpath){
    long old_mod = GB_mode_of_file(newpath);
    if (old_mod == -1) old_mod = GB_mode_of_file(oldpath);
    if (rename(oldpath,newpath)) {
        return GB_export_error("Cannot rename '%s' to '%s'",oldpath,newpath);
    }
    if (GB_set_mode_of_file(newpath,old_mod)) {
        return GB_export_error("Cannot set modes of '%s'",newpath);
    }
    sync();
    return 0;
}

/********************************************************************************************
                    read a file to memory
********************************************************************************************/
char *GB_read_file(const char *path)
{
    FILE *input;
    char *epath = 0;
    long data_size;
    char *buffer = 0;

    if (!strcmp(path,"-")) {
        /* stdin; */
        int c;
        void *str = GBS_stropen(1000);
        c = getc(stdin);
        while (c!= EOF) {
            GBS_chrcat(str,c);
            c = getc(stdin);
        }
        return GBS_strclose(str);
    }
    epath = GBS_eval_env(path);

    if ((input = fopen(epath, "r")) == NULL) {
        GB_export_error("File %s=%s not found",path,epath);
        free(epath);
        return NULL;
    }else{
        data_size = GB_size_of_file(epath);
        buffer =  (char *)malloc((size_t)(data_size+1));
        data_size = fread(buffer,1,(size_t)data_size,input);
        buffer[data_size] = 0;
        fclose(input);
    }
    free(epath);
    return buffer;
}

char *GB_map_FILE(FILE *in,int writeable){
    int fi = fileno(in);
    long size = GB_size_of_FILE(in);
    char *buffer;
    if (size<=0) {
        GB_export_error("GB_map_file: sorry file not found");
        return 0;
    }
    if (writeable){
        buffer = (char*)mmap(0, (int)size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fi, 0);
    }else{
        buffer = (char*)mmap(0, (int)size, PROT_READ, MAP_SHARED, fi, 0);
    }
    if (!buffer){
        GB_export_error("GB_map_file: Error Out of Memory: mmap failes ");
        return 0;
    }
    return buffer;
}

char *GB_map_file(const char *path,int writeable){
    FILE *in;
    char *buffer;
    in = fopen(path,"r");
    if (!in) {
        GB_export_error("GB_map_file: sorry file '%s' not readable",path);
        return 0;
    }
    buffer = GB_map_FILE(in,writeable);
    fclose(in);
    return buffer;
}

long GB_size_of_FILE(FILE *in){
    int fi = fileno(in);
    struct stat st;
    if (fstat(fi, &st)) {
        GB_export_error("GB_size_of_FILE: sorry file is not readable");
        return -1;
    }
    return st.st_size;
}


GB_ULONG GB_time_of_day(void){
    struct timeval tp;
    if (gettimeofday(&tp,0)) return 0;
    return tp.tv_sec;
}

long GB_last_saved_clock(GBDATA *gb_main){
    return GB_MAIN(gb_main)->last_saved_transaction;
}

GB_ULONG GB_last_saved_time(GBDATA *gb_main){
    return GB_MAIN(gb_main)->last_saved_time;
}

void GB_textprint(const char *path){
    char buffer[1024];
    char *fpath = GBS_eval_env(path);
    sprintf(buffer, "%s %s &","arb_textprint",fpath);
    GB_information("Executing '%s'", buffer);
    system(buffer);
    free(fpath);
}

GB_CSTR GB_getcwd(void){
    static char *lastcwd = 0;
    if (!lastcwd) lastcwd = (char *)getcwd(0,GB_PATH_MAX);
    return lastcwd;
}

void GB_xterm(void){
    char buffer[1024];
    const char *xt = GB_getenv("ARB_XTERM"); // doc in arb_envar.hlp
    if (!xt) xt = "xterm -sl 1000 -sb -geometry 120x40";
    sprintf(buffer, "%s &",xt);
    system(buffer);
}

void GB_xcmd(const char *cmd, GB_BOOL background, GB_BOOL wait_only_if_error) {
    // runs a command in an xterm
    // if 'background' is true -> run asyncronous
    // if 'wait_only_if_error' is true -> asyncronous does wait for keypress only if cmd fails

    void       *strstruct = GBS_stropen(1024);
    const char *xt        = GB_getenv("ARB_XCMD"); // doc in arb_envar.hlp
    char       *sys;
    if (!xt) xt           = "xterm -sl 1000 -sb -geometry 140x30 -e";
    GBS_strcat(strstruct, "(");
    GBS_strcat(strstruct, xt);
    GBS_strcat(strstruct, " sh -c 'LD_LIBRARY_PATH=\"");
    GBS_strcat(strstruct,GB_getenv("LD_LIBRARY_PATH"));
    GBS_strcat(strstruct,"\";export LD_LIBRARY_PATH; (");
    GBS_strcat(strstruct, cmd);
    if (background) {
        if (wait_only_if_error) {
            GBS_strcat(strstruct, ") || (echo; echo Press RETURN to close Window; read a)' ) &");
        }
        else {
            GBS_strcat(strstruct, "; echo; echo Press RETURN to close Window; read a)' ) &");
        }
    }
    else {
        if (wait_only_if_error) {
            GBS_strcat(strstruct, ") || (echo; echo Press RETURN to close Window; read a)' )");
        }
        else { // no wait
            GBS_strcat(strstruct, " )' ) ");
        }
    }
    sys = GBS_strclose(strstruct);
    GB_information("Executing '%s'", sys);
    system(sys);
    free(sys);
}

/* -------------------------------------------------------------------------------- */
/* Functions to find an executable */

static char *search_executable(GB_CSTR exe_name) {
    GB_CSTR     path   = GB_getenvPATH();
    char       *buffer = GB_give_buffer(strlen(path)+1+strlen(exe_name)+1);
    const char *start  = path;
    int         found  = 0;

    while (!found && start) {
        const char *colon = strchr(start, ':');
        int         len   = colon ? (colon-start) : (int)strlen(start);

        memcpy(buffer, start, len);
        buffer[len] = '/';
        strcpy(buffer+len+1, exe_name);

        found = GB_is_executablefile(buffer);
        start = colon ? colon+1 : 0;
    }

    return found ? strdup(buffer) : 0;
}

char *GB_find_executable(GB_CSTR description_of_executable, ...) {
    /* search the path for an executable with any of the given names (...)
     * if any is found, it's full path is returned
     * if none is found, a warning call is returned (which can be executed without harm)
    */

    GB_CSTR  name;
    char    *found = 0;
    va_list  args;

    va_start(args, description_of_executable);
    while (!found && (name = va_arg(args, GB_CSTR)) != 0) found = search_executable(name);
    va_end(args);

    if (!found) { /* none of the executables has been found */
        char *looked_for;
        char *msg;
        {
            void *buf   = GBS_stropen(100);
            int   first = 1;

            va_start(args, description_of_executable);
            while ((name = va_arg(args, GB_CSTR)) != 0) {
                if (!first) GBS_strcat(buf, ", ");
                first = 0;
                GBS_strcat(buf, name);
            }
            va_end(args);
            looked_for = GBS_strclose(buf);
        }

        msg   = GBS_global_string_copy("Could not find a %s (looked for: %s)", description_of_executable, looked_for);
        GB_warning(msg);
        found = GBS_global_string_copy("echo \"%s\" ; arb_ign Parameters", msg);
        free(msg);
        free(looked_for);
    }
    else {
        GB_information("Using %s '%s' ('%s')", description_of_executable, name, found);
    }
    return found;
}

/* -------------------------------------------------------------------------------- */
/* Functions to access the environment variables used by ARB: */

static GB_CSTR getenv_ignore_empty(GB_CSTR envvar) {
    GB_CSTR result = getenv(envvar);
    return (result && result[0]) ? result : 0;
}

static char *getenv_executable(GB_CSTR envvar) {
    // get full path of executable defined by 'envvar'
    // returns 0 if
    //  - envvar not defined or
    //  - not defining an executable (warns about that)

    char       *result   = 0;
    const char *exe_name = getenv_ignore_empty(envvar);

    if (exe_name) {
        result = search_executable(exe_name);
        if (!result) {
            GB_warning("Environment variable '%s' contains '%s' (which is not an executable)", envvar, exe_name);
        }
    }

    return result;
}

static char *getenv_existing_directory(GB_CSTR envvar) {
    // get full path of directory defined by 'envvar'
    // return 0 if
    // - envvar is not defined or
    // - does not point to a directory (warns about that)

    char       *result   = 0;
    const char *dir_name = getenv_ignore_empty(envvar);

    if (dir_name) {
        if (GB_is_directory(dir_name)) {
            result = strdup(dir_name);
        }
        else {
            GB_warning("Environment variable '%s' should contain the path of an existing directory.\n"
                       "(current content '%s' has been ignored.)", envvar, dir_name);
        }
    }
    return result;
}

GB_CSTR GB_getenvUSER(void) {
    static const char *user = 0;
    if (!user) {
        user = getenv_ignore_empty("USER");
        if (!user) user = getenv_ignore_empty("LOGNAME");
        if (!user) {
            user = getenv_ignore_empty("HOME");
            if (user && strrchr(user,'/')) user = strrchr(user,'/')+1;
        }
        if (!user) {
            fprintf(stderr, "WARNING: Cannot identify user: environment variables USER, LOGNAME and HOME not set\n");
            user = "UnknownUser";
        }
    }
    return user;
}


GB_CSTR GB_getenvHOME(void) {
    static const char *home = 0;
    if (!home){
        home = getenv_existing_directory("HOME");
        if (!home) {
            home = GB_getcwd();
            if (!home) home = ".";
            fprintf(stderr, "WARNING: Cannot identify user's home directory: environment variable HOME not set\n"
                    "Using current directory (%s) as home.\n", home);
        }
    }
    return home;
}

GB_CSTR GB_getenvARBHOME(void) {
    static char *arbhome = 0;
    if (!arbhome) {
        arbhome = getenv_existing_directory("ARBHOME"); // doc in arb_envar.hlp
        if (!arbhome){
            fprintf(stderr, "ERROR: Environment Variable ARBHOME not found !!!\n"
                    "   Please set 'ARBHOME' to the installation path of ARB\n");
            exit(-1);
        }
    }
    return arbhome;
}

GB_CSTR GB_getenvARBMACRO(void) {
    static const char *am = 0;
    if (!am) {
        am          = getenv_existing_directory("ARBMACRO"); // doc in arb_envar.hlp
        if (!am) am = GBS_eval_env("$(ARBHOME)/lib/macros");
    }
    return am;
}

GB_CSTR GB_getenvARBMACROHOME(void) {
    static const char *amh = 0;
    if (!amh) {
        amh = getenv_existing_directory("ARBMACROHOME"); // doc in arb_envar.hlp
        if (!amh) amh = GBS_eval_env("$(HOME)/.arb_prop/macros");
    }
    return amh;
}

GB_CSTR GB_getenvPATH() {
    static const char *path = 0;
    if (!path) {
        path = getenv_ignore_empty("PATH");
        if (!path) {
            path = GBS_eval_env("/bin:/usr/bin:/$(ARBHOME)/bin");
            GB_information("Your PATH variable is empty - using '%s' as search path.", path);
        }
    }
    return path;
}

GB_CSTR GB_getenvARB_GS(void) {
    static const char *gs = 0;
    if (!gs) {
        gs = getenv_executable("ARB_GS"); // doc in arb_envar.hlp
        if (!gs) gs = GB_find_executable("Postscript viewer", "gv", "ghostview", 0); 
    }
    return gs;
}

GB_CSTR GB_getenvARB_TEXTEDIT(void) {
    static const char *editor = 0;
    if (!editor) {
        editor = getenv_executable("ARB_TEXTEDIT"); // doc in arb_envar.hlp
        if (!editor) editor = "arb_textedit"; // a smart editor shell script
    }
    return editor;
}

GB_CSTR GB_getenvDOCPATH(void) {
    static const char *dp = 0;
    if (!dp) {
        char *res = getenv_existing_directory("ARB_DOC"); // doc in arb_envar.hlp
        if (res) dp = res;
        else     dp = GBS_eval_env("$(ARBHOME)/lib/help");
    }
    return dp;
}

GB_CSTR GB_getenvHTMLDOCPATH(void) {
    static const char *dp = 0;
    if (!dp) {
        char *res = getenv_existing_directory("ARB_HTMLDOC"); // doc in arb_envar.hlp
        if (res) dp = res;
        else     dp = GBS_eval_env("$(ARBHOME)/lib/help_html");
    }
    return dp;

}

GB_CSTR GB_getenv(const char *env){
    if (strncmp(env, "ARB", 3) == 0) {

        // doc in arb_envar.hlp

        if (strcmp(env, "ARBMACROHOME") == 0) return GB_getenvARBMACROHOME();
        if (strcmp(env, "ARBMACRO")     == 0) return GB_getenvARBMACRO();
        if (strcmp(env, "ARBHOME")      == 0) return GB_getenvARBHOME();
        if (strcmp(env, "ARB_GS")       == 0) return GB_getenvARB_GS();
        if (strcmp(env, "ARB_DOC")      == 0) return GB_getenvDOCPATH();
        if (strcmp(env, "ARB_TEXTEDIT") == 0) return GB_getenvARB_TEXTEDIT();
    }
    else {
        if (strcmp(env, "HOME") == 0) return GB_getenvHOME();
        if (strcmp(env, "USER") == 0) return GB_getenvUSER();
    }

    return getenv_ignore_empty(env);
}



int GB_host_is_local(const char *hostname){
    /* returns 1 if host is local */
    if (strcmp(hostname,"localhost")        == 0) return 1;
    if (strcmp(hostname,GBC_get_hostname()) == 0) return 1;
    if (strstr(hostname, "127.0.0.")        == hostname) return 1;
    return 0;
}

#define MIN(a,b) (((a)<(b))?(a):(b))

/* Returns the physical memory size in k available for one process */
GB_ULONG GB_get_physical_memory(void){
#if defined(SUN5) || defined(LINUX)
    long pagesize = sysconf(_SC_PAGESIZE);
    long pages    = sysconf(_SC_PHYS_PAGES);
    /*    long test = sysconf(_SC_AVPHYS_PAGES); */

    long memsize        = (pagesize/1024) * pages;
    long nettomemsize   = memsize - 10240; /* reduce by 10Mb (for kernel etc.) */
    long maxmemsize4arb = (nettomemsize*95)/100; /* arb uses max. 95 % of available memory (was 70% in the past) */
    long addressable    = (long)1<<((long)sizeof(void*)*(long)8-(long)10); /* e.g. 2^32 with 4byte-pointers; -10 because we calculate in kBytes */
    long usedmemsize    = MIN(maxmemsize4arb, addressable); /* limit to max addressable memory */

#if defined(DEBUG)
    printf("- memsize(real)        = %20li k\n", memsize);
    printf("- memsize(netto)       = %20li k\n", nettomemsize);
    printf("- memsize(95%%)         = %20li k\n", maxmemsize4arb);
    printf("- memsize(addressable) = %20li k\n", addressable);
    printf("- memsize(used by ARB) = %20li k\n", usedmemsize);
#endif /* DEBUG */
    
    arb_assert(usedmemsize != 0);
    return usedmemsize;
#else
    return 128*1024;            /* 128 Mb default memory */
#endif
}
