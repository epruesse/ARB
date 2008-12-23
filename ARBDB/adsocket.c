#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>
#include <errno.h>
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

#if defined(DARWIN)
# include <sys/sysctl.h>
#endif /* DARWIN */


/************************************ signal handling ****************************************/
/************************************ valid memory tester ************************************/
int gbcm_sig_violation_flag;
int gbcm_pipe_violation_flag;

#if defined(DIGITAL)
# if defined(__cplusplus)
extern "C" {    extern void usleep(unsigned int );  }
# else
extern void usleep(unsigned int);
# endif
#endif // DIGITAL

void GB_usleep(long usec){
# if defined(SUN5)
    struct timespec timeout,time2;
    timeout.tv_sec  = 0;
    timeout.tv_nsec = usec*1000;
    nanosleep(&timeout,&time2);
# else
    usleep(usec);
# endif
}

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
        strcpy(so_ad.sun_path, mach_name[0]);

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
        GB_internal_error("receive failed: %zu bytes expected, %li got, keyword %lX",
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

GB_BOOL GB_is_regularfile(const char *path){
    struct stat stt;
    return stat(path, &stt) == 0 && S_ISREG(stt.st_mode);
}

GB_BOOL GB_is_executablefile(const char *path) {
    struct stat stt;
    GB_BOOL     executable = GB_FALSE;
    
    if (stat(path, &stt) == 0) {
        uid_t my_userid = geteuid(); // effective user id
        if (stt.st_uid == my_userid) { // I am the owner of the file
            executable = !!(stt.st_mode&S_IXUSR); // owner execution permission
        }
        else {
            gid_t my_groupid = getegid(); // effective group id
            if (stt.st_gid == my_groupid) { // I am member of the file's group
                executable = !!(stt.st_mode&S_IXGRP); // group execution permission
            }
            else {
                executable = !!(stt.st_mode&S_IXOTH); // others execution permission
            }
        }
    }

    return executable;
}

GB_BOOL GB_is_readablefile(const char *filename) {
    FILE *in = fopen(filename, "r");

    if (in) {
        fclose(in);
        return GB_TRUE;
    }
    return GB_FALSE;
}

GB_BOOL GB_is_directory(const char *path) {
    struct stat stt;
    return stat(path, &stt) == 0 && S_ISDIR(stt.st_mode);
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
    size_t size = GB_size_of_FILE(in);
    char *buffer;
    if (size<=0) {
        GB_export_error("GB_map_file: sorry file not found");
        return NULL;
    }
    if (writeable){
        buffer = (char*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fi, 0);
    }else{
        buffer = (char*)mmap(NULL, size, PROT_READ, MAP_SHARED, fi, 0);
    }
    if (buffer == MAP_FAILED){
        GB_export_error("GB_map_file: Error Out of Memory: mmap failes (errno: %i)", errno);
        return NULL;
    }
    return buffer;
}

char *GB_map_file(const char *path,int writeable){
    FILE *in;
    char *buffer;
    in = fopen(path,"r");
    if (!in) {
        GB_export_error("GB_map_file: sorry file '%s' not readable",path);
        return NULL;
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

GB_ERROR GB_textprint(const char *path){
    /* goes to header: __ATTR__USERESULT */
    char       *fpath   = GBS_eval_env(path);
    const char *command = GBS_global_string("arb_textprint '%s' &", fpath);
    GB_ERROR    error   = GB_system(command);
    free(fpath);
    return GB_failedTo_error("print textfile", fpath, error);
}

GB_CSTR GB_getcwd(void) {
    // get the current working directory
    // (directory from which application has been started)

    static char *lastcwd  = 0;
    if (!lastcwd) lastcwd = (char *)getcwd(0,GB_PATH_MAX);
    return lastcwd;
}

#if defined(DEVEL_RALF)
#warning search for '\b(system)\b\s*\(' and use GB_system instead
#endif /* DEVEL_RALF */
GB_ERROR GB_system(const char *system_command) {
    /* goes to header: __ATTR__USERESULT */
    fprintf(stderr, "[Action: '%s']\n", system_command);
    int      res   = system(system_command);
    GB_ERROR error = NULL;
    if (res) {
        error = GBS_global_string("System call '%s' failed (result=%i)", system_command, res);
    }
    return error;
}

GB_ERROR GB_xterm(void) {
    /* goes to header: __ATTR__USERESULT */
    const char *xt = GB_getenv("ARB_XTERM"); // doc in arb_envar.hlp
    if (!xt) xt = "xterm -sl 1000 -sb -geometry 120x40";

    const char *command = GBS_global_string("%s &", xt);
    return GB_system(command);
}

GB_ERROR GB_xcmd(const char *cmd, GB_BOOL background, GB_BOOL wait_only_if_error) {
    /* goes to header: __ATTR__USERESULT */
    
    // runs a command in an xterm
    // if 'background' is true -> run asyncronous
    // if 'wait_only_if_error' is true -> asyncronous does wait for keypress only if cmd fails

    void       *strstruct = GBS_stropen(1024);
    const char *xt        = GB_getenv("ARB_XCMD"); // doc in arb_envar.hlp
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

    GB_ERROR error = GB_system(GBS_mempntr(strstruct));
    GBS_strforget(strstruct);

    return error;
}

/* -------------------------------------------------------------------------------- */
/* Functions to find an executable */

char *GB_executable(GB_CSTR exe_name) {
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
    /* goes to header: __ATTR__SENTINEL  */
    /* search the path for an executable with any of the given names (...)
     * if any is found, it's full path is returned
     * if none is found, a warning call is returned (which can be executed without harm)
    */

    GB_CSTR  name;
    char    *found = 0;
    va_list  args;

    va_start(args, description_of_executable);
    while (!found && (name = va_arg(args, GB_CSTR)) != 0) found = GB_executable(name);
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
        result = GB_executable(exe_name);
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
        if (!am) am = strdup(GB_path_in_ARBLIB("macros", NULL));
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
            path = GBS_eval_env("/bin:/usr/bin:$(ARBHOME)/bin");
            GB_information("Your PATH variable is empty - using '%s' as search path.", path);
        }
        else {
            char *arbbin = GBS_eval_env("$(ARBHOME)/bin");
            if (strstr(path, arbbin) == 0) {
                GB_warning("Your PATH variable does not contain '%s'. Things may not work as expected.", arbbin);
            }
            free(arbbin);
        }
    }
    return path;
}

GB_CSTR GB_getenvARB_GS(void) {
    static const char *gs = 0;
    if (!gs) {
        gs = getenv_executable("ARB_GS"); // doc in arb_envar.hlp
        if (!gs) gs = GB_find_executable("Postscript viewer", "gv", "ghostview", NULL);
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
        else     dp = strdup(GB_path_in_ARBLIB("help", NULL));
    }
    return dp;
}

GB_CSTR GB_getenvHTMLDOCPATH(void) {
    static const char *dp = 0;
    if (!dp) {
        char *res = getenv_existing_directory("ARB_HTMLDOC"); // doc in arb_envar.hlp
        if (res) dp = res;
        else     dp = strdup(GB_path_in_ARBLIB("help_html", NULL));
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

GB_ULONG GB_get_physical_memory(void) {
    /* Returns the physical available memory size in k available for one process */
    ulong memsize; /* real existing memory in k */
    
#if defined(SUN5) || defined(LINUX) 
    {
        long pagesize = sysconf(_SC_PAGESIZE); 
        long pages    = sysconf(_SC_PHYS_PAGES);

        memsize = (pagesize/1024) * pages;
    }
#elif defined(DARWIN)
#warning memsize detection needs to be tested for Darwin
    {
        int      mib[2];
        uint64_t bytes;
        size_t   len;

        mib[0] = CTL_HW;
        mib[1] = HW_MEMSIZE; /*uint64_t: physical ram size */
        len = sizeof(bytes);
        sysctl(mib, 2, &bytes, &len, NULL, 0);

        memsize = bytes/1024;
    }
#else
    memsize = 512*1024; // assume 512 Mb
    printf("\n"
           "Warning: ARB is not prepared to detect the memory size on your system!\n"
           "         (it assumes you have %ul Mb,  but does not use more)\n\n", memsize/1024);
#endif

    ulong nettomemsize = memsize - 10240;         /* reduce by 10Mb (for kernel etc.) */

    // detect max allocateable memory by ... allocating
    ulong max_malloc_try = nettomemsize*1024;
    ulong max_malloc     = 0;
    {
        ulong step_size  = 4096;
        void *head = 0;

        do {
            void **tmp;
            while((tmp=malloc(step_size))) {
                *tmp        = head;
                head        = tmp;
                max_malloc += step_size;
                if (max_malloc >= max_malloc_try) break;
                step_size *= 2;
            }
        } while((step_size=step_size/2) > sizeof(void*));

        while(head) {
            void *tmp;
            tmp=*(void**)head;
            free(head);
            head=tmp;
        }

        max_malloc /= 1024;
    }

    ulong usedmemsize = (MIN(nettomemsize,max_malloc)*95)/100;  /* arb uses max. 95 % of available memory (was 70% in the past) */

#if defined(DEBUG)
    printf("- memsize(real)        = %20lu k\n", memsize);
    printf("- memsize(netto)       = %20lu k\n", nettomemsize);
    printf("- memsize(max_malloc)  = %20lu k\n", max_malloc);
#endif /* DEBUG */
    printf("- memsize(used by ARB) = %20lu k\n", usedmemsize);

    arb_assert(usedmemsize != 0);

    return usedmemsize;
}

/* --------------------------------------------- */
/* path completion (parts former located in AWT) */

static int  path_toggle = 0;
static char path_buf[2][PATH_MAX];

GB_CSTR GB_get_full_path(const char *anypath) {
    // expands '~' '..' etc in 'anypath'

    GB_CSTR result = NULL;
    if (!anypath) {
        GB_export_error("NULL path (internal error)");
    }
    else if (strlen(anypath) >= PATH_MAX) {
        GB_export_error("Path too long (> %i chars)", PATH_MAX-1);
    }
    else {
        path_toggle = 1-path_toggle; // use 2 buffers in turn
        result = path_buf[path_toggle];

        if (strncmp(anypath, "~/", 2) == 0) {
            GB_CSTR home    = GB_getenvHOME();
            GB_CSTR newpath = GBS_global_string("%s%s", home, anypath+1);
            realpath(newpath, path_buf[path_toggle]);
            GBS_reuse_buffer(newpath);
        }
        else {
            realpath(anypath, path_buf[path_toggle]);
        }
    }
    return result;
}

GB_CSTR GB_concat_path(GB_CSTR anypath_left, GB_CSTR anypath_right) {
    // concats left and right part of a path.
    // '/' is inserted inbetween
    //
    // if one of the arguments is NULL = > returns the other argument

    GB_CSTR result = NULL;

    if (anypath_left) {
        if (anypath_right) {
            path_toggle = 1-path_toggle;
            result = GBS_global_string_to_buffer(path_buf[path_toggle], sizeof(path_buf[path_toggle]), "%s/%s", anypath_left, anypath_right);
        }
        else {
            result = anypath_left;
        }
    }
    else {
        result = anypath_right;
    }

    return result;
}

GB_CSTR GB_concat_full_path(const char *anypath_left, const char *anypath_right) {
    // like GB_concat_path(), but returns the full path
    GB_CSTR result = GB_concat_path(anypath_left, anypath_right);

    gb_assert(result != anypath_left);
    gb_assert(result != anypath_right);

    if (result) result = GB_get_full_path(result);
    return result;
}

GB_CSTR GB_path_in_ARBHOME(const char *relative_path_left, const char *anypath_right) {
    if (anypath_right) {
        return GB_concat_full_path(GB_concat_path(GB_getenvARBHOME(), relative_path_left), anypath_right);
    }
    else {
        return GB_concat_full_path(GB_getenvARBHOME(), relative_path_left);
    }
}

GB_CSTR GB_path_in_ARBLIB(const char *relative_path_left, const char *anypath_right) {
    if (anypath_right) {
        return GB_path_in_ARBHOME(GB_concat_path("lib", relative_path_left), anypath_right);
    }
    else {
        return GB_path_in_ARBHOME("lib", relative_path_left);
    }
}

void GB_split_full_path(const char *fullpath, char **res_dir, char **res_fullname, char **res_name_only, char **res_suffix) {
    // Takes a file (or directory) name and splits it into "path/name.suffix".
    // If result pointers (res_*) are non-NULL, they are assigned heap-copies of the splitted parts.
    // If parts are not valid (e.g. cause 'fullpath' doesn't have a .suffix) the corresponding result pointer
    // is set to NULL.
    // 
    // The '/' and '.' characters are not included in the results (except the '.' in 'res_fullname')

    if (fullpath && fullpath[0]) {
        const char *lslash     = strrchr(fullpath, '/');
        const char *name_start = lslash ? lslash+1 : fullpath;
        const char *ldot       = strrchr(lslash ? lslash : fullpath, '.');
        const char *terminal   = strchr(name_start, 0);

        gb_assert(terminal);
        gb_assert(name_start);

        gb_assert(terminal > fullpath); // ensure (terminal-1) is a valid character position in path

        if (res_dir)       *res_dir       = lslash ? GB_strpartdup(fullpath, lslash-1) : NULL;
        if (res_fullname)  *res_fullname  = GB_strpartdup(name_start, terminal-1);
        if (res_name_only) *res_name_only = GB_strpartdup(name_start, ldot ? ldot-1 : terminal-1);
        if (res_suffix)    *res_suffix    = ldot ? GB_strpartdup(ldot+1, terminal-1) : strdup(name_start);
    }
    else {
        if (res_dir)       *res_dir       = NULL;
        if (res_fullname)  *res_fullname  = NULL;
        if (res_name_only) *res_name_only = NULL;
        if (res_suffix)    *res_suffix    = NULL;
    }
}
