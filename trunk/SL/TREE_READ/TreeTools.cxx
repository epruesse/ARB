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

void GBT_scale_tree(GBT_TREE *tree, double length_scale, double bootstrap_scale) {
    if (tree->leftson) {
        if (tree->leftlen <= DEFAULT_LENGTH_MARKER) tree->leftlen  = DEFAULT_LENGTH;
        else                                        tree->leftlen *= length_scale;
        
        GBT_scale_tree(tree->leftson, length_scale, bootstrap_scale);
    }
    if (tree->rightson) {
        if (tree->rightlen <= DEFAULT_LENGTH_MARKER) tree->rightlen  = DEFAULT_LENGTH; 
        else                                         tree->rightlen *= length_scale;
        
        GBT_scale_tree(tree->rightson, length_scale, bootstrap_scale);
    }

    if (tree->remark_branch) {
        const char *end          = 0;
        double      bootstrap    = strtod(tree->remark_branch, (char**)&end);
        bool        is_bootstrap = end[0] == '%' && end[1] == 0;

        freeset(tree->remark_branch, 0);

        if (is_bootstrap) {
            bootstrap = bootstrap*bootstrap_scale+0.5;
            tree->remark_branch  = GBS_global_string_copy("%i%%", (int)bootstrap);
        }
    }
}

char *GBT_newick_comment(const char *comment, GB_BOOL escape) {
    /* escape or unescape a newick tree comment
     * (']' is not allowed there)
     */

    char *result = 0;
    if (escape) {
        result = GBS_string_eval(comment, "\\\\=\\\\\\\\:[=\\\\{:]=\\\\}", NULL); // \\\\ has to be first!
    }
    else {
        result = GBS_string_eval(comment, "\\\\}=]:\\\\{=[:\\\\\\\\=\\\\", NULL); // \\\\ has to be last!
    }
    return result;
}

