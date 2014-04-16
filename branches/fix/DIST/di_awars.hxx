// =============================================================== //
//                                                                 //
//   File      : di_awars.hxx                                      //
//   Purpose   : Global awars for ARB_DIST                         //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2009   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef DI_AWARS_HXX
#define DI_AWARS_HXX

#define AWAR_DIST_PREFIX             "dist/"
#define AWAR_DIST_FILTER_PREFIX      AWAR_DIST_PREFIX "filter/"
#define AWAR_DIST_COLUMN_STAT_PREFIX AWAR_DIST_PREFIX "colstat/"
#define AWAR_DIST_TREE_PREFIX        AWAR_DIST_PREFIX "tree/"

#define AWAR_DIST_WHICH_SPECIES AWAR_DIST_PREFIX "which_species"
#define AWAR_DIST_ALIGNMENT     AWAR_DIST_PREFIX "alignment"

#define AWAR_DIST_FILTER_ALIGNMENT AWAR_DIST_FILTER_PREFIX "alignment"
#define AWAR_DIST_FILTER_NAME      AWAR_DIST_FILTER_PREFIX "name"
#define AWAR_DIST_FILTER_FILTER    AWAR_DIST_FILTER_PREFIX "filter"
#define AWAR_DIST_FILTER_SIMPLIFY  AWAR_DIST_FILTER_PREFIX "simplify"

#define AWAR_DIST_TREE_CURR_NAME "tmp/" AWAR_DIST_TREE_PREFIX "curr_tree_name"

#define AWAR_DIST_MATRIX_DNA_BASE    AWAR_DIST_PREFIX "dna/matrix"
#define AWAR_DIST_MATRIX_DNA_ENABLED "tmp/" AWAR_DIST_MATRIX_DNA_BASE "/enable"

#else
#error di_awars.hxx included twice
#endif // DI_AWARS_HXX
