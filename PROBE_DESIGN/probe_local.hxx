// ============================================================ //
//                                                              //
//   File      : probe_local.hxx                                //
//   Purpose   :                                                //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2015   //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef PROBE_LOCAL_HXX
#define PROBE_LOCAL_HXX

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define pd_assert(bed) arb_assert(bed)

#else
#error probe_local.hxx included twice
#endif // PROBE_LOCAL_HXX
