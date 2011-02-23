// ================================================================ //
//                                                                  //
//   File      : UnitTester.cxx                                     //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in February 2010   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "UnitTester.hxx"

#include <cstdarg>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <climits>

#include <sys/time.h>

#define SIMPLE_ARB_ASSERT
#include <test_unit.h>
#include <arb_backtrace.h>
#include <valgrind.h>
#define ut_assert(cond) arb_assert(cond)

#include <SigHandler.h>
#include <setjmp.h>
#include <string>

using namespace std;

// --------------------------------------------------------------------------------

struct Globals {
    bool   inside_test;
    bool   running_on_valgrind;
    char  *runDir;
    pid_t  pid;

    Globals()
        : inside_test(false),
          runDir(NULL),
          pid(getpid())
    {
        running_on_valgrind = (RUNNING_ON_VALGRIND>0);
    }
    ~Globals() {
        free(runDir);
    }

    inline void setup_test_precondition() {
        TEST_ASSERT_ZERO_OR_SHOW_ERRNO(chdir(runDir));
    }
    inline void setup_test_postcondition() {
        TEST_ANNOTATE_ASSERT(NULL);
    }
};

static Globals GLOBAL;

// --------------------------------------------------------------------------------
// #define TRACE_PREFIX "UnitTester:0: "
#define TRACE_PREFIX "UnitTester: "

static void trace(const char *format, ...) {
    va_list parg;

    fflush(stdout);
    fflush(stderr);

    fputs(TRACE_PREFIX, stderr);
    va_start(parg, format);
    vfprintf(stderr, format, parg);
    va_end(parg);
    fputc('\n', stderr);
    fflush(stderr);
}

// --------------------------------------------------------------------------------

static const char *readable_result[] = {
    "OK",
    "TRAPPED", 
};

// --------------------------------------------------------------------------------

static jmp_buf UNITTEST_return_after_segv;

enum TrapCode {
    TRAP_UNEXPECTED = 668, 
    TRAP_SEGV,
    TRAP_INT,
    TRAP_TERM,
};

static void UNITTEST_sigsegv_handler(int sig) {
    if (GLOBAL.inside_test) {
        int trap_code;
        switch (sig) {
            case SIGSEGV:
                trap_code = TRAP_SEGV;
                if (!arb_test::test_data().assertion_failed) { // not caused by assertion
                    BackTraceInfo(0).dump(stderr, "Catched SIGSEGV not caused by assertion");
                }
                break;
                
            case SIGINT:
                trap_code = TRAP_INT;
                BackTraceInfo(0).dump(stderr, "Catched SIGINT (deadlock in test function?)");
                break;

            case SIGTERM:
                trap_code = TRAP_TERM;
                BackTraceInfo(0).dump(stderr, "Catched SIGTERM (deadlock in uninterruptable test function?)");
                break;

            default:
                trap_code = TRAP_UNEXPECTED;
                TEST_ASSERT(0);
                break;
        }
        siglongjmp(UNITTEST_return_after_segv, trap_code); // suppress signal
    }

    const char *signame = NULL;
    switch (sig) {
        case SIGSEGV: signame = "SEGV"; break;
        case SIGINT: signame  = "INT"; break;
        case SIGTERM: signame = "TERM"; break;
    }

    fputs("[UnitTester catched unexpected signal ", stderr);
    if (signame) fputs(signame, stderr); else fprintf(stderr, "%i", sig);
    fputs("]\n", stderr);
    BackTraceInfo(0).dump(stderr, "Unexpected signal (NOT raised in test-code)");
    exit(EXIT_FAILURE);
}

#define SECOND 1000000

