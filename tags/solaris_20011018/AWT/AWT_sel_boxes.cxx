#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <malloc.h>
#include <memory.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include "awt.hxx"
#include "awtlocal.hxx"

// ******************** selection boxes on alignments ********************

void awt_create_selection_list_on_ad_cb(GBDATA *dummy, struct adawcbstruct *cbs)
	{
	GBDATA *gb_alignment;
	GBDATA *gb_alignment_name;
	GBDATA *gb_alignment_type;
	char   *alignment_name;
	char   *alignment_type;

	AWUSE(dummy);

	cbs->aws->clear_selection_list(cbs->id);

	for (gb_alignment = GB_search(cbs->gb_main,"presets/alignment",GB_FIND);
         gb_alignment;
         gb_alignment = GB_find(gb_alignment,"alignment",0,this_level|search_next))
    {
		gb_alignment_type = GB_find(gb_alignment,"alignment_type",0,down_level);
		gb_alignment_name = GB_find(gb_alignment,"alignment_name",0,down_level);
		alignment_type    = GB_read_string(gb_alignment_type);
		alignment_name    = GB_read_string(gb_alignment_name);

		char *str = GBS_string_eval(alignment_type,cbs->comm,0);
		if (!*str){
			cbs->aws->insert_selection( cbs->id, alignment_name, alignment_name );
		}
		delete str;
		delete alignment_type;
		delete alignment_name;
	}
	cbs->aws->insert_default_selection( cbs->id, "????", "????" );
	cbs->aws->update_selection_list( cbs->id );
}

void awt_create_selection_list_on_ad(GBDATA *gb_main,AW_window *aws, const char *varname,const char *comm)
	// if comm is set then only those alignments are taken
	// which can be parsed by comm
{
	AW_selection_list*	 id;
	GBDATA	            *gb_presets;
	struct adawcbstruct *cbs;

	GB_push_transaction(gb_main);

	id           = aws->create_selection_list(varname,0,"",20,3);
	cbs          = new adawcbstruct;
	cbs->aws     = aws;
	cbs->gb_main = gb_main;
	cbs->id      = id;
	cbs->comm    = 0; if (comm) cbs->comm = strdup(comm);

	awt_create_selection_list_on_ad_cb(0,cbs);

	gb_presets = GB_search(gb_main,"presets",GB_CREATE_CONTAINER);
	GB_add_callback(gb_presets,GB_CB_CHANGED, (GB_CB)awt_create_selection_list_on_ad_cb, (int *)cbs);

	GB_pop_transaction(gb_main);
}


// ******************** selection boxes on trees ********************

void awt_create_selection_list_on_trees_cb(GBDATA *dummy, struct adawcbstruct *cbs)
{
	AWUSE(dummy);

	cbs->aws->clear_selection_list(cbs->id);
	char **tree_names = GBT_get_tree_names(cbs->gb_main);
	if (tree_names) {
		for (char **tree= tree_names ; *tree; tree++){
			char *info = GBT_tree_info_string(cbs->gb_main, *tree);
			if (info) {
				cbs->aws->insert_selection( cbs->id, info, *tree );
				delete info;
			}else{
				cbs->aws->insert_selection( cbs->id, *tree, *tree );
			}
		}
		GBT_free_names(tree_names);
	}
	cbs->aws->insert_default_selection( cbs->id, "????", "????" );
	cbs->aws->update_selection_list( cbs->id );
}

void awt_create_selection_list_on_trees(GBDATA *gb_main,AW_window *aws,const char *varname)
{
	AW_selection_list *id;
	GBDATA	*gb_tree_data;
	struct adawcbstruct *cbs;
	GB_push_transaction(gb_main);

	id = aws->create_selection_list(varname,0,"",40,8);
	cbs = new adawcbstruct;
	cbs->aws = aws;
	cbs->gb_main = gb_main;
	cbs->id = id;

	awt_create_selection_list_on_trees_cb(0,cbs);

	gb_tree_data = GB_search(gb_main,"tree_data",GB_CREATE_CONTAINER);
	GB_add_callback(gb_tree_data,GB_CB_CHANGED,
		(GB_CB)awt_create_selection_list_on_trees_cb, (int *)cbs);

	GB_pop_transaction(gb_main);
}
// ******************** selection boxes on tables ********************

