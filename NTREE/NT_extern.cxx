

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>

#include "ad_trees.hxx"
#include "ad_ext.hxx"
#include "ad_spec.hxx"
#include <awt.hxx>
#include <awt_nds.hxx>
#include <awt_canvas.hxx>
#include <aw_preset.hxx>
#include <awt_preset.hxx>
#include <awtc_rename.hxx>
#include <awtc_submission.hxx>
#include <gde.hxx>

#include <awt_www.hxx>
#include <awt_tree.hxx>
#include <awt_dtree.hxx>
#include <awt_tree_cb.hxx>
#include "ntree.hxx"
#include "nt_cb.hxx"
#include "etc_check_gcg.hxx"
#include "nt_sort.hxx"
#include "ap_consensus.hxx"
#include "ap_csp_2_gnuplot.hxx"
#include <awti_export.hxx>
#include "nt_join.hxx"
#include "nt_edconf.hxx"
#include "ap_pos_var_pars.hxx"
#include "ad_ali.hxx"
#include "nt_import.hxx"
#include "nt_date.h"
#include <st_window.hxx>
#include <probe_design.hxx>
#include <primer_design.hxx>
#include <GEN.hxx>


void create_probe_design_variables(AW_root *aw_root,AW_default def,AW_default global);
void create_cprofile_var(AW_root *aw_root, AW_default aw_def);

void create_insertchar_variables(AW_root *root,AW_default db1);
AW_window *create_insertchar_window( AW_root *root, AW_default def);

AW_window *AP_open_cprofile_window( AW_root *root );

extern AW_window 	*MP_main(AW_root *root, AW_default def);

AW_window *create_tree_window(AW_root *aw_root,AWT_graphic *awd);

#define F_ALL ((AW_active)-1)

void NT_show_message(AW_root *awr)
{
    char *mesg = awr->awar("tmp/message")->read_string();
    if (*mesg){
        aw_message(mesg);
        awr->awar("tmp/message")->write_string("");
    }
    delete mesg;
}

void nt_test_ascii_print(AW_window *aww){
    AWT_create_ascii_print_window(aww->get_root(),"hello world","Just a test");
}

void nt_changesecurity(AW_root *aw_root) {
    long level = aw_root->awar(AWAR_SECURITY_LEVEL)->read_int();
    GB_push_transaction(gb_main);
    GB_change_my_security(gb_main,(int)level,"");
    GB_pop_transaction(gb_main);
}

void export_nds_cb(AW_window *aww,AW_CL print_flag) {
    GB_transaction dummy(gb_main);
    GBDATA *gb_species;
    char *buf;
    FILE *out;
    char *name = aww->get_root()->awar(AWAR_EXPORT_NDS"/file_name")->read_string();
    out = fopen(name,"w");
    if (!out) {
        delete name;
        aw_message("Error: Cannot open and write to file");
        return;
    }
    make_node_text_init(gb_main);

    for (gb_species = GBT_first_marked_species(gb_main);
         gb_species;
         gb_species = GBT_next_marked_species(gb_species)){
        buf = make_node_text_nds(gb_main, gb_species,1,0);
        fprintf(out,"%s\n",buf);
    }
    aww->get_root()->awar(AWAR_EXPORT_NDS"/directory")->touch();
    fclose(out);
    if (print_flag){
        GB_textprint(name);
    }
    delete name;
}

AW_window *create_nds_export_window(AW_root *root){
	AW_window_simple *aws = new AW_window_simple;
	aws->init( root, "EXPORT_NDS_OF_MARKED", "EXPORT NDS OF MARKED SPECIES", 100, 100 );
	aws->load_xfig("sel_box.fig");

	aws->callback( (AW_CB0)AW_POPDOWN);
	aws->at("close");
	aws->create_button("CLOSE","CLOSE","C");

	aws->callback( AW_POPUP_HELP, (AW_CL)"arb_export_nds.hlp");
	aws->at("help");
	aws->create_button("HELP","HELP","H");


	aws->callback( (AW_CB0)AW_POPDOWN);
	aws->at("cancel");
	aws->create_button("CLOSE","CANCEL","C");

	aws->at("save");
	aws->callback(export_nds_cb,0);
	aws->create_button("SAVE","SAVE","S");

	aws->at("print");
	aws->callback(export_nds_cb,1);
	aws->create_button("PRINT","PRINT","P");


	awt_create_selection_box((AW_window *)aws,AWAR_EXPORT_NDS);

	return (AW_window *)aws;
}

void create_export_nds_awars(AW_root *awr, AW_default def)
{
    awr->awar_string( AWAR_EXPORT_NDS"/file_name", "export.nds", def);
    awr->awar_string( AWAR_EXPORT_NDS"/directory", "", def);
    awr->awar_string( AWAR_EXPORT_NDS"/filter", "nds", def);
}

void create_all_awars(AW_root *awr, AW_default def)
{
	GB_transaction dummy(gb_main);
	awr->awar_string( "tmp/LeftFooter", "", def);

	if (GB_read_clients(gb_main)>=0){
		awr->awar_string( AWAR_TREE, "tree_main", gb_main);
	}else{
		awr->awar_string( AWAR_TREE, "tree_main", def);
	}
	awr->awar_int( AWAR_SECURITY_LEVEL, 0, def);
	awr->awar_string( AWAR_SPECIES_NAME, "" ,	gb_main);
    GEN_create_awars(awr, def);

	awr->awar(AWAR_SECURITY_LEVEL)->add_callback(nt_changesecurity);

	create_insertchar_variables(awr,def);
	create_probe_design_variables(awr,def,gb_main);
	create_primer_design_variables(awr,def);
	create_trees_var(awr,def);
	create_extendeds_var(awr,def);
	create_species_var(awr,def);
	create_consensus_var(awr,def);
	create_gde_var(awr,def);
	create_cprofile_var(awr,def);
	create_transpro_variables(awr,def);
	NT_build_resort_awars(awr,def);

	create_alignment_vars(awr,def);
	create_nds_vars(awr,def,gb_main);
	create_export_nds_awars(awr,def);
	AWTC_create_rename_variables(awr,def);
	create_check_gcg_awars(awr,def);
	awt_create_dtree_awars(awr,gb_main);
	awt_create_aww_vars(awr,def);

	awr->awar_string( "tmp/message", "", gb_main);
	awr->awar_string( AWAR_DB_COMMENT, "<no description>", gb_main);

	AWTC_create_submission_variables(awr, gb_main);

	if (GB_read_clients(gb_main) >=0) {	// no i am the server
		awr->awar("tmp/message")->add_callback( NT_show_message);
	}
}



void
nt_exit(AW_window *aw_window){
#ifdef DEBUG
    exit(0);
#endif
    AWUSE(aw_window);
    if (gb_main) {
        if (GB_read_clients(gb_main)>=0) {
#ifdef NDEBUG
            if (GB_read_clock(gb_main) > GB_last_saved_clock(gb_main)){
                long secs;
                secs = GB_last_saved_time(gb_main);
                if (secs) {
                    secs = GB_time_of_day() - secs;
                    if (secs>10){
                        sprintf(AW_ERROR_BUFFER,"You last saved your data %li:%li minutes ago\nSure to quit ?",secs/60,secs%60);
                        if (aw_message(AW_ERROR_BUFFER,
                                       "QUIT ARB,DO NOT QUIT")) return;
                    }
                }else{
                    if (aw_message("You never saved any data\nSure to quit ???",
                                   "QUIT ARB,DO NOT QUIT")) return;
                }
            }
#endif // NDEBUG
        }
        GBCMS_shutdown(gb_main);
        GB_exit(gb_main);
    }
    exit(0);
}

void
NT_save_cb(AW_window *aww)
{
	char *filename = aww->get_root()->awar(AWAR_DB_PATH)->read_string();
	GB_ERROR error = GB_save(gb_main, filename, "b");
	delete filename;
	if (error) aw_message(error);
	else	aww->get_root()->awar("tmp/nt/arbdb/directory")->touch();
}


