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
        GB_export_error(    "\narb_export_tree:     export tree to stdout"
                            "       syntax: arb_export_tree TREE_NAME");
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
        if (tree){
            GBT_export_tree(gb_main,stdout,tree,GB_TRUE);
        }
        printf(";\n");
    }
    GB_close(gb_main);
    return 0;
}
        
