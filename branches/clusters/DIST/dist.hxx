// =========================================================== //
//                                                             //
//   File      : dist.hxx                                      //
//   Purpose   :                                               //
//                                                             //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef DIST_HXX
#define DIST_HXX

#ifndef ARBDBT_H
#include <arbdbt.h>
#endif

#define di_assert(cond) arb_assert(cond)

extern GBDATA *GLOBAL_gb_main;

#else
#error dist.hxx included twice
#endif // DIST_HXX