void
NT_save_quick_cb(AW_window *aww)
{
	char *filename = aww->get_root()->awar(AWAR_DB_PATH)->read_string();
	GB_ERROR error = GB_save_quick(gb_main,filename);
	delete filename;
	if (error) aw_message(error);
	else	aww->get_root()->awar("tmp/nt/arbdb/directory")->touch();
}

void
NT_save_quick_as_cb(AW_window *aww)
{
	char *filename = aww->get_root()->awar(AWAR_DB_PATH)->read_string();
	GB_ERROR error = GB_save_quick_as(gb_main, filename);
	delete filename;
	if (error) {
		aw_message(error);
	}else{
		aww->hide();
		aww->get_root()->awar("tmp/nt/arbdb/directory")->touch();
	}
}


AW_window *NT_create_save_quick_as(AW_root *aw_root, char *base_name)
{
	static AW_window_simple *aws = 0;
	if (aws) return (AW_window *)aws;

	aws = new AW_window_simple;
	aws->init( aw_root, "SAVE_CHANGES_TO", "SAVE CHANGES TO", 10, 10 );
	aws->load_xfig("save_as.fig");

	aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
	aws->create_button("CLOSE","CLOSE","C");

	aws->callback( (AW_CB0)AW_POPDOWN);
	aws->at("cancel4");
	aws->create_button("CANCEL","CANCEL","C");

	aws->callback( AW_POPUP_HELP, (AW_CL)"save.hlp");
	aws->at("help");
	aws->create_button("HELP","HELP","H");

	awt_create_selection_box((AW_window *)aws,base_name);

	aws->at("user2");
	aws->create_button(0,"Database Description");

	aws->at("user3");
	aws->create_text_field(AWAR_DB_COMMENT);

	aws->at("save4");aws->callback(NT_save_quick_as_cb);
	aws->create_button("SAVE","SAVE","S");

	return (AW_window *)aws;
}

void NT_database_optimization(AW_window *aww){
    GB_ERROR error = 0;

    {
        aw_openstatus("Optimizing Database, Please Wait");
        char *tree_name = aww->get_root()->awar("tmp/nt/arbdb/optimize_tree_name")->read_string();
        GB_begin_transaction(gb_main);
        char **ali_names = GBT_get_alignment_names(gb_main);
        aw_status("Checking Sequence Lengths");
        GBT_check_data(gb_main,0);
        GB_commit_transaction(gb_main);

        char **ali_name;
        for (ali_name = ali_names;*ali_name; ali_name++){
            aw_status(*ali_name);
            error = GBT_compress_sequence_tree2(gb_main,tree_name,*ali_name);
            if (error) break;
        }
        GBT_free_names(ali_names);
        delete tree_name;
        aw_closestatus();
    }

    int errors = 0;
    if (error)
    {
        errors++;
        aw_message(error);
    }

    aw_openstatus("(Re-)Compressing database, Please Wait");
    error = GB_optimize(gb_main);
    aw_closestatus();

    if (error)
    {
        errors++;
        aw_message(error);
    }

    if (!errors) aww->hide();

}

AW_window *NT_create_database_optimization_window(AW_root *aw_root){
	static AW_window_simple *aws = 0;
	if (aws) return (AW_window *)aws;
	GB_transaction dummy(gb_main);

	char *largest_tree = GBT_find_largest_tree(gb_main);
	aw_root->awar_string("tmp/nt/arbdb/optimize_tree_name",largest_tree);
	delete largest_tree;

	aws = new AW_window_simple;
	aws->init( aw_root, "OPTIMIZE_DATABASE", "OPTIMIZE DATABASE", 10, 10 );
	aws->load_xfig("optimize.fig");

	aws->at("trees");
	awt_create_selection_list_on_trees(gb_main,(AW_window *)aws,"tmp/nt/arbdb/optimize_tree_name");

	aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
	aws->create_button("CLOSE","CLOSE","C");

	aws->callback( AW_POPUP_HELP, (AW_CL)"optimize.hlp");
	aws->at("help");
	aws->create_button("HELP","HELP","H");

	aws->at("go");
	aws->callback(NT_database_optimization);
	aws->create_button("GO","GO");
	return aws;
}

void
NT_save_as_cb(AW_window *aww)
{
	char *filename = aww->get_root()->awar(AWAR_DB_PATH)->read_string();
	char *filetype = aww->get_root()->awar(AWAR_DB"type")->read_string();
	GB_ERROR error = GB_save(gb_main, filename, filetype);
	delete filename;
	delete filetype;
	if (error) {
		aw_message(error);
	}else{
		aww->hide();
		aww->get_root()->awar("tmp/nt/arbdb/directory")->touch();
	}
}


AW_window *NT_create_save_as(AW_root *aw_root,const char *base_name)
{
	static AW_window_simple *aws = 0;
	if (aws) return (AW_window *)aws;

	aws = new AW_window_simple;
	aws->init( aw_root, "SAVE_DB", "SAVE ARB DB", 10, 10 );
	aws->load_xfig("save_as.fig");

	aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
	aws->create_button("CLOSE","CLOSE","C");

	aws->callback( AW_POPUP_HELP, (AW_CL)"save.hlp");
	aws->at("help");
	aws->help_text("optimize.hlp");
	aws->create_button("HELP","HELP","H");

	awt_create_selection_box((AW_window *)aws,base_name);

	aws->at("user");
	aws->label("Type");
	aws->create_option_menu(AWAR_DB"type");
    aws->insert_option("Binary","B","b");
    aws->insert_option("Bin (with FastLoad File)","f","bm");
    aws->insert_default_option("Ascii","A","a");
	aws->update_option_menu();

	aws->at("user2");
	aws->create_button(0,"Database Description");


	aws->callback( (AW_CB0)AW_POPDOWN);
	aws->at("cancel4");
	aws->create_button("CANCEL","CANCEL","C");


	aws->at("opti4");
	aws->callback(AW_POPUP,(AW_CL)NT_create_database_optimization_window,0);
	aws->create_button("OPTIMIZE","OPTIMIZE...");

	aws->at("save4");aws->callback(NT_save_as_cb);
	aws->create_button("SAVE","SAVE","S");

	aws->at("user3");
	aws->create_text_field(AWAR_DB_COMMENT);

	return (AW_window *)aws;
}

void NT_undo_cb(AW_window *, AW_CL undo_type, AW_CL ntw){
	GB_ERROR error = GB_undo(gb_main,(GB_UNDO_TYPE)undo_type);
	if (error) aw_message(error);
	else{
	    GB_transaction dummy(gb_main);
	    ((AWT_canvas *)ntw)->refresh();
	}
}

void NT_undo_info_cb(AW_window *,AW_CL undo_type){
    char *undo_info = GB_undo_info(gb_main,(GB_UNDO_TYPE)undo_type);
    if (undo_info){
        aw_message(undo_info);
    }
    delete undo_info;
}

AW_window *NT_create_tree_setting(AW_root *aw_root)
{
	static AW_window_simple *aws = 0;
	if (aws) return (AW_window *)aws;

	aws = new AW_window_simple;
	aws->init( aw_root, "TREE_PROPS", "TREE SETTINGS", 10, 10 );
	aws->load_xfig("awt/tree_settings.fig");

	aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
	aws->create_button("CLOSE","CLOSE","C");

	aws->at("help");aws->callback(AW_POPUP_HELP,(AW_CL)"nt_tree_settings.hlp");
	aws->create_button("HELP","HELP","H");

	aws->at("button");
	aws->auto_space(10,10);
	aws->label_length(30);

	aws->label("Base Line Width");
	aws->create_input_field(AWAR_DTREE_BASELINEWIDTH,4);
	aws->at_newline();

	aws->label("Relativ vert. Dist");
	aws->create_input_field(AWAR_DTREE_VERICAL_DIST,4);
	aws->at_newline();

	aws->label("Auto Jump (list tree)");
	aws->create_toggle(AWAR_DTREE_AUTO_JUMP);
	aws->at_newline();

	aws->label("Show Bootstrap Circles");
	aws->create_toggle(AWAR_DTREE_SHOW_CIRCLE);
	aws->at_newline();

	aws->label("Bootstrap circle zoom factor");
	aws->create_input_field(AWAR_DTREE_CIRCLE_ZOOM,4);
	aws->at_newline();

	aws->label("Grey Level of Groups%");
	aws->create_input_field(AWAR_DTREE_GREY_LEVEL,4);
	aws->at_newline();

	return (AW_window *)aws;

}