void awt_create_selection_list_on_tables_cb(GBDATA *dummy, struct awt_sel_list_for_tables *cbs){
    AWUSE(dummy);
    cbs->aws->clear_selection_list(cbs->id);
    GBDATA *gb_table;
    for (gb_table = GBT_first_table(cbs->gb_main);
         gb_table;
         gb_table = GBT_next_table(gb_table)){

        GBDATA *gb_name = GB_find(gb_table,"name",0,down_level);
        GBDATA *gb_description = GB_search(gb_table,"description",GB_STRING);
        if (!gb_name) continue;
        char *table_name = GB_read_string(gb_name);
        char *description = GB_read_string(gb_description);
        const char *info_text = GBS_global_string("%s: %s",table_name,description);
        cbs->aws->insert_selection(cbs->id,info_text,table_name);
        delete table_name;
        delete description;
    }
    cbs->aws->insert_default_selection( cbs->id, "", "" );
    cbs->aws->update_selection_list( cbs->id );
}

void awt_create_selection_list_on_tables(GBDATA *gb_main,AW_window *aws,const char *varname)
{
	AW_selection_list *id;
	GBDATA	*gb_table_data;
	struct awt_sel_list_for_tables *cbs;
	GB_push_transaction(gb_main);

	id = aws->create_selection_list(varname,0,"",40,8);
	cbs = new awt_sel_list_for_tables;
	cbs->aws = aws;
	cbs->gb_main = gb_main;
	cbs->id = id;

	awt_create_selection_list_on_tables_cb(0,cbs);

	gb_table_data = GB_search(gb_main,"table_data",GB_CREATE_CONTAINER);
	GB_add_callback(gb_table_data,GB_CB_CHANGED,(GB_CB)awt_create_selection_list_on_tables_cb, (int *)cbs);

	GB_pop_transaction(gb_main);
}
// ******************** selection boxes on tables ********************

void awt_create_selection_list_on_table_fields_cb(GBDATA *dummy, struct awt_sel_list_for_tables *cbs){
    AWUSE(dummy);
    cbs->aws->clear_selection_list(cbs->id);
    GBDATA	*gb_table = GBT_open_table(cbs->gb_main,cbs->table_name, 1); // read only
    GBDATA *gb_table_field;
    for (gb_table_field = GBT_first_table_field(gb_table);
         gb_table_field;
         gb_table_field = GBT_next_table_field(gb_table_field)){
        GBDATA *gb_name = GB_find(gb_table_field,"name",0,down_level);
        GBDATA *gb_description = GB_search(gb_table_field,"description",GB_STRING);
        if (!gb_name) continue;
        char *table_name = GB_read_string(gb_name);
        char *description = GB_read_string(gb_description);
        const char *info_text = GBS_global_string("%s: %s",table_name,description);
        cbs->aws->insert_selection(cbs->id,info_text,table_name);
        delete table_name;
        delete description;
    }
    cbs->aws->insert_default_selection( cbs->id, "", "" );
    cbs->aws->update_selection_list( cbs->id );
}

void awt_create_selection_list_on_table_fields(GBDATA *gb_main,AW_window *aws,const char *table_name, const char *varname)
{
	AW_selection_list *id;
	struct awt_sel_list_for_tables *cbs;
	GB_push_transaction(gb_main);

	id = aws->create_selection_list(varname,0,"",40,8);
	cbs = new awt_sel_list_for_tables;
	cbs->aws = aws;
	cbs->gb_main = gb_main;
	cbs->id = id;
	cbs->table_name = strdup(table_name);

	awt_create_selection_list_on_table_fields_cb(0,cbs);

	GBDATA	*gb_table = GBT_open_table(gb_main,table_name, 1); // read only
	if (gb_table){
	    GB_add_callback(gb_table,GB_CB_CHANGED,(GB_CB)awt_create_selection_list_on_table_fields_cb, (int *)cbs);
	}
	GB_pop_transaction(gb_main);
}

// ******************** selection boxes on editor configurations ********************

