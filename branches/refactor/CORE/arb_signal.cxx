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

void sigsegv_handler(int sig) {
    // Note: sigsegv_handler is intentionally global, to show it in backtrace!

    if (suppress_sigsegv) {
        siglongjmp(return_after_segv, 667); // suppress SIGSEGV (see below)
    }

    if (dump_backtrace_on_sigsegv) {
        GBK_dump_backtrace(stderr, GBS_global_string("received signal %i", sig));
    }
    fprintf(stderr, "[Terminating with signal %i]\n", sig);
    exit(EXIT_FAILURE);
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

bool GBK_raises_SIGSEGV(void (*cb)(void), bool result_in_valgrind) {
    // test whether 'cb' aborts with SIGSEGV
    // (does nothing and always returns 'result_in_valgrind' if executable is running under valgrind!)

    volatile bool segv_occurred = false;
    if (GBK_running_on_valgrind()) {
        segv_occurred = result_in_valgrind;
    }
    else {
        arb_assert(!suppress_sigsegv);
        SigHandler old_handler = INSTALL_SIGHANDLER(SIGSEGV, sigsegv_handler, "GBK_raises_SIGSEGV");

        suppress_sigsegv = true;

        // ----------------------------------------
        // start of critical section
        // (need volatile for modified local auto variables, see man longjump)
        {
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
    }

    return segv_occurred;
}

