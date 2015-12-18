// =============================================================== //
//                                                                 //
//   File      : NT_extern.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "map_viewer.h"
#include "NT_local.h"
#include "ad_trees.h"

#include <seq_quality.h>
#include <multi_probe.hxx>
#include <st_window.hxx>
#include <GEN.hxx>
#include <EXP.hxx>

#include <TreeCallbacks.hxx>
#include <AW_rename.hxx>
#include <probe_gui.hxx>
#include <primer_design.hxx>
#include <gde.hxx>
#include <awtc_submission.hxx>

#include <awti_export.hxx>

#include <awt.hxx>
#include <macros.hxx>
#include <awt_input_mask.hxx>
#include <awt_sel_boxes.hxx>
#include <awt_www.hxx>
#include <awt_TreeAwars.hxx>
#include <awt_misc.hxx>
#include <nds.h>

#include <db_query.h>
#include <dbui.h>

#include <aw_advice.hxx>
#include <aw_preset.hxx>
#include <aw_awars.hxx>
#include <aw_global_awars.hxx>
#include <aw_file.hxx>
#include <aw_msg.hxx>
#include <arb_progress.h>
#include <aw_root.hxx>
#include <aw_question.hxx>
#include <arb_strbuf.h>
#include <arb_strarray.h>
#include <arb_file.h>
#include <ad_cb.h>

#include <arb_version.h>
#include <refentries.h>
#include <rootAsWin.h>
#include <insdel.h>
#include <awti_import.hxx>

#define AWAR_EXPORT_NDS             "tmp/export_nds"
#define AWAR_EXPORT_NDS_SEPARATOR   AWAR_EXPORT_NDS "/separator"
#define AWAR_MARKED_SPECIES_COUNTER "tmp/disp_marked_species"
#define AWAR_NTREE_TITLE_MODE       "tmp/title_mode"

void create_probe_design_variables(AW_root *aw_root, AW_default def, AW_default global);

void       create_insertDeleteColumn_variables(AW_root *root, AW_default db1);
AW_window *create_insertDeleteColumn_window(AW_root *root);
AW_window *create_insertDeleteBySAI_window(AW_root *root, GBDATA *gb_main);

AW_window *create_tree_window(AW_root *aw_root, AWT_graphic *awd);

static void nt_changesecurity(AW_root *aw_root) {
    int level = aw_root->awar(AWAR_SECURITY_LEVEL)->read_int();
    GB_push_transaction(GLOBAL.gb_main);
    GB_change_my_security(GLOBAL.gb_main, level);
    GB_pop_transaction(GLOBAL.gb_main);
}

static void export_nds_cb(AW_window *aww, bool do_print) {
    AW_root *aw_root = aww->get_root();
    char    *name    = aw_root->awar(AWAR_EXPORT_NDS"/file_name")->read_string();
    FILE    *out     = fopen(name, "w");

    if (!out) {
        aw_message("Error: Cannot open and write to file");
    }
    else {
        GB_transaction ta(GLOBAL.gb_main);

        make_node_text_init(GLOBAL.gb_main);
        NDS_Type  nds_type  = (NDS_Type)aw_root->awar(AWAR_EXPORT_NDS_SEPARATOR)->read_int();
        char     *tree_name = aw_root->awar(AWAR_TREE)->read_string();

        for (GBDATA *gb_species = GBT_first_marked_species(GLOBAL.gb_main);
             gb_species;
             gb_species = GBT_next_marked_species(gb_species))
        {
            const char *buf = make_node_text_nds(GLOBAL.gb_main, gb_species, nds_type, 0, tree_name);
            fprintf(out, "%s\n", buf);
        }
        AW_refresh_fileselection(aw_root, AWAR_EXPORT_NDS);
        fclose(out);
        if (do_print) {
            GB_ERROR error = GB_textprint(name);
            if (error) aw_message(error);
        }
        free(tree_name);
    }
    free(name);
}

static AW_window *create_nds_export_window(AW_root *root) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "EXPORT_NDS_OF_MARKED", "EXPORT NDS OF MARKED SPECIES");
    aws->load_xfig("sel_box_nds.fig");

    aws->callback(AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(makeHelpCallback("arb_export_nds.hlp"));
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    aws->at("save");
    aws->callback(makeWindowCallback(export_nds_cb, false));
    aws->create_button("SAVE", "SAVE", "S");

    aws->at("print");
    aws->callback(makeWindowCallback(export_nds_cb, true));
    aws->create_button("PRINT", "PRINT", "P");

    aws->at("toggle1");
    aws->label("Column output");
    aws->create_option_menu(AWAR_EXPORT_NDS_SEPARATOR, true);
    aws->insert_default_option("Space padded",    "S", NDS_OUTPUT_SPACE_PADDED);
    aws->insert_option        ("TAB separated",   "T", NDS_OUTPUT_TAB_SEPARATED);
    aws->insert_option        ("Comma separated", "C", NDS_OUTPUT_COMMA_SEPARATED);
    aws->update_option_menu();

    AW_create_standard_fileselection(aws, AWAR_EXPORT_NDS);

    return aws;
}

static void create_export_nds_awars(AW_root *awr, AW_default def)
{
    AW_create_fileselection_awars(awr, AWAR_EXPORT_NDS, "", ".nds", "export.nds");
    awr->awar_int(AWAR_EXPORT_NDS_SEPARATOR, NDS_OUTPUT_SPACE_PADDED, def);
}

static void AWAR_INFO_BUTTON_TEXT_change_cb(AW_root *awr) {
    char *value = awr->awar(AWAR_SPECIES_NAME)->read_string();
    awr->awar(AWAR_INFO_BUTTON_TEXT)->write_string(value[0] ? value : "Species Info");
    free(value);
}

static void expert_mode_changed_cb(AW_root *aw_root) {
    aw_root->awar(AWAR_AWM_MASK)->write_int(aw_root->awar(AWAR_EXPERT)->read_int() ? AWM_ALL : AWM_BASIC); // publish expert-mode globally
}

static void NT_toggle_expert_mode(AW_window *aww) { aww->get_root()->awar(AWAR_EXPERT)->toggle_toggle(); }
static void NT_toggle_focus_policy(AW_window *aww) { aww->get_root()->awar(AWAR_AW_FOCUS_FOLLOWS_MOUSE)->toggle_toggle(); }

static void nt_create_all_awars(AW_root *awr, AW_default def) {
    // creates awars for all modules reachable from ARB_NT main window

    awr->awar_string(AWAR_FOOTER, "", def);
    if (GB_read_clients(GLOBAL.gb_main)>=0) {
        awr->awar_string(AWAR_TREE, "", GLOBAL.gb_main);
    }
    else {
        awr->awar_string(AWAR_TREE, "", def);
    }

    awr->awar_string(AWAR_SPECIES_NAME, "",     GLOBAL.gb_main);
    awr->awar_string(AWAR_SAI_NAME, "",     GLOBAL.gb_main);
    awr->awar_string(AWAR_SAI_GLOBAL, "",     GLOBAL.gb_main);
    awr->awar_string(AWAR_MARKED_SPECIES_COUNTER, "unknown",    GLOBAL.gb_main);
    awr->awar_string(AWAR_INFO_BUTTON_TEXT, "Species Info",    GLOBAL.gb_main);
    awr->awar(AWAR_SPECIES_NAME)->add_callback(AWAR_INFO_BUTTON_TEXT_change_cb);
    awr->awar_int(AWAR_NTREE_TITLE_MODE, 1);

    awr->awar_string(AWAR_SAI_COLOR_STR, "", GLOBAL.gb_main); // sai visualization in probe match

    GEN_create_awars(awr, def, GLOBAL.gb_main);
    EXP_create_awars(awr, def, GLOBAL.gb_main);
#if defined(DEBUG)
    AWT_create_db_browser_awars(awr, def);
#endif // DEBUG

    AW_create_namesadmin_awars(awr, GLOBAL.gb_main);

    awr->awar_int(AWAR_SECURITY_LEVEL, 0, def);
    awr->awar(AWAR_SECURITY_LEVEL)->add_callback(nt_changesecurity);
#if defined(DEBUG) && 0
    awr->awar(AWAR_SECURITY_LEVEL)->write_int(6); // no security for debugging..
#endif // DEBUG

    create_insertDeleteColumn_variables(awr, def);
    create_probe_design_variables(awr, def, GLOBAL.gb_main);
    create_primer_design_variables(awr, def, GLOBAL.gb_main);
    create_trees_var(awr, def);
    DBUI::create_dbui_awars(awr, def);
    AP_create_consensus_var(awr, def);
    {
        GB_ERROR gde_err = GDE_init(awr, def, GLOBAL.gb_main, 0, ARB_format_alignment, GDE_WINDOWTYPE_DEFAULT);
        if (gde_err) GBK_terminatef("Fatal: %s", gde_err);
    }
    NT_create_transpro_variables(awr, def);
    NT_create_resort_awars(awr, def);
    NT_create_compare_taxonomy_awars(awr, def);
    NT_create_trackAliChanges_Awars(awr, GLOBAL.gb_main);

    NT_create_alignment_vars(awr, def, GLOBAL.gb_main);
    NT_create_extendeds_vars(awr, def, GLOBAL.gb_main);
    create_nds_vars(awr, def, GLOBAL.gb_main, false);
    create_export_nds_awars(awr, def);
    TREE_create_awars(awr, GLOBAL.gb_main);

    awr->awar_string(AWAR_ERROR_MESSAGES, "", GLOBAL.gb_main);
    awr->awar_string(AWAR_DB_COMMENT, "<no description>", GLOBAL.gb_main);

    AWTC_create_submission_variables(awr, GLOBAL.gb_main);
    NT_createConcatenationAwars(awr, def, GLOBAL.gb_main);
    NT_createValidNamesAwars(awr, def); // lothar
    SQ_create_awars(awr, def);
    RefEntries::create_refentries_awars(awr, def);

    NT_create_multifurcate_tree_awars(awr, def);

    GB_ERROR error = ARB_bind_global_awars(GLOBAL.gb_main);
    if (!error) {
        AW_awar *awar_expert = awr->awar(AWAR_EXPERT);
        awar_expert->add_callback(expert_mode_changed_cb);
        awar_expert->touch();

        awt_create_aww_vars(awr, def);
    }

    if (error) aw_message(error);
}

