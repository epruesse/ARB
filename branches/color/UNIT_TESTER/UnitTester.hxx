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
#ifndef _GLIBCXX_CSTDIO
#include <cstdio>
#endif

typedef void (*UnitTest_function)();

enum UnitTestResult {
    TEST_OK,
    TEST_TRAPPED,
    TEST_FAILED_POSTCONDITION,
    TEST_INTERRUPTED, 
    TEST_THREW,
    TEST_INVALID,
    TEST_UNKNOWN_RESULT,
};

struct UnitTest_simple {
    UnitTest_function  fun;
    const char        *name;
    const char        *location;

    void print_error(FILE *out, UnitTestResult result) const;
};

struct UnitTester {
    UnitTester(const char *libname, const UnitTest_simple *simple_tests, int warn_level, size_t skippedTests, const UnitTest_simple *postcond) __attribute__((noreturn));
};

UnitTestResult execute_guarded(UnitTest_function fun, long *duration_usec, long max_allowed_duration_ms, bool detect_environment_calls);
void sleepms(long ms);

// ------------------------------
//      execution time limits

const long SECONDS = 1000;
const long MINUTES = 60*SECONDS;

#if defined(DEVEL_RALF)

const long MAX_EXEC_MS_NORMAL = 12 * SECONDS;       // kill with segfault after time passed
const long MAX_EXEC_MS_SLOW   = 60 * SECONDS;       // same for slow tests
const long MAX_EXEC_MS_ENV    = 80 * SECONDS;       // same for environment setup/cleanup
const long MAX_EXEC_MS_VGSYS  = 140 * SECONDS;      // same for valgrinded system calls (especially pt_server)

const long WARN_SLOW_ABOVE_MS = 1 * SECONDS;        // when too warn about slow test

#else // !defined(DEVEL_RALF)

// these are temporary test-timings to avoid test timeouts on jenkins build server

const long MAX_EXEC_MS_NORMAL = 30 * MINUTES;       // kill with segfault after time passed
const long MAX_EXEC_MS_SLOW   = 45 * MINUTES;       // same for slow tests
const long MAX_EXEC_MS_ENV    = 45 * MINUTES;       // same for environment setup/cleanup
const long MAX_EXEC_MS_VGSYS  = 45 * MINUTES;       // same for valgrinded system calls (especially pt_server)
const long WARN_SLOW_ABOVE_MS = 30 * MINUTES;       // when too warn about slow test

#endif

#define FLAGS_DIR "flags"
#define FLAGS_EXT "flag"

#else
#error UnitTester.hxx included twice
#endif // UNITTESTER_HXX
