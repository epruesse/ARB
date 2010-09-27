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

#else
#error global.h included twice
#endif // GLOBAL_H
