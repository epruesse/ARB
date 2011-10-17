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

#include <climits>
#include <cstdarg>
#include <cctype>

#include <netdb.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>

#include <arb_cs.h>
#include <arb_str.h>
#include <arb_strbuf.h>
#include <arb_file.h>

#include "gb_comm.h"
#include "gb_data.h"
#include "gb_localdata.h"
#include "gb_main.h"

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

void gbcm_read_flush() {
    gb_local->write_ptr  = gb_local->write_buffer;
    gb_local->write_free = gb_local->write_bufsize;
}

__ATTR__USERESULT inline long gbcm_read_buffered(int socket, char *ptr, long size) {
    // copy size 'byte' from read-buffer to 'ptr'.
    // read next block from 'socket' if necessary.
    // 
    // return number of bytes copied (or 0 on IOfailure)
    
    gb_assert(size>0);
    long holding = gb_local->write_bufsize - gb_local->write_free;
    if (holding <= 0) {
        holding = read(socket, gb_local->write_buffer, (size_t)gb_local->write_bufsize);
        if (holding < 0) return 0;

        gbcm_read_flush();
        gb_local->write_free -= holding;
    }

    if (size>holding) size  = holding;
    memcpy(ptr, gb_local->write_ptr, (int)size);
    gb_local->write_ptr    += size;
    gb_local->write_free   += size;
    return size;
}

__ATTR__USERESULT inline long gbcm_read(int socket, char *ptr, long size) {
    // read 'size' byte from 'socket' to 'ptr'
    // return 'size' or 0 in case of error
    gb_assert(size>0);
    long leftsize = size;
    while (leftsize) {
        long readsize = gbcm_read_buffered(socket, ptr, leftsize);
        if (readsize<=0) return 0;

        ptr      += readsize;
        leftsize -= readsize;
    }
    return size;
}

GBCM_ServerResult gbcm_read_expect_size(int socket, char *ptr, long size) { // goes to header: __ATTR__USERESULT
    long read = gbcm_read(socket, ptr, size);
    if (read == size) return GBCM_ServerResult::OK();
    return GBCM_ServerResult::FAULT(GBS_global_string("expected to receive %li bytes, got %li (reason: %s)",
                                                      size, read, strerror(errno)));
}

GBCM_ServerResult gbcm_write_flush(int socket) { // goes to header: __ATTR__USERESULT
    char *ptr      = gb_local->write_buffer;
    long  leftsize = gb_local->write_ptr - ptr;

    gb_local->write_free = gb_local->write_bufsize;

    GBCM_ServerResult result = GBCM_ServerResult::OK();

    if (leftsize) {
        gb_assert(leftsize>0);

        gb_local->write_ptr      = ptr;
        gbcm_pipe_violation_flag = 0;

        while (result.ok() && leftsize>0) {
            long writesize = write(socket, ptr, (size_t)leftsize);

            if (gbcm_pipe_violation_flag) {
                result = GBCM_ServerResult::FAULT("broken pipe");
            }
            else if (writesize<0) {
                result = GBCM_ServerResult::FAULT(GB_IO_error("writing", GBS_global_string("<socket %i>", socket)));
            }

            if (result.failed()) {
                if (gb_local->iamclient) {
                    fprintf(stderr, "DB_Server is killed, Now I kill myself\n");
                    exit(-1);
                }
            }
            else {
                ptr      += writesize;
                leftsize -= writesize;

                if (leftsize>0) GB_usleep(10000);
            }
        }
    }
    return result;
}

