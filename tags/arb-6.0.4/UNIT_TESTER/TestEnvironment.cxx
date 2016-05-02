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

#include <arb_simple_assert.h>

#include <arb_defs.h>
#include <arb_str.h>
#include <arb_file.h>
#include <arb_diff.h>

#include <arbdb.h>
#include <PT_com.h>
#include <client.h>
#include <servercntrl.h>

#include "test_unit.h"
#include "UnitTester.hxx"

#include <string>
#include <set>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/time.h>

#include "../SOURCE_TOOLS/arb_main.h"

#define env_assert(cond) arb_assert(cond)

// #define SIMULATE_ENVSETUP_TIMEOUT // makes tests fail
// #define SIMULATE_ENVSETUP_SLOW_OVERTIME // should not make tests fail

using namespace arb_test;
using namespace std;

enum Mode { UNKNOWN, SETUP, CLEAN };
static const char *mode_command[] = { NULL, "setup", "clean" }; // same order as in enum Mode
static Mode other_mode[] = { UNKNOWN, CLEAN, SETUP }; 

typedef SmartCharPtr Error;

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
static char *mutexDir(const char *name) {
    return strdup(GB_concat_full_path(flagDir(), GBS_global_string("mutex_%s", name)));
}

#if defined(DEBUG)
// #define DUMP_MUTEX_ACCESS
#endif


class Mutex : virtual Noncopyable {
    string name;
    string mutexdir;

    bool mutexdir_exists() const { return GB_is_directory(mutexdir.c_str()); }

    static const char *now() {
        static char buf[100];
        timeval t;
        gettimeofday(&t, NULL);
        sprintf(buf, "%li.%li", t.tv_sec, t.tv_usec);
        return buf;
    }

    static set<string> known_mutexes;

public:
    Mutex(const char *mutex_name)
        : name(mutex_name)
    {
        mutexdir = mutexDir(mutex_name);

        bool gotMutex = false;
        int  maxwait  = MAX_EXEC_MS_SLOW-5; // seconds
        int  wait     = 0;

        arb_assert(maxwait>0);

        test_data().entered_mutex_loop = true; // avoid race-condition
        while (!gotMutex) {
            if (mutexdir_exists()) {
                if (wait>maxwait && mutexdir_exists()) {
                    GBK_terminatef("[%s] Failed to get mutex for more than %i seconds", now(), maxwait);
                }
                printf("[%s] mutex '%s' exists.. sleeping\n", now(), name.c_str());
                sleepms(1000);
                wait++;
            }
            else {
                int res = mkdir(mutexdir.c_str(), S_IRWXU);
                if (res == 0) {
#if defined(DUMP_MUTEX_ACCESS)
                    printf("[%s] allocated mutex '%s'\n", now(), name.c_str());
#endif
                    gotMutex = true;
                }
                else {
                    wait = 0; // reset timeout
                    printf("[%s] lost race for mutex '%s'\n", now(), name.c_str());
                }
            }
        }
        known_mutexes.insert(name);
    }
    ~Mutex() {
        if (!mutexdir_exists()) {
            printf("[%s] Strange - mutex '%s' has vanished\n", now(), name.c_str());
        }
        else {
            if (rmdir(mutexdir.c_str()) != 0) {
                const char *error = GB_IO_error("remove", mutexdir.c_str());
                GBK_terminatef("[%s] Failed to release mutex dir (%s)", now(), error);
            }
            else {
#if defined(DUMP_MUTEX_ACCESS)
                printf("[%s] released mutex '%s'\n", now(), mutexdir.c_str());
#endif
            }
        }
        known_mutexes.erase(name);
    }

    static bool owned(const char *mutex_name) {
        return known_mutexes.find(mutex_name) != known_mutexes.end();
    }

    static void own_or_terminate(const char *mutex_name) {
        if (!Mutex::owned(mutex_name)) {
            GBK_terminatef("Expected to own mutex '%s'", mutex_name);
        }
    }
};