void NT_submit_mail(AW_window *aww){
    char *address = aww->get_root()->awar("/tmp/nt/register/mail")->read_string();
    char *plainaddress = GBS_string_eval(address,"\"=:'=\\=",0); // Remove all dangerous symbols
    char *text = aww->get_root()->awar("/tmp/nt/register/text")->read_string();
    char buffer[256];
    sprintf(buffer,"/tmp/arb_bugreport_%s",GB_getenvUSER());
    FILE *mail = fopen(buffer,"w");
    if (!mail){
        aw_message(GB_export_error("Cannot write file %s",buffer));
    }else{
        fprintf(mail,"%s\n",text);
        fprintf(mail,"VERSION		:" DATE "\n");
        fprintf(mail,"SYSTEMINFO	:\n");
        fclose(mail);

        system(GBS_global_string("uname -a >>%s",buffer));
        system(GBS_global_string("date  >>%s",buffer));
        const char *command = GBS_global_string("mail '%s' <%s",plainaddress,buffer);
        system(command);
        printf("%s\n",command);
        GB_unlink(buffer);
        aww->hide();
    }
    delete text;
    delete address;
    delete plainaddress;
}

AW_window *NT_submit_bug(AW_root *aw_root, int bug_report){
	static AW_window_simple *aws = 0;
	if (aws) return (AW_window *)aws;

	aws = new AW_window_simple;
	aws->init( aw_root, "SUBMIT_BUG", "SUBMIT INFO", 10, 10 );
	aws->load_xfig("bug_report.fig");

	aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
	aws->create_button("CLOSE","CLOSE","C");

	aws->at("help");aws->callback(AW_POPUP_HELP,(AW_CL)"registration.hlp");
	aws->create_button("HELP","HELP","H");

	aw_root->awar_string("/tmp/nt/register/mail","arb@mikro.biologie.tu-muenchen.de");
	aws->at("mail");
	aws->create_input_field("/tmp/nt/register/mail");

	if (bug_report){
	    aw_root->awar_string("/tmp/nt/register/text","Enter your bug report here:\n");
	}else{
	    aw_root->awar_string("/tmp/nt/register/text",
                             "******* Registration *******\n"
                             "\n"
                             "Name			:\n"
                             "Department		:\n"
                             "How many users	:\n"
                             "Why do you want to use arb ?\n"
                             );
	}
	aws->at("box");
	aws->create_text_field("/tmp/nt/register/text");


	aws->at("go");
	aws->callback(NT_submit_mail);
	aws->create_button("SEND","SEND");

	return aws;
}

void NT_focus_cb(AW_window *aww)
{
	AWUSE(aww);
	GB_transaction dummy(gb_main);
}


void NT_modify_cb(AW_window *aww,AW_CL cd1,AW_CL cd2)
{
	AW_window *aws = create_species_window(aww->get_root());
	aws->show();
	nt_mode_event(aww,(AWT_canvas*)cd1,(AWT_COMMAND_MODE)cd2);
}

void NT_primer_cb(void) {
	GB_xcmd("arb_primer",1);
}


void NT_set_compression(AW_window *, AW_CL dis_compr, AW_CL){
	GB_transaction dummy(gb_main);
	GB_ERROR       error = GB_set_compression(gb_main,dis_compr);
	if (error) aw_message(error);
}

void NT_mark_long_branches(AW_window *aww, AW_CL ntwcl){
    GB_transaction dummy(gb_main);
    AWT_canvas *ntw = (AWT_canvas *)ntwcl;
    char *val = aw_input("Enter relativ diff [0 .. 5.0]",0);
	if (!val) return;	// operation cancelled
    NT_unmark_all_cb(aww,ntw);
    AWT_TREE(ntw)->tree_root->mark_long_branches(gb_main,atof(val));
    AWT_TREE(ntw)->tree_root->compute_tree(ntw->gb_main);
    delete val;
    ntw->refresh();
}
void NT_justify_branch_lenghs(AW_window *, AW_CL ntwcl){
    GB_transaction dummy(gb_main);
    AWT_canvas *ntw = (AWT_canvas *)ntwcl;
    if (AWT_TREE(ntw)->tree_root){
        AWT_TREE(ntw)->tree_root->justify_branch_lenghs(gb_main);
        AWT_TREE(ntw)->tree_root->compute_tree(ntw->gb_main);
        AWT_TREE(ntw)->save(ntw->gb_main,0,0,0);
        ntw->refresh();
    }
}

GB_ERROR NT_create_configuration_cb(AW_window *, AW_CL cl_GBT_TREE_ptr, AW_CL cl_use_species_aside) {
    GBT_TREE **ptree             = (GBT_TREE**)(cl_GBT_TREE_ptr);
    int        use_species_aside = int(cl_use_species_aside);
    return NT_create_configuration(0, ptree, 0, use_species_aside);
}