UnitTestResult execute_guarded_ClientCode(UnitTest_function fun, long *duration_usec) {
    SigHandler old_int_handler  = INSTALL_SIGHANDLER(SIGINT,  UNITTEST_sigsegv_handler, "execute_guarded");
    SigHandler old_term_handler = INSTALL_SIGHANDLER(SIGTERM, UNITTEST_sigsegv_handler, "execute_guarded");
    SigHandler old_segv_handler = INSTALL_SIGHANDLER(SIGSEGV, UNITTEST_sigsegv_handler, "execute_guarded");

    // ----------------------------------------
    // start of critical section
    // (need volatile for modified local auto variables, see man longjump)
    volatile timeval        t1;
    volatile UnitTestResult result  = TEST_OK;
    volatile int            trapped = sigsetjmp(UNITTEST_return_after_segv, 1);

    if (trapped) {
        switch (trapped) {
            case TRAP_UNEXPECTED: ut_assert(0); // fall-through
            case TRAP_SEGV: result = TEST_TRAPPED; break;

            case TRAP_INT:
            case TRAP_TERM: result = TEST_INTERRUPTED; break;
        }
    }
    else { // normal execution
        GLOBAL.inside_test = true;

        gettimeofday((timeval*)&t1, NULL);

        arb_test::test_data().assertion_failed = false;
        arb_test::test_data().running_test = true;

        // Note: fun() may do several ugly things, e.g.
        // - segfault                           (handled)
        // - never return                       (handled by caller)
        // - exit() or abort()
        // - change working dir                 (should always be reset before i get called)

        fun();
        // sleep(10); // simulate a deadlock
    }
    // end of critical section
    // ----------------------------------------

    timeval t2;
    gettimeofday(&t2, NULL);
    *duration_usec = (t2.tv_sec - t1.tv_sec) * SECOND + (t2.tv_usec - t1.tv_usec);

    GLOBAL.inside_test = false;

    UNINSTALL_SIGHANDLER(SIGSEGV, UNITTEST_sigsegv_handler, old_segv_handler, "execute_guarded");
    UNINSTALL_SIGHANDLER(SIGTERM, UNITTEST_sigsegv_handler, old_term_handler, "execute_guarded");
    UNINSTALL_SIGHANDLER(SIGINT,  UNITTEST_sigsegv_handler, old_int_handler,  "execute_guarded");

    return result;
}

inline bool kill_verbose(pid_t pid, int sig, const char *signame) {
    int result = kill(pid, sig);
    if (result != 0) {
        fprintf(stderr, "Failed to send %s to test (%s)\n", signame, strerror(errno));
        fflush(stderr);
    }
    return result == 0;
}

UnitTestResult execute_guarded(UnitTest_function fun, long *duration_usec, long max_allowed_duration_ms) {
    if (GLOBAL.running_on_valgrind) max_allowed_duration_ms *= 4;
    UnitTestResult result;

    pid_t child_pid = fork();
    if (child_pid) { // parent
        result = execute_guarded_ClientCode(fun, duration_usec);
        if (kill(child_pid, SIGKILL) != 0) {
            fprintf(stderr, "Failed to kill deadlock-guard (%s)\n", strerror(errno)); fflush(stderr);
        }
    }
    else { // child
#if (DEADLOCKGUARD == 1)
        // the following section is completely incompatible with debuggers
        int  seconds = max_allowed_duration_ms/1000;
        long rest_ms = max_allowed_duration_ms - seconds*1000;

        sleep(seconds);
        usleep(rest_ms*1000);

        const long aBIT = 50*1000; // µs
        
        fprintf(stderr,
                "[deadlock-guard woke up after %li ms]\n"
                "[interrupting possibly deadlocked test]\n",
                max_allowed_duration_ms); fflush(stderr);
        kill_verbose(GLOBAL.pid, SIGINT, "SIGINT");
        usleep(aBIT); // give parent a chance to kill me

        fprintf(stderr, "[test still running -> terminate]\n"); fflush(stderr);
        kill_verbose(GLOBAL.pid, SIGTERM, "SIGTERM");
        usleep(aBIT); // give parent a chance to kill me

        fprintf(stderr, "[still running -> kill]\n"); fflush(stderr);
        kill_verbose(GLOBAL.pid, SIGKILL, "SIGKILL"); // commit suicide
        // parent had his chance
        fprintf(stderr, "[still alive after suicide -> perplexed]\n"); fflush(stderr);
#else        
#warning DEADLOCKGUARD has been disabled (not default!)
#endif
        exit(EXIT_FAILURE);
    }

    return result;
}

// --------------------------------------------------------------------------------

// --------------------------------------------------------------------------------

class SimpleTester {
    const UnitTest_simple *tests;
    size_t                 count;
    double                 duration_ms;

    bool perform(size_t which);

public:
    SimpleTester(const UnitTest_simple *tests_)
        : tests(tests_),
          duration_ms(0.0)
    {
        for (count = 0; tests[count].fun; ++count) {}
    }

    size_t perform_all();
    size_t get_test_count() const { return count; }

    double overall_duration_ms() const { return duration_ms; }
};