// --------------------------------------------------------------------------------

bool nt_disconnect_from_db(AW_root *aw_root, GBDATA*& gb_main_ref) {
    // ask user if DB was not saved
    // returns true if user allows to quit

    // @@@ code here could as well be applied to both merge DBs
    // - question about saving only with target DB!

    if (gb_main_ref) {
        if (GB_read_clients(gb_main_ref)>=0) {
            if (GB_read_clock(gb_main_ref) > GB_last_saved_clock(gb_main_ref)) {
                long secs;
                secs = GB_last_saved_time(gb_main_ref);

#if defined(DEBUG)
                secs =  GB_time_of_day(); // simulate "just saved"
#endif // DEBUG

                const char *quit_buttons = "Quit ARB,Do NOT quit";
                if (secs) {
                    secs = GB_time_of_day() - secs;
                    if (secs>10) {
                        char *question = GBS_global_string_copy("You last saved your data %li:%li minutes ago\nSure to quit ?", secs/60, secs%60);
                        int   dontQuit = aw_question("quit_arb", question, quit_buttons);
                        free(question);
                        if (dontQuit) {
                            return false;
                        }
                    }
                }
                else {
                    if (aw_question("quit_arb", "You never saved any data\nSure to quit ???", quit_buttons)) {
                        return false;
                    }
                }
            }
        }

        GBCMS_shutdown(gb_main_ref);

        GBDATA *gb_main = gb_main_ref;
        gb_main_ref     = NULL;                  // avoid further usage

        nt_assert(aw_root);
#if defined(WARN_TODO)
#warning instead of directly calling the following functions, try to add them as GB_atclose-callbacks
#endif
        aw_root->unlink_awars_from_DB(gb_main);
        AWT_destroy_input_masks();
#if defined(DEBUG)
        AWT_browser_forget_db(gb_main);
#endif // DEBUG

        GB_close(gb_main);
    }
    return true;
}

static void nt_run(const char *command) {
    GB_ERROR error = GBK_system(command);
    if (error) {
        fprintf(stderr, "nt_run: Failed to run '%s' (Reason: %s)\n", command, error);
#if defined(DEBUG)
        GBK_dump_backtrace(stderr, "nt_run-error");
#endif
    }
}

void NT_start(const char *arb_ntree_args, bool restart_with_new_ARB_PID) {
    char *command = GBS_global_string_copy("arb_launcher --async %s %s", restart_with_new_ARB_PID ? "arb" : "arb_ntree", arb_ntree_args);
    nt_run(command);
    free(command);
}

__ATTR__NORETURN static void really_exit(int exitcode, bool kill_my_clients) {
    if (kill_my_clients) {
        nt_run("(arb_clean session; echo ARB done) &"); // kills all clients
    }
    exit(exitcode);
}

void NT_exit(AW_window *aws, int exitcode) {
    AW_root *aw_root = aws->get_root();
    AWTI_cleanup_importer();
    shutdown_macro_recording(aw_root);
    bool is_server_and_has_clients = GLOBAL.gb_main && GB_read_clients(GLOBAL.gb_main)>0;
    if (nt_disconnect_from_db(aw_root, GLOBAL.gb_main)) {
        really_exit(exitcode, is_server_and_has_clients);
    }
}
void NT_restart(AW_root *aw_root, const char *arb_ntree_args) {
    // restarts arb_ntree (with new ARB_PID)
    bool is_server_and_has_clients = GLOBAL.gb_main && GB_read_clients(GLOBAL.gb_main)>0;
    if (nt_disconnect_from_db(aw_root, GLOBAL.gb_main))  {
        NT_start(arb_ntree_args, true);
        really_exit(EXIT_SUCCESS, is_server_and_has_clients);
    }
}

static void nt_start_2nd_arb(AW_window *aww, bool quit) {
    // start 2nd arb with intro window
    AW_root *aw_root = aww->get_root();
    char    *dir4intro;
    GB_split_full_path(aw_root->awar(AWAR_DB_PATH)->read_char_pntr(), &dir4intro, NULL, NULL, NULL);
    if (!dir4intro) {
        dir4intro = strdup(".");
    }

    if (quit) {
        NT_restart(aw_root, dir4intro);
    }
    else {
        NT_start(dir4intro, true);
    }
    free(dir4intro);
}

// --------------------------------------------------------------------------------

static void NT_save_quick_cb(AW_window *aww) {
    AW_root *awr      = aww->get_root();
    char    *filename = awr->awar(AWAR_DB_PATH)->read_string();

    awr->dont_save_awars_with_default_value(GLOBAL.gb_main);

    GB_ERROR error = GB_save_quick(GLOBAL.gb_main, filename);
    free( filename);
    if (error) aw_message(error);
    else       AW_refresh_fileselection(awr, AWAR_DBBASE);
}


static void NT_save_quick_as_cb(AW_window *aww) {
    AW_root  *awr      = aww->get_root();
    char     *filename = awr->awar(AWAR_DB_PATH)->read_string();
    GB_ERROR  error    = GB_save_quick_as(GLOBAL.gb_main, filename);
    if (!error) AW_refresh_fileselection(awr, AWAR_DBBASE);
    aww->hide_or_notify(error);

    free(filename);
}

#define AWAR_DB_TYPE          AWAR_DBBASE "/type"      // created by AWT_insert_DBsaveType_selector
#define AWAR_DB_COMPRESSION   AWAR_DBBASE "/compression" // created by AWT_insert_DBcompression_selector
#define AWAR_DB_OPTI_TREENAME AWAR_DBBASE "/optimize_tree_name"

static void NT_save_as_cb(AW_window *aww) {
    AW_root    *awr      = aww->get_root();
    char       *filename = awr->awar(AWAR_DB_PATH)->read_string();
    const char *atype    = awr->awar(AWAR_DB_TYPE)->read_char_pntr();
    const char *ctype    = awr->awar(AWAR_DB_COMPRESSION)->read_char_pntr();
    char       *savetype = GBS_global_string_copy("%s%s", atype, ctype);

    awr->dont_save_awars_with_default_value(GLOBAL.gb_main);
    GB_ERROR error = GB_save(GLOBAL.gb_main, filename, savetype);
    if (!error) AW_refresh_fileselection(awr, AWAR_DBBASE);
    aww->hide_or_notify(error);

    free(savetype);
    free(filename);
}

static AW_window *NT_create_save_quick_as_window(AW_root *aw_root, const char *base_name) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;
        aws->init(aw_root, "SAVE_CHANGES_TO", "Quicksave changes as");
        aws->load_xfig("save_as.fig");

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(makeHelpCallback("save.hlp"));
        aws->at("help");
        aws->create_button("HELP", "HELP", "H");

        AW_create_standard_fileselection(aws, base_name);

        aws->at("comment");
        aws->create_text_field(AWAR_DB_COMMENT);

        aws->at("save");
        aws->callback(NT_save_quick_as_cb);
        aws->create_button("SAVE", "SAVE", "S");
    }
    return aws;
}

static void NT_database_optimization(AW_window *aww) {
    arb_progress progress("Optimizing database compression", 2);

    GB_push_my_security(GLOBAL.gb_main);
    GB_ERROR error = GB_begin_transaction(GLOBAL.gb_main);
    if (!error) {
        ConstStrArray ali_names;
        GBT_get_alignment_names(ali_names, GLOBAL.gb_main);

        arb_progress ali_progress("Optimizing sequence data", ali_names.size());
        ali_progress.allow_title_reuse();

        error = GBT_check_data(GLOBAL.gb_main, 0);
        error = GB_end_transaction(GLOBAL.gb_main, error);

        if (!error) {
            char *tree_name = aww->get_root()->awar(AWAR_DB_OPTI_TREENAME)->read_string();
            for (int i = 0; ali_names[i]; ++i) {
                error = GBT_compress_sequence_tree2(GLOBAL.gb_main, tree_name, ali_names[i]);
                ali_progress.inc_and_check_user_abort(error);
            }
            free(tree_name);
        }
    }
    progress.inc_and_check_user_abort(error);

    if (!error) {
        error = GB_optimize(GLOBAL.gb_main);
        progress.inc_and_check_user_abort(error);
    }

    GB_pop_my_security(GLOBAL.gb_main);
    aww->hide_or_notify(error);
}

static AW_window *NT_create_database_optimization_window(AW_root *aw_root) {
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;
    GB_transaction ta(GLOBAL.gb_main);

    const char *largest_tree = GBT_name_of_largest_tree(GLOBAL.gb_main);
    aw_root->awar_string(AWAR_DB_OPTI_TREENAME, largest_tree);

    aws = new AW_window_simple;
    aws->init(aw_root, "OPTIMIZE_DATABASE", "Optimize database compression");
    aws->load_xfig("optimize.fig");

    aws->at("trees");
    awt_create_TREE_selection_list(GLOBAL.gb_main, aws, AWAR_DB_OPTI_TREENAME, true);

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(makeHelpCallback("optimize.hlp"));
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    aws->at("go");
    aws->callback(NT_database_optimization);
    aws->create_button("GO", "GO");
    return aws;
}

