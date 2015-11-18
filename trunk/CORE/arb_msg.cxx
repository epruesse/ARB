// ================================================================ //
//                                                                  //
//   File      : arb_msg.cxx                                        //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2010   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include <arb_msg_fwd.h>
#include <arb_assert.h>
#include <arb_string.h>
#include <arb_backtrace.h>
#include <smartptr.h>
#include <arb_handlers.h>
#include <arb_defs.h>
#include "arb_strbuf.h"

// AISC_MKPT_PROMOTE:#ifndef _GLIBCXX_CSTDLIB
// AISC_MKPT_PROMOTE:#include <cstdlib>
// AISC_MKPT_PROMOTE:#endif
// AISC_MKPT_PROMOTE:#ifndef ARB_CORE_H
// AISC_MKPT_PROMOTE:#include "arb_core.h"
// AISC_MKPT_PROMOTE:#endif
// AISC_MKPT_PROMOTE:
// AISC_MKPT_PROMOTE:// return error and ensure none is exported 
// AISC_MKPT_PROMOTE:#define RETURN_ERROR(err)  arb_assert(!GB_have_error()); return (err)
// AISC_MKPT_PROMOTE:

#if defined(DEBUG)
#if defined(DEVEL_RALF)
// #define TRACE_BUFFER_USAGE
#endif // DEBUG
#endif // DEVEL_RALF

#define GLOBAL_STRING_BUFFERS 4

static size_t last_global_string_size = 0;
#define GBS_GLOBAL_STRING_SIZE 64000

// --------------------------------------------------------------------------------

#ifdef LINUX
# define HAVE_VSNPRINTF
#endif

#ifdef HAVE_VSNPRINTF
# define PRINT2BUFFER(buffer, bufsize, templat, parg) vsnprintf(buffer, bufsize, templat, parg)
#else
# define PRINT2BUFFER(buffer, bufsize, templat, parg) vsprintf(buffer, templat, parg)
#endif

#define PRINT2BUFFER_CHECKED(printed, buffer, bufsize, templat, parg)   \
    (printed) = PRINT2BUFFER(buffer, bufsize, templat, parg);           \
    if ((printed) < 0 || (size_t)(printed) >= (bufsize)) {              \
        GBK_terminatef("Internal buffer overflow (size=%zu, used=%i)\n", \
                       (bufsize), (printed));                           \
    }

// --------------------------------------------------------------------------------

class GlobalStringBuffers {
    char buffer[GLOBAL_STRING_BUFFERS][GBS_GLOBAL_STRING_SIZE+2]; // several buffers - used alternately
    int  idx;
    char lifetime[GLOBAL_STRING_BUFFERS];
    char nextIdx[GLOBAL_STRING_BUFFERS];

public:
    GlobalStringBuffers()
        : idx(0)
    {
        for (int i = 0; i<GLOBAL_STRING_BUFFERS; ++i) {
            nextIdx[i]  = 0;
            lifetime[i] = 0;
        }
    }

    __ATTR__VFORMAT_MEMBER(1) const char *vstrf(const char *templat, va_list parg, int allow_reuse);
};

static GlobalStringBuffers globBuf;