size_t SimpleTester::perform_all() {
    // returns number of passed tests

    trace("performing %zu simple tests..", count);
    size_t passed = 0;
    for (size_t c = 0; c<count; ++c) {
        GLOBAL.setup_test_precondition();
        passed += perform(c);
        GLOBAL.setup_test_postcondition();
    }
    return passed;
}


bool SimpleTester::perform(size_t which) {
    ut_assert(which<count);

    const UnitTest_simple& test = tests[which];
    UnitTest_function      fun  = test.fun;

    bool           marked_as_slow   = strlen(test.name) >= 10 && memcmp(test.name, "TEST_SLOW_", 10) == 0;
    long           duration_usec;
    const long     abort_after_ms   = marked_as_slow ? MAX_EXEC_MS_SLOW : MAX_EXEC_MS_NORMAL;
    UnitTestResult result           = execute_guarded(fun, &duration_usec, abort_after_ms);
    double         duration_ms_this = duration_usec/1000.0;

    duration_ms += duration_ms_this;
    trace("* %s = %s (%.1f ms)", test.name, readable_result[result], duration_ms_this);

    switch (result) {
        case TEST_OK:
            if (duration_ms_this >= WHATS_SLOW) {                    // long test duration
                if (!marked_as_slow) {
                    if (!GLOBAL.running_on_valgrind) {
                        fprintf(stderr, "%s: Warning: Name of slow tests shall start with TEST_SLOW_ (it'll be run after other tests)\n",
                                test.location);
                    }
                }
            }
            break;

        case TEST_TRAPPED:
            fprintf(stderr, "%s: Error: %s failed (details above)\n", test.location, test.name);
            break;
            
        case TEST_INTERRUPTED:
            fprintf(stderr, "%s: Error: %s has been interrupted (details above)\n", test.location, test.name);
            break;
    }

    return result == TEST_OK;
}


// --------------------------------------------------------------------------------

static char text_buffer[100];

inline const char *as_text(size_t z) { sprintf(text_buffer, "%zu", z); return text_buffer; }
inline const char *as_text(double d) { sprintf(text_buffer, "%.1f", d); return text_buffer; }

inline void appendValue(string& report, const char *tag, const char *value) { report = report+' '+tag+'='+value; }

inline void appendValue(string& report, const char *tag, size_t value) { appendValue(report, tag, as_text(value)); }
inline void appendValue(string& report, const char *tag, double value) { appendValue(report, tag, as_text(value)); }

static const char *generateReport(const char *libname, size_t tests, size_t skipped, size_t passed, double duration_ms, size_t warnings) {
    // generate a report consumed by reporter.pl@parse_log
    
    static string report;
    report.clear();

    size_t tested = tests-skipped;
    size_t failed = tested-passed;

    appendValue(report, "target", libname);
    appendValue(report, "time", duration_ms);
    appendValue(report, "tests", tests);
    if (skipped) appendValue(report, "skipped", skipped);
    if (tests) {
        if (!failed) appendValue(report, "passed", "ALL");
        else         appendValue(report, "passed", passed);
    }
    if (failed) appendValue(report, "failed", failed);
    if (warnings) appendValue(report, "warnings", warnings);

    return report.c_str()+1; // skip leading space
}

UnitTester::UnitTester(const char *libname, const UnitTest_simple *simple_tests, int warn_level, size_t skippedTests) {
    // this is the "main()" of the unit test
    // it is invoked from code generated by sym2testcode.pl@InvokeUnitTester

    TEST_ASSERT_ZERO_OR_SHOW_ERRNO(chdir("run"));
    GLOBAL.runDir = getcwd(0, PATH_MAX);

    size_t tests  = 0;
    size_t passed = 0;

    {
        arb_test::GlobalTestData& global = arb_test::test_data();
    
        global.show_warnings = (warn_level != 0);

        double duration_ms = 0;
        {
            SimpleTester simple_tester(simple_tests);

            tests = simple_tester.get_test_count();
            if (tests) {
                passed      = simple_tester.perform_all();
                duration_ms = simple_tester.overall_duration_ms();
            }
        }

        trace(generateReport(libname,
                             tests+skippedTests, skippedTests, passed,
                             duration_ms, global.warnings));
    }

    arb_test::GlobalTestData::erase_instance();
    exit(tests == passed ? EXIT_SUCCESS : EXIT_FAILURE);
}

