// =============================================================== //
//                                                                 //
//   File      : arb_pathlen.h                                     //
//   Purpose   : stop PATH_MAX and accomplices driving me crazy    //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2013   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef ARB_PATHLEN_H
#define ARB_PATHLEN_H

#ifndef STATIC_ASSERT_H
#include <static_assert.h>
#endif
#ifndef _GLIBCXX_CLIMITS
#include <climits>
#endif
#ifndef _GLIBCXX_CSTDIO
#include <cstdio>
#endif

#define ARB_PATH_LOWER_LIMIT 256
#define ARB_PATH_UPPER_LIMIT 10000 // ought to be enough for anybody ;-)

#ifdef PATH_MAX
#define ARB_PATH_MAX PATH_MAX
#else
#define ARB_PATH_MAX 4096 // use a reasonable amout if PATH_MAX is undefined (i.e. if there is NO limit) 
#endif

#ifndef ARB_PATH_MAX
#error Failed to detect useable allowed size for filenames (ARB_PATH_MAX)
#endif
#ifndef FILENAME_MAX
#error FILENAME_MAX is undefined
#endif

STATIC_ASSERT(FILENAME_MAX>=ARB_PATH_MAX); // if this fails, either PATH_MAX lies or the reasonable amount set above is too high
STATIC_ASSERT(ARB_PATH_MAX>=ARB_PATH_LOWER_LIMIT);
STATIC_ASSERT(ARB_PATH_UPPER_LIMIT>=ARB_PATH_LOWER_LIMIT);

#if (ARB_PATH_MAX > ARB_PATH_UPPER_LIMIT)
#undef ARB_PATH_MAX
#define ARB_PATH_MAX ARB_PATH_UPPER_LIMIT
#endif

#else
#error arb_pathlen.h included twice
#endif // ARB_PATHLEN_H
