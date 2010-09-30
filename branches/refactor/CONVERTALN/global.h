// =============================================================== //
//                                                                 //
//   File      : global.h                                          //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GLOBAL_H
#define GLOBAL_H

#ifndef _CPP_CSTDIO
#include <cstdio>
#endif
#ifndef PROTOTYPES_H
#include "prototypes.h"
#endif

#define ca_assert(cond) arb_assert(cond)

#define LINENUM         126
#define LONGTEXT        5000
#define TOKENNUM        80
#define GBMAXCHAR       66      /* exclude the keyword(12 chars) */
#define EMBLMAXCHAR     68      /* exclude the keyword(6 chars) */
#define MACKEMAXCHAR    78      /* limit for each line */
#define COMMSKINDENT    2       /* indent for subkey line of RDP defined comment */
#define COMMCNINDENT    6       /* indent for continue line of RDP defined comment */
#define SEPDEFINED      1       /* define a particular char as print line separator */
#define SEPNODEFINED    0       /* no particular line separator is defined, use
                                   general rule to separate lines. */
#define p_nonkey_start  5


inline bool str_equal(const char *s1, const char *s2) { return strcmp(s1, s2) == 0; }
inline bool str_iequal(const char *s1, const char *s2) { return strcasecmp(s1, s2) == 0; }

// --------------------
// Logging

#if defined(DEBUG)
#define CALOG // be more verbose in debugging mode
#endif // DEBUG

#if defined(CALOG)
inline void log_processed(int seqCount) {
    fprintf(stderr, "Total %d sequences have been processed\n", seqCount);
}
#else
#define log_processed(xxx)
#endif // CALOG


#else
#error global.h included twice
#endif // GLOBAL_H