set<string> Mutex::known_mutexes;

// -----------------------
//      PersistantFlag

static const char *FLAG_MUTEX = "flag_access";

class PersistantFlag {
    // persistant flags keep their value even if the program is restarted(!)

    string       name;
    mutable bool value; // has to be mutable to support volatile behavior
    bool         is_volatile;

    const char *flagFileName() const {
        return GB_concat_full_path(flagDir(), GBS_global_string("%s." FLAGS_EXT, name.c_str()));
    }

    bool flagFileExists() const {
        Mutex::own_or_terminate(FLAG_MUTEX);
        return GB_is_regularfile(flagFileName());
    }

    void createFlagFile() const {
        Mutex::own_or_terminate(FLAG_MUTEX);
        const char *flagfile = flagFileName();
        FILE       *fp       = fopen(flagfile, "w");
        if (!fp) {
            GB_ERROR error = GB_IO_error("creating flag", flagfile);
            HERE.errorf(true, "%s\n", error);
        }
        else {
            fclose(fp);
        }
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
            HERE.errorf(true, "%s\n", error);
        }
        env_assert(!flagFileExists());
    }
    void updateFlagFile() const {
        if (value) createFlagFile();
        else removeFlagFile();
    }

public:
    PersistantFlag(const string& name_)
        : name(name_),
          value(flagFileExists()), 
          is_volatile(false)
    {}
    PersistantFlag(const string& name_, bool is_volatile_)
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
            StaticCode::printf("Changing flag '%s' to %i\n", name.c_str(), int(b));
            value = b;
            updateFlagFile();
        }
        return *this;
    }
};

class LazyPersistantFlag {
    // wrapper for PersistantFlag - instanciates on r/w access

    SmartPtr<PersistantFlag> flag;

    string name;
    bool   will_be_volatile;

    void instanciate() {
        flag = new PersistantFlag(name, will_be_volatile);
    }
    void instanciate_on_demand() const {
        if (flag.isNull()) {
            const_cast<LazyPersistantFlag*>(this)->instanciate();
        }
    }

public:
    LazyPersistantFlag(const char *name_) : name(strdup(name_)), will_be_volatile(false) {}
    LazyPersistantFlag(const char *name_, bool is_volatile_) : name(strdup(name_)), will_be_volatile(is_volatile_) {}

    operator bool() const {
        instanciate_on_demand();
        return *flag;
    }

    LazyPersistantFlag& operator = (bool b) {
        instanciate_on_demand();
        *flag = b;
        return *this;
    }