GBCM_ServerResult gbcm_write(int socket, const char *ptr, long size) { // goes to header: __ATTR__USERESULT
    while (size >= gb_local->write_free) {
        memcpy(gb_local->write_ptr, ptr, (int)gb_local->write_free);

        gb_local->write_ptr  += gb_local->write_free;
        size                 -= gb_local->write_free;
        ptr                  += gb_local->write_free;
        gb_local->write_free  = 0;

        GBCM_ServerResult result = gbcm_write_flush(socket);
        if (result.failed()) return result;
    }

    memcpy(gb_local->write_ptr, ptr, (int)size);
    gb_local->write_ptr  += size;
    gb_local->write_free -= size;

    return GBCM_ServerResult::OK();
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
    long      socket_id;
    char     *mach_name;
    in_addr   addr;                                 // union -> u_long
    hostent  *he;
    GB_ERROR  err;

    mach_name = 0;
    err = gbcm_get_m_id(path, &mach_name, &socket_id);
    if (err) {
        if (mach_name) free(mach_name);
        return err;
    }
    if (socket_id >= 0) {    // TCP
        sockaddr_in so_ad;
        memset((char *)&so_ad, 0, sizeof(sockaddr_in));
        *psocket = socket(PF_INET, SOCK_STREAM, 0);
        if (*psocket <= 0) {
            return "CANNOT CREATE SOCKET";
        }

        arb_gethostbyname(mach_name, he, err);
        if (err) return err;

        // simply take first address
        addr.s_addr = *(long *) (he->h_addr);
        so_ad.sin_addr = addr;
        so_ad.sin_family = AF_INET;
        so_ad.sin_port = htons((unsigned short)socket_id); // @@@ = pb_socket
        if (do_connect) {
            if (connect(*psocket, (sockaddr *)(&so_ad), sizeof(so_ad))) {
                GB_warningf("Cannot connect to %s:%li   errno %i", mach_name, socket_id, errno);
                return "";
            }
        }
        else {
            int one = 1;
            setsockopt(*psocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one));
            if (bind(*psocket, (sockaddr *)(&so_ad), sizeof(so_ad))) {
                return "Could not open socket on Server";
            }
        }
        if (mach_name) free(mach_name);
        if (delay2 == TCP_NODELAY) {
            GB_UINT4      optval;
            optval = 1;
            setsockopt(*psocket, IPPROTO_TCP, TCP_NODELAY, (char *)&optval, sizeof (GB_UINT4));
        }
        *unix_name = 0;
        return 0;
    }
    else {        // UNIX
        sockaddr_un so_ad;
        memset((char *)&so_ad, 0, sizeof(so_ad));
        *psocket = socket(PF_UNIX, SOCK_STREAM, 0);
        if (*psocket <= 0) {
            return "CANNOT CREATE SOCKET";
        }
        so_ad.sun_family = AF_UNIX;
        strcpy(so_ad.sun_path, mach_name);

        if (do_connect) {
            if (connect(*psocket, (sockaddr*)(&so_ad), strlen(so_ad.sun_path)+2)) {
                if (mach_name) free(mach_name);
                return "";
            }
        }
        else {
            if (unlink(mach_name) == 0) printf("old socket found\n");
            if (bind(*psocket, (sockaddr*)(&so_ad), strlen(mach_name)+2)) {
                if (mach_name) free(mach_name);
                return "Could not open socket on Server";
            }
            if (chmod(mach_name, 0777)) return GB_export_errorf("Cannot change mode of socket '%s'", mach_name);
        }
        *unix_name = mach_name;
        return 0;
    }
}

static void gbcmc_suppress_sigpipe(int) {
}

inline bool is_default_or_ignore_or_own_sighandler(SigHandler sh) {
    return sh == gbcmc_suppress_sigpipe || is_default_or_ignore_sighandler(sh);
}

gbcmc_comm *gbcmc_open(const char *path) {
    gbcmc_comm *link = (gbcmc_comm *)GB_calloc(sizeof(gbcmc_comm), 1);
    GB_ERROR    err  = gbcm_open_socket(path, TCP_NODELAY, 1, &link->socket, &link->unix_name);

    if (err) {
        if (link->unix_name) free(link->unix_name); // @@@
        free(link);
        if (*err) {
            GB_internal_errorf("ARB_DB_CLIENT_OPEN\n(Reason: %s)", err);
        }
        return 0;
    }
    ASSERT_RESULT_PREDICATE(is_default_or_ignore_or_own_sighandler,
                            link->old_SIGPIPE_handler = INSTALL_SIGHANDLER(SIGPIPE, gbcmc_suppress_sigpipe, "gbcmc_open"));
    gb_local->iamclient = 1;
    return link;
}