void awt_create_selection_list_on_configurations_cb(GBDATA *dummy, struct adawcbstruct *cbs) {
    AWUSE(dummy);
 restart:
    cbs->aws->clear_selection_list(cbs->id);
    GBDATA *gb_configuration_data = GB_search(cbs->gb_main,AWAR_CONFIG_DATA,GB_CREATE_CONTAINER);
    GBDATA *gb_config;
    for (gb_config = GB_find(gb_configuration_data,0,0,down_level);
	 gb_config;
	 gb_config = GB_find(gb_config,0,0,this_level | search_next))
    {
	GBDATA *gb_name = GB_find(gb_config,"name",0,down_level);
	if (!gb_name){
	    aw_message("internal error: unnamed configuration (now renamed to 'unnamed_config')");
	    gb_name = GB_create(gb_config, "name", GB_STRING);
	    if (!gb_name) {
		char *err = strdup(GB_get_error());
		GB_CSTR question = GBS_global_string("Rename of configuration failed (reason: '%s')\n"
						     "Do you like do delete the unnamed configuration?", err);
		free(err);

		int del_config = 0==aw_message(question, "Yes,No");
		if (del_config) {
		    GB_delete(gb_config);
		    goto restart;
		}
		continue;
	    }
	    GB_write_string(gb_name, "unnamed_config");
	}
	char *name = GB_read_char_pntr(gb_name);
	cbs->aws->insert_selection( cbs->id, name, name );
    }

    cbs->aws->insert_default_selection( cbs->id, "????", "????" );
    cbs->aws->update_selection_list( cbs->id );
}

void awt_create_selection_list_on_configurations(GBDATA *gb_main,AW_window *aws,const char *varname)
{
	AW_selection_list *id;
	GBDATA	*gb_configuration_data;
	struct adawcbstruct *cbs;
	GB_push_transaction(gb_main);

	id           = aws->create_selection_list(varname,0,"",40,15);
	cbs          = new adawcbstruct;
	cbs->aws     = aws;
	cbs->gb_main = gb_main;
	cbs->id      = id;

	awt_create_selection_list_on_configurations_cb(0,cbs);

	gb_configuration_data = GB_search(gb_main,AWAR_CONFIG_DATA,GB_CREATE_CONTAINER);
	GB_add_callback(gb_configuration_data,GB_CB_CHANGED,
		(GB_CB)awt_create_selection_list_on_configurations_cb, (int *)cbs);

	GB_pop_transaction(gb_main);
}

// ******************** selection boxes on SAIs ********************

void awt_create_selection_list_on_extendeds_update(GBDATA *dummy, void *cbsid)
	{
	struct awt_sel_list_for_sai *cbs = (struct awt_sel_list_for_sai *)cbsid;
	GBDATA *gb_extended_data;
	gb_extended_data = GB_search(cbs->gb_main,"extended_data",GB_CREATE_CONTAINER);
	AWUSE(dummy);
	cbs->aws->clear_selection_list(cbs->id);
	GBDATA *gb_extended;
	GBDATA *gb_name;
	for (	gb_extended = GBT_first_SAI(cbs->gb_main);
		gb_extended;
		gb_extended = GBT_next_SAI(gb_extended)){
		gb_name = GB_find(gb_extended,"name",0,down_level);
		if (!gb_name) continue;
		if (cbs->filter_poc) {
			char *res = cbs->filter_poc(gb_extended,cbs->filter_cd);
			if (res) {
				char *name = GB_read_char_pntr(gb_name);
				cbs->aws->insert_selection( cbs->id, res, name );
				delete res;
			}
		}else{
			char *name = GB_read_char_pntr(gb_name);
			cbs->aws->insert_selection( cbs->id, name, name );
		}
	}
	if (cbs->add_selected_species){
		GBDATA *gb_sel = GB_search(cbs->gb_main,AWAR_SPECIES_NAME,GB_STRING);
		char *name = GB_read_string(gb_sel);
		if (strlen(name)){
			char *sname = (char *)calloc(1, strlen(name)+2);
			sprintf(sname+1,"%s",name);
			sname[0] = 1;
			char *text = (char *)GBS_global_string("Selected Species: '%s'",name);
			cbs->aws->insert_selection( cbs->id, text, sname );
			delete name;
		}
		delete name;
	}
	cbs->aws->insert_default_selection( cbs->id, "- none -", "none" );
	cbs->aws->update_selection_list( cbs->id );
}

