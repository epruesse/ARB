#include <stdio.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>

#define _NO_AWT_NDS_WINDOW_FUNCTIONS
#include <awt_nds.hxx>

static void awt_export_tree_node_print_remove(char *str)
{
    int i,len;
    len = strlen (str);
    for(i=0;i<len;i++) {
	if (str[i] =='\'') str[i] ='.';
	if (str[i] =='\"') str[i] ='.';
    }
}

static const char *awt_export_tree_node_print(GBDATA *gb_main, FILE *out, GBT_TREE *tree, int deep, int NDS)
{
    int             i;
    const char *error = 0;
    char *buf;
    for (i = 0; i < deep; i++){
	putc(' ',out);
	putc(' ',out);
    }
    if (tree->is_leaf) {
	if (NDS){
	    buf = make_node_text_nds(gb_main, tree->gb_node,0,tree);
	}else{
	    buf = tree->name;
	}
	awt_export_tree_node_print_remove(buf);
	fprintf(out," '%s' ",buf);
    }else{
	fprintf(out, "(\n");
	error = awt_export_tree_node_print(gb_main,out,tree->leftson,deep+1,NDS);
	fprintf(out, ":%.5f,\n", tree->leftlen);
	if (error) return error;
	error = awt_export_tree_node_print(gb_main,out,tree->rightson,deep+1,NDS);
	fprintf(out, ":%.5f\n", tree->rightlen);
	if (tree->name) {
	    if (NDS){
		buf = make_node_text_nds(gb_main, tree->gb_node,0,tree);
	    }else{
		buf = tree->name;
	    }
	    awt_export_tree_node_print_remove(buf);
	}else{
	    buf = 0;
	}
	for (i = 0; i < deep; i++)
	    fprintf(out, "  ");
	if (buf) {
	    fprintf(out, ")'%s'",buf);
	}else{
	    fprintf(out, ")");
	}
    }
    return error;
}

GB_ERROR AWT_export_tree(GBDATA *gb_main, char *tree_name, int NDS, char *path)
{
    FILE           *output = fopen(path, "w");
    if ( output == NULL) {
	return ("file could not be opened for writing");
    }

    GB_ERROR error = 0;
    GB_transaction gb_dummy(gb_main);

    GBT_TREE *tree = GBT_read_tree(gb_main,tree_name,sizeof(GBT_TREE));
    if (!tree) {
	return (char *)GB_get_error();		
    }
	
    error = GBT_link_tree(tree,gb_main,GB_TRUE);
    if (NDS && !error) make_node_text_init(gb_main);
    if (!error) error = 
		    awt_export_tree_node_print(gb_main,output,tree,0,NDS);

    fprintf(output, ";\n");
    fclose(output);
    GBT_delete_tree(tree);
    return error;
}
