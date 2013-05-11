// =========================================================== //
//                                                             //
//   File      : arb_sleep.h                                   //
//   Purpose   :                                               //
//                                                             //
//   Coded by Ralf Westram (coder@reallysoft.de) in May 2013   //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef ARB_SLEEP_H
#define ARB_SLEEP_H

#ifndef _UNISTD_H
#include <unistd.h>
#endif

enum TimeUnit { USEC = 1, MS = 1000, SEC = 1000*MS };

inline void GB_sleep(int amount, TimeUnit tu) {
    usleep(amount*tu);
}

#else
#error arb_sleep.h included twice
#endif // ARB_SLEEP_H