const char *GlobalStringBuffers::vstrf(const char *templat, va_list parg, int allow_reuse) {
    int my_idx;
    int psize;

    if (nextIdx[0] == 0) { // initialize nextIdx
        for (my_idx = 0; my_idx<GLOBAL_STRING_BUFFERS; my_idx++) {
            nextIdx[my_idx] = (my_idx+1)%GLOBAL_STRING_BUFFERS;
        }
    }

    if (allow_reuse == -1) { // called from GBS_reuse_buffer
        // buffer to reuse is passed in 'templat'

        for (my_idx = 0; my_idx<GLOBAL_STRING_BUFFERS; my_idx++) {
            if (buffer[my_idx] == templat) {
                lifetime[my_idx] = 0;
#if defined(TRACE_BUFFER_USAGE)
                printf("Reusing buffer #%i\n", my_idx);
#endif // TRACE_BUFFER_USAGE
                if (nextIdx[my_idx] == idx) idx = my_idx;
                return 0;
            }
#if defined(TRACE_BUFFER_USAGE)
            else {
                printf("(buffer to reuse is not buffer #%i (%p))\n", my_idx, buffer[my_idx]);
            }
#endif // TRACE_BUFFER_USAGE
        }
        for (my_idx = 0; my_idx<GLOBAL_STRING_BUFFERS; my_idx++) {
            printf("buffer[%i]=%p\n", my_idx, buffer[my_idx]);
        }
        arb_assert(0);       // GBS_reuse_buffer called with illegal buffer
        return 0;
    }

    if (lifetime[idx] == 0) {
        my_idx = idx;
    }
    else {
        for (my_idx = nextIdx[idx]; lifetime[my_idx]>0; my_idx = nextIdx[my_idx]) {
#if defined(TRACE_BUFFER_USAGE)
            printf("decreasing lifetime[%i] (%i->%i)\n", my_idx, lifetime[my_idx], lifetime[my_idx]-1);
#endif // TRACE_BUFFER_USAGE
            lifetime[my_idx]--;
        }
    }

    PRINT2BUFFER_CHECKED(psize, buffer[my_idx], (size_t)GBS_GLOBAL_STRING_SIZE, templat, parg);

#if defined(TRACE_BUFFER_USAGE)
    printf("Printed into global buffer #%i ('%s')\n", my_idx, buffer[my_idx]);
#endif // TRACE_BUFFER_USAGE

    last_global_string_size = psize;

    if (!allow_reuse) {
        idx           = my_idx;
        lifetime[idx] = 1;
    }
#if defined(TRACE_BUFFER_USAGE)
    else {
        printf("Allow reuse of buffer #%i\n", my_idx);
    }
#endif // TRACE_BUFFER_USAGE

    return buffer[my_idx];
}

GlobalStringBuffers *GBS_store_global_buffers() {
    GlobalStringBuffers *stored = new GlobalStringBuffers(globBuf);
    globBuf                     = GlobalStringBuffers();
    return stored;
}

void GBS_restore_global_buffers(GlobalStringBuffers *saved) {
    globBuf = *saved;
    delete saved;
}

const char *GBS_vglobal_string(const char *templat, va_list parg) {
    // goes to header: __ATTR__VFORMAT(1)
    return globBuf.vstrf(templat, parg, 0);
}

char *GBS_vglobal_string_copy(const char *templat, va_list parg) {
    // goes to header: __ATTR__VFORMAT(1)
    const char *gstr = globBuf.vstrf(templat, parg, 1);
    return GB_strduplen(gstr, last_global_string_size);
}

const char *GBS_global_string_to_buffer(char *buffer, size_t bufsize, const char *templat, ...) {
    // goes to header: __ATTR__FORMAT(3)

#if defined(WARN_TODO)
#warning search for '\b(sprintf)\b\s*\(' and replace by GBS_global_string_to_buffer
#endif

    va_list parg;
    int     psize;

    arb_assert(buffer);
    va_start(parg, templat);
    PRINT2BUFFER_CHECKED(psize, buffer, bufsize, templat, parg);
    va_end(parg);

    return buffer;
}

char *GBS_global_string_copy(const char *templat, ...) {
    // goes to header: __ATTR__FORMAT(1)
    va_list parg;
    va_start(parg, templat);
    char *result = GBS_vglobal_string_copy(templat, parg);
    va_end(parg);
    return result;
}

const char *GBS_global_string(const char *templat, ...) {
    // goes to header: __ATTR__FORMAT(1)
    va_list parg;
    va_start(parg, templat);
    const char *result = globBuf.vstrf(templat, parg, 0);
    va_end(parg);
    return result;
}

const char *GBS_static_string(const char *str) {
    return GBS_global_string("%s", str);
}