void gbcmc_restore_sighandlers(gbcmc_comm * link) {
    ASSERT_RESULT(SigHandler, gbcmc_suppress_sigpipe, INSTALL_SIGHANDLER(SIGPIPE, link->old_SIGPIPE_handler, "gbcmc_close"));
}

GBCM_ServerResult gbcm_write_string(int socket, const char *key) { // goes to header: __ATTR__USERESULT
    GBCM_ServerResult result = GBCM_ServerResult::OK();
    if (key) {
        size_t len = strlen(key);

        result                         = gbcm_write_long(socket, len);
        if (result.ok() && len) result = gbcm_write(socket, key, len);;
    }
    else {
        result = gbcm_write_long(socket, -1);
    }
    return result;
}

char *gbcm_read_string(int socket, GBCM_ServerResult& result) {
    char *key = 0;
    long  len = gbcm_read_long(socket, result);

    if (result.ok()) {
        if (len) {
            if (len>0) {
                key    = (char *)GB_calloc(sizeof(char), (size_t)len+1);
                result = gbcm_read_expect_size(socket, key, len);
            }
        }
        else {
            key = strdup("");
        }
    }
    return key;
}

GBCM_ServerResult gbcm_write_long(int socket, long data) { // goes to header: __ATTR__USERESULT
    return gbcm_write(socket, (char*)&data, sizeof(data));
}

long gbcm_read_long(int socket, GBCM_ServerResult& result) {
    long data = 0;
    if (result.ok()) result = gbcm_read_expect_size(socket, (char*)&data, sizeof(data));
    return data;
}