void *awt_create_selection_list_on_extendeds(GBDATA *gb_main,AW_window *aws, const char *varname,
			char *(*filter_poc)(GBDATA *gb_ext, AW_CL), AW_CL filter_cd,
			AW_BOOL add_sel_species)
{
	AW_selection_list *id;
	GBDATA	*gb_extended_data;
	struct awt_sel_list_for_sai *cbs;
	GB_push_transaction(gb_main);

	id = aws->create_selection_list(varname,0,"",40,8);
	cbs = new awt_sel_list_for_sai;
	cbs->aws = aws;
	cbs->gb_main = gb_main;
	cbs->id = id;
	cbs->filter_poc = filter_poc;
	cbs->filter_cd = filter_cd;
	cbs->add_selected_species = add_sel_species;
	awt_create_selection_list_on_extendeds_update(0,(void *)cbs);

	gb_extended_data = GB_search(gb_main,"extended_data",GB_CREATE_CONTAINER);
	GB_add_callback(gb_extended_data,GB_CB_CHANGED,
		(GB_CB)awt_create_selection_list_on_extendeds_update, (int *)cbs);
	if (add_sel_species){		// update box if another species is selected
		GBDATA *gb_sel = GB_search(gb_main,AWAR_SPECIES_NAME,GB_STRING);
		GB_add_callback(gb_sel,GB_CB_CHANGED,
			(GB_CB)awt_create_selection_list_on_extendeds_update, (int *)cbs);
	}
	GB_pop_transaction(gb_main);
	return (void *)cbs;
}




// ******************** selection boxes on saving selection lists ********************

void create_save_box_for_selection_lists_save(AW_window *aws,AW_CL selidcd,AW_CL basenamecd)
	{
	AW_selection_list *selid = (AW_selection_list *)selidcd;
	char *awar_prefix = (char *)basenamecd;
	char bfile_name[GB_PATH_MAX];
	char bline_anz[GB_PATH_MAX];
	sprintf(bfile_name,"%s/file_name",awar_prefix);
	sprintf(bline_anz,"%s/line_anz",awar_prefix);

	AW_root *aw_root = aws->get_root();
	long lineanz = aw_root->awar(bline_anz)->read_int();
	char *filename = aw_root->awar(bfile_name)->read_string();
	GB_ERROR error = aws->save_selection_list(selid,filename,lineanz);
	if (error) aw_message(error);
	else {
		sprintf(bfile_name,"%s/directory",awar_prefix);
		aws->get_root()->awar(bfile_name)->touch();
		aws->hide();
	}
	delete filename;
}

AW_window *create_save_box_for_selection_lists(AW_root *aw_root,AW_CL selid)
	{
	char base_name[100];
	sprintf(base_name,"tmp/save_box_sel_%li",(long)selid);
	char file_name[100];
	char directory[100];
	char filter[100];
	char line_anz[100];

	sprintf(line_anz,"%s/line_anz",base_name);
	aw_root->awar_int( line_anz, 80);

	sprintf(filter,"%s/filter",base_name);
	aw_root->awar_string( filter, "list");

	sprintf(file_name,"%s/file_name",base_name);
	char *suffix = aw_root->awar(filter)->read_string();
	aw_root->awar_string( file_name, GBS_global_string("noname.%s",suffix));
	delete suffix;

	sprintf(directory,"%s/directory",base_name);
	aw_root->awar_string( directory, ".");


	AW_window_simple *aws = new AW_window_simple;
	aws->init( aw_root, "SAVE_SELECTCION_BOX", "SAVE BOX", 10, 10 );
	aws->load_xfig("sl_s_box.fig");

	aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
	aws->create_button("CLOSE", "CLOSE","C");

	aws->at("save");
	aws->highlight();
	aws->callback(create_save_box_for_selection_lists_save,selid,(AW_CL)strdup(base_name));
	aws->create_button("SAVE", "SAVE","S");


	aws->at("nlines");
	aws->create_option_menu(line_anz,0,"");
	aws->insert_default_option("all","a",0);
	aws->insert_option("40","a",40);
	aws->insert_option("80","a",80);
	aws->insert_option("120","a",120);
	aws->insert_option("160","a",160);
	aws->insert_option("240","a",240);
	aws->update_option_menu();

	awt_create_selection_box((AW_window *)aws,base_name);

	return (AW_window *)aws;
}

void AWT_load_list(AW_window *aww, AW_CL sel_id, AW_CL ibase_name)
{
    AW_selection_list *	selid 		= (AW_selection_list *)sel_id;
    char *basename = (char *)ibase_name;

    AW_root 	*aw_root 	= aww->get_root();
    char 	bfile_name[GB_PATH_MAX];
    GB_ERROR	error;

    sprintf(bfile_name,"%s/file_name",basename);

    char *filename = aw_root->awar(bfile_name)->read_string();
    error = aww->load_selection_list(selid,filename);

    if (error)
	aw_message(error);

    AW_POPDOWN(aww);

    delete filename;
}

