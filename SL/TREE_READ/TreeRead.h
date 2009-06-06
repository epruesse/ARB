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
#define DEFAULT_LENGTH_MARKER -1000.0 /* tree-edges w/o length are marked with this value during read and corrected in TREE_scale */

GBT_TREE *TREE_load(const char *path, int structuresize, char **commentPtr, int allow_length_scaling, char **warningPtr);
void      TREE_scale(GBT_TREE *tree, double length_scale, double bootstrap_scale);
char     *TREE_log_action_to_tree_comment(const char *comment, const char *action);

#else
#error TreeRead.h included twice
#endif // TREEREAD_H
