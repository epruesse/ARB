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
#include <PT_com.h>
#include <client.h>
#include <servercntrl.h>

#include "test_unit.h"
#include "UnitTester.hxx"

#include <string>
#include <unistd.h>

#define env_assert(cond) arb_assert(cond)

// #define SIMULATE_ENVSETUP_TIMEOUT // makes tests fail
// #define SIMULATE_ENVSETUP_SLOW_OVERTIME // should not make tests fail

using namespace arb_test;
using namespace std;

enum Mode { UNKNOWN, SETUP, CLEAN };
static const char *mode_command[] = { NULL, "setup", "clean" }; // same order as in enum Mode

typedef GB_ERROR Error;

static const char *upcase(const char *str) {
    char *upstr = strdup(str);
    ARB_strupper(upstr);
    RETURN_LOCAL_ALLOC(upstr);
}

#define WARN_NOT_IMPLEMENTED(fun,mode) TEST_WARNING2("%s(%s) does nothing", #fun, upcase(mode_command[mode]));

static const char *unitTesterDir() {
    RETURN_ONETIME_ALLOC(strdup(GB_concat_full_path(GB_getenvARBHOME(), "UNIT_TESTER")));
}
static const char *flagDir() {
    RETURN_ONETIME_ALLOC(strdup(GB_concat_full_path(unitTesterDir(), FLAGS_DIR)));
}
static const char *runDir() {
    RETURN_ONETIME_ALLOC(strdup(GB_concat_full_path(unitTesterDir(), "run")));
}

// -----------------------
//      PersistantFlag

class PersistantFlag {
    // persistant flags keep their value even if the program is restarted(!)

    string       name;
    mutable bool value; // has to be mutable to support volatile behavior
    bool         is_volatile;

    const char *flagFileName() const {
        return GB_concat_full_path(flagDir(), GBS_global_string("%s." FLAGS_EXT, name.c_str()));
    }

    bool flagFileExists() const { return GB_is_regularfile(flagFileName()); }

    void createFlagFile() const {
        const char *flagfile = flagFileName();
        FILE       *fp       = fopen(flagfile, "w");
        if (!fp) {
            GB_ERROR error = GB_IO_error("creating flag", flagfile);
            StaticCode::errorf(__FILE__, __LINE__, "%s\n", error);
        }
        fclose(fp);
        env_assert(flagFileExists());
    }
    void removeFlagFile() const {
        const char *flagfile = flagFileName();
        StaticCode::printf("flagfile='%s'\n", flagfile);
        if (!flagFileExists()) {
            GBK_dump_backtrace(stderr, "tried to remove non-existing flagfile");
        }
        int res = unlink(flagfile);
        if (res != 0) {
            GB_ERROR error = GB_IO_error("unlinking", flagfile);
            StaticCode::errorf(__FILE__, __LINE__, "%s\n", error);
        }
        env_assert(!flagFileExists());
    }
    void updateFlagFile() const {
        if (value) createFlagFile();
        else removeFlagFile();
    }

public:
    PersistantFlag(const char *name_)
        : name(name_),
          value(flagFileExists()), 
          is_volatile(false)
    {}
    PersistantFlag(const char *name_, bool is_volatile_)
        : name(name_),
          value(flagFileExists()),
          is_volatile(is_volatile_)
    {}
    ~PersistantFlag() {
        if (flagFileExists() != bool(*this)) {
            StaticCode::printf("Mismatch between internal value(=%i) and flagfile='%s'\n", int(value), name.c_str());
        }
        env_assert(flagFileExists() == value);
    }
    operator bool() const {
        if (is_volatile) value = flagFileExists();
        return value;
    }

    PersistantFlag& operator = (bool b) {
        if (b != bool(*this)) {
            value = b;
            updateFlagFile();
        }
        return *this;
    }
};

// --------------------------------------------------------------------------------
// add environment setup/cleanup functions here (and add name to environments[] below)
//
// each environment
// - is setup on client-request (using TEST_SETUP_GLOBAL_ENVIRONMENT)
// - is cleaned up after ALL unit tests were executed (only if it has been setup)
//
// Functions start in same directory where tests starts.
//
// Note: you cannot simply share data between different modes since they are
//       executed in different instances of the executable 'test_environment'!
//
//       Use 'PersistantFlag' to share bools between instances.
// --------------------------------------------------------------------------------

// -----------------
//      ptserver

static void test_ptserver_activate(bool start) {
    const char *server_tag = GBS_ptserver_tag(TEST_SERVER_ID);
    if (start) {
        TEST_ASSERT_NO_ERROR(arb_look_and_start_server(AISC_MAGIC_NUMBER, server_tag, 0));
    }
    else { // stop
        GB_ERROR kill_error = arb_look_and_kill_server(AISC_MAGIC_NUMBER, server_tag);
        if (kill_error) TEST_ASSERT_EQUAL(kill_error, "Server is not running");
    }
}

static Error ptserver(Mode mode) {
    // test-ptserver is restarted and rebuild.
    // This is done only once in the complete test suite.
    // 
    // every unit-test using the test-ptserver should simply call
    // TEST_SETUP_GLOBAL_ENVIRONMENT("ptserver");

    switch (mode) {
        case SETUP: {
            test_ptserver_activate(false);                        // first kill pt-server (otherwise we may test an outdated pt-server)
            TEST_ASSERT_NO_ERROR(GB_system("touch TEST_pt.arb")); // force rebuild
            test_ptserver_activate(true);
            TEST_ASSERT_FILES_EQUAL("TEST_pt.arb.pt.expected", "TEST_pt.arb.pt");
            TEST_ASSERT(GB_time_of_file("TEST_pt.arb.pt") >= GB_time_of_file("TEST_pt.arb"));
            break;
        }
        case CLEAN: {
            test_ptserver_activate(false);
            TEST_ASSERT_ZERO_OR_SHOW_ERRNO(unlink("TEST_pt.arb.pt"));
            break;
        }
        case UNKNOWN:
            env_assert(0);
            break;
    }

    return NULL;
}

