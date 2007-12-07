// ================================================================= //
//                                                                   //
//   File      : sec_defs.hxx                                        //
//   Purpose   :                                                     //
//   Time-stamp: <Thu Sep/06/2007 10:59 MET Coder@ReallySoft.de>     //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2007   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef SEC_DEFS_HXX
#define SEC_DEFS_HXX

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

#define sec_assert(cond) arb_assert(cond)

#else
#error sec_defs.hxx included twice
#endif // SEC_DEFS_HXX
