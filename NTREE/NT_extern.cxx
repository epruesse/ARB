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
#include "ad_spec.hxx"
#include <awt.hxx>
#include <awt_nds.hxx>
#include <awt_canvas.hxx>
#include <aw_preset.hxx>
#include <aw_global.hxx>
#include <awt_preset.hxx>
#include <awt_advice.hxx>
#include <awt_config_manager.hxx>
#include <awt_sel_boxes.hxx>
#include <awt_macro.hxx>

#include <AW_rename.hxx>
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
#include "ap_conservProfile2Gnuplot.hxx"
#include <awti_export.hxx>
#include "nt_join.hxx"
#include "nt_edconf.hxx"
#include "ap_pos_var_pars.hxx"
#include "nt_import.hxx"
#include "nt_date.h"
#include "nt_internal.h"
#include <st_window.hxx>
#include <probe_design.hxx>
#include <primer_design.hxx>
#include <GEN.hxx>
#include <EXP.hxx>
#include <awt_input_mask.hxx>

#include "nt_concatenate.hxx"
#include "seq_quality.h"

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define nt_assert(bed) arb_assert(bed)

void create_probe_design_variables(AW_root *aw_root,AW_default def,AW_default global);
void create_cprofile_var(AW_root *aw_root, AW_default aw_def);

void create_insertchar_variables(AW_root *root,AW_default db1);
AW_window *create_insertchar_window( AW_root *root, AW_default def);

AW_window *AP_open_cprofile_window( AW_root *root );

extern AW_window    *MP_main(AW_root *root, AW_default def);

AW_window *create_tree_window(AW_root *aw_root,AWT_graphic *awd);

#define F_ALL ((AW_active)-1)

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
    GB_transaction  dummy(gb_main);
    GBDATA         *gb_species;
    const char     *buf;
    AW_root        *aw_root = aww->get_root();
    char           *name    = aw_root->awar(AWAR_EXPORT_NDS"/file_name")->read_string();
    FILE           *out     = fopen(name,"w");

    if (!out) {
        delete name;
        aw_message("Error: Cannot open and write to file");
        return;
    }
    make_node_text_init(gb_main);
    int   tabbed    = aw_root->awar(AWAR_EXPORT_NDS"/tabbed")->read_int();
    char *tree_name = aw_root->awar(AWAR_TREE)->read_string();

    for (gb_species = GBT_first_marked_species(gb_main);
         gb_species;
         gb_species = GBT_next_marked_species(gb_species))
    {
        buf = make_node_text_nds(gb_main, gb_species,(tabbed ? 2 : 1),0, tree_name);
        fprintf(out,"%s\n",buf);
    }
    awt_refresh_selection_box(aw_root, AWAR_EXPORT_NDS);
    fclose(out);
    if (print_flag) GB_textprint(name);
    free(tree_name);
    free(name);
}

AW_window *create_nds_export_window(AW_root *root){
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "EXPORT_NDS_OF_MARKED", "EXPORT NDS OF MARKED SPECIES");
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

    aws->at("toggle1");
    aws->label("Use TABs for columns");
    aws->create_toggle(AWAR_EXPORT_NDS"/tabbed");

    awt_create_selection_box((AW_window *)aws,AWAR_EXPORT_NDS);

    return (AW_window *)aws;
}

void create_export_nds_awars(AW_root *awr, AW_default def)
{
    aw_create_selection_box_awars(awr, AWAR_EXPORT_NDS, "", ".nds", "export.nds", def);
    awr->awar_int( AWAR_EXPORT_NDS"/tabbed", 0, def);
}

static void AWAR_SEARCH_BUTTON_TEXT_change_cb(AW_root *awr) {
    char *value  = awr->awar(AWAR_SPECIES_NAME)->read_string();

    if (strcmp(value,"")==0) {
        awr->awar(AWAR_SEARCH_BUTTON_TEXT)->write_string("Species Information");
    }
    else {
        awr->awar(AWAR_SEARCH_BUTTON_TEXT)->write_string(value);
    }

    free(value);
}

void create_all_awars(AW_root *awr, AW_default def)
{
    GB_transaction dummy(gb_main);
    awr->awar_string( AWAR_FOOTER, "", def);

    if (GB_read_clients(gb_main)>=0){
        awr->awar_string( AWAR_TREE, "tree_main", gb_main);
    }else{
        awr->awar_string( AWAR_TREE, "tree_main", def);
    }

    awr->awar_string( AWAR_SPECIES_NAME, "" ,   gb_main);
    awr->awar_string( AWAR_SAI_NAME, "" ,   gb_main);
    awr->awar_string( AWAR_SAI_GLOBAL, "" ,   gb_main);
    awr->awar_string( AWAR_MARKED_SPECIES_COUNTER, "unknown" ,  gb_main);
    awr->awar_string( AWAR_SEARCH_BUTTON_TEXT, "Species Information" ,  gb_main);
    awr->awar(AWAR_SPECIES_NAME)->add_callback(AWAR_SEARCH_BUTTON_TEXT_change_cb);
    awr->awar_int( AWAR_NTREE_TITLE_MODE, 1);

    awr->awar_string(AWAR_SAI_COLOR_STR, "", gb_main); //sai visualization in probe match

    GEN_create_awars(awr, def);
    EXP_create_awars(awr, def);
    AWT_create_db_browser_awars(awr, def);

    awr->awar_int( AWAR_SECURITY_LEVEL, 0, def);
    awr->awar(AWAR_SECURITY_LEVEL)->add_callback(nt_changesecurity);
#if defined(DEBUG) && 0
    awr->awar(AWAR_SECURITY_LEVEL)->write_int(6); // no security for debugging..
#endif // DEBUG

    create_insertchar_variables(awr,def);
    create_probe_design_variables(awr,def,gb_main);
    create_primer_design_variables(awr,def, gb_main);
    create_trees_var(awr,def);
    NT_create_extendeds_var(awr,def);
    create_species_var(awr,def);
    create_consensus_var(awr,def);
    create_gde_var(awr,def);
    create_cprofile_var(awr,def);
    NT_create_transpro_variables(awr,def);
    NT_build_resort_awars(awr,def);

    NT_create_alignment_vars(awr,def);
    create_nds_vars(awr,def,gb_main);
    create_export_nds_awars(awr,def);
    AWTC_create_rename_variables(awr,def);
    create_check_gcg_awars(awr,def);
    awt_create_dtree_awars(awr,gb_main);

    awr->awar_string( AWAR_ERROR_MESSAGES, "", gb_main);
    awr->awar_string( AWAR_DB_COMMENT, "<no description>", gb_main);

    AWTC_create_submission_variables(awr, gb_main);
    NT_createConcatenationAwars(awr,def);
    NT_createValidNamesAwars(awr,def); // lothar
    SQ_create_awars(awr, def);

    ARB_init_global_awars(awr, def, gb_main);
    awt_create_aww_vars(awr,def);
    NT_create_MAUS_awars(awr, def, gb_main);
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
    else awt_refresh_selection_box(aww->get_root(), "tmp/nt/arbdb");
}


void
NT_save_quick_cb(AW_window *aww)
{
    char *filename = aww->get_root()->awar(AWAR_DB_PATH)->read_string();
    GB_ERROR error = GB_save_quick(gb_main,filename);
    free(filename);
    if (error) aw_message(error);
    else awt_refresh_selection_box(aww->get_root(), "tmp/nt/arbdb");
}

void
NT_save_quick_as_cb(AW_window *aww)
{
    char *filename = aww->get_root()->awar(AWAR_DB_PATH)->read_string();
    GB_ERROR error = GB_save_quick_as(gb_main, filename);
    free(filename);
    if (error) {
        aw_message(error);
    }else{
        awt_refresh_selection_box(aww->get_root(), "tmp/nt/arbdb");
        aww->hide();
    }
}


AW_window *NT_create_save_quick_as(AW_root *aw_root, char *base_name)
{
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    aws = new AW_window_simple;
    aws->init( aw_root, "SAVE_CHANGES_TO", "SAVE CHANGES TO");
    aws->load_xfig("save_as.fig");

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->callback( AW_POPUP_HELP, (AW_CL)"save.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","H");

    awt_create_selection_box((AW_window *)aws,base_name);

    aws->at("comment");
    aws->create_text_field(AWAR_DB_COMMENT);

    aws->at("save");aws->callback(NT_save_quick_as_cb);
    aws->create_button("SAVE","SAVE","S");

    return (AW_window *)aws;
}

void NT_database_optimization(AW_window *aww){
    GB_ERROR error = 0;
    GB_push_my_security(gb_main);

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
        free(tree_name);
        aw_closestatus();
    }

    int errors = 0;
    if (error) {
        errors++;
        aw_message(error);
    }

    aw_openstatus("(Re-)Compressing database, Please Wait");
    error = GB_optimize(gb_main);
    aw_closestatus();

    GB_pop_my_security(gb_main);

    if (error) {
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
    free(largest_tree);

    aws = new AW_window_simple;
    aws->init( aw_root, "OPTIMIZE_DATABASE", "OPTIMIZE DATABASE");
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

    free(filename);
    free(filetype);

    if (error) {
        aw_message(error);
    }else{
        aww->hide();
        awt_refresh_selection_box(aww->get_root(), "tmp/nt/arbdb");
    }
}


