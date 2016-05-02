// ================================================================ //
//                                                                  //
//   File      : arb_signal.cxx                                     //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2010   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include <SuppressOutput.h>

#include <arb_signal.h>
#include <arb_msg.h>

#include <arb_assert.h>
#include <SigHandler.h>
#include <setjmp.h>
#include <valgrind.h>
#include <arb_backtrace.h>

// AISC_MKPT_PROMOTE:#ifndef ARB_CORE_H
// AISC_MKPT_PROMOTE:#include <arb_core.h>
// AISC_MKPT_PROMOTE:#endif

// -----------------------
//      catch SIGSEGV

static bool    dump_backtrace_on_sigsegv = false;
static bool    suppress_sigsegv          = false;
static jmp_buf return_after_segv;

__ATTR__NORETURN static void sigsegv_handler(int sig) {
    // Note: sigsegv_handler is intentionally global, to show it in backtrace!

    if (suppress_sigsegv) {
        siglongjmp(return_after_segv, 667); // suppress SIGSEGV (see below)
    }

    if (dump_backtrace_on_sigsegv) {
        GBK_dump_backtrace(stderr, GBS_global_string("received signal %i", sig));
    }
    fprintf(stderr, "[Terminating with signal %i]\n", sig);
    exit(ARB_CRASH_CODE(sig));
}

void GBK_install_SIGSEGV_handler(bool dump_backtrace) {
    dump_backtrace_on_sigsegv = dump_backtrace;
    SigHandler old_handler = INSTALL_SIGHANDLER(SIGSEGV, sigsegv_handler, "GBK_install_SIGSEGV_handler");
    if (old_handler != sigsegv_handler && old_handler != SIG_ERR && old_handler != SIG_DFL) {
#if defined(DEBUG)
        fprintf(stderr, "GBK_install_SIGSEGV_handler: Did not install SIGSEGV handler (there's already another one installed)\n");
#endif // DEBUG
        UNINSTALL_SIGHANDLER(SIGSEGV, sigsegv_handler, old_handler, "GBK_install_SIGSEGV_handler"); // restore existing signal handler (AISC servers install their own)
    }
}

GB_ERROR GBK_test_address(long *address, long key) {
    arb_assert(!suppress_sigsegv);
    suppress_sigsegv = true;

    // ----------------------------------------
    // start of critical section
    // (need volatile for modified local auto variables, see man longjump / NOTES)
    volatile bool segv_occurred = false;
    volatile long found_key;
    volatile int  trapped       = sigsetjmp(return_after_segv, 1);

    if (!trapped) {                                 // normal execution
        found_key = *address;
    }
    else {
        segv_occurred = true;
    }
    // end of critical section
    // ----------------------------------------

    suppress_sigsegv = false;
    arb_assert(implicated(trapped, trapped == 667));

    GB_ERROR error = NULL;
    if (segv_occurred) {
        error = GBS_global_string("ARBDB memory manager error: Cannot access address %p", address);
    }
    else if (key && found_key != key) {
        error = GBS_global_string("ARBDB memory manager error: object at address %p has wrong type (found: 0x%lx, expected: 0x%lx)",
                                  address, found_key, key);
    }

    if (error) {
        fputs(error, stderr);
        fputc('\n', stderr);
    }

    return error;
}

bool GBK_running_on_valgrind() {
    return RUNNING_ON_VALGRIND>0;
}

bool GBK_raises_SIGSEGV(void (*cb)(void)) {
    // test whether 'cb' aborts with SIGSEGV
    // (Note: never true under valgrind!)

    volatile bool segv_occurred = false;
    arb_assert(!suppress_sigsegv);
    SigHandler old_handler = INSTALL_SIGHANDLER(SIGSEGV, sigsegv_handler, "GBK_raises_SIGSEGV");

    suppress_sigsegv = true;

    // ----------------------------------------
    // start of critical section
    // (need volatile for modified local auto variables, see man longjump)
    {
        // cppcheck-suppress variableScope (false positive)
        volatile int trapped;
        {
            volatile SuppressOutput toConsole;           // comment-out this line to show console output of 'cb'

            int old_suppression       = BackTraceInfo::suppress();
            BackTraceInfo::suppress() = true;

            trapped = sigsetjmp(return_after_segv, 1);

            if (!trapped) {                     // normal execution
                cb();
            }
            else {
                segv_occurred = true;
            }
            BackTraceInfo::suppress() = old_suppression;
        }
        suppress_sigsegv = false;
        arb_assert(implicated(trapped, trapped == 667));
    }
    // end of critical section
    // ----------------------------------------

    UNINSTALL_SIGHANDLER(SIGSEGV, sigsegv_handler, old_handler, "GBK_raises_SIGSEGV");

    return segv_occurred;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

// Tests here contain special failure cases concerning C++-exceptions.
// They exist for tuning/debugging the unit-tester.
// Most of them should normally be disabled.

#if 0
// this (wrong) example-test triggers ../UNIT_TESTER/UnitTester.cxx@TEST_THREW
__ATTR__NORETURN void TEST_exception() {
    throw(666); // bad! test code should not throw out an exception
}
#endif

void TEST_catched_exception() {
    try {
        throw(0x815); // throwing is not bad in general, as long as exceptions do not leave the test-code
        TEST_EXPECT(0);
    }
    catch (...) {
    }
}

#if 0
// this (wrong) example-test triggers ../UNIT_TESTER/UnitTester.cxx@terminate_called
struct throw_on_destroy {
    int j;
    throw_on_destroy(int i) : j(i) {}
    ~throw_on_destroy() { if (j == 666) throw(667); } // bad! if another exception was throw, this will call std::terminate()
    void do_nothing() {}
};
void TEST_throw_during_throw() {
    throw_on_destroy tod(666);
    tod.do_nothing();
    throw(668);
}
#endif

#if 0
void TEST_modify_std_terminate() {
    std::set_terminate(TEST_catched_exception); // modify to whatever
}
#endif

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
