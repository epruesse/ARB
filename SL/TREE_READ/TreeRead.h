// ============================================================ //
//                                                              //
//   File      : TreeRead.h                                     //
//   Purpose   :                                                //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2009   //
//   Institute of Microbiology (Technical University Munich)    //
//   www.arb-home.de                                            //
//                                                              //
// ============================================================ //

#ifndef TREEREAD_H
#define TREEREAD_H

#ifndef ARBDBT_H
#include <arbdbt.h>
#endif

#define DEFAULT_LENGTH        0.1 /* default length of tree-edge w/o given length */
#define DEFAULT_LENGTH_MARKER -1000.0 /* tree-edges w/o length are marked with this value during read and corrected in GBT_scale_tree */

GBT_TREE *GBT_load_tree(const char *path, int structuresize, char **commentPtr, int allow_length_scaling, char **warningPtr);
char     *GBT_newick_comment(const char *comment, GB_BOOL escape);
void      GBT_scale_tree(GBT_TREE *tree, double length_scale, double bootstrap_scale);

#else
#error TreeRead.h included twice
#endif // TREEREAD_H
