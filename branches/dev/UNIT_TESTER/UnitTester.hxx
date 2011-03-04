// ================================================================ //
//                                                                  //
//   File      : UnitTester.hxx                                     //
//   Purpose   : unit testing - test one unit                       //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in February 2010   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef UNITTESTER_HXX
#define UNITTESTER_HXX

#ifndef _GLIBCXX_CSTDLIB
#include <cstdlib>
#endif

typedef void (*UnitTest_function)();

struct UnitTest_simple {
    UnitTest_function  fun;
    const char        *name;
    const char        *location;
};
enum UnitTestResult {
    TEST_OK,
    TEST_TRAPPED,
    TEST_INTERRUPTED, 
};

struct UnitTester {
    UnitTester(const char *libname, const UnitTest_simple *simple_tests, int warn_level, size_t skippedTests);
};

UnitTestResult execute_guarded(UnitTest_function fun, long *duration_usec, long max_allowed_duration_ms, bool detect_environment_calls);
void sleepms(long ms);

// ------------------------------
//      execution time limits

const long SECONDS = 1000;

const long MAX_EXEC_MS_NORMAL = 3 * SECONDS;        // kill with segfault after time passed
const long MAX_EXEC_MS_SLOW   = 7 * SECONDS;        // same for slow tests
const long MAX_EXEC_MS_ENV    = 15 * SECONDS;       // same for environment setup/cleanup
const long MAX_EXEC_MS_VGSYS  = 21 * SECONDS;       // same for valgrinded system calls (especially pt_server)

const long WARN_SLOW_ABOVE_MS = 1 * SECONDS;        // when too warn about slow test

#define FLAGS_DIR "flags"
#define FLAGS_EXT "flag"

#define ANY_SETUP "any_setup"

#else
#error UnitTester.hxx included twice
#endif // UNITTESTER_HXX
