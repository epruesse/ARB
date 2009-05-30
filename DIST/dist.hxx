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

#ifndef AWT_SEQ_DNA_HXX
#include <awt_seq_dna.hxx>
#endif
#ifndef AWT_SEQ_SIMPLE_PRO_HXX
#include <awt_seq_simple_pro.hxx>
#endif

#define di_assert(cond) arb_assert(cond)

extern GBDATA *GLOBAL_gb_main;
GBT_TREE *neighbourjoining(char **names, AP_FLOAT **m, long size, size_t structure_size);

#else
#error dist.hxx included twice
#endif // DIST_HXX
