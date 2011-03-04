// =============================================================== //
//                                                                 //
//   File      : arb_export_tree.cxx                               //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <TreeWrite.h>
#include <arbdbt.h>

/*      Input   Tree_name       argv[1]
 *      Output(stdout)  one line tree
 *      */


int main(int argc, char **argv) {
    int exitcode = EXIT_SUCCESS;

    FILE *real_STDOUT = stdout;
    stdout            = stderr;

    if (argc < 2) {
        fprintf(stderr,
                "\n"
                "arb_export_tree - exports tree from running ARB to stdout\n"
                "syntax: arb_export_tree [--bifurcated] TREE_NAME [DBNAME]\n"
                "        --bifurcated     write a bifurcated tree (default is a trifurcated tree)\n"
                "        --nobranchlens   do not write branchlengths\n"
                "        --doublequotes   use doublequotes (default is singlequotes)\n"
                "\n"
                "Note: If TREE_NAME is '\?\?\?\?' or '' an empty tree is written.\n"
                "      If DBNAME is not specified, the default DB server ':' will be used.\n"
                );
        exitcode = EXIT_FAILURE;
    }
    else {
        GB_ERROR    error        = NULL;
        bool        trifurcated  = true;
        bool        branchlens   = true;
        bool        doublequotes = false;
        const char *tree_name    = NULL;
        const char *db_name      = NULL;

        for (int a = 1; a < argc && !error; a++) {
            if      (strcmp(argv[a], "--bifurcated")   == 0) trifurcated  = false;
            else if (strcmp(argv[a], "--nobranchlens") == 0) branchlens   = false;
            else if (strcmp(argv[a], "--doublequotes") == 0) doublequotes = true;
            else {
                if      (!tree_name) tree_name = argv[a];
                else if (!db_name)   db_name   = argv[a];
                else {
                    error = GBS_global_string("Superfluous argument '%s'", argv[a]);
                }
            }
        }

        if (!tree_name && !error) error = "Missing argument TREE_NAME";

        if (!error) {
            if (!db_name) db_name = ":";

            GB_shell  shell;
            GBDATA   *gb_main = GBT_open(db_name, "r", 0);

            if (!gb_main) error = GB_await_error();
            else {
                { 
                    GB_transaction dummy(gb_main);

                    GBT_TREE *tree = GBT_read_tree(gb_main, tree_name, - sizeof(GBT_TREE));
                    if (tree) {
                        error = TREE_export_tree(gb_main, real_STDOUT, tree, trifurcated, branchlens, doublequotes);
                        GBT_delete_tree(tree);
                    }
                    else if (tree_name[0] && strcmp(tree_name, "????") != 0) {
                        // ignore tree names '????' and '' (no error, just export empty tree)
                        char *warning = GBS_global_string_copy("arb_export_tree: Tree '%s' does not exist in DB '%s'", tree_name, db_name);
                        GBT_message(gb_main, warning);
                        free(warning);
                    }
                }

                if (!error) fprintf(real_STDOUT, ";\n"); // aka empty tree
                GB_close(gb_main);
            }
        }

        if (error) {
            fprintf(stderr, "\narb_export_tree: Error: %s\n", error);
            exitcode = EXIT_FAILURE;
        }
        
    }
    return exitcode;
}

