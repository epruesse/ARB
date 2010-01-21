#include <stdio.h>

#include <arbdbt.h>
#include <TreeWrite.h>

/*      Input   Tree_name       argv[1]
 *      Output(stdout)  one line tree
 *      */


int main(int argc, char **argv) {
    int exitcode = EXIT_SUCCESS;

    if (argc < 2) {
        fprintf(stderr, "\narb_export_tree - exports tree from running ARB to stdout\n"
                        "syntax: arb_export_tree [--bifurcated] TREE_NAME\n"
                        "        --bifurcated     write a bifurcated tree (default is a trifurcated tree)\n"
                        "        --nobranchlens   do not write branchlengths\n"
                        "        --doublequotes   use doublequotes (default is singlequotes)\n"
                        "\n"
                "Note: If TREE_NAME is '\?\?\?\?' or '' an empty tree is written.");
        exitcode = EXIT_FAILURE;
    }
    else {
        GB_ERROR  error   = 0;
        GBDATA   *gb_main = GBT_open(":", "r", 0);
        
        if (!gb_main) {
            error    = GB_await_error();
            exitcode = EXIT_FAILURE;
        }
        else {
            bool        trifurcated  = true;
            bool        branchlens   = true;
            bool        doublequotes = false;
            const char *tree_name    = 0;

            for (int a = 1; a < argc; a++) {
                if (strcmp(argv[a], "--bifurcated") == 0) trifurcated = false;
                else if (strcmp(argv[a], "--nobranchlens") == 0) branchlens = false;
                else if (strcmp(argv[a], "--doublequotes") == 0) doublequotes = true;
                else tree_name = argv[a];
            }

            if (!tree_name) error = "Missing argument TREE_NAME";
            else {
                GB_transaction dummy(gb_main);
                GBT_TREE *tree = GBT_read_tree(gb_main, tree_name, - sizeof(GBT_TREE));
                if (tree) {
                    error = TREE_export_tree(gb_main, stdout, tree, trifurcated, branchlens, doublequotes);
                }
                else {
                    if (tree_name[0] && strcmp(tree_name, "????") != 0) // ignore tree names '????' and '' (no error, just export empty tree)
                    {
                        char *err = GBS_global_string_copy("arb_export_tree: Tree '%s' does not exist in DB\n", tree_name);
                        GBT_message(gb_main, err);
                        free(err);
                    }
                    else {
                        error = GB_await_error();
                    }
                }
                printf(";\n"); // aka empty tree
            }
            GB_close(gb_main);
        }

        if (error) {
            fprintf(stderr, "\narb_export_tree: Error: %s\n", error);
        }
    }
    return exitcode;
}

