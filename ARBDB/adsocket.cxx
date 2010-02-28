// =============================================================== //
//                                                                 //
//   File      : adsocket.cxx                                      //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <unistd.h>

#include <cerrno>
#include <climits>
#include <cstdarg>

#include <netdb.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>

#include "gb_comm.h"
#include "gb_data.h"
#include "gb_localdata.h"
#include "gb_main.h"

#include <SigHandler.h>

#if defined(DARWIN)
# include <sys/sysctl.h>
#endif // DARWIN

void GB_usleep(long usec) {
    usleep(usec);
}


static int gbcm_pipe_violation_flag = 0;
void gbcms_sigpipe(int) {
    gbcm_pipe_violation_flag = 1;
}

// ------------------------------------------------
//      private read and write socket functions

void gbcm_read_flush(int socket) {
    gb_local->write_ptr  = gb_local->write_buffer;
    gb_local->write_free = gb_local->write_bufsize;
    socket               = socket; // @@@ wtf?
}

static long gbcm_read_buffered(int socket, char *ptr, long size) {
    /* write_ptr ptr to not read data
       write_free   = write_bufsize-size of non read data;
    */
    long holding;
    holding = gb_local->write_bufsize - gb_local->write_free;
    if (holding <= 0) {
        holding = read(socket, gb_local->write_buffer, (size_t)gb_local->write_bufsize);

        if (holding < 0)
        {
            fprintf(stderr, "Cannot read data from client: len=%li (%s, errno %i)\n",
                    holding, strerror(errno), errno);
            return 0;
        }
        gbcm_read_flush(socket);
        gb_local->write_free-=holding;
    }
    if (size>holding) size = holding;
    memcpy(ptr, gb_local->write_ptr, (int)size);
    gb_local->write_ptr += size;
    gb_local->write_free += size;
    return size;
}

long gbcm_read(int socket, char *ptr, long size)
{
    long    leftsize, readsize;
    leftsize = size;
    readsize = 0;

    while (leftsize) {
        readsize = gbcm_read_buffered(socket, ptr, leftsize);
        if (readsize<=0) return 0;
        ptr += readsize;
        leftsize -= readsize;
    }

    return size;
}

int gbcm_write_flush(int socket) {
    char *ptr;
    long    leftsize, writesize;
    ptr = gb_local->write_buffer;
    leftsize = gb_local->write_ptr - ptr;
    gb_local->write_free = gb_local->write_bufsize;
    if (!leftsize) return GBCM_SERVER_OK;

    gb_local->write_ptr = ptr;
    gbcm_pipe_violation_flag = 0;

    writesize = write(socket, ptr, (size_t)leftsize);

    if (gbcm_pipe_violation_flag || writesize<0) {
        if (gb_local->iamclient) {
            fprintf(stderr, "DB_Server is killed, Now I kill myself\n");
            exit(-1);
        }
        fprintf(stderr, "writesize: %li ppid %i\n", writesize, getppid());
        return GBCM_SERVER_FAULT;
    }
    ptr += writesize;
    leftsize = leftsize - writesize;

    while (leftsize) {

        GB_usleep(10000);

        writesize = write(socket, ptr, (size_t)leftsize);
        if (gbcm_pipe_violation_flag || writesize<0) {
            if ((int)getppid() <= 1) {
                fprintf(stderr, "DB_Server is killed, Now I kill myself\n");
                exit(-1);
            }
            fprintf(stderr, "write error\n");
            return GBCM_SERVER_FAULT;
        }
        ptr += writesize;
        leftsize -= writesize;
    }
    return GBCM_SERVER_OK;
}

int gbcm_write(int socket, const char *ptr, long size) {

    while  (size >= gb_local->write_free) {
        memcpy(gb_local->write_ptr, ptr, (int)gb_local->write_free);
        gb_local->write_ptr += gb_local->write_free;
        size -= gb_local->write_free;
        ptr += gb_local->write_free;

        gb_local->write_free = 0;
        if (gbcm_write_flush(socket)) return GBCM_SERVER_FAULT;
    }
    memcpy(gb_local->write_ptr, ptr, (int)size);
    gb_local->write_ptr += size;
    gb_local->write_free -= size;
    return GBCM_SERVER_OK;
}

