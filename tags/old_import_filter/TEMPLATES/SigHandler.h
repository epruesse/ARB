// =============================================================== //
//                                                                 //
//   File      : SigHandler.h                                      //
//   Purpose   : declare function type for signal handlers         //
//               and wrappers for install/uninstall                //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef SIGHANDLER_H
#define SIGHANDLER_H

#if defined(LINUX)
# include <signal.h>
# ifdef SIG_PF
#  define SigHandler SIG_PF
# else
#  define SigHandler __sighandler_t
# endif
#else
typedef void (*SigHandler)(int);
#endif

#ifndef _GLIBCXX_CSTDIO
#include <cstdio>
#endif
#ifndef _GLIBCXX_CERRNO
#include <cerrno>
#endif

#ifndef ARB_ASSERT_H
#error missing include to arb_assert.h
#endif

inline bool is_default_or_ignore_sighandler(SigHandler sh) {
    return sh == SIG_DFL || sh == SIG_IGN;
}

#ifdef DEBUG

// #define TRACE_SIGNAL_HANDLERS

inline SigHandler install_SigHandler(int sig, SigHandler handler, const char *context, const char *signame) {
    arb_assert(handler != SIG_ERR);
    SigHandler old_handler = signal(sig, handler);
#if defined(TRACE_SIGNAL_HANDLERS)
    fprintf(stderr, "> sighandler[%s] changed (%p->%p)\n", signame, old_handler, handler);
    fflush(stderr);
#endif // TRACE_SIGNAL_HANDLERS
    if (old_handler == SIG_ERR) {
        fprintf(stderr, "%s: failed to install %s handler (Reason: %s)\n",
                context, signame, strerror(errno));
        fflush(stderr);
        arb_assert(0);
    }
    return old_handler;
}

inline void uninstall_SigHandler(int sig, SigHandler handler, SigHandler old_handler, const char *context, const char *signame) {
    if (old_handler != SIG_ERR) {                   // do not try to uninstall if installation failed
        SigHandler uninstalled_handler = install_SigHandler(sig, old_handler, context, signame);

        if (uninstalled_handler != SIG_IGN) {
            // if signal occurred, handler might have been reset to SIG_IGN
            // (this behavior is implementation defined)
            arb_assert(uninstalled_handler == handler);
        }
    }
}

#define INSTALL_SIGHANDLER(sig, handler, context)                install_SigHandler(sig, handler, context, #sig)
#define UNINSTALL_SIGHANDLER(sig, handler, old_handler, context) uninstall_SigHandler(sig, handler, old_handler, context, #sig)

#else

inline SigHandler install_SigHandler(int sig, SigHandler handler) {
    return signal(sig, handler);
}
inline void uninstall_SigHandler(int sig, SigHandler IF_ASSERTION_USED(handler), SigHandler old_handler) {
    if (old_handler != SIG_ERR) {
        ASSERT_RESULT(SigHandler, handler, signal(sig, old_handler));
    }
}

#define INSTALL_SIGHANDLER(sig, handler, context)                install_SigHandler(sig, handler)
#define UNINSTALL_SIGHANDLER(sig, handler, old_handler, context) uninstall_SigHandler(sig, handler, old_handler)

#endif

#else
#error SigHandler.h included twice
#endif // SIGHANDLER_H