#define AWMIMT awm->insert_menu_topic
AW_window * create_nt_main_window(AW_root *awr, AW_CL clone){
    GB_push_transaction(gb_main);
    AW_gc_manager  aw_gc_manager;
    char	      *awar_tree;
    char	       window_title[256];
    awar_tree = (char *)GB_calloc(sizeof(char),strlen(AWAR_TREE) + 10);	// do not free this

    if (clone){
        sprintf(awar_tree,AWAR_TREE "_%li", clone);
        sprintf(window_title,"ARB_NT_%li",clone);
    }else{
        sprintf(awar_tree,AWAR_TREE);
        sprintf(window_title,"ARB_NT");
    }
    AW_window_menu_modes *awm = new AW_window_menu_modes();
    awm->init(awr,window_title, window_title, 0,0,0,0);

    awm->button_length(5);

    nt.tree = (AWT_graphic_tree*)NT_generate_tree(awr,gb_main);
    AWT_canvas *ntw = new AWT_canvas(gb_main,(AW_window *)awm,nt.tree, aw_gc_manager,awar_tree) ;
    char *tree_name = awr->awar_string(awar_tree)->read_string();
    char *existing_tree_name = GBT_existing_tree(gb_main,tree_name);
    delete tree_name;
    if (existing_tree_name) {
        awr->awar(awar_tree)->write_string(existing_tree_name);

        NT_reload_tree_event(awr,ntw,GB_FALSE); // load first tree !!!!!!!
    }else{
#if !defined(DEBUG)
        AW_POPUP_HELP(awm,(AW_CL)"no_tree.hlp");
#endif // DEBUG
    }

    awr->awar( awar_tree)->add_callback( (AW_RCB)NT_reload_tree_event, (AW_CL)ntw,(AW_CL)GB_FALSE);
    delete existing_tree_name;
    awr->awar( AWAR_SPECIES_NAME)->add_callback( (AW_RCB)NT_jump_cb_auto, (AW_CL)ntw,0);
    awr->awar( AWAR_DTREE_VERICAL_DIST)->add_callback( (AW_RCB)AWT_resize_cb, (AW_CL)ntw,0);
    awr->awar( AWAR_DTREE_BASELINEWIDTH)->add_callback( (AW_RCB)AWT_expose_cb, (AW_CL)ntw,0);
    awr->awar( AWAR_DTREE_SHOW_CIRCLE)->add_callback( (AW_RCB)AWT_expose_cb, (AW_CL)ntw,0);
    awr->awar( AWAR_DTREE_CIRCLE_ZOOM)->add_callback( (AW_RCB)AWT_expose_cb, (AW_CL)ntw,0);

    GBDATA *gb_arb_presets =	GB_search(gb_main,"arb_presets",GB_CREATE_CONTAINER);
    GB_add_callback(gb_arb_presets,GB_CB_CHANGED,(GB_CB)AWT_expose_cb, (int *)ntw);

    // --------------------------------------------------------------------------------
    //     File
    // --------------------------------------------------------------------------------
    if (clone){
        awm->create_menu(       0,   "File",     "F", "nt_file.hlp",  AWM_ALL );
        AWMIMT( "close", "Close",					"C",0,		AWM_ALL, (AW_CB)AW_POPDOWN, 	0, 0 );

    }else{
        awm->create_menu(       0,   "File",     "F", "nt_file.hlp",  AWM_ALL );
        {
            //	AWMIMT("save_whole_db",	"Save Whole Database",		       	"S","save.hlp",	F_ALL, (AW_CB)NT_save_cb, 	0, 	0);
            AWMIMT("save_changes",	"Save Changes",			"S","save.hlp",	AWM_ALL, (AW_CB)NT_save_quick_cb, 0, 	0);
            //	AWMIMT("save_changes_as","Save Changes As ...",			"A","save.hlp",	AWM_EXP, AW_POPUP,	(AW_CL)NT_create_save_quick_as, (AW_CL)"tmp/nt/arbdb");
            AWMIMT("save_all_as",	"Save Whole Database As ...",		"W","save.hlp",	AWM_ALL, AW_POPUP, 	(AW_CL)NT_create_save_as, (AW_CL)"tmp/nt/arbdb");
            awm->insert_separator();

            AWMIMT("optimize_db",	"Optimize Database ...",		"O","optimize.hlp",AWM_EXP, AW_POPUP, 	(AW_CL)NT_create_database_optimization_window, 0);
            awm->insert_separator();

            AWMIMT("import_seq",	"Import Sequences and Fields (ARB) ...","I","arb_import.hlp",AWM_ALL, NT_import_sequences,0,0);
            GDE_load_menu(awm,"import");

            awm->insert_sub_menu(  0,"Export",		"E" );
            {
                AWMIMT("export_to_ARB",	"Export Seq/Tree/SAI's to New ARB Database ...","X", "arb_ntree.hlp",	AWM_ALL, (AW_CB)NT_system_cb,	(AW_CL)"arb_ntree -export &",0 );
                AWMIMT("export_seqs",	"Export Sequences to Foreign Format ...",	 "E","arb_export.hlp",	AWM_ALL, AW_POPUP, (AW_CL)open_AWTC_export_window, (AW_CL)gb_main);
                AWMIMT("export_nds",	"Export Database Fields Selected by NDS of Marked Species ...","N","arb_export_nds.hlp",AWM_EXP, AW_POPUP, (AW_CL)create_nds_export_window, 0);
                GDE_load_menu(awm,"export");
            }
            awm->close_sub_menu();
            awm->insert_separator();

            AWMIMT("print_tree", "Print Tree ...",			"P","tree2prt.hlp",	AWM_ALL,	(AW_CB)AWT_create_print_window, (AW_CL)ntw,	0 );
            GDE_load_menu(awm,"pretty_print");
            awm->insert_separator();


            AWMIMT("macros",	"Macros  ...",				"M", "macro.hlp",	AWM_ALL, 		(AW_CB)AW_POPUP,	(AW_CL)awt_open_macro_window,(AW_CL)"ARB_NT");
            AWMIMT("new_window",	"New Window",			"N","newwindow.hlp",		F_ALL, AW_POPUP, (AW_CL)create_nt_main_window, clone+1 );
            awm->insert_sub_menu(  0,"Registration/Bug report/Version info ",		"A" );
            {
                AWMIMT( "registration",	"Registration ...",			"R","registration.hlp", AWM_EXP, (AW_CB)AW_POPUP, 	(AW_CL)NT_submit_bug, 0 );
                AWMIMT( "bug_report",	"Bug Report ...", 			"B","registration.hlp",	AWM_ALL, (AW_CB)AW_POPUP, 	(AW_CL)NT_submit_bug, 1 );
                AWMIMT( "version_info",	"Version Info (" DATE ") ...",	"V","version.hlp",	AWM_ALL, (AW_CB)AW_POPUP_HELP, 	(AW_CL)"version.hlp", 0 );
            }
            awm->close_sub_menu();
#if 0
            awm->insert_separator();
            AWMIMT( "undo", 	"Undo",				"U","undo.hlp",		F_ALL, (AW_CB)NT_undo_cb, 	(AW_CL)GB_UNDO_UNDO,(AW_CL)ntw );
            AWMIMT( "redo",		"Redo",				"d","undo.hlp",		F_ALL, (AW_CB)NT_undo_cb, 	(AW_CL)GB_UNDO_REDO,(AW_CL)ntw );
            AWMIMT( "undo_info",	"Undo Info",			"f","undo.hlp",		F_ALL, (AW_CB)NT_undo_info_cb, 	(AW_CL)GB_UNDO_UNDO,(AW_CL)0 );
            AWMIMT( "undo_info",	"Redo Info",			"o","undo.hlp",		F_ALL, (AW_CB)NT_undo_info_cb, 	(AW_CL)GB_UNDO_REDO,(AW_CL)0 );

            awm->insert_separator();
#endif
            if (!nt.extern_quit_button){
                AWMIMT( "quit",		"Quit",				"Q","quit.hlp",		AWM_ALL, (AW_CB)nt_exit, 	0, 0 );
            }

        }
        // --------------------------------------------------------------------------------
        //     Species
        // --------------------------------------------------------------------------------
        awm->create_menu(0,"Species","p","species.hlp",	AWM_ALL);
        {
            AWMIMT( "species_info",		"Info (Copy Delete Rename Modify) ...", "I",	"sp_info.hlp",		AWM_ALL,AW_POPUP,   (AW_CL)create_species_window,	0 );
            AWMIMT( "species_search",	"Search and Query",			"Q",	"sp_search.hlp",	AWM_ALL,AW_POPUP,   (AW_CL)ad_create_query_window,	0 );
            awm->insert_sub_menu(0, "Species Database Fields Admin","F");
            {
                ad_spec_create_field_items(awm);
            }
            awm->close_sub_menu();
            AWMIMT( "species_submission", "Submission...",				"S",	"submission.hlp",	AWM_ALL,AW_POPUP,   (AW_CL)AWTC_create_submission_window,	0 );
            awm->insert_separator();
            AWMIMT("mark_all",	"Mark all Species",		"M","sp_mrk_all.hlp",	AWM_ALL, (AW_CB)NT_mark_all_cb,			(AW_CL)ntw, 0 );
            AWMIMT("unmark_all",	"Unmark all Species",		"U","sp_umrk_all.hlp",	AWM_ALL, (AW_CB)NT_unmark_all_cb,		(AW_CL)ntw, 0 );
            AWMIMT("swap_marked",	"Swap Marked Species",		"w","sp_invert_mrk.hlp",AWM_ALL, (AW_CB)NT_invert_mark_all_cb,		(AW_CL)ntw, 0 );
            AWMIMT("mark_tree",	"Mark Species in Tree",		"T","sp_mrk_tree.hlp",	AWM_EXP, (AW_CB)NT_mark_tree_cb,		(AW_CL)ntw, 0 );
            AWMIMT("unmark_tree",	"Unmark Species in Tree",	"n","sp_umrk_tree.hlp",	AWM_EXP, (AW_CB)NT_unmark_all_tree_cb,		(AW_CL)ntw, 0 );
            AWMIMT("count_marked",	"Count Marked Species",		"C","sp_count_mrk.hlp",	AWM_ALL, (AW_CB)NT_count_mark_all_cb,		(AW_CL)ntw, 0 );
            awm->insert_separator();
            AWMIMT( "create_config","Create Selection from Marked Species ...", "r", "configuration.hlp",AWM_SEQ2,	(AW_CB)NT_create_configuration_cb, (AW_CL)&(nt.tree->tree_root), 0);
            AWMIMT( "read_config",	"Extract Marked Species from Selection ...","x", "configuration.hlp",AWM_SEQ2,	(AW_CB)AW_POPUP, (AW_CL)NT_extract_configuration, 0 );
            AWMIMT( "del_config",	"Delete Selection ...",			    "D", "configuration.hlp",AWM_SEQ2,	(AW_CB)AW_POPUP, (AW_CL)NT_extract_configuration, 0 );
            awm->insert_separator();
            awm->insert_sub_menu(0, "Destroy Species",		"y");
            {
                AWMIMT("del_marked",	"Delete Marked Species ...",	"D","sp_del_mrkd.hlp",	AWM_EXP, (AW_CB)NT_delete_mark_all_cb,		(AW_CL)ntw, 0 );
                AWMIMT("join_marked",	"Join Marked Species ...",	"J","join_species.hlp",	AWM_EXP, AW_POPUP, (AW_CL)create_species_join_window,	0 );
            }
            awm->close_sub_menu();
            awm->insert_sub_menu(0, "Sort Species",			"o");
            {
                AWMIMT("sort_by_field",	"According Database Entries ...","D","sp_sort_fld.hlp",	AWM_EXP, AW_POPUP,				(AW_CL)NT_build_resort_window, 0 );
                AWMIMT("sort_by_tree",	"According Phylogeny",		"P","sp_sort_phyl.hlp",	AWM_EXP, (AW_CB)NT_resort_data_by_phylogeny,	(AW_CL)&(nt.tree->tree_root), 0 );
            }
            awm->close_sub_menu();

            awm->insert_separator();
            awm->insert_sub_menu(0, "Etc",				"E");
            {
                AWMIMT( "new_names",	"Generate New Names ...",	"G", "sp_rename.hlp",	AWM_EXP, AW_POPUP,   (AW_CL)AWTC_create_rename_window, 		(AW_CL)gb_main );
            }
            awm->close_sub_menu();

        }
        //  -------------
        //      Genes
        //  -------------

        bool is_genom_db = false;
        {
            GB_transaction  dummy(gb_main);
            GBDATA         *gb_main_genom_db  = GB_find(gb_main, GENOM_DB_TYPE, 0, down_level);
            if (gb_main_genom_db) is_genom_db = GB_read_int(gb_main_genom_db) != 0;
        }

        if (is_genom_db) GEN_create_genes_submenu(awm, true);

        // --------------------------------------------------------------------------------
        //     Sequence
        // --------------------------------------------------------------------------------
        awm->create_menu(0,"Sequence","S","sequence.hlp",	AWM_ALL);
        {
            AWMIMT("seq_admin",	"Admin ...",			"","ad_align.hlp",	AWM_EXP, 	AW_POPUP, (AW_CL)create_alignment_window,	0 );
            awm->insert_separator();
            AWMIMT("new_arb_edit4",	"Edit Marked Sequences using Selected Species and Tree",	"4", "arb_edit4.hlp",AWM_ALL,	(AW_CB)NT_start_editor_on_tree, (AW_CL)&(nt.tree->tree_root), 0 );
            AWMIMT("new2_arb_edit4","Edit Marked Sequences (plus sequences aside) ...",	"4", "arb_edit4.hlp",AWM_ALL,	(AW_CB)NT_start_editor_on_tree, (AW_CL)&(nt.tree->tree_root), -1);
            AWMIMT("old_arb_edit4", "Edit Sequences using earlier Selection",	"O", "arb_edit4.hlp",AWM_SEQ2,	AW_POPUP, (AW_CL)NT_start_editor_on_old_configuration, 0 );
            awm->insert_sub_menu(0, "Other Sequence Editors","E");
            {
                AWMIMT( "arb_edit",	"Edit Marked Sequences (ARB) ...",	"A", "arb_edit.hlp",	AWM_SEQ2,	(AW_CB)NT_system_cb,	(AW_CL)"arb_edit &",	0 );
                AWMIMT( "arb_ale",	"Edit Marked Sequences (ALE) ...",	"L", "ale.hlp",		AWM_SEQ2,	(AW_CB)NT_system_cb,	(AW_CL)"arb_ale : &", (AW_CL)"ale.hlp" );
                AWMIMT( "arb_gde",	"Edit Marked Sequences (GDE) ...",	"G", "gde.hlp",		AWM_SEQ2,	(AW_CB)NT_system_cb,	(AW_CL)"arb_gde : &", (AW_CL)"gde.hlp" );
            }
            awm->close_sub_menu();

            awm->insert_separator();
            AWMIMT("ins_del_col",	"Insert_Delete Column ...",		"I","insdelchar.hlp",		AWM_SEQ2, 	AW_POPUP, (AW_CL)create_insertchar_window,	0 );
            //	awm->insert_sub_menu(0, "Translate",			"T");
            AWMIMT("dna_2_pro",	"Translate Nucleic to Amino Acid ...",	"T","translate_dna_2_pro.hlp",	AWM_PRO,	AW_POPUP, (AW_CL)create_dna_2_pro_window, 0 );
            //	awm->close_sub_menu();
            awm->insert_sub_menu(0, "Multiple Sequence Comparison",	"M");
            {
                AWMIMT("arb_dist",	"Distance Matrix ...",			"D", "dist.hlp",		AWM_SEQ,	(AW_CB)NT_system_cb,	(AW_CL)"arb_dist &",	0 );
                AWMIMT("seq_quality",	"Sequence Quality Check ...",		"Q", "check_quality.hlp",	AWM_SEQ2,	AW_POPUP,(AW_CL)st_create_quality_check_window,	(AW_CL)gb_main );
            }
            awm->close_sub_menu();


            awm->insert_sub_menu(0, "Align Sequences",	"A");
            {
                AWMIMT("arb_align",	"Align Sequence into an Existing Alignment ...","a","align.hlp",		AWM_ALL,	(AW_CB)	AW_POPUP_HELP,(AW_CL)"align.hlp",0);
                AWMIMT("realign_dna",	"Realign Nucleic Acid according to Aligned Protein ...","r","realign_dna.hlp",	AWM_ALL,	AW_POPUP, (AW_CL)create_realign_dna_window, 0 );
                GDE_load_menu(awm,"align");
            }
            awm->close_sub_menu();
        }
        // --------------------------------------------------------------------------------
        //     SAI
        // --------------------------------------------------------------------------------
        awm->create_menu(0,"SAI","A","extended.hlp",	AWM_ALL);
        {
            AWMIMT("sai_admin", "Admin ...",				"A","ad_extended.hlp",	AWM_EXP, 	AW_POPUP, (AW_CL)create_extendeds_window,	0 );
            awm->insert_separator();
            awm->insert_sub_menu(0, "Functions: Create SAI From Sequences",	"F");
            {
                AWMIMT("sai_max_freq",	"Max. Frequency ...",				"M","max_freq.hlp",	AWM_ALL, AW_POPUP, (AW_CL)AP_open_max_freq_window, 0 );
                AWMIMT("sai_consensus",	"Consensus ...",				"C","consensus.hlp",	AWM_ALL, AW_POPUP, (AW_CL)AP_open_con_expert_window, 0 );
                AWMIMT("pos_var_pars",	"Positional Variability + Column Statistic (Parsimony Method) ...","a","pos_var_pars.hlp", AWM_TREE2, AW_POPUP, (AW_CL)AP_open_pos_var_pars_window, 0 );
                AWMIMT("arb_phyl", 	"Filter by Base Frequency ...",			"F", "phylo.hlp",	AWM_TREE2,(AW_CB)NT_system_cb,	(AW_CL)"arb_phylo &",	0 );

                GDE_load_menu(awm,"SAI");
            }
            awm->close_sub_menu();

            if (!GB_NOVICE){
                awm->insert_sub_menu(0, "Etc",	"E");
                {
                    AWMIMT("pos_var_dist",	"Positional Variability (Distance Method) ...",		"P","pos_variability.ps", AWM_EXP, AW_POPUP, (AW_CL)AP_open_cprofile_window, 0 );
                    AWMIMT("count_different_chars","Count Different Chars/Column ...",			"V","count_chars.hlp",	  AWM_EXP, NT_system_cb2, (AW_CL)"arb_count_chars", 0);
                    AWMIMT("export_pos_var",	"Export Column Statistic ( GNUPLOT format) ...",	"E","csp_2_gnuplot.hlp",  AWM_EXP, AW_POPUP, (AW_CL)AP_open_csp_2_gnuplot_window, 0 );
                }
                awm->close_sub_menu();
            }
        }
    } // clone

    // --------------------------------------------------------------------------------
    //     Tree
    // --------------------------------------------------------------------------------
    awm->create_menu( 0,   "Tree", "T", "nt_tree.hlp",  AWM_ALL );
    {
        if (!clone){
            AWMIMT(	"nds",		"NDS ( Select Node Information ) ...",		"N","props_nds.hlp",	AWM_ALL, AW_POPUP, (AW_CL)AWT_open_nds_window, (AW_CL)gb_main );
            AWMIMT(	"tree_select",	"Select ...",					"S",0,			AWM_ALL, AW_POPUP, (AW_CL)NT_open_select_tree_window,	(AW_CL)awar_tree );
        }
        AWMIMT("tree_select_latest","Select Latest",				"L",0,			AWM_TREE, 	(AW_CB)NT_select_last_tree, (AW_CL)awar_tree, 0 );
        if (!clone){
            awm->insert_separator();
            AWMIMT("tree_admin",	"Copy_Delete_Rename_Import_Export ...",		"C","treeadm.hlp",	AWM_TREE, 	AW_POPUP, (AW_CL)create_trees_window,	0 );
            AWMIMT("tree_2_xfig",	"Edit Tree View using XFIG ...",		"X","tree2file.hlp",	AWM_ALL,	AW_POPUP, (AW_CL)AWT_create_export_window,	(AW_CL)ntw );
            AWMIMT("tree_print",	"Print Tree View to Printer ...",		"P","tree2prt.hlp",	AWM_ALL,	(AW_CB)AWT_create_print_window, (AW_CL)ntw,	0 );
            awm->insert_separator();
        }
        awm->insert_sub_menu(0, "Collapse/Expand Tree",			"G");
        {
            AWMIMT("tree_group_all", 		"Group All",			"A","tgroupall.hlp",	AWM_ALL, 	(AW_CB)NT_group_tree_cb, 	(AW_CL)ntw, 0 );
            AWMIMT("tree_group_not_marked", 	"Group All Except Marked",	"M","tgroupnmrkd.hlp",	AWM_ALL, 	(AW_CB)NT_group_not_marked_cb,	(AW_CL)ntw, 0 );
            AWMIMT("tree_group_term_groups", 	"Group Terminal Groups",	"T","tgroupterm.hlp",	AWM_TREE, 	(AW_CB)NT_group_terminal_cb,	(AW_CL)ntw, 0 );
            AWMIMT("tree_ungroup_all",		"Ungroup All",			"U","tungroupall.hlp",	AWM_ALL,	(AW_CB)NT_ungroup_all_cb,	(AW_CL)ntw, 0 );
        }
        awm->close_sub_menu();

        awm->insert_sub_menu(0, "Beautify Tree",			"B");
        {
            AWMIMT("beautifyt_tree",		"#beautifyt.bitmap",		"B","resorttree.hlp",	AWM_TREE, 	(AW_CB)NT_resort_tree_cb,	(AW_CL)ntw, 0 );
            AWMIMT("beautifyc_tree",		"#beautifyc.bitmap",		"B","resorttree.hlp",	AWM_TREE, 	(AW_CB)NT_resort_tree_cb,	(AW_CL)ntw, 1 );
            AWMIMT("beautifyb_tree",		"#beautifyb.bitmap",		"B","resorttree.hlp",	AWM_TREE, 	(AW_CB)NT_resort_tree_cb,	(AW_CL)ntw, 2 );
        }
        awm->close_sub_menu();

        awm->insert_sub_menu(0, "Remove Species from Tree",		"R");
        {
            AWMIMT("tree_remove_deleted",	"Remove Zombies",		"Z","trm_del.hlp",	AWM_TREE, 	(AW_CB)NT_remove_leafs, 	(AW_CL)ntw, AWT_REMOVE_DELETED );
            AWMIMT("tree_remove_marked", 	"Remove Marked",		"M","trm_mrkd.hlp",	AWM_TREE, 	(AW_CB)NT_remove_leafs, 	(AW_CL)ntw, AWT_REMOVE_MARKED );
            AWMIMT("tree_keep_marked",		"Keep Marked",			"K","tkeep_mrkd.hlp",	AWM_TREE, 	(AW_CB)NT_remove_leafs, 	(AW_CL)ntw, AWT_REMOVE_NOT_MARKED|AWT_REMOVE_DELETED );
        }
        awm->close_sub_menu();
        AWMIMT("tree_remove_remark",	"Remove Bootstrap Values",	"V","trm_boot.hlp",	AWM_TREE, 	(AW_CB)NT_remove_bootstrap, 	(AW_CL)ntw, 0 );


        if (!clone){
            awm->insert_separator();
            awm->insert_sub_menu(0, "Add Species to Existing Tree",	"A");
            {
                AWMIMT( "arb_pars_quick",	"Quick Add Marked to a Tree using Parsimony ...",	"P", "pars.hlp",	AWM_TREE,	(AW_CB)NT_system_cb,	(AW_CL)"arb_pars -add_marked -quit &",0 );
                AWMIMT( "arb_pars",		"Parsimony interaktiv ...",				"I", "pars.hlp",	AWM_TREE,	(AW_CB)NT_system_cb,	(AW_CL)"arb_pars &",	0 );
                GDE_load_menu(awm,"Incremental_Phylogeny");
            }
            awm->close_sub_menu();
            awm->insert_sub_menu(0, "Build Tree From Sequence Data",	"D");
            {
                AWMIMT( "arb_dist",		"Neighbor Joining ...",		"N", "dist.hlp",	AWM_TREE,	(AW_CB)NT_system_cb,	(AW_CL)"arb_dist &",	0 );
                GDE_load_menu(awm,"Phylogeny");
            }
            awm->close_sub_menu();
        }
        awm->insert_separator();
        AWMIMT("transversion",		"Transversion Analysis ...",	"T","trans_anal.hlp",	AWM_TREE, 	(AW_CB)AW_POPUP_HELP, 	(AW_CL)"trans_anal.hlp", 0 );
        awm->insert_sub_menu(0, "Etc",	"E");
        {
            AWMIMT("mark_long_branches",	"Mark Long Branches",		"L","mark_long_branches.hlp",	AWM_EXP, 	(AW_CB)NT_mark_long_branches, 	(AW_CL)ntw, 0);
            AWMIMT("justify_branch_lengths",	"Beautify Branch_Lengths",	"R","beautify_bl.hlp",		AWM_EXP, 	(AW_CB)NT_justify_branch_lenghs, (AW_CL)ntw, 0);
        }
        awm->close_sub_menu();
    }

    if(!clone){

        // --------------------------------------------------------------------------------
        //     Properties
        // --------------------------------------------------------------------------------
        awm->create_menu(0,"Properties","r","properties.hlp", AWM_ALL);
        {
            AWMIMT("props_menu",	"Menu: Colors and Fonts ...",	"M","props_frame.hlp",		AWM_ALL, AW_POPUP, (AW_CL)AWT_preset_window, 0 );
            AWMIMT("props_tree",	"Tree: Colors and Fonts ...",	"C","nt_props_data.hlp",	AWM_ALL, AW_POPUP, (AW_CL)AW_create_gc_window, (AW_CL)aw_gc_manager );
            AWMIMT("props_tree2",	"Tree Settings ...",		"T","nt_tree_settings.hlp",	AWM_ALL, AW_POPUP, (AW_CL)NT_create_tree_setting, (AW_CL)ntw );
            AWMIMT("props_www",	"WWW  ...",			"W","props_www.hlp",		AWM_ALL, AW_POPUP, (AW_CL)AWT_open_www_window,  (AW_CL)gb_main );
            awm->insert_separator();
            AWMIMT("save_props",	"Save Properties (in ~/.arb_prop/ntree.arb)",	"S","savedef.hlp",AWM_ALL, (AW_CB) AW_save_defaults, 0, 0 );

            //	AWMIMT(0,"Tree: Setup ...",		"S","nt_props_tree.hlp",NT_F_ALL, AW_POPUP, (AW_CL)NT_preset_tree_window, 0);
        }

        // --------------------------------------------------------------------------------
        //     Etc
        // --------------------------------------------------------------------------------
        awm->create_menu(0,"Etc","E","nt_etc.hlp", AWM_ALL);
        {
            AWMIMT("reset_logical_zoom",	"Reset Logical Zoom",		"o","rst_log_zoom.hlp",		AWM_ALL, (AW_CB)NT_reset_lzoom_cb, (AW_CL)ntw, 0 );
            AWMIMT("reset_physical_zoom",	"Reset Physical Zoom",		"y","rst_phys_zoom.hlp",	AWM_ALL, (AW_CB)NT_reset_pzoom_cb, (AW_CL)ntw, 0 );
            awm->insert_separator();
            AWMIMT("table_aming",		"More Data Admin ...",		"M","tableadm.hlp",		AWM_ALL, AW_POPUP,(AW_CL)AWT_create_tables_admin_window, (AW_CL)gb_main);
            awm->insert_sub_menu(0,"Probe Functions",		"F");
            {
                AWMIMT( "probe_design",		"Probe Design ...", "D", "probedesign.hlp",			AWM_PRB, AW_POPUP, (AW_CL)create_probe_design_window, 0 );
                AWMIMT( "probe_match",		"Probe Match ...",  "M", "probematch.hlp",			AWM_PRB, AW_POPUP, (AW_CL)create_probe_match_window, 0 );
                AWMIMT( "probe_multi",		"Multi Probe ...", "P", "multiprobe.hlp",			AWM_PRB, AW_POPUP, (AW_CL)MP_main, (AW_CL)ntw );
                AWMIMT( "pt_server_admin",	"PT_SERVER Admin ...",  "A", "probeadmin.hlp",			AWM_ALL, AW_POPUP, (AW_CL)create_probe_admin_window, 0 );
                awm->insert_separator();
                AWMIMT("view_probe_group_result",	"View Probe Group Result ...",	"G","",  AWM_EXP, AW_POPUP, (AW_CL)create_probe_group_result_window, (AW_CL)ntw );
            }
            awm->close_sub_menu();
            awm->insert_sub_menu(0,"Primer Design ...","");
            {
                AWMIMT( "primer_design_new",		"Primer Design ...", "P", "primer_new.hlp",			AWM_PRB, AW_POPUP, (AW_CL)create_primer_design_window, 0 );
                AWMIMT("primer_design",		"Primer Design (old)",		"","primer.hlp",	AWM_EXP, (AW_CB)NT_primer_cb, 0, 0);
            }
            awm->close_sub_menu();

            awm->insert_sub_menu( 0,   "Network", "N");
            {
                AWMIMT("ors",	"ORS ...",		"O","ors.hlp",			AWM_ALL, (AW_CB)NT_system_cb, (AW_CL)"netscape http://pop.mikro.biologie.tu-muenchen.de/ORS/ &", 0);
                GDE_load_menu(awm,"user");
            }
            awm->close_sub_menu();

            AWMIMT("names_admin",		"Name Server Admin ...",	"S","namesadmin.hlp",	AWM_EXP, AW_POPUP, (AW_CL)create_awtc_names_admin_window, 0 );
            awm->insert_separator();
            awm->insert_sub_menu(0,"GDE Specials","G");
            {
                GDE_load_menu(awm,0,0);
            }
            awm->close_sub_menu();

            if (!GB_NOVICE){
                awm->insert_sub_menu(0,"W. Ludwig Specials","L");
                {
                    AWMIMT("chewck_gcg_list",	"Check GCG List ...",		"C","checkgcg.hlp",	AWM_TUM, AW_POPUP, (AW_CL)create_check_gcg_window, 0);
                }
                awm->close_sub_menu();
            }

            if (!GB_NOVICE) {
                awm->insert_separator();
                AWMIMT("xterm",			"XTERM ...",			"X",0	,		AWM_EXP, (AW_CB)GB_xterm, (AW_CL)0, 0 );
            }
            if (GBS_do_core()){
                AWMIMT("debug_arbdb",	"DEBUG ARBDB ...",		"D",0	,		AWM_ALL, (AW_CB)GB_print_debug_information, (AW_CL)gb_main, 0 );
                AWMIMT("test_compr",	"TEST COMPR ...",		"C",0	,		AWM_ALL, (AW_CB)GBT_compression_test, (AW_CL)gb_main, 0 );
            }
        }
    } // clone

    awm->insert_help_topic("help_how",		"How to use Help",		"H","help.hlp",			AWM_ALL, (AW_CB)AW_POPUP_HELP, (AW_CL)"help.hlp", 0);
    awm->insert_help_topic("help_arb",		"ARB Help",			"A","arb.hlp",			AWM_ALL, (AW_CB)AW_POPUP_HELP, (AW_CL)"arb.hlp", 0);
    awm->insert_help_topic("help_arb_nt",	"ARB_NT Help",			"N","arb_ntree.hlp",		AWM_ALL, (AW_CB)AW_POPUP_HELP, (AW_CL)"arb_ntree.hlp", 0);


    awm->create_mode( 0, "mark.bitmap", "mode_mark.hlp", AWM_ALL, (AW_CB)nt_mode_event,(AW_CL)ntw,(AW_CL)AWT_MODE_MARK);
    awm->create_mode( 0, "group.bitmap", "mode_group.hlp", AWM_ALL, (AW_CB)nt_mode_event,(AW_CL)ntw,(AW_CL)AWT_MODE_GROUP);
    awm->create_mode( 0, "zoom.bitmap", "mode_pzoom.hlp", AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw,(AW_CL)AWT_MODE_ZOOM);
    awm->create_mode( 0, "lzoom.bitmap", "mode_lzoom.hlp", AWM_EXP, (AW_CB)nt_mode_event, (AW_CL)ntw,(AW_CL)AWT_MODE_LZOOM);
    awm->create_mode( 0, "modify.bitmap", "mode_info.hlp", AWM_ALL, (AW_CB)NT_modify_cb, (AW_CL)ntw, (AW_CL)AWT_MODE_MOD);
    awm->create_mode( 0, "www_mode.bitmap", "mode_www.hlp", AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw,(AW_CL)AWT_MODE_WWW);

    awm->create_mode( 0, "line.bitmap", "mode_width.hlp", AWM_TREE, (AW_CB)nt_mode_event,
                      (AW_CL)ntw,
                      (AW_CL)AWT_MODE_LINE
                      );
    awm->create_mode( 0, "rot.bitmap", "mode_rotate.hlp", AWM_TREE, (AW_CB)nt_mode_event,
                      (AW_CL)ntw,
                      (AW_CL)AWT_MODE_ROT
                      );
    awm->create_mode( 0, "spread.bitmap", "mode_angle.hlp", AWM_TREE, (AW_CB)nt_mode_event,
                      (AW_CL)ntw,
                      (AW_CL)AWT_MODE_SPREAD
                      );
    awm->create_mode( 0, "swap.bitmap", "mode_swap.hlp", AWM_TREE, (AW_CB)nt_mode_event,
                      (AW_CL)ntw,
                      (AW_CL)AWT_MODE_SWAP
                      );
    awm->create_mode( 0, "length.bitmap", "mode_length.hlp", AWM_TREE, (AW_CB)nt_mode_event,
                      (AW_CL)ntw,
                      (AW_CL)AWT_MODE_LENGTH
                      );
    awm->create_mode( 0, "move.bitmap", "mode_move.hlp", AWM_TREE, (AW_CB)nt_mode_event,
                      (AW_CL)ntw,
                      (AW_CL)AWT_MODE_MOVE
                      );
    awm->create_mode( 0, "setroot.bitmap", "mode_set_root.hlp", AWM_TREE, (AW_CB)nt_mode_event,
                      (AW_CL)ntw,
                      (AW_CL)AWT_MODE_SETROOT
                      );
    awm->create_mode( 0, "reset.bitmap", "mode_reset.hlp", AWM_TREE, (AW_CB)nt_mode_event,
                      (AW_CL)ntw,
                      (AW_CL)AWT_MODE_RESET
                      );
    awm->create_mode( 0, "jm1.bitmap", "mad.hlp", AWM_ALL, (AW_CB)nt_mode_event,
                      (AW_CL)ntw,
                      (AW_CL)AWT_MODE_JUMP
                      );

    awm->set_info_area_height( 250 );
    awm->at(5,2);
    awm->auto_space(0,-5);
    awm->shadow_width(1);

    // path, tree, alignment & searchSpecies buttons:

    int first_liney;
    int db_pathx;
    awm->get_at_position( &db_pathx, &first_liney);
    awm->callback( AW_POPUP, (AW_CL)NT_create_save_quick_as, (AW_CL)"tmp/nt/arbdb");
    awm->button_length(20);
    awm->help_text("saveas.hlp");
    awm->create_button("SAVE_AS",AWAR_DB_PATH);

    int db_treex;
    awm->get_at_position( &db_treex, &first_liney);
    awm->callback((AW_CB2)AW_POPUP,(AW_CL)NT_open_select_tree_window,(AW_CL)awar_tree);
    awm->button_length(20);
    awm->help_text("nt_tree_select.hlp");
    awm->create_button("SELECT_A_TREE", awar_tree);

    int db_alignx;
    awm->get_at_position( &db_alignx, &first_liney);
    awm->callback(AW_POPUP,   (AW_CL)NT_open_select_alignment_window, 0 );
    awm->help_text("nt_align_select.hlp");
    awm->create_button("SELECT_AN_ALIGNMENT", AWAR_DEFAULT_ALIGNMENT);

    int db_searchx;
    awm->get_at_position(&db_searchx, &first_liney);
    awm->callback(AW_POPUP,   (AW_CL)ad_create_query_window, 0 );
    awm->help_text("sp_search.hlp");
    awm->create_button("SEARCH_SPECIES", AWAR_SPECIES_NAME);

    int behind_buttonsx; // after first row of buttons
    awm->get_at_position( &behind_buttonsx, &first_liney );

    awm->at_newline();
    int second_linex, second_liney;
    awm->get_at_position(&second_linex, &second_liney);
    second_liney += 5;
    awm->at_y(second_liney);

    // undo/redo:

    awm->auto_space(-2,-2);

    awm->button_length(0);
    awm->at_x(db_pathx);
    awm->callback(NT_undo_cb,(AW_CL)GB_UNDO_UNDO,(AW_CL)ntw);
    awm->help_text("undo.hlp");
    awm->create_button("UNDO", "#undo.bitmap",0);

    awm->callback(NT_undo_cb,(AW_CL)GB_UNDO_REDO,(AW_CL)ntw);
    awm->help_text("undo.hlp");
    awm->create_button("REDO", "#redo.bitmap",0);

    // tree buttons:

    awm->button_length(7);
    awm->at(db_treex, second_liney);
    awm->callback((AW_CB)NT_set_tree_style,(AW_CL)ntw,(AW_CL)AP_RADIAL_TREE);
    awm->help_text("tr_type_radial.hlp");
    awm->create_button("RADIAL_TREE_TYPE", "#radial.bitmap",0);

    awm->callback((AW_CB)NT_set_tree_style,(AW_CL)ntw,(AW_CL)AP_LIST_TREE);
    awm->help_text("tr_type_list.hlp");
    awm->create_button("LIST_TREE_TYPE", "#list.bitmap",0);

    awm->at_newline();
    int third_linex, third_liney;
    awm->get_at_position(&third_linex, &third_liney);

    awm->at(db_treex, third_liney);
    awm->callback((AW_CB)NT_set_tree_style,(AW_CL)ntw,(AW_CL)AP_IRS_TREE);
    awm->help_text("tr_type_nds.hlp");
    awm->create_button("NO_TREE_TYPE", "#list.bitmap",0);

    awm->callback((AW_CB)NT_set_tree_style,(AW_CL)ntw,(AW_CL)AP_NDS_TREE);
    awm->help_text("tr_type_nds.hlp");
    awm->create_button("NO_TREE_TYPE", "#ndstree.bitmap",0);

    awm->at_newline();
    awm->button_length(100);
    awm->create_button(0,"tmp/LeftFooter");
    awm->at_newline();
    int last_linex, last_liney;
    awm->get_at_position( &last_linex,&last_liney );

    // edit, jump & web buttons (+help buttons):

    awm->button_length(9);
    awm->at(db_alignx, second_liney);
    awm->callback((AW_CB)NT_start_editor_on_tree, (AW_CL)&(nt.tree->tree_root), 0);
    awm->help_text("arb_edit4.hlp");
    awm->create_button("EDIT_SEQUENCES", "#edit.bitmap",0);

    awm->at(db_searchx, second_liney);
    awm->callback((AW_CB)NT_jump_cb,(AW_CL)ntw,1);
    awm->help_text("tr_jump.hlp");
    awm->create_button("JUMP", "#pjump.bitmap",0);

    awm->callback((AW_CB1)awt_openURL,(AW_CL)gb_main);
    awm->help_text("www.hlp");
    awm->create_button("WWW", "#www.bitmap",0);

    // help:

    awm->at(behind_buttonsx, first_liney);
    awm->callback(AW_POPUP_HELP,(AW_CL)"arb_ntree.hlp");
    awm->button_length(0);
    awm->help_text("help.hlp");
    awm->create_button("HELP","HELP","H");

    awm->callback( AW_help_entry_pressed );
    awm->create_button("?","?");

    // protect:

    awm->at(behind_buttonsx, second_liney+10);

    if (!GB_NOVICE){
        awm->label("Protect");
        awm->create_option_menu(AWAR_SECURITY_LEVEL);
        awm->insert_option("0",0,0);
        awm->insert_option("1",0,1);
        awm->insert_option("2",0,2);
        awm->insert_option("3",0,3);
        awm->insert_option("4",0,4);
        awm->insert_option("5",0,5);
        awm->insert_default_option("6",0,6);
        awm->update_option_menu();
    }else{
        awm->get_root()->awar(AWAR_SECURITY_LEVEL)->write_int(6);
    }

    awm->set_info_area_height( last_liney+6 );
    awm->set_bottom_area_height( 0 );
    awm->get_root()->set_focus_callback((AW_RCB)NT_focus_cb,(AW_CL)gb_main,0);

    GB_pop_transaction(gb_main);

    return (AW_window *)awm;

}

