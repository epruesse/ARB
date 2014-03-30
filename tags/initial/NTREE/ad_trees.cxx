#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <malloc.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <awt.hxx>
#include <awt_imp_exp.hxx>
#include <cat_tree.hxx>
#include <awt_tree_cmp.hxx>

extern GBDATA *gb_main;

const char *AWAR_TREE_NAME		=	"tmp/ad_tree/tree_name";
const char *AWAR_TREE_DEST		=	"tmp/ad_tree/tree_dest";
const char *AWAR_TREE_SECURITY	=	"tmp/ad_tree/tree_security";
const char *AWAR_TREE_REM		=	"tmp/ad_tree/tree_rem";

#define AWAR_TREE_EXPORT	            "tmp/ad_tree/export_tree"
#define AWAR_TREE_IMPORT	            "tmp/ad_tree/import_tree"
#define AWAR_NODE_INFO_ONLY_MARKED	    "tmp/ad_tree/import_only_marked_node_info"

void tree_vars_callback(AW_root *aw_root)		// Map tree vars to display objects
{
	GB_push_transaction(gb_main);
	char *treename = aw_root->awar(AWAR_TREE_NAME)->read_string();
	GBDATA *ali_cont = GBT_get_tree(gb_main,treename);
	if (!ali_cont) {
		aw_root->awar(AWAR_TREE_SECURITY)->unmap();
		aw_root->awar(AWAR_TREE_REM)->unmap();
	}else{

        GBDATA *tree_prot =	GB_search(ali_cont,"security",	GB_FIND);
        if (!tree_prot) {
            GBT_read_int2(ali_cont,"security",
                          GB_read_security_write(ali_cont));
        }
        tree_prot =	GB_search(ali_cont,"security",	GB_INT);
        GBDATA *tree_rem =	GB_search(ali_cont,"remark",	GB_STRING);
        aw_root->awar(AWAR_TREE_SECURITY)->map((void*)tree_prot);
        aw_root->awar(AWAR_TREE_REM)->map((void*)tree_rem);
	}
	char *fname = GBS_string_eval(treename,"*=*1.tree:tree_*=*1",0);
	aw_root->awar(AWAR_TREE_EXPORT "/file_name")->write_string(fname);	// create default file name
	delete fname;
	GB_pop_transaction(gb_main);	
	free(treename);
}
/*	update import tree name depending on file name */
void tree_import_callback(AW_root *aw_root) {
	GB_transaction dummy(gb_main);
	char *treename = aw_root->awar(AWAR_TREE_IMPORT "/file_name")->read_string();

	char *fname = GBS_string_eval(treename,"*.tree=tree_*1",0);
	aw_root->awar(AWAR_TREE_IMPORT "/tree_name")->write_string(fname);
	delete fname;
	delete treename;
}


void ad_tree_set_security(AW_root *aw_root)
{
	GB_transaction dummy(gb_main);
	char *treename = aw_root->awar(AWAR_TREE_NAME)->read_string();
	GBDATA *ali_cont = GBT_get_tree(gb_main,treename);
	if (ali_cont) {
		long prot = aw_root->awar(AWAR_TREE_SECURITY)->read_int();
		long old;
		old = GB_read_security_delete(ali_cont);
		GB_ERROR error = 0;
		if (old != prot){
			error = GB_write_security_delete(ali_cont,prot);
			if (!error)
                error = GB_write_security_write(ali_cont,prot);
		}
		if (error ) aw_message(error);
	}
	free(treename);
}

void update_filter_cb(AW_root *root){
	int NDS = root->awar(AWAR_TREE_EXPORT "/NDS")->read_int();
	switch(NDS){
        case 1:	root->awar(AWAR_TREE_EXPORT "/filter")->write_string("tree");break;
        case 2:	root->awar(AWAR_TREE_EXPORT "/filter")->write_string("otb");break;
        default:root->awar(AWAR_TREE_EXPORT "/filter")->write_string("ntree");break;
	}
}

