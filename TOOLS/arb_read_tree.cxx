#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arbdb.h>
#include <arbdbt.h>

void error_msg(char **argv){
    printf("syntax: %s [-consense] tree_name treefile [comment]\n",argv[0]);
    exit(-1);

}

void add_bootstrap(GBT_TREE *node){
    char buffer[256];
    if (node->is_leaf) return;
    if (node->leftlen < 1.0){
	sprintf(buffer,"%2.0f%%",node->leftlen*100.0);
	node->leftson->remark_branch = strdup(buffer);
    }
    if (node->rightlen < 1.0){
	sprintf(buffer,"%2.0f%%",node->rightlen*100.0);
	node->rightson->remark_branch = strdup(buffer);
    }
    add_bootstrap(node->leftson);
    add_bootstrap(node->rightson);
}

int main(int argc,char **argv)
	{

	int arg = 1;
	int consense = 0;
	if (argc == 1) error_msg(argv);
	if (!strcmp("-consense",argv[arg])){
	    consense = 1;
	    arg++;
	}
	    
	if (argc!= arg+2 && argc!= arg+3) error_msg(argv);
	
	const char *comment = "";
	if (argc==arg+3) comment = argv[arg+2];

	GBDATA *gb_main = GB_open(":","r");
	if (!gb_main){
		printf("error: you have to start an arbdb server first\n",argv[0]);
		return -1;
	}
    
    GBT_message(gb_main, "Reading tree...");
	GBT_TREE *tree = GBT_load_tree(argv[arg+1],sizeof(GBT_TREE));
	if (!tree) {
		GB_print_error();
		return -1;
	}
	if (consense){
	    add_bootstrap(tree);
	}
	GB_begin_transaction(gb_main);
	if (tree->is_leaf){
	    GBT_message(gb_main,"Cannot load tree");
	    GB_commit_transaction(gb_main);
	    GB_close(gb_main);
	    return -1;
	}
	
	char *tree_name = argv[arg];
	GB_ERROR error = GBT_write_tree(gb_main,0,tree_name,tree);
	if (error) {
        GBT_message(gb_main,error);
	    GB_commit_transaction(gb_main);
	    GB_close(gb_main);
        return -1;
    }
	GBT_write_tree_rem(gb_main,tree_name,comment);
    
	char buffer[256];
	sprintf(buffer,"Tree %s read into the database",tree_name);
	GBT_message(gb_main,buffer);
	GB_commit_transaction(gb_main);
	GB_close(gb_main);
	return 0;
}
