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

#ifndef _CPP_CSTDLIB
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

UnitTestResult execute_guarded(UnitTest_function fun, long *duration_usec, long max_allowed_duration_ms);

// ------------------------------
//      execution time limits

const long WHATS_SLOW = 1000;         // ms

const long MAX_EXEC_MS_NORMAL = WHATS_SLOW;          // kill with segfault after time passed
const long MAX_EXEC_MS_SLOW   = WHATS_SLOW * 5;      // 5 seconds
const long MAX_EXEC_MS_ENV    = MAX_EXEC_MS_SLOW *3; // 15 seconds


#else
#error UnitTester.hxx included twice
#endif // UNITTESTER_HXX