void create_trees_var(AW_root *aw_root, AW_default aw_def)
{
	aw_root->awar_string( AWAR_TREE_NAME,		0,	aw_def ) ->set_srt( GBT_TREE_AWAR_SRT);
	aw_root->awar_string( AWAR_TREE_DEST,		0,	aw_def ) ->set_srt( GBT_TREE_AWAR_SRT);
	aw_root->awar_int( 	AWAR_TREE_SECURITY, 	0,	aw_def );
	aw_root->awar_string( AWAR_TREE_REM,		0,	aw_def );

	aw_root->awar_string( AWAR_TREE_EXPORT "/file_name", "treefile",aw_def);
	aw_root->awar_string( AWAR_TREE_EXPORT "/directory", "",	aw_def);
	aw_root->awar_string( AWAR_TREE_EXPORT "/filter", "tree",	aw_def);
	aw_root->awar_int(    AWAR_TREE_EXPORT "/NDS", 0,		aw_def)-> add_callback(update_filter_cb);

	aw_root->awar_string( AWAR_TREE_IMPORT "/file_name", "treefile",aw_def);
	aw_root->awar_string( AWAR_TREE_IMPORT "/directory", "",	aw_def);
	aw_root->awar_string( AWAR_TREE_IMPORT "/filter", "tree",	aw_def);
	aw_root->awar_string( AWAR_TREE_IMPORT "/tree_name", "tree_",	aw_def) ->set_srt( GBT_TREE_AWAR_SRT);

	aw_root->awar(AWAR_TREE_IMPORT "/file_name")->add_callback( tree_import_callback);
	aw_root->awar(AWAR_TREE_NAME)->add_callback( tree_vars_callback);
	aw_root->awar(AWAR_TREE_SECURITY)->add_callback( ad_tree_set_security);
    
	aw_root->awar_int( AWAR_NODE_INFO_ONLY_MARKED, 0,	aw_def);
    
	tree_vars_callback(aw_root);
}

void tree_rename_cb(AW_window *aww){
	GB_ERROR error = 0;
	char *source = aww->get_root()->awar(AWAR_TREE_NAME)->read_string();
	char *dest = aww->get_root()->awar(AWAR_TREE_DEST)->read_string();
	error = GBT_check_tree_name(dest);
	if (!error) {
		GB_begin_transaction(gb_main);
		GBDATA *gb_tree_data =
			GB_search(gb_main,"tree_data",GB_CREATE_CONTAINER);
		GBDATA *gb_tree_name =
			GB_find(gb_tree_data,source,0,down_level);
		GBDATA *gb_dest =
			GB_find(gb_tree_data,dest,0,down_level);
		if (gb_dest) {
			error = "Sorry: Tree already exists";
		}else 	if (gb_tree_name) {
			GBDATA *gb_new_tree = GB_create_container(gb_tree_data,dest);
			GB_copy(gb_new_tree, gb_tree_name);
			error = GB_delete(gb_tree_name);
		}else{
			error = "Please select a tree first";
		}
		if (!error) GB_commit_transaction(gb_main);
		else	GB_abort_transaction(gb_main);
	}
	if (error) aw_message(error);
	else aww->hide();
	delete source;
	delete dest;
}

void tree_copy_cb(AW_window *aww){
	GB_ERROR error = 0;
	char *source = aww->get_root()->awar(AWAR_TREE_NAME)->read_string();
	char *dest = aww->get_root()->awar(AWAR_TREE_DEST)->read_string();
	error = GBT_check_tree_name(dest);
	if (!error) {
		GB_begin_transaction(gb_main);
		GBDATA *gb_tree_data =
			GB_search(gb_main,"tree_data",GB_CREATE_CONTAINER);
		GBDATA *gb_tree_name =
			GB_find(gb_tree_data,source,0,down_level);
		GBDATA *gb_dest =
			GB_find(gb_tree_data,dest,0,down_level);
		if (gb_dest) {
			error = "Sorry: Tree already exists";
		}else 			if (gb_tree_name) {
			gb_dest = GB_create_container(gb_tree_data,dest);
			error = GB_copy(gb_dest,gb_tree_name);
		}else{
			error = "Please select a tree first";
		}
		if (!error) GB_commit_transaction(gb_main);
		else	GB_abort_transaction(gb_main);
	}
	if (error) aw_message(error);
	else aww->hide();
	delete source;
	delete dest;
}

void create_tree_last_window(AW_window *aww) {
	GB_ERROR error = 0;
	char *source = aww->get_root()->awar(AWAR_TREE_NAME)->read_string();
	GB_begin_transaction(gb_main);
	GBDATA *gb_tree_data =	GB_search(gb_main,"tree_data",GB_CREATE_CONTAINER);
	GBDATA *gb_tree_name =	GB_find(gb_tree_data,source,0,down_level);
	GBDATA *gb_dest = GB_create_container(gb_tree_data,source);
	error = GB_copy(gb_dest,gb_tree_name);
	if (!error) error = GB_delete(gb_tree_name);
	if (!error) GB_commit_transaction(gb_main);
	else	GB_abort_transaction(gb_main);
	if (error) aw_message(error);
	delete source;

}

