// =============================================================== //
//                                                                 //
//   File      : MG_main.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "merge.hxx"
#include <AW_rename.hxx>
#include <awt.hxx>
#include <awt_misc.hxx>

#include <aw_preset.hxx>
#include <aw_awars.hxx>
#include <aw_file.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <aw_question.hxx>

#include <arb_progress.h>
#include <arb_file.h>
#include <macros.hxx>

// AISC_MKPT_PROMOTE:// source and destination DBs for merge:
// AISC_MKPT_PROMOTE:extern GBDATA *GLOBAL_gb_src;
// AISC_MKPT_PROMOTE:extern GBDATA *GLOBAL_gb_dst;

GBDATA *GLOBAL_gb_src = NULL;
GBDATA *GLOBAL_gb_dst = NULL;

static void (*MG_exit_cb)(const char *) = NULL;

static void MG_exit(AW_window *aww, bool start_dst_db) {
    char *arb_ntree_restart_args = NULL;

    if (start_dst_db) {
        // restart with destination DB
        const char *dst_db_name = aww->get_root()->awar(AWAR_DB_DST"/file_name")->read_char_pntr();
        arb_ntree_restart_args  = GBS_global_string_copy("'%s'", dst_db_name);
    }
    else {
        // restart in directory of destination- or source-DB
        const char *dst_db_dir = aww->get_root()->awar(AWAR_DB_DST"/directory")->read_char_pntr();
        if (GB_is_directory(dst_db_dir)) {
            arb_ntree_restart_args = GBS_global_string_copy("'%s'", dst_db_dir);
        }
        else {
            const char *src_db_dir = aww->get_root()->awar(AWAR_DB_SRC"/directory")->read_char_pntr();
            if (GB_is_directory(src_db_dir)) {
                arb_ntree_restart_args = GBS_global_string_copy("'%s'", src_db_dir);
            }
        }
    }

    shutdown_macro_recording(AW_root::SINGLETON);

    // @@@ code below duplicates code from nt_disconnect_from_db()
    aww->get_root()->unlink_awars_from_DB(GLOBAL_gb_src);
    aww->get_root()->unlink_awars_from_DB(GLOBAL_gb_dst);

    GB_close(GLOBAL_gb_src);
    GB_close(GLOBAL_gb_dst);

    mg_assert(MG_exit_cb);

    MG_exit_cb(arb_ntree_restart_args);
}

static void MG_save_cb(AW_window *aww, bool source_database) {
    GBDATA     *gb_db_to_save = source_database ? GLOBAL_gb_src : GLOBAL_gb_dst;
    const char *base_name     = source_database ? AWAR_DB_SRC : AWAR_DB_DST; // awar basename

    AW_root    *awr   = aww->get_root();
    char       *name  = awr->awar(GBS_global_string("%s/file_name", base_name))->read_string();
    const char *atype = awr->awar(GBS_global_string("%s/type", base_name))->read_char_pntr();
    const char *ctype = awr->awar(GBS_global_string("%s/compression", base_name))->read_char_pntr();
    char       *type  = GBS_global_string_copy("%s%s", atype, ctype);

    arb_progress progress(GBS_global_string("Saving %s database", source_database ? "source" : "target"));

    awr->dont_save_awars_with_default_value(gb_db_to_save); // has to be done outside transaction!
    GB_begin_transaction(gb_db_to_save);
    GBT_check_data(gb_db_to_save, 0);
    GB_commit_transaction(gb_db_to_save);

    GB_ERROR error = GB_save(gb_db_to_save, name, type);
    if (error) aw_message(error);
    else     AW_refresh_fileselection(awr, base_name);

    free(type);
    free(name);
}