    void get_lazy_again() { flag.SetNull(); }
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

static void test_ptserver_activate(bool start, int serverid) {
    const char *server_tag = GBS_ptserver_tag(serverid);
    if (start) {
        TEST_EXPECT_NO_ERROR(arb_look_and_start_server(AISC_MAGIC_NUMBER, server_tag));
    }
    else { // stop
        GB_ERROR kill_error = arb_look_and_kill_server(AISC_MAGIC_NUMBER, server_tag);
        if (kill_error) TEST_EXPECT_EQUAL(kill_error, "Server is not running");
    }
}

// to activate INDEX_HEXDUMP uncomment TEST_AUTO_UPDATE and INDEX_HEXDUMP once (before modifying index),
// then comment out TEST_AUTO_UPDATE again
// 
// #define TEST_AUTO_UPDATE
// #define INDEX_HEXDUMP

static Error ptserver(Mode mode) {
    // test-ptserver is restarted and rebuild.
    // This is done only once in the complete test suite.
    // 
    // every unit-test using the test-ptserver should simply call
    // TEST_SETUP_GLOBAL_ENVIRONMENT("ptserver");

    Error error;
    switch (mode) {
        case SETUP: {
            test_ptserver_activate(false, TEST_SERVER_ID);                      // first kill pt-server (otherwise we may test an outdated pt-server)
            TEST_EXPECT_NO_ERROR(GBK_system("cp TEST_pt_src.arb TEST_pt.arb")); // force rebuild
            test_ptserver_activate(true, TEST_SERVER_ID);
#if defined(INDEX_HEXDUMP)
            TEST_DUMP_FILE("TEST_pt.arb.pt", "TEST_pt.arb.pt.dump");
#endif

#if defined(TEST_AUTO_UPDATE)
            TEST_COPY_FILE("TEST_pt.arb.pt", "TEST_pt.arb.pt.expected");
# if defined(INDEX_HEXDUMP)
            TEST_COPY_FILE("TEST_pt.arb.pt.dump", "TEST_pt.arb.pt.dump.expected");
# endif
#else // !defined(TEST_AUTO_UPDATE)
# if defined(INDEX_HEXDUMP)
            TEST_EXPECT_TEXTFILES_EQUAL("TEST_pt.arb.pt.dump.expected", "TEST_pt.arb.pt.dump");
# endif
            TEST_EXPECT_FILES_EQUAL("TEST_pt.arb.pt.expected", "TEST_pt.arb.pt");
#endif
            TEST_EXPECT_LESS_EQUAL(GB_time_of_file("TEST_pt.arb"), GB_time_of_file("TEST_pt.arb.pt"));
            break;
        }
        case CLEAN: {
            test_ptserver_activate(false, TEST_SERVER_ID);
            TEST_EXPECT_ZERO_OR_SHOW_ERRNO(unlink("TEST_pt.arb.pt"));
#if defined(INDEX_HEXDUMP)
            TEST_EXPECT_ZERO_OR_SHOW_ERRNO(unlink("TEST_pt.arb.pt.dump"));
#endif
            break;
        }
        case UNKNOWN:
            env_assert(0);
            break;
    }

    return error;
}

static Error ptserver_gene(Mode mode) {
    // test-gene-ptserver is restarted and rebuild.
    // This is done only once in the complete test suite.
    // 
    // every unit-test using the test-gene-ptserver should simply call
    // TEST_SETUP_GLOBAL_ENVIRONMENT("ptserver_gene");

    Error error;
    switch (mode) {
        case SETUP: {
            test_ptserver_activate(false, TEST_GENESERVER_ID);                     // first kill pt-server (otherwise we may test an outdated pt-server)
            TEST_EXPECT_NO_ERROR(GBK_system("arb_gene_probe TEST_gpt_src.arb TEST_gpt.arb")); // prepare gene-ptserver-db (forcing rebuild)

            // GBK_terminatef("test-crash of test_environment"); 

#if defined(TEST_AUTO_UPDATE)
            TEST_COPY_FILE("TEST_gpt.arb", "TEST_gpt.arb.expected");
#else // !defined(TEST_AUTO_UPDATE)
            TEST_EXPECT_FILES_EQUAL("TEST_gpt.arb.expected", "TEST_gpt.arb");
#endif

            test_ptserver_activate(true, TEST_GENESERVER_ID);
#if defined(INDEX_HEXDUMP)
            TEST_DUMP_FILE("TEST_gpt.arb.pt", "TEST_gpt.arb.pt.dump");
#endif

#if defined(TEST_AUTO_UPDATE)
            TEST_COPY_FILE("TEST_gpt.arb.pt", "TEST_gpt.arb.pt.expected");
# if defined(INDEX_HEXDUMP)
            TEST_COPY_FILE("TEST_gpt.arb.pt.dump", "TEST_gpt.arb.pt.dump.expected");
# endif
#else // !defined(TEST_AUTO_UPDATE)
# if defined(INDEX_HEXDUMP)
            TEST_EXPECT_TEXTFILES_EQUAL("TEST_gpt.arb.pt.dump.expected", "TEST_gpt.arb.pt.dump");
# endif
            TEST_EXPECT_FILES_EQUAL("TEST_gpt.arb.pt.expected", "TEST_gpt.arb.pt");
#endif

            TEST_EXPECT_LESS_EQUAL(GB_time_of_file("TEST_gpt.arb"), GB_time_of_file("TEST_gpt.arb.pt"));
            break;
        }
        case CLEAN: {
            test_ptserver_activate(false, TEST_GENESERVER_ID);
            TEST_EXPECT_ZERO_OR_SHOW_ERRNO(unlink("TEST_gpt.arb.pt"));
#if defined(INDEX_HEXDUMP)
            TEST_EXPECT_ZERO_OR_SHOW_ERRNO(unlink("TEST_gpt.arb.pt.dump"));
#endif
            break;
        }
        case UNKNOWN:
            env_assert(0);
            break;
    }

    return error;
}

#undef TEST_AUTO_UPDATE

// --------------------------------------------------------------------------------

typedef Error (*Environment_cb)(Mode mode);

static Environment_cb wrapped_cb   = NULL;
static Mode           wrapped_mode = UNKNOWN;
static SmartCharPtr   wrapped_error;

static void wrapped() { wrapped_error = wrapped_cb(wrapped_mode); }

class FunInfo {
    Environment_cb     cb;
    string             name;
    LazyPersistantFlag is_setup;
    LazyPersistantFlag changing; // some process is currently setting up/cleaning the environment
    LazyPersistantFlag failed;   // some process failed to setup the environment

