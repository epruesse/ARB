#include <stdio.h>
#include <stdlib.h>
#include <arbdb.h>
#include <arbdbt.h>

void gen_newick(GBT_TREE *node, FILE *out){
    if (node->is_leaf){
	fprintf(out,"'%s'",node->name);
	return;
    }
    fprintf(out,"(");
    gen_newick(node->leftson,out);
    fprintf(out,",");
    gen_newick(node->rightson,out);
    fprintf(out,")\n");
}


int main(int argc, char **argv)
{
    //	GB_ERROR error;

    GBDATA *gb_main;
    if (argc<1){
	printf("Please specify treename\n");
	exit(-1);
    }
    char *tree_name = argv[1];
	
    gb_main = GBT_open(":","rw",0);
    //    GBDATA *gb_species;
    GB_begin_transaction(gb_main);

#if 0	
    for ( gb_species = GBT_first_marked_species(gb_main);
	  gb_species;
	  gb_species = GBT_next_marked_species(gb_species)){
	GBDATA *gb_name;
	gb_name = GB_search(gb_species,"organism/culture",GB_FIND);
	if (!gb_name) continue;
	GB_write_string(gb_name,"deleted");
	char *name = GB_read_string(gb_name);
	printf("name = '%s'\n",name);
	delete name;
    }

#endif
    GBT_TREE *tree = GBT_read_tree(gb_main,tree_name, sizeof(GBT_TREE));
    if (!tree){
	GB_ERROR error = GB_get_error();
	GB_print_error();
	exit(-1);
    }
    GB_commit_transaction(gb_main);

    gen_newick(tree,stdout);
    return 0;
}