AW_window *NT_create_save_as(AW_root *aw_root,const char *base_name)
{
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    aws = new AW_window_simple;
    aws->init( aw_root, "SAVE_DB", "SAVE ARB DB" );
    aws->load_xfig("save_as.fig");

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->callback( AW_POPUP_HELP, (AW_CL)"save.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","H");

    awt_create_selection_box((AW_window *)aws,base_name);

    aws->at("type");
    aws->label("Type ");
    aws->create_option_menu(AWAR_DB"type");
    aws->insert_option("Binary","B","b");
    aws->insert_option("Bin (with FastLoad File)","f","bm");
    aws->insert_default_option("Ascii","A","a");
    aws->update_option_menu();

    aws->at("optimize");
    aws->callback(AW_POPUP,(AW_CL)NT_create_database_optimization_window,0);
    aws->help_text("optimize.hlp");
    aws->create_button("OPTIMIZE","OPTIMIZE");

    aws->at("save");aws->callback(NT_save_as_cb);
    aws->create_button("SAVE","SAVE","S");

    aws->at("comment");
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

static AWT_config_mapping_def tree_setting_config_mapping[] = {
    { AWAR_DTREE_BASELINEWIDTH,   "line_width" },      
    { AWAR_DTREE_VERICAL_DIST,    "vert_dist" },       
    { AWAR_DTREE_AUTO_JUMP,       "auto_jump" },       
    { AWAR_DTREE_SHOW_CIRCLE,     "show_circle" },     
    { AWAR_DTREE_USE_ELLIPSE,     "use_ellipse" },     
    { AWAR_DTREE_CIRCLE_ZOOM,     "circle_zoom" },     
    { AWAR_DTREE_CIRCLE_MAX_SIZE, "circle_max_size" }, 
    { AWAR_DTREE_GREY_LEVEL,      "grey_level" },      
    { 0, 0 }
};

static char *tree_setting_store_config(AW_window *aww, AW_CL , AW_CL ) {
    AWT_config_definition cdef(aww->get_root(), tree_setting_config_mapping);
    return cdef.read();
}
static void tree_setting_restore_config(AW_window *aww, const char *stored_string, AW_CL , AW_CL ) {
    AWT_config_definition cdef(aww->get_root(), tree_setting_config_mapping);
    cdef.write(stored_string);
}

AW_window *NT_create_tree_setting(AW_root *aw_root)
{
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    aws = new AW_window_simple;
    aws->init( aw_root, "TREE_PROPS", "TREE SETTINGS");
    aws->load_xfig("awt/tree_settings.fig");

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"nt_tree_settings.hlp");
    aws->create_button("HELP","HELP","H");

    aws->at("button");
    aws->auto_space(10,10);
    aws->label_length(30);

    aws->label("Base line width");
    aws->create_input_field(AWAR_DTREE_BASELINEWIDTH,4);
    aws->at_newline();

    aws->label("Relative vertical distance");
    aws->create_input_field(AWAR_DTREE_VERICAL_DIST,4);
    aws->at_newline();

    aws->label("Auto Jump");
    aws->create_toggle(AWAR_DTREE_AUTO_JUMP);
    aws->at_newline();

    aws->label("Show Bootstrap Circles");
    aws->create_toggle(AWAR_DTREE_SHOW_CIRCLE);
    aws->at_newline();

    aws->label("Use ellipses");
    aws->create_toggle(AWAR_DTREE_USE_ELLIPSE);
    aws->at_newline();

    aws->label("Bootstrap circle zoom factor");
    aws->create_input_field(AWAR_DTREE_CIRCLE_ZOOM, 4);
    aws->at_newline();

    aws->label("Boostrap radius limit");
    aws->create_input_field(AWAR_DTREE_CIRCLE_MAX_SIZE, 4);
    aws->at_newline();

    aws->label("Grey Level of Groups%");
    aws->create_input_field(AWAR_DTREE_GREY_LEVEL,4);
    aws->at_newline();

    aws->at("config");
    AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "tree_settings", tree_setting_store_config, tree_setting_restore_config, 0, 0);

    return (AW_window *)aws;

}

void NT_submit_mail(AW_window *aww, AW_CL cl_awar_base) {
    char *awar_base = (char*)cl_awar_base;
    char *address   = aww->get_root()->awar(GBS_global_string("%s/address", awar_base))->read_string();
    char *text      = aww->get_root()->awar(GBS_global_string("%s/text", awar_base))->read_string();
    char *plainaddress = GBS_string_eval(address,"\"=:'=\\=",0); // Remove all dangerous symbols

    char buffer[256];
    sprintf(buffer,"/tmp/arb_bugreport_%s",GB_getenvUSER());

    FILE *mail = fopen(buffer,"w");
    if (!mail){
        aw_message(GB_export_error("Cannot write file %s",buffer));
    }
    else{
        fprintf(mail,"%s\n",text);
        fprintf(mail,"------------------------------\n");
        fprintf(mail,"VERSION       :" DATE "\n");
        fprintf(mail,"SYSTEMINFO    :\n");
        fclose(mail);

        system(GBS_global_string("uname -a >>%s",buffer));
        system(GBS_global_string("date  >>%s",buffer));
        const char *command = GBS_global_string("mail '%s' <%s",plainaddress,buffer);
        printf("%s\n",command);
        system(command);
        // GB_unlink(buffer);
        aww->hide();
    }

    free(plainaddress);
    free(text);
    free(address);
}

AW_window *NT_submit_bug(AW_root *aw_root, int bug_report){
    static AW_window_simple *awss[2] = { 0, 0 };
    if (awss[bug_report]) return (AW_window *)awss[bug_report];

    AW_window_simple *aws = new AW_window_simple;
    if (bug_report) {
        aws->init( aw_root, "SUBMIT_BUG", "Submit a bug");
    }
    else {
        aws->init( aw_root, "SUBMIT_REG", "Submit registration");
    }
    aws->load_xfig("bug_report.fig");

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"registration.hlp");
    aws->create_button("HELP","HELP","H");

    aws->at("what");
    aws->create_autosize_button("WHAT", (bug_report ? "Bug report" : "ARB Registration"));

    char *awar_name_start = GBS_global_string_copy("/tmp/nt/feedback/%s", bug_report ? "bugreport" : "registration");

    {
        const char *awar_name_address = GBS_global_string("%s/address", awar_name_start);
        aw_root->awar_string(awar_name_address, "arb@arb-home.de");

        aws->at("mail");
        aws->create_input_field(awar_name_address);
    }

    {
        const char *awar_name_text = GBS_global_string("%s/text", awar_name_start);

        if (bug_report){
            aw_root->awar_string(awar_name_text,
                                 "Bug occurred in: [which part of ARB?]\n"
                                 "The bug [ ] is reproducable\n"
                                 "        [ ] occurs randomly\n"
                                 "        [ ] occurs with specific data\n"
                                 "\n"
                                 "Detailed description:\n"
                                 "\n"
                                 );
        }
        else {
            aw_root->awar_string(awar_name_text,
                                 GBS_global_string("******* Registration *******\n"
                                                   "\n"
                                                   "Name           : %s\n"
                                                   "Department     :\n"
                                                   "How many users :\n"
                                                   "\n"
                                                   "Why do you want to use arb ?\n"
                                                   "\n",
                                                   GB_getenvUSER())
                                 );
        }

        aws->at("box");
        aws->create_text_field(awar_name_text);
    }

    aws->at("go");
    aws->callback(NT_submit_mail, (AW_CL)awar_name_start); // do not free awar_name_start
    aws->create_button("SEND","SEND");

    awss[bug_report] = aws; // store for further use

    return aws;
}

void NT_focus_cb(AW_window *aww)
{
    AWUSE(aww);
    GB_transaction dummy(gb_main);
}


void NT_modify_cb(AW_window *aww,AW_CL cd1,AW_CL cd2)
{
    AW_window *aws = NT_create_species_window(aww->get_root());
    aws->show();
    nt_mode_event(aww,(AWT_canvas*)cd1,(AWT_COMMAND_MODE)cd2);
}

void NT_primer_cb(void) {
    GB_xcmd("arb_primer",GB_TRUE, GB_FALSE);
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
    if (!val) return;   // operation cancelled
    NT_mark_all_cb(aww,(AW_CL)ntw, (AW_CL)0);
    AWT_TREE(ntw)->tree_root->mark_long_branches(gb_main,atof(val));
    AWT_TREE(ntw)->tree_root->compute_tree(ntw->gb_main);
    free(val);
    ntw->refresh();
}

void NT_mark_duplicates(AW_window *aww, AW_CL ntwcl){
    GB_transaction dummy(gb_main);
    AWT_canvas *ntw = (AWT_canvas *)ntwcl;
    NT_mark_all_cb(aww,(AW_CL)ntw, (AW_CL)0);
    AWT_TREE(ntw)->tree_root->mark_duplicates(gb_main);
    AWT_TREE(ntw)->tree_root->compute_tree(ntw->gb_main);
    ntw->refresh();
}

void NT_justify_branch_lenghs(AW_window *, AW_CL cl_ntw, AW_CL){
    GB_transaction dummy(gb_main);
    AWT_canvas *ntw = (AWT_canvas *)cl_ntw;
    if (AWT_TREE(ntw)->tree_root){
        AWT_TREE(ntw)->tree_root->justify_branch_lenghs(gb_main);
        AWT_TREE(ntw)->tree_root->compute_tree(ntw->gb_main);
        AWT_TREE(ntw)->save(ntw->gb_main,0,0,0);
        ntw->refresh();
    }
}

void NT_fix_database(AW_window *) {
    GB_ERROR err = 0;
    err = GB_fix_database(gb_main);
    if (err) aw_message(err);
}

static void relink_pseudo_species_to_organisms(GBDATA *&ref_gb_node, char *&ref_name, GB_HASH *organism_hash) {
    if (ref_gb_node) {
        if (GEN_is_pseudo_gene_species(ref_gb_node)) {
            GBDATA *gb_organism = GEN_find_origin_organism(ref_gb_node, organism_hash);
            if (gb_organism) {
                GBDATA *gb_name = GB_find(gb_organism, "name", 0, down_level);
                if (gb_name) {
                    char *name  = GB_read_string(gb_name);
                    free(ref_name);
                    ref_name    = name;
                    ref_gb_node = gb_organism;
                }
            }
        }
    }
}

void NT_pseudo_species_to_organism(AW_window *, AW_CL ntwcl){
    GB_transaction  dummy(gb_main);
    AWT_canvas     *ntw = (AWT_canvas *)ntwcl;
    if (AWT_TREE(ntw)->tree_root){
        GB_HASH *organism_hash = GEN_create_organism_hash(gb_main);
        AWT_TREE(ntw)->tree_root->relink_tree(ntw->gb_main, relink_pseudo_species_to_organisms, organism_hash);
        AWT_TREE(ntw)->tree_root->compute_tree(ntw->gb_main);
        AWT_TREE(ntw)->save(ntw->gb_main,0,0,0);
        ntw->refresh();
        GBS_free_hash(organism_hash);
    }
}


