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

#define DEFAULT_BRANCH_LENGTH_MARKER -1000.0 // tree-edges w/o length are marked with this value during read and corrected in TREE_scale

inline bool is_marked_as_default_len(GBT_LEN len) { return len <= DEFAULT_BRANCH_LENGTH_MARKER; }

GBT_TREE *TREE_load(const char *path, const TreeNodeFactory& nodeMaker, char **commentPtr, bool allow_length_scaling, char **warningPtr);
void TREE_scale(GBT_TREE *tree, double length_scale, double bootstrap_scale);

#else
#error TreeRead.h included twice
#endif // TREEREAD_H
