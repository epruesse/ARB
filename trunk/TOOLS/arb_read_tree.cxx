#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arbdb.h>
#include <arbdbt.h>

void error_msg(char **argv){
    printf("syntax: %s [-consense #ofTrees] tree_name treefile [comment]\n",argv[0]);
    exit(-1);
}

void show_error(GBDATA *gb_main) {
    GBT_message(gb_main, GB_get_error());
    GB_print_error();
}

// this interprets the length of the branches as bootstrap value
// (needed by Phylip DNAPARS/PROTPARS with bootstrapping)
//
void add_bootstrap(GBT_TREE *node, double max_val) {
    char buffer[256];
    if (node->is_leaf) return;

    if (max_val <= 1.0) {
        if (node->leftlen < 1.0){
            sprintf(buffer,"%2.0f%%",node->leftlen*100.0+0.5);
            node->leftson->remark_branch = strdup(buffer);
        }
        if (node->rightlen < 1.0){
            sprintf(buffer,"%2.0f%%",node->rightlen*100.0+0.5);
            node->rightson->remark_branch = strdup(buffer);
        }
    }
    else {
        sprintf(buffer,"%2.0f%%",node->leftlen*100.0/max_val);
        node->leftson->remark_branch = strdup(buffer);
        sprintf(buffer,"%2.0f%%",node->rightlen*100.0/max_val);
        node->rightson->remark_branch = strdup(buffer);
    }

    // set all branchlens to same value
    node->leftlen  = 0.1;
    node->rightlen = 0.1;

    add_bootstrap(node->leftson, max_val);
    add_bootstrap(node->rightson, max_val);
}

// -------------------------------------------------
//      double max_branchlength(GBT_TREE *node)
// -------------------------------------------------
double max_branchlength(GBT_TREE *node) {
    if (node->is_leaf) return 0.0;

    double max_val                      = node->leftlen;
    if (node->rightlen>max_val) max_val = node->rightlen;

    double left_max  = max_branchlength(node->leftson);
    double right_max = max_branchlength(node->rightson);

    if (left_max>max_val) max_val = left_max;
    if (right_max>max_val) max_val = right_max;

    return max_val;
}


// ---------------------------------------
//      int main(int argc,char **argv)
// ---------------------------------------
int main(int argc,char **argv)
{
	int arg              = 1;
 	int consense         = 0;
    int calculated_trees = 0;

	if (argc == 1) error_msg(argv);

	GBDATA *gb_main = GB_open(":","r");
	if (!gb_main){
		printf("%s: Error: you have to start an arbdb server first\n", argv[0]);
		return -1;
	}
    GBT_message(gb_main, "Reading tree...");

	if (!strcmp("-consense",argv[arg])){
 	    consense = 1;
	    arg++;

        calculated_trees = atoi(argv[arg++]);
        GBT_message(gb_main, GBS_global_string("Reinterpreting branch lengths as consense values (%i trees)", calculated_trees));
	}

	if (argc!= arg+2 && argc!= arg+3) error_msg(argv);

	const char *comment = "";
	if (argc==arg+3) comment = argv[arg+2];

    char     *comment_from_treefile = 0;
	GBT_TREE *tree                  = GBT_load_tree(argv[arg+1],sizeof(GBT_TREE), &comment_from_treefile);
	if (!tree) {
        show_error(gb_main);
		return -1;
	}

    if (consense) {
//         add_bootstrap(tree, max_branchlength(tree));
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

    char *tree_name = argv[arg];
	GB_ERROR error = GBT_write_tree(gb_main,0,tree_name,tree);
	if (error) {
        GB_export_error(error);
        show_error(gb_main);

	    GB_commit_transaction(gb_main);
	    GB_close(gb_main);
        return -1;
    }

    {
        char *comment_merged = GB_strdup(comment_from_treefile ? GBS_global_string("%s\n%s", comment, comment_from_treefile) : comment);
        GBT_write_tree_rem(gb_main,tree_name,comment_merged);
        free(comment_merged);
    }
    free(comment_from_treefile);

	char buffer[256];

	sprintf(buffer,"Tree %s read into the database",tree_name);
	GBT_message(gb_main,buffer);

	GB_commit_transaction(gb_main);
	GB_close(gb_main);
	return 0;
}
