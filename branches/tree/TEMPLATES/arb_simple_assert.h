// ============================================================ //
//                                                              //
//   File      : arb_simple_assert.h                            //
//   Purpose   : simple assert (independent from libCORE)       //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2013   //
//   Institute of Microbiology (Technical University Munich)    //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef ARB_SIMPLE_ASSERT_H
#define ARB_SIMPLE_ASSERT_H

#if !defined(SIMPLE_ARB_ASSERT)
#error you have to define SIMPLE_ARB_ASSERT in Makefile and pass it to compiler and makedepend
// please DO NOT define SIMPLE_ARB_ASSERT in code
#endif

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#else
#error included arb_simple_assert.h too late (arb_assert.h already has been included)
#endif

#else
#error arb_simple_assert.h included twice
#endif // ARB_SIMPLE_ASSERT_H