static AW_window *MG_create_save_as_window(AW_root *aw_root, bool source_database) {
    GBDATA     *gb_db_to_save = source_database ? GLOBAL_gb_src : GLOBAL_gb_dst;
    const char *base_name     = source_database ? AWAR_DB_SRC : AWAR_DB_DST; // awar basename
    const char *window_id     = source_database ? "MERGE_SAVE_DB_I" : "MERGE_SAVE_WHOLE_DB";

    aw_root->awar_string(AWAR_DB_COMMENT, "<no description>", gb_db_to_save);

    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root, window_id, GBS_global_string("Save whole %s database", source_database ? "source" : "target"));
    aws->load_xfig("save_as.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("save.hlp"));
    aws->create_button("HELP", "HELP", "H");

    AW_create_standard_fileselection(aws, base_name);

    aws->at("type");
    AWT_insert_DBsaveType_selector(aws, GBS_global_string("%s/type", base_name));

    aws->at("compression");
    AWT_insert_DBcompression_selector(aws, GBS_global_string("%s/compression", base_name));

    aws->at("save");
    aws->callback(makeWindowCallback(MG_save_cb, source_database));
    aws->create_button("SAVE", "SAVE", "S");

    aws->at("comment");
    aws->create_text_field(AWAR_DB_COMMENT);

    return aws;
}

static void MG_save_quick_result_cb(AW_window *aww) {
    char *name = aww->get_root()->awar(AWAR_DB_DST"/file_name")->read_string();

    arb_progress progress("Saving database");
    GB_begin_transaction(GLOBAL_gb_dst);
    GBT_check_data(GLOBAL_gb_dst, 0);
    GB_commit_transaction(GLOBAL_gb_dst);
    
    GB_ERROR error = GB_save_quick_as(GLOBAL_gb_dst, name);
    if (error) aw_message(error);
    else AW_refresh_fileselection(aww->get_root(), AWAR_DB_DST);

    free(name);
}

static void MG_create_db_dependent_awars(AW_root *aw_root, GBDATA *gb_src, GBDATA *gb_dst) {
    MG_create_db_dependent_rename_awars(aw_root, gb_src, gb_dst);
}