    void all_get_lazy_again() {
        changing.get_lazy_again();
        is_setup.get_lazy_again();
        failed.get_lazy_again();
    }

    Error set_to(Mode mode) {
        StaticCode::printf("[%s environment '%s' START]\n", mode_command[mode], get_name());

        wrapped_cb   = cb;
        wrapped_mode = mode;
        wrapped_error.SetNull();

        Error error;
        if (chdir(runDir()) != 0) {
            error = strdup(GB_IO_error("changing dir to", runDir()));
        }
        else {
            long           duration;
            UnitTestResult guard_says = execute_guarded(wrapped, &duration, MAX_EXEC_MS_ENV, false);

            switch (guard_says) {
                case TEST_OK:
                    if (wrapped_error.isSet()) {
                        error = GBS_global_string_copy("returns error: %s", &*wrapped_error);
                    }
                    break;

                case TEST_TRAPPED:
                case TEST_INTERRUPTED:  {
                    const char *what_happened = guard_says == TEST_TRAPPED
                        ? "trapped"
                        : "has been interrupted (might be a deaklock)";

                    error = GBS_global_string_copy("%s%s",
                                                   what_happened,
                                                   wrapped_error.isSet() ? GBS_global_string(" (wrapped_error='%s')", &*wrapped_error) : "");
                    break;
                }
                case TEST_THREW:
                    error = strdup("has thrown an exception");
                    break;

                case TEST_INVALID:
                    error = strdup("is invalid");
                    break;

                case TEST_FAILED_POSTCONDITION:
                case TEST_UNKNOWN_RESULT:
                    env_assert(0); // should not happen here
                    break;
            }
            if (error.isSet()) {
                error = GBS_global_string_copy("%s(%s) %s", name.c_str(), upcase(mode_command[mode]), &*error);
            }
            else {
                Mutex m(FLAG_MUTEX);

                env_assert(changing);
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
                changing = false;
                all_get_lazy_again();
            }
        }

        StaticCode::printf("[%s environment '%s' END]\n", mode_command[mode], get_name());
        return error;
    }

public:

    FunInfo(Environment_cb cb_, const char *name_)
        : cb(cb_),
          name(name_),
          is_setup(name_),
          changing(GBS_global_string("changing_%s", name_)),
          failed(GBS_global_string("failed_%s", name_))
    {}