static AW_window *NT_create_save_as(AW_root *aw_root, const char *base_name)
{
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    aws = new AW_window_simple;
    aws->init(aw_root, "SAVE_DB", "SAVE ARB DB");
    aws->load_xfig("save_as.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("save.hlp"));
    aws->create_button("HELP", "HELP", "H");

    AW_create_standard_fileselection(aws, base_name);

    aws->at("type");
    AWT_insert_DBsaveType_selector(aws, AWAR_DB_TYPE);

    aws->at("compression");
    AWT_insert_DBcompression_selector(aws, AWAR_DB_COMPRESSION);

    aws->at("optimize");
    aws->callback(NT_create_database_optimization_window);
    aws->help_text("optimize.hlp");
    aws->create_autosize_button("OPTIMIZE", "Optimize database compression");

    aws->at("save");
    aws->callback(NT_save_as_cb);
    aws->create_button("SAVE", "SAVE", "S");

    aws->at("comment");
    aws->create_text_field(AWAR_DB_COMMENT);

    return aws;
}

static void NT_undo_cb(AW_window*, GB_UNDO_TYPE undo_type, AWT_canvas *ntw) {
    GB_ERROR error = GB_undo(GLOBAL.gb_main, undo_type);
    if (error) aw_message(error);
    else {
        GB_transaction ta(GLOBAL.gb_main);
        ntw->refresh();
    }
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

static void NT_modify_cb(UNFIXED, AWT_canvas *canvas, AWT_COMMAND_MODE mode) {
    DBUI::popup_species_info_window(canvas->awr, canvas->gb_main);
    nt_mode_event(NULL, canvas, mode);
}

static void NT_primer_cb(AW_window*) {
    GB_ERROR error = GB_xcmd("arb_primer", true, false);
    if (error) aw_message(error);
}

static void NT_mark_duplicates(UNFIXED, AWT_canvas *ntw) {
    AP_tree *tree_root = AWT_TREE(ntw)->get_root_node();
    if (tree_root) {
        GB_transaction ta(ntw->gb_main);
        NT_mark_all_cb(NULL, ntw, 0);

        tree_root->mark_duplicates();
        tree_root->compute_tree();
        ntw->refresh();
    }
}

static void NT_justify_branch_lenghs(UNFIXED, AWT_canvas *ntw) {
    GB_transaction  ta(ntw->gb_main);
    AP_tree        *tree_root = AWT_TREE(ntw)->get_root_node();

    if (tree_root) {
        tree_root->justify_branch_lenghs(ntw->gb_main);
        tree_root->compute_tree();
        GB_ERROR error = AWT_TREE(ntw)->save(ntw->gb_main, NULL);
        if (error) aw_message(error);
        ntw->refresh();
    }
}

#if defined(DEBUG)
static void NT_fix_database(AW_window *) {
    GB_ERROR err = 0;
    err = GB_fix_database(GLOBAL.gb_main);
    if (err) aw_message(err);
}
#endif

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

static void NT_pseudo_species_to_organism(AW_window *, AWT_canvas *ntw) {
    GB_transaction  ta(ntw->gb_main);
    AP_tree        *tree_root = AWT_TREE(ntw)->get_root_node();

    if (tree_root) {
        GB_HASH *organism_hash = GBT_create_organism_hash(ntw->gb_main);
        tree_root->relink_tree(ntw->gb_main, relink_pseudo_species_to_organisms, organism_hash);
        tree_root->compute_tree();
        GB_ERROR error = AWT_TREE(ntw)->save(ntw->gb_main, NULL);
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

struct nt_item_type_species_selector : public awt_item_type_selector {
    nt_item_type_species_selector() : awt_item_type_selector(AWT_IT_SPECIES) {}
    virtual ~nt_item_type_species_selector() OVERRIDE {}

    virtual const char *get_self_awar() const OVERRIDE {
        return AWAR_SPECIES_NAME;
    }
    virtual size_t get_self_awar_content_length() const OVERRIDE {
        return 12; // should be enough for "nnaammee.999"
    }
    virtual GBDATA *current(AW_root *root, GBDATA *gb_main) const OVERRIDE { // give the current item
        char           *species_name = root->awar(get_self_awar())->read_string();
        GBDATA         *gb_species   = 0;

        if (species_name[0]) {
            GB_transaction ta(gb_main);
            gb_species = GBT_find_species(gb_main, species_name);
        }

        free(species_name);
        return gb_species;
    }
    virtual const char *getKeyPath() const OVERRIDE { // give the keypath for items
        return CHANGE_KEY_PATH;
    }
};

static nt_item_type_species_selector item_type_species;

static void NT_open_mask_window(AW_window *aww, int id, GBDATA *gb_main) {
    const awt_input_mask_descriptor *descriptor = AWT_look_input_mask(id);
    nt_assert(descriptor);
    if (descriptor) AWT_initialize_input_mask(aww->get_root(), gb_main, &item_type_species, descriptor->get_internal_maskname(), descriptor->is_local_mask());
}

static void NT_create_mask_submenu(AW_window_menu_modes *awm) {
    AWT_create_mask_submenu(awm, AWT_IT_SPECIES, NT_open_mask_window, GLOBAL.gb_main);
}
static AW_window *create_colorize_species_window(AW_root *aw_root) {
    return QUERY::create_colorize_items_window(aw_root, GLOBAL.gb_main, SPECIES_get_selector());
}

static void NT_update_marked_counter(GBDATA*, AW_window* aww) {
    /*! Updates marked counter and issues redraw on tree if number of marked species changes.
     * Called on any change of species_information container.
     */
    long        count   = GBT_count_marked_species(GLOBAL.gb_main);
    const char *buffer  = count ? GBS_global_string("%li marked", count) : "";
    AW_awar    *counter = aww->get_root()->awar(AWAR_MARKED_SPECIES_COUNTER);
    char       *oldval  = counter->read_string();
    if (strcmp(oldval, buffer)) {
        counter->write_string(buffer);
        aww->get_root()->awar(AWAR_TREE_REFRESH)->touch();
    }
    free(oldval);
}

// --------------------------------------------------------------------------------------------------

static void NT_alltree_remove_leafs(AW_window *, GBT_TreeRemoveType mode, GBDATA *gb_main) {
    GB_ERROR       error = 0;
    GB_transaction ta(gb_main);

    ConstStrArray tree_names;
    GBT_get_tree_names(tree_names, gb_main, false);

    if (!tree_names.empty()) {
        int treeCount = tree_names.size();

        const char *whats_removed = NULL;
        switch (mode) {
            case GBT_REMOVE_ZOMBIES: whats_removed = "zombies"; break;
            case GBT_REMOVE_MARKED:  whats_removed = "marked";  break;
            default: nt_assert(0); break;
        }

        const char *task = GBS_global_string("Deleting %s from %i trees", whats_removed, treeCount);

        arb_progress  progress(task, treeCount);
        GB_HASH      *species_hash = GBT_create_species_hash(gb_main);

        int modified = 0;

        for (int t = 0; t<treeCount && !error; t++) {
            progress.subtitle(tree_names[t]);
            TreeNode *tree = GBT_read_tree(gb_main, tree_names[t], new SimpleRoot);
            if (!tree) {
                aw_message(GBS_global_string("Can't load tree '%s' - skipped", tree_names[t]));
            }
            else {
                int removed        = 0;
                int groups_removed = 0;

                tree = GBT_remove_leafs(tree, mode, species_hash, &removed, &groups_removed);

                nt_assert(removed >= groups_removed);

                if (!tree) {
                    aw_message(GBS_global_string("'%s' would disappear. Please delete tree manually.", tree_names[t]));
                }
                else {
                    if (removed>0) {
                        error = GBT_write_tree(gb_main, tree_names[t], tree);
                        if (error) {
                            aw_message(GBS_global_string("Failed to write '%s' (Reason: %s)", tree_names[t], error));
                        }
                        else {
                            if (groups_removed>0) {
                                aw_message(GBS_global_string("Removed %i species and %i groups from '%s'", removed, groups_removed, tree_names[t]));
                            }
                            else {
                                aw_message(GBS_global_string("Removed %i species from '%s'", removed, tree_names[t]));
                            }
                            modified++;
                        }
                    }
                    UNCOVERED();
                    destroy(tree);
                }
            }
            progress.inc_and_check_user_abort(error);
        }

        if (modified) {
            aw_message(GBS_global_string("Changed %i of %i trees.", modified, treeCount));
        }
        else {
            aw_message("No tree modified");
        }

        GBS_free_hash(species_hash);
    }

    aw_message_if(ta.close(error));
}

TreeNode *NT_get_tree_root_of_canvas(AWT_canvas *ntw) {
    AWT_graphic_tree *tree = AWT_TREE(ntw);
    if (tree) {
        AP_tree *root = tree->get_root_node();
        if (root) return root;
    }
    return NULL;
}

// --------------------------------------------------------------------------------------------------

static ARB_ERROR mark_referred_species(GBDATA *gb_main, const DBItemSet& referred) {
    GB_transaction ta(gb_main);
    GBT_mark_all(gb_main, 0);

    DBItemSetIter end = referred.end();
    for (DBItemSetIter s = referred.begin(); s != end; ++s) {
        GB_write_flag(*s, 1);
    }
    return ARB_ERROR();
}

static AW_window *create_mark_by_refentries_window(AW_root *awr, GBDATA *gbmain) {
    static AW_window *aws = NULL;
    if (!aws) {
        static RefEntries::ReferringEntriesHandler reh(gbmain, SPECIES_get_selector());
        aws = RefEntries::create_refentries_window(awr, &reh, "markbyref", "Mark by reference", "markbyref.hlp", NULL, "Mark referenced", mark_referred_species);
    }
    return aws;
}

// --------------------------------------------------------------------------------------------------

static void merge_from_to(AW_root *awr, const char *db_awar_name, bool merge_to) {
    const char *db_name  = awr->awar(db_awar_name)->read_char_pntr();
    char       *quotedDB = GBK_singlequote(db_name);

    char *cmd = GBS_global_string_copy(
        merge_to
        ? "arb_ntree : %s &"
        : "arb_ntree %s : &",
        quotedDB);

    aw_message_if(GBK_system(cmd));
    free(cmd);
    free(quotedDB);
}

static void merge_from_cb(AW_window *aww, AW_CL cl_awarNamePtr) { merge_from_to(aww->get_root(), *(const char**)cl_awarNamePtr, false); }
static void merge_into_cb(AW_window *aww, AW_CL cl_awarNamePtr) { merge_from_to(aww->get_root(), *(const char**)cl_awarNamePtr, true); }

static AW_window *NT_create_merge_from_window(AW_root *awr) {
    static char *awar_name = NULL; // do not free, bound to callback
    AW_window *aw_from     =
        awt_create_load_box(awr, "Merge data from", "other ARB database",
                            ".", ".arb", &awar_name,
                            makeWindowCallback(merge_from_cb, AW_CL(&awar_name)));
    return aw_from;
}
static AW_window *NT_create_merge_to_window(AW_root *awr) {
    static char *awar_name = NULL; // do not free, bound to callback
    AW_window *aw_to       =
        awt_create_load_box(awr, "Merge data to", "other ARB database",
                            ".", ".arb", &awar_name,
                            makeWindowCallback(merge_into_cb, AW_CL(&awar_name)),
                            makeWindowCallback(AW_POPDOWN), NULL);
    return aw_to;
}

// --------------------------------------------------------------------------------------------------

int NT_get_canvas_id(AWT_canvas *ntw) {
    // return number of canvas [0..MAX_NT_WINDOWS-1]

    const char *tree_awar_name = ntw->user_awar;

    const unsigned LEN = 15;
#if defined(ASSERTION_USED)
    const char *EXPECT = "focus/tree_name";
#endif

    nt_assert(strlen(EXPECT)                      == LEN);
    nt_assert(memcmp(tree_awar_name, EXPECT, LEN) == 0);

    int id;
    switch (tree_awar_name[LEN]) {
        default :
            nt_assert(0);
            // NDEBUG: fallback to 0
        case 0:
            id = 0;
            break;

        case '_':
            id = atoi(tree_awar_name+LEN+1);
            nt_assert(id >= 1);
            break;
    }
    return id;
}

static void update_main_window_title(AW_root* awr, AW_window_menu_modes* aww, int clone) {
    const char* filename = awr->awar(AWAR_DB_NAME)->read_char_pntr();
    if (clone) {
        aww->set_window_title(GBS_global_string("%s - ARB (%i)",  filename, clone));
    }
    else {
        aww->set_window_title(GBS_global_string("%s - ARB", filename));
    }
}

static void canvas_tree_awar_changed_cb(AW_awar *, bool, AWT_canvas *ntw) {
    NT_reload_tree_event(AW_root::SINGLETON, ntw, true);
}

// ##########################################
// ##########################################
// ###                                    ###
// ##          create main window          ##
// ###                                    ###
// ##########################################
// ##########################################

static AW_window *popup_new_main_window(AW_root *awr, int clone, AWT_canvas **result_ntw) {
    /*! create ARB_NTREE main window
     * @param awr application root
     * @param clone == 0 -> first window (full functionality); >0 -> additional tree views (restricted functionality)
     * @param result_ntw is set to the created AWT_canvas (passed pointer may be NULL if result is not needed)
     */
    GB_push_transaction(GLOBAL.gb_main);

    char        window_title[256];
    const char *awar_tree = NULL;

    if (clone) {
        awar_tree = GB_keep_string(GBS_global_string_copy(AWAR_TREE "_%i", clone));
        sprintf(window_title, "ARB_NT_%i", clone);
    }
    else {
        awar_tree = AWAR_TREE;
        sprintf(window_title, "ARB_NT");
    }
    AW_window_menu_modes *awm = new AW_window_menu_modes;
    awm->init(awr, window_title, window_title, 0, 0);

    awr->awar(AWAR_DB_NAME)
       ->add_callback(makeRootCallback(update_main_window_title, awm, clone))
       ->update();

    awm->button_length(5);

    if (!clone) AW_init_color_group_defaults("arb_ntree");

    AWT_graphic_tree *tree = NT_generate_tree(awr, GLOBAL.gb_main, launch_MapViewer_cb);

    AWT_canvas *ntw;
    {
        AP_tree_display_type old_sort_type = tree->tree_sort;
        tree->set_tree_type(AP_LIST_SIMPLE, NULL); // avoid NDS warnings during startup

        ntw = new AWT_canvas(GLOBAL.gb_main, awm, "ARB_NT", tree, awar_tree);
        tree->set_tree_type(old_sort_type, ntw);
        ntw->set_mode(AWT_MODE_SELECT);
    }

    if (result_ntw) *result_ntw = ntw;

    {
        AW_awar    *tree_awar          = awr->awar_string(awar_tree);
        const char *tree_name          = tree_awar->read_char_pntr();
        const char *existing_tree_name = GBT_existing_tree(GLOBAL.gb_main, tree_name);

        if (existing_tree_name) {
            tree_awar->write_string(existing_tree_name);
            NT_reload_tree_event(awr, ntw, true); // load first tree
        }
        else {
            AW_advice("Your database contains no tree.", AW_ADVICE_TOGGLE_AND_HELP, 0, "no_tree.hlp");
            tree->set_tree_type(AP_LIST_NDS, ntw); // no tree -> show NDS list
        }

        AWT_registerTreeAwarCallback(tree_awar, makeTreeAwarCallback(canvas_tree_awar_changed_cb, ntw), false);
    }

    TREE_install_update_callbacks(ntw);
    awr->awar(AWAR_TREE_NAME)->add_callback(makeRootCallback(TREE_auto_jump_cb, ntw, true)); // NT specific (tree name never changes in parsimony!)

    bool is_genome_db = GEN_is_genome_db(GLOBAL.gb_main, 0); //  is this a genome database ? (default = 0 = not a genom db)

    WindowCallback popupinfo_wcb = RootAsWindowCallback::simple(DBUI::popup_species_info_window, GLOBAL.gb_main);

    // --------------
    //      File

#if defined(DEBUG)
    AWT_create_debug_menu(awm);
#endif // DEBUG

    bool allow_new_window = (NT_get_canvas_id(ntw)+1) < MAX_NT_WINDOWS;

    if (clone) {
        awm->create_menu("File", "F", AWM_ALL);
        if (allow_new_window) {
            awm->insert_menu_topic(awm->local_id("new_window"), "New window (same database)", "N", "newwindow.hlp", AWM_ALL, makeCreateWindowCallback(popup_new_main_window, clone+1, (AWT_canvas**)NULL));
        }
        awm->insert_menu_topic("close", "Close", "C", 0, AWM_ALL, AW_POPDOWN);
    }
    else {
#if defined(DEBUG)
        // add more to debug-menu
        awm->sep______________();
        awm->insert_menu_topic("fix_db",      "Fix database",            "F", 0, AWM_ALL, NT_fix_database);
        awm->insert_menu_topic("debug_arbdb", "Print debug information", "d", 0, AWM_ALL, makeWindowCallback(GB_print_debug_information, GLOBAL.gb_main));
        awm->insert_menu_topic("test_compr",  "Test compression",        "T", 0, AWM_ALL, makeWindowCallback(GBT_compression_test,       GLOBAL.gb_main));
#endif // DEBUG

        awm->create_menu("File", "F", AWM_ALL);
        {
            // keep the following entries in sync with ../EDIT4/ED4_root.cxx@common_save_menu_entries
            awm->insert_menu_topic("save_changes", "Quicksave changes",          "s", "save.hlp",      AWM_ALL, NT_save_quick_cb);
            awm->insert_menu_topic("save_all_as",  "Save whole database as ...", "w", "save.hlp",      AWM_ALL, makeCreateWindowCallback(NT_create_save_as, AWAR_DBBASE));
            if (allow_new_window) {
                awm->sep______________();
                awm->insert_menu_topic("new_window", "New window (same database)", "N", "newwindow.hlp", AWM_ALL, makeCreateWindowCallback(popup_new_main_window, clone+1, (AWT_canvas**)NULL));
            }
            awm->sep______________();
            awm->insert_menu_topic("optimize_db",  "Optimize database compression", "O", "optimize.hlp",  AWM_ALL, NT_create_database_optimization_window);
            awm->sep______________();

            awm->insert_sub_menu("Import",      "I");
            {
                awm->insert_menu_topic("merge_from", "Merge from other ARB database", "d", "arb_merge_into.hlp", AWM_ALL, NT_create_merge_from_window);
                awm->insert_menu_topic("import_seq", "Import from external format",   "I", "arb_import.hlp",     AWM_ALL, makeWindowCallback(NT_import_sequences, ntw));
                GDE_load_menu(awm, AWM_EXP, "Import");
            }
            awm->close_sub_menu();

            awm->insert_sub_menu("Export",      "E");
            {
                awm->insert_menu_topic("export_to_ARB", "Merge to (new) ARB database", "A", "arb_merge_outof.hlp", AWM_ALL, NT_create_merge_to_window);
                awm->insert_menu_topic("export_seqs",   "Export to external format",   "f", "arb_export.hlp",      AWM_ALL, makeCreateWindowCallback(create_AWTC_export_window, GLOBAL.gb_main));
                GDE_load_menu(awm, AWM_ALL, "Export");
                awm->insert_menu_topic("export_nds",    "Export fields (to calc-sheet using NDS)", "N", "arb_export_nds.hlp",  AWM_ALL, create_nds_export_window);
            }
            awm->close_sub_menu();
            awm->sep______________();

            insert_macro_menu_entry(awm, false);

            awm->insert_sub_menu("VersionInfo/Bugreport/MailingList", "V");
            {
                awm->insert_menu_topic("version_info", "Version info (" ARB_VERSION_DETAILED ")", "V", "version.hlp", AWM_ALL, makeHelpCallback  ("version.hlp"));
                awm->insert_menu_topic("bug_report",   "Report bug",                              "b", NULL,          AWM_ALL, makeWindowCallback(AWT_openURL, "http://bugs.arb-home.de/wiki/BugReport"));
                awm->insert_menu_topic("mailing_list", "Mailing list",                            "M", NULL,          AWM_ALL, makeWindowCallback(AWT_openURL, "http://bugs.arb-home.de/wiki/ArbMailingList"));
            }
            awm->close_sub_menu();
            awm->sep______________();

            awm->insert_menu_topic("new_arb",     "Start second database",      "d", "quit.hlp", AWM_ALL, makeWindowCallback(nt_start_2nd_arb, false));
            awm->insert_menu_topic("restart_arb", "Quit + load other database", "l", "quit.hlp", AWM_ALL, makeWindowCallback(nt_start_2nd_arb, true));
            awm->insert_menu_topic("quit",        "Quit",                       "Q", "quit.hlp", AWM_ALL, makeWindowCallback(NT_exit, EXIT_SUCCESS));
        }

        // -----------------
        //      Species

        awm->create_menu("Species", "c", AWM_ALL);
        {
            awm->insert_menu_topic("species_search", "Search and query",    "q", "sp_search.hlp", AWM_ALL, makeCreateWindowCallback(DBUI::create_species_query_window, GLOBAL.gb_main));
            awm->insert_menu_topic("species_info",   "Species information", "i", "sp_info.hlp",   AWM_ALL, popupinfo_wcb);

            awm->sep______________();

            NT_insert_mark_submenus(awm, ntw, 1);
            awm->insert_menu_topic("mark_by_ref",     "Mark by reference..", "r", "markbyref.hlp",       AWM_EXP, makeCreateWindowCallback(create_mark_by_refentries_window, GLOBAL.gb_main));
            awm->insert_menu_topic("species_colors",  "Colors ...",          "l", "colorize.hlp",        AWM_ALL, create_colorize_species_window);
            awm->insert_menu_topic("selection_admin", "Selections ...",      "o", "species_configs.hlp", AWM_ALL, makeWindowCallback(NT_popup_configuration_admin, ntw));

            awm->sep______________();

            awm->insert_sub_menu("Database fields admin", "f");
            DBUI::insert_field_admin_menuitems(awm, GLOBAL.gb_main);
            awm->close_sub_menu();
            NT_create_mask_submenu(awm);

            awm->sep______________();

            awm->insert_menu_topic("del_marked",    "Delete Marked Species",    "D", "sp_del_mrkd.hlp", AWM_ALL, makeWindowCallback(NT_delete_mark_all_cb, ntw));

            awm->insert_sub_menu("Sort Species",         "s");
            {
                awm->insert_menu_topic("$sort_by_field", "According to Database Entries", "D", "sp_sort_fld.hlp",  AWM_ALL, NT_create_resort_window);
                awm->insert_menu_topic("$sort_by_tree",  "According to Phylogeny",        "P", "sp_sort_phyl.hlp", AWM_ALL, makeWindowCallback(NT_resort_data_by_phylogeny, ntw));
            }
            awm->close_sub_menu();

            awm->insert_sub_menu("Merge Species", "g", AWM_EXP);
            {
                awm->insert_menu_topic("merge_species", "Create merged species from similar species", "m", "merge_species.hlp", AWM_EXP, NT_createMergeSimilarSpeciesWindow);
                awm->insert_menu_topic("join_marked",   "Join Marked Species",                        "J", "species_join.hlp",  AWM_EXP, NT_create_species_join_window);
            }
            awm->close_sub_menu();

            awm->sep______________();

            awm->insert_menu_topic("species_submission", "Submit Species", "b", "submission.hlp", AWM_EXP, makeCreateWindowCallback(AWTC_create_submission_window, GLOBAL.gb_main));

            awm->sep______________();

            awm->insert_menu_topic("new_names", "Synchronize IDs", "e", "sp_rename.hlp", AWM_ALL, makeCreateWindowCallback(AWTC_create_rename_window, GLOBAL.gb_main));

            awm->insert_sub_menu("Valid Names ...", "V", AWM_EXP);
            {
                awm->insert_menu_topic("imp_names",    "Import names from file", "I", "vn_import.hlp",  AWM_EXP, NT_importValidNames);
                awm->insert_menu_topic("del_names",    "Delete names from DB",   "D", "vn_delete.hlp",  AWM_EXP, NT_deleteValidNames);
                awm->insert_menu_topic("sug_names",    "Suggest valid names",    "v", "vn_suggest.hlp", AWM_EXP, NT_suggestValidNames);
                awm->insert_menu_topic("search_names", "Search manually",        "m", "vn_search.hlp",  AWM_EXP, NT_create_searchManuallyNames_window);
            }
            awm->close_sub_menu();
        }

        // ----------------------------
        //      Genes + Experiment

        if (is_genome_db) GEN_create_genes_submenu(awm, GLOBAL.gb_main, true);

        // ------------------
        //      Sequence

        awm->create_menu("Sequence", "S", AWM_ALL);
        {
            awm->insert_menu_topic("seq_admin",   "Sequence/Alignment Admin", "A", "ad_align.hlp",   AWM_ALL, makeCreateWindowCallback(NT_create_alignment_admin_window, (AW_window*)0));
            awm->insert_sub_menu("Insert/delete", "I");
            {
                awm->insert_menu_topic("ins_del_col", ".. column",     "c", "insdel.hlp",     AWM_ALL, create_insertDeleteColumn_window);
                awm->insert_menu_topic("ins_del_sai", ".. using SAI",  "S", "insdel_sai.hlp", AWM_ALL, makeCreateWindowCallback(create_insertDeleteBySAI_window,  GLOBAL.gb_main));
            }
            awm->close_sub_menu();
            awm->sep______________();

            awm->insert_sub_menu("Edit Sequences", "E");
            {
                awm->insert_menu_topic("new_arb_edit4",  "Using marked species and tree", "m", "arb_edit4.hlp", AWM_ALL, makeWindowCallback(NT_start_editor_on_tree,  0, ntw));
                awm->insert_menu_topic("new2_arb_edit4", "... plus relatives",            "r", "arb_edit4.hlp", AWM_ALL, makeWindowCallback(NT_start_editor_on_tree, -1, ntw));
                awm->insert_menu_topic("old_arb_edit4",  "Using earlier configuration",   "c", "arb_edit4.hlp", AWM_ALL, NT_create_startEditorOnOldConfiguration_window);
            }
            awm->close_sub_menu();

            awm->sep______________();

            awm->insert_sub_menu("Align Sequences",  "S");
            {
                awm->insert_menu_topic("realign_dna", "Realign DNA according to aligned protein", "R", "realign_dna.hlp", AWM_ALL, NT_create_realign_dna_window);
                GDE_load_menu(awm, AWM_ALL, "Align");
            }
            awm->close_sub_menu();
            awm->insert_menu_topic("seq_concat",    "Concatenate Sequences/Alignments", "C", "concatenate.hlp",       AWM_ALL, NT_createConcatenationWindow);
            awm->insert_menu_topic("track_changes", "Track alignment changes",          "k", "track_ali_changes.hlp", AWM_EXP, NT_create_trackAliChanges_window);
            awm->sep______________();

            awm->insert_menu_topic("dna_2_pro", "Perform translation",      "t", "translate_dna_2_pro.hlp", AWM_ALL, NT_create_dna_2_pro_window);
            awm->insert_menu_topic("arb_dist",  "Distance Matrix + ARB NJ", "D", "dist.hlp",                AWM_ALL, makeWindowCallback(NT_system_cb, "arb_dist &"));
            awm->sep______________();

            awm->insert_menu_topic("seq_quality",   "Check Sequence Quality", "Q", "seq_quality.hlp",   AWM_EXP, makeCreateWindowCallback(SQ_create_seq_quality_window, GLOBAL.gb_main));
            awm->insert_menu_topic("chimera_check", "Chimera Check",          "m", "chimera_check.hlp", AWM_EXP, makeCreateWindowCallback(STAT_create_chimera_check_window, GLOBAL.gb_main));

            awm->sep______________();

            GDE_load_menu(awm, AWM_ALL, "Print");
        }

        // -------------
        //      SAI

        awm->create_menu("SAI", "A", AWM_ALL);
        {
            awm->insert_menu_topic("sai_admin", "Manage SAIs", "S", "ad_extended.hlp", AWM_ALL, NT_create_extendeds_window);
            awm->insert_sub_menu("Create SAI using ...", "C");
            {
                awm->insert_menu_topic("sai_max_freq",  "Max. Frequency",                                               "M", "max_freq.hlp",     AWM_EXP, AP_create_max_freq_window);
                awm->insert_menu_topic("sai_consensus", "Consensus",                                                    "C", "consensus.hlp",    AWM_ALL, AP_create_con_expert_window);
                awm->insert_menu_topic("pos_var_pars",  "Positional variability + Column statistic (parsimony method)", "a", "pos_var_pars.hlp", AWM_ALL, AP_create_pos_var_pars_window);
                awm->insert_menu_topic("arb_phyl",      "Filter by base frequency",                                     "F", "phylo.hlp",        AWM_ALL, makeWindowCallback(NT_system_cb, "arb_phylo &"));
                awm->insert_menu_topic("sai_pfold",     "Protein secondary structure (field \"sec_struct\")",           "P", "pfold.hlp",        AWM_EXP, makeWindowCallback(NT_create_sai_from_pfold, ntw));

                GDE_load_menu(awm, AWM_EXP, "SAI");
            }
            awm->close_sub_menu();

            awm->insert_sub_menu("Other functions", "O");
            {
                awm->insert_menu_topic("count_different_chars", "Count different chars/column",             "C", "count_chars.hlp",    AWM_EXP, makeWindowCallback(NT_count_different_chars, GLOBAL.gb_main));
                awm->insert_menu_topic("corr_mutat_analysis",   "Compute clusters of correlated positions", "m", "",                   AWM_ALL, makeWindowCallback(NT_system_in_xterm_cb,    "arb_rnacma"));
                awm->insert_menu_topic("export_pos_var",        "Export Column Statistic (GNUPLOT format)", "E", "csp_2_gnuplot.hlp",  AWM_ALL, NT_create_colstat_2_gnuplot_window);
            }
            awm->close_sub_menu();
        }

        // ----------------
        //      Probes

        awm->create_menu("Probes", "P", AWM_ALL);
        {
            awm->insert_menu_topic("probe_design", "Design Probes",          "D", "probedesign.hlp", AWM_ALL, makeCreateWindowCallback(create_probe_design_window, GLOBAL.gb_main));
            awm->insert_menu_topic("probe_multi",  "Calculate Multi-Probes", "u", "multiprobe.hlp",  AWM_ALL, makeCreateWindowCallback(create_multiprobe_window, ntw));
            awm->insert_menu_topic("probe_match",  "Match Probes",           "M", "probematch.hlp",  AWM_ALL, makeCreateWindowCallback(create_probe_match_window, GLOBAL.gb_main));
            awm->sep______________();
            awm->insert_menu_topic("probe_match_sp",   "Match Probes with Specificity", "f", "probespec.hlp", AWM_ALL, makeCreateWindowCallback(create_probe_match_with_specificity_window,    ntw));
            awm->sep______________();
            awm->insert_menu_topic("primer_design_new", "Design Primers",            "P", "primer_new.hlp", AWM_EXP, makeCreateWindowCallback(create_primer_design_window, GLOBAL.gb_main));
            awm->insert_menu_topic("primer_design",     "Design Sequencing Primers", "S", "primer.hlp",     AWM_EXP, NT_primer_cb);
            awm->sep______________();
            awm->insert_menu_topic("pt_server_admin",   "PT_SERVER Admin",           "A", "probeadmin.hlp",  AWM_ALL, makeCreateWindowCallback(create_probe_admin_window, GLOBAL.gb_main));
        }

    }

    // --------------
    //      Tree

    awm->create_menu("Tree", "T", AWM_ALL);
    {
        if (!clone) {
            awm->insert_sub_menu("Add Species to Existing Tree", "A");
            {
                awm->insert_menu_topic("arb_pars_quick", "ARB Parsimony (Quick add marked)", "Q", "pars.hlp", AWM_ALL, makeWindowCallback(NT_system_cb, "arb_pars -add_marked -quit &"));
                awm->insert_menu_topic("arb_pars",       "ARB Parsimony interactive",        "i", "pars.hlp", AWM_ALL, makeWindowCallback(NT_system_cb, "arb_pars &"));
                GDE_load_menu(awm, AWM_EXP, "Incremental phylogeny");
            }
            awm->close_sub_menu();
        }

        awm->insert_sub_menu("Remove Species from Tree",     "R");
        {
            awm->insert_menu_topic(awm->local_id("tree_remove_deleted"), "Remove zombies", "z", "trm_del.hlp",    AWM_ALL, makeWindowCallback(NT_remove_leafs, ntw, AWT_REMOVE_ZOMBIES));
            awm->insert_menu_topic(awm->local_id("tree_remove_marked"),  "Remove marked",  "m", "trm_mrkd.hlp",   AWM_ALL, makeWindowCallback(NT_remove_leafs, ntw, AWT_REMOVE_MARKED));
            awm->insert_menu_topic(awm->local_id("tree_keep_marked"),    "Keep marked",    "K", "tkeep_mrkd.hlp", AWM_ALL, makeWindowCallback(NT_remove_leafs, ntw, AWT_KEEP_MARKED));
            awm->sep______________();
            awm->insert_menu_topic("all_tree_remove_deleted", "Remove zombies from ALL trees", "A", "trm_del.hlp",  AWM_ALL, makeWindowCallback(NT_alltree_remove_leafs, GBT_REMOVE_ZOMBIES, GLOBAL.gb_main));
            awm->insert_menu_topic("all_tree_remove_marked",  "Remove marked from ALL trees",  "L", "trm_mrkd.hlp", AWM_ALL, makeWindowCallback(NT_alltree_remove_leafs, GBT_REMOVE_MARKED,  GLOBAL.gb_main));
        }
        awm->close_sub_menu();

        if (!clone) {
            awm->insert_sub_menu("Build tree from sequence data",    "B");
            {
                awm->insert_sub_menu("Distance matrix methods", "D");
                awm->insert_menu_topic("arb_dist", "Distance Matrix + ARB NJ", "J", "dist.hlp", AWM_ALL, makeWindowCallback(NT_system_cb, "arb_dist &"));
                GDE_load_menu(awm, AWM_ALL, "Phylogeny Distance Matrix");
                awm->close_sub_menu();

                awm->insert_sub_menu("Maximum Parsimony methods", "P");
                GDE_load_menu(awm, AWM_ALL, "Phylogeny max. parsimony");
                awm->close_sub_menu();

                awm->insert_sub_menu("Maximum Likelihood methods", "L");
                GDE_load_menu(awm, AWM_EXP, "Phylogeny max. Likelihood EXP");
                GDE_load_menu(awm, AWM_ALL, "Phylogeny max. Likelihood");
                awm->close_sub_menu();

                awm->insert_sub_menu("Other methods", "O", AWM_EXP);
                GDE_load_menu(awm, AWM_EXP, "Phylogeny (Other)");
                awm->close_sub_menu();
            }
            awm->close_sub_menu();
        }

        awm->insert_menu_topic(awm->local_id("consense_tree"), "Build consensus tree", "c", "consense_tree.hlp", AWM_ALL, NT_create_consense_window);
        awm->sep______________();

        awm->insert_sub_menu("Reset zoom",         "z");
        {
            awm->insert_menu_topic(awm->local_id("reset_logical_zoom"),  "Logical zoom",  "L", "rst_log_zoom.hlp",  AWM_ALL, makeWindowCallback(NT_reset_lzoom_cb, ntw));
            awm->insert_menu_topic(awm->local_id("reset_physical_zoom"), "Physical zoom", "P", "rst_phys_zoom.hlp", AWM_ALL, makeWindowCallback(NT_reset_pzoom_cb, ntw));
        }
        awm->close_sub_menu();
        NT_insert_collapse_submenu(awm, ntw);
        awm->insert_sub_menu("Beautify tree", "e");
        {
            awm->insert_menu_topic(awm->local_id("beautifyt_tree"),  "#beautifyt.xpm",  "",  "resorttree.hlp", AWM_ALL, makeWindowCallback(NT_resort_tree_cb, ntw, BIG_BRANCHES_TO_TOP));
            awm->insert_menu_topic(awm->local_id("beautifyc_tree"),  "#beautifyc.xpm",  "",  "resorttree.hlp", AWM_ALL, makeWindowCallback(NT_resort_tree_cb, ntw, BIG_BRANCHES_TO_EDGE));
            awm->insert_menu_topic(awm->local_id("beautifyb_tree"),  "#beautifyb.xpm",  "",  "resorttree.hlp", AWM_ALL, makeWindowCallback(NT_resort_tree_cb, ntw, BIG_BRANCHES_TO_BOTTOM));
            awm->insert_menu_topic(awm->local_id("beautifyr_tree"),  "Radial tree (1)", "1", "resorttree.hlp", AWM_ALL, makeWindowCallback(NT_resort_tree_cb, ntw, BIG_BRANCHES_TO_CENTER));
            awm->insert_menu_topic(awm->local_id("beautifyr2_tree"), "Radial tree (2)", "2", "resorttree.hlp", AWM_ALL, makeWindowCallback(NT_resort_tree_cb, ntw, BIG_BRANCHES_ALTERNATING));
            awm->sep______________();
            awm->insert_menu_topic(awm->local_id("sort_by_other"),   "By other tree",   "o", "resortbyother.hlp", AWM_ALL, makeCreateWindowCallback(NT_create_sort_tree_by_other_tree_window, ntw));
        }
        awm->close_sub_menu();
        awm->insert_sub_menu("Modify branches", "M");
        {
            awm->insert_menu_topic(awm->local_id("tree_remove_remark"),     "Remove bootstraps",      "b", "trm_boot.hlp", AWM_ALL, makeWindowCallback(NT_remove_bootstrap,    ntw));
            awm->insert_menu_topic(awm->local_id("tree_toggle_bootstraps"), "Toggle 100% bootstraps", "1", "trm_boot.hlp", AWM_ALL, makeWindowCallback(NT_toggle_bootstrap100, ntw));
            awm->sep______________();
            awm->insert_menu_topic(awm->local_id("tree_reset_lengths"),     "Reset branchlengths",   "R", "tbl_reset.hlp",   AWM_ALL, makeWindowCallback(NT_reset_branchlengths,   ntw));
            awm->insert_menu_topic(awm->local_id("justify_branch_lengths"), "Justify branchlengths", "J", "tbl_justify.hlp", AWM_ALL, makeWindowCallback(NT_justify_branch_lenghs, ntw));
            awm->insert_menu_topic(awm->local_id("tree_scale_lengths"),     "Scale Branchlengths",   "S", "tbl_scale.hlp",   AWM_ALL, makeWindowCallback(NT_scale_tree,            ntw));
            awm->sep______________();
            awm->insert_menu_topic(awm->local_id("tree_multifurcate"), "Multifurcate...", "M", "multifurcate.hlp", AWM_ALL, makeCreateWindowCallback(NT_create_multifurcate_tree_window, ntw));
            awm->sep______________();
            awm->insert_menu_topic(awm->local_id("tree_boot2len"), "Bootstraps -> Branchlengths", "l", "tbl_boot2len.hlp", AWM_ALL, makeWindowCallback(NT_move_boot_branch, ntw, 0));
            awm->insert_menu_topic(awm->local_id("tree_len2boot"), "Bootstraps <- Branchlengths", "o", "tbl_boot2len.hlp", AWM_ALL, makeWindowCallback(NT_move_boot_branch, ntw, 1));

        }
        awm->close_sub_menu();

        awm->insert_menu_topic(awm->local_id("branch_analysis"), "Branch analysis", "s", "branch_analysis.hlp", AWM_ALL, makeCreateWindowCallback(NT_create_branch_analysis_window, ntw));
        awm->insert_menu_topic(awm->local_id("mark_duplicates"), "Mark duplicates", "u", "mark_duplicates.hlp", AWM_ALL, makeWindowCallback(NT_mark_duplicates, ntw));

        awm->sep______________();

        awm->insert_menu_topic(awm->local_id("compare_taxonomy"), "Compare taxonomy...", "x", "compare_taxonomy.hlp", AWM_ALL, makeCreateWindowCallback(NT_create_compare_taxonomy_window, ntw));

        awm->sep______________();

        awm->insert_menu_topic(awm->local_id("tree_select"),        "Select Tree",      "T", 0, AWM_ALL, makeCreateWindowCallback(NT_create_select_tree_window, awar_tree));
        awm->insert_menu_topic(awm->local_id("tree_select_latest"), "Select Last Tree", "L", 0, AWM_ALL, makeWindowCallback(NT_select_bottom_tree, awar_tree));

        awm->sep______________();

        if (!clone) {
            awm->insert_menu_topic("tree_admin", "Tree admin",               "i", "treeadm.hlp",   AWM_ALL, popup_tree_admin_window);
            awm->insert_menu_topic("nds",        "NDS (Node display setup)", "N", "props_nds.hlp", AWM_ALL, makeCreateWindowCallback(AWT_create_nds_window, GLOBAL.gb_main));
        }
        awm->sep______________();

        awm->insert_menu_topic("transversion",       "Transversion analysis",   "y", "trans_anal.hlp", AWM_EXP, makeHelpCallback("trans_anal.hlp"));

        awm->sep______________();

        if (!clone) {
            awm->insert_menu_topic("print_tree",  "Print tree",          "P", "tree2prt.hlp",  AWM_ALL, makeWindowCallback(AWT_popup_print_window, ntw));
            awm->insert_menu_topic("tree_2_xfig", "Export tree to XFIG", "X", "tree2file.hlp", AWM_ALL, makeWindowCallback(AWT_popup_tree_export_window, ntw));
            awm->sep______________();
        }

        if (is_genome_db) {
            awm->insert_sub_menu("Other..",  "O", AWM_EXP);
            {
                awm->insert_menu_topic(awm->local_id("tree_pseudo_species_to_organism"), "Relink tree to organisms", "o", "tree_pseudo.hlp", AWM_EXP, makeWindowCallback(NT_pseudo_species_to_organism, ntw));
            }
            awm->close_sub_menu();
        }
    }

    if (!clone) {
        // ---------------
        //      Tools

        awm->create_menu("Tools", "o", AWM_ALL);
        {
            awm->insert_menu_topic("names_admin",      "Name server admin (IDs)",    "s", "namesadmin.hlp",  AWM_ALL, makeCreateWindowCallback(AW_create_namesadmin_window, GLOBAL.gb_main));
            awm->insert_sub_menu("DB admin", "D", AWM_EXP);
            {
                awm->insert_menu_topic("db_admin", "Re-repair DB", "R", "rerepair.hlp", AWM_EXP, makeWindowCallback(NT_rerepair_DB, GLOBAL.gb_main));
            }
            awm->close_sub_menu();
            awm->insert_sub_menu("Network", "N", AWM_EXP);
            {
                GDE_load_menu(awm, AWM_EXP, "Network");
            }
            awm->close_sub_menu();
            awm->sep______________();

            awm->insert_sub_menu("GDE specials", "G", AWM_EXP);
            {
                GDE_load_menu(awm, AWM_EXP, NULL);
            }
            awm->close_sub_menu();

            awm->insert_sub_menu("WL specials", "W", AWM_EXP);
            {
                awm->insert_menu_topic(awm->local_id("view_probe_group_result"), "View probe group result", "V", "", AWM_EXP, makeCreateWindowCallback(create_probe_group_result_window, ntw));
            }
            awm->close_sub_menu();

            awm->sep______________();
            awm->insert_menu_topic("xterm", "Start XTERM", "X", 0, AWM_ALL, NT_xterm);
            awm->sep______________();
            GDE_load_menu(awm, AWM_EXP, "User");
        }
        // --------------------
        //      Properties

        awm->create_menu("Properties", "r", AWM_ALL);
        {
#if defined(ARB_MOTIF)
            awm->insert_menu_topic("props_menu",                 "Frame settings ...",          "F", "props_frame.hlp",      AWM_ALL, AW_preset_window);
#endif
            awm->insert_menu_topic(awm->local_id("props_tree2"), "Tree options",                "o", "nt_tree_settings.hlp", AWM_ALL, TREE_create_settings_window);
            awm->insert_menu_topic("props_tree",                 "Tree colors & fonts",         "c", "color_props.hlp",      AWM_ALL, makeCreateWindowCallback(AW_create_gc_window, ntw->gc_manager));
            awm->insert_menu_topic("props_www",                  "Search world wide web (WWW)", "W", "props_www.hlp",        AWM_ALL, makeCreateWindowCallback(AWT_create_www_window, GLOBAL.gb_main));
            awm->sep______________();
            awm->insert_menu_topic("!toggle_expert", "Toggle expert mode",         "x", 0, AWM_ALL, NT_toggle_expert_mode);
#if defined(ARB_MOTIF)
            awm->insert_menu_topic("!toggle_focus",  "Toggle focus follows mouse", "m", 0, AWM_ALL, NT_toggle_focus_policy);
#endif
            awm->sep______________();
            AW_insert_common_property_menu_entries(awm);
            awm->sep______________();
            awm->insert_menu_topic("save_props", "Save properties (ntree.arb)", "S", "savedef.hlp", AWM_ALL, AW_save_properties);
        }
    }

    awm->insert_help_topic("ARB_NT help",     "N", "arb_ntree.hlp", AWM_ALL, makeHelpCallback("arb_ntree.hlp"));

    // ----------------------
    //      mode buttons
    //
    // keep them synchronized as far as possible with those in ARB_PARSIMONY
    // see ../PARSIMONY/PARS_main.cxx@keepModesSynchronized

    awm->create_mode("mode_select.xpm", "mode_select.hlp", AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_SELECT));
    awm->create_mode("mode_mark.xpm",   "mode_mark.hlp",   AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_MARK));
    awm->create_mode("mode_group.xpm",  "mode_group.hlp",  AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_GROUP));
    awm->create_mode("mode_zoom.xpm",   "mode_pzoom.hlp",  AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_ZOOM));
    awm->create_mode("mode_lzoom.xpm",  "mode_lzoom.hlp",  AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_LZOOM));


    awm->create_mode("mode_info.xpm", "mode_info.hlp", AWM_ALL, makeWindowCallback(NT_modify_cb,  ntw, AWT_MODE_INFO));
    awm->create_mode("mode_www.xpm",  "mode_www.hlp",  AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_WWW));

    // topology-modification-modes
    awm->create_mode("mode_setroot.xpm",   "mode_setroot.hlp", AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_SETROOT));
    awm->create_mode("mode_swap.xpm",      "mode_swap.hlp",    AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_SWAP));
    awm->create_mode("mode_move.xpm",      "mode_move.hlp",    AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_MOVE));
    awm->create_mode("mode_length.xpm",    "mode_length.hlp",  AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_LENGTH));
    awm->create_mode("mode_multifurc.xpm", "mode_length.hlp",  AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_MULTIFURC));

    // layout-modes
    awm->create_mode("mode_line.xpm",   "mode_width.hlp",  AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_LINE));
    awm->create_mode("mode_rotate.xpm", "mode_rotate.hlp", AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_ROTATE));
    awm->create_mode("mode_spread.xpm", "mode_spread.hlp", AWM_ALL, makeWindowCallback(nt_mode_event, ntw, AWT_MODE_SPREAD));

    awm->set_info_area_height(250);
    awm->at(5, 2);
    awm->auto_space(-2, -2);
    awm->shadow_width(1);

    // -------------------------
    //      create top area

    int first_liney;
    int leftx;
    awm->get_at_position(&leftx, &first_liney);

    // ---------------------
    //      quit + help

    awm->button_length(0);

    if (clone) {
        awm->callback(AW_POPDOWN);
        awm->create_button("CLOSE", "#close.xpm");
    }
    else {
        awm->callback(makeWindowCallback(NT_exit, EXIT_SUCCESS));
#if defined(ARB_GTK)
        awm->set_hide_on_close(false); //the main window should really close when closed
#endif
        awm->help_text("quit.hlp");
        awm->create_button("QUIT", "#quit.xpm");
#if defined(ARB_GTK)
        awm->set_close_action("QUIT");
#endif
    }

    int db_pathx = awm->get_at_xposition();

    // now fetch the yposition for the 2nd button line
    awm->at_newline();
    int second_liney = awm->get_at_yposition();

    awm->callback(AW_help_entry_pressed);
    awm->help_text("arb_ntree.hlp");
    awm->create_button("?", "#help.xpm");

    // --------------------------
    //      save + undo/redo

    awm->callback(NT_save_quick_cb);
    awm->help_text("save.hlp");
    awm->create_button("SAVE", "#save.xpm");

    awm->callback(makeCreateWindowCallback(NT_create_save_as, AWAR_DBBASE));
    awm->help_text("save.hlp");
    awm->create_button("SAVE_AS", "#saveAs.xpm");

    // undo/redo:
    awm->callback(makeWindowCallback(NT_undo_cb, GB_UNDO_UNDO, ntw));
    awm->help_text("undo.hlp");
    awm->create_button("UNDO", "#undo.xpm");

    awm->callback(makeWindowCallback(NT_undo_cb, GB_UNDO_REDO, ntw));
    awm->help_text("undo.hlp");
    awm->create_button("REDO", "#redo.xpm");

    int db_pathx2 = awm->get_at_xposition();

    // fetch position for mode help-line:
    awm->at_newline();
    int third_liney = awm->get_at_yposition();

    awm->at(db_pathx, first_liney);
    // size of DB-name button is determined by buttons below:
    awm->at_set_to(false, false, db_pathx2-db_pathx-1, second_liney-first_liney+1);
    awm->callback(makeCreateWindowCallback(NT_create_save_quick_as_window, AWAR_DBBASE));
    awm->help_text("save.hlp");
    awm->create_button("QUICK_SAVE_AS", AWAR_DB_NAME);

    // -----------------------------
    //      tree + tree display

