// ============================================================= //
//                                                               //
//   File      : arb_read_tree.cxx                               //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include <TreeRead.h>
#include <arb_strbuf.h>
#include <arb_defs.h>
#include <ctime>


static void add_bootstrap(GBT_TREE *node, double hundred) {
    // add_bootstrap interprets the length of the branches as bootstrap value
    // (this is needed by Phylip DNAPARS/PROTPARS with bootstrapping)
    //
    // 'hundred' specifies which value represents 100%

    if (node->is_leaf) {
        node->remove_remark();
        return;
    }

    node->leftlen  /= hundred;
    node->rightlen /= hundred;

    double left_bs  = node->leftlen  * 100.0;
    double right_bs = node->rightlen * 100.0;

#if defined(DEBUG) && 0
    fprintf(stderr, "node->leftlen  = %f left_bs  = %f\n", node->leftlen, left_bs);
    fprintf(stderr, "node->rightlen = %f right_bs = %f\n", node->rightlen, right_bs);
#endif // DEBUG

    node->leftson ->set_bootstrap(left_bs);
    node->rightson->set_bootstrap(right_bs);

    node->leftlen  = DEFAULT_BRANCH_LENGTH; // reset branchlengths
    node->rightlen = DEFAULT_BRANCH_LENGTH;

    add_bootstrap(node->leftson, hundred);
    add_bootstrap(node->rightson, hundred);
}

static void show_message(GBDATA *gb_main, const char *msg) {
    if (gb_main) {
        GBT_message(gb_main, msg);
    }
    else {
        fflush(stdout);
        printf("arb_read_tree: %s\n", msg);
    }
}
static void show_error(GBDATA *gb_main, GB_ERROR error) {
    if (error) show_message(gb_main, GBS_global_string("Error running arb_read_tree (%s)", error));
}

static void error_with_usage(GBDATA *gb_main, GB_ERROR error) {
    fputs("Usage: arb_read_tree [options] tree_name treefile [comment]\n"
          "Available options:\n"
          "    -db database savename    specify database and savename (default is 'running ARB')\n"
          "    -scale factor            scale branchlengths by 'factor'\n"
          "    -consense numberOfTrees  reinterpret branchlengths as consense values\n"
          "    -commentFromFile file    read tree comment from 'file'\n"
          , stdout);

    show_error(gb_main, error);
}

struct parameters {
    const char *dbname;
    const char *dbsavename;
    const char *tree_name;
    const char *treefilename;
    const char *comment;
    const char *commentFile;
    
    bool   scale;
    double scale_factor;

    bool consense;
    int  calculated_trees;

    parameters()
        : dbname(":"),
          dbsavename(NULL),
          tree_name(NULL),
          treefilename(NULL),
          comment(NULL),
          commentFile(NULL),
          scale(false),
          scale_factor(0.0),
          consense(false),
          calculated_trees(0)
    {
    }
    
#define SHIFT_ARGS(off) do { argc -= off; argv += off; } while (0)
#define SHIFT_NONSWITCHES(off) do { nonSwitches -= off; nonSwitch += off; } while (0)

    GB_ERROR scan(int argc, char **argv) {
        GB_ERROR error = NULL;

        const char  *nonSwitch_buf[20];
        const char **nonSwitch   = nonSwitch_buf;
        int          nonSwitches = 0;

        SHIFT_ARGS(1);              // position onto first argument

        while (argc>0 && !error) {
            if (strcmp("-scale", argv[0]) == 0) {
                scale = true;
                if (argc<2) error = "-scale expects a 2nd argument (scale factor)";
                else {
                    scale_factor = atof(argv[1]);
                    SHIFT_ARGS(2);
                }
            }
            else if (strcmp("-consense", argv[0]) == 0) {
                consense = true;
                if (argc<2) error = "-consense expects a 2nd argument (number of trees)";
                else {
                    calculated_trees = atoi(argv[1]);
                    if (calculated_trees < 1) {
                        error = GBS_global_string("Illegal # of trees (%i) for -consense (minimum is 1)", calculated_trees);
                    }
                    else SHIFT_ARGS(2);
                }
            }
            else if (strcmp("-commentFromFile", argv[0]) == 0) {
                if (argc<2) error = "-commentFromFile expects a 2nd argument (file containing comment)";
                else {
                    commentFile = argv[1];
                    SHIFT_ARGS(2);
                }
            }
            else if (strcmp("-db", argv[0]) == 0) {
                if (argc<3) error = "-db expects two arguments (database and savename)";
                else {
                    dbname     = argv[1];
                    dbsavename = argv[2];
                    SHIFT_ARGS(3);
                }
            }
            else {
                nonSwitch[nonSwitches++] = argv[0];
                SHIFT_ARGS(1);
            }
        }

        if (!error) {
            if (!nonSwitches) error = "Missing argument 'tree_name'";
            else {
                tree_name = nonSwitch[0];
                SHIFT_NONSWITCHES(1);
            }
        }
        if (!error) {
            if (!nonSwitches) error = "Missing argument 'treefile'";
            else {
                treefilename = nonSwitch[0];
                SHIFT_NONSWITCHES(1);
            }
        }
        if (!error && nonSwitches>0) {
            comment = nonSwitch[0];
            SHIFT_NONSWITCHES(1);
        }

        if (!error && nonSwitches>0) {
            error = GBS_global_string("unexpected argument(s): %s ..", nonSwitch[0]);
        }
        return error;
    }
};