static GB_ERROR gbcm_get_m_id(const char *path, char **m_name, long *id) {
    // find the mach name and id
    GB_ERROR error = 0;

    if (!path) error = "missing hostname:socketid";
    else {
        if (strcmp(path, ":") == 0) {
            path             = GBS_read_arb_tcp("ARB_DB_SERVER");
            if (!path) error = GB_await_error();
        }

        if (!error) {
            const char *p = strchr(path, ':');

            if (!p) error = GBS_global_string("missing ':' in '%s'", path);
            else {
                if (path[0] == '*' || path[0] == ':') { // UNIX MODE
                    *m_name = strdup(p+1);
                    *id     = -1;
                }
                else {
                    char *mn = GB_strpartdup(path, p-1);

                    int i = atoi(p + 1);
                    if ((i < 1) || (i > 4096)) {
                        error = GBS_global_string("socketnumber %i not in [1..4096]", i);
                    }

                    if (!error) {
                        *m_name = mn;
                        *id     = i;
                    }
                    else {
                        free(mn);
                    }
                }
            }
        }
    }

    if (error) error = GBS_global_string("OPEN_ARB_DB_CLIENT ERROR: %s", error);
    return error;
}

GB_ERROR gbcm_open_socket(const char *path, long delay2, long do_connect, int *psocket, char **unix_name) {
    long      socket_id[1];
    char    *mach_name[1];
    struct in_addr  addr;   // union -> u_long
    struct hostent *he;
    GB_ERROR err;

    mach_name[0] = 0;
    err = gbcm_get_m_id(path, mach_name, socket_id);
    if (err) {
        if (mach_name[0]) free((char *)mach_name[0]);
        return err;
    }
    if (socket_id[0] >= 0) {    // TCP
        struct sockaddr_in so_ad;
        memset((char *)&so_ad, 0, sizeof(struct sockaddr_in));
        *psocket = socket(PF_INET, SOCK_STREAM, 0);
        if (*psocket <= 0) {
            return "CANNOT CREATE SOCKET";
        }
        he = gethostbyname(mach_name[0]);

        if (!he) {
            return "Unknown host";
        }

        // simply take first address
        addr.s_addr = *(long *) (he->h_addr);
        so_ad.sin_addr = addr;
        so_ad.sin_family = AF_INET;
        so_ad.sin_port = htons((unsigned short)(socket_id[0])); // @@@ = pb_socket
        if (do_connect) {
            if (connect(*psocket, (struct sockaddr *)(&so_ad), sizeof(so_ad))) {
                GB_warningf("Cannot connect to %s:%li   errno %i", mach_name[0], socket_id[0], errno);
                return "";
            }
        }
        else {
            int one = 1;
            setsockopt(*psocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one));
            if (bind(*psocket, (struct sockaddr *)(&so_ad), sizeof(so_ad))) {
                return "Could not open socket on Server";
            }
        }
        if (mach_name[0]) free((char *)mach_name[0]);
        if (delay2 == TCP_NODELAY) {
            GB_UINT4      optval;
            optval = 1;
            setsockopt(*psocket, IPPROTO_TCP, TCP_NODELAY, (char *)&optval, sizeof (GB_UINT4));
        }
        *unix_name = 0;
        return 0;
    }
    else {        // UNIX
        struct sockaddr_un so_ad; memset((char *)&so_ad, 0, sizeof(so_ad));
        *psocket = socket(PF_UNIX, SOCK_STREAM, 0);
        if (*psocket <= 0) {
            return "CANNOT CREATE SOCKET";
        }
        so_ad.sun_family = AF_UNIX;
        strcpy(so_ad.sun_path, mach_name[0]);

        if (do_connect) {
            if (connect(*psocket, (struct sockaddr*)(&so_ad), strlen(so_ad.sun_path)+2)) {
                if (mach_name[0]) free((char *)mach_name[0]);
                return "";
            }
        }
        else {
            if (unlink(mach_name[0]) == 0) printf("old socket found\n");
            if (bind(*psocket, (struct sockaddr*)(&so_ad), strlen(mach_name[0])+2)) {
                if (mach_name[0]) free((char *)mach_name[0]);
                return "Could not open socket on Server";
            }
            if (chmod(mach_name[0], 0777)) return GB_export_errorf("Cannot change mode of socket '%s'", mach_name[0]);
        }
        *unix_name = mach_name[0];
        return 0;
    }
}

#if defined(DEVEL_RALF)
#warning gbcms_close is unused
#endif // DEVEL_RALF
long gbcms_close(gbcmc_comm *link)
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