GB_ERROR GBK_assert_msg(const char *assertion, const char *file, int linenr) {
#define BUFSIZE 1000
    static char *buffer   = 0;
    const char  *result   = 0;
    int          old_size = last_global_string_size;

    if (!buffer) buffer = (char *)malloc(BUFSIZE);
    result              = GBS_global_string_to_buffer(buffer, BUFSIZE, "assertion '%s' failed in %s #%i", assertion, file, linenr);

    last_global_string_size = old_size;

    return result;
#undef BUFSIZE
}

// -------------------------
//      Error "handling"


#if defined(WARN_TODO)
#warning redesign GB_export_error et al
/* To clearly distinguish between the two ways of error handling
 * (which are: return GB_ERROR
 *  and:       export the error)
 *
 * GB_export_error() shall only export, not return the error message.
 * if only used for formatting GBS_global_string shall be used
 * (most cases where GB_export_errorf is used are candidates for this.
 *  GB_export_error was generally misused for this, before
 *  GBS_global_string was added!)
 *
 * GB_export_IO_error() shall not export and be renamed into GB_IO_error()
 *
 * GB_export_error() shall fail if there is already an exported error
 * (maybe always remember a stack trace of last error export (try whether copy of backtrace-array works))
 *
 * use GB_get_error() to import AND clear the error
 */
#endif

static char *GB_error_buffer = 0;

GB_ERROR GB_export_error(const char *error) { // just a temp hack around format-warnings
    return GB_export_errorf("%s", error);
}

GB_ERROR GB_export_errorf(const char *templat, ...) {
    /* goes to header:
     * __ATTR__FORMAT(1)
     * __ATTR__DEPRECATED_LATER("use GB_export_error(GBS_global_string(...))")
     *          because it's misused (where GBS_global_string should be used)
     *          old functionality will remain available via 'GB_export_error(GBS_global_string(...))' 
     */

    char     buffer[GBS_GLOBAL_STRING_SIZE];
    char    *p = buffer;
    va_list  parg;

#if defined(WARN_TODO)
#warning dont prepend ARB ERROR here
#endif

    p += sprintf(buffer, "ARB ERROR: ");
    va_start(parg, templat);

    vsprintf(p, templat, parg);

    freedup(GB_error_buffer, buffer);
    return GB_error_buffer;
}

#if defined(WARN_TODO)
#warning replace GB_export_IO_error() by GB_IO_error() and then export it if really needed 
#endif

GB_ERROR GB_IO_error(const char *action, const char *filename) {
    /*! creates error message from current 'errno'
     * @param action may be NULL (otherwise it should contain sth like "writing" or "deleting")
     * @param filename may be NULL (otherwise it should contain the filename, the IO-Error occurred for)
     * @return error message (in static buffer)
     */

    GB_ERROR io_error;
    if (errno) {
        io_error = strerror(errno);
    }
    else {
        arb_assert(0);           // unhandled error (which is NOT an IO-Error)
        io_error =
            "Some unhandled error occurred, but it was not an IO-Error. "
            "Please send detailed information about how the error occurred to devel@arb-home.de "
            "or ignore this error (if possible).";
    }

    GB_ERROR error;
    if (action) {
        if (filename) error = GBS_global_string("While %s '%s': %s", action, filename, io_error);
        else error          = GBS_global_string("While %s <unknown file>: %s", action, io_error);
    }
    else {
        if (filename) error = GBS_global_string("Concerning '%s': %s", filename, io_error);
        else error          = io_error; 
    }

    return error;
}

GB_ERROR GB_export_IO_error(const char *action, const char *filename) {
    // goes to header: __ATTR__DEPRECATED_TODO("use GB_export_error(GB_IO_error(...))")
    return GB_export_error(GB_IO_error(action, filename));
}

#if defined(WARN_TODO)
#warning reactivate deprecations below
#endif


GB_ERROR GB_print_error() {
    // goes to header: __ATTR__DEPRECATED_TODO("will be removed completely")
    if (GB_error_buffer) {
        fflush(stdout);
        fprintf(stderr, "%s\n", GB_error_buffer);
    }
    return GB_error_buffer;
}

