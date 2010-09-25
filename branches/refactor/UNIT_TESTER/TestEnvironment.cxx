// ================================================================= //
//                                                                   //
//   File      : TestEnvironment.cxx                                 //
//   Purpose   : Global! setup/cleanup for tests                     //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2010   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#define SIMPLE_ARB_ASSERT

#include <arb_defs.h>
#include <arb_str.h>
#include <arbdb.h>
// #include <smartptr.h>

#include "test_unit.h"
#include "UnitTester.hxx"

using namespace arb_test;

enum Mode { UNKNOWN, SETUP, CLEANUP };
static const char *mode_command[] = { NULL, "setup", "cleanup" }; // same order as in enum Mode

typedef GB_ERROR Error;

static const char *upcase(const char *str) {
    static  SmartMallocPtr(char) dup = strdup(str);
    char   *result                   = &*dup;
    ARB_strupper(result);
    return result; // leak
}

#define WARN_NOT_IMPLEMENTED(fun,mode) TEST_WARNING2("%s(%s) does nothing", #fun, upcase(mode_command[mode]));

// --------------------------------------------------------------------------------
// add environment setup/cleanup functions here (and add name to environments[] below)
//
// each environment
// - is setup before ANY unit test runs
// - is cleaned up after ALL unit tests were executed
// 
// Functions start in same directory where tests starts.

static Error ptserver(Mode mode) {
    // TODO: ensure ptserver is restarted and rebuild (once for complete test suite)

    WARN_NOT_IMPLEMENTED(ptserver, mode);
    return NULL;
}

// --------------------------------------------------------------------------------

typedef Error (*Environment_cb)(Mode mode);

static Environment_cb  wrapped_cb    = NULL;
static Mode            wrapped_mode  = UNKNOWN;
static const char     *wrapped_error = NULL;
void wrapped() { wrapped_error = wrapped_cb(wrapped_mode); }

class FunInfo {
    Environment_cb  cb;
    const char     *name;

    // const char *fake_location() const {
    // return GBS_global_string("%s:%i", __FILE__, __LINE__);
    // }

public:
    FunInfo(Environment_cb cb_, const char *name_) : cb(cb_), name(name_) {}
    Error call(Mode mode) const {
        wrapped_cb    = cb;
        wrapped_mode  = mode;
        wrapped_error = NULL;

        long            duration;
        UnitTestResult  guard_says = execute_guarded(wrapped, &duration, MAX_EXEC_MS_ENV);
        const char     *error      = wrapped_error;

        switch (guard_says) {
            case TEST_OK:
                error = wrapped_error;
                break;
            case TEST_TRAPPED:
            case TEST_INTERRUPTED:  {
                const char *what_happened = guard_says == TEST_TRAPPED
                    ? "trapped"
                    : "has been interrupted (might be a deaklock)";

                error = GBS_global_string("%s(%s) %s%s",
                                          name, upcase(mode_command[mode]),
                                          what_happened,
                                          wrapped_error ? GBS_global_string(" (wrapped_error='%s')", wrapped_error) : "");
            }
        }

        if (error) error = GBS_global_string("%s: (during %s of '%s')", error, mode_command[mode], name);

        return error;
    }
};

#define FUNINFO(fun) FunInfo(fun,#fun)

static FunInfo environments[] = {
    FUNINFO(ptserver),
};

const int ENVIRONMENTS = ARRAY_ELEMS(environments);

static Error set_environments_to(Mode mode) {
    Error error = NULL;
    for (int e = 0; !error && e<ENVIRONMENTS; ++e) error = environments[e].call(mode);
    return error;
}

static const char *known_modes(char separator) {
    const char *modes = NULL;
    for (size_t i = 0; i<ARRAY_ELEMS(mode_command); ++i) {
        if (mode_command[i]) {
            modes = modes
                ? GBS_global_string("%s%c%s", modes, separator, mode_command[i])
                : mode_command[i];
        }
    }
    return modes;
}

int main(int argc, char* argv[]) {
    Error error = NULL;
    if (argc != 2) error = "Not enough arguments";
    else {
        const char *arg  = argv[1];
        Mode        mode = UNKNOWN;

        for (size_t i = 0; i<ARRAY_ELEMS(mode_command); ++i) {
            if (mode_command[i] && strcmp(arg, mode_command[i]) == 0) mode = (Mode)i;
        }

        if (mode == UNKNOWN) {
            error = GBS_global_string("unknown argument '%s' (known modes are: %s)", arg, known_modes(' '));
        }
        else {
            error = set_environments_to(mode);
        }
    }

    if (error) {
        const char *exename = argv[0];
        StaticCode::printf("%s: %s\n", exename, error);
        StaticCode::printf("Usage: %s [%s]\n", exename, known_modes('|'));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