static void gbcmc_suppress_sigpipe(int) {
}

struct gbcmc_comm *gbcmc_open(const char *path)
{
    struct gbcmc_comm *link;
    GB_ERROR err;

    link = (struct gbcmc_comm *)GB_calloc(sizeof(struct gbcmc_comm), 1);
    err = gbcm_open_socket(path, TCP_NODELAY, 1, &link->socket, &link->unix_name);
    if (err) {
        if (link->unix_name) free(link->unix_name); // @@@
        free((char *)link);
        if (*err) {
            GB_internal_errorf("ARB_DB_CLIENT_OPEN\n(Reason: %s)", err);
        }
        return 0;
    }
    ASSERT_RESULT(SigHandler, SIG_DFL, signal(SIGPIPE, gbcmc_suppress_sigpipe));
    gb_local->iamclient = 1;
    return link;
}

long gbcm_write_two(int socket, long a, long c)
{
    long    ia[3];
    ia[0] = a;
    ia[1] = 3;
    ia[2] = c;
    if (!socket) return 1;
    return  gbcm_write(socket, (const char *)ia, sizeof(long)*3);
}


long gbcm_read_two(int socket, long a, long *b, long *c) {
    /*! read two values: length and any user long
     *
     *  if data is send by gbcm_write_two() then @param b should be zero
     *  and is not used!
     */

    long    ia[3];
    long    size;
    size = gbcm_read(socket, (char *)&(ia[0]), sizeof(long)*3);
    if (size != sizeof(long) * 3) {
        GB_internal_errorf("receive failed: %zu bytes expected, %li got, keyword %lX",
                           sizeof(long) * 3, size, a);
        return GBCM_SERVER_FAULT;
    }
    if (ia[0] != a) {
        GB_internal_errorf("received keyword failed %lx != %lx\n", ia[0], a);
        return GBCM_SERVER_FAULT;
    }
    if (b) {
        *b = ia[1];
    }
    else {
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
        if (len) gbcm_write(socket, key, len);
    }
    else {
        gbcm_write_long(socket, -1);
    }
    return GBCM_SERVER_OK;
}