GB_ERROR GB_get_error() {
    // goes to header: __ATTR__DEPRECATED_TODO("consider using either GB_have_error() or GB_await_error()")
    return GB_error_buffer;
}

bool GB_have_error() {
    return GB_error_buffer != 0;
}

GB_ERROR GB_await_error() {
    if (GB_error_buffer) {
        static SmartCharPtr err;
        err             = GB_error_buffer;
        GB_error_buffer = NULL;
        return &*err;
    }
    arb_assert(0);               // please correct error handling

    return "Program logic error: Something went wrong, but reason is unknown";
}

void GB_clear_error() {         // clears the error buffer
    freenull(GB_error_buffer);
}

#if defined(WARN_TODO)
#warning search for 'GBS_global_string.*error' and replace with GB_failedTo_error or GB_append_exportedError
#endif
GB_ERROR GB_failedTo_error(const char *do_something, const char *special, GB_ERROR error) {
    if (error) {
        if (special) {
            error = GBS_global_string("Failed to %s '%s'.\n(Reason: %s)", do_something, special, error);
        }
        else {
            error = GBS_global_string("Failed to %s.\n(Reason: %s)", do_something, error);
        }
    }
    return error;
}

GB_ERROR GB_append_exportedError(GB_ERROR error) {
    // If an error has been exported, it gets appended as reason to given 'error'.
    // If error is NULL, the exported error is returned (if any)
    //
    // This is e.g. useful if you search for SOMETHING in the DB w/o success (i.e. get NULL as result).
    // In that case you can't be sure, whether SOMETHING just does not exist or whether it was not
    // found because some other error occurred.

    if (GB_have_error()) {
        if (error) return GBS_global_string("%s (Reason: %s)", error, GB_await_error());
        return GB_await_error();
    }
    return error;
}

// ---------------------
//      Backtracing

class BackTraceInfo *GBK_get_backtrace(size_t skipFramesAtBottom) { // only used ifdef TRACE_ALLOCS
    return new BackTraceInfo(skipFramesAtBottom);
}
void GBK_dump_former_backtrace(class BackTraceInfo *trace, FILE *out, const char *message) { // only used ifdef TRACE_ALLOCS
    demangle_backtrace(*trace, out, message);
}

void GBK_free_backtrace(class BackTraceInfo *trace) { // only used ifdef TRACE_ALLOCS
    delete trace;
}

void GBK_dump_backtrace(FILE *out, const char *message) {
    demangle_backtrace(BackTraceInfo(1), out ? out : stderr, message);
}

// -------------------------------------------
//      Error/notification functions

void GB_internal_error(const char *message) {
    /* Use GB_internal_error, when something goes badly wrong
     * but you want to give the user a chance to save his database
     *
     * Note: it's NOT recommended to use this function!
     */

    char *full_message = GBS_global_string_copy("Internal ARB Error: %s", message);
    active_arb_handlers->show_error(full_message);
    active_arb_handlers->show_error("ARB is most likely unstable now (due to this error).\n"
                                    "If you've made changes to the database, consider to save it using a different name.\n"
                                    "Try to fix the cause of the error and restart ARB.");

#ifdef ASSERTION_USED
    fputs(full_message, stderr);
    arb_assert(0);               // internal errors shall not happen, go fix it
#else
    GBK_dump_backtrace(stderr, full_message);
#endif

    free(full_message);
}

void GB_internal_errorf(const char *templat, ...) {
    // goes to header: __ATTR__FORMAT(1)
    FORWARD_FORMATTED_NORETURN(GB_internal_error, templat);
}

void GBK_terminate(const char *error) { // goes to header __ATTR__NORETURN
    /* GBK_terminate is the emergency exit!
     * only used if no other way to recover
     */

    fprintf(stderr, "Error: '%s'\n", error);
    fputs("Can't continue - terminating..\n", stderr);
    GBK_dump_backtrace(stderr, "GBK_terminate (reason above) ");

    fflush(stderr);
    ARB_SIGSEGV(0); // GBK_terminate shall not be called, fix reason for call (this will crash in RELEASE version)
    exit(ARB_CRASH_CODE(0)); // should not be reached..just ensure ARB really terminates if somebody changes ARB_SIGSEGV
}