void tree_save_cb(AW_window *aww){
	AW_root *aw_root = aww->get_root();
	GB_ERROR error = 0;
	long NDS = aw_root->awar(AWAR_TREE_EXPORT "/NDS")->read_int();
	char *fname = aw_root->awar(AWAR_TREE_EXPORT "/file_name")->read_string();
	char *tree_name = aw_root->awar(AWAR_TREE_NAME)->read_string();
	if (!tree_name || !strlen(tree_name)) error = GB_export_error("Please select a tree first");
	if (!error){
		switch(NDS){
			case 2:
				error = create_and_save_CAT_tree(gb_main,tree_name,fname);
				break;
			default:
				error = AWT_export_tree(gb_main,tree_name,(int)NDS,fname);
				break;
		}
	}

	if (error) aw_message(error);
	else{
		aw_root->awar(AWAR_TREE_EXPORT "/directory")->touch();
		aww->hide();
	}
	delete fname;
	delete tree_name;
}

AW_window *create_tree_export_window(AW_root *root)
{
	AW_window_simple *aws = new AW_window_simple;
	aws->init( root, "SAVE_TREE", "TREE SAVE", 100, 100 );
	aws->load_xfig("sel_box.fig");

	aws->callback( (AW_CB0)AW_POPDOWN);
	aws->at("close");
	aws->create_button("CLOSE","CLOSE","C");			   

	aws->at("help");
	aws->callback(AW_POPUP_HELP,(AW_CL)"tr_export.hlp");
	aws->create_button("HELP","HELP","H");			   

	aws->at("user");
	aws->create_option_menu(AWAR_TREE_EXPORT "/NDS",0,0);
	aws->insert_option("PLAIN FORMAT","P",0);
	aws->insert_option("USE NDS (CANNOT BE RELOADED)","N",1);
	aws->insert_option("ORS TRANSFER BINARY FORMAT","O",2);
	aws->update_option_menu();

	awt_create_selection_box((AW_window *)aws,AWAR_TREE_EXPORT "");

	aws->at("save2");
	aws->callback(tree_save_cb);
	aws->create_button("SAVE","SAVE","o");			   

	aws->callback( (AW_CB0)AW_POPDOWN);
	aws->at("cancel2");
	aws->create_button("CANCEL","CANCEL","C");			   

	return (AW_window *)aws;
}

void tree_load_cb(AW_window *aww){
	AW_root *aw_root = aww->get_root();
	GB_ERROR error = 0;
	char *fname = aw_root->awar(AWAR_TREE_IMPORT "/file_name")->read_string();
	char *tree_name = aw_root->awar(AWAR_TREE_IMPORT "/tree_name")->read_string();
	GBT_TREE *tree = GBT_load_tree(fname,sizeof(GBT_TREE));

    //	long fuzzy = aw_root->awar( AWAR_TREE_IMPORT "/fuzzy")->read_int();

	if (!tree){
		error = GB_get_error();
	}else{
		GB_transaction dummy(gb_main);
        //		if (fuzzy) {
        //			error = GBT_fuzzy_link_tree(tree,gb_main);
        //		}
		if (!error) error = GBT_write_tree(gb_main,0,tree_name,tree);
		GBT_delete_tree(tree);
	}
	if (error) aw_message(error);
	else{
	    aww->hide();
	    aw_root->awar(AWAR_TREE)->write_string(tree_name);	// show new tree
	}
	delete fname;
	delete tree_name;
}

AW_window *create_tree_import_window(AW_root *root)
{
	AW_window_simple *aws = new AW_window_simple;
	aws->init( root, "LOAD_TREE", "TREE LOAD", 100, 100 );
	aws->load_xfig("sel_box.fig");

	aws->callback( (AW_CB0)AW_POPDOWN);
	aws->at("close");
	aws->create_button("CLOSE","CLOSE","C");			   


	aws->at("user");
	aws->label("tree_name:");
	aws->create_input_field(AWAR_TREE_IMPORT "/tree_name",15);

	awt_create_selection_box((AW_window *)aws,AWAR_TREE_IMPORT "");

	aws->at("save2");
	aws->callback(tree_load_cb);
	aws->create_button("LOAD","LOAD","o");			   

	aws->callback( (AW_CB0)AW_POPDOWN);
	aws->at("cancel2");
	aws->create_button("CANCEL","CANCEL","C");			   

	return (AW_window *)aws;
}