char *GB_read_fp(FILE *in) {
    /*! like GB_read_file(), but works on already open file
     * (useful together with GB_fopen_tempfile())
     *
     * Note: File should be opened in text-mode (e.g. "rt")
     */

    GBS_strstruct *buf = GBS_stropen(4096);
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
     * @return NULL in case of error (use GB_await_error() to get the message)
     */
    char *result = 0;

    if (strcmp(path, "-") == 0) {
        result = GB_read_fp(stdin);
    }
    else {
        char *epath = GBS_eval_env(path);

        if (epath) {
            FILE *in = fopen(epath, "rt");

            if (!in) GB_export_error(GB_IO_error("reading", epath));
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

GB_ULONG GB_time_of_day() {
    timeval tp;
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
    GB_ERROR    error   = GBK_system(command);
    free(fpath);
    return GB_failedTo_error("print textfile", fpath, error);
}

GB_ERROR GB_xterm() {
    // goes to header: __ATTR__USERESULT
    const char *xt      = GB_getenvARB_XTERM();
    const char *command = GBS_global_string("%s &", xt);
    return GBK_system(command);
}

GB_ERROR GB_xcmd(const char *cmd, bool background, bool wait_only_if_error) {
    // goes to header: __ATTR__USERESULT_TODO

    // runs a command in an xterm
    // if 'background' is true -> run asynchronous
    // if 'wait_only_if_error' is true -> asynchronous does wait for keypress only if cmd fails

    GBS_strstruct *strstruct = GBS_stropen(1024);
    const char    *xcmd      = GB_getenvARB_XCMD();

    GBS_strcat(strstruct, "(");
    GBS_strcat(strstruct, xcmd);
    GBS_strcat(strstruct, " bash -c 'LD_LIBRARY_PATH=\"");
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

    GB_ERROR error = GBK_system(GBS_mempntr(strstruct));
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

void GB_setenv(const char *var, const char *value) {
    if (setenv(var, value, 1) != 0) {
        GB_warningf("Could not set environment variable '%s'. This might cause problems in subprocesses.\n"
                    "(Reason: %s)", var, strerror(errno));
    }
}

GB_CSTR GB_getenvARB_XTERM() {
    static const char *xterm = 0;
    if (!xterm) {
        xterm = getenv_ignore_empty("ARB_XTERM"); // doc in ../HELP_SOURCE/oldhelp/arb_envar.hlp@ARB_XTERM
        if (!xterm) xterm = "xterm -sl 1000 -sb -geometry 120x50";
    }
    return xterm;
}

GB_CSTR GB_getenvARB_XCMD() {
    static const char *xcmd = 0;
    if (!xcmd) {
        xcmd = getenv_ignore_empty("ARB_XCMD"); // doc in ../HELP_SOURCE/oldhelp/arb_envar.hlp@ARB_XCMD
        if (!xcmd) {
            const char *xterm = GB_getenvARB_XTERM();
            gb_assert(xterm);
            xcmd = GBS_global_string_copy("%s -e", xterm);
        }
    }
    return xcmd;
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
    static SmartCharPtr Home;
    if (Home.isNull()) {
        char *home = getenv_existing_directory("HOME");
        if (!home) {
            home = nulldup(GB_getcwd());
            if (!home) home = strdup(".");
            fprintf(stderr, "WARNING: Cannot identify user's home directory: environment variable HOME not set\n"
                    "Using current directory (%s) as home.\n", home);
        }
        gb_assert(home);
        Home = home;
    }
    return &*Home;
}

GB_CSTR GB_getenvARBHOME() {
    static SmartCharPtr Arbhome;
    if (Arbhome.isNull()) {
        char *arbhome = getenv_existing_directory("ARBHOME"); // doc in ../HELP_SOURCE/oldhelp/arb_envar.hlp@ARBHOME
        if (!arbhome) {
            fprintf(stderr, "Fatal ERROR: Environment Variable ARBHOME not found !!!\n"
                    "   Please set 'ARBHOME' to the installation path of ARB\n");
            exit(-1);
        }
        Arbhome = arbhome;
    }
    return &*Arbhome;
}

GB_CSTR GB_getenvARBMACRO() {
    static const char *am = 0;
    if (!am) {
        am          = getenv_existing_directory("ARBMACRO"); // doc in ../HELP_SOURCE/oldhelp/arb_envar.hlp@ARBMACRO
        if (!am) am = strdup(GB_path_in_ARBLIB("macros"));
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
                if (error) GB_warning(error);
            }
        }
    }
    return dir;
}

GB_CSTR GB_getenvARBMACROHOME() {
    static const char *amh = 0;
    if (!amh) amh = getenv_autodirectory("ARBMACROHOME", GB_path_in_arbprop("macros"));  // doc in ../HELP_SOURCE/oldhelp/arb_envar.hlp@ARBMACROHOME
    return amh;
}

GB_CSTR GB_getenvARBCONFIG() {
    static const char *ac = 0;
    if (!ac) ac = getenv_autodirectory("ARBCONFIG", GB_path_in_arbprop("cfgSave")); // doc in ../HELP_SOURCE/oldhelp/arb_envar.hlp@ARBCONFIG
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
        else     dp = strdup(GB_path_in_ARBLIB("help"));
    }
    return dp;
}

GB_CSTR GB_getenvHTMLDOCPATH() {
    static const char *dp = 0;
    if (!dp) {
        char *res = getenv_existing_directory("ARB_HTMLDOC"); // doc in ../HELP_SOURCE/oldhelp/arb_envar.hlp@ARB_HTMLDOC
        if (res) dp = res;
        else     dp = strdup(GB_path_in_ARBLIB("help_html"));
    }
    return dp;

}

static gb_getenv_hook getenv_hook = NULL;

NOT4PERL gb_getenv_hook GB_install_getenv_hook(gb_getenv_hook hook) {
    // Install 'hook' to be called by GB_getenv().
    // If the 'hook' returns NULL, normal expansion takes place.
    // Otherwise GB_getenv() returns result from 'hook'

    gb_getenv_hook oldHook = getenv_hook;
    getenv_hook            = hook;
    return oldHook;
}

GB_CSTR GB_getenv(const char *env) {
    if (getenv_hook) {
        const char *result = getenv_hook(env);
        if (result) return result;
    }
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
        if (strcmp(env, "ARB_XTERM") == 0)    return GB_getenvARB_XTERM();
        if (strcmp(env, "ARB_XCMD") == 0)     return GB_getenvARB_XCMD();
    }
    else {
        if (strcmp(env, "HOME") == 0) return GB_getenvHOME();
        if (strcmp(env, "USER") == 0) return GB_getenvUSER();
    }

    return getenv_ignore_empty(env);
}

