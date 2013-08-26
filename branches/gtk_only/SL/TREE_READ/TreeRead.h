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

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif

#define TREE_DEFLEN        0.1     // default length of tree-edge w/o given length
#define TREE_DEFLEN_MARKER -1000.0 // tree-edges w/o length are marked with this value during read and corrected in TREE_scale

inline bool is_marked_as_default_len(GBT_LEN len) { return len <= TREE_DEFLEN_MARKER; }

GBT_TREE *TREE_load(const char *path, int structuresize, char **commentPtr, bool allow_length_scaling, char **warningPtr);
void TREE_scale(GBT_TREE *tree, double length_scale, double bootstrap_scale);

#else
#error TreeRead.h included twice
#endif // TREEREAD_H
