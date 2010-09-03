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

#include <sys/time.h>

#define SIMPLE_ARB_ASSERT
#include <test_unit.h>
#include <arb_backtrace.h>
#define ut_assert(cond) arb_assert(cond)

#include <SigHandler.h>
#include <setjmp.h>
#include <string>

using namespace std;

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

enum UnitTestResult {
    TEST_OK,
    TEST_TRAPPED,
};

static const char *readable_result[] = {
    "OK",
    "TRAPPED", 
};

// --------------------------------------------------------------------------------

static jmp_buf UNITTEST_return_after_segv;
static bool    inside_test = false;

static void UNITTEST_sigsegv_handler(int sig) {
    if (inside_test) {
        if (!arb_test::test_data().assertion_failed) { // not caused by assertion
            BackTraceInfo(0).dump(stderr, "Catched SIGSEGV not caused by assertion");
        }
        siglongjmp(UNITTEST_return_after_segv, 668);                    // suppress SIGSEGV
    }
    fprintf(stderr, "[UnitTester terminating with signal %i]\n", sig);
    exit(EXIT_FAILURE);
}

#define SECOND 1000000

static UnitTestResult execute_guarded(UnitTest_function fun, long *duration_usec) {
    SigHandler old_handler = INSTALL_SIGHANDLER(SIGSEGV, UNITTEST_sigsegv_handler, "execute_guarded");
    
    // ----------------------------------------
    // start of critical section
    // (need volatile for modified local auto variables, see man longjump)
    volatile timeval        t1;
    volatile UnitTestResult result  = TEST_OK;
    volatile int            trapped = sigsetjmp(UNITTEST_return_after_segv, 1);

    if (trapped) {                                  // trapped
        ut_assert(trapped == 668);
        result = TEST_TRAPPED;
    }
    else {                                          // normal execution
        inside_test = true;
        gettimeofday((timeval*)&t1, NULL);

        arb_test::test_data().assertion_failed = false;
        fun();
    }
    // end of critical section
    // ----------------------------------------

    timeval t2;
    gettimeofday(&t2, NULL);
    *duration_usec = (t2.tv_sec - t1.tv_sec) * SECOND + (t2.tv_usec - t1.tv_usec);

    inside_test = false;
    UNINSTALL_SIGHANDLER(SIGSEGV, UNITTEST_sigsegv_handler, old_handler, "execute_guarded");

    return result;
}

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
        passed += perform(c);
    }
    return passed;
}


bool SimpleTester::perform(size_t which) {
    ut_assert(which<count);

    const UnitTest_simple& test = tests[which];
    UnitTest_function      fun  = test.fun;

    long           duration_usec;
    UnitTestResult result           = execute_guarded(fun, &duration_usec);
    double         duration_ms_this = duration_usec/1000.0;

    duration_ms += duration_ms_this;
    trace("* %s = %s (%.1f ms)", test.name, readable_result[result], duration_ms_this);
    if (result != TEST_OK) {
        fprintf(stderr, "%s: Error: %s failed (details above)\n", test.location, test.name);
    }

    if (duration_ms_this>1000) {                    // long test duration
        if (strlen(test.name) <= 10 || memcmp(test.name, "TEST_SLOW_", 10) != 0) {
            fprintf(stderr, "%s: Warning: Name of slow tests shall start with TEST_SLOW_ (it'll be run after other tests)\n",
                    test.location);
        }
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

