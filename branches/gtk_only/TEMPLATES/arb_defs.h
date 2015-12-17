// ================================================================= //
//                                                                   //
//   File      : arb_defs.h                                          //
//   Purpose   : global defines and mini inlines                     //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2010   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef ARB_DEFS_H
#define ARB_DEFS_H

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

#define ARRAY_ELEMS(array)            (sizeof(array)/sizeof(array[0]))
#define TERMINATED_ARRAY_ELEMS(array) (ARRAY_ELEMS(array)-1)

// Internally ARB shall use position [0..N-1] for calculations.
// Only while showing positions to the user, we use [1..N] (as well in displayed AWARs).
// Please use the following functions to make the conversion explicit:

inline int bio2info(int biopos) { arb_assert(biopos >= 1); return biopos-1; }
inline int info2bio(int infopos) { arb_assert(infopos >= 0); return infopos+1; }

#endif // ARB_DEFS_H
