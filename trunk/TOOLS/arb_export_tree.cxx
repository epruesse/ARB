#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>

/*      Input   Tree_name       argv[1]
 *      Output(stdout)  one line tree
 *      */


int main(int argc, char **argv){
    //  GB_ERROR error;
    if (argc <= 1) {
        GB_export_error("arb_export_tree: export tree to stdout\n"
                        "         syntax: arb_export_tree TREE_NAME\n"
                        "         Note: If TREE_NAME is '\?\?\?\?' or '' an empty tree is written.");
        GB_print_error();
        exit(-1);
    }

    GBDATA *gb_main = GBT_open(":","r",0);
    if (!gb_main){
        GB_print_error();
        return 1;
    }
    char *tree_name = argv[1];

    {
        GB_transaction dummy(gb_main);
        GBT_TREE *tree = GBT_read_tree(gb_main,tree_name, - sizeof(GBT_TREE));
        if (tree) {
            GBT_export_tree(gb_main,stdout,tree,GB_TRUE);
        }
        else {
            if (tree_name[0] && strcmp(tree_name, "????") != 0) // ignore tree names '????' and '' (no error, just export empty tree)
            {
                char *err = GBS_global_string_copy("arb_export_tree: Tree '%s' does not exist in DB\n", tree_name);
                GBT_message(gb_main, err);
                free(err);
            }
        }
        printf(";\n");
    }
    GB_close(gb_main);
    return 0;
}
        