char *gbcm_read_string(int socket)
{
    char *key;
    long  len = gbcm_read_long(socket);

    if (len) {
        if (len>0) {
            key = (char *)GB_calloc(sizeof(char), (size_t)len+1);
            gbcm_read(socket, key, len);
        }
        else {
            key = 0;
        }
    }
    else {
        key = strdup("");
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
    if (path) {
        char *path2 = GBS_eval_env(path);
        if (stat(path2, &gb_global_stt)) {
            free(path2);
            return 0;
        }
        free(path2);
    }
    return gb_global_stt.st_mtime;
}

long GB_size_of_file(const char *path) {
    if (!path || stat(path, &gb_global_stt)) return -1;
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

bool GB_is_regularfile(const char *path) {
    // Warning : returns true for symbolic links to files (use GB_is_link() to test)
    struct stat stt;
    return stat(path, &stt) == 0 && S_ISREG(stt.st_mode);
}

bool GB_is_link(const char *path) {
    struct stat stt;
    return lstat(path, &stt) == 0 && S_ISLNK(stt.st_mode);
}

bool GB_is_executablefile(const char *path) {
    struct stat stt;
    bool        executable = false;

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

bool GB_is_privatefile(const char *path, bool read_private) {
    // return true, if nobody but user has write permission
    // if 'read_private' is true, only return true if nobody but user has read permission
    //
    // Note: Always returns true for missing files!
    //
    // GB_is_privatefile is mainly used to assert that files generated in /tmp have secure permissions

    struct stat stt;
    bool        isprivate = true;

    if (stat(path, &stt) == 0) {
        if (read_private) {
            isprivate = (stt.st_mode & (S_IWGRP|S_IWOTH|S_IRGRP|S_IROTH)) == 0;
        }
        else {
            isprivate = (stt.st_mode & (S_IWGRP|S_IWOTH)) == 0;
        }
    }
    return isprivate;
}

bool GB_is_readablefile(const char *filename) {
    FILE *in = fopen(filename, "r");

    if (in) {
        fclose(in);
        return true;
    }
    return false;
}

bool GB_is_directory(const char *path) {
    // Warning : returns true for symbolic links to directories (use GB_is_link())
    struct stat stt;
    return stat(path, &stt) == 0 && S_ISDIR(stt.st_mode);
}

long GB_getuid_of_file(const char *path) {
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
        GB_export_IO_error("removing", path);
        return -1;
    }
    return 0;
}

void GB_unlink_or_warn(const char *path, GB_ERROR *error) {
    /* Unlinks 'path'
     *
     * In case of a real unlink failure:
     * - if 'error' is given -> set error if not already set
     * - otherwise only warn
     */

    if (GB_unlink(path)<0) {
        GB_ERROR unlink_error = GB_await_error();
        if (error && *error == NULL) *error = unlink_error;
        else GB_warning(unlink_error);
    }
}

char *GB_follow_unix_link(const char *path) {   // returns the real path of a file
    char buffer[1000];
    char *path2;
    char *pos;
    char *res;
    int len = readlink(path, buffer, 999);
    if (len<0) return 0;
    buffer[len] = 0;
    if (path[0] == '/') return strdup(buffer);

    path2 = strdup(path);
    pos = strrchr(path2, '/');
    if (!pos) {
        free(path2);
        return strdup(buffer);
    }
    *pos = 0;
    res  = GBS_global_string_copy("%s/%s", path2, buffer);
    free(path2);
    return res;
}

GB_ERROR GB_symlink(const char *name1, const char *name2) { // name1 is the existing file !!!
    if (symlink(name1, name2)<0) {
        return GB_export_errorf("Cannot create symlink '%s' to file '%s'", name2, name1);
    }
    return 0;
}

GB_ERROR GB_set_mode_of_file(const char *path, long mode)
{
    if (chmod(path, (int)mode)) return GB_export_errorf("Cannot change mode of '%s'", path);
    return 0;
}

GB_ERROR GB_rename_file(const char *oldpath, const char *newpath) {
    long old_mod = GB_mode_of_file(newpath);
    if (old_mod == -1) old_mod = GB_mode_of_file(oldpath);
    if (rename(oldpath, newpath)) {
        return GB_export_IO_error("renaming", GBS_global_string("%s into %s", oldpath, newpath));
    }
    if (GB_set_mode_of_file(newpath, old_mod)) {
        return GB_export_IO_error("setting mode", newpath);
    }
    sync();
    return 0;
}

char *GB_read_fp(FILE *in) {
    /*! like GB_read_file(), but works on already open file
     * (useful together with GB_fopen_tempfile())
     *
     * Note: File should be opened in text-mode (e.g. "rt")
     */

    struct GBS_strstruct *buf = GBS_stropen(4096);
    int            c;

    while (EOF != (c = getc(in))) {
        GBS_chrcat(buf, c);
    }
    return GBS_strclose(buf);
}

char *GB_read_file(const char *path) {
    /*! read content of file 'path' into string (heap-copy)
     *
     * if path is '-', read from STDIN
     *
     * @return NULL in case of error (which is exported then)
     */
    char *result = 0;

    if (strcmp(path, "-") == 0) {
        result = GB_read_fp(stdin);
    }
    else {
        char *epath = GBS_eval_env(path);

        if (epath) {
            FILE *in = fopen(epath, "rt");

            if (!in) GB_export_IO_error("reading", epath);
            else {
                long data_size = GB_size_of_file(epath);

                if (data_size >= 0) {
                    result = (char*)malloc(data_size+1);

                    data_size         = fread(result, 1, data_size, in);
                    result[data_size] = 0;
                }
                fclose(in);
            }
        }
        free(epath);
    }
    return result;
}

char *GB_map_FILE(FILE *in, int writeable) {
    int fi = fileno(in);
    size_t size = GB_size_of_FILE(in);
    char *buffer;
    if (size<=0) {
        GB_export_error("GB_map_file: sorry file not found");
        return NULL;
    }
    if (writeable) {
        buffer = (char*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fi, 0);
    }
    else {
        buffer = (char*)mmap(NULL, size, PROT_READ, MAP_SHARED, fi, 0);
    }
    if (buffer == MAP_FAILED) {
        GB_export_errorf("GB_map_file: Error: Out of Memory: mmap failed (errno: %i)", errno);
        return NULL;
    }
    return buffer;
}

char *GB_map_file(const char *path, int writeable) {
    FILE *in;
    char *buffer;
    in = fopen(path, "r");
    if (!in) {
        GB_export_errorf("GB_map_file: sorry file '%s' not readable", path);
        return NULL;
    }
    buffer = GB_map_FILE(in, writeable);
    fclose(in);
    return buffer;
}

long GB_size_of_FILE(FILE *in) {
    int fi = fileno(in);
    struct stat st;
    if (fstat(fi, &st)) {
        GB_export_error("GB_size_of_FILE: sorry file is not readable");
        return -1;
    }
    return st.st_size;
}


GB_ULONG GB_time_of_day() {
    struct timeval tp;
    if (gettimeofday(&tp, 0)) return 0;
    return tp.tv_sec;
}

long GB_last_saved_clock(GBDATA *gb_main) {
    return GB_MAIN(gb_main)->last_saved_transaction;
}

GB_ULONG GB_last_saved_time(GBDATA *gb_main) {
    return GB_MAIN(gb_main)->last_saved_time;
}

GB_ERROR GB_textprint(const char *path) {
    // goes to header: __ATTR__USERESULT
    char       *fpath   = GBS_eval_env(path);
    const char *command = GBS_global_string("arb_textprint '%s' &", fpath);
    GB_ERROR    error   = GB_system(command);
    free(fpath);
    return GB_failedTo_error("print textfile", fpath, error);
}

#if defined(DEVEL_RALF)
#warning search for '\b(system)\b\s*\(' and use GB_system instead
#endif // DEVEL_RALF
GB_ERROR GB_system(const char *system_command) {
    // goes to header: __ATTR__USERESULT
    fprintf(stderr, "[Action: '%s']\n", system_command);
    int      res   = system(system_command);
    GB_ERROR error = NULL;
    if (res) {
        error = GBS_global_string("System call failed (result=%i)\n"
                                  "System call was '%s'\n"
                                  "(Note: console window may contain additional information)", res, system_command);
    }
    return error;
}

GB_ERROR GB_xterm() {
    // goes to header: __ATTR__USERESULT
    const char *xt = GB_getenv("ARB_XTERM"); // doc in ../HELP_SOURCE/oldhelp/arb_envar.hlp@ARB_XTERM
    if (!xt) xt = "xterm -sl 1000 -sb -geometry 120x40";

    const char *command = GBS_global_string("%s &", xt);
    return GB_system(command);
}

GB_ERROR GB_xcmd(const char *cmd, bool background, bool wait_only_if_error) {
    // goes to header: __ATTR__USERESULT

    // runs a command in an xterm
    // if 'background' is true -> run asynchronous
    // if 'wait_only_if_error' is true -> asynchronous does wait for keypress only if cmd fails

    GBS_strstruct *strstruct = GBS_stropen(1024);
    const char    *xt        = GB_getenv("ARB_XCMD"); // doc in ../HELP_SOURCE/oldhelp/arb_envar.hlp@ARB_XCMD
    if (!xt) xt              = "xterm -sl 1000 -sb -geometry 140x30 -e";
    GBS_strcat(strstruct, "(");
    GBS_strcat(strstruct, xt);
    GBS_strcat(strstruct, " sh -c 'LD_LIBRARY_PATH=\"");
    GBS_strcat(strstruct, GB_getenv("LD_LIBRARY_PATH"));
    GBS_strcat(strstruct, "\";export LD_LIBRARY_PATH; (");
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

// --------------------------------------------------------------------------------
// Functions to find an executable

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
    // goes to header: __ATTR__SENTINEL
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

    if (!found) { // none of the executables has been found
        char *looked_for;
        char *msg;
        {
            GBS_strstruct *buf   = GBS_stropen(100);
            int            first = 1;

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
        GB_informationf("Using %s '%s' ('%s')", description_of_executable, name, found);
    }
    return found;
}

// --------------------------------------------------------------------------------
// Functions to access the environment variables used by ARB:

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
            GB_warningf("Environment variable '%s' contains '%s' (which is not an executable)", envvar, exe_name);
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
            GB_warningf("Environment variable '%s' should contain the path of an existing directory.\n"
                        "(current content '%s' has been ignored.)", envvar, dir_name);
        }
    }
    return result;
}

