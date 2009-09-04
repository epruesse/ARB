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
#include "nt_sort.hxx"
#include "ap_consensus.hxx"
#include "ap_csp_2_gnuplot.hxx"
#include "ap_conservProfile2Gnuplot.hxx"
#include <awti_export.hxx>
#include "nt_join.hxx"
#include "nt_edconf.hxx"
#include "ap_pos_var_pars.hxx"
#include <arb_version.h>
#include "nt_internal.h"
#include <st_window.hxx>
#include <probe_design.hxx>
#include <primer_design.hxx>
#include <GEN.hxx>
#include <EXP.hxx>
#include <awt_input_mask.hxx>

#include "seq_quality.h"
#include "NT_trackAliChanges.h"
#include "NT_dbrepair.hxx"

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

void nt_test_ascii_print(AW_window *aww){
    AWT_create_ascii_print_window(aww->get_root(),"hello world","Just a test");
}

void nt_changesecurity(AW_root *aw_root) {
    long level = aw_root->awar(AWAR_SECURITY_LEVEL)->read_int();
    GB_push_transaction(GLOBAL_gb_main);
    GB_change_my_security(GLOBAL_gb_main,(int)level,"");
    GB_pop_transaction(GLOBAL_gb_main);
}

void export_nds_cb(AW_window *aww,AW_CL print_flag) {
    GB_transaction  dummy(GLOBAL_gb_main);
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
    make_node_text_init(GLOBAL_gb_main);
    int   tabbed    = aw_root->awar(AWAR_EXPORT_NDS"/tabbed")->read_int();
    char *tree_name = aw_root->awar(AWAR_TREE)->read_string();

    for (gb_species = GBT_first_marked_species(GLOBAL_gb_main);
         gb_species;
         gb_species = GBT_next_marked_species(gb_species))
    {
        buf = make_node_text_nds(GLOBAL_gb_main, gb_species,(tabbed ? 2 : 1),0, tree_name);
        fprintf(out,"%s\n",buf);
    }
    awt_refresh_selection_box(aw_root, AWAR_EXPORT_NDS);
    fclose(out);
    if (print_flag) {
        GB_ERROR error = GB_textprint(name);
        if (error) aw_message(error);
    }
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

static void AWAR_INFO_BUTTON_TEXT_change_cb(AW_root *awr) {
    char *value = awr->awar(AWAR_SPECIES_NAME)->read_string();
    awr->awar(AWAR_INFO_BUTTON_TEXT)->write_string(value[0] ? value : "Species Info");
    free(value);
}

static void expert_mode_changed_cb(AW_root *aw_root) {
    aw_root->awar(AWAR_AWM_MASK)->write_int(aw_root->awar(AWAR_EXPERT)->read_int() ? AWM_ALL : AWM_BASIC); // publish expert-mode globally
}
static void NT_toggle_expert_mode(AW_window *aww, AW_CL, AW_CL) {
    AW_root *aw_root     = aww->get_root();
    AW_awar *awar_expert = aw_root->awar(AWAR_EXPERT);
    awar_expert->write_int(!awar_expert->read_int());
}

void nt_create_all_awars(AW_root *awr, AW_default def) {
    // creates awars for all modules reachable from ARB_NT main window

    awr->awar_string( AWAR_FOOTER, "", def);
    if (GB_read_clients(GLOBAL_gb_main)>=0){
        awr->awar_string( AWAR_TREE, "tree_main", GLOBAL_gb_main);
    }else{
        awr->awar_string( AWAR_TREE, "tree_main", def);
    }

    awr->awar_string( AWAR_SPECIES_NAME, "" ,   GLOBAL_gb_main);
    awr->awar_string( AWAR_SAI_NAME, "" ,   GLOBAL_gb_main);
    awr->awar_string( AWAR_SAI_GLOBAL, "" ,   GLOBAL_gb_main);
    awr->awar_string( AWAR_MARKED_SPECIES_COUNTER, "unknown" ,  GLOBAL_gb_main);
    awr->awar_string( AWAR_INFO_BUTTON_TEXT, "Species Info" ,  GLOBAL_gb_main);
    awr->awar(AWAR_SPECIES_NAME)->add_callback(AWAR_INFO_BUTTON_TEXT_change_cb);
    awr->awar_int( AWAR_NTREE_TITLE_MODE, 1);

    awr->awar_string(AWAR_SAI_COLOR_STR, "", GLOBAL_gb_main); //sai visualization in probe match

    GEN_create_awars(awr, def);
    EXP_create_awars(awr, def);
#if defined(DEBUG)
    AWT_create_db_browser_awars(awr, def);
#endif // DEBUG

    AW_create_namesadmin_awars(awr, GLOBAL_gb_main);

    awr->awar_int( AWAR_SECURITY_LEVEL, 0, def);
    awr->awar(AWAR_SECURITY_LEVEL)->add_callback(nt_changesecurity);
#if defined(DEBUG) && 0
    awr->awar(AWAR_SECURITY_LEVEL)->write_int(6); // no security for debugging..
#endif // DEBUG

    create_insertchar_variables(awr,def);
    create_probe_design_variables(awr,def,GLOBAL_gb_main);
    create_primer_design_variables(awr,def, GLOBAL_gb_main);
    create_trees_var(awr,def);
    create_species_var(awr,def);
    create_consensus_var(awr,def);
    create_gde_var(awr,def);
    create_cprofile_var(awr,def);
    NT_create_transpro_variables(awr,def);
    NT_build_resort_awars(awr,def);
    NT_create_trackAliChanges_Awars(awr, GLOBAL_gb_main);

    NT_create_alignment_vars(awr,def);
    create_nds_vars(awr,def,GLOBAL_gb_main);
    create_export_nds_awars(awr,def);
    AWTC_create_rename_awars(awr,def);
    awt_create_dtree_awars(awr,GLOBAL_gb_main);

    awr->awar_string( AWAR_ERROR_MESSAGES, "", GLOBAL_gb_main);
    awr->awar_string( AWAR_DB_COMMENT, "<no description>", GLOBAL_gb_main);

    AWTC_create_submission_variables(awr, GLOBAL_gb_main);
    NT_createConcatenationAwars(awr,def);
    NT_createValidNamesAwars(awr,def); // lothar
    SQ_create_awars(awr, def);

    GB_ERROR error = ARB_init_global_awars(awr, def, GLOBAL_gb_main);
    if (!error) {
        AW_awar *awar_expert = awr->awar(AWAR_EXPERT);
        awar_expert->add_callback(expert_mode_changed_cb);
        awar_expert->touch();
        
        awt_create_aww_vars(awr,def);
    }

    if (error) aw_message(error);
}


void nt_exit(AW_window *aws) {
    if (GLOBAL_gb_main) {
        if (GB_read_clients(GLOBAL_gb_main)>=0) {
            if (GB_read_clock(GLOBAL_gb_main) > GB_last_saved_clock(GLOBAL_gb_main)){
                long secs;
                secs = GB_last_saved_time(GLOBAL_gb_main);

#if defined(DEBUG)
                secs =  GB_time_of_day(); // simulate "just saved"
#endif // DEBUG

                if (secs) {
                    secs = GB_time_of_day() - secs;
                    if (secs>10){
                        sprintf(AW_ERROR_BUFFER,"You last saved your data %li:%li minutes ago\nSure to quit ?",secs/60,secs%60);
                        if (aw_question(AW_ERROR_BUFFER, "QUIT ARB,DO NOT QUIT")) return;
                    }
                }
                else {
                    if (aw_question("You never saved any data\nSure to quit ???", "QUIT ARB,DO NOT QUIT")) return;
                }
            }
        }
        GBCMS_shutdown(GLOBAL_gb_main);

        GBDATA *gb_main = GLOBAL_gb_main;
        GLOBAL_gb_main  = NULL;                     // avoid further usage

        AW_root *aw_root = aws->get_root();
#if defined(DEVEL_RALF)
#warning instead of directly calling the following functions, try to add them as GB_atclose-callbacks
#endif // DEVEL_RALF
        aw_root->unlink_awars_from_DB(gb_main);
        AWT_destroy_input_masks();
#if defined(DEBUG)
        AWT_browser_forget_db(gb_main);
#endif // DEBUG

        GB_close(gb_main);
    }
    exit(0);
}

void NT_save_cb(AW_window *aww) {
    char *filename = aww->get_root()->awar(AWAR_DB_PATH)->read_string();
    GB_ERROR error = GB_save(GLOBAL_gb_main, filename, "b");
    delete filename;
    if (error) aw_message(error);
    else awt_refresh_selection_box(aww->get_root(), "tmp/nt/arbdb");
}


void NT_save_quick_cb(AW_window *aww) {
    char *filename = aww->get_root()->awar(AWAR_DB_PATH)->read_string();
    GB_ERROR error = GB_save_quick(GLOBAL_gb_main,filename);
    free(filename);
    if (error) aw_message(error);
    else awt_refresh_selection_box(aww->get_root(), "tmp/nt/arbdb");
}

void NT_save_quick_as_cb(AW_window *aww) {
    char     *filename = aww->get_root()->awar(AWAR_DB_PATH)->read_string();
    GB_ERROR  error    = GB_save_quick_as(GLOBAL_gb_main, filename);

    awt_refresh_selection_box(aww->get_root(), "tmp/nt/arbdb");
    aww->hide_or_notify(error);
    
    free(filename);
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

void NT_database_optimization(AW_window *aww) {
    GB_ERROR error = 0;
    GB_push_my_security(GLOBAL_gb_main);
    
    {
        aw_openstatus("Optimizing sequence data");

        error = GB_begin_transaction(GLOBAL_gb_main);
        if (!error) {
            char **ali_names = GBT_get_alignment_names(GLOBAL_gb_main);

            aw_status("Checking Sequence Lengths");
            error = GBT_check_data(GLOBAL_gb_main,0);
            error = GB_end_transaction(GLOBAL_gb_main, error);

            if (!error) {
                char *tree_name = aww->get_root()->awar("tmp/nt/arbdb/optimize_tree_name")->read_string();
                for (char **ali_name = ali_names; !error && *ali_name; ali_name++) {
                    aw_status(*ali_name);
                    error = GBT_compress_sequence_tree2(GLOBAL_gb_main,tree_name,*ali_name);
                }
                free(tree_name);
            }
            GBT_free_names(ali_names);
        }
        aw_closestatus();
    }

    if (!error) {
        aw_openstatus("Optimizing non-sequence data");
        error = GB_optimize(GLOBAL_gb_main);
        aw_closestatus();
    }

    GB_pop_my_security(GLOBAL_gb_main);
    aww->hide_or_notify(error);
}

AW_window *NT_create_database_optimization_window(AW_root *aw_root){
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;
    GB_transaction dummy(GLOBAL_gb_main);

    char *largest_tree = GBT_find_largest_tree(GLOBAL_gb_main);
    aw_root->awar_string("tmp/nt/arbdb/optimize_tree_name",largest_tree);
    free(largest_tree);

    aws = new AW_window_simple;
    aws->init( aw_root, "OPTIMIZE_DATABASE", "OPTIMIZE DATABASE");
    aws->load_xfig("optimize.fig");

    aws->at("trees");
    awt_create_selection_list_on_trees(GLOBAL_gb_main,(AW_window *)aws,"tmp/nt/arbdb/optimize_tree_name");

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

void NT_save_as_cb(AW_window *aww) {
    char *filename = aww->get_root()->awar(AWAR_DB_PATH)->read_string();
    char *filetype = aww->get_root()->awar(AWAR_DB"type")->read_string();

    GB_ERROR error = GB_save(GLOBAL_gb_main, filename, filetype);
    awt_refresh_selection_box(aww->get_root(), "tmp/nt/arbdb");

    aww->hide_or_notify(error);

    free(filetype);
    free(filename);
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
    GB_ERROR error = GB_undo(GLOBAL_gb_main,(GB_UNDO_TYPE)undo_type);
    if (error) aw_message(error);
    else{
        GB_transaction dummy(GLOBAL_gb_main);
        ((AWT_canvas *)ntw)->refresh();
    }
}

void NT_undo_info_cb(AW_window *,AW_CL undo_type){
    char *undo_info = GB_undo_info(GLOBAL_gb_main,(GB_UNDO_TYPE)undo_type);
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

enum streamSource { FROM_PIPE, FROM_FILE };
static char *stream2str(streamSource source, const char *commandOrFile) {
    char *output = 0;
    FILE *in     = 0;

    switch (source) {
        case FROM_PIPE: in = popen(commandOrFile, "r"); break;
        case FROM_FILE: in = fopen(commandOrFile, "rt"); break;
    }

    if (in) {
        GBS_strstruct *out = GBS_stropen(2000);
        int c;

        while (EOF != (c = fgetc(in))) GBS_chrcat(out, c);

        switch (source) {
            case FROM_PIPE: pclose(in); break;
            case FROM_FILE: fclose(in); break;
        }

        // skip trailing linefeeds
        long offset = GBS_memoffset(out);
        while (offset>0 && GBS_mempntr(out)[offset-1] == '\n') {
            GBS_str_cut_tail(out, 1);
            offset--;
        }

        output = GBS_strclose(out);
    }
    return output;
}

inline void removeTrailingNewlines(char *str) {
    char *eos = strchr(str, 0)-1;
    while (eos >= str && eos[0] == '\n') *eos-- = 0;
}

inline void append_named_value(GBS_strstruct *out, const char *prefix, const char *value) {
    GBS_strcat(out, GBS_global_string("%-*s: %s\n", 12, prefix, value));
}
inline void append_command_output(GBS_strstruct *out, const char *prefix, const char *command) {
    char *output = stream2str(FROM_PIPE, command);
    if (output) {
        removeTrailingNewlines(output);
        if (strcmp(output, "unknown") != 0) append_named_value(out, prefix, output);
        free(output);
    }
}

static void append_existing_file(GBS_strstruct *out, const char *file) {
    char *content = stream2str(FROM_FILE, file);
    if (content) {
        GBS_strcat(out, file);
        GBS_strcat(out, ":");
        GBS_chrcat(out, strchr(content, '\n') ? '\n' : ' ');
        GBS_strcat(out, content);
        GBS_chrcat(out, '\n');
        free(content);
    }
}

static char *get_system_info(bool extended) {
    GBS_strstruct *info = GBS_stropen(2000);

    GBS_strcat(info,
               "\n\n"
               "----------------------------------------\n"
               "Information about your system:\n"
               "----------------------------------------\n"
               );
    append_named_value(info, "ARB version", ARB_VERSION);
#if defined(SHOW_WHERE_BUILD)
    append_named_value(info, "build by", ARB_BUILD_USER "@" ARB_BUILD_HOST);
#endif // SHOW_WHERE_BUILD
    append_named_value(info, "ARBHOME", GB_getenvARBHOME());

    GBS_chrcat(info, '\n');
    append_command_output(info, "Kernel",   "uname -srv");
    append_command_output(info, "Machine",  "uname -m");
    append_command_output(info, "CPU",      "uname -p");
    append_command_output(info, "Platform", "uname -i");
    append_command_output(info, "OS",       "uname -o");

    GBS_chrcat(info, '\n');
    append_existing_file(info, "/proc/version");
    append_existing_file(info, "/proc/version_signature");
    if (extended) {
        append_existing_file(info, "/proc/cpuinfo");
        append_existing_file(info, "/proc/meminfo");
    }

    return GBS_strclose(info);
}

void NT_submit_mail(AW_window *aww, AW_CL cl_awar_base) {
    char     *mail_file;
    char     *mail_name = GB_unique_filename("arb_bugreport", "txt");
    FILE     *mail      = GB_fopen_tempfile(mail_name, "wt", &mail_file);
    GB_ERROR  error     = 0;

    if (!mail) error = GB_await_error();
    else {
        char *awar_base    = (char *)cl_awar_base;
        char *address      = aww->get_root()->awar(GBS_global_string("%s/address", awar_base))->read_string();
        char *text         = aww->get_root()->awar(GBS_global_string("%s/text", awar_base))->read_string();
        char *plainaddress = GBS_string_eval(address, "\"=:'=\\=", 0);                                    // Remove all dangerous symbols

        fprintf(mail, "%s\n", text);
        fclose(mail);

        const char *command = GBS_global_string("mail '%s' <%s", plainaddress, mail_file);

        gb_assert(GB_is_privatefile(mail_file, GB_FALSE));

        error = GB_system(command);
        GB_unlink_or_warn(mail_file, &error);

        free(plainaddress);
        free(text);
        free(address);
    }

    aww->hide_or_notify(error);

    free(mail_file);
    free(mail_name);
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
        char *system_info = get_system_info(bug_report);
        char *custom_text = 0;

        if (bug_report){
            custom_text = strdup("Bug occurred in:\n"
                                 "    [which part of ARB?]\n"
                                 "\n"
                                 "The bug [ ] is reproducible\n"
                                 "        [ ] occurs randomly\n"
                                 "        [ ] occurs with specific data\n"
                                 "\n"
                                 "Detailed description:\n"
                                 "[put your bug description here]\n"
                                 "\n"
                                 "\n"
                                 "\n"
                                 "Note: If the bug only occurs with a specific database,\n"
                                 "      please provide a way how we can download your database.\n"
                                 "      (e.g. upload it to a filesharing site and provide the URL here)\n"
                                 "\n"
                                 );
        }
        else {
            custom_text = GBS_global_string_copy("******* Registration *******\n"
                                                 "\n"
                                                 "Name           : %s\n"
                                                 "Department     : \n"
                                                 "How many users : \n"
                                                 "\n"
                                                 "Why do you want to use arb ?\n"
                                                 "\n",
                                                 GB_getenvUSER());
        }

        const char *awar_name_text = GBS_global_string("%s/text", awar_name_start);
        aw_root->awar_string(awar_name_text, GBS_global_string("%s\n%s", custom_text, system_info));
        aws->at("box");
        aws->create_text_field(awar_name_text);

        free(custom_text);
        free(system_info);
    }

    aws->at("go");
    aws->callback(NT_submit_mail, (AW_CL)awar_name_start); // do not free awar_name_start
    aws->create_button("SEND","SEND");

    awss[bug_report] = aws; // store for further use

    char *haveMail = GB_executable("mail");
    if (haveMail) free(haveMail);
    else aw_message("Won't be able to send your mail (no mail program found)\nCopy&paste the text to your favorite mail software");

    return aws;
}

void NT_focus_cb(AW_window *aww)
{
    AWUSE(aww);
    GB_transaction dummy(GLOBAL_gb_main);
}


void NT_modify_cb(AW_window *aww,AW_CL cd1,AW_CL cd2)
{
    AW_window *aws = NT_create_species_window(aww->get_root());
    aws->activate();
    nt_mode_event(aww,(AWT_canvas*)cd1,(AWT_COMMAND_MODE)cd2);
}

void NT_primer_cb(void) {
    GB_ERROR error = GB_xcmd("arb_primer",GB_TRUE, GB_FALSE);
    if (error) aw_message(error);
}


void NT_set_compression(AW_window *, AW_CL dis_compr, AW_CL){
    GB_transaction dummy(GLOBAL_gb_main);
    GB_ERROR       error = GB_set_compression(GLOBAL_gb_main,dis_compr);
    if (error) aw_message(error);
}


void NT_mark_degenerated_branches(AW_window *aww, AW_CL ntwcl) {
    char *val = aw_input("Enter degeneration factor [ 1 .. ]");
    if (val) {
        AWT_canvas     *ntw = (AWT_canvas *)ntwcl;
        GB_transaction  dummy(ntw->gb_main);
        
        NT_mark_all_cb(aww,(AW_CL)ntw, (AW_CL)0);
        AWT_TREE(ntw)->tree_root->mark_degenerated_branches(ntw->gb_main,atof(val));
        AWT_TREE(ntw)->tree_root->compute_tree(ntw->gb_main);
        free(val);
        ntw->refresh();
    }
}

void NT_mark_deep_branches(AW_window *aww, AW_CL ntwcl) {
    char *val = aw_input("Enter depth [ 1 .. ]");
    if (val) {
        AWT_canvas     *ntw = (AWT_canvas *)ntwcl;
        GB_transaction  dummy(ntw->gb_main);
        
        NT_mark_all_cb(aww,(AW_CL)ntw, (AW_CL)0);
        AWT_TREE(ntw)->tree_root->mark_deep_branches(ntw->gb_main,atoi(val));
        AWT_TREE(ntw)->tree_root->compute_tree(ntw->gb_main);
        free(val);
        ntw->refresh();
    }
}

void NT_mark_long_branches(AW_window *aww, AW_CL ntwcl){
    char *val = aw_input("Enter min.rel.diff.(%),min.abs.diff");
    if (val) {
        GB_ERROR  error = 0;
        char     *comma = strchr(val, ',');

        if (!comma) error = "Expected ','";
        else {
            float min_rel_diff = atof(val)/100.0;
            float min_abs_diff = atof(comma+1);

            AWT_canvas     *ntw = (AWT_canvas *)ntwcl;
            GB_transaction  dummy(ntw->gb_main);

            NT_mark_all_cb(aww,(AW_CL)ntw, (AW_CL)0);
            AWT_TREE(ntw)->tree_root->mark_long_branches(ntw->gb_main, min_rel_diff, min_abs_diff);
            AWT_TREE(ntw)->tree_root->compute_tree(ntw->gb_main);
            ntw->refresh();
        }
        if (error) aw_message(error);
        free(val);
    }
}

void NT_mark_duplicates(AW_window *aww, AW_CL ntwcl){
    AWT_canvas *ntw = (AWT_canvas *)ntwcl;
    GB_transaction dummy(ntw->gb_main);
    NT_mark_all_cb(aww,(AW_CL)ntw, (AW_CL)0);
    AWT_TREE(ntw)->tree_root->mark_duplicates(ntw->gb_main);
    AWT_TREE(ntw)->tree_root->compute_tree(ntw->gb_main);
    ntw->refresh();
}

void NT_justify_branch_lenghs(AW_window *, AW_CL cl_ntw, AW_CL){
    AWT_canvas *ntw = (AWT_canvas *)cl_ntw;
    GB_transaction dummy(ntw->gb_main);
    if (AWT_TREE(ntw)->tree_root){
        AWT_TREE(ntw)->tree_root->justify_branch_lenghs(ntw->gb_main);
        AWT_TREE(ntw)->tree_root->compute_tree(ntw->gb_main);
        GB_ERROR error = AWT_TREE(ntw)->save(ntw->gb_main,0,0,0);
        if (error) aw_message(error);
        ntw->refresh();
    }
}

void NT_fix_database(AW_window *) {
    GB_ERROR err = 0;
    err = GB_fix_database(GLOBAL_gb_main);
    if (err) aw_message(err);
}

static void relink_pseudo_species_to_organisms(GBDATA *&ref_gb_node, char *&ref_name, GB_HASH *organism_hash) {
    if (ref_gb_node) {
        if (GEN_is_pseudo_gene_species(ref_gb_node)) {
            GBDATA *gb_organism = GEN_find_origin_organism(ref_gb_node, organism_hash);
            
            if (gb_organism) {
                char *name = GBT_read_string(gb_organism, "name");

                if (name) {
                    freeset(ref_name, name);
                    ref_gb_node = gb_organism;
                }
            }
        }
    }
}

void NT_pseudo_species_to_organism(AW_window *, AW_CL ntwcl){
    AWT_canvas     *ntw = (AWT_canvas *)ntwcl;
    GB_transaction  dummy(ntw->gb_main);
    if (AWT_TREE(ntw)->tree_root){
        GB_HASH *organism_hash = GBT_create_organism_hash(ntw->gb_main);
        AWT_TREE(ntw)->tree_root->relink_tree(ntw->gb_main, relink_pseudo_species_to_organisms, organism_hash);
        AWT_TREE(ntw)->tree_root->compute_tree(ntw->gb_main);
        GB_ERROR error = AWT_TREE(ntw)->save(ntw->gb_main,0,0,0);
        if (error) aw_message(error);
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
            GB_transaction dummy(GLOBAL_gb_main);
            gb_species = GBT_find_species(GLOBAL_gb_main,species_name);
        }

        free(species_name);
        return gb_species;
    }
    virtual const char *getKeyPath() const { // give the keypath for items
        return CHANGE_KEY_PATH;
    }
};

static nt_item_type_species_selector item_type_species;

static void NT_open_mask_window(AW_window *aww, AW_CL cl_id, AW_CL) {
    int                              id         = int(cl_id);
    const awt_input_mask_descriptor *descriptor = AWT_look_input_mask(id);
    nt_assert(descriptor);
    if (descriptor) AWT_initialize_input_mask(aww->get_root(), GLOBAL_gb_main, &item_type_species, descriptor->get_internal_maskname(), descriptor->is_local_mask());
}

static void NT_create_mask_submenu(AW_window_menu_modes *awm) {
    AWT_create_mask_submenu(awm, AWT_IT_SPECIES, NT_open_mask_window);
}
static AW_window *NT_create_species_colorize_window(AW_root *aw_root) {
    return awt_create_item_colorizer(aw_root, GLOBAL_gb_main, &AWT_species_selector);
}

void NT_update_marked_counter(AW_window *aww, long count) {
    const char *buffer = count ? GBS_global_string("%li marked", count) : "";
    aww->get_root()->awar(AWAR_MARKED_SPECIES_COUNTER)->write_string(buffer);
}

static void nt_count_marked(AW_window *aww) {
#if defined(DEBUG)
//     printf("Counting marked species\n");
#endif // DEBUG
    long count = GBT_count_marked_species(GLOBAL_gb_main);
    NT_update_marked_counter(aww, count);
}

static void nt_auto_count_marked_species(GBDATA*, int* cl_aww, GB_CB_TYPE ) {
    nt_count_marked((AW_window*)cl_aww);
}

void NT_popup_species_window(AW_window *aww, AW_CL, AW_CL) {
    // used to avoid that the species info window is stored in a menu (or with a button)
    NT_create_species_window(aww->get_root())->activate();
}

//--------------------------------------------------------------------------------------------------

void NT_alltree_remove_leafs(AW_window *, AW_CL cl_mode, AW_CL cl_gb_main) {
    GBDATA               *gb_main = (GBDATA*)cl_gb_main;
    GBT_TREE_REMOVE_TYPE  mode    = (GBT_TREE_REMOVE_TYPE)cl_mode;

    GB_transaction ta(gb_main);

    int    treeCount;
    char **tree_names = GBT_get_tree_names_and_count(gb_main, &treeCount);

    if (tree_names) {
        aw_openstatus("Deleting from trees");
        GB_HASH *species_hash = GBT_create_species_hash(gb_main);

        for (int t = 0; t<treeCount; t++) {
            GB_ERROR  error = 0;
            aw_status(t/double(treeCount));
            aw_status(tree_names[t]);
            GBT_TREE *tree = GBT_read_tree(gb_main, tree_names[t], -sizeof(GBT_TREE));
            if (!tree) {
                aw_message(GBS_global_string("Can't load tree '%s' - skipped", tree_names[t]));
            }
            else {
                int removed        = 0;
                int groups_removed = 0;

                tree = GBT_remove_leafs(tree, mode, species_hash, &removed, &groups_removed);

                gb_assert(removed >= groups_removed);

                if (!tree) {
                    aw_message(GBS_global_string("'%s' would disappear. Please delete tree manually.", tree_names[t]));
                }
                else {
                    if (removed>0) {
                        error = GBT_write_tree(gb_main, 0, tree_names[t], tree);
                        if (groups_removed>0) {
                            aw_message(GBS_global_string("Removed %i species and %i groups from '%s'", removed, groups_removed, tree_names[t]));
                        }
                        else {
                            aw_message(GBS_global_string("Removed %i species from '%s'", removed, tree_names[t]));
                        }
                    }
                    GBT_delete_tree(tree);
                }
            }
            if (error) aw_message(error);
        }
        aw_status(1.0);

        GBT_free_names(tree_names);
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

AW_window * create_nt_main_window(AW_root *awr, AW_CL clone){
    GB_push_transaction(GLOBAL_gb_main);
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

    GLOBAL_NT.tree = NT_generate_tree(awr,GLOBAL_gb_main);

    AWT_canvas *ntw;
    {
        AP_tree_sort old_sort_type = GLOBAL_NT.tree->tree_sort;
        GLOBAL_NT.tree->set_tree_type(AP_LIST_SIMPLE); // avoid NDS warnings during startup

        ntw = new AWT_canvas(GLOBAL_gb_main, awm, GLOBAL_NT.tree, aw_gc_manager, awar_tree);
        GLOBAL_NT.tree->set_tree_type(old_sort_type);
        ntw->set_mode(AWT_MODE_SELECT);
    }

    {
        char *tree_name          = awr->awar_string(awar_tree)->read_string();
        char *existing_tree_name = GBT_existing_tree(GLOBAL_gb_main,tree_name);

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

    GBDATA *gb_arb_presets =    GB_search(GLOBAL_gb_main,"arb_presets",GB_CREATE_CONTAINER);
    GB_add_callback(gb_arb_presets,GB_CB_CHANGED,(GB_CB)AWT_expose_cb, (int *)ntw);

    bool is_genome_db = GEN_is_genome_db(GLOBAL_gb_main, 0); //  is this a genome database ? (default = 0 = not a genom db)

    // -------------
    //      File
    // -------------
#if defined(DEBUG)
    AWT_create_debug_menu(awm);
#endif // DEBUG

    if (clone){
        awm->create_menu("File", "F", AWM_ALL);
        AWMIMT("close", "Close", "C", 0, AWM_ALL, (AW_CB)AW_POPDOWN, 0, 0);
    }
    else {
#if defined(DEBUG)
        // add more to debug-menu
        SEP________________________SEP();
        AWMIMT("fix_db",      "Fix database",            "F", 0, AWM_ALL, (AW_CB)NT_fix_database,            0,                                       0);
        AWMIMT("debug_arbdb", "Print debug information", "d", 0, AWM_ALL, (AW_CB)GB_print_debug_information, (AW_CL)                  GLOBAL_gb_main, 0);
        AWMIMT("test_compr",  "Test compression",        "T", 0, AWM_ALL, (AW_CB)GBT_compression_test,       (AW_CL)                  GLOBAL_gb_main, 0);
        SEP________________________SEP();
        AWMIMT("table_admin",       "Table Admin (unfinished/unknown purpose)",  "A","tableadm.hlp",     AWM_ALL, AW_POPUP,(AW_CL)AWT_create_tables_admin_window, (AW_CL)GLOBAL_gb_main);
#endif // DEBUG

        awm->create_menu("File", "F", AWM_ALL);
        {
            AWMIMT("save_changes", "Quicksave changes",          "s", "save.hlp",      AWM_ALL, (AW_CB)NT_save_quick_cb, 0, 0);
            AWMIMT("save_all_as",  "Save whole database as ...", "w", "save.hlp",      AWM_ALL, AW_POPUP,(AW_CL)NT_create_save_as, (AW_CL)"tmp/nt/arbdb");
            SEP________________________SEP();
            AWMIMT("new_window",   "New window",                 "N", "newwindow.hlp", AWM_ALL, AW_POPUP,(AW_CL)create_nt_main_window, clone+1);
            SEP________________________SEP();
            AWMIMT("optimize_db",  "Optimize database",          "O", "optimize.hlp",  AWM_ALL, AW_POPUP,(AW_CL)NT_create_database_optimization_window, 0);
            SEP________________________SEP();

            awm->insert_sub_menu("Import",      "I");
            {
                AWMIMT("import_seq", "Import sequences and fields","I","arb_import.hlp",AWM_ALL, NT_import_sequences,0,0);
                GDE_load_menu(awm,AWM_EXP,"Import");
            }
            awm->close_sub_menu();

            awm->insert_sub_menu("Export",      "E");
            {
                AWMIMT("export_to_ARB", "Export seq/tree/SAI's to new ARB database", "A", "arb_ntree.hlp",      AWM_ALL, (AW_CB)NT_system_cb, (AW_CL)"arb_ntree -export &",0);
                AWMIMT("export_seqs",   "Export sequences to foreign format",        "f", "arb_export.hlp",     AWM_ALL, AW_POPUP,            (AW_CL)open_AWTC_export_window, (AW_CL)GLOBAL_gb_main);
                GDE_load_menu(awm,AWM_ALL,"Export");
                AWMIMT("export_nds",    "Export fields using NDS",                   "N", "arb_export_nds.hlp", AWM_ALL, AW_POPUP,            (AW_CL)create_nds_export_window, 0);
            }
            awm->close_sub_menu();
            SEP________________________SEP();

            AWMIMT("macros",    "Macros ",              "M", "macro.hlp",   AWM_ALL,        (AW_CB)AW_POPUP,    (AW_CL)awt_open_macro_window,(AW_CL)"ARB_NT");

            awm->insert_sub_menu("Registration/Bug report/Version info",       "R" );
            {
                AWMIMT("registration", "Registration",                   "R", "registration.hlp", AWM_ALL, (AW_CB)AW_POPUP,      (AW_CL)NT_submit_bug, 0);
                AWMIMT("bug_report",   "Bug report",                     "B", "registration.hlp", AWM_ALL, (AW_CB)AW_POPUP,      (AW_CL)NT_submit_bug, 1);
                AWMIMT("version_info", "Version info (" ARB_VERSION ")", "V", "version.hlp",      AWM_ALL, (AW_CB)AW_POPUP_HELP, (AW_CL)"version.hlp", 0);
            }
            awm->close_sub_menu();
#if 0
            SEP________________________SEP();
            AWMIMT("undo",      "Undo",      "U", "undo.hlp", AWM_ALL, (AW_CB)NT_undo_cb,      (AW_CL)GB_UNDO_UNDO, (AW_CL)ntw);
            AWMIMT("redo",      "Redo",      "d", "undo.hlp", AWM_ALL, (AW_CB)NT_undo_cb,      (AW_CL)GB_UNDO_REDO, (AW_CL)ntw);
            AWMIMT("undo_info", "Undo info", "f", "undo.hlp", AWM_ALL, (AW_CB)NT_undo_info_cb, (AW_CL)GB_UNDO_UNDO, (AW_CL)0  );
            AWMIMT("redo_info", "Redo info", "o", "undo.hlp", AWM_ALL, (AW_CB)NT_undo_info_cb, (AW_CL)GB_UNDO_REDO, (AW_CL)0  );

            SEP________________________SEP();
#endif
            if (!GLOBAL_NT.extern_quit_button){
                AWMIMT( "quit",     "Quit",             "Q","quit.hlp",     AWM_ALL, (AW_CB)nt_exit,    0, 0 );
            }

        }
        // ----------------
        //      Species
        // ----------------
        awm->create_menu("Species", "c", AWM_ALL);
        {
            AWMIMT("species_search", "Search and query",    "q", "sp_search.hlp", AWM_ALL, AW_POPUP,                (AW_CL)ad_create_query_window, 0);
            AWMIMT("species_info",   "Species information", "i", "sp_info.hlp",   AWM_ALL, NT_popup_species_window, 0,                             0);

            SEP________________________SEP();

            NT_insert_mark_submenus(awm, ntw, 1);
            AWMIMT("species_colors",  "Set Colors",     "l", "mark_colors.hlp",   AWM_ALL, AW_POPUP,               (AW_CL)NT_create_species_colorize_window, 0);
            AWMIMT("selection_admin", "Configurations", "o", "configuration.hlp", AWM_ALL, NT_popup_configuration_admin, (AW_CL)&(GLOBAL_NT.tree->tree_root),      (AW_CL)0);

            SEP________________________SEP();

            awm->insert_sub_menu("Database fields admin","f");
            ad_spec_create_field_items(awm);
            awm->close_sub_menu();
            NT_create_mask_submenu(awm);

            SEP________________________SEP();

            AWMIMT("del_marked",    "Delete Marked Species",    "D","sp_del_mrkd.hlp",  AWM_ALL, (AW_CB)NT_delete_mark_all_cb,      (AW_CL)ntw, 0 );

            awm->insert_sub_menu("Sort Species",         "r");
            {
                AWMIMT("sort_by_field", "According to Database Entries", "D", "sp_sort_fld.hlp",  AWM_ALL, AW_POPUP,                           (AW_CL)NT_build_resort_window,       0);
                AWMIMT("sort_by_tree",  "According to Phylogeny",        "P", "sp_sort_phyl.hlp", AWM_ALL, (AW_CB)NT_resort_data_by_phylogeny, (AW_CL)&(GLOBAL_NT.tree->tree_root), 0);
            }
            awm->close_sub_menu();

            awm->insert_sub_menu("Merge Species", "g", AWM_EXP);
            {
                AWMIMT("merge_species", "Create merged species from similar species", "m", "sp_merge.hlp",     AWM_EXP, AW_POPUP, (AW_CL)NT_createMergeSimilarSpeciesWindow, 0);
                AWMIMT("join_marked",   "Join Marked Species",                        "J", "join_species.hlp", AWM_EXP, AW_POPUP, (AW_CL)create_species_join_window,         0);
            }
            awm->close_sub_menu();

            SEP________________________SEP();

            AWMIMT( "species_submission", "Submit Species", "b",  "submission.hlp",   AWM_EXP, AW_POPUP,   (AW_CL)AWTC_create_submission_window,   0 );

            SEP________________________SEP();

            AWMIMT( "new_names",    "Generate New Names",   "e", "sp_rename.hlp",   AWM_ALL, AW_POPUP,   (AW_CL)AWTC_create_rename_window,      (AW_CL)GLOBAL_gb_main );

            awm->insert_sub_menu("Valid Names ...", "V", AWM_EXP);
            {
                AWMIMT("imp_names",    "Import names from file", "I", "vn_import.hlp",  AWM_EXP, NT_importValidNames,  0, 0);
                AWMIMT("del_names",    "Delete names from DB",   "D", "vn_delete.hlp",  AWM_EXP, NT_deleteValidNames,  0, 0);
                AWMIMT("sug_names",    "Suggest valid names",    "v", "vn_suggest.hlp", AWM_EXP, NT_suggestValidNames, 0, 0);
                AWMIMT("search_names", "Search manually",        "m", "vn_search.hlp",  AWM_EXP, AW_POPUP,             (AW_CL)NT_searchManuallyNames , 0);
           }
            awm->close_sub_menu();
        }
        //  --------------------------
        //      Genes + Experiment
        //  --------------------------

        if (is_genome_db) GEN_create_genes_submenu(awm, true/*, ntw*/);

        // -----------------
        //      Sequence
        // -----------------
        awm->create_menu("Sequence", "S", AWM_ALL);
        {
            AWMIMT("seq_admin",   "Sequence/Alignment Admin", "A", "ad_align.hlp",   AWM_ALL,  AW_POPUP, (AW_CL)NT_create_alignment_window, 0);
            AWMIMT("ins_del_col", "Insert/Delete Column",     "I", "insdelchar.hlp", AWM_ALL,  AW_POPUP, (AW_CL)create_insertchar_window,   0);
            SEP________________________SEP();

            awm->insert_sub_menu("Edit Sequences","E");
            {
                AWMIMT("new_arb_edit4",  "Using marked species and tree", "m", "arb_edit4.hlp", AWM_ALL, (AW_CB)NT_start_editor_on_tree, (AW_CL)&(GLOBAL_NT.tree->tree_root),         0);
                AWMIMT("new2_arb_edit4", "... plus relatives",            "r", "arb_edit4.hlp", AWM_ALL, (AW_CB)NT_start_editor_on_tree, (AW_CL)&(GLOBAL_NT.tree->tree_root),         -1);
                AWMIMT("old_arb_edit4",  "Using earlier configuration",   "c", "arb_edit4.hlp", AWM_ALL, AW_POPUP,                       (AW_CL)NT_start_editor_on_old_configuration, 0);
            }
            awm->close_sub_menu();

            awm->insert_sub_menu("Other sequence editors", "O", AWM_EXP);
            {
                AWMIMT("arb_edit", "ARB Editor (old)", "o", "arb_edit.hlp", AWM_EXP, (AW_CB)NT_system_cb, (AW_CL)"arb_edit &",  0);
            }
            awm->close_sub_menu();
            SEP________________________SEP();

            awm->insert_sub_menu("Align Sequences",  "S");
            {
                AWMIMT("arb_align",   "Align sequence into an existing alignment",         "A", "align.hlp",       AWM_EXP, (AW_CB) AW_POPUP_HELP, (AW_CL)"align.hlp",                  0);
                AWMIMT("realign_dna", "Realign nucleic acid according to aligned protein", "R", "realign_dna.hlp", AWM_ALL, AW_POPUP,              (AW_CL)NT_create_realign_dna_window, 0);
                GDE_load_menu(awm,AWM_ALL,"Align");
            }
            awm->close_sub_menu();
            AWMIMT("seq_concat",    "Concatenate Sequences/Alignments", "C", "concatenate_align.hlp", AWM_ALL, AW_POPUP, (AW_CL)NT_createConcatenationWindow,     0);
            AWMIMT("track_changes", "Track alignment changes",          "k", "track_ali_changes.hlp", AWM_EXP, AW_POPUP, (AW_CL)NT_create_trackAliChanges_window, 0);
            SEP________________________SEP();

            AWMIMT("dna_2_pro", "Perform translation", "t", "translate_dna_2_pro.hlp", AWM_ALL, AW_POPUP,            (AW_CL)NT_create_dna_2_pro_window, 0);
            AWMIMT("arb_dist",  "Distance matrix",     "D", "dist.hlp",                AWM_ALL, (AW_CB)NT_system_cb, (AW_CL)"arb_dist &",               0);
            SEP________________________SEP();

            AWMIMT("seq_quality",   "Check Sequence Quality", "Q", "seq_quality.hlp",   AWM_EXP, AW_POPUP, (AW_CL)SQ_create_seq_quality_window,   0);
            AWMIMT("chimera_check", "Chimera Check",          "m", "check_quality.hlp", AWM_EXP, AW_POPUP, (AW_CL)st_create_quality_check_window, (AW_CL)GLOBAL_gb_main);

            SEP________________________SEP();

            GDE_load_menu(awm,AWM_ALL,"Print");
        }
        // ------------
        //      SAI
        // ------------
        awm->create_menu("SAI", "A", AWM_ALL);
        {
            AWMIMT("sai_admin", "Manage SAIs",                "S","ad_extended.hlp",  AWM_ALL,    AW_POPUP, (AW_CL)NT_create_extendeds_window,   0 );
            awm->insert_sub_menu("Create SAI using ...", "C");
            {
                AWMIMT("sai_max_freq",  "Max. Frequency",                                               "M", "max_freq.hlp",     AWM_EXP, AW_POPUP,                     (AW_CL)AP_open_max_freq_window,     0);
                AWMIMT("sai_consensus", "Consensus",                                                    "C", "consensus.hlp",    AWM_ALL, AW_POPUP,                     (AW_CL)AP_open_con_expert_window,   0);
                AWMIMT("pos_var_pars",  "Positional variability + Column statistic (parsimony method)", "a", "pos_var_pars.hlp", AWM_ALL, AW_POPUP,                     (AW_CL)AP_open_pos_var_pars_window, 0);
                AWMIMT("arb_phyl",      "Filter by base frequency",                                     "F", "phylo.hlp",        AWM_ALL, (AW_CB)NT_system_cb,          (AW_CL)"arb_phylo &",               0);
                AWMIMT("sai_pfold",     "Protein secondary structure (field \"sec_struct\")",           "P", "pfold.hlp",        AWM_EXP, (AW_CB)create_sai_from_pfold, (AW_CL)ntw,                         0);

                GDE_load_menu(awm,AWM_EXP,"SAI");
            }
            awm->close_sub_menu();

            awm->insert_sub_menu("Other functions", "O");
            {
                AWMIMT("pos_var_dist",          "Positional variability (distance method)",     "P", "pos_variability.ps", AWM_EXP, AW_POPUP,      (AW_CL)AP_open_cprofile_window,          0);
                AWMIMT("count_different_chars", "Count different chars/column",                 "C", "count_chars.hlp",    AWM_EXP, NT_system_cb2, (AW_CL)"arb_count_chars",                0);
                AWMIMT("export_pos_var",        "Export Column Statistic (GNUPLOT format)",     "E", "csp_2_gnuplot.hlp",  AWM_ALL, AW_POPUP,      (AW_CL)AP_open_csp_2_gnuplot_window,     0);
            }
            awm->close_sub_menu();
        }

        // ---------------
        //      Probes
        // ---------------
        awm->create_menu("Probes", "P", AWM_ALL);
        {
            AWMIMT("probe_design",      "Design Probes",             "D", "probedesign.hlp", AWM_ALL, AW_POPUP, (AW_CL)create_probe_design_window, (AW_CL)is_genome_db );
            AWMIMT("probe_multi",       "Calculate Multi-Probes",    "u", "multiprobe.hlp",  AWM_ALL, AW_POPUP, (AW_CL)MP_main, (AW_CL)ntw           );
            AWMIMT("probe_match",       "Match Probes",              "M", "probematch.hlp",  AWM_ALL, AW_POPUP, (AW_CL)create_probe_match_window, 0  );
            SEP________________________SEP();
            AWMIMT("primer_design_new", "Design Primers",            "P", "primer_new.hlp",  AWM_EXP, AW_POPUP, (AW_CL)create_primer_design_window, 0);
            AWMIMT("primer_design",     "Design Sequencing Primers", "S", "primer.hlp",      AWM_EXP, (AW_CB)NT_primer_cb, 0, 0                      );
            SEP________________________SEP();
            AWMIMT("pt_server_admin",   "PT_SERVER Admin",           "A", "probeadmin.hlp",  AWM_ALL, AW_POPUP, (AW_CL)create_probe_admin_window, (AW_CL)is_genome_db);
        }

    } // clone

    // -------------
    //      Tree
    // -------------
    awm->create_menu("Tree", "T", AWM_ALL);
    {
        if (!clone) {
            awm->insert_sub_menu("Add Species to Existing Tree", "A");
            {
                AWMIMT( "arb_pars_quick",  "ARB Parsimony (Quick add marked)", "Q", "pars.hlp",    AWM_ALL,   (AW_CB)NT_system_cb,    (AW_CL)"arb_pars -add_marked -quit &",0 );
                AWMIMT( "arb_pars",        "ARB Parsimony interactive",        "i", "pars.hlp",    AWM_ALL,   (AW_CB)NT_system_cb,    (AW_CL)"arb_pars &",    0 );
                GDE_load_menu(awm, AWM_EXP, "Incremental phylogeny");
            }
            awm->close_sub_menu();
        }

        awm->insert_sub_menu("Remove Species from Tree",     "R");
        {
            AWMIMT(awm->local_id("tree_remove_deleted"), "Remove zombies", "z", "trm_del.hlp",    AWM_ALL, (AW_CB)NT_remove_leafs, (AW_CL)ntw, AWT_REMOVE_DELETED );
            AWMIMT(awm->local_id("tree_remove_marked"),  "Remove marked",  "m", "trm_mrkd.hlp",   AWM_ALL, (AW_CB)NT_remove_leafs, (AW_CL)ntw, AWT_REMOVE_MARKED );
            AWMIMT(awm->local_id("tree_keep_marked"),    "Keep marked",    "K", "tkeep_mrkd.hlp", AWM_ALL, (AW_CB)NT_remove_leafs, (AW_CL)ntw, AWT_REMOVE_NOT_MARKED|AWT_REMOVE_DELETED);
#if defined(DEVEL_RALF)
#warning add "remove duplicates from tree"
            SEP________________________SEP();
            AWMIMT("all_tree_remove_deleted", "Remove zombies from all trees", "a", "trm_del.hlp", AWM_ALL, (AW_CB)NT_alltree_remove_leafs, AWT_REMOVE_DELETED, (AW_CL)GLOBAL_gb_main);
            AWMIMT("all_tree_remove_marked",  "Remove marked from all trees",  "l", "trm_del.hlp", AWM_ALL, (AW_CB)NT_alltree_remove_leafs, AWT_REMOVE_MARKED, (AW_CL)GLOBAL_gb_main);
#endif // DEVEL_RALF
        }
        awm->close_sub_menu();

        if (!clone) {
            awm->insert_sub_menu("Build tree from sequence data",    "B");
            {
                awm->insert_sub_menu("Distance matrix methods", "D");
                AWMIMT( "arb_dist",     "ARB Neighbour Joining",     "J", "dist.hlp",    AWM_ALL,   (AW_CB)NT_system_cb,    (AW_CL)"arb_dist &",    0 );
                GDE_load_menu(awm,AWM_ALL,"Phylogeny Distance Matrix");
                awm->close_sub_menu();

                awm->insert_sub_menu("Maximum Parsimony methods", "P");
                GDE_load_menu(awm,AWM_ALL,"Phylogeny max. parsimony");
                awm->close_sub_menu();

                awm->insert_sub_menu("Maximum Likelihood methods", "L");
                GDE_load_menu(awm,AWM_EXP,"Phylogeny max. Likelyhood EXP");
                GDE_load_menu(awm,AWM_ALL,"Phylogeny max. Likelyhood");
                awm->close_sub_menu();

                awm->insert_sub_menu("Other methods", "O", AWM_EXP);
                GDE_load_menu(awm,AWM_EXP,"Phylogeny (Other)");
                awm->close_sub_menu();
            }
            awm->close_sub_menu();
        }

        SEP________________________SEP();

        // if (!clone){
            awm->insert_sub_menu("Reset zoom",         "z");
            {
                AWMIMT(awm->local_id("reset_logical_zoom"),  "Logical zoom",  "L", "rst_log_zoom.hlp",  AWM_ALL, (AW_CB)NT_reset_lzoom_cb, (AW_CL)ntw, 0);
                AWMIMT(awm->local_id("reset_physical_zoom"), "Physical zoom", "P", "rst_phys_zoom.hlp", AWM_ALL, (AW_CB)NT_reset_pzoom_cb, (AW_CL)ntw, 0);
            }
            awm->close_sub_menu();
        // }
        awm->insert_sub_menu("Collapse/Expand tree",         "C");
        {
            AWMIMT(awm->local_id("tree_group_all"),         "Group all",               "a", "tgroupall.hlp",   AWM_ALL,  (AW_CB)NT_group_tree_cb,       (AW_CL)ntw, 0);
            AWMIMT(awm->local_id("tree_group_not_marked"),  "Group all except marked", "m", "tgroupnmrkd.hlp", AWM_ALL,  (AW_CB)NT_group_not_marked_cb, (AW_CL)ntw, 0);
            AWMIMT(awm->local_id("tree_group_term_groups"), "Group terminal groups",   "t", "tgroupterm.hlp",  AWM_ALL, (AW_CB)NT_group_terminal_cb,   (AW_CL)ntw, 0);
            AWMIMT(awm->local_id("tree_ungroup_all"),       "Ungroup all",             "U", "tungroupall.hlp", AWM_ALL,  (AW_CB)NT_ungroup_all_cb,      (AW_CL)ntw, 0);
            SEP________________________SEP();
            NT_insert_color_collapse_submenu(awm, ntw);
        }
        awm->close_sub_menu();
        awm->insert_sub_menu("Beautify tree", "e");
        {
            AWMIMT(awm->local_id("beautifyt_tree"), "#beautifyt.bitmap", "", "resorttree.hlp", AWM_ALL, (AW_CB)NT_resort_tree_cb, (AW_CL)ntw, 0);
            AWMIMT(awm->local_id("beautifyc_tree"), "#beautifyc.bitmap", "", "resorttree.hlp", AWM_ALL, (AW_CB)NT_resort_tree_cb, (AW_CL)ntw, 1);
            AWMIMT(awm->local_id("beautifyb_tree"), "#beautifyb.bitmap", "", "resorttree.hlp", AWM_ALL, (AW_CB)NT_resort_tree_cb, (AW_CL)ntw, 2);
        }
        awm->close_sub_menu();
        awm->insert_sub_menu("Modify branches", "M");
        {
            AWMIMT(awm->local_id("tree_remove_remark"),     "Remove bootstraps",       "b", "trm_boot.hlp",      AWM_ALL, NT_remove_bootstrap,      (AW_CL)ntw, 0);
            SEP________________________SEP();
            AWMIMT(awm->local_id("tree_reset_lengths"),     "Reset branchlengths",    "R", "tbl_reset.hlp",   AWM_ALL, NT_reset_branchlengths,   (AW_CL)ntw, 0);
            AWMIMT(awm->local_id("justify_branch_lengths"), "Justify branchlengths",  "J", "tbl_justify.hlp", AWM_ALL, NT_justify_branch_lenghs, (AW_CL)ntw, 0);
            AWMIMT(awm->local_id("tree_scale_lengths"),     "Scale Branchlengths",    "S", "tbl_scale.hlp",   AWM_ALL, NT_scale_tree,            (AW_CL)ntw, 0);
            SEP________________________SEP();
            AWMIMT(awm->local_id("tree_boot2len"),      "Bootstraps -> Branchlengths", "l", "tbl_boot2len.hlp", AWM_ALL, NT_move_boot_branch, (AW_CL)ntw, 0);
            AWMIMT(awm->local_id("tree_len2boot"),      "Bootstraps <- Branchlengths", "o", "tbl_boot2len.hlp", AWM_ALL, NT_move_boot_branch, (AW_CL)ntw, 1);

        }
        awm->close_sub_menu();
        AWMIMT(awm->local_id("mark_long_branches") , "Mark long branches",        "k", "mark_long_branches.hlp",  AWM_ALL, (AW_CB)NT_mark_long_branches,        (AW_CL)ntw, 0);
        AWMIMT(awm->local_id("mark_deep_branches") , "Mark deep branches",        "d", "mark_deep_branches.hlp",  AWM_EXP, (AW_CB)NT_mark_deep_branches,        (AW_CL)ntw, 0);
        AWMIMT(awm->local_id("mark_degen_branches"), "Mark degenerated branches", "g", "mark_degen_branches.hlp", AWM_EXP, (AW_CB)NT_mark_degenerated_branches, (AW_CL)ntw, 0);
        AWMIMT(awm->local_id("mark_duplicates")    , "Mark duplicates",           "u", "mark_duplicates.hlp",     AWM_ALL, (AW_CB)NT_mark_duplicates,           (AW_CL)ntw, 0);

        SEP________________________SEP();

        AWMIMT(awm->local_id("tree_select"),        "Select Tree",      "T", 0, AWM_ALL, AW_POPUP,(AW_CL)NT_open_select_tree_window, (AW_CL)awar_tree);
        AWMIMT(awm->local_id("tree_select_latest"), "Select Last Tree", "L", 0, AWM_ALL,          (AW_CB)NT_select_last_tree,        (AW_CL)awar_tree, 0);

        SEP________________________SEP();

        if (!clone){
            AWMIMT("tree_admin", "Tree admin",               "i", "treeadm.hlp",   AWM_ALL, AW_POPUP, (AW_CL)create_trees_window,   0);
            AWMIMT("nds",        "NDS (Node display setup)", "N", "props_nds.hlp", AWM_ALL, AW_POPUP, (AW_CL)AWT_create_nds_window, (AW_CL)GLOBAL_gb_main);
        }
        SEP________________________SEP();

        AWMIMT("transversion",       "Transversion analysis",   "y", "trans_anal.hlp", AWM_EXP, (AW_CB)AW_POPUP_HELP,       (AW_CL)"trans_anal.hlp", 0);

        SEP________________________SEP();

        if (!clone){
            AWMIMT("print_tree",  "Print tree",          "P", "tree2prt.hlp",  AWM_ALL, AWT_popup_print_window,       (AW_CL)ntw, 0);
            AWMIMT("tree_2_xfig", "Export tree to XFIG", "X", "tree2file.hlp", AWM_ALL, AWT_popup_tree_export_window, (AW_CL)ntw, 0);
            SEP________________________SEP();
        }

        if (is_genome_db) {
            awm->insert_sub_menu("Other..",  "O", AWM_EXP);
            {
                AWMIMT(awm->local_id("tree_pseudo_species_to_organism"), "Change pseudo species to organisms in tree", "p", "tree_pseudo.hlp",        AWM_EXP, (AW_CB)NT_pseudo_species_to_organism, (AW_CL)ntw, 0);
            }
            awm->close_sub_menu();
        }
    }

    if (!clone) {
        // --------------
        //      Tools
        // --------------
        awm->create_menu("Tools", "o", AWM_ALL);
        {
            AWMIMT("names_admin",      "Name server admin",    "s","namesadmin.hlp",   AWM_ALL, AW_POPUP, (AW_CL)AW_create_namesadmin_window, (AW_CL)GLOBAL_gb_main );
            awm->insert_sub_menu("DB admin", "D", AWM_EXP);
            {
                AWMIMT("db_admin", "Re-repair DB", "R", "rerepair.hlp", AWM_EXP, NT_rerepair_DB, (AW_CL)GLOBAL_gb_main, 0);
            }
            awm->close_sub_menu();
            awm->insert_sub_menu("Network", "N", AWM_EXP);
            {
                GDE_load_menu(awm,AWM_EXP,"User");
            }
            awm->close_sub_menu();
            SEP________________________SEP();

            awm->insert_sub_menu("GDE specials","G", AWM_EXP);
            {
                GDE_load_menu(awm,AWM_EXP,0,0);
            }
            awm->close_sub_menu();

            awm->insert_sub_menu("WL specials","W", AWM_EXP);
            {
                AWMIMT(awm->local_id("view_probe_group_result"), "View probe group result", "V", "",             AWM_EXP, AW_POPUP, (AW_CL)create_probe_group_result_window, (AW_CL)ntw);
            }
            awm->close_sub_menu();

            SEP________________________SEP();
            AWMIMT("xterm",         "Start XTERM",             "X",0   ,       AWM_ALL, (AW_CB)GB_xterm, 0, 0);
        }
        // -------------------
        //      Properties
        // -------------------
        awm->create_menu("Properties", "r", AWM_ALL);
        {
            AWMIMT("props_menu",    "Frame settings", "F","props_frame.hlp",      AWM_ALL, AW_POPUP, (AW_CL)AWT_preset_window, 0 );
            awm->insert_sub_menu("Tree settings",  "T");
            {
                AWMIMT(awm->local_id("props_tree2"), "Tree options",        "o", "nt_tree_settings.hlp", AWM_ALL, AW_POPUP, (AW_CL)NT_create_tree_setting, (AW_CL)ntw);
                AWMIMT("props_tree",                 "Tree colors & fonts", "c", "nt_props_data.hlp",    AWM_ALL, AW_POPUP, (AW_CL)AW_create_gc_window,    (AW_CL)aw_gc_manager);
            }
            awm->close_sub_menu();
            AWMIMT("props_www",      "Search world wide web (WWW)",                 "W","props_www.hlp",        AWM_ALL, AW_POPUP, (AW_CL)AWT_open_www_window,  (AW_CL)GLOBAL_gb_main );
            SEP________________________SEP();
            AWMIMT("enable_advices", "Reactivate advices",                          "R","advice.hlp", AWM_ALL, (AW_CB) AWT_reactivate_all_advices, 0, 0 );
            AWMIMT("!toggle_expert", "Toggle expert mode",                          "x",0,            AWM_ALL, NT_toggle_expert_mode, 0, 0 );
            SEP________________________SEP();
            AWMIMT("save_props",     "Save properties (in ~/.arb_prop/ntree.arb)",  "S","savedef.hlp",AWM_ALL, (AW_CB) AW_save_defaults, 0, 0 );
        }
    } // !clone

    awm->insert_help_topic("How to use help", "H", "help.hlp",      AWM_ALL, (AW_CB)AW_POPUP_HELP, (AW_CL)"help.hlp",      0);
    awm->insert_help_topic("ARB help",        "A", "arb.hlp",       AWM_ALL, (AW_CB)AW_POPUP_HELP, (AW_CL)"arb.hlp",       0);
    awm->insert_help_topic("ARB_NT help",     "N", "arb_ntree.hlp", AWM_ALL, (AW_CB)AW_POPUP_HELP, (AW_CL)"arb_ntree.hlp", 0);

    awm->create_mode("select.bitmap",   "mode_select.hlp", AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_SELECT);
    awm->create_mode("mark.bitmap",     "mode_mark.hlp",   AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_MARK);
    awm->create_mode("group.bitmap",    "mode_group.hlp",  AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_GROUP);
    awm->create_mode("pzoom.bitmap",    "mode_pzoom.hlp",  AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_ZOOM);
    awm->create_mode("lzoom.bitmap",    "mode_lzoom.hlp",  AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_LZOOM);
    awm->create_mode("modify.bitmap",   "mode_info.hlp",   AWM_ALL, (AW_CB)NT_modify_cb,  (AW_CL)ntw, (AW_CL)AWT_MODE_MOD);
    awm->create_mode("www_mode.bitmap", "mode_www.hlp",    AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_WWW);

    awm->create_mode("line.bitmap",    "mode_width.hlp",    AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_LINE);
    awm->create_mode("rot.bitmap",     "mode_rotate.hlp",   AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_ROT);
    awm->create_mode("spread.bitmap",  "mode_angle.hlp",    AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_SPREAD);
    awm->create_mode("swap.bitmap",    "mode_swap.hlp",     AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_SWAP);
    awm->create_mode("length.bitmap",  "mode_length.hlp",   AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_LENGTH);
    awm->create_mode("move.bitmap",    "mode_move.hlp",     AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_MOVE);
    awm->create_mode("setroot.bitmap", "mode_set_root.hlp", AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_SETROOT);
    awm->create_mode("reset.bitmap",   "mode_reset.hlp",    AWM_ALL, (AW_CB)nt_mode_event, (AW_CL)ntw, (AW_CL)AWT_MODE_RESET);

    awm->set_info_area_height( 250 );
    awm->at(5,2);
    awm->auto_space(-2,-2);
    awm->shadow_width(1);

    // -------------------------
    //      create top area
    // -------------------------

    int first_liney;
    int leftx;
    awm->get_at_position( &leftx, &first_liney);

    // ---------------------
    //      quit + help
    // ---------------------

    awm->button_length(0);

    if (clone) {
        awm->callback(AW_POPDOWN);
        awm->create_button("CLOSE", "#close.xpm");
    }
    else {
        awm->callback(nt_exit);
        awm->help_text("quit.hlp");
        awm->create_button("QUIT", "#quit.xpm");
    }

    int db_pathx = awm->get_at_xposition();

    // now fetch the yposition for the 2nd button line
    awm->at_newline();
    int second_liney = awm->get_at_yposition();

    awm->callback( AW_help_entry_pressed );
    awm->help_text("arb_ntree.hlp");
    awm->create_button("?","#help.xpm");

    // -------------------------
    //      save + undo/redo
    // -------------------------
    
    awm->callback(NT_save_quick_cb);
    awm->help_text("save.hlp");
    awm->create_button("SAVE", "#save.xpm");

    awm->callback( AW_POPUP, (AW_CL)NT_create_save_as, (AW_CL)"tmp/nt/arbdb");
    awm->help_text("saveas.hlp");
    awm->create_button("SAVE_AS", "#saveAs.xpm");

    // undo/redo:
    awm->callback(NT_undo_cb,(AW_CL)GB_UNDO_UNDO,(AW_CL)ntw);
    awm->help_text("undo.hlp");
    awm->create_button("UNDO", "#undo.bitmap");

    awm->callback(NT_undo_cb,(AW_CL)GB_UNDO_REDO,(AW_CL)ntw);
    awm->help_text("undo.hlp");
    awm->create_button("REDO", "#redo.bitmap");

    int db_pathx2 = awm->get_at_xposition();

    // fetch position for mode help-line: 
    awm->at_newline();
    int third_liney = awm->get_at_yposition();
    
    awm->at(db_pathx, first_liney);
    // size of DB-name button is determined by buttons below:
    awm->at_set_to(false, false, db_pathx2-db_pathx-1, second_liney-first_liney+1);
    awm->callback( AW_POPUP, (AW_CL)NT_create_save_quick_as, (AW_CL)"tmp/nt/arbdb"); // do sth else
    awm->help_text("saveas.hlp");
    awm->create_button("QUICK_SAVE_AS",AWAR_DB_NAME);

    // ----------------------------
    //      tree + tree display
    // ----------------------------

#define TREE_BUTTON_OVERSIZE 6

    int db_treex      = awm->get_at_xposition();
    int second_uppery = second_liney-TREE_BUTTON_OVERSIZE;

    awm->at(db_treex, second_uppery);
    awm->button_length(0);

    awm->callback((AW_CB)NT_set_tree_style,(AW_CL)ntw,(AW_CL)AP_TREE_RADIAL);
    awm->help_text("tr_type_radial.hlp");
    awm->create_button("RADIAL_TREE_TYPE", "#radial.xpm");

    awm->callback((AW_CB)NT_set_tree_style,(AW_CL)ntw,(AW_CL)AP_TREE_NORMAL);
    awm->help_text("tr_type_list.hlp");
    awm->create_button("LIST_TREE_TYPE", "#dendro.xpm");

    awm->callback((AW_CB)NT_set_tree_style,(AW_CL)ntw,(AW_CL)AP_TREE_IRS);
    awm->help_text("tr_type_irs.hlp");
    awm->create_button("FOLDED_LIST_TREE_TYPE", "#dendroIrs.xpm");
    
    awm->callback((AW_CB)NT_set_tree_style, (AW_CL)ntw, (AW_CL)AP_LIST_NDS);
    awm->help_text("tr_type_nds.hlp");
    awm->create_button("NO_TREE_TYPE", "#listdisp.bitmap");

    int db_treex2 = awm->get_at_xposition();
    
    awm->at(db_treex, first_liney);
    // size of tree-name button is determined by buttons below:
    awm->at_set_to(false, false, db_treex2-db_treex-1, second_uppery-first_liney+1);
    awm->callback((AW_CB2)AW_POPUP,(AW_CL)NT_open_select_tree_window,(AW_CL)awar_tree);
    awm->help_text("nt_tree_select.hlp");
    awm->create_button("SELECT_A_TREE", awar_tree);

    // --------------------------------------------------------------
    //      protection, alignment + editor (and maybe Genome Map)
    // --------------------------------------------------------------

    int db_alignx = awm->get_at_xposition();

    // editor and genemap buttons have to be 44x24 sized
#define EDIT_XSIZE 50
#define EDIT_YSIZE 30 // same as 'save as' buttons

    // first draw protection:
    int protectx = db_alignx + 2*EDIT_XSIZE;

    awm->at(protectx+2, first_liney+1);
    awm->button_length(0);
    awm->create_button("PROTECT","#protect.xpm");

    awm->at(protectx, second_liney+2);
    awm->create_option_menu(AWAR_SECURITY_LEVEL);
    awm->insert_option("0",0,0);
    awm->insert_option("1",0,1);
    awm->insert_option("2",0,2);
    awm->insert_option("3",0,3);
    awm->insert_option("4",0,4);
    awm->insert_option("5",0,5);
    awm->insert_default_option("6",0,6);
    awm->update_option_menu();

    int db_searchx = awm->get_at_xposition() - 7;

    // draw ali/editor buttons AFTER protect menu (to get rid of it's label)
    awm->at(db_alignx, second_liney);
    
    awm->at_set_to(false, false, ((2-is_genome_db)*EDIT_XSIZE), EDIT_YSIZE);
    awm->callback((AW_CB)NT_start_editor_on_tree, (AW_CL)&(GLOBAL_NT.tree->tree_root), 0);
    awm->help_text("arb_edit4.hlp");
    awm->create_button("EDIT_SEQUENCES", "#edit.xpm");

    if (is_genome_db) {
        awm->at_set_to(false, false, EDIT_XSIZE, EDIT_YSIZE);
        awm->callback((AW_CB)AW_POPUP, (AW_CL)GEN_map_first, 0); // initial gene map
        awm->help_text("gene_map.hlp");
        awm->create_button("OPEN_GENE_MAP", "#gen_map.xpm");
    }

    int db_alignx2 = awm->get_at_xposition();
    
    awm->at(db_alignx, first_liney);
    awm->at_set_to(false, false, db_alignx2-db_alignx-1, second_liney-first_liney+1);
    awm->callback(AW_POPUP,   (AW_CL)NT_open_select_alignment_window, 0 );
    awm->help_text("nt_align_select.hlp");
    awm->create_button("SELECT_AN_ALIGNMENT", AWAR_DEFAULT_ALIGNMENT);

    // ------------------------------
    //      create mode-help line
    // ------------------------------
    
    awm->at(leftx, third_liney);
    awm->button_length(AWAR_FOOTER_MAX_LEN);
    awm->create_button(0,AWAR_FOOTER);

    awm->at_newline();
    int bottomy = awm->get_at_yposition();

    // --------------------------------------
    //      Info / marked / Search / Jump
    // --------------------------------------

    awm->button_length(7);
    
    awm->at(db_searchx, first_liney);
    awm->callback(AW_POPUP, (AW_CL)ad_create_query_window, 0 );
    awm->help_text("sp_search.hlp");
    awm->create_button("SEARCH",  "Search");

    awm->at(db_searchx, second_uppery);
    awm->callback((AW_CB)NT_jump_cb,(AW_CL)ntw,1);
    awm->help_text("tr_jump.hlp");
    awm->create_button("JUMP", "Jump");
    
    int db_infox = awm->get_at_xposition();
    
    awm->at(db_infox, first_liney);
    awm->button_length(13);
    awm->callback(NT_popup_species_window, 0, 0);
    awm->help_text("sp_search.hlp");
    awm->create_button("INFO",  AWAR_INFO_BUTTON_TEXT);

    awm->at(db_infox, second_uppery);
    awm->button_length(13);
    awm->help_text("marked_species.hlp");
    awm->callback(NT_popup_configuration_admin, (AW_CL)&(GLOBAL_NT.tree->tree_root), 0);
    awm->create_button(0, AWAR_MARKED_SPECIES_COUNTER);
    {
        GBDATA *gb_species_data = GB_search(GLOBAL_gb_main,"species_data",GB_CREATE_CONTAINER);
        GB_add_callback(gb_species_data, GB_CB_CHANGED, nt_auto_count_marked_species, (int*)awm);
        nt_count_marked(awm);
    }

    // set height of top area:
    awm->set_info_area_height(bottomy+2); 
    awm->set_bottom_area_height( 0 );
    awr->set_focus_callback((AW_RCB)NT_focus_cb,(AW_CL)GLOBAL_gb_main,0);

    //  -----------------------------------
    //  Autostarts for development
    //  -----------------------------------

#if defined(DEVEL_RALF)
    // if (is_genome_db) GEN_map_first(awr)->show();
#endif // DEVEL_RALF

    GB_pop_transaction(GLOBAL_gb_main);

    return awm;
}