AW_window *MERGE_create_main_window(AW_root *aw_root, bool dst_is_new, void (*exit_cb)(const char *)) {
    // 'dst_is_new' == true -> setup dest DB according to source-DB
    //
    // GLOBAL_gb_src and GLOBAL_gb_dst have to be opened before

    // @@@ add GB_is_client and use that here
    bool src_is_client = GB_read_clients(GLOBAL_gb_src)<0;
    bool dst_is_client = GB_read_clients(GLOBAL_gb_dst)<0;

    MG_exit_cb = exit_cb;

    mg_assert(contradicted(src_is_client, GB_is_server(GLOBAL_gb_src)));
    mg_assert(contradicted(dst_is_client, GB_is_server(GLOBAL_gb_dst)));

    mg_assert(!(src_is_client && dst_is_client));

    bool save_src_enabled = !src_is_client;
    bool save_dst_enabled = !dst_is_client;

    {
        GBDATA     *gb_main_4_macros = NULL;
        const char *app_id           = NULL;

        if (src_is_client) {
            gb_main_4_macros = GLOBAL_gb_src;
            app_id           = "ARB_MERGE_OUTOF";
        }
        else if (dst_is_client) {
            gb_main_4_macros = GLOBAL_gb_dst;
            app_id           = "ARB_MERGE_INTO";
        }
        else {
            gb_main_4_macros = GLOBAL_gb_dst; // does not matter
            app_id           = "ARB_MERGE";
        }

        GB_ERROR error = configure_macro_recording(aw_root, app_id, gb_main_4_macros);
        aw_message_if(error);
    }

    mg_assert(aw_root);

    {
        GB_transaction ta_merge(GLOBAL_gb_src);
        GB_transaction ta_dest(GLOBAL_gb_dst);

        GBT_mark_all(GLOBAL_gb_dst, 0); // unmark everything in dest DB

        // set DB-type to non-genome (compatibility to old DBs)
        // when exporting to new DB (dest_is_new == true) -> use DB-type of merge-DB
        bool merge_is_genome = GEN_is_genome_db(GLOBAL_gb_src, 0);

        int dest_genome = 0;
        if (dst_is_new) {
            if (merge_is_genome) {
                dest_genome = aw_question("select_dest_dbtype", "Enter destination DB-type", "Normal,Genome");
            }
            else {
                dest_genome = 0; // from non-genome -> to non-genome
            }
        }

        GEN_is_genome_db(GLOBAL_gb_dst, dest_genome); // does not change anything if type is already defined
    }

    {
        GB_transaction ta_merge(GLOBAL_gb_src);
        GB_transaction ta_dest(GLOBAL_gb_dst);

        GB_change_my_security(GLOBAL_gb_dst, 6);
        GB_change_my_security(GLOBAL_gb_src, 6);

        MG_create_db_dependent_awars(aw_root, GLOBAL_gb_src, GLOBAL_gb_dst);
    }

    {
        GB_transaction ta_merge(GLOBAL_gb_src);
        GB_transaction ta_dest(GLOBAL_gb_dst);

        MG_set_renamed(false, aw_root, "Not renamed yet.");

        AW_window_simple_menu *awm = new AW_window_simple_menu;
        awm->init(aw_root, "ARB_MERGE", "ARB_MERGE");
        awm->load_xfig("merge/main.fig");

        // create menus
#if defined(DEBUG)
        AWT_create_debug_menu(awm);
#endif // DEBUG

        awm->create_menu("File", "F", AWM_ALL);
        if (save_src_enabled) {
            awm->insert_menu_topic("save_DB1", "Save source DB ...", "S", "save.hlp", AWM_ALL, makeCreateWindowCallback(MG_create_save_as_window, true));
        }

        awm->insert_menu_topic("quit", "Quit", "Q", "quit.hlp", AWM_ALL, makeWindowCallback(MG_exit, false));
        if (save_dst_enabled) {
            awm->insert_menu_topic("quitnstart", "Quit & start target DB", "D", "quit.hlp", AWM_ALL, makeWindowCallback(MG_exit, true));
        }

        insert_macro_menu_entry(awm, true);

        awm->create_menu("Properties", "P", AWM_ALL);
        AW_insert_common_property_menu_entries(awm);
        awm->sep______________();
        awm->insert_menu_topic("save_props", "Save properties (ntree.arb)", "p", "savedef.hlp", AWM_ALL, AW_save_properties);

        awm->insert_help_topic("ARB_MERGE help", "M", "arb_merge.hlp", AWM_ALL, makeHelpCallback("arb_merge.hlp"));


        // display involved databases
        awm->button_length(28);

        awm->at("db1"); awm->create_button(0, AWAR_DB_SRC"/name", 0, "+");
        awm->at("db2"); awm->create_button(0, AWAR_DB_DST"/name", 0, "+");


        // add main controls
        awm->button_length(32);

        {
            // conditional section:
            // when exporting into a new database there is no need to adapt alignments or names

            if (dst_is_new) awm->sens_mask(AWM_DISABLED);

            awm->at("alignment");
            awm->callback(MG_create_merge_alignment_window);
            awm->help_text("mg_alignment.hlp");
            awm->create_button("CHECK_ALIGNMENTS", "Check alignments ...");

            awm->at("names");
            awm->callback(MG_create_merge_names_window);
            awm->help_text("mg_names.hlp");
            awm->create_button("CHECK_NAMES", "Check IDs ...");

            if (dst_is_new) {
                awm->sens_mask(AWM_ALL);
                MG_set_renamed(true, aw_root, "Not necessary");
            }
        }

        awm->at("species");
        awm->callback(makeCreateWindowCallback(MG_create_merge_species_window, dst_is_new));
        awm->help_text("mg_species.hlp");
        awm->create_button("TRANSFER_SPECIES", "Transfer species ... ");

        awm->at("extendeds");
        awm->callback(MG_create_merge_SAIs_window);
        awm->help_text("mg_extendeds.hlp");
        awm->create_button("TRANSFER_SAIS", "Transfer SAIs ...");

        awm->at("trees");
        awm->callback(MG_create_merge_trees_window);
        awm->help_text("mg_trees.hlp");
        awm->create_button("TRANSFER_TREES", "Transfer trees ...");

        awm->at("configs");
        awm->callback(MG_create_merge_configs_window);
        awm->help_text("mg_species_configs.hlp");
        awm->create_button("TRANSFER_CONFIGS", "Transfer configurations ...");

        {
            // conditional section:
            // allow save only when not merging into DB running inside ARB_NT

            if (!save_dst_enabled) awm->sens_mask(AWM_DISABLED);

            awm->at("save");
            awm->callback(makeCreateWindowCallback(MG_create_save_as_window, false));
            awm->help_text("save.hlp");
            awm->create_button("SAVE_WHOLE_DB2", "Save whole target DB as ...");

            awm->at("save_quick");
            awm->callback(MG_save_quick_result_cb);
            awm->help_text("save.hlp");
            awm->create_button("SAVE_CHANGES_OF_DB2", "Quick-save changes of target DB");

            awm->sens_mask(AWM_ALL);
        }

        awm->at("quit");
        awm->callback(makeWindowCallback(MG_exit, false));
        awm->create_button("QUIT", save_dst_enabled ? "Quit" : "Close");

        awm->activate();

        return awm;
    }
}