// PJ vectorfont stuff
AW_window *NT_preset_tree_window( AW_root *root )  {

    AW_window_simple *aws = new AW_window_simple;
    const int       tabstop = 400;
    aws->init( root, "TREE_PROPS2", "Tree Options", 400, 300 );

    aws->label_length( 25 );
    aws->button_length( 20 );

    aws->at           ( 10,10 );
    aws->auto_space(10,10);
    aws->callback     ( AW_POPDOWN );
    aws->create_button( "CLOSE", "CLOSE", "C" );

    aws->at_newline();

    // No Help, as we usually do not know the associated help file from within presets of AW Lib

    aws->create_option_menu( "vectorfont/active", "Data font type", "1" );
    aws->insert_option        ( "Use vectorfont",     " ", (int) 1);
    aws->insert_option        ( "Use standard font",  " ", (int) 0);
    aws->update_option_menu();
    aws->at_newline();

    //              AW_preset_create_scale_chooser(aws,"vectorfont/userscale","VF: absolute scaling");
    aws->at_x(tabstop);
    aws->create_input_field( "vectorfont/userscale",6);
    aws->at_newline();

    //aws->callback( (AW_CB0) aw_xfig_font_create_filerequest);
    //aws->create_button( "VF: select", "V" );
    //aws->at_x(tabstop);
    //aws->create_input_field( "vectorfont/file_name",20);
    //aws->at_newline();

    aws->window_fit();
    return (AW_window *)aws;

}