AW_window *create_tree_rename_window(AW_root *root)
{
	AW_window_simple *aws = new AW_window_simple;
	aws->init( root, "RENAME_TREE","TREE RENAME", 100, 100 );
	aws->load_xfig("ad_al_si.fig");

	aws->callback( (AW_CB0)AW_POPDOWN);
	aws->at("close");
	aws->create_button("CLOSE","CLOSE","C");			   

	aws->at("label");
	aws->create_button(0,"Please enter the new name\nof the tree");

	aws->at("input");
	aws->create_input_field(AWAR_TREE_DEST,15);

	aws->at("ok");
	aws->callback(tree_rename_cb);
	aws->create_button("GO","GO","G");			   

	return (AW_window *)aws;
}

AW_window *create_tree_copy_window(AW_root *root)
{
	AW_window_simple *aws = new AW_window_simple;
	aws->init( root, "COPY_TREE", "TREE COPY", 100, 100 );
	aws->load_xfig("ad_al_si.fig");

	aws->callback( (AW_CB0)AW_POPDOWN);
	aws->at("close");
	aws->create_button("CLOSE","CLOSE","C");			   

	aws->at("label");
	aws->create_button(0,"Please enter the name\nof the new tree");

	aws->at("input");
	aws->create_input_field(AWAR_TREE_DEST,15);

	aws->at("ok");
	aws->callback(tree_copy_cb);
	aws->create_button("GO","GO","G");			   

	return (AW_window *)aws;
}

void ad_move_tree_info(AW_window *aww,AW_CL mode){
    /*
      mode == 0 -> move info (=overwrite info from source tree)
      mode == 1 -> compare info
      mode == 2 -> add info 
    */
    char *t1 = aww->get_root()->awar(AWAR_TREE_NAME)->read_string();
    char *t2 = aww->get_root()->awar(AWAR_TREE_DEST)->read_string();
    char *log_file = strdup(GBS_global_string("/tmp/arb_log_%s_%i",GB_getenvUSER(),GB_getuid()));
    
    AW_BOOL compare_node_info = mode==1;
    AW_BOOL delete_old_nodes = mode==0;
    AW_BOOL nodes_with_marked_only = (mode==0 || mode==2) && aww->get_root()->awar(AWAR_NODE_INFO_ONLY_MARKED)->read_int();
    
    // move or add node-info writes a log file (containing errors)
    // compare_node_info only sets remark branches
    
    if ( compare_node_info ) { delete log_file;log_file = 0; } // no log if compare_node_info
    
    AWT_move_info(gb_main, t1, t2, log_file, compare_node_info, delete_old_nodes, nodes_with_marked_only);
    
    if (log_file) GB_edit(log_file);
    
    delete log_file;
    delete t2;
    delete t1;
    aww->hide();
}

