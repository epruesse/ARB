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

#if defined(DARWIN)
# include <sys/sysctl.h>
#endif // DARWIN

#include <arb_cs.h>
#include <arb_str.h>
#include <arb_strbuf.h>
#include <arb_file.h>
#include <arb_sleep.h>
#include <arb_pathlen.h>

#include "gb_comm.h"
#include "gb_data.h"
#include "gb_localdata.h"

#include <SigHandler.h>

#include <algorithm>
#include <arb_misc.h>
#include <arb_defs.h>

// ------------------------------------------------
//      private read and write socket functions

void gbcm_read_flush() {
    gb_local->write_ptr  = gb_local->write_buffer;
    gb_local->write_free = gb_local->write_bufsize;
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
        gbcm_read_flush();
        gb_local->write_free-=holding;
    }
    if (size>holding) size = holding;
    memcpy(ptr, gb_local->write_ptr, (int)size);
    gb_local->write_ptr += size;
    gb_local->write_free += size;
    return size;
}

long gbcm_read(int socket, char *ptr, long size) {
    long leftsize = size;
    while (leftsize) {
        long readsize = gbcm_read_buffered(socket, ptr, leftsize);
        if (readsize<=0) return 0;
        ptr += readsize;
        leftsize -= readsize;
    }

    return size;
}

GBCM_ServerResult gbcm_write_flush(int socket) {
    long     leftsize = gb_local->write_ptr - gb_local->write_buffer;
    ssize_t  writesize;
    char    *ptr      = gb_local->write_buffer;

    // once we're done, the buffer will be free
    gb_local->write_free = gb_local->write_bufsize;
    gb_local->write_ptr = gb_local->write_buffer;
    
    while (leftsize) {
#ifdef MSG_NOSIGNAL
        // Linux has MSG_NOSIGNAL, but not SO_NOSIGPIPE
        // prevent SIGPIPE here
        writesize = send(socket, ptr, leftsize, MSG_NOSIGNAL);
#else
        writesize = write(socket, ptr, leftsize);
#endif

        if (writesize<0) {
            if (gb_local->iamclient) {
                fprintf(stderr, 
                        "Client (pid=%i) terminating after failure to contact database (%s).",
                        getpid(), strerror(errno));
                exit(EXIT_SUCCESS);
            }
            else {
                fprintf(stderr, "Error sending data to client (%s).", strerror(errno));
                return GBCM_SERVER_FAULT;
            }
        }
        ptr      += writesize;
        leftsize -= writesize;
    }

    return GBCM_SERVER_OK;
}