void MERGE_create_all_awars(AW_root *awr, AW_default aw_def) {
    MG_create_trees_awar(awr, aw_def);
    MG_create_config_awar(awr, aw_def);
    MG_create_extendeds_awars(awr, aw_def);
    MG_create_species_awars(awr, aw_def);
    MG_create_gene_species_awars(awr, aw_def);

    MG_create_rename_awars(awr, aw_def);

#if defined(DEBUG)
    AWT_create_db_browser_awars(awr, aw_def);
#endif // DEBUG
}

inline const char *get_awar_name(const char *awar_base_name, const char *entry) {
    return GBS_global_string("%s/%s", awar_base_name, entry);
}

static void filename_changed_cb(AW_root *awr, const char *awar_base_name) {
    AW_awar *filename_awar = awr->awar(get_awar_name(awar_base_name, "file_name"));

    char *name;
    char *suffix;
    GB_split_full_path(filename_awar->read_char_pntr(), NULL, NULL, &name, &suffix);

    if (name) {
        AW_awar    *name_awar = awr->awar(get_awar_name(awar_base_name, "name"));
        const char *shown_name;
        if (strcmp(name, ":") == 0 && !suffix) {
            shown_name = ": (DB loaded in ARB_NT)";
        }
        else {
            shown_name = GB_append_suffix(name, suffix);
        }
        name_awar->write_string(shown_name);
    }

    free(name);
    free(suffix);
}

static void create_fileselection_and_name_awars(AW_root *awr, AW_default aw_def, const char *awar_base_name, const char *filename) {
    AW_create_fileselection_awars(awr, awar_base_name, "", ".arb", filename);

    awr->awar_string(get_awar_name(awar_base_name, "name"), "", aw_def); // create awar to display DB-names w/o path
    AW_awar *filename_awar = awr->awar(get_awar_name(awar_base_name, "file_name")); // defined by AW_create_fileselection_awars above

    filename_awar->add_callback(makeRootCallback(filename_changed_cb, awar_base_name));
    filename_awar->touch(); // trigger callback once
}

void MERGE_create_db_file_awars(AW_root *awr, AW_default aw_def, const char *src_name, const char *dst_name) {
    create_fileselection_and_name_awars(awr, aw_def, AWAR_DB_DST, dst_name);
    create_fileselection_and_name_awars(awr, aw_def, AWAR_DB_SRC, src_name);
}