    Error switch_to(Mode mode) { // @@@ need to return allocated msg (it gets overwritten)
        Error error;
        bool  want_setup = (mode == SETUP);

        bool perform_change  = false;
        bool wait_for_change = false;
        {
            Mutex m(FLAG_MUTEX);

            if (changing) { // somebody is changing the environment state
                if (is_setup == want_setup) { // wanted state was reached, but somebody is altering it
                    error = GBS_global_string_copy("[environment '%s' was %s, but somebody is changing it to %s]",
                                                   get_name(), mode_command[mode], mode_command[other_mode[mode]]);
                }
                else { // the somebody is changing to my wanted state
                    wait_for_change = true;
                }
            }
            else {
                if (is_setup == want_setup) {
                    StaticCode::printf("[environment '%s' already was %s]\n", get_name(), mode_command[mode]);
                }
                else {
                    changing = perform_change = true;
                }
            }

            all_get_lazy_again();
        }

        env_assert(!(perform_change && wait_for_change));

        if (perform_change) {
            error = set_to(mode);
            if (error.isSet()) {
                {
                    Mutex m(FLAG_MUTEX);
                    failed = true;
                    all_get_lazy_again();
                }
                Error clean_error = set_to(CLEAN);
                if (clean_error.isSet()) {
                    StaticCode::printf("[environment '%s' failed to reach '%s' after failure (Reason: %s)]\n",
                                       get_name(), mode_command[CLEAN], &*clean_error);
                }
            }
        }

        if (wait_for_change) {
            bool reached = false;
            while (!reached && !error.isSet()) {
                StaticCode::printf("[waiting until environment '%s' reaches '%s']\n", get_name(), mode_command[mode]);
                sleepms(1000);
                {
                    Mutex m(FLAG_MUTEX);
                    if (!changing && is_setup == want_setup) reached = true;
                    if (failed) {
                        error = GBS_global_string_copy("[environment '%s' failed to reach '%s' (in another process)]", get_name(), mode_command[mode]);
                    }
                    all_get_lazy_again();
                }
            }
            if (reached) StaticCode::printf("[environment '%s' has reached '%s']\n", get_name(), mode_command[mode]);
        }

        return error;
    }

    const char *get_name() const { return name.c_str(); }
    bool has_name(const char *oname) const { return name == oname; }
};

#define FUNINFO(fun) FunInfo(fun,#fun)

static FunInfo modules[] = { // ExistingEnvironments
    FUNINFO(ptserver),
    FUNINFO(ptserver_gene),
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
    Error error;
    for (size_t e = 0; error.isNull() && e<MODULES; ++e) {
        error = modules[e].switch_to(mode);
    }
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
    start_of_main();

    Error error;
    bool  showUsage = true;

    if (argc<2 || argc>3) error = strdup("Wrong number of arguments");
    else {
        const char *modearg = argv[1];
        Mode        mode    = UNKNOWN;

        for (size_t i = 0; i<ARRAY_ELEMS(mode_command); ++i) {
            if (mode_command[i] && strcmp(modearg, mode_command[i]) == 0) mode = (Mode)i;
        }

        if (mode == UNKNOWN) {
            error = GBS_global_string_copy("unknown argument '%s' (known modes are: %s)", modearg, known_modes(' '));
        }
        else {
            if (argc == 2) {
                error     = set_all_modules_to(mode);
                showUsage = false;
            }
            else {
                const char *modulearg = argv[2];
                FunInfo    *module    = find_module(modulearg);

                if (!module) {
                    error = GBS_global_string_copy("unknown argument '%s' (known modules are: %s)", modulearg, known_modules(' '));
                }
                else {
                    error     = module->switch_to(mode);
                    showUsage = false;
                }
            }
        }
    }

    if (error.isSet()) {
        const char *exename = argv[0];
        StaticCode::printf("Error in %s: %s\n", exename, &*error);
        if (showUsage) StaticCode::printf("Usage: %s [%s] [%s]\n", exename, known_modes('|'), known_modules('|'));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