struct export_environment {
    export_environment() {
        // set all variables needed in ARB subprocesses
        GB_setenv("ARB_XCMD", GB_getenvARB_XCMD());
    }
};

static export_environment expenv; 

bool GB_host_is_local(const char *hostname) {
    // returns true if host is local
    return
        ARB_stricmp(hostname, "localhost")       == 0 ||
        ARB_strscmp(hostname, "127.0.0.")        == 0 ||
        ARB_stricmp(hostname, arb_gethostname()) == 0;
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

#if defined(DEBUG) && 0
    printf("- memsize(real)        = %20lu k\n", memsize);
    printf("- memsize(net)         = %20lu k\n", net_memsize);
    printf("- memsize(max_malloc)  = %20lu k\n", max_malloc);
    printf("- memsize(used by ARB) = %20lu k\n", usedmemsize);
#endif // DEBUG

    arb_assert(usedmemsize != 0);

    return usedmemsize;
}

// ---------------------------------------------
// path completion (parts former located in AWT)
// @@@ whole section (+ corresponding tests) should move to adfile.cxx

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

GB_CSTR GB_canonical_path(const char *anypath) {
    // expands '~' '..' symbolic links etc in 'anypath'.
    // 
    // Never returns NULL (if called correctly)
    // Instead might return non-canonical path (when a directory
    // in 'anypath' does not exist)

    GB_CSTR result = NULL;
    if (!anypath) {
        GB_export_error("NULL path (internal error)");
    }
    else if (strlen(anypath) >= PATH_MAX) {
        GB_export_errorf("Path too long (> %i chars)", PATH_MAX-1);
    }
    else {
        if (anypath[0] == '~' && (!anypath[1] || anypath[1] == '/')) {
            GB_CSTR home    = GB_getenvHOME();
            GB_CSTR homeexp = GBS_global_string("%s%s", home, anypath+1);
            result          = GB_canonical_path(homeexp);
            GBS_reuse_buffer(homeexp);
        }
        else {
            result = realpath(anypath, path_buf[1-path_toggle]);
            if (result) {
                path_toggle = 1-path_toggle;
            }
            else { // realpath failed, content of path_buf[path_toggle] is UNDEFINED!
                char *dir, *fullname;
                GB_split_full_path(anypath, &dir, &fullname, NULL, NULL);

                gb_assert(dir);                       // maybe you'd like to use GB_unfold_path()
                gb_assert(strcmp(dir, anypath) != 0); // avoid deadlock

                const char *canonical_dir = GB_canonical_path(dir);
                gb_assert(canonical_dir);

                result = GB_concat_path(canonical_dir, fullname);

                free(dir);
                free(fullname);
            }
        }
        gb_assert(result);
    }
    return result;
}

