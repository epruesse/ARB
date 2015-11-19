// ================================================================ //
//                                                                  //
//   File      : defs.h                                             //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2006   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //
#ifndef DEFS_H
#define DEFS_H

#ifndef _GLIBCXX_STRING
#include <string>
#endif

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif


#define gi_assert(cond) arb_assert(cond)

// simple helper functions from ARBDB
const char *GBS_global_string(const char *templat, ...) __attribute__((format(printf, 1, 2)));

#else
#error defs.h included twice
#endif // DEFS_H