// #########################################
// #########################################
// ###                                   ###
// ##          user mask section          ##
// ###                                   ###
// #########################################
// #########################################

//  ----------------------------------------------------------------------------
//      class nt_item_type_species_selector : public awt_item_type_selector
//  ----------------------------------------------------------------------------
class nt_item_type_species_selector : public awt_item_type_selector {
public:
    nt_item_type_species_selector() : awt_item_type_selector(AWT_IT_SPECIES) {}
    virtual ~nt_item_type_species_selector() {}

    virtual const char *get_self_awar() const {
        return AWAR_SPECIES_NAME;
    }
    virtual size_t get_self_awar_content_length() const {
        return 12; // should be enough for "nnaammee.999"
    }
    virtual void add_awar_callbacks(AW_root *root, void (*f)(AW_root*, AW_CL), AW_CL cl_mask) const { // add callbacks to awars
        root->awar(get_self_awar())->add_callback(f, cl_mask);
    }
    virtual void remove_awar_callbacks(AW_root *root, void (*f)(AW_root*, AW_CL), AW_CL cl_mask) const { // remove callbacks to awars
        root->awar(get_self_awar())->remove_callback(f, cl_mask);
    }
    virtual GBDATA *current(AW_root *root) const { // give the current item
        char           *species_name = root->awar(get_self_awar())->read_string();
        GBDATA         *gb_species   = 0;

        if (species_name[0]) {
            GB_transaction dummy(gb_main);
            gb_species = GBT_find_species(gb_main,species_name);
        }

        free(species_name);
        return gb_species;
    }
    virtual const char *getKeyPath() const { // give the keypath for items
        return CHANGE_KEY_PATH;
    }
};

static nt_item_type_species_selector item_type_species;

//  ----------------------------------------------------------------------------
//      static void NT_open_mask_window(AW_window *aww, AW_CL cl_id, AW_CL)
//  ----------------------------------------------------------------------------
static void NT_open_mask_window(AW_window *aww, AW_CL cl_id, AW_CL) {
    int                              id         = int(cl_id);
    const awt_input_mask_descriptor *descriptor = AWT_look_input_mask(id);
    nt_assert(descriptor);
    if (descriptor) AWT_initialize_input_mask(aww->get_root(), gb_main, &item_type_species, descriptor->get_internal_maskname(), descriptor->is_local_mask());
}

//  ----------------------------------------------------------------------
//      static void NT_create_mask_submenu(AW_window_menu_modes *awm)
//  ----------------------------------------------------------------------
static void NT_create_mask_submenu(AW_window_menu_modes *awm) {
    AWT_create_mask_submenu(awm, AWT_IT_SPECIES, NT_open_mask_window);
}
//  -----------------------------------------------
//      void NT_test_input_mask(AW_root *root)
//  -----------------------------------------------
void NT_test_input_mask(AW_root *root) {
    AWT_initialize_input_mask(root, gb_main, &item_type_species, "test.mask", true);
}
// -------------------------------------------------------------------------------
//      static AW_window *NT_create_species_colorize_window(AW_root *aw_root)
// -------------------------------------------------------------------------------
static AW_window *NT_create_species_colorize_window(AW_root *aw_root) {
    return awt_create_item_colorizer(aw_root, gb_main, &AWT_species_selector);
}

// -------------------------------------------------------------------
//      void NT_update_marked_counter(AW_window *aww, long count)
// -------------------------------------------------------------------
void NT_update_marked_counter(AW_window *aww, long count) {
    const char *buffer = count ? GBS_global_string("%li marked", count) : "";
    aww->get_root()->awar(AWAR_MARKED_SPECIES_COUNTER)->write_string(buffer);
}

// -----------------------------------------------------
//      static void nt_count_marked(AW_window *aww)
// -----------------------------------------------------
static void nt_count_marked(AW_window *aww) {
#if defined(DEBUG)
//     printf("Counting marked species\n");
#endif // DEBUG
    long count = GBT_count_marked_species(gb_main);
    NT_update_marked_counter(aww, count);
}

// -------------------------------------------------------------------------------------
//      static void nt_auto_count_marked_species(GBDATA*, int* cl_aww, GB_CB_TYPE )
// -------------------------------------------------------------------------------------
static void nt_auto_count_marked_species(GBDATA*, int* cl_aww, GB_CB_TYPE ) {
    nt_count_marked((AW_window*)cl_aww);
}

// used to avoid that the species info window is stored in a menu (or with a button)
void NT_popup_species_window(AW_window *aww, AW_CL, AW_CL) {
    AW_window *aws = NT_create_species_window(aww->get_root());
    aws->show();
}

//--------------------------------------- to increase the area of display --------------------
static int windowHeight = 0;
static void title_mode_changed(AW_root *aw_root, AW_window *aww)
{
    int title_mode = aw_root->awar(AWAR_NTREE_TITLE_MODE)->read_int();

    if (title_mode==0) {
        aww->set_info_area_height(25);
    }
    else {
        aww->set_info_area_height(windowHeight);
    }
}
//--------------------------------------------------------------------------------------------------
#if defined(DEBUG)
void NT_rename_test(AW_window *, AW_CL cl_gb_main, AW_CL) {
    GBDATA   *gb_main = (GBDATA*)cl_gb_main;
    GB_begin_transaction(gb_main);
    GBDATA   *gbd     = GBT_find_species(gb_main, "MssVanni");
    GB_ERROR  error   = 0;

    if (gbd) {
        GBDATA *gb_remark = GB_find(gbd, "ali_16s", 0, down_level);
        if (gb_remark) {
            if (GB_rename(gb_remark, "ali_16s_new") != 0) {
                error = GB_get_error();
            }
        }
        else {
            error = "MssVanni has no entry 'ali_16s'";
        }
    }
    else {
        error = "MssVanni not found";
    }

    if (error) {
        GB_abort_transaction(gb_main);
        aw_message(error);
    }
    else {
        GB_commit_transaction(gb_main);
    }


}

#if defined(DEVEL_RALF)