// --------------------------------------------------------------------------------

typedef Error (*Environment_cb)(Mode mode);

static Environment_cb  wrapped_cb    = NULL;
static Mode            wrapped_mode  = UNKNOWN;
static const char     *wrapped_error = NULL;
void wrapped() { wrapped_error = wrapped_cb(wrapped_mode); }

static PersistantFlag any_setup(ANY_SETUP, true); // tested and removed by UnitTester.cxx@ANY_SETUP

class FunInfo {
    Environment_cb cb;
    string         name;
    PersistantFlag is_setup;

    Error set_to(Mode mode) {
        StaticCode::printf("[%s environment '%s' START]\n", mode_command[mode], get_name());

        any_setup = true;

        wrapped_cb    = cb;
        wrapped_mode  = mode;
        wrapped_error = NULL;

        chdir(runDir());

        long            duration;
        UnitTestResult  guard_says = execute_guarded(wrapped, &duration, MAX_EXEC_MS_ENV, false);
        const char     *error      = NULL;

        switch (guard_says) {
            case TEST_OK:
                if (wrapped_error) {
                    error = GBS_global_string("returns error: %s", wrapped_error);
                }
                break;

            case TEST_TRAPPED:
            case TEST_INTERRUPTED:  {
                const char *what_happened = guard_says == TEST_TRAPPED
                    ? "trapped"
                    : "has been interrupted (might be a deaklock)";

                error = GBS_global_string("%s%s",
                                          what_happened,
                                          wrapped_error ? GBS_global_string(" (wrapped_error='%s')", wrapped_error) : "");
                break;
            }
        }
        if (error) {
            error = GBS_global_string("%s(%s) %s", name.c_str(), upcase(mode_command[mode]), error);
        }
        else {
            is_setup = (mode == SETUP);
#if defined(SIMULATE_ENVSETUP_TIMEOUT)
            if (mode == SETUP) {
                StaticCode::printf("[simulating a timeout during SETUP]\n");
                sleepms(MAX_EXEC_MS_ENV+MAX_EXEC_MS_SLOW+100); // simulate a timeout
            }
#endif
#if defined(SIMULATE_ENVSETUP_SLOW_OVERTIME)
            if (mode == SETUP) {
                StaticCode::printf("[simulating overtime during SETUP]\n");
                sleepms(MAX_EXEC_MS_SLOW+100); // simulate overtime
            }
#endif
        }

        StaticCode::printf("[%s environment '%s' END]\n", mode_command[mode], get_name());
        return error;
    }

public:

    FunInfo(Environment_cb cb_, const char *name_)
        : cb(cb_),
          name(name_),
          is_setup(name_)
    {}

    Error switch_to(Mode mode) {
        Error error      = NULL;
        bool  want_setup = (mode == SETUP);
        if (is_setup == want_setup) {
            StaticCode::printf("[environment '%s' already was %s]\n", get_name(), mode_command[mode]);
        }
        else {
            error = set_to(mode);
        }
        return error;
    }

    const char *get_name() const { return name.c_str(); }
    bool has_name(const char *oname) const { return name == oname; }
};

#define FUNINFO(fun) FunInfo(fun,#fun)

static FunInfo modules[] = {
    FUNINFO(ptserver),
};

const size_t MODULES = ARRAY_ELEMS(modules);

static FunInfo *find_module(const char *moduleName) {
    for (size_t e = 0; e<MODULES; ++e) {
        if (modules[e].has_name(moduleName)) {
            return &modules[e];
        }
    }
    return NULL;
}

static Error set_all_modules_to(Mode mode) {
    Error error = NULL;
    for (size_t e = 0; !error && e<MODULES; ++e) error = modules[e].switch_to(mode);
    return error;
}

static const char *known_modules(char separator) {
    const char *mods = NULL;
    for (size_t i = 0; i<MODULES; ++i) {
        mods = mods
            ? GBS_global_string("%s%c%s", mods, separator, modules[i].get_name())
            : modules[i].get_name();
    }
    return mods;
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
    if (argc<2 || argc>3) error = "Wrong number of arguments";
    else {
        const char *modearg = argv[1];
        Mode        mode    = UNKNOWN;

        for (size_t i = 0; i<ARRAY_ELEMS(mode_command); ++i) {
            if (mode_command[i] && strcmp(modearg, mode_command[i]) == 0) mode = (Mode)i;
        }

        if (mode == UNKNOWN) {
            error = GBS_global_string("unknown argument '%s' (known modes are: %s)", modearg, known_modes(' '));
        }
        else {
            if (argc == 2) {
                error = set_all_modules_to(mode);
                if (mode == CLEAN) any_setup = false; // reset during final environment cleanup
            }
            else {
                const char *modulearg = argv[2];
                FunInfo    *module    = find_module(modulearg);

                if (!module) {
                    error = GBS_global_string("unknown argument '%s' (known modules are: %s)", modulearg, known_modules(' '));
                }
                else {
                    error = module->switch_to(mode);
                }
            }
        }
    }

    if (error) {
        const char *exename = argv[0];
        StaticCode::printf("%s: %s\n", exename, error);
        StaticCode::printf("Usage: %s [%s] [%s]\n", exename, known_modes('|'), known_modules('|'));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