AW_window *create_load_box_for_selection_lists(AW_root *aw_root, AW_CL selid)
{
    char file_name[100];
    char directory[100];
    char filter[100];
	char base_name[100];
	sprintf(base_name,"tmp/load_box_sel_%li",(long)selid);

    sprintf(file_name,"%s/file_name",base_name);
    aw_root->awar_string( file_name, "");

    sprintf(directory,"%s/directory",base_name);
    aw_root->awar_string( directory, ".");

    sprintf(filter,"%s/filter",base_name);
    aw_root->awar_string( filter, "list");

    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "LOAD_SELECTION_BOX", "Load box", 10, 10 );
    aws->load_xfig("sl_l_box.fig");

    aws->at("close");
	aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE","C");

    aws->at("load");
    aws->highlight();
    aws->callback(AWT_load_list,selid,(AW_CL)strdup(base_name));
    aws->create_button("LOAD", "LOAD","L");

    awt_create_selection_box((AW_window *)aws,base_name);
	return (AW_window*) aws;
}


void create_print_box_for_selection_lists(AW_window *aw_window,AW_CL selid){
    AW_root *aw_root = aw_window->get_root();
    char *data = aw_window->get_selection_list_contents((AW_selection_list *)selid);
    AWT_create_ascii_print_window(aw_root,data,"no title");
    delete data;
}



/* ************************************************** */
AW_window *awt_create_load_box(AW_root *aw_root, const char *load_what, const char *file_extension, char **set_file_name_awar,
                               void (*callback)(AW_window*),
                               AW_window* (*create_popup)(AW_root *, AW_default))

    /*      You can either provide a normal callback or a create_popup-callback
     *      (the not-used callback has to be 0)
     */


{
    char file_name[100];
    char directory[100];
    char filter[100];
	char base_name[100];

	sprintf(base_name,"tmp/load_box_%s",load_what);

    sprintf(file_name,"%s/file_name",base_name);
    aw_root->awar_string( file_name, "");
    if (set_file_name_awar) *set_file_name_awar = strdup(file_name);

    sprintf(directory,"%s/directory",base_name);
    aw_root->awar_string( directory, ".");

    sprintf(filter,"%s/filter",base_name);
    aw_root->awar_string( filter, file_extension);

    AW_window_simple *aws = new AW_window_simple;
    {
        char title[100];
        sprintf(title, "Load %s", load_what);
        aws->init( aw_root, title, title, 600, 600);
        aws->load_xfig("load_box.fig");
    }

    aws->at("close");
	aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE","C");

	aws->at("help");
	aws->callback(AW_POPUP_HELP,(AW_CL)"");
	aws->create_button("HELP","HELP");

    aws->at("go");
    aws->highlight();

    if (callback) {
        awt_assert(!create_popup);
        aws->callback((AW_CB0)callback);
    }
    else  {
        awt_assert(create_popup);
        aws->callback((AW_CB1)AW_POPUP, (AW_CL)create_popup);
    }

    aws->create_button("LOAD", "LOAD","L");

    awt_create_selection_box((AW_window *)aws,base_name);
	return (AW_window*) aws;
}

/* ************************************************** */



void awt_set_long(AW_window *aws, AW_CL varname, AW_CL value)	// set an awar
	{
	aws->get_root()->awar((char *)varname)->write_int((long) value);
}

void awt_write_string( AW_window *aws, AW_CL varname, AW_CL value)	// set an awar
	{
	aws->get_root()->awar((char *)varname)->write_string((char *)value);
}


void awt_edit(AW_root *awr, const char *path, int x, int y, const char *font){
	char buffer[1024];
	AWUSE(awr);
	char *path2 = GBS_eval_env(path);
	const char *ae = getenv("ARB_TEXTEDIT");
	if (!ae) ae = "textedit";
	if (strcmp(ae,"textedit")==0){
	    sprintf(buffer, "textedit -Ws %i %i -font %s %s &",x,y,font,path2);
	}else{
	    sprintf(buffer, "%s %s &",ae,path2);
	}
	system(buffer);
	delete(path2);
}


