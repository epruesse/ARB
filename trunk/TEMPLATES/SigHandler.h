// =============================================================== //
//                                                                 //
//   File      : SigHandler.h                                      //
//   Purpose   : declare function type for signal handlers         //
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

inline bool is_default_or_ignore_sighandler(SigHandler sh) {
    return sh == SIG_DFL || sh == SIG_IGN;
}

#else
#error SigHandler.h included twice
#endif // SIGHANDLER_H