GB_CSTR GB_getenvUSER() {
    static const char *user = 0;
    if (!user) {
        user = getenv_ignore_empty("USER");
        if (!user) user = getenv_ignore_empty("LOGNAME");
        if (!user) {
            user = getenv_ignore_empty("HOME");
            if (user && strrchr(user, '/')) user = strrchr(user, '/')+1;
        }
        if (!user) {
            fprintf(stderr, "WARNING: Cannot identify user: environment variables USER, LOGNAME and HOME not set\n");
            user = "UnknownUser";
        }
    }
    return user;
}


GB_CSTR GB_getenvHOME() {
    static const char *home = 0;
    if (!home) {
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

GB_CSTR GB_getenvARBHOME() {
    static char *arbhome = 0;
    if (!arbhome) {
        arbhome = getenv_existing_directory("ARBHOME"); // doc in ../HELP_SOURCE/oldhelp/arb_envar.hlp@ARBHOME
        if (!arbhome) {
            fprintf(stderr, "ERROR: Environment Variable ARBHOME not found !!!\n"
                    "   Please set 'ARBHOME' to the installation path of ARB\n");
            exit(-1);
        }
    }
    return arbhome;
}

GB_CSTR GB_getenvARBMACRO() {
    static const char *am = 0;
    if (!am) {
        am          = getenv_existing_directory("ARBMACRO"); // doc in ../HELP_SOURCE/oldhelp/arb_envar.hlp@ARBMACRO
        if (!am) am = strdup(GB_path_in_ARBLIB("macros", NULL));
    }
    return am;
}

static GB_CSTR getenv_autodirectory(const char *envvar, const char *defaultDirectory) {
    static const char *dir = 0;
    if (!dir) {
        dir = getenv_existing_directory(envvar);
        if (!dir) {
            dir = GBS_eval_env(defaultDirectory);
            if (!GB_is_directory(dir)) {
                GB_ERROR error = GB_create_directory(dir);
                if (error) GB_warningf("Failed to create directory '%s' (Reason: %s)", dir, error);
            }
        }
    }
    return dir;
}

GB_CSTR GB_getenvARBMACROHOME() {
    static const char *amh = 0;
    if (!amh) amh = getenv_autodirectory("ARBMACROHOME", "$(HOME)/.arb_prop/macros");  // doc in ../HELP_SOURCE/oldhelp/arb_envar.hlp@ARBMACROHOME
    return amh;
}

GB_CSTR GB_getenvARBCONFIG() {
    static const char *ac = 0;
    if (!ac) ac = getenv_autodirectory("ARBCONFIG", "$(HOME)/.arb_prop/cfgSave"); // doc in ../HELP_SOURCE/oldhelp/arb_envar.hlp@ARBCONFIG
    return ac;
}

GB_CSTR GB_getenvPATH() {
    static const char *path = 0;
    if (!path) {
        path = getenv_ignore_empty("PATH");
        if (!path) {
            path = GBS_eval_env("/bin:/usr/bin:$(ARBHOME)/bin");
            GB_informationf("Your PATH variable is empty - using '%s' as search path.", path);
        }
        else {
            char *arbbin = GBS_eval_env("$(ARBHOME)/bin");
            if (strstr(path, arbbin) == 0) {
                GB_warningf("Your PATH variable does not contain '%s'. Things may not work as expected.", arbbin);
            }
            free(arbbin);
        }
    }
    return path;
}

GB_CSTR GB_getenvARB_GS() {
    static const char *gs = 0;
    if (!gs) {
        gs = getenv_executable("ARB_GS"); // doc in ../HELP_SOURCE/oldhelp/arb_envar.hlp@ARB_GS
        if (!gs) gs = GB_find_executable("Postscript viewer", "gv", "ghostview", NULL);
    }
    return gs;
}

GB_CSTR GB_getenvARB_PDFVIEW() {
    static const char *pdfview = 0;
    if (!pdfview) {
        pdfview = getenv_executable("ARB_PDFVIEW"); // doc in ../HELP_SOURCE/oldhelp/arb_envar.hlp@ARB_PDFVIEW
        if (!pdfview) pdfview = GB_find_executable("PDF viewer", "epdfview", "xpdf", "kpdf", "acroread", "gv", NULL);
    }
    return pdfview;
}

GB_CSTR GB_getenvARB_TEXTEDIT() {
    static const char *editor = 0;
    if (!editor) {
        editor = getenv_executable("ARB_TEXTEDIT"); // doc in ../HELP_SOURCE/oldhelp/arb_envar.hlp@ARB_TEXTEDIT
        if (!editor) editor = "arb_textedit"; // a smart editor shell script
    }
    return editor;
}

GB_CSTR GB_getenvDOCPATH() {
    static const char *dp = 0;
    if (!dp) {
        char *res = getenv_existing_directory("ARB_DOC"); // doc in ../HELP_SOURCE/oldhelp/arb_envar.hlp@ARB_DOC
        if (res) dp = res;
        else     dp = strdup(GB_path_in_ARBLIB("help", NULL));
    }
    return dp;
}

GB_CSTR GB_getenvHTMLDOCPATH() {
    static const char *dp = 0;
    if (!dp) {
        char *res = getenv_existing_directory("ARB_HTMLDOC"); // doc in ../HELP_SOURCE/oldhelp/arb_envar.hlp@ARB_HTMLDOC
        if (res) dp = res;
        else     dp = strdup(GB_path_in_ARBLIB("help_html", NULL));
    }
    return dp;

}

GB_CSTR GB_getenv(const char *env) {
    if (strncmp(env, "ARB", 3) == 0) {

        // doc in ../HELP_SOURCE/oldhelp/arb_envar.hlp

        if (strcmp(env, "ARBCONFIG")    == 0) return GB_getenvARBCONFIG();
        if (strcmp(env, "ARBMACROHOME") == 0) return GB_getenvARBMACROHOME();
        if (strcmp(env, "ARBMACRO")     == 0) return GB_getenvARBMACRO();
        if (strcmp(env, "ARBHOME")      == 0) return GB_getenvARBHOME();
        if (strcmp(env, "ARB_GS")       == 0) return GB_getenvARB_GS();
        if (strcmp(env, "ARB_PDFVIEW")  == 0) return GB_getenvARB_PDFVIEW();
        if (strcmp(env, "ARB_DOC")      == 0) return GB_getenvDOCPATH();
        if (strcmp(env, "ARB_TEXTEDIT") == 0) return GB_getenvARB_TEXTEDIT();
    }
    else {
        if (strcmp(env, "HOME") == 0) return GB_getenvHOME();
        if (strcmp(env, "USER") == 0) return GB_getenvUSER();
    }

    return getenv_ignore_empty(env);
}



int GB_host_is_local(const char *hostname) {
    // returns 1 if host is local
    if (strcmp(hostname, "localhost")      == 0) return 1;
    if (strcmp(hostname, GB_get_hostname()) == 0) return 1;
    if (strstr(hostname, "127.0.0.")       == hostname) return 1;
    return 0;
}

#define MIN(a, b) (((a)<(b)) ? (a) : (b))

GB_ULONG GB_get_physical_memory() {
    // Returns the physical available memory size in k available for one process
    GB_ULONG memsize; // real existing memory in k

#if defined(LINUX)
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
        mib[1] = HW_MEMSIZE; // uint64_t: physical ram size
        len = sizeof(bytes);
        sysctl(mib, 2, &bytes, &len, NULL, 0);

        memsize = bytes/1024;
    }
#else
    memsize = 1024*1024; // assume 1 Gb
    printf("\n"
           "Warning: ARB is not prepared to detect the memory size on your system!\n"
           "         (it assumes you have %ul Mb,  but does not use more)\n\n", memsize/1024);
#endif

    GB_ULONG net_memsize = memsize - 10240;         // reduce by 10Mb

    // detect max allocateable memory by ... allocating
    GB_ULONG max_malloc_try = net_memsize*1024;
    GB_ULONG max_malloc     = 0;
    {
        GB_ULONG step_size  = 4096;
        void *head = 0;

        do {
            void **tmp;
            while ((tmp=(void**)malloc(step_size))) {
                *tmp        = head;
                head        = tmp;
                max_malloc += step_size;
                if (max_malloc >= max_malloc_try) break;
                step_size *= 2;
            }
        } while ((step_size=step_size/2) > sizeof(void*));

        while (head) freeset(head, *(void**)head);
        max_malloc /= 1024;
    }

    GB_ULONG usedmemsize = (MIN(net_memsize, max_malloc)*95)/100; // arb uses max. 95 % of available memory (was 70% in the past)

#if defined(DEBUG)
    printf("- memsize(real)        = %20lu k\n", memsize);
    printf("- memsize(net)         = %20lu k\n", net_memsize);
    printf("- memsize(max_malloc)  = %20lu k\n", max_malloc);
#endif // DEBUG
    printf("- memsize(used by ARB) = %20lu k\n", usedmemsize);

    arb_assert(usedmemsize != 0);

    return usedmemsize;
}

// ---------------------------------------------
// path completion (parts former located in AWT)

static int  path_toggle = 0;
static char path_buf[2][PATH_MAX];

GB_CSTR GB_append_suffix(const char *name, const char *suffix) {
    // if suffix != NULL -> append .suffix
    // (automatically removes duplicated '.'s)

    GB_CSTR result = name;
    if (suffix) {
        while (suffix[0] == '.') suffix++;
        if (suffix[0]) {
            path_toggle = 1-path_toggle; // use 2 buffers in turn
            result = GBS_global_string_to_buffer(path_buf[path_toggle], PATH_MAX, "%s.%s", name, suffix);
        }
    }
    return result;
}

GB_CSTR GB_get_full_path(const char *anypath) {
    // expands '~' '..' etc in 'anypath'

    GB_CSTR result = NULL;
    if (!anypath) {
        GB_export_error("NULL path (internal error)");
    }
    else if (strlen(anypath) >= PATH_MAX) {
        GB_export_errorf("Path too long (> %i chars)", PATH_MAX-1);
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
    // '/' is inserted in-between
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

#ifdef P_tmpdir
#define GB_PATH_TMP P_tmpdir
#else
#define GB_PATH_TMP "/tmp"
#endif

FILE *GB_fopen_tempfile(const char *filename, const char *fmode, char **res_fullname) {
    // fopens a tempfile
    //
    // Returns
    // - NULL in case of error (which is exported then)
    // - otherwise returns open filehandle
    //
    // Always sets
    // - heap-copy of used filename in 'res_fullname' (if res_fullname != NULL)
    // (even if fopen failed)

    GB_CSTR   file  = GB_concat_path(GB_PATH_TMP, filename);
    bool      write = strpbrk(fmode, "wa") != 0;
    GB_ERROR  error = 0;
    FILE     *fp    = 0;

    fp = fopen(file, fmode);
    if (fp) {
        // make file private
        if (fchmod(fileno(fp), S_IRUSR|S_IWUSR) != 0) {
            error = GB_export_IO_error("changing permissions of", file);
        }
    }
    else {
        error = GB_export_IO_error(GBS_global_string("opening(%s) tempfile", write ? "write" : "read"), file);
    }

    if (res_fullname) {
        *res_fullname = file ? strdup(file) : 0;
    }

    if (error) {
        // don't care if anything fails here..
        if (fp) { fclose(fp); fp = 0; }
        if (file) { unlink(file); file = 0; }
        GB_export_error(error);
    }

    return fp;
}

char *GB_create_tempfile(const char *name) {
    // creates a tempfile and returns full name of created file
    // returns NULL in case of error (which is exported then)

    char *fullname;
    FILE *out = GB_fopen_tempfile(name, "wt", &fullname);

    if (out) fclose(out);
    return fullname;
}

char *GB_unique_filename(const char *name_prefix, const char *suffix) {
    // generates a unique (enough) filename
    //
    // scheme: name_prefix_USER_PID_COUNT.suffix

    static int counter = 0;
    return GBS_global_string_copy("%s_%s_%i_%i.%s",
                                  name_prefix,
                                  GB_getenvUSER(), getpid(), counter++,
                                  suffix);
}

static GB_HASH *files_to_remove_on_exit = 0;
static long exit_remove_file(const char *key, long val, void *unused) {
    // GBUSE(val);
    // GBUSE(unused);
    if (unlink(key) != 0) {
        fprintf(stderr, "Warning: %s\n", GB_export_IO_error("removing", key));
    }
    return 0;
}
static void exit_removal() {
    if (files_to_remove_on_exit) {
        GBS_hash_do_loop(files_to_remove_on_exit, exit_remove_file, NULL);
    }
}
void GB_remove_on_exit(const char *filename) {
    // mark a file for removal on exit

    if (!files_to_remove_on_exit) {
        files_to_remove_on_exit = GBS_create_hash(20, GB_MIND_CASE);
        atexit(exit_removal);
    }
    GBS_write_hash(files_to_remove_on_exit, filename, 1);
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
        if (res_suffix)    *res_suffix    = ldot ? GB_strpartdup(ldot+1, terminal-1) : NULL;
    }
    else {
        if (res_dir)       *res_dir       = NULL;
        if (res_fullname)  *res_fullname  = NULL;
        if (res_name_only) *res_name_only = NULL;
        if (res_suffix)    *res_suffix    = NULL;
    }
}
