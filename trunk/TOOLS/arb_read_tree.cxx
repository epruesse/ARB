#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arbdb.h>
#include <arbdbt.h>

void show_error(GBDATA *gb_main) {
    GBT_message(gb_main, GB_get_error());
    GB_print_error();
    GB_clear_error();
}

// add_bootstrap interprets the length of the branches as bootstrap value
// (this is needed by Phylip DNAPARS/PROTPARS with bootstrapping)
//
// 'hundred' specifies which value represents 100%

void add_bootstrap(GBT_TREE *node, double hundred) {
    if (node->is_leaf) {
        if (node->remark_branch) {
            free(node->remark_branch);
            node->remark_branch = 0;
        }
        return;
    }

    node->leftlen  /= hundred;
    node->rightlen /= hundred;

    double left_bs  = node->leftlen  * 100.0 + 0.5;
    double right_bs = node->rightlen * 100.0 + 0.5;

#if defined(DEBUG) && 0
    fprintf(stderr, "node->leftlen  = %f left_bs  = %f\n", node->leftlen, left_bs);
    fprintf(stderr, "node->rightlen = %f right_bs = %f\n", node->rightlen, right_bs);
#endif // DEBUG

    node->leftson->remark_branch  = GBS_global_string_copy("%2.0f%%", left_bs);
    node->rightson->remark_branch = GBS_global_string_copy("%2.0f%%", right_bs);

    node->leftlen  = 0.1; // reset branchlengths
    node->rightlen = 0.1;

    add_bootstrap(node->leftson, hundred);
    add_bootstrap(node->rightson, hundred);
}

void abort_with_usage(GBDATA *gb_main, const char *error) {
    printf("Usage: arb_read_tree [-scale factor] [-consense #ofTrees] tree_name treefile [comment]\n"); 
    if (error) {
        printf("Error: %s\n", error);
        GBT_message(gb_main, GBS_global_string("Error running arb_read_tree (%s)", error));
    }
    exit(-1);
}

int main(int argc,char **argv)
{
    GBDATA *gb_main = GB_open(":","r");
    if (!gb_main){
        printf("arb_read_tree: Error: you have to start an arbdb server first\n");
        return -1;
    }

#define SHIFT_ARGS(off) do { argc -= off; argv += off; } while(0)

    int args = argc-1; argv++; // position onto first argument

    bool   scale        = false;
    double scale_factor = 0.0;

    if (args>0 && strcmp("-scale",argv[0]) == 0) {
        scale = 1;
        if (args<2) abort_with_usage(gb_main, "-scale expects a 2nd argument (scale factor)");
        scale_factor = atof(argv[1]);
        SHIFT_ARGS(2);
    }

    bool consense         = false;
    int  calculated_trees = 0;

    if (args>0 && strcmp("-consense",argv[0]) == 0) {
        consense = true;
        if (args<2) abort_with_usage(gb_main, "-consense expects a 2nd argument (number of trees)");

        calculated_trees = atoi(argv[1]);
        if (calculated_trees < 1) {
            abort_with_usage(gb_main, GBS_global_string("Illegal # of trees (%i) for -consense", calculated_trees));
        }

        SHIFT_ARGS(2);
    }

    if (!args) abort_with_usage(gb_main, "Missing argument 'tree_name'");
    const char *tree_name = argv[0];
    SHIFT_ARGS(1);

    if (!args) abort_with_usage(gb_main, "Missing argument 'treefile'");
    const char *filename = argv[0];
    SHIFT_ARGS(1);

    const char *comment = 0;
    if (args>0) {
        comment = argv[0];
        SHIFT_ARGS(1);
    }

    char *comment_from_treefile = 0;

    GBT_message(gb_main, GBS_global_string("Reading tree from '%s' ..", filename));

    GBT_TREE *tree = GBT_load_tree(filename, sizeof(GBT_TREE), &comment_from_treefile, consense ? 0 : 1);
    if (!tree) {
        show_error(gb_main);
        return -1;
    }

    if (scale) {
        GBT_message(gb_main, GBS_global_string("Scaling branch lengths by factor %f.", scale_factor));
        GBT_scale_tree(tree, scale_factor, 1.0);
    }

    if (consense) {
        if (calculated_trees < 1) {
            GB_export_error("Min. for -consense is 1");
            show_error(gb_main);
            GB_close(gb_main);
            return -1;
        }
        GBT_message(gb_main, GBS_global_string("Reinterpreting branch lengths as consense values (%i trees).", calculated_trees));
        add_bootstrap(tree, calculated_trees);
    }

    GB_begin_transaction(gb_main);
    if (tree->is_leaf){
        const char *err = "Cannot load tree (need at least 2 leafs)";
        GB_export_error(err);
        show_error(gb_main);

        GB_commit_transaction(gb_main);
        GB_close(gb_main);
        return -1;
    }

    GB_ERROR error = GBT_write_tree(gb_main,0,tree_name,tree);
    if (error) {
        GB_export_error(error);
        show_error(gb_main);

        GB_commit_transaction(gb_main);
        GB_close(gb_main);
        return -1;
    }

    // write tree comment:
    {
        const char *cmt = 0;
        if (comment) {
            if (comment_from_treefile)  cmt = GBS_global_string("%s\n%s", comment, comment_from_treefile);
            else                        cmt = comment;
        }
        else {
            if (comment_from_treefile)  cmt = comment_from_treefile;
            else                        cmt = GBS_global_string("loaded from '%s'", filename);
        }
        GBT_write_tree_rem(gb_main, tree_name, cmt);
    }

    free(comment_from_treefile);

    char buffer[256];

    sprintf(buffer,"Tree %s read into the database",tree_name);
    GBT_message(gb_main,buffer);

    GB_commit_transaction(gb_main);
    GB_close(gb_main);
    return 0;
}
