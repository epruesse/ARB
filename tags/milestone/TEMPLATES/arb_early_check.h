// ================================================================ //
//                                                                  //
//   File      : arb_early_check.h                                  //
//   Purpose   : early check for 32/64bit mismatch                  //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in December 2012   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef ARB_EARLY_CHECK_H
#define ARB_EARLY_CHECK_H

#ifndef STATIC_ASSERT_H
#include "static_assert.h"
#endif

// --------------------------------------------------------------------------------
// if any of the assertions below fails, then ...
// 
// - edit ../config.makefile and correct ARB_64 to match your machine
// - make clean
// - make all

#if defined(ARB_64)
STATIC_ASSERT_ANNOTATED(sizeof(void*) == 8, "You cannot compile a 64bit-ARB on a 32bit-machine");
#else // !defined(ARB_64)
STATIC_ASSERT_ANNOTATED(sizeof(void*) == 4, "You cannot compile a 32bit-ARB on a 64bit-machine");
#endif


#else
#error arb_early_check.h included twice
#endif // ARB_EARLY_CHECK_H
