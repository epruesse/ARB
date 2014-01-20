// =============================================================== //
//                                                                 //
//   File      : NT_tree_cmp.h                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef NT_TREE_CMP_H
#define NT_TREE_CMP_H

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif

enum TreeInfoMode {
    TREE_INFO_COPY,    // overwrites existing info
    TREE_INFO_COMPARE, // compare node info
    TREE_INFO_ADD,     // doesn't overwrite
};

GB_ERROR AWT_move_info(GBDATA *gb_main, const char *tree_source, const char *tree_dest, const char *log_file, TreeInfoMode mode, bool nodes_with_marked_only);

#else
#error NT_tree_cmp.h included twice
#endif // NT_TREE_CMP_H
