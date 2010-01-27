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

#include <ctime>


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


static char *dated_info(const char *info) {
    char *dated_info = 0;
    time_t      date;
    if (time(&date) != -1) {
        char *dstr = ctime(&date);
        char *nl   = strchr(dstr, '\n');

        if (nl) nl[0] = 0; // cut off LF

        dated_info = GBS_global_string_copy("%s: %s", dstr, info);
    }
    else {
        dated_info = strdup(info);
    }
    return dated_info;
}

char *TREE_log_action_to_tree_comment(const char *comment, const char *action) {
    size_t clen = comment ? strlen(comment) : 0;
    size_t alen = strlen(action);

    GBS_strstruct *new_comment = GBS_stropen(clen+alen+100);

    if (comment) {
        GBS_strcat(new_comment, comment);
        if (comment[clen-1] != '\n') GBS_chrcat(new_comment, '\n');
    }

    char *dated_action = dated_info(action);
    GBS_strcat(new_comment, dated_action);
    GBS_chrcat(new_comment, '\n');

    free(dated_action);

    return GBS_strclose(new_comment);
}
