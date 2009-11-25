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

#ifndef ARBDBT_H
#include <arbdbt.h>
#endif

GBT_TREE *neighbourjoining(char **names, AP_FLOAT **m, long size, size_t structure_size);

#else
#error neighbourjoin.hxx included twice
#endif // NEIGHBOURJOIN_HXX