void GBK_terminatef(const char *templat, ...) {
    // goes to header: __ATTR__FORMAT(1) __ATTR__NORETURN
    FORWARD_FORMATTED_NORETURN(GBK_terminate, templat);
}

// AISC_MKPT_PROMOTE:inline void GBK_terminate_on_error(const char *error) { if (error) GBK_terminatef("Fatal error: %s", error); }

void GB_warning(const char *message) {
    /* If program uses GUI, the message is printed via aw_message, otherwise it goes to stdout
     * see also : GB_information
     */
    active_arb_handlers->show_warning(message);
}
void GB_warningf(const char *templat, ...) {
    // goes to header: __ATTR__FORMAT(1)
    FORWARD_FORMATTED(GB_warning, templat);
}

void GB_information(const char *message) {
    /* this message is always printed to stdout (regardless whether program uses GUI or not)
     * (if it is not redirected using ARB_redirect_handlers_to)
     * see also : GB_warning
     */
    active_arb_handlers->show_message(message);
}
void GB_informationf(const char *templat, ...) {
    // goes to header: __ATTR__FORMAT(1)
    FORWARD_FORMATTED(GB_information, templat);
}


#pragma GCC diagnostic ignored "-Wmissing-format-attribute"

void GBS_reuse_buffer(const char *global_buffer) {
    // If you've just shortely used a buffer, you can put it back here
    va_list empty;
    globBuf.vstrf(global_buffer, empty, -1); // omg hax
}

#if defined(WARN_TODO)
#warning search for '\b(system)\b\s*\(' and use GBK_system instead
#endif
GB_ERROR GBK_system(const char *system_command) {
    // goes to header: __ATTR__USERESULT
    fflush(stdout);
    fprintf(stderr, "[Action: '%s']\n", system_command); fflush(stderr);

    int res = system(system_command);

    fflush(stdout);
    fflush(stderr);

    GB_ERROR error = NULL;
    if (res) {
        if (res == -1) {
            error = GB_IO_error("forking", system_command);
            error = GBS_global_string("System call failed (Reason: %s)", error);
        }
        else {
            error = GBS_global_string("System call failed (result=%i)", res);
        }

        error = GBS_global_string("%s\n"
                                  "System call was '%s'%s",
                                  error, system_command,
                                  res == -1 ? "" : "\n(Note: console window may contain additional information)");
    }
    return error;
}

char *GBK_singlequote(const char *arg) {
    /*! Enclose argument in single quotes (like 'arg') for POSIX shell commands.
     */

    if (!arg[0]) return strdup("''");

    GBS_strstruct  out(500);
    const char    *existing_quote = strchr(arg, '\'');

    while (existing_quote) {
        if (existing_quote>arg) {
            out.put('\'');
            out.ncat(arg, existing_quote-arg);
            out.put('\'');
        }
        out.put('\\');
        out.put('\'');
        arg            = existing_quote+1;
        existing_quote = strchr(arg, '\'');
    }

    if (arg[0]) {
        out.put('\'');
        out.cat(arg);
        out.put('\'');
    }
    return out.release();
}

char *GBK_doublequote(const char *arg) {
    /*! Enclose argument in single quotes (like "arg") for POSIX shell commands.
     * Opposed to single quoted strings, shell will interpret double quoted strings.
     */

    const char    *charsToEscape = "\"\\";
    const char    *escape      = arg+strcspn(arg, charsToEscape);
    GBS_strstruct  out(500);

    out.put('"');
    while (escape[0]) {
        out.ncat(arg, escape-arg);
        out.put('\\');
        out.put(escape[0]);
        arg    = escape+1;
        escape = arg+strcspn(arg, charsToEscape);
    }
    out.cat(arg);
    out.put('"');
    return out.release();
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

#include "FileContent.h"
#include <unistd.h>

#define TEST_EXPECT_SQUOTE(plain,squote_expected) do {          \
        char *plain_squoted = GBK_singlequote(plain);           \
        TEST_EXPECT_EQUAL(plain_squoted, squote_expected);      \
        free(plain_squoted);                                    \
    } while(0)

