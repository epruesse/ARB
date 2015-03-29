// =============================================================== //
//                                                                 //
//   File      : neighbourjoin.hxx                                 //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef NEIGHBOURJOIN_HXX
#define NEIGHBOURJOIN_HXX

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif

TreeNode *neighbourjoining(const char *const *names, const AP_smatrix& smatrix, TreeRoot *troot);

#else
#error neighbourjoin.hxx included twice
#endif // NEIGHBOURJOIN_HXX
