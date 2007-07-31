#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <cat_tree.hxx>

static void error_msg(char **argv) __attribute__((noreturn));
static void error_msg(char **argv){
    printf("syntax: %s treefile otb-file\n"
           "	syntax of tree tips:	name#full_name\n"
	   ,argv[0]);
    exit(-1);
}



int main(int argc,char **argv)
	{

	if (argc != 3) error_msg(argv);
	char *nname = argv[1];
	char *otb_name = argv[2];


	GBT_TREE *tree = GBT_load_tree(nname,sizeof(GBT_TREE), 0, 1);
	if (!tree) {
	    GB_print_error();
	    return -1;
	}
	GB_ERROR error = create_and_save_CAT_tree(tree,otb_name);
	if (error){
	    fprintf(stderr,"%s\n",error);
	    return -1;
	}
	return 0;
}
