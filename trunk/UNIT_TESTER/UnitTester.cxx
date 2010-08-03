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

#include <SigHandler.h>
#include <setjmp.h>

#include <cstdarg>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <sys/time.h>

#define SIMPLE_ARB_ASSERT
#include <arb_assert.h>
#define ut_assert(cond) arb_assert(cond)

using namespace std;

// --------------------------------------------------------------------------------

#define TRACE
#if defined(TRACE)
static void trace(const char *format, ...) {
    va_list parg;

    fflush(stdout);
    fflush(stderr);
    
    va_start(parg, format);
    vfprintf(stderr, format, parg);
    va_end(parg);
    fflush(stderr);
}
#else
inline void trace(const char *, ...) {
    fflush(stdout);
    fflush(stderr);
}
#endif // TRACE

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

static jmp_buf UT_return_after_segv;
static bool    inside_test = false;

static void UT_sigsegv_handler(int sig) {
    if (inside_test) {
        longjmp(UT_return_after_segv, 668);             // suppress SIGSEGV
    }
    fprintf(stderr, "[UnitTester terminating with signal %i]\n", sig);
    exit(EXIT_FAILURE);
}

#define SECOND 1000000

static UnitTestResult execute_guarded(UnitTest_function fun, long *duration_usec) {
    SigHandler     old_handler = signal(SIGSEGV, UT_sigsegv_handler);
    UnitTestResult result      = TEST_OK;
    timeval        t1, t2;
    int            trapped     = setjmp(UT_return_after_segv);

    if (trapped) {                                  // trapped
        ut_assert(trapped == 668);
        result = TEST_TRAPPED;
    }
    else {                                          // normal execution
        inside_test = true;
        gettimeofday(&t1, NULL);
        fun();
    }

    gettimeofday(&t2, NULL);
    *duration_usec = (t2.tv_sec - t1.tv_sec) * SECOND + (t2.tv_usec - t1.tv_usec);
    
    inside_test = false;
    signal(SIGSEGV, old_handler);

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
        : tests(tests_)
    {
        for (count = 0; tests[count].fun; ++count) {}
    }

    size_t perform_all();
    size_t get_test_count() const { return count; }

    double overall_duration_ms() const { return duration_ms; }
};


size_t SimpleTester::perform_all() {
    // returns number of passed tests

    trace("performing %zu simple tests..\n", count);
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
    trace("* %s = %s (%.1f ms)\n", test.name, readable_result[result], duration_ms_this);
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

UnitTester::UnitTester(const char *libname, const UnitTest_simple *simple_tests) {
    size_t tests  = 0;
    size_t passed = 0;
    double duration_ms;

    {
        SimpleTester simple_tester(simple_tests);

        tests             = simple_tester.get_test_count();
        if (tests) passed = simple_tester.perform_all();
        duration_ms       = simple_tester.overall_duration_ms();
    }

    if (tests>0) {
        if (passed == tests) {
            trace("all unit tests for '%s' passed OK (%.0f ms)\n\n", libname, duration_ms);
        }
        else {
            trace("unit tests for '%s': %zu OK / %zu FAILED (%.0f ms)\n\n", libname, passed, tests-passed, duration_ms);
        }
    }
    else {
        trace("No tests defined for '%s'\n\n", libname);
    }

    exit(tests == passed ? EXIT_SUCCESS : EXIT_FAILURE);
}