#define TREE_BUTTON_OVERSIZE 6

    int db_treex      = awm->get_at_xposition();
    int second_uppery = second_liney-TREE_BUTTON_OVERSIZE;

    awm->at(db_treex, second_uppery);
    awm->button_length(0);

    awm->callback(makeWindowCallback(NT_set_tree_style, ntw, AP_TREE_RADIAL));
    awm->help_text("tr_type_radial.hlp");
    awm->create_button("RADIAL_TREE_TYPE", "#radial.xpm");

    awm->callback(makeWindowCallback(NT_set_tree_style, ntw, AP_TREE_NORMAL));
    awm->help_text("tr_type_list.hlp");
    awm->create_button("LIST_TREE_TYPE", "#dendro.xpm");

    awm->callback(makeWindowCallback(NT_set_tree_style, ntw, AP_TREE_IRS));
    awm->help_text("tr_type_irs.hlp");
    awm->create_button("FOLDED_LIST_TREE_TYPE", "#dendroIrs.xpm");

    awm->callback(makeWindowCallback(NT_set_tree_style, ntw, AP_LIST_NDS));
    awm->help_text("tr_type_nds.hlp");
    awm->create_button("NO_TREE_TYPE", "#listdisp.xpm");

    int db_treex2 = awm->get_at_xposition();

    awm->at(db_treex, first_liney);
    // size of tree-name button is determined by buttons below:
    awm->at_set_to(false, false, db_treex2-db_treex-1, second_uppery-first_liney+1);
    awm->callback(makeCreateWindowCallback(NT_create_select_tree_window, awar_tree));
    awm->help_text("nt_tree_select.hlp");
    awm->create_button("SELECT_A_TREE", awar_tree);

    // ---------------------------------------------------------------
    //      protection, alignment + editor (and maybe Genome Map)

    int db_alignx = awm->get_at_xposition();

    // editor and genemap buttons have to be 44x24 sized