GBCM_ServerResult gbcm_write(int socket, const char *ptr, long size) {
    while (size >= gb_local->write_free) {
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

GB_ERROR gbcm_open_socket(const char *path, bool do_connect, int *psocket, char **unix_name) {
    if (path && strcmp(path, ":") == 0) {
        path = GBS_read_arb_tcp("ARB_DB_SERVER");
        if (!path) {
            return GB_await_error();
        }
    }

    return arb_open_socket(path, do_connect, psocket, unix_name);
}

#if defined(WARN_TODO)
#warning gbcms_close is unused
#endif
long gbcms_close(gbcmc_comm *link) {
    if (link->socket) {
        close(link->socket);
        link->socket = 0;
        if (link->unix_name) {
            unlink(link->unix_name);
        }
    }
    return 0;
}

gbcmc_comm *gbcmc_open(const char *path) {
    gbcmc_comm *link = (gbcmc_comm *)GB_calloc(sizeof(gbcmc_comm), 1);
    GB_ERROR    err  = gbcm_open_socket(path, true, &link->socket, &link->unix_name);

    if (err) {
        if (link->unix_name) free(link->unix_name); // @@@
        free(link);
        if (*err) {
            GB_internal_errorf("ARB_DB_CLIENT_OPEN\n(Reason: %s)", err);
        }
        return 0;
    }
    gb_local->iamclient = true;
    return link;
}

long gbcm_write_two(int socket, long a, long c) {
    long    ia[3];
    ia[0] = a;
    ia[1] = 3;
    ia[2] = c;
    if (!socket) return 1;
    return  gbcm_write(socket, (const char *)ia, sizeof(long)*3);
}


GBCM_ServerResult gbcm_read_two(int socket, long a, long *b, long *c) {
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

GBCM_ServerResult gbcm_write_string(int socket, const char *key) {
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

GBCM_ServerResult gbcm_write_long(int socket, long data) {
    gbcm_write(socket, (char*)&data, sizeof(data));
    return GBCM_SERVER_OK;
}

long gbcm_read_long(int socket) {
    long data;
    gbcm_read(socket, (char*)&data, sizeof(data));
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

char *GB_read_file(const char *path) { // consider using class FileContent instead
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

GB_ERROR GB_textprint(const char *path) {
    // goes to header: __ATTR__USERESULT
    char       *fpath        = GBS_eval_env(path);
    char       *quoted_fpath = GBK_singlequote(fpath);
    const char *command      = GBS_global_string("arb_textprint %s &", quoted_fpath);
    GB_ERROR    error        = GBK_system(command);
    error                    = GB_failedTo_error("print textfile", fpath, error);
    free(quoted_fpath);
    free(fpath);
    return error;
}

// --------------------------------------------------------------------------------

static GB_CSTR GB_getenvPATH() {
    static const char *path = 0;
    if (!path) {
        path = ARB_getenv_ignore_empty("PATH");
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

// --------------------------------------------------------------------------------
// Functions to find an executable

static char *GB_find_executable(GB_CSTR description_of_executable, ...) {
    // goes to header: __ATTR__SENTINEL
    /* search the path for an executable with any of the given names (...)
     * if any is found, it's full path is returned
     * if none is found, a warning call is returned (which can be executed without harm)
    */

    GB_CSTR  name;
    char    *found = 0;
    va_list  args;

    va_start(args, description_of_executable);
    while (!found && (name = va_arg(args, GB_CSTR)) != 0) found = ARB_executable(name, GB_getenvPATH());
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

static char *getenv_executable(GB_CSTR envvar) {
    // get full path of executable defined by 'envvar'
    // returns 0 if
    //  - envvar not defined or
    //  - not defining an executable (warns about that)

    char       *result   = 0;
    const char *exe_name = ARB_getenv_ignore_empty(envvar);

    if (exe_name) {
        result = ARB_executable(exe_name, GB_getenvPATH());
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
    const char *dir_name = ARB_getenv_ignore_empty(envvar);

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

static void GB_setenv(const char *var, const char *value) {
    if (setenv(var, value, 1) != 0) {
        GB_warningf("Could not set environment variable '%s'. This might cause problems in subprocesses.\n"
                    "(Reason: %s)", var, strerror(errno));
    }
}

static GB_CSTR GB_getenvARB_XTERM() {
    static const char *xterm = 0;
    if (!xterm) {
        xterm = ARB_getenv_ignore_empty("ARB_XTERM"); // doc in ../HELP_SOURCE/oldhelp/arb_envar.hlp@ARB_XTERM
        if (!xterm) xterm = "xterm -sl 1000 -sb -geometry 150x60";
    }
    return xterm;
}

static GB_CSTR GB_getenvARB_XCMD() {
    static const char *xcmd = 0;
    if (!xcmd) {
        xcmd = ARB_getenv_ignore_empty("ARB_XCMD"); // doc in ../HELP_SOURCE/oldhelp/arb_envar.hlp@ARB_XCMD
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
        user = ARB_getenv_ignore_empty("USER");
        if (!user) user = ARB_getenv_ignore_empty("LOGNAME");
        if (!user) {
            user = ARB_getenv_ignore_empty("HOME");
            if (user && strrchr(user, '/')) user = strrchr(user, '/')+1;
        }
        if (!user) {
            fprintf(stderr, "WARNING: Cannot identify user: environment variables USER, LOGNAME and HOME not set\n");
            user = "UnknownUser";
        }
    }
    return user;
}


static GB_CSTR GB_getenvHOME() {
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
            exit(EXIT_FAILURE);
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

static char *getenv_autodirectory(const char *envvar, const char *defaultDirectory) {
    // if environment variable 'envvar' contains an existing directory -> use that
    // otherwise fallback to 'defaultDirectory' (create if not existing)
    // return heap-copy of full directory name
    char *dir = getenv_existing_directory(envvar);
    if (!dir) {
        dir = GBS_eval_env(defaultDirectory);
        if (!GB_is_directory(dir)) {
            GB_ERROR error = GB_create_directory(dir);
            if (error) GB_warning(error);
        }
    }
    return dir;
}

GB_CSTR GB_getenvARB_PROP() {
    static SmartCharPtr ArbProps;
    if (ArbProps.isNull()) ArbProps = getenv_autodirectory("ARB_PROP", GB_path_in_HOME(".arb_prop")); // doc in ../HELP_SOURCE/oldhelp/arb_envar.hlp@ARB_PROP
    return &*ArbProps;
}

GB_CSTR GB_getenvARBMACROHOME() {
    static SmartCharPtr ArbMacroHome;
    if (ArbMacroHome.isNull()) ArbMacroHome = getenv_autodirectory("ARBMACROHOME", GB_path_in_arbprop("macros"));  // doc in ../HELP_SOURCE/oldhelp/arb_envar.hlp@ARBMACROHOME
    return &*ArbMacroHome;
}

GB_CSTR GB_getenvARBCONFIG() {
    static SmartCharPtr ArbConfig;
    if (ArbConfig.isNull()) ArbConfig = getenv_autodirectory("ARBCONFIG", GB_path_in_arbprop("cfgSave")); // doc in ../HELP_SOURCE/oldhelp/arb_envar.hlp@ARBCONFIG
    return &*ArbConfig;
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

        if (strcmp(env, "ARBHOME")      == 0) return GB_getenvARBHOME();
        if (strcmp(env, "ARB_PROP")     == 0) return GB_getenvARB_PROP();
        if (strcmp(env, "ARBCONFIG")    == 0) return GB_getenvARBCONFIG();
        if (strcmp(env, "ARBMACROHOME") == 0) return GB_getenvARBMACROHOME();
        if (strcmp(env, "ARBMACRO")     == 0) return GB_getenvARBMACRO();

        if (strcmp(env, "ARB_GS")       == 0) return GB_getenvARB_GS();
        if (strcmp(env, "ARB_PDFVIEW")  == 0) return GB_getenvARB_PDFVIEW();
        if (strcmp(env, "ARB_DOC")      == 0) return GB_getenvDOCPATH();
        if (strcmp(env, "ARB_TEXTEDIT") == 0) return GB_getenvARB_TEXTEDIT();
        if (strcmp(env, "ARB_XTERM")    == 0) return GB_getenvARB_XTERM();
        if (strcmp(env, "ARB_XCMD")     == 0) return GB_getenvARB_XCMD();
    }
    else {
        if (strcmp(env, "HOME") == 0) return GB_getenvHOME();
        if (strcmp(env, "USER") == 0) return GB_getenvUSER();
    }

    return ARB_getenv_ignore_empty(env);
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

    arb_assert(hostname);
    arb_assert(hostname[0]);

    return
        ARB_stricmp(hostname, "localhost")       == 0 ||
        ARB_strBeginsWith(hostname, "127.0.0.")       ||
        ARB_stricmp(hostname, arb_gethostname()) == 0;
}

static GB_ULONG get_physical_memory() {
    // Returns the physical available memory size in k available for one process
    static GB_ULONG physical_memsize = 0;
    if (!physical_memsize) {
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

        physical_memsize = std::min(net_memsize, max_malloc);

#if defined(DEBUG) && 0
        printf("- memsize(real)        = %20lu k\n", memsize);
        printf("- memsize(net)         = %20lu k\n", net_memsize);
        printf("- memsize(max_malloc)  = %20lu k\n", max_malloc);
#endif // DEBUG

        GB_informationf("Visible memory: %s", GBS_readable_size(physical_memsize*1024, "b"));
    }

    arb_assert(physical_memsize>0);
    return physical_memsize;
}

static GB_ULONG parse_env_mem_definition(const char *env_override, GB_ERROR& error) {
    const char *end;
    GB_ULONG    num = strtoul(env_override, const_cast<char**>(&end), 10);

    error = NULL;

    bool valid = num>0 || env_override[0] == '0';
    if (valid) {
        const char *formatSpec = end;
        double      factor     = 1;

        switch (tolower(formatSpec[0])) {
            case 0:
                num = GB_ULONG(num/1024.0+0.5); // byte->kb
                break; // no format given

            case 'g': factor *= 1024;
            case 'm': factor *= 1024;
            case 'k': break;

            case '%':
                factor = num/100.0;
                num    = get_physical_memory();
                break;

            default: valid = false; break;
        }

        if (valid) return GB_ULONG(num*factor+0.5);
    }

    error = "expected digits (optionally followed by k, M, G or %)";
    return 0;
}

GB_ULONG GB_get_usable_memory() {
    // memory allowed to be used by a single ARB process (in kbyte)

    static GB_ULONG useable_memory = 0;
    if (!useable_memory) {
        bool        allow_fallback = true;
        const char *env_override   = GB_getenv("ARB_MEMORY");
        const char *via_whom;
        if (env_override) {
            via_whom = "via envar ARB_MEMORY";
        }
        else {
          FALLBACK:
            env_override   = "90%"; // ARB processes do not use more than 90% of physical memory
            via_whom       = "by internal default";
            allow_fallback = false;
        }

        gb_assert(env_override);

        GB_ERROR env_error;
        GB_ULONG env_memory = parse_env_mem_definition(env_override, env_error);
        if (env_error) {
            GB_warningf("Ignoring invalid setting '%s' %s (%s)", env_override, via_whom, env_error);
            if (allow_fallback) goto FALLBACK;
            GBK_terminate("failed to detect usable memory");
        }

        GB_informationf("Restricting used memory (%s '%s') to %s", via_whom, env_override, GBS_readable_size(env_memory*1024, "b"));
        if (!allow_fallback) {
            GB_informationf("Note: Setting envar ARB_MEMORY will override that restriction (percentage or absolute memsize)");
        }
        useable_memory = env_memory;
        gb_assert(useable_memory>0 && useable_memory<get_physical_memory());
    }
    return useable_memory;
}

// ---------------------------
//      external commands

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

    const int     BUFSIZE = 1024;
    GBS_strstruct system_call(BUFSIZE);

    const char *xcmd = GB_getenvARB_XCMD();

    system_call.put('(');
    system_call.cat(xcmd);

    {
        GBS_strstruct bash_command(BUFSIZE);

        bash_command.cat("LD_LIBRARY_PATH=");
        {
            char *dquoted_library_path = GBK_doublequote(GB_getenv("LD_LIBRARY_PATH"));
            bash_command.cat(dquoted_library_path);
            free(dquoted_library_path);
        }
        bash_command.cat(";export LD_LIBRARY_PATH; (");
        bash_command.cat(cmd);

        const char *wait_commands = "echo; echo Press ENTER to close this window; read a";
        if (wait_only_if_error) {
            bash_command.cat(") || (");
            bash_command.cat(wait_commands);
        }
        else if (background) {
            bash_command.cat("; ");
            bash_command.cat(wait_commands);
        }
        bash_command.put(')');

        system_call.cat(" bash -c ");
        char *squoted_bash_command = GBK_singlequote(bash_command.get_data());
        system_call.cat(squoted_bash_command);
        free(squoted_bash_command);
    }
    system_call.cat(" )");
    if (background) system_call.cat(" &");

    return GBK_system(system_call.get_data());
}

// ---------------------------------------------
// path completion (parts former located in AWT)
// @@@ whole section (+ corresponding tests) should move to adfile.cxx

static int  path_toggle = 0;
static char path_buf[2][ARB_PATH_MAX];

static char *use_other_path_buf() {
    path_toggle = 1-path_toggle;
    return path_buf[path_toggle];
}

GB_CSTR GB_append_suffix(const char *name, const char *suffix) {
    // if suffix != NULL -> append .suffix
    // (automatically removes duplicated '.'s)

    GB_CSTR result = name;
    if (suffix) {
        while (suffix[0] == '.') suffix++;
        if (suffix[0]) {
            result = GBS_global_string_to_buffer(use_other_path_buf(), ARB_PATH_MAX, "%s.%s", name, suffix);
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
    else if (!anypath[0]) {
        result = "/";
    }
    else if (strlen(anypath) >= ARB_PATH_MAX) {
        GB_export_errorf("Path too long (> %i chars)", ARB_PATH_MAX-1);
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
            else { // realpath failed (happens e.g. when using a non-existing path, e.g. if user entered the name of a new file)
                   // => content of path_buf[path_toggle] is UNDEFINED!
                char *dir, *fullname;
                GB_split_full_path(anypath, &dir, &fullname, NULL, NULL);

                const char *canonical_dir = NULL;
                if (!dir) {
                    gb_assert(!strchr(anypath, '/'));
                    canonical_dir = GB_canonical_path("."); // use working directory
                }
                else {
                    gb_assert(strcmp(dir, anypath) != 0); // avoid deadlock
                    canonical_dir = GB_canonical_path(dir);
                }
                gb_assert(canonical_dir);

                // manually resolve '.' and '..' in non-existing parent directories
                if (strcmp(fullname, "..") == 0) {
                    char *parent;
                    GB_split_full_path(canonical_dir, &parent, NULL, NULL, NULL);
                    if (parent) {
                        result = strcpy(use_other_path_buf(), parent);
                        free(parent);
                    }
                }
                else if (strcmp(fullname, ".") == 0) {
                    result = canonical_dir;
                }

                if (!result) result = GB_concat_path(canonical_dir, fullname);

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

    if (anypath_right) {
        if (anypath_right[0] == '/') {
            result = GB_concat_path(anypath_left, anypath_right+1);
        }
        else if (anypath_left && anypath_left[0]) {
            if (anypath_left[strlen(anypath_left)-1] == '/') {
                result = GBS_global_string_to_buffer(use_other_path_buf(), sizeof(path_buf[0]), "%s%s", anypath_left, anypath_right);
            }
            else {
                result = GBS_global_string_to_buffer(use_other_path_buf(), sizeof(path_buf[0]), "%s/%s", anypath_left, anypath_right);
            }
        }
        else {
            result = anypath_right;
        }
    }
    else {
        result = anypath_left;
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

GB_CSTR GB_unfold_in_directory(const char *relative_directory, const char *path) {
    // If 'path' is an absolute path, return canonical path.
    //
    // Otherwise unfolds relative 'path' using 'relative_directory' as start directory.

    if (is_absolute_path(path)) return GB_canonical_path(path);
    return GB_concat_full_path(relative_directory, path);
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

static GB_CSTR GB_path_in_ARBHOME(const char *relative_path_left, const char *anypath_right) {
    return GB_path_in_ARBHOME(GB_concat_path(relative_path_left, anypath_right));
}

GB_CSTR GB_path_in_ARBHOME(const char *relative_path) {
    return GB_unfold_path("ARBHOME", relative_path);
}
GB_CSTR GB_path_in_ARBLIB(const char *relative_path) {
    return GB_path_in_ARBHOME("lib", relative_path);
}
GB_CSTR GB_path_in_HOME(const char *relative_path) {
    return GB_unfold_path("HOME", relative_path);
}
GB_CSTR GB_path_in_arbprop(const char *relative_path) {
    return GB_unfold_path("ARB_PROP", relative_path);
}
GB_CSTR GB_path_in_ARBLIB(const char *relative_path_left, const char *anypath_right) {
    return GB_path_in_ARBLIB(GB_concat_path(relative_path_left, anypath_right));
}
GB_CSTR GB_path_in_arb_temp(const char *relative_path) {
    return GB_path_in_HOME(GB_concat_path(".arb_tmp", relative_path));
}

#define GB_PATH_TMP GB_path_in_arb_temp("tmp") // = "~/.arb_tmp/tmp" (used wherever '/tmp' was used in the past)

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

    char     *file  = strdup(GB_concat_path(GB_PATH_TMP, filename));
    GB_ERROR  error = GB_create_parent_directory(file);
    FILE     *fp    = NULL;

    if (!error) {
        bool write = strpbrk(fmode, "wa") != 0;

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
    }

    if (error) {
        // don't care if anything fails here..
        if (fp) { fclose(fp); fp = 0; }
        if (file) unlink(file);
        GB_export_error(error);
    }

    free(file);

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
    // If result pointers (res_*) are non-NULL, they are assigned heap-copies of the split parts.
    // If parts are not valid (e.g. cause 'fullpath' doesn't have a .suffix) the corresponding result pointer
    // is set to NULL.
    //
    // The '/' and '.' characters at the split-positions will be removed (not included in the results-strings).
    // Exceptions:
    // - the '.' in 'res_fullname'
    // - the '/' if directory part is the rootdir
    //
    // Note:
    // - if the filename starts with '.' (and that is the only '.' in the filename, an empty filename is returned: "")

    if (fullpath && fullpath[0]) {
        const char *lslash     = strrchr(fullpath, '/');
        const char *name_start = lslash ? lslash+1 : fullpath;
        const char *ldot       = strrchr(lslash ? lslash : fullpath, '.');
        const char *terminal   = strchr(name_start, 0);

        gb_assert(terminal);
        gb_assert(name_start);
        gb_assert(terminal > fullpath); // ensure (terminal-1) is a valid character position in path

        if (!lslash && fullpath[0] == '.' && (fullpath[1] == 0 || (fullpath[1] == '.' && fullpath[2] == 0))) { // '.' and '..'
            if (res_dir)       *res_dir       = strdup(fullpath);
            if (res_fullname)  *res_fullname  = NULL;
            if (res_name_only) *res_name_only = NULL;
            if (res_suffix)    *res_suffix    = NULL;
        }
        else {
            if (res_dir)       *res_dir       = lslash ? GB_strpartdup(fullpath, lslash == fullpath ? lslash : lslash-1) : NULL;
            if (res_fullname)  *res_fullname  = GB_strpartdup(name_start, terminal-1);
            if (res_name_only) *res_name_only = GB_strpartdup(name_start, ldot ? ldot-1 : terminal-1);
            if (res_suffix)    *res_suffix    = ldot ? GB_strpartdup(ldot+1, terminal-1) : NULL;
        }
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

#define TEST_EXPECT_IS_CANONICAL(file)                  \
    do {                                                \
        char *dup = strdup(file);                       \
        TEST_EXPECT_EQUAL(GB_canonical_path(dup), dup); \
        free(dup);                                      \
    } while(0)

#define TEST_EXPECT_CANONICAL_TO(not_cano,cano)                         \
    do {                                                                \
        char *arb_not_cano = strdup(GB_concat_path(arbhome, not_cano)); \
        char *arb_cano     = strdup(GB_concat_path(arbhome, cano));     \
        TEST_EXPECT_EQUAL(GB_canonical_path(arb_not_cano), arb_cano);   \
        free(arb_cano);                                                 \
        free(arb_not_cano);                                             \
    } while (0)

static arb_test::match_expectation path_splits_into(const char *path, const char *Edir, const char *Enameext, const char *Ename, const char *Eext) {
    using namespace arb_test;
    expectation_group expected;

    char *Sdir,*Snameext,*Sname,*Sext;
    GB_split_full_path(path, &Sdir, &Snameext, &Sname, &Sext);

    expected.add(that(Sdir).is_equal_to(Edir));
    expected.add(that(Snameext).is_equal_to(Enameext));
    expected.add(that(Sname).is_equal_to(Ename));
    expected.add(that(Sext).is_equal_to(Eext));

    free(Sdir);
    free(Snameext);
    free(Sname);
    free(Sext);

    return all().ofgroup(expected);
}

#define TEST_EXPECT_PATH_SPLITS_INTO(path,dir,nameext,name,ext)         TEST_EXPECTATION(path_splits_into(path,dir,nameext,name,ext))
#define TEST_EXPECT_PATH_SPLITS_INTO__BROKEN(path,dir,nameext,name,ext) TEST_EXPECTATION__BROKEN(path_splits_into(path,dir,nameext,name,ext))

static arb_test::match_expectation path_splits_reversible(const char *path) {
    using namespace arb_test;
    expectation_group expected;

    char *Sdir,*Snameext,*Sname,*Sext;
    GB_split_full_path(path, &Sdir, &Snameext, &Sname, &Sext);

    expected.add(that(GB_append_suffix(Sname, Sext)).is_equal_to(Snameext)); // GB_append_suffix should reverse name.ext-split
    expected.add(that(GB_concat_path(Sdir, Snameext)).is_equal_to(path));    // GB_concat_path should reverse dir/file-split

    free(Sdir);
    free(Snameext);
    free(Sname);
    free(Sext);

    return all().ofgroup(expected);
}

#define TEST_SPLIT_REVERSIBILITY(path)         TEST_EXPECTATION(path_splits_reversible(path))
#define TEST_SPLIT_REVERSIBILITY__BROKEN(path) TEST_EXPECTATION__BROKEN(path_splits_reversible(path))

void TEST_paths() {
    // test GB_concat_path
    TEST_EXPECT_EQUAL(GB_concat_path("a", NULL), "a");
    TEST_EXPECT_EQUAL(GB_concat_path(NULL, "b"), "b");
    TEST_EXPECT_EQUAL(GB_concat_path("a", "b"), "a/b");

    TEST_EXPECT_EQUAL(GB_concat_path("/", "test.fig"), "/test.fig");

    // test GB_split_full_path
    TEST_EXPECT_PATH_SPLITS_INTO("dir/sub/.ext",              "dir/sub",   ".ext",            "",            "ext");
    TEST_EXPECT_PATH_SPLITS_INTO("/root/sub/file.notext.ext", "/root/sub", "file.notext.ext", "file.notext", "ext");

    TEST_EXPECT_PATH_SPLITS_INTO("./file.ext", ".", "file.ext", "file", "ext");
    TEST_EXPECT_PATH_SPLITS_INTO("/file",      "/", "file",     "file", NULL);
    TEST_EXPECT_PATH_SPLITS_INTO(".",          ".", NULL,       NULL,   NULL);

    // test reversibility of GB_split_full_path and GB_concat_path/GB_append_suffix
    {
        const char *prefix[] = {
            "",
            "dir/",
            "dir/sub/",
            "/dir/",
            "/dir/sub/",
            "/",
            "./",
            "../",
        };

        for (size_t d = 0; d<ARRAY_ELEMS(prefix); ++d) {
            TEST_ANNOTATE(GBS_global_string("prefix='%s'", prefix[d]));

            TEST_SPLIT_REVERSIBILITY(GBS_global_string("%sfile.ext", prefix[d]));
            TEST_SPLIT_REVERSIBILITY(GBS_global_string("%sfile", prefix[d]));
            TEST_SPLIT_REVERSIBILITY(GBS_global_string("%s.ext", prefix[d]));
            if (prefix[d][0]) { // empty string "" reverts to NULL
                TEST_SPLIT_REVERSIBILITY(prefix[d]);
            }
        }
    }

    // GB_canonical_path basics
    TEST_EXPECT_CONTAINS(GB_canonical_path("./bla"), "UNIT_TESTER/run/bla");
    TEST_EXPECT_CONTAINS(GB_canonical_path("bla"),   "UNIT_TESTER/run/bla");

    {
        char        *arbhome    = strdup(GB_getenvARBHOME());
        const char*  nosuchfile = "nosuchfile";
        const char*  somefile   = "arb_README.txt";

        char *somefile_in_arbhome   = strdup(GB_concat_path(arbhome, somefile));
        char *nosuchfile_in_arbhome = strdup(GB_concat_path(arbhome, nosuchfile));
        char *nosuchpath_in_arbhome = strdup(GB_concat_path(arbhome, "nosuchpath"));
        char *somepath_in_arbhome   = strdup(GB_concat_path(arbhome, "lib"));
        char *file_in_nosuchpath    = strdup(GB_concat_path(nosuchpath_in_arbhome, "whatever"));

        TEST_REJECT(GB_is_directory(nosuchpath_in_arbhome));

        // test GB_get_full_path
        TEST_EXPECT_IS_CANONICAL(somefile_in_arbhome);
        TEST_EXPECT_IS_CANONICAL(nosuchpath_in_arbhome);
        TEST_EXPECT_IS_CANONICAL(file_in_nosuchpath);

        TEST_EXPECT_IS_CANONICAL("/sbin"); // existing (most likely)
        TEST_EXPECT_IS_CANONICAL("/tmp/arbtest.fig");
        TEST_EXPECT_IS_CANONICAL("/arbtest.fig"); // not existing (most likely)

        TEST_EXPECT_CANONICAL_TO("./PARSIMONY/./../ARBDB/./arbdb.h",     "ARBDB/arbdb.h"); // test parent-path
        TEST_EXPECT_CANONICAL_TO("INCLUDE/arbdb.h",                   "ARBDB/arbdb.h"); // test symbolic link to file
        TEST_EXPECT_CANONICAL_TO("NAMES_COM/AISC/aisc.pa",            "AISC_COM/AISC/aisc.pa"); // test symbolic link to directory
        TEST_EXPECT_CANONICAL_TO("./NAMES_COM/AISC/..",               "AISC_COM");              // test parent-path through links

        TEST_EXPECT_CANONICAL_TO("./PARSIMONY/./../ARBDB/../nosuchpath", "nosuchpath"); // nosuchpath does not exist, but involved parent dirs do
        // test resolving of non-existent parent dirs:
        TEST_EXPECT_CANONICAL_TO("./PARSIMONY/./../nosuchpath/../ARBDB", "ARBDB");
        TEST_EXPECT_CANONICAL_TO("./nosuchpath/./../ARBDB", "ARBDB");

        // test GB_unfold_path
        TEST_EXPECT_EQUAL(GB_unfold_path("ARBHOME", somefile), somefile_in_arbhome);
        TEST_EXPECT_EQUAL(GB_unfold_path("ARBHOME", nosuchfile), nosuchfile_in_arbhome);

        char *inhome = strdup(GB_unfold_path("HOME", "whatever"));
        TEST_EXPECT_EQUAL(inhome, GB_canonical_path("~/whatever"));
        free(inhome);

        // test GB_unfold_in_directory
        TEST_EXPECT_EQUAL(GB_unfold_in_directory(arbhome, somefile), somefile_in_arbhome);
        TEST_EXPECT_EQUAL(GB_unfold_in_directory(nosuchpath_in_arbhome, somefile_in_arbhome), somefile_in_arbhome);
        TEST_EXPECT_EQUAL(GB_unfold_in_directory(arbhome, nosuchfile), nosuchfile_in_arbhome);
        TEST_EXPECT_EQUAL(GB_unfold_in_directory(nosuchpath_in_arbhome, "whatever"), file_in_nosuchpath);
        TEST_EXPECT_EQUAL(GB_unfold_in_directory(somepath_in_arbhome, "../nosuchfile"), nosuchfile_in_arbhome);

        // test unfolding absolute paths (HOME is ignored)
        TEST_EXPECT_EQUAL(GB_unfold_path("HOME", arbhome), arbhome);
        TEST_EXPECT_EQUAL(GB_unfold_path("HOME", somefile_in_arbhome), somefile_in_arbhome);
        TEST_EXPECT_EQUAL(GB_unfold_path("HOME", nosuchfile_in_arbhome), nosuchfile_in_arbhome);

        // test GB_path_in_ARBHOME
        TEST_EXPECT_EQUAL(GB_path_in_ARBHOME(somefile), somefile_in_arbhome);
        TEST_EXPECT_EQUAL(GB_path_in_ARBHOME(nosuchfile), nosuchfile_in_arbhome);

        free(file_in_nosuchpath);
        free(somepath_in_arbhome);
        free(nosuchpath_in_arbhome);
        free(nosuchfile_in_arbhome);
        free(somefile_in_arbhome);
        free(arbhome);
    }

    TEST_EXPECT_EQUAL(GB_path_in_ARBLIB("help"), GB_path_in_ARBHOME("lib", "help"));

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
        TEST_EXPECT(file.exists());
        TEST_EXPECT(TestFile(file.get_name()).exists()); // removes the file
        TEST_REJECT(file.exists());
    }

    TestFile t("test1");
    {
        GB_shell shell;
        GBDATA *gb_main = GB_open("no.arb", "c");

        GB_remove_on_exit(t.get_name());
        GB_close(gb_main);
    }
    TEST_REJECT(t.exists());
}

void TEST_some_paths() {
    gb_getenv_hook old = GB_install_getenv_hook(arb_test::fakeenv);
    {
        // ../UNIT_TESTER/run/homefake

        TEST_EXPECT_CONTAINS__BROKEN(GB_getenvHOME(), "/UNIT_TESTER/run/homefake"); // GB_getenvHOME() ignores the hook
        // @@@ this is a general problem - unit tested code cannot use GB_getenvHOME() w/o problems

        TEST_EXPECT_CONTAINS(GB_getenvARB_PROP(), "/UNIT_TESTER/run/homefake/.arb_prop");
        TEST_EXPECT_CONTAINS(GB_getenvARBMACRO(), "/lib/macros");

        TEST_EXPECT_CONTAINS(GB_getenvARBCONFIG(),    "/UNIT_TESTER/run/homefake/.arb_prop/cfgSave");
        TEST_EXPECT_CONTAINS(GB_getenvARBMACROHOME(), "/UNIT_TESTER/run/homefake/.arb_prop/macros");  // works in [11068]
    }
    TEST_EXPECT_EQUAL((void*)arb_test::fakeenv, (void*)GB_install_getenv_hook(old));
}

#endif // UNIT_TESTS