#define TEST_EXPECT_DQUOTE(plain,dquote_expected) do {          \
        char *plain_dquoted = GBK_doublequote(plain);           \
        TEST_EXPECT_EQUAL(plain_dquoted, dquote_expected);      \
        free(plain_dquoted);                                    \
    } while(0)

#define TEST_EXPECT_ECHOED_EQUALS(echoarg,expected_echo) do {                   \
        char *cmd = GBS_global_string_copy("echo %s > %s", echoarg, tmpfile);   \
        TEST_EXPECT_NO_ERROR(GBK_system(cmd));                                  \
        FileContent out(tmpfile);                                               \
        TEST_EXPECT_NO_ERROR(out.has_error());                                  \
        TEST_EXPECT_EQUAL(out.lines()[0], expected_echo);                       \
        free(cmd);                                                              \
    } while(0)

#define TEST_EXPECT_SQUOTE_IDENTITY(plain) do {         \
        char *plain_squoted = GBK_singlequote(plain);   \
        TEST_EXPECT_ECHOED_EQUALS(plain_squoted,plain); \
        free(plain_squoted);                            \
    } while(0)

#define TEST_EXPECT_DQUOTE_IDENTITY(plain) do {         \
        char *plain_dquoted = GBK_doublequote(plain);   \
        TEST_EXPECT_ECHOED_EQUALS(plain_dquoted,plain); \
        free(plain_dquoted);                            \
    } while(0)

void TEST_quoting() {
    const char *tmpfile = "quoted.output";

    struct quoting {
        const char *plain;
        const char *squoted;
        const char *dquoted;
    } args[] = {
        { "",                       "''",                          "\"\"" },  // empty
        { " ",                      "' '",                         "\" \"" }, // a space
        { "unquoted",               "'unquoted'",                  "\"unquoted\"" },
        { "part 'is' squoted",      "'part '\\''is'\\'' squoted'", "\"part 'is' squoted\"" },
        { "part \"is\" dquoted",    "'part \"is\" dquoted'",       "\"part \\\"is\\\" dquoted\"" },
        { "'squoted'",              "\\''squoted'\\'",             "\"'squoted'\"" },
        { "\"dquoted\"",            "'\"dquoted\"'",               "\"\\\"dquoted\\\"\"" },
        { "'",                      "\\'",                         "\"'\"" },  // a single quote
        { "\"",                     "'\"'",                        "\"\\\"\"" },  // a double quote
        { "\\",                     "'\\'",                        "\"\\\\\"" },  // a backslash
        { "'\"'\"",                 "\\''\"'\\''\"'",              "\"'\\\"'\\\"\"" },  // interleaved quotes
        { "`wc -c <min_ascii.arb`", "'`wc -c <min_ascii.arb`'",    "\"`wc -c <min_ascii.arb`\"" },  // interleaved quotes
    };

    for (unsigned a = 0; a<ARRAY_ELEMS(args); ++a) {
        TEST_ANNOTATE(GBS_global_string("a=%i", a));
        const quoting& arg = args[a];

        TEST_EXPECT_SQUOTE(arg.plain, arg.squoted);
        TEST_EXPECT_SQUOTE_IDENTITY(arg.plain);

        TEST_EXPECT_DQUOTE(arg.plain, arg.dquoted);
        if (a != 11) {
            TEST_EXPECT_DQUOTE_IDENTITY(arg.plain);
        }
        else { // backticked wc call
            char *dquoted = GBK_doublequote(arg.plain);
            TEST_EXPECT_ECHOED_EQUALS(dquoted, "16"); // 16 is number of chars in min_ascii.arb
            free(dquoted);
        }
    }

    TEST_EXPECT_EQUAL(unlink(tmpfile), 0);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