GB_CSTR GB_concat_path(GB_CSTR anypath_left, GB_CSTR anypath_right) {
    // concats left and right part of a path.
    // '/' is inserted in-between
    //
    // if one of the arguments is NULL => returns the other argument
    // if both arguments are NULL      => return NULL (@@@ maybe forbid?)

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
    // like GB_concat_path(), but returns the canonical path
    GB_CSTR result = GB_concat_path(anypath_left, anypath_right);

    gb_assert(result != anypath_left); // consider using GB_canonical_path() directly
    gb_assert(result != anypath_right);

    if (result) result = GB_canonical_path(result);
    return result;
}

inline bool is_absolute_path(const char *path) { return path[0] == '/' || path[0] == '~'; }
inline bool is_name_of_envvar(const char *name) {
    for (int i = 0; name[i]; ++i) {
        if (isalnum(name[i]) || name[i] == '_') continue;
        return false;
    }
    return true;
}

GB_CSTR GB_unfold_path(const char *pwd_envar, const char *path) {
    // If 'path' is an absolute path, return canonical path.
    // 
    // Otherwise unfolds relative 'path' using content of environment
    // variable 'pwd_envar' as start directory.
    // If environment variable is not defined, fall-back to current directory

    gb_assert(is_name_of_envvar(pwd_envar));
    if (is_absolute_path(path)) {
        return GB_canonical_path(path);
    }

    const char *pwd = GB_getenv(pwd_envar);
    if (!pwd) pwd = GB_getcwd(); // @@@ really wanted ?
    return GB_concat_full_path(pwd, path);
}