#define EDIT_XSIZE 50
#define EDIT_YSIZE 30 // same as 'save as' buttons

    // first draw protection:
    int protectx = db_alignx + 2*EDIT_XSIZE;

    awm->at(protectx+2, first_liney+1);
    awm->button_length(0);
    awm->create_button(NULL, "#protect.xpm");

    awm->at(protectx, second_liney+2);
    awm->create_option_menu(AWAR_SECURITY_LEVEL, true);
    awm->insert_option("0", 0, 0);
    awm->insert_option("1", 0, 1);
    awm->insert_option("2", 0, 2);
    awm->insert_option("3", 0, 3);
    awm->insert_option("4", 0, 4);
    awm->insert_option("5", 0, 5);
    awm->insert_default_option("6", 0, 6);
    awm->update_option_menu();

    int db_searchx = awm->get_at_xposition() - 7;

    // draw ali/editor buttons AFTER protect menu (to get rid of it's label)
    awm->at(db_alignx, second_liney);

    awm->at_set_to(false, false, ((2-is_genome_db)*EDIT_XSIZE), EDIT_YSIZE);
    awm->callback(makeWindowCallback(NT_start_editor_on_tree, 0, ntw));
    awm->help_text("arb_edit4.hlp");
    if (is_genome_db) awm->create_button("EDIT_SEQUENCES", "#editor_small.xpm");
    else              awm->create_button("EDIT_SEQUENCES", "#editor.xpm");

    if (is_genome_db) {
        awm->at_set_to(false, false, EDIT_XSIZE, EDIT_YSIZE);
        awm->callback(makeCreateWindowCallback(GEN_create_first_map, GLOBAL.gb_main)); // initial gene map
        awm->help_text("gene_map.hlp");
        awm->create_button("OPEN_GENE_MAP", "#gen_map.xpm");
    }

    int db_alignx2 = awm->get_at_xposition();

    awm->at(db_alignx, first_liney);
    awm->at_set_to(false, false, db_alignx2-db_alignx-1, second_liney-first_liney+1);
    awm->callback(NT_create_select_alignment_window);
    awm->help_text("nt_align_select.hlp");
    awm->create_button("SELECT_AN_ALIGNMENT", AWAR_DEFAULT_ALIGNMENT);

    // -------------------------------
    //      create mode-help line

    awm->at(leftx, third_liney);
    awm->button_length(AWAR_FOOTER_MAX_LEN);
    awm->create_button(0, AWAR_FOOTER);

    awm->at_newline();
    int bottomy = awm->get_at_yposition();

    // ---------------------------------------
    //      Info / marked / Search / Jump

    awm->button_length(7);

    awm->at(db_searchx, first_liney);
    awm->callback(makeCreateWindowCallback(DBUI::create_species_query_window, GLOBAL.gb_main));
    awm->help_text("sp_search.hlp");
    awm->create_button("SEARCH",  "Search");

    awm->at(db_searchx, second_uppery);
    awm->callback(makeWindowCallback(NT_jump_cb, ntw, AP_JUMP_BY_BUTTON));
    awm->help_text("tr_jump.hlp");
    awm->create_button("JUMP", "Jump");

    int db_infox = awm->get_at_xposition();

    awm->at(db_infox, first_liney);
    awm->button_length(13);
    awm->callback(popupinfo_wcb);
    awm->help_text("sp_search.hlp");
    awm->create_button("INFO",  AWAR_INFO_BUTTON_TEXT);

    awm->at(db_infox, second_uppery);
    awm->button_length(13);
    awm->help_text("species_configs.hlp");
    awm->callback(makeWindowCallback(NT_popup_configuration_admin, ntw));
    awm->create_button("selection_admin2", AWAR_MARKED_SPECIES_COUNTER);
    {
        GBDATA *gb_species_data = GBT_get_species_data(GLOBAL.gb_main);
        GB_add_callback(gb_species_data, GB_CB_CHANGED, makeDatabaseCallback(NT_update_marked_counter, static_cast<AW_window*>(awm)));
        NT_update_marked_counter(NULL, awm);
    }

    // set height of top area:
    awm->set_info_area_height(bottomy+2);
    awm->set_bottom_area_height(0);

    NT_activate_configMarkers_display(ntw);

    // ------------------------------------
    //      Autostarts for development

#if defined(DEVEL_RALF)
    // if (is_genome_db) GEN_map_first(awr)->show();
#endif // DEVEL_RALF

    GB_pop_transaction(GLOBAL.gb_main);

#if defined(DEBUG)
    AWT_check_action_ids(awr, is_genome_db ? "_genome" : "");
#endif

    return awm;
}

AWT_canvas *NT_create_main_window(AW_root *aw_root) {
    GB_ERROR error = GB_request_undo_type(GLOBAL.gb_main, GB_UNDO_NONE);
    if (error) aw_message(error);

    nt_create_all_awars(aw_root, AW_ROOT_DEFAULT);

    AWT_canvas *ntw = NULL;
    AW_window  *aww = popup_new_main_window(aw_root, 0, &ntw);
    aww->show();

    error = GB_request_undo_type(GLOBAL.gb_main, GB_UNDO_UNDO);
    if (error) aw_message(error);

    return ntw;
}
