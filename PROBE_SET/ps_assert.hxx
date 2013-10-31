// ================================================================= //
//                                                                   //
//   File      : ps_assert.hxx                                       //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2012   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef PS_ASSERT_HXX
#define PS_ASSERT_HXX

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

#define ps_assert(cond) arb_assert(cond)
#define CRASH           provoke_core_dump

#else
#error ps_assert.hxx included twice
#endif // PS_ASSERT_HXX
