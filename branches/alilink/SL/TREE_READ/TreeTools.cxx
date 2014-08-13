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

void TREE_scale(GBT_TREE *tree, double length_scale, double bootstrap_scale) { // @@@ make a member of GBT_TREE?
    if (tree->leftson) {
        if (is_marked_as_default_len(tree->leftlen)) tree->leftlen  = DEFAULT_BRANCH_LENGTH;
        else                                         tree->leftlen *= length_scale;

        TREE_scale(tree->leftson, length_scale, bootstrap_scale);
    }
    if (tree->rightson) {
        if (is_marked_as_default_len(tree->rightlen)) tree->rightlen  = DEFAULT_BRANCH_LENGTH;
        else                                          tree->rightlen *= length_scale;

        TREE_scale(tree->rightson, length_scale, bootstrap_scale);
    }

    double bootstrap;
    switch (tree->parse_bootstrap(bootstrap)) {
        case REMARK_BOOTSTRAP: tree->set_bootstrap(bootstrap*bootstrap_scale); break;
        case REMARK_OTHER:     tree->remove_remark(); break; // remove non-bootstrap comments
        case REMARK_NONE:      break;
    }
}