int main(int argc, char **argv) {
    parameters param;
    GB_ERROR error = param.scan(argc, argv);

    GBDATA   *gb_main      = NULL;
    GBDATA   *gb_msg_main  = NULL;
    bool      connectToArb = strcmp(param.dbname, ":") == 0;
    GB_shell  shell;

    if (!error || connectToArb) {
        gb_main                       = GB_open(param.dbname, connectToArb ? "r" : "rw");
        if (connectToArb) gb_msg_main = gb_main;
    }

    if (error) error_with_usage(gb_main, error);
    else {
        if (!gb_main) {
            if (connectToArb) error = "you have to start an arbdb server first";
            else error = GBS_global_string("can't open db (Reason: %s)", GB_await_error());
        }

        char     *comment_from_file     = 0;
        char     *comment_from_treefile = 0;
        GBT_TREE *tree                  = 0;

        if (!error) {
            if (param.commentFile) {
                comment_from_file = GB_read_file(param.commentFile);
                if (!comment_from_file) {
                    comment_from_file = GBS_global_string_copy("Error reading from comment-file '%s':\n%s", param.commentFile, GB_await_error());
                }
            }

            show_message(gb_msg_main, GBS_global_string("Reading tree from '%s' ..", param.treefilename));
            {
                char *warnings             = 0;
                bool  allow_length_scaling = !param.consense && !param.scale;

                tree = TREE_load(param.treefilename, GBT_TREE_NodeFactory(), &comment_from_treefile, allow_length_scaling, &warnings);
                if (!tree) {
                    error = GB_await_error();
                }
                else if (warnings) {
                    show_message(gb_msg_main, warnings);
                    free(warnings);
                }
            }
        }

        if (!error) {
            if (param.scale) {
                show_message(gb_msg_main, GBS_global_string("Scaling branch lengths by factor %f", param.scale_factor));
                TREE_scale(tree, param.scale_factor, 1.0);
            }

            if (param.consense) {
                if (param.calculated_trees < 1) {
                    error = "Minimum for -consense is 1";
                }
                else {
                    show_message(gb_msg_main, GBS_global_string("Reinterpreting branch lengths as consense values (%i trees)", param.calculated_trees));
                    add_bootstrap(tree, param.calculated_trees);
                }
            }
        }

        if (!error) {
            error = GB_begin_transaction(gb_main);

            if (!error && tree->is_leaf) error = "Cannot load tree (need at least 2 leafs)";
            if (!error) error                  = GBT_write_tree(gb_main, param.tree_name, tree);

            if (!error) {
                // write tree comment
                const char *comments[] = {
                    param.comment,
                    comment_from_file,
                    comment_from_treefile,
                };

                GBS_strstruct *buf   = GBS_stropen(5000);
                bool           empty = true;

                for (size_t c = 0; c<ARRAY_ELEMS(comments); c++) {
                    if (comments[c]) {
                        if (!empty) GBS_chrcat(buf, '\n');
                        GBS_strcat(buf, comments[c]);
                        empty = false;
                    }
                }

                char *cmt = GBS_strclose(buf);

                error = GBT_write_tree_remark(gb_main, param.tree_name, cmt);

                free(cmt);
            }

            error = GB_end_transaction(gb_main, error);
        }

        if (error) show_error(gb_main, error);
        else       show_message(gb_msg_main, GBS_global_string("Tree %s read into the database", param.tree_name));

        delete tree;
        free(comment_from_file);
        free(comment_from_treefile);
    }

    if (gb_main) {
        if (!error && !connectToArb) {
            error = GB_save_as(gb_main, param.dbsavename, "a");
            if (error) show_error(gb_main, error);
        }
        GB_close(gb_main);
    }

    return error ? EXIT_FAILURE : EXIT_SUCCESS;
}
