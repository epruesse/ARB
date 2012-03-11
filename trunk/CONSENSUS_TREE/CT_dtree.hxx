// ============================================================= //
//                                                               //
//   File      : CT_dtree.hxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef CT_DTREE_HXX
#define CT_DTREE_HXX

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif

void remember_subtrees(const GBT_TREE *tree, int weight);

#else
#error CT_dtree.hxx included twice
#endif // CT_DTREE_HXX
