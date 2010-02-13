// ============================================================ //
//                                                              //
//   File      : TreeTools.cxx                                  //
//   Purpose   :                                                //
//                                                              //
//   Institute of Microbiology (Technical University Munich)    //
//   www.arb-home.de                                            //
//                                                              //
// ============================================================ //

#include "TreeRead.h"

void TREE_scale(GBT_TREE *tree, double length_scale, double bootstrap_scale) {
    if (tree->leftson) {
        if (tree->leftlen <= TREE_DEFLEN_MARKER) tree->leftlen  = TREE_DEFLEN;
        else                                     tree->leftlen *= length_scale;

        TREE_scale(tree->leftson, length_scale, bootstrap_scale);
    }
    if (tree->rightson) {
        if (tree->rightlen <= TREE_DEFLEN_MARKER) tree->rightlen  = TREE_DEFLEN;
        else                                      tree->rightlen *= length_scale;

        TREE_scale(tree->rightson, length_scale, bootstrap_scale);
    }

    if (tree->remark_branch) {
        const char *end          = 0;
        double      bootstrap    = strtod(tree->remark_branch, (char**)&end);
        bool        is_bootstrap = end[0] == '%' && end[1] == 0;

        freenull(tree->remark_branch);

        if (is_bootstrap) {
            bootstrap = bootstrap*bootstrap_scale+0.5;
            tree->remark_branch  = GBS_global_string_copy("%i%%", (int)bootstrap);
        }
    }
}