/**************************************************************
******************* MACROS **********************************
**************************************************************/
#define AWAR_MACRO_BASE "tmp/macro"
#define AWAR_MACRO_FILENAME AWAR_MACRO_BASE"/file_name"
#define AWAR_MACRO_SUFFIX AWAR_MACRO_BASE"/filter"
#define AWAR_MACRO_DIRECTORY AWAR_MACRO_BASE"/directory"
#define AWAR_MACRO_RECORDING_MACRO_TEXT AWAR_MACRO_BASE"/button_label"

void awt_delete_macro_cb(AW_window *aww){
    char *mn = aww->get_root()->awar(AWAR_MACRO_FILENAME)->read_string();
    int error = GB_unlink(mn);
    if (error) aw_message("Cannot delete");
    else     aww->get_root()->awar(AWAR_MACRO_DIRECTORY)->touch();
    delete mn;
}



void awt_exec_macro_cb(AW_window *aww){
    char *mn = aww->get_root()->awar(AWAR_MACRO_FILENAME)->read_string();
    GB_ERROR error = aww->get_root()->execute_macro(mn);
    if (error) aw_message(error);
    delete mn;
}

void awt_start_macro_cb(AW_window *aww,const char *application_name_for_macros){
    static int toggle = 0;
    char *mn = aww->get_root()->awar(AWAR_MACRO_FILENAME)->read_string();
    GB_ERROR error;
    if (!toggle){
	char *sac = strdup(GBS_global_string("%s/%s",aww->window_defaults_name,AWAR_MACRO_RECORDING_MACRO_TEXT));
	error = aww->get_root()->start_macro_recording(mn,application_name_for_macros,sac);
	delete sac;
	if (!error){
	    aww->get_root()->awar(AWAR_MACRO_RECORDING_MACRO_TEXT)->write_string("STOP");
	    toggle = 1;
	}
    }else{
	error = aww->get_root()->stop_macro_recording();
	aww->get_root()->awar(AWAR_MACRO_DIRECTORY)->touch();
	aww->get_root()->awar(AWAR_MACRO_RECORDING_MACRO_TEXT)->write_string("RECORD");
	toggle = 0;
    }
    if (error) aw_message(error);
    delete mn;
}

void awt_stop_macro_cb(AW_window *aww){
    GB_ERROR error = aww->get_root()->stop_macro_recording();
    aww->get_root()->awar(AWAR_MACRO_DIRECTORY)->touch();
    if (error) aw_message(error);
}

void awt_edit_macro_cb(AW_window *aww){
    char *mn = aww->get_root()->awar(AWAR_MACRO_FILENAME)->read_string();
    char *path = 0;
    if (mn[0] == '/'){
	path = strdup(mn);
    }else{
	path = strdup(GBS_global_string("%s/%s",GB_getenvARBMACROHOME(),mn));
    }
    GB_edit(path);
    delete path;
    delete mn;
}

AW_window *awt_open_macro_window(AW_root *aw_root,const char *application_id){
    static AW_window_simple *aws = 0;
    if (aws) return aws;
    aws = new AW_window_simple;
    aws->init( aw_root, "MACROS", "MACROS", 10, 10 );
    aws->load_xfig("macro_select.fig");

    aw_root->awar_string(AWAR_MACRO_FILENAME,"");
    aw_root->awar_string(AWAR_MACRO_DIRECTORY,".");
    aw_root->awar_string(AWAR_MACRO_SUFFIX,".amc");
    aw_root->awar_string(AWAR_MACRO_RECORDING_MACRO_TEXT,"RECORD");

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE","C");

    aws->at("help");aws->callback(AW_POPUP_HELP,(AW_CL)"macro.hlp");
    aws->create_button("HELP", "HELP");

    aws->at("start");aws->callback((AW_CB1)awt_start_macro_cb,(AW_CL)application_id);
    aws->create_button(0, AWAR_MACRO_RECORDING_MACRO_TEXT);

//    aws->at("stop");aws->callback(awt_stop_macro_cb);
//    aws->create_button("STOP");

    aws->at("delete");aws->callback(awt_delete_macro_cb);
    aws->create_button("DELETE", "DELETE");

    aws->at("edit");aws->callback(awt_edit_macro_cb);
    aws->create_button("EDIT", "EDIT");

    aws->at("exec");aws->callback(awt_exec_macro_cb);
    aws->create_button("EXECUTE", "EXECUTE");

    awt_create_selection_box((AW_window *)aws,AWAR_MACRO_BASE,"","ARBMACROHOME");

    return (AW_window *)aws;
}
