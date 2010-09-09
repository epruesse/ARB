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

#define ARRAY_ELEMS(array)            (sizeof(array)/sizeof(array[0]))
#define TERMINATED_ARRAY_ELEMS(array) (ARRAY_ELEMS(array)-1)

#else
#error arb_defs.h included twice
#endif // ARB_DEFS_H