GB_CSTR GB_path_in_ARBHOME(const char *relative_path) {
    return GB_unfold_path("ARBHOME", relative_path);
}
GB_CSTR GB_path_in_ARBLIB(const char *relative_path) {
    return GB_path_in_ARBHOME("lib", relative_path);
}
GB_CSTR GB_path_in_arbprop(const char *relative_path) {
    return GB_unfold_path("HOME", GB_concat_path(".arb_prop", relative_path));
}
GB_CSTR GB_path_in_ARBHOME(const char *relative_path_left, const char *anypath_right) {
    return GB_path_in_ARBHOME(GB_concat_path(relative_path_left, anypath_right));
}
GB_CSTR GB_path_in_ARBLIB(const char *relative_path_left, const char *anypath_right) {
    return GB_path_in_ARBLIB(GB_concat_path(relative_path_left, anypath_right));
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
            error = GB_IO_error("changing permissions of", file);
        }
    }
    else {
        error = GB_IO_error(GBS_global_string("opening(%s) tempfile", write ? "write" : "read"), file);
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
static long exit_remove_file(const char *file, long, void *) {
    if (unlink(file) != 0) {
        fprintf(stderr, "Warning: %s\n", GB_IO_error("removing", file));
    }
    return 0;
}
static void exit_removal() {
    if (files_to_remove_on_exit) {
        GBS_hash_do_loop(files_to_remove_on_exit, exit_remove_file, NULL);
        GBS_free_hash(files_to_remove_on_exit);
        files_to_remove_on_exit = NULL;
    }
}
void GB_remove_on_exit(const char *filename) {
    // mark a file for removal on exit

    if (!files_to_remove_on_exit) {
        files_to_remove_on_exit = GBS_create_hash(20, GB_MIND_CASE);
        GB_atexit(exit_removal);
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


// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS

#include <test_unit.h>

static const char *ANY_NAME = "ANY_NAME";

struct gbcm_get_m_id_TESTER : virtual Noncopyable {
    const char *path;
    GB_ERROR    error;
    char       *name;
    long        id;

    gbcm_get_m_id_TESTER(const char *path_)
        : path(path_)
        , name(NULL)
    {
        error = gbcm_get_m_id(path, &name, &id);
    }
    ~gbcm_get_m_id_TESTER() {
        TEST_ASSERT(!error || !name);               // if error occurs no name should exist!
        TEST_ASSERT(error || name);                 // either error or result
        free(name);
    }

    bool no_error() {
        if (error) {
            fprintf(stderr, "Unexpected error for path '%s': %s\n", path, error);
            TEST_ASSERT(!name);
        }
        else if (!name) fprintf(stderr, "Neither error nor name for path '%s'\n", path);
        return !error;
    }
    bool parsed_name(const char *expected_name) {
        bool ok = no_error();
        if (ok && strcmp(expected_name, name) != 0) {
            if (expected_name != ANY_NAME) {
                fprintf(stderr, "path '%s' produces unexpected name '%s' (expected '%s')\n", path, name, expected_name);
                ok = false;
            }
        }
        return ok;
    }
    bool parsed_id(long expected_id) {
        bool ok = no_error();
        if (ok && id != expected_id) {
            fprintf(stderr, "path '%s' produces unexpected id '%li' (expected '%li')\n", path, id, expected_id);
            ok = false;
        }
        return ok;
    }

    bool parsed(const char *expected_name, long expected_id) { return parsed_name(expected_name) && parsed_id(expected_id); }
};

void TEST_gbcm_get_m_id() {
    TEST_ASSERT(gbcm_get_m_id_TESTER(NULL).error);
    TEST_ASSERT(gbcm_get_m_id_TESTER("").error);
    TEST_ASSERT(gbcm_get_m_id_TESTER(":").parsed(ANY_NAME, -1));

    TEST_ASSERT(gbcm_get_m_id_TESTER("localhost:71").parsed("localhost", 71)); // fixed with [6486]
    TEST_ASSERT(gbcm_get_m_id_TESTER("localhost:0").error);
    TEST_ASSERT(gbcm_get_m_id_TESTER("localhost:4096").error == NULL);
    TEST_ASSERT(gbcm_get_m_id_TESTER("localhost:4097").error);

    TEST_ASSERT(gbcm_get_m_id_TESTER(":/tmp/arb_pt_ralf1").parsed("/tmp/arb_pt_ralf1", -1));

    // @@@ no idea what "*" is used for, so the following tests are only descriptive!
    TEST_ASSERT(gbcm_get_m_id_TESTER("*whatever:bar").parsed("bar", -1));
    TEST_ASSERT(gbcm_get_m_id_TESTER("*:bar").parsed("bar", -1));
}

#define TEST_ASSERT_IS_CANONICAL(file)                  \
    do {                                                \
        char *dup = strdup(file);                       \
        TEST_ASSERT_EQUAL(GB_canonical_path(dup), dup); \
        free(dup);                                      \
    } while(0)

#define TEST_ASSERT_CANONICAL_TO(not_cano,cano)                         \
    do {                                                                \
        char *arb_not_cano = strdup(GB_concat_path(arbhome, not_cano)); \
        char *arb_cano     = strdup(GB_concat_path(arbhome, cano));     \
        TEST_ASSERT_EQUAL(GB_canonical_path(arb_not_cano), arb_cano);   \
        free(arb_cano);                                                 \
        free(arb_not_cano);                                             \
    } while (0)

void TEST_paths() {

    // test GB_concat_path
    TEST_ASSERT_EQUAL(GB_concat_path("a", NULL), "a");
    TEST_ASSERT_EQUAL(GB_concat_path(NULL, "b"), "b");
    TEST_ASSERT_EQUAL(GB_concat_path("a", "b"), "a/b");

    {
        char        *arbhome    = strdup(GB_getenvARBHOME());
        const char*  nosuchfile = "nosuchfile";
        const char*  somefile   = "arb_README.txt";

        char *somefile_in_arbhome   = strdup(GB_concat_path(arbhome, somefile));
        char *nosuchfile_in_arbhome = strdup(GB_concat_path(arbhome, nosuchfile));
        char *nosuchpath_in_arbhome = strdup(GB_concat_path(arbhome, "nosuchpath"));
        char *file_in_nosuchpath    = strdup(GB_concat_path(nosuchpath_in_arbhome, "whatever"));

        TEST_ASSERT(!GB_is_directory(nosuchpath_in_arbhome));

        // test GB_get_full_path
        TEST_ASSERT_IS_CANONICAL(somefile_in_arbhome);
        TEST_ASSERT_IS_CANONICAL(nosuchpath_in_arbhome);
        TEST_ASSERT_IS_CANONICAL(file_in_nosuchpath);

        TEST_ASSERT_CANONICAL_TO("./WINDOW/./../ARBDB/./arbdb.h",     "ARBDB/arbdb.h"); // test parent-path
        TEST_ASSERT_CANONICAL_TO("INCLUDE/arbdb.h",                   "ARBDB/arbdb.h"); // test symbolic link to file
        TEST_ASSERT_CANONICAL_TO("NAMES_COM/AISC/aisc.pa",            "AISC_COM/AISC/aisc.pa"); // test symbolic link to directory
        TEST_ASSERT_CANONICAL_TO("./NAMES_COM/AISC/..",               "AISC_COM"); // test parent-path through links
        TEST_ASSERT_CANONICAL_TO("./WINDOW/./../ARBDB/../nosuchpath", "nosuchpath");
        TEST_ASSERT_CANONICAL_TO("./WINDOW/./../nosuchpath/../ARBDB", "nosuchpath/../ARBDB"); // cannot resolve through non-existing paths!

        // test GB_unfold_path
        TEST_ASSERT_EQUAL(GB_unfold_path("ARBHOME", somefile), somefile_in_arbhome);
        TEST_ASSERT_EQUAL(GB_unfold_path("ARBHOME", nosuchfile), nosuchfile_in_arbhome);

        char *inhome = strdup(GB_unfold_path("HOME", "whatever"));
        TEST_ASSERT_EQUAL(inhome, GB_canonical_path("~/whatever"));
        free(inhome);

        // test unfolding absolute paths (HOME is ignored)
        TEST_ASSERT_EQUAL(GB_unfold_path("HOME", arbhome), arbhome);
        TEST_ASSERT_EQUAL(GB_unfold_path("HOME", somefile_in_arbhome), somefile_in_arbhome);
        TEST_ASSERT_EQUAL(GB_unfold_path("HOME", nosuchfile_in_arbhome), nosuchfile_in_arbhome);

        // test GB_path_in_ARBHOME
        TEST_ASSERT_EQUAL(GB_path_in_ARBHOME(somefile), somefile_in_arbhome);
        TEST_ASSERT_EQUAL(GB_path_in_ARBHOME(nosuchfile), nosuchfile_in_arbhome);

        free(file_in_nosuchpath);
        free(nosuchpath_in_arbhome);
        free(nosuchfile_in_arbhome);
        free(somefile_in_arbhome);
        free(arbhome);
    }

    TEST_ASSERT_EQUAL(GB_path_in_ARBLIB("help"), GB_path_in_ARBHOME("lib", "help"));
}

// ----------------------------------------

class TestFile : virtual Noncopyable {
    const char *name;
    bool open(const char *mode) {
        FILE *out = fopen(name, mode);
        if (out) fclose(out);
        return out;
    }
    void create() { ASSERT_RESULT(bool, true, open("w")); }
    void unlink() { ::unlink(name); }
public:
    TestFile(const char *name_) : name(name_) { create(); }
    ~TestFile() { if (exists()) unlink(); }
    const char *get_name() const { return name; }
    bool exists() { return open("r"); }
};

void TEST_GB_remove_on_exit() {
    {
        // first test class TestFile
        TestFile file("test1");
        TEST_ASSERT(file.exists());
        TEST_ASSERT(TestFile(file.get_name()).exists()); // removes the file
        TEST_ASSERT(!file.exists());
    }

    TestFile t("test1");
    {
        GB_shell shell;
        GBDATA *gb_main = GB_open("no.arb", "c");

        GB_remove_on_exit(t.get_name());
        GB_close(gb_main);
    }
    TEST_ASSERT(!t.exists());
}

#endif