void NT_test_AWT(AW_window *aww) {
    AW_root          *root = aww->get_root();
    AW_window_simple *aws  = new AW_window_simple;

    aws->init( root, "AWT_TEST", "AWT test");

    aws->load_xfig("awt_test.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->auto_space(0, 0);

    aws->at("buttons");

#if 0
    aws->label("Test label");
    aws->callback( AW_POPUP_HELP, (AW_CL)"button1.hlp");
    aws->create_button("B1","Button 1","1");

    aws->callback( AW_POPUP_HELP, (AW_CL)"button2.hlp");
    aws->create_button("B2","Button 2","2");

    aws->at_newline(); // ------------------------------

    aws->label("Longer test label");
    aws->callback( AW_POPUP_HELP, (AW_CL)"button1.hlp");
    aws->create_button("B1","Button 1","1");

    aws->callback( AW_POPUP_HELP, (AW_CL)"button2.hlp");
    aws->create_button("B2","Button 2","2");

    aws->at_newline();          // ------------------------------
#endif

    aws->callback( AW_POPUP_HELP, (AW_CL)"button1.hlp");
    aws->create_button("B1","Button 1","1");

    aws->label("Label with 2nd button");
    aws->highlight();
    aws->callback( AW_POPUP_HELP, (AW_CL)"button2.hlp");
    aws->create_button("B2","Button 2","2");

    aws->callback( AW_POPUP_HELP, (AW_CL)"button3.hlp");
    aws->create_button("B3","Button 3","3");

    aws->at_newline();          // ------------------------------

#if 0

    aws->callback( AW_POPUP_HELP, (AW_CL)"button1.hlp");
    aws->create_button("B1","Button 1","1");

    aws->callback( AW_POPUP_HELP, (AW_CL)"button2.hlp");
    aws->create_button("B2","Button 2","2");

#endif

    aws->at("buttons2");

    aws->label("Test label");
    aws->highlight();
    aws->callback( AW_POPUP_HELP, (AW_CL)"button1.hlp");
    aws->create_button("B1","buttons2","1");

    aws->at("buttons3");

    aws->label("Test label");
    aws->callback( AW_POPUP_HELP, (AW_CL)"button1.hlp");
    aws->create_button("B1","buttons3","1");

    aws->at("buttons4");

    aws->label("Test label");
    aws->callback( AW_POPUP_HELP, (AW_CL)"button1.hlp");
    aws->create_button("B1","buttons4","1");

    aws->at("buttons5");

    aws->label("Test label");
    aws->highlight();
    aws->callback( AW_POPUP_HELP, (AW_CL)"button1.hlp");
    aws->create_button("B1","buttons5","1");

    aws->at("buttons6");

    aws->label("Test label");
    aws->callback( AW_POPUP_HELP, (AW_CL)"button1.hlp");
    aws->create_button("B1","buttons6","1");

    aws->at("buttons7");

    aws->label("Test label");
    aws->highlight();
    aws->callback( AW_POPUP_HELP, (AW_CL)"button1.hlp");
    aws->create_button("B1","buttons7","1");

    aws->show();
}
#endif // DEVEL_RALF


void NT_dump_gcs(AW_window *aww, AW_CL, AW_CL) {
    for (int gc = int(AWT_GC_CURSOR); gc <= AWT_GC_MAX;  ++gc) {
        int   r, g, b;
        const char *err = aww->GC_to_RGB(aww->get_device(AW_MIDDLE_AREA), gc, r, g, b);
        if (err) {
            printf("Error retrieving RGB values for GC #%i: %s\n", gc, err);
        }
        else {
            printf("GC #%i RGB values: r=%i g=%i b=%i\n", gc, r, g, b);
        }
    }
}

#endif // DEBUG

//--------------------------------------------------------------------------------------------------

static GBT_TREE *fixDeletedSon(GBT_TREE *tree) {
    GBT_TREE *delNode = tree;

    if (delNode->leftson) {
        nt_assert(!delNode->rightson);
        tree             = delNode->leftson;
        delNode->leftson = 0;
    }
    else {
        nt_assert(!delNode->leftson);
        tree             = delNode->rightson;
        delNode->rightson = 0;
    }
    tree->father = delNode->father;
    if (delNode->remark_branch && !tree->remark_branch) { // rescue remarks if easily possible
        tree->remark_branch    = delNode->remark_branch;
        delNode->remark_branch = 0;
    }
    delNode->is_leaf = GB_TRUE; // don't try recursive delete
    if (!delNode->father) {
        delNode->father = delNode; // hack: avoid freeing memory (if tree_is_one_piece_of_memory == 1)
    }
    GBT_delete_tree(delNode);
    return tree;
}

static GBT_TREE *remove_leafs(GBT_TREE *tree, long mode, GB_HASH *species_hash, int& removed) {
    if (tree->is_leaf) {
        if (tree->name) {
            GBDATA *gb_node    = (GBDATA*)GBS_read_hash(species_hash, tree->name);
            bool    deleteSelf = false;
            if (gb_node) {
                if (mode & AWT_REMOVE_MARKED) {
                    if (GB_read_flag(gb_node)) deleteSelf = true;
                }
            }
            else { // zombie
                if (mode & AWT_REMOVE_DELETED) deleteSelf = true;
            }

            if (deleteSelf) {
                GBT_delete_tree(tree);
                removed++;
                tree = 0;
            }
        }
    }
    else {
        tree->leftson  = remove_leafs(tree->leftson, mode, species_hash, removed);
        tree->rightson = remove_leafs(tree->leftson, mode, species_hash, removed);

        if (tree->leftson) {
            if (tree->rightson) { // no son deleted
                
            }
            else { // right son deleted
                tree = fixDeletedSon(tree);
            }
        }
        else if (tree->rightson) { // left son deleted
            tree = fixDeletedSon(tree);
        }
        else {                  // everything deleted -> delete self
            tree->is_leaf = GB_TRUE;
            GBT_delete_tree(tree);
            tree          = 0;
        }
    }

    return tree;
}

void NT_alltree_remove_leafs(AW_window *, AW_CL cl_mode, AW_CL cl_gb_main) {
    GBDATA *gb_main = (GBDATA*)cl_gb_main;
    long    mode    = (long)cl_mode;

    GB_transaction ta(gb_main);

    int    treeCount;
    char **tree_names = GBT_get_tree_names_and_count(gb_main, &treeCount);

#if defined(DEVEL_RALF)
#warning test NT_alltree_remove_leafs. Crashes!
#endif // DEVEL_RALF

    if (tree_names) {
        aw_openstatus("Deleting from trees");
        GB_HASH *species_hash = GBT_generate_species_hash(gb_main, 1);

        for (int t = 0; t<treeCount; t++) {
            GB_ERROR  error = 0;
            aw_status(t/double(treeCount));
            GBT_TREE *tree  = GBT_read_tree(gb_main, tree_names[t], -sizeof(GBT_TREE));
            if (!tree) {
                aw_message(GBS_global_string("Can't load tree '%s' - skipped", tree_names[t]));
            }
            else {
                int removed = 0;
                tree        = remove_leafs(tree, mode, species_hash, removed);

                if (!tree) {
                    aw_message(GBS_global_string("'%s' would disappear. Please delete tree manually.", tree_names[t]));
                }
                else {
                    if (removed>0) {
                        error = GBT_write_tree(gb_main, 0, tree_names[t], tree);
                        aw_message(GBS_global_string("Removed %i species from '%s'", removed, tree_names[t]));
                    }
                    GBT_delete_tree(tree);
                }
            }
            if (error) aw_message(error);
        }
        aw_status(1.0);

        GBS_free_names(tree_names);
        GBS_free_hash(species_hash);
        aw_closestatus();
    }
}

//--------------------------------------------------------------------------------------------------

// ##########################################
// ##########################################
// ###                                    ###
// ##          create main window          ##
// ###                                    ###
// ##########################################
// ##########################################

#define AWMIMT awm->insert_menu_topic
#define SEP________________________SEP() awm->insert_separator()

//  --------------------------------------------------------------------
//      AW_window * create_nt_main_window(AW_root *awr, AW_CL clone)
//  --------------------------------------------------------------------
AW_window * create_nt_main_window(AW_root *awr, AW_CL clone){
    GB_push_transaction(gb_main);
    AW_gc_manager  aw_gc_manager;
    char          *awar_tree;
    char           window_title[256];
    awar_tree = (char *)GB_calloc(sizeof(char),strlen(AWAR_TREE) + 10); // do not free this

    if (clone){
        sprintf(awar_tree,AWAR_TREE "_%li", clone);
        sprintf(window_title,"ARB_NT_%li",clone);
    }else{
        sprintf(awar_tree,AWAR_TREE);
        sprintf(window_title,"ARB_NT");
    }
    AW_window_menu_modes *awm = new AW_window_menu_modes();
    awm->init(awr,window_title, window_title, 0,0);

    awm->button_length(5);

    if (!clone) AW_init_color_group_defaults("arb_ntree");

    nt.tree = (AWT_graphic_tree*)NT_generate_tree(awr,gb_main);

    AWT_canvas *ntw;
    {
        AP_tree_sort  old_sort_type = nt.tree->tree_sort;
        nt.tree->set_tree_type(AP_LIST_SIMPLE); // avoid NDS warnings during startup
        ntw = new AWT_canvas(gb_main,(AW_window *)awm,nt.tree, aw_gc_manager,awar_tree) ;
        nt.tree->set_tree_type(old_sort_type);
        ntw->set_mode(AWT_MODE_SELECT);
    }

    {
        char *tree_name          = awr->awar_string(awar_tree)->read_string();
        char *existing_tree_name = GBT_existing_tree(gb_main,tree_name);

        if (existing_tree_name) {
            awr->awar(awar_tree)->write_string(existing_tree_name);
            NT_reload_tree_event(awr,ntw,GB_FALSE); // load first tree !!!!!!!
        }else{
            AWT_advice("Your database contains no tree.", AWT_ADVICE_TOGGLE|AWT_ADVICE_HELP, 0, "no_tree.hlp");
        }

        awr->awar( awar_tree)->add_callback( (AW_RCB)NT_reload_tree_event, (AW_CL)ntw,(AW_CL)GB_FALSE);

        free(existing_tree_name);
        free(tree_name);
    }

    awr->awar( AWAR_SPECIES_NAME)->add_callback( (AW_RCB)NT_jump_cb_auto, (AW_CL)ntw,0);
    awr->awar( AWAR_DTREE_VERICAL_DIST)->add_callback( (AW_RCB)AWT_resize_cb, (AW_CL)ntw,0);
    awr->awar( AWAR_DTREE_BASELINEWIDTH)->add_callback( (AW_RCB)AWT_expose_cb, (AW_CL)ntw,0);
    awr->awar( AWAR_DTREE_SHOW_CIRCLE)->add_callback( (AW_RCB)AWT_expose_cb, (AW_CL)ntw,0);
    awr->awar( AWAR_DTREE_CIRCLE_ZOOM)->add_callback( (AW_RCB)AWT_expose_cb, (AW_CL)ntw,0);
    awr->awar( AWAR_DTREE_CIRCLE_MAX_SIZE)->add_callback( (AW_RCB)AWT_expose_cb, (AW_CL)ntw,0);
    awr->awar( AWAR_DTREE_USE_ELLIPSE)->add_callback( (AW_RCB)AWT_expose_cb, (AW_CL)ntw,0);
    awr->awar( AWAR_DTREE_REFRESH)->add_callback( (AW_RCB)AWT_expose_cb, (AW_CL)ntw,0);
    awr->awar( AWAR_COLOR_GROUPS_USE)->add_callback( (AW_RCB)NT_recompute_cb, (AW_CL)ntw,0);

    GBDATA *gb_arb_presets =    GB_search(gb_main,"arb_presets",GB_CREATE_CONTAINER);
    GB_add_callback(gb_arb_presets,GB_CB_CHANGED,(GB_CB)AWT_expose_cb, (int *)ntw);

    bool is_genome_db = GEN_is_genome_db(gb_main, 0); //  is this a genome database ? (default = 0 = not a genom db)

    // --------------------------------------------------------------------------------
    //     File
    // --------------------------------------------------------------------------------
    if (clone){
        awm->create_menu(       0,   "File",     "F", "nt_file.hlp",  AWM_ALL );
#if defined(DEBUG)
        AWMIMT("db_browser", "Browse loaded database(s)", "", "db_browser.hlp", AWM_ALL, AW_POPUP, (AW_CL)AWT_create_db_browser, 0);
        SEP________________________SEP();
#endif // DEBUG
        AWMIMT( "close", "Close",                   "C",0,      AWM_ALL, (AW_CB)AW_POPDOWN,     0, 0 );

    }else{
        awm->create_menu(       0,   "File",     "F", "nt_file.hlp",  AWM_ALL );
        {
#if defined(DEBUG)
            AWMIMT("db_browser", "Browse loaded database(s)", "", "db_browser.hlp", AWM_ALL, AW_POPUP, (AW_CL)AWT_create_db_browser, 0);
            AWMIMT("debug", "Dump GCs", "", "", AWM_ALL, NT_dump_gcs, 0, 0);
            SEP________________________SEP();
#endif // DEBUG
            AWMIMT("save_changes",  "Quicksave changes",            "s","save.hlp", AWM_ALL, (AW_CB)NT_save_quick_cb, 0,    0);
            AWMIMT("save_all_as",   "Save whole database as ...",       "w","save.hlp", AWM_ALL, AW_POPUP,  (AW_CL)NT_create_save_as, (AW_CL)"tmp/nt/arbdb");

            SEP________________________SEP();

            AWMIMT("new_window",    "New window",           "N","newwindow.hlp",        F_ALL, AW_POPUP, (AW_CL)create_nt_main_window, clone+1 );
            SEP________________________SEP();

            AWMIMT("optimize_db",   "Optimize database",        "O","optimize.hlp",AWM_EXP, AW_POPUP,   (AW_CL)NT_create_database_optimization_window, 0);
            SEP________________________SEP();

            awm->insert_sub_menu(  0,"Import",      "I" );
            {
                AWMIMT("import_seq",    "Import sequences and fields (ARB)","I","arb_import.hlp",AWM_ALL, NT_import_sequences,0,0);
                GDE_load_menu(awm,"import");
#if defined(DEVEL_LOTHAR) || defined(DEVEL_RALF)
                AWMIMT("MAUS", "Filter through MAUS..", "M", "MAUS.hlp", AWM_ALL, AW_POPUP, (AW_CL)NT_create_MAUS_window, 0);
#endif // DEVEL_LOTHAR
            }
            awm->close_sub_menu();

            awm->insert_sub_menu(  0,"Export",      "E" );
            {
                AWMIMT("export_to_ARB", "Export seq/tree/SAI's to new ARB database","A", "arb_ntree.hlp",   AWM_ALL, (AW_CB)NT_system_cb,   (AW_CL)"arb_ntree -export &",0 );
                AWMIMT("export_seqs",   "Export sequences to foreign format",    "f","arb_export.hlp",  AWM_ALL, AW_POPUP, (AW_CL)open_AWTC_export_window, (AW_CL)gb_main);
                GDE_load_menu(awm,"export");
                AWMIMT("export_nds",    "Export fields using NDS","N","arb_export_nds.hlp",AWM_EXP, AW_POPUP, (AW_CL)create_nds_export_window, 0);
            }
            awm->close_sub_menu();
            SEP________________________SEP();

            AWMIMT("macros",    "Macros ",              "M", "macro.hlp",   AWM_ALL,        (AW_CB)AW_POPUP,    (AW_CL)awt_open_macro_window,(AW_CL)"ARB_NT");

            awm->insert_sub_menu(  0,"Registration/Bug report/Version info",       "R" );
            {
                AWMIMT( "registration", "Registration",         "R","registration.hlp", AWM_EXP, (AW_CB)AW_POPUP,   (AW_CL)NT_submit_bug, 0 );
                AWMIMT( "bug_report",   "Bug report",           "B","registration.hlp", AWM_ALL, (AW_CB)AW_POPUP,   (AW_CL)NT_submit_bug, 1 );
                AWMIMT( "version_info", "Version info (" DATE ")",  "V","version.hlp",  AWM_ALL, (AW_CB)AW_POPUP_HELP,  (AW_CL)"version.hlp", 0 );
            }
            awm->close_sub_menu();
#if 0
            SEP________________________SEP();
            AWMIMT("undo",      "Undo",      "U", "undo.hlp", F_ALL, (AW_CB)NT_undo_cb,      (AW_CL)GB_UNDO_UNDO, (AW_CL)ntw);
            AWMIMT("redo",      "Redo",      "d", "undo.hlp", F_ALL, (AW_CB)NT_undo_cb,      (AW_CL)GB_UNDO_REDO, (AW_CL)ntw);
            AWMIMT("undo_info", "Undo info", "f", "undo.hlp", F_ALL, (AW_CB)NT_undo_info_cb, (AW_CL)GB_UNDO_UNDO, (AW_CL)0  );
            AWMIMT("redo_info", "Redo info", "o", "undo.hlp", F_ALL, (AW_CB)NT_undo_info_cb, (AW_CL)GB_UNDO_REDO, (AW_CL)0  );

            SEP________________________SEP();
#endif
            if (!nt.extern_quit_button){
                AWMIMT( "quit",     "Quit",             "Q","quit.hlp",     AWM_ALL, (AW_CB)nt_exit,    0, 0 );
            }

        }
        // --------------------------------------------------------------------------------
        //     Species
        // --------------------------------------------------------------------------------
        awm->create_menu(0,"Species","c","species.hlp", AWM_ALL);
        {
            AWMIMT( "species_search", "Search and query",    "q",    "sp_search.hlp",    AWM_ALL,AW_POPUP,   (AW_CL)ad_create_query_window,  0 );
            AWMIMT( "species_info",   "Species information", "i",    "sp_info.hlp",      AWM_ALL,NT_popup_species_window, 0, 0);
//             AWMIMT( "species_info",   "Species information", "i",    "sp_info.hlp",      AWM_ALL,AW_POPUP,   (AW_CL)NT_create_species_window,    0);

            SEP________________________SEP();

            NT_insert_mark_submenus(awm, ntw, 1);
            AWMIMT("gene_colors",     "Set Colors",     "l", "mark_colors.hlp",   AWM_ALL,  AW_POPUP,               (AW_CL)NT_create_species_colorize_window, 0);
            AWMIMT("selection_admin", "Configurations", "o", "configuration.hlp", AWM_SEQ2, NT_configuration_admin, (AW_CL)&(nt.tree->tree_root), (AW_CL)0     );

            SEP________________________SEP();

            awm->insert_sub_menu(0, "Database fields admin","f");
            ad_spec_create_field_items(awm);
            awm->close_sub_menu();
            NT_create_mask_submenu(awm);

            SEP________________________SEP();

            AWMIMT("del_marked",    "Delete Marked Species",    "D","sp_del_mrkd.hlp",  AWM_EXP, (AW_CB)NT_delete_mark_all_cb,      (AW_CL)ntw, 0 );

            awm->insert_sub_menu(0, "Sort Species",         "r");
            {
                AWMIMT("sort_by_field", "According to Database Entries","D","sp_sort_fld.hlp", AWM_EXP, AW_POPUP,              (AW_CL)NT_build_resort_window, 0 );
                AWMIMT("sort_by_tree",  "According to Phylogeny",      "P","sp_sort_phyl.hlp", AWM_EXP, (AW_CB)NT_resort_data_by_phylogeny,    (AW_CL)&(nt.tree->tree_root), 0 );
            }
            awm->close_sub_menu();

            awm->insert_sub_menu(0, "Merge Species",      "g");
            {
                AWMIMT("merge_species", "Create merged species from similar species",  "m", "sp_merge.hlp", AWM_EXP,AW_POPUP,(AW_CL)NT_createMergeSimilarSpeciesWindow, (AW_CL)ntw );
                AWMIMT("join_marked",   "Join Marked Species",    "J","join_species.hlp", AWM_EXP, AW_POPUP, (AW_CL)create_species_join_window,   0 );
            }
            awm->close_sub_menu();

            SEP________________________SEP();

            AWMIMT( "species_submission", "Submit Species", "b",  "submission.hlp",   AWM_ALL,AW_POPUP,   (AW_CL)AWTC_create_submission_window,   0 );

            SEP________________________SEP();

            AWMIMT( "new_names",    "Generate New Names",   "e", "sp_rename.hlp",   AWM_EXP, AW_POPUP,   (AW_CL)AWTC_create_rename_window,      (AW_CL)gb_main );

            awm->insert_sub_menu(0, "Valid Names ...",     "");
            {
                AWMIMT("imp_names",    "Import Names from File", "", "vn_import.hlp",  AWM_EXP, NT_importValidNames,  0, 0);
                AWMIMT("del_names",    "Delete Names from DB",   "", "vn_delete.hlp",  AWM_EXP, NT_deleteValidNames,  0, 0);
                AWMIMT("sug_names",    "Suggest Valid Names",    "", "vn_suggest.hlp", AWM_EXP, NT_suggestValidNames, 0, 0);
                AWMIMT("search_names", "Search manually",        "", "vn_search.hlp",  AWM_ALL, AW_POPUP,             (AW_CL)NT_searchManuallyNames , 0);
           }
            awm->close_sub_menu();
        }
        //  --------------------------
        //      Genes + Experiment
        //  --------------------------

        if (is_genome_db) GEN_create_genes_submenu(awm, true/*, ntw*/);

        // --------------------------------------------------------------------------------
        //     Sequence
        // --------------------------------------------------------------------------------
        awm->create_menu(0,"Sequence","S","sequence.hlp",   AWM_ALL);
        {
            AWMIMT("seq_admin",   "Sequence/Alignment Admin", "A", "ad_align.hlp",   AWM_EXP,  AW_POPUP, (AW_CL)NT_create_alignment_window, 0);
            AWMIMT("ins_del_col", "Insert/Delete Column",     "I", "insdelchar.hlp", AWM_SEQ2, AW_POPUP, (AW_CL)create_insertchar_window,   0);
            SEP________________________SEP();

            awm->insert_sub_menu(0, "Edit Sequences","E");
            {
                AWMIMT("new_arb_edit4",  "Using marked species and tree", "m", "arb_edit4.hlp", AWM_ALL,  (AW_CB)NT_start_editor_on_tree, (AW_CL)&(nt.tree->tree_root), 0 );
                AWMIMT("new2_arb_edit4", "... plus relatives",            "r", "arb_edit4.hlp", AWM_ALL,  (AW_CB)NT_start_editor_on_tree, (AW_CL)&(nt.tree->tree_root), -1);
                AWMIMT("old_arb_edit4",  "Using earlier configuration",   "c", "arb_edit4.hlp", AWM_SEQ2, AW_POPUP, (AW_CL)NT_start_editor_on_old_configuration, 0        );
            }
            awm->close_sub_menu();

            awm->insert_sub_menu(0, "Other sequence editors","O");
            {
                AWMIMT("arb_edit", "ARB Editor (old)", "o", "arb_edit.hlp", AWM_SEQ2, (AW_CB)NT_system_cb, (AW_CL)"arb_edit &",  0               );
                // AWMIMT("arb_ale",  "ALE Editor",       "A", "ale.hlp",      AWM_SEQ2, (AW_CB)NT_system_cb, (AW_CL)"arb_ale : &", (AW_CL)"ale.hlp");
                AWMIMT("arb_gde",  "GDE Editor",       "G", "gde.hlp",      AWM_SEQ2, (AW_CB)NT_system_cb, (AW_CL)"arb_gde : &", (AW_CL)"gde.hlp");
            }
            awm->close_sub_menu();
            SEP________________________SEP();

            awm->insert_sub_menu(0, "Align Sequences",  "S");
            {
                AWMIMT("arb_align",   "Align sequence into an existing alignment",         "A", "align.hlp",       AWM_ALL, (AW_CB) AW_POPUP_HELP,(AW_CL)"align.hlp",0   );
                AWMIMT("realign_dna", "Realign nucleic acid according to aligned protein", "R", "realign_dna.hlp", AWM_ALL, AW_POPUP, (AW_CL)NT_create_realign_dna_window, 0);
                GDE_load_menu(awm,"align");
            }
            awm->close_sub_menu();
            AWMIMT("seq_concat","Concatenate Sequences/Alignments", "C","concatenate_align.hlp", AWM_EXP,    AW_POPUP, (AW_CL)NT_createConcatenationWindow, (AW_CL)ntw );
            SEP________________________SEP();

            AWMIMT("dna_2_pro", "Perform translation",                     "t", "translate_dna_2_pro.hlp", AWM_PRO, AW_POPUP, (AW_CL)NT_create_dna_2_pro_window, 0      );
            AWMIMT("arb_dist",  "Compare sequences using Distance Matrix", "D", "dist.hlp",                AWM_SEQ, (AW_CB)NT_system_cb,    (AW_CL)"arb_dist &",    0);
            SEP________________________SEP();

#if defined(DEBUG)
#if defined(DEVEL_RALF)
#warning sequence quality check for big databases needs to be fixed
#endif // DEVEL_RALF
            AWMIMT("seq_quality", "Check Sequence Quality",    "",  "seq_quality.hlp",   AWM_EXP,  AW_POPUP, (AW_CL)SQ_create_seq_quality_window, 0);
#endif // DEBUG

#if defined(DEBUG)
#if defined(DEVEL_RALF)
#warning chimere check needs to be fixed
#endif // DEVEL_RALF
            AWMIMT("seq_quality", "Chimere Check [broken!]",   "m", "check_quality.hlp", AWM_SEQ2, AW_POPUP, (AW_CL)st_create_quality_check_window, (AW_CL)gb_main);

            SEP________________________SEP();
#endif // DEBUG

            GDE_load_menu(awm,"pretty_print");
        }
        // --------------------------------------------------------------------------------
        //     SAI
        // --------------------------------------------------------------------------------
        awm->create_menu(0,"SAI","A","extended.hlp",    AWM_ALL);
        {
            AWMIMT("sai_admin", "Manage SAIs",                "S","ad_extended.hlp",  AWM_EXP,    AW_POPUP, (AW_CL)NT_create_extendeds_window,   0 );
            SEP________________________SEP();
            awm->insert_sub_menu(0, "Create SAI using ...", "C");
            {
                AWMIMT("sai_max_freq",  "Max. Frequency",                                               "M", "max_freq.hlp",     AWM_ALL,   AW_POPUP, (AW_CL)AP_open_max_freq_window, 0    );
                AWMIMT("sai_consensus", "Consensus",                                                    "C", "consensus.hlp",    AWM_ALL,   AW_POPUP, (AW_CL)AP_open_con_expert_window, 0  );
                AWMIMT("pos_var_pars",  "Positional variability + Column statistic (parsimony method)", "a", "pos_var_pars.hlp", AWM_TREE2, AW_POPUP, (AW_CL)AP_open_pos_var_pars_window, 0);
                AWMIMT("arb_phyl",      "Filter by base frequency",                                     "F", "phylo.hlp",        AWM_TREE2, (AW_CB)NT_system_cb,  (AW_CL)"arb_phylo &",   0);

                GDE_load_menu(awm,"SAI");
            }
            awm->close_sub_menu();

            if (!GB_NOVICE){
                awm->insert_sub_menu(0, "Other functions",  "O");
                {
                    AWMIMT("pos_var_dist",          "Positional variability (distance method)",     "P", "pos_variability.ps", AWM_EXP, AW_POPUP,      (AW_CL)AP_open_cprofile_window,          0);
                    AWMIMT("count_different_chars", "Count different chars/column",                 "C", "count_chars.hlp",    AWM_EXP, NT_system_cb2, (AW_CL)"arb_count_chars",                0);
                    AWMIMT("export_pos_var",        "Export Column Statistic (GNUPLOT format)",     "E", "csp_2_gnuplot.hlp",  AWM_EXP, AW_POPUP,      (AW_CL)AP_open_csp_2_gnuplot_window,     0);
#ifdef DEVEL_YADHU
#warning will deal later &^%*^*!!
                    // AWMIMT("conservation_profile",  "Display Conservation Profile (Using GNUPLOT)", "D", "conser_profile.hlp", AWM_EXP, AW_POPUP,      (AW_CL)AP_openConservationProfileWindow, 0); // spelling fixed [ralf]
#endif
                }
                awm->close_sub_menu();
            }
        }

        // --------------------------------------------------------------------------------
        //      Probes
        // --------------------------------------------------------------------------------
        awm->create_menu(0,"Probes","P","probe_menu.hlp", AWM_ALL);
        {
            AWMIMT("probe_design",      "Design Probes",             "D", "probedesign.hlp", AWM_PRB, AW_POPUP, (AW_CL)create_probe_design_window, (AW_CL)is_genome_db );
            AWMIMT("probe_multi",       "Calculate Multi-Probes",    "u", "multiprobe.hlp",  AWM_PRB, AW_POPUP, (AW_CL)MP_main, (AW_CL)ntw           );
            AWMIMT("probe_match",       "Match Probes",              "M", "probematch.hlp",  AWM_PRB, AW_POPUP, (AW_CL)create_probe_match_window, 0  );
            SEP________________________SEP();
            AWMIMT("primer_design_new", "Design Primers",            "P", "primer_new.hlp",  AWM_PRB, AW_POPUP, (AW_CL)create_primer_design_window, 0);
            AWMIMT("primer_design",     "Design Sequencing Primers", "",  "primer.hlp",      AWM_EXP, (AW_CB)NT_primer_cb, 0, 0                      );
            SEP________________________SEP();
            AWMIMT("pt_server_admin",   "PT_SERVER Admin",           "A", "probeadmin.hlp",  AWM_ALL, AW_POPUP, (AW_CL)create_probe_admin_window, (AW_CL)is_genome_db);
        }

    } // clone

    // --------------------------------------------------------------------------------
    //     Tree
    // --------------------------------------------------------------------------------
    awm->create_menu( 0,   "Tree", "T", "nt_tree.hlp",  AWM_ALL );
    {
        if (!clone) {
            awm->insert_sub_menu(0, "Add Species to Existing Tree", "A");
            {
                AWMIMT( "arb_pars_quick",  "ARB Parsimony (Quick add marked)", "Q", "pars.hlp",    AWM_TREE,   (AW_CB)NT_system_cb,    (AW_CL)"arb_pars -add_marked -quit &",0 );
                AWMIMT( "arb_pars",        "ARB Parsimony interactive",        "i", "pars.hlp",    AWM_TREE,   (AW_CB)NT_system_cb,    (AW_CL)"arb_pars &",    0 );
                GDE_load_menu(awm,"Incremental_Phylogeny");
            }
            awm->close_sub_menu();
        }

        awm->insert_sub_menu(0, "Remove Species from Tree",     "R");
        {
            AWMIMT("tree_remove_deleted", "Remove zombies", "z", "trm_del.hlp",    AWM_TREE, (AW_CB)NT_remove_leafs, (AW_CL)ntw, AWT_REMOVE_DELETED );
            AWMIMT("tree_remove_marked",  "Remove marked",  "m", "trm_mrkd.hlp",   AWM_TREE, (AW_CB)NT_remove_leafs, (AW_CL)ntw, AWT_REMOVE_MARKED );
            AWMIMT("tree_keep_marked",    "Keep marked",    "K", "tkeep_mrkd.hlp", AWM_TREE, (AW_CB)NT_remove_leafs, (AW_CL)ntw, AWT_REMOVE_NOT_MARKED|AWT_REMOVE_DELETED);
#if defined(DEVEL_RALF)
#warning fix NT_alltree_remove_leafs. does SIGSEGV.
#warning add "remove duplicates from tree"
            SEP________________________SEP();
            AWMIMT("all_tree_remove_deleted", "Remove zombies from all trees", "a", "trm_del.hlp", AWM_TREE, (AW_CB)NT_alltree_remove_leafs, AWT_REMOVE_DELETED, (AW_CL)gb_main);
            AWMIMT("all_tree_remove_marked",  "Remove marked from all trees",  "l", "trm_del.hlp", AWM_TREE, (AW_CB)NT_alltree_remove_leafs, AWT_REMOVE_MARKED, (AW_CL)gb_main);
#endif // DEVEL_RALF
        }
        awm->close_sub_menu();

        if (!clone) {
            awm->insert_sub_menu(0, "Build tree from sequence data",    "B");
            {
                awm->insert_sub_menu(0, "Distance matrix methods", "D");
                AWMIMT( "arb_dist",     "ARB Neighbor Joining",     "J", "dist.hlp",    AWM_TREE,   (AW_CB)NT_system_cb,    (AW_CL)"arb_dist &",    0 );
                GDE_load_menu(awm,"Phylogeny_DistMatrix");
                awm->close_sub_menu();

                awm->insert_sub_menu(0, "Maximum Parsimony methods", "P");
                GDE_load_menu(awm,"Phylogeny_MaxParsimony");
                awm->close_sub_menu();

                awm->insert_sub_menu(0, "Maximum Likelihood methods", "L");
                GDE_load_menu(awm,"Phylogeny_MaxLikelyhood");
                awm->close_sub_menu();

                awm->insert_sub_menu(0, "Other methods", "O");
                GDE_load_menu(awm,"Phylogeny_Other");
                awm->close_sub_menu();
            }
            awm->close_sub_menu();
        }

        SEP________________________SEP();

        if (!clone){
            awm->insert_sub_menu(0, "Reset zoom",         "z");
            {
                AWMIMT("reset_logical_zoom",  "Logical zoom",  "L", "rst_log_zoom.hlp",  AWM_ALL, (AW_CB)NT_reset_lzoom_cb, (AW_CL)ntw, 0);
                AWMIMT("reset_physical_zoom", "Physical zoom", "P", "rst_phys_zoom.hlp", AWM_ALL, (AW_CB)NT_reset_pzoom_cb, (AW_CL)ntw, 0);
            }
            awm->close_sub_menu();
        }
        awm->insert_sub_menu(0, "Collapse/Expand tree",         "C");
        {
            AWMIMT("tree_group_all",         "Group all",               "a", "tgroupall.hlp",   AWM_ALL,  (AW_CB)NT_group_tree_cb,       (AW_CL)ntw, 0);
            AWMIMT("tree_group_not_marked",  "Group all except marked", "m", "tgroupnmrkd.hlp", AWM_ALL,  (AW_CB)NT_group_not_marked_cb, (AW_CL)ntw, 0);
            AWMIMT("tree_group_term_groups", "Group terminal groups",   "t", "tgroupterm.hlp",  AWM_TREE, (AW_CB)NT_group_terminal_cb,   (AW_CL)ntw, 0);
            AWMIMT("tree_ungroup_all",       "Ungroup all",             "U", "tungroupall.hlp", AWM_ALL,  (AW_CB)NT_ungroup_all_cb,      (AW_CL)ntw, 0);
            SEP________________________SEP();
            NT_insert_color_collapse_submenu(awm, ntw);
        }
        awm->close_sub_menu();
        awm->insert_sub_menu(0, "Beautify tree", "e");
        {
            AWMIMT("beautifyt_tree", "#beautifyt.bitmap", "", "resorttree.hlp", AWM_TREE, (AW_CB)NT_resort_tree_cb, (AW_CL)ntw, 0);
            AWMIMT("beautifyc_tree", "#beautifyc.bitmap", "", "resorttree.hlp", AWM_TREE, (AW_CB)NT_resort_tree_cb, (AW_CL)ntw, 1);
            AWMIMT("beautifyb_tree", "#beautifyb.bitmap", "", "resorttree.hlp", AWM_TREE, (AW_CB)NT_resort_tree_cb, (AW_CL)ntw, 2);
        }
        awm->close_sub_menu();
        awm->insert_sub_menu(0, "Modify branches", "M");
        {
            AWMIMT("tree_remove_remark",     "Remove bootstraps",       "b", "trm_boot.hlp",      AWM_TREE, NT_remove_bootstrap,      (AW_CL)ntw, 0);
            SEP________________________SEP();
            AWMIMT("tree_reset_lengths",     "Reset branchlengths",    "R", "tbl_reset.hlp",   AWM_TREE, NT_reset_branchlengths,   (AW_CL)ntw, 0);
            AWMIMT("justify_branch_lengths", "Justify branchlengths",  "J", "tbl_justify.hlp", AWM_TREE, NT_justify_branch_lenghs, (AW_CL)ntw, 0);
            AWMIMT("tree_scale_lengths",     "Scale Branchlengths",    "S", "tbl_scale.hlp",   AWM_TREE, NT_scale_tree,            (AW_CL)ntw, 0);
            SEP________________________SEP();
            AWMIMT("tree_boot2len",      "Bootstraps -> Branchlengths", "", "tbl_boot2len.hlp", AWM_TREE, NT_move_boot_branch, (AW_CL)ntw, 0);
            AWMIMT("tree_len2boot",      "Bootstraps <- Branchlengths", "", "tbl_boot2len.hlp", AWM_TREE, NT_move_boot_branch, (AW_CL)ntw, 1);

        }
        awm->close_sub_menu();
        AWMIMT("mark_long_branches", "Mark long branches", "k", "mark_long_branches.hlp", AWM_EXP, (AW_CB)NT_mark_long_branches, (AW_CL)ntw, 0);
        AWMIMT("mark_duplicates", "Mark duplicates", "u", "mark_duplicates.hlp", AWM_EXP, (AW_CB)NT_mark_duplicates, (AW_CL)ntw, 0);

        SEP________________________SEP();

        AWMIMT("tree_select",        "Select Tree",      "T", 0, AWM_ALL,  AW_POPUP, (AW_CL)NT_open_select_tree_window,   (AW_CL)awar_tree);
        AWMIMT("tree_select_latest", "Select Last Tree", "L", 0, AWM_TREE, (AW_CB)NT_select_last_tree, (AW_CL)awar_tree, 0                );

        SEP________________________SEP();

        if (!clone){
            AWMIMT("tree_admin", "Tree admin",                "d", "treeadm.hlp",   AWM_TREE, AW_POPUP, (AW_CL)create_trees_window, 0             );
            AWMIMT("nds",        "Select visible info (NDS)", "N", "props_nds.hlp", AWM_ALL,  AW_POPUP, (AW_CL)AWT_open_nds_window, (AW_CL)gb_main);
        }
        SEP________________________SEP();

        AWMIMT("transversion",       "Transversion analysis",   "y", "trans_anal.hlp", AWM_TREE, (AW_CB)AW_POPUP_HELP,       (AW_CL)"trans_anal.hlp", 0);

        SEP________________________SEP();

        if (!clone){
            AWMIMT("print_tree",  "Print tree",          "P", "tree2prt.hlp",  AWM_ALL, (AW_CB)AWT_create_print_window, (AW_CL)ntw, 0         );
            AWMIMT("tree_2_xfig", "Export tree to XFIG", "X", "tree2file.hlp", AWM_ALL, AW_POPUP, (AW_CL)AWT_create_export_window,  (AW_CL)ntw);
            SEP________________________SEP();
        }

        awm->insert_sub_menu(0, "Other..",  "O");
        {
            AWMIMT("tree_pseudo_species_to_organism", "Change pseudo species to organisms in tree", "p", "tree_pseudo.hlp",        AWM_EXP, (AW_CB)NT_pseudo_species_to_organism, (AW_CL)ntw, 0);
        }
        awm->close_sub_menu();
    }

    if(!clone){

        // --------------------------------------------------------------------------------
        //     Tools
        // --------------------------------------------------------------------------------
        awm->create_menu(0,"Tools","o","nt_etc.hlp", AWM_ALL);
        {
            AWMIMT("names_admin",      "Name server admin",    "s","namesadmin.hlp",   AWM_EXP, AW_POPUP, (AW_CL)create_awtc_names_admin_window, 0 );
            awm->insert_sub_menu( 0,   "Network", "N");
            {
                AWMIMT("ors",   "ORS (disabled) ...",      "O","ors.hlp",          AWM_ALL, (AW_CB)NT_system_cb, (AW_CL)"netscape http://pop.mikro.biologie.tu-muenchen.de/ORS/ &", 0);
                GDE_load_menu(awm,"user");
            }
            awm->close_sub_menu();
            SEP________________________SEP();

            awm->insert_sub_menu(0,"GDE specials","G");
            {
                GDE_load_menu(awm,0,0);
            }
            awm->close_sub_menu();

            if (!GB_NOVICE){
                awm->insert_sub_menu(0,"WL specials","W");
                {
                    AWMIMT("chewck_gcg_list",         "Check GCG list",          "C", "checkgcg.hlp", AWM_TUM, AW_POPUP, (AW_CL)create_check_gcg_window,          0         );
                    AWMIMT("view_probe_group_result", "View probe group result", "V", "",             AWM_EXP, AW_POPUP, (AW_CL)create_probe_group_result_window, (AW_CL)ntw);
                }
                awm->close_sub_menu();
            }

            if (!GB_NOVICE) {
                SEP________________________SEP();
                AWMIMT("xterm",         "Start XTERM",             "X",0   ,       AWM_EXP, (AW_CB)GB_xterm, (AW_CL)0, 0 );
            }
            if (GBS_do_core()){
            }
        }
#if defined(DEBUG)
        awm->create_menu(0,"Debugging","D","", AWM_ALL);
        {
            AWMIMT("fix_db",      "Fix database",            "F", "fixdb.hlp", AWM_EXP, (AW_CB)NT_fix_database, 0, 0                        );
            AWMIMT("debug_arbdb", "Print debug information", "d", 0,           AWM_ALL, (AW_CB)GB_print_debug_information, (AW_CL)gb_main, 0);
            AWMIMT("test_compr",  "Test compression",        "c", 0,           AWM_ALL, (AW_CB)GBT_compression_test, (AW_CL)gb_main, 0      );
            AWMIMT("test_rename", "Test rename",             "",  0,           AWM_ALL, NT_rename_test, (AW_CL)gb_main, 0                   );
            SEP________________________SEP();
            AWMIMT("table_admin",       "Table Admin (unfinished/unknown purpose)",  "T","tableadm.hlp",     AWM_ALL, AW_POPUP,(AW_CL)AWT_create_tables_admin_window, (AW_CL)gb_main);
        }
#endif // DEBUG

        // --------------------------------------------------------------------------------
        //     Properties
        // --------------------------------------------------------------------------------
        awm->create_menu(0,"Properties","r","properties.hlp", AWM_ALL);
        {
            AWMIMT("props_menu",    "Frame settings", "F","props_frame.hlp",      AWM_ALL, AW_POPUP, (AW_CL)AWT_preset_window, 0 );
            awm->insert_sub_menu(0, "Tree settings",  "T");
            {
                AWMIMT("props_tree2",   "Tree options",        "o","nt_tree_settings.hlp", AWM_ALL, AW_POPUP, (AW_CL)NT_create_tree_setting, (AW_CL)ntw );
                AWMIMT("props_tree",    "Tree colors & fonts", "c","nt_props_data.hlp",    AWM_ALL, AW_POPUP, (AW_CL)AW_create_gc_window, (AW_CL)aw_gc_manager );
            }
            awm->close_sub_menu();
            AWMIMT("props_www",      "Search world wide web (WWW)",                 "W","props_www.hlp",        AWM_ALL, AW_POPUP, (AW_CL)AWT_open_www_window,  (AW_CL)gb_main );
            SEP________________________SEP();
            AWMIMT("enable_advices", "Reactivate advices",                          "R","advice.hlp", AWM_ALL, (AW_CB) AWT_reactivate_all_advices, 0, 0 );
            SEP________________________SEP();
            AWMIMT("save_props",     "Save properties (in ~/.arb_prop/ntree.arb)",  "S","savedef.hlp",AWM_ALL, (AW_CB) AW_save_defaults, 0, 0 );
        }
    } // clone

    awm->insert_help_topic("help_how",    "How to use help", "H", "help.hlp",      AWM_ALL, (AW_CB)AW_POPUP_HELP, (AW_CL)"help.hlp",      0);
    awm->insert_help_topic("help_arb",    "ARB help",        "A", "arb.hlp",       AWM_ALL, (AW_CB)AW_POPUP_HELP, (AW_CL)"arb.hlp",       0);
    awm->insert_help_topic("help_arb_nt", "ARB_NT help",     "N", "arb_ntree.hlp", AWM_ALL, (AW_CB)AW_POPUP_HELP, (AW_CL)"arb_ntree.hlp", 0);

    awm->create_mode(0, "select.bitmap",   "mode_select.hlp", AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_SELECT);
    awm->create_mode(0, "mark.bitmap",     "mode_mark.hlp",   AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_MARK  );
    awm->create_mode(0, "group.bitmap",    "mode_group.hlp",  AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_GROUP );
    awm->create_mode(0, "pzoom.bitmap",     "mode_pzoom.hlp",  AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_ZOOM  );
    awm->create_mode(0, "lzoom.bitmap",    "mode_lzoom.hlp",  AWM_EXP, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_LZOOM );
    awm->create_mode(0, "modify.bitmap",   "mode_info.hlp",   AWM_ALL, (AW_CB)NT_modify_cb,  (AW_CL)ntw, (AW_CL)AWT_MODE_MOD   );
    awm->create_mode(0, "www_mode.bitmap", "mode_www.hlp",    AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_WWW   );

    awm->create_mode(0, "line.bitmap",    "mode_width.hlp",    AWM_TREE, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_LINE   );
    awm->create_mode(0, "rot.bitmap",     "mode_rotate.hlp",   AWM_TREE, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_ROT    );
    awm->create_mode(0, "spread.bitmap",  "mode_angle.hlp",    AWM_TREE, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_SPREAD );
    awm->create_mode(0, "swap.bitmap",    "mode_swap.hlp",     AWM_TREE, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_SWAP   );
    awm->create_mode(0, "length.bitmap",  "mode_length.hlp",   AWM_TREE, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_LENGTH );
    awm->create_mode(0, "move.bitmap",    "mode_move.hlp",     AWM_TREE, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_MOVE   );
    awm->create_mode(0, "setroot.bitmap", "mode_set_root.hlp", AWM_TREE, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_SETROOT);
    awm->create_mode(0, "reset.bitmap",   "mode_reset.hlp",    AWM_TREE, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_RESET  );

    awm->set_info_area_height( 250 );
    awm->at(5,2);
    awm->auto_space(0,-5);
    awm->shadow_width(1);

    // path, tree, alignment & searchSpecies buttons:

    int first_liney;
    int db_pathx;
    awm->get_at_position( &db_pathx, &first_liney);
    awm->callback( AW_POPUP, (AW_CL)NT_create_save_quick_as, (AW_CL)"tmp/nt/arbdb");
    awm->button_length(19);
    awm->help_text("saveas.hlp");
    awm->create_button("SAVE_AS",AWAR_DB_NAME);

    int db_treex;
    awm->get_at_position( &db_treex, &first_liney);
    awm->callback((AW_CB2)AW_POPUP,(AW_CL)NT_open_select_tree_window,(AW_CL)awar_tree);
    awm->button_length(19);
    awm->help_text("nt_tree_select.hlp");
    awm->create_button("SELECT_A_TREE", awar_tree);

    int db_alignx;
    awm->get_at_position( &db_alignx, &first_liney);
    awm->callback(AW_POPUP,   (AW_CL)NT_open_select_alignment_window, 0 );
    awm->help_text("nt_align_select.hlp");
    awm->create_button("SELECT_AN_ALIGNMENT", AWAR_DEFAULT_ALIGNMENT);

    int db_searchx;
    awm->get_at_position(&db_searchx, &first_liney);
    awm->callback(NT_popup_species_window, (AW_CL)awm, 0);
//     awm->callback(AW_POPUP,   (AW_CL)create_speciesOrganismWindow/*ad_create_query_window*/, 0 );
    awm->button_length(20);
    awm->help_text("sp_search.hlp");
    awm->create_button("SEARCH_SPECIES",  AWAR_SEARCH_BUTTON_TEXT);

    int behind_buttonsx; // after first row of buttons
    awm->get_at_position( &behind_buttonsx, &first_liney );

    awm->at_newline();
    int second_linex, second_liney;
    awm->get_at_position(&second_linex, &second_liney);
    second_liney += 5;
    awm->at_y(second_liney);

    // undo/redo:

    awm->auto_space(-2,-2);

    awm->button_length(9);
    awm->at_x(db_pathx);
    awm->callback(NT_undo_cb,(AW_CL)GB_UNDO_UNDO,(AW_CL)ntw);
    awm->help_text("undo.hlp");
    awm->create_button("UNDO", "#undo.bitmap",0);

    awm->callback(NT_undo_cb,(AW_CL)GB_UNDO_REDO,(AW_CL)ntw);
    awm->help_text("undo.hlp");
    awm->create_button("REDO", "#redo.bitmap",0);

    awm->at_newline();
    int third_linex, third_liney;
    awm->get_at_position(&third_linex, &third_liney);

    // tree buttons:

    awm->button_length(9);
    awm->at(db_treex, second_liney);
    awm->callback((AW_CB)NT_set_tree_style,(AW_CL)ntw,(AW_CL)AP_TREE_RADIAL);
    awm->help_text("tr_type_radial.hlp");
    awm->create_button("RADIAL_TREE_TYPE", "#radial.bitmap",0);

    awm->callback((AW_CB)NT_set_tree_style,(AW_CL)ntw,(AW_CL)AP_TREE_NORMAL);
    awm->help_text("tr_type_list.hlp");
    awm->create_button("LIST_TREE_TYPE", "#list.bitmap",0);

    awm->at(db_treex, third_liney);
    awm->callback((AW_CB)NT_set_tree_style,(AW_CL)ntw,(AW_CL)AP_TREE_IRS);
    awm->help_text("tr_type_irs.hlp");
    awm->create_button("FOLDED_LIST_TREE_TYPE", "#list.bitmap",0);

    awm->callback((AW_CB)NT_set_tree_style,(AW_CL)ntw,(AW_CL)AP_LIST_NDS);
    awm->help_text("tr_type_nds.hlp");
    awm->create_button("NO_TREE_TYPE", "#ndstree.bitmap",0);

    awm->at_newline();
    awm->button_length(AWAR_FOOTER_MAX_LEN);
    awm->create_button(0,AWAR_FOOTER);
    awm->at_newline();
    int last_linex, last_liney;
    awm->get_at_position( &last_linex,&last_liney );

    // edit, gene-map, jump & web buttons (+help buttons):

    awm->button_length(9);
    awm->at(db_alignx, second_liney);
    awm->callback((AW_CB)NT_start_editor_on_tree, (AW_CL)&(nt.tree->tree_root), 0);
    awm->help_text("arb_edit4.hlp");
    awm->create_button("EDIT_SEQUENCES", "#edit.bitmap",0);

    if (is_genome_db) {
        awm->button_length(4);
        awm->callback((AW_CB)AW_POPUP, (AW_CL)GEN_map_first, 0/*, (AW_CL)ntw*/); // initial gene map
        awm->help_text("gene_map.hlp");
        awm->create_button("OPEN_GENE_MAP", "#gen_map.bitmap",0);
    }

    awm->button_length(0);
    awm->at(db_searchx, second_liney);

    awm->callback(AW_POPUP,   (AW_CL)ad_create_query_window, 0 );
    awm->create_button("SEARCH", "#search.bitmap",0);

    awm->callback((AW_CB)NT_jump_cb,(AW_CL)ntw,1);
    awm->help_text("tr_jump.hlp");
    awm->create_button("JUMP", "#pjump.bitmap",0);

    awm->callback((AW_CB1)awt_openDefaultURL_on_species,(AW_CL)gb_main);
    awm->help_text("www.hlp");
    awm->create_button("WWW", "#www.bitmap",0);

    // count marked species

    awm->button_length(20);
    awm->at(db_searchx, third_liney);
    awm->callback(nt_count_marked);
    awm->help_text("marked_species.hlp");
    awm->create_button(0, AWAR_MARKED_SPECIES_COUNTER);
    {
        GBDATA *gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
        GB_add_callback(gb_species_data, GB_CB_CHANGED, nt_auto_count_marked_species, (int*)awm);
        nt_count_marked(awm);
    }

    // help:

    awm->at(behind_buttonsx, first_liney);
    awm->callback(AW_POPUP_HELP,(AW_CL)"arb_ntree.hlp");
    awm->button_length(0);
    awm->help_text("help.hlp");
    awm->create_button("HELP","HELP","H");

    awm->callback( AW_help_entry_pressed );
    awm->create_button("?","?");

    // expand and collapse display icons

    awr->awar_int(AWAR_NTREE_TITLE_MODE)->add_callback((AW_RCB1)title_mode_changed, (AW_CL)awm);
    awm->create_toggle(AWAR_NTREE_TITLE_MODE, "#more.bitmap", "#less.bitmap");

    // protect:

    awm->at(behind_buttonsx, second_liney+3);
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
        awr->awar(AWAR_SECURITY_LEVEL)->write_int(6);
    }

    awm->set_info_area_height( last_liney+4 ); windowHeight = last_liney+4;
    awm->set_bottom_area_height( 0 );
    awr->set_focus_callback((AW_RCB)NT_focus_cb,(AW_CL)gb_main,0);

    //  -----------------------------------
    //  Autostarts for development
    //  -----------------------------------

#if defined(DEVEL_RALF)
    // Automatically start:
    // --------------------

    // NT_test_AWT(awm);
    // NT_test_input_mask(awm->get_root());
    // if (is_genome_db) GEN_map_first(awr)->show();

#endif // DEVEL_RALF

    GB_pop_transaction(gb_main);

    return (AW_window *)awm;

}

