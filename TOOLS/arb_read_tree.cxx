#include <TreeRead.h>
#include <ctime>

// add_bootstrap interprets the length of the branches as bootstrap value
// (this is needed by Phylip DNAPARS/PROTPARS with bootstrapping)
//
// 'hundred' specifies which value represents 100%

void add_bootstrap(GBT_TREE *node, double hundred) {
    if (node->is_leaf) {
        freenull(node->remark_branch);
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

ATTRIBUTED(__ATTR__NORETURN, static void abort_with_usage(GBDATA *gb_main, const char *error)) {
    printf("Usage: arb_read_tree [-scale factor] [-consense #ofTrees] tree_name treefile [comment] [-commentFromFile file]\n");
    if (error) {
        printf("Error: %s\n", error);
        GBT_message(gb_main, GBS_global_string("Error running arb_read_tree (%s)", error));
    }
    exit(-1);
}

int main(int argc, char **argv)
{
    GBDATA *gb_main = GB_open(":", "r");
    if (!gb_main) {
        printf("arb_read_tree: Error: you have to start an arbdb server first\n");
        return -1;
    }

#define SHIFT_ARGS(off) do { argc -= off; argv += off; } while (0)

    SHIFT_ARGS(1);              // position onto first argument

    bool   scale        = false;
    double scale_factor = 0.0;

    if (argc>0 && strcmp("-scale", argv[0]) == 0) {
        scale = true;
        if (argc<2) abort_with_usage(gb_main, "-scale expects a 2nd argument (scale factor)");
        scale_factor = atof(argv[1]);
        SHIFT_ARGS(2);
    }

    bool consense         = false;
    int  calculated_trees = 0;

    if (argc>0 && strcmp("-consense", argv[0]) == 0) {
        consense = true;
        if (argc<2) abort_with_usage(gb_main, "-consense expects a 2nd argument (number of trees)");

        calculated_trees = atoi(argv[1]);
        if (calculated_trees < 1) {
            abort_with_usage(gb_main, GBS_global_string("Illegal # of trees (%i) for -consense", calculated_trees));
        }

        SHIFT_ARGS(2);
    }

    if (!argc) abort_with_usage(gb_main, "Missing argument 'tree_name'");
    const char *tree_name = argv[0];
    SHIFT_ARGS(1);

    if (!argc) abort_with_usage(gb_main, "Missing argument 'treefile'");
    const char *filename = argv[0];
    SHIFT_ARGS(1);

    const char *comment = 0;
    if (argc>0) {
        comment = argv[0];
        SHIFT_ARGS(1);
    }

    const char *commentFile = 0;
    if (argc>0 && strcmp("-commentFromFile", argv[0]) == 0) {
        if (argc<2) abort_with_usage(gb_main, "-commentFromFile expects a 2nd argument (file containing comment)");
        commentFile = argv[1];
        SHIFT_ARGS(2);
    }

    // end of argument parsing!
    if (argc>0) abort_with_usage(gb_main, GBS_global_string("unexpected argument(s): %s ..", argv[0]));

    // --------------------------------------------------------------------------------

    char *comment_from_file = 0;
    if (commentFile) {
        comment_from_file = GB_read_file(commentFile);
        if (!comment_from_file) {
            comment_from_file = GBS_global_string_copy("Error reading from comment-file '%s':\n%s", commentFile, GB_await_error());
        }
    }

    char *comment_from_treefile = 0;
    GBT_message(gb_main, GBS_global_string("Reading tree from '%s' ..", filename));

    GB_ERROR  error = 0;
    GBT_TREE *tree;
    {
        char *scaleWarning = 0;

        tree = TREE_load(filename, sizeof(GBT_TREE), &comment_from_treefile, (consense||scale) ? 0 : 1, &scaleWarning);
        if (!tree) {
            error = GB_await_error();
        }
        else {
            if (scaleWarning) {
                GBT_message(gb_main, scaleWarning);
                free(scaleWarning);
            }
        }
    }

    if (!error) {
        if (scale) {
            GBT_message(gb_main, GBS_global_string("Scaling branch lengths by factor %f.", scale_factor));
            TREE_scale(tree, scale_factor, 1.0);
        }

        if (consense) {
            if (calculated_trees < 1) {
                error = "Minimum for -consense is 1";
            }
            else {
                GBT_message(gb_main, GBS_global_string("Reinterpreting branch lengths as consense values (%i trees).", calculated_trees));
                add_bootstrap(tree, calculated_trees);
            }
        }
    }

    if (!error) {
        error = GB_begin_transaction(gb_main);
        
        if (!error && tree->is_leaf) error = "Cannot load tree (need at least 2 leafs)";
        if (!error) error                  = GBT_write_tree(gb_main, 0, tree_name, tree);

        if (!error) {
            // write tree comment:
            const char *datestring;
            {
                time_t date;
                if (time(&date) == -1) datestring = "<Error calculating time>";
                else datestring = ctime(&date);
            }

            char *load_info = GBS_global_string_copy("Tree loaded from '%s' on %s", filename, datestring);

#define COMMENT_SOURCES 4
            const char *comments[COMMENT_SOURCES] = {
                comment,
                comment_from_file,
                comment_from_treefile,
                load_info,
            };

            GBS_strstruct *buf   = GBS_stropen(5000);
            bool           empty = true;

            for (int c = 0; c<COMMENT_SOURCES; c++) {
                if (comments[c]) {
                    if (!empty) GBS_chrcat(buf, '\n');
                    GBS_strcat(buf, comments[c]);
                    empty = false;
                }
            }

            char *cmt = GBS_strclose(buf);

            error = GBT_write_tree_rem(gb_main, tree_name, cmt);

            free(cmt);
            free(load_info);
        }

        error = GB_end_transaction(gb_main, error);
    }

    if (error) GBT_message(gb_main, error);
    else GBT_message(gb_main, GBS_global_string("Tree %s read into the database", tree_name));
    
    GB_close(gb_main);

    free(comment_from_file);
    free(comment_from_treefile);

    return error ? EXIT_FAILURE : EXIT_SUCCESS;
}