AW_window *create_tree_diff_window(AW_root *root){
    static AW_window_simple *aws = 0;
    if (aws) return aws;
    GB_transaction dummy(gb_main);
    aws = new AW_window_simple;
    aws->init( root, "CMP_TOPOLOGY", "COMPARE TREE TOPOLOGIES", 200, 0 );
    aws->load_xfig("ad_tree_cmp.fig");
    
    aws->callback( AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");		       

    aws->callback( AW_POPUP_HELP,(AW_CL)"tree_diff.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","H");		       

    root->awar_string(AWAR_TREE_NAME);
    root->awar_string(AWAR_TREE_DEST);

    aws->at("tree1");
    awt_create_selection_list_on_trees(gb_main,(AW_window *)aws,AWAR_TREE_NAME);
    aws->at("tree2");
    awt_create_selection_list_on_trees(gb_main,(AW_window *)aws,AWAR_TREE_DEST);
	
    aws->button_length(20);
    
    aws->at("move");
    aws->callback(ad_move_tree_info,1); // compare
    aws->create_button("CMP_TOPOLOGY","COMPARE TOPOLOGY");		       

    return aws;
}
AW_window *create_tree_cmp_window(AW_root *root){
    static AW_window_simple *aws = 0;
    if (aws) return aws;
    GB_transaction dummy(gb_main);
    aws = new AW_window_simple;
    aws->init( root, "COPY_NODE_INFO_OF_TREE", "TREE COPY INFO", 200, 0 );
    aws->load_xfig("ad_tree_cmp.fig");
    
    aws->callback( AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");		       

    aws->callback( AW_POPUP_HELP,(AW_CL)"tree_cmp.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","H");		       

    root->awar_string(AWAR_TREE_NAME);
    root->awar_string(AWAR_TREE_DEST);
    
    aws->at("tree1");
    awt_create_selection_list_on_trees(gb_main,(AW_window *)aws,AWAR_TREE_NAME);
    aws->at("tree2");
    awt_create_selection_list_on_trees(gb_main,(AW_window *)aws,AWAR_TREE_DEST);
	
    aws->at("move");
    aws->callback(ad_move_tree_info,0); // move
    aws->create_button("COPY_INFO","COPY INFO");		       

    aws->at("cmp");
    aws->callback(ad_move_tree_info,2); // add
    aws->create_button("ADD_INFO","ADD INFO");		       
    
    aws->at("only_marked");
    aws->label("only info containing marked species");
    aws->create_toggle(AWAR_NODE_INFO_ONLY_MARKED);    
    
    return aws;
}
void ad_tr_delete_cb(AW_window *aww){
	GB_ERROR error = 0;
	char *source = aww->get_root()->awar(AWAR_TREE_NAME)->read_string();
	GB_begin_transaction(gb_main);
	GBDATA *gb_tree_data =	GB_search(gb_main,"tree_data",GB_CREATE_CONTAINER);
	GBDATA *gb_tree_name =	GB_find(gb_tree_data,source,0,down_level);
	if (gb_tree_name) {
	    char *newname = GBT_get_next_tree_name(gb_main,source);
	    error = GB_delete(gb_tree_name);
	    if (newname){
            aww->get_root()->awar(AWAR_TREE_NAME)->write_string(newname);
            delete newname;
	    }
	}else{
        error = "Please select a tree first";
	}

	if (!error) GB_commit_transaction(gb_main);
	else	GB_abort_transaction(gb_main);

	if (error) aw_message(error);
	delete source;
}

AW_window *create_trees_window(AW_root *aw_root)
{
	static AW_window_simple *aws = 0;
	if (aws) return aws;
	aws = new AW_window_simple;
	aws->init( aw_root, "TREE_ADMIN","TREE ADMIN", 200, 0 );
	aws->load_xfig("ad_tree.fig");

	aws->callback( AW_POPDOWN);
	aws->at("close");
	aws->create_button("CLOSE","CLOSE","C");			   

	aws->callback( AW_POPUP_HELP,(AW_CL)"treeadm.hlp");
	aws->at("help");
	aws->create_button("HELP","HELP","H");			   

	aws->button_length(20);

	aws->at("delete");
	aws->callback(ad_tr_delete_cb);
	aws->create_button("DELETE","DELETE","D");			   

	aws->at("rename");
	aws->callback(AW_POPUP,(AW_CL)create_tree_rename_window,0);
	aws->create_button("RENAME","RENAME","R");			   

	aws->at("copy");
	aws->callback(AW_POPUP,(AW_CL)create_tree_copy_window,0);
	aws->create_button("COPY","COPY","C");			   

	aws->at("move");
	aws->callback(AW_POPUP,(AW_CL)create_tree_cmp_window,0);
	aws->create_button("MOVE_NODE_INFO","MOVE NODE INFO","C");			   
	
	aws->at("cmp");
	aws->callback(AW_POPUP,(AW_CL)create_tree_diff_window,0);
	aws->create_button("CMP_TOPOLOGY","COMPARE TOPOLOGY","T");			   
	
	aws->at("export");
	aws->callback(AW_POPUP,(AW_CL)create_tree_export_window,0);
	aws->create_button("EXPORT","EXPORT","E");			   

	aws->at("import");
	aws->callback(AW_POPUP,(AW_CL)create_tree_import_window,0);
	aws->create_button("IMPORT","IMPORT","I");			   

	aws->at("last");
	aws->callback(create_tree_last_window);
	aws->create_button("PUT_TO_END","PUT TO END","P");			   

	aws->at("list");
	awt_create_selection_list_on_trees(gb_main,(AW_window *)aws,AWAR_TREE_NAME);

	aws->at("security");
	aws->create_option_menu(AWAR_TREE_SECURITY);
	aws->insert_option("0","0",0);
	aws->insert_option("1","1",1);
	aws->insert_option("2","2",2);
	aws->insert_option("3","3",3);
	aws->insert_option("4","4",4);
	aws->insert_option("5","5",5);
	aws->insert_default_option("6","6",6);
	aws->update_option_menu();

	aws->at("rem");
	aws->create_text_field(AWAR_TREE_REM);

	return (AW_window *)aws;
}