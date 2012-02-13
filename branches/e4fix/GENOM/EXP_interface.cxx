//  ==================================================================== //
//                                                                       //
//    File      : EXP_interface.cxx                                      //
//    Purpose   :                                                        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in September 2001        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#include "EXP_local.hxx"
#include "GEN_local.hxx"

#include <db_query.h>
#include <db_scanner.hxx>
#include <dbui.h>
#include <awt_sel_boxes.hxx>
#include <item_sel_list.h>
#include <aw_awar_defs.hxx>
#include <aw_detach.hxx>
#include <aw_msg.hxx>
#include <arbdbt.h>

using namespace std;

#define AD_F_ALL AWM_ALL

static GBDATA* EXP_get_current_experiment_data(GBDATA *gb_main, AW_root *aw_root) {
    GBDATA *gb_species         = GEN_get_current_organism(gb_main, aw_root);
    GBDATA *gb_experiment_data = 0;

    if (gb_species) gb_experiment_data = EXP_get_experiment_data(gb_species);

    return gb_experiment_data;
}

static void EXP_select_experiment(GBDATA* /* gb_main */, AW_root *aw_root, const char *item_name) {
    char *name  = strdup(item_name);
    char *slash = strchr(name, '/');

    if (slash) {
        slash[0] = 0;
        aw_root->awar(AWAR_ORGANISM_NAME)->write_string(name);
        aw_root->awar(AWAR_EXPERIMENT_NAME)->write_string(slash+1);
    }
    free(name);
}

static char *EXP_get_experiment_id(GBDATA * /* gb_main */, GBDATA *gb_experiment) {
    GBDATA *gb_species = GB_get_grandfather(gb_experiment);
    return GBS_global_string_copy("%s/%s", GBT_read_name(gb_species), GBT_read_name(gb_experiment));
}

static GBDATA *EXP_find_experiment_by_id(GBDATA *gb_main, const char *id) {
    char   *organism = strdup(id);
    char   *exp     = strchr(organism, '/');
    GBDATA *result   = 0;

    if (exp) {
        *exp++ = 0;
        GBDATA *gb_organism = GEN_find_organism(gb_main, organism);
        if (gb_organism) {
            result = EXP_find_experiment(gb_organism, exp);
        }
    }

    free(organism);
    return result;
}

static char *old_species_marks = 0; // configuration storing marked species

inline void exp_restore_old_species_marks(GBDATA *gb_main) {
    if (old_species_marks) {
        GBT_restore_marked_species(gb_main, old_species_marks);
        freenull(old_species_marks);
    }
}

static GBDATA *EXP_get_first_experiment_data(GBDATA *gb_main, AW_root *aw_root, QUERY_RANGE range) {
    GBDATA   *gb_organism = 0;
    GB_ERROR  error      = 0;

    exp_restore_old_species_marks(gb_main);

    switch (range) {
        case QUERY_CURRENT_ITEM: {
            char *species_name = aw_root->awar(AWAR_ORGANISM_NAME)->read_string();
            gb_organism         = GBT_find_species(gb_main, species_name);
            free(species_name);
            break;
        }
        case QUERY_MARKED_ITEMS: {
            GBDATA *gb_pseudo = GEN_first_marked_pseudo_species(gb_main);

            if (gb_pseudo) {    // there are marked pseudo-species..
                old_species_marks = GBT_store_marked_species(gb_main, 1); // store and unmark marked species
                error             = GBT_with_stored_species(gb_main, old_species_marks, GEN_mark_organism_or_corresponding_organism, 0); // mark organisms related with stored

                if (!error) gb_organism = GEN_first_marked_organism(gb_main);
            }
            else {
                gb_organism = GEN_first_marked_organism(gb_main);
            }

            break;
        }
        case QUERY_ALL_ITEMS: {
            gb_organism = GBT_first_species(gb_main);
            break;
        }
        default: {
            gen_assert(0);
            break;
        }
    }

    if (error) GB_export_error(error);
    return gb_organism ? EXP_get_experiment_data(gb_organism) : 0;
}

static GBDATA *EXP_get_next_experiment_data(GBDATA *gb_experiment_data, QUERY_RANGE range) {
    GBDATA *gb_organism = 0;
    switch (range) {
        case QUERY_CURRENT_ITEM: {
            break;
        }
        case QUERY_MARKED_ITEMS: {
            GBDATA *gb_last_species = GB_get_father(gb_experiment_data);
            gb_organism             = GEN_next_marked_organism(gb_last_species);

            if (!gb_organism) exp_restore_old_species_marks(GB_get_root(gb_experiment_data)); // got all -> clean up

            break;
        }
        case QUERY_ALL_ITEMS: {
            GBDATA *gb_last_species = GB_get_father(gb_experiment_data);
            gb_organism             = GBT_next_species(gb_last_species);
            break;
        }
        default: {
            gen_assert(0);
            break;
        }
    }

    return gb_organism ? EXP_get_experiment_data(gb_organism) : 0;
}

static GBDATA *EXP_get_current_experiment(GBDATA *gb_main, AW_root *aw_root) {
    GBDATA *gb_organism    = GEN_get_current_organism(gb_main, aw_root);
    GBDATA *gb_experiment = 0;

    if (gb_organism) {
        char *experiment_name = aw_root->awar(AWAR_EXPERIMENT_NAME)->read_string();
        gb_experiment         = EXP_find_experiment(gb_organism, experiment_name);
        free(experiment_name);
    }

    return gb_experiment;
}

static GBDATA *first_experiment_in_range(GBDATA *gb_experiment_data, QUERY_RANGE range) {
    GBDATA *gb_first = NULL;
    switch (range) {
        case QUERY_ALL_ITEMS:    gb_first = EXP_first_experiment_rel_exp_data(gb_experiment_data); break;
        case QUERY_MARKED_ITEMS: gb_first = GB_first_marked(gb_experiment_data, "experiment"); break;
        case QUERY_CURRENT_ITEM: gb_first = EXP_get_current_experiment(GB_get_root(gb_experiment_data), AW_root::SINGLETON); break;
    }
    return gb_first;
}
static GBDATA *next_experiment_in_range(GBDATA *gb_prev, QUERY_RANGE range) {
    GBDATA *gb_next = NULL;
    switch (range) {
        case QUERY_ALL_ITEMS:    gb_next = EXP_next_experiment(gb_prev); break;
        case QUERY_MARKED_ITEMS: gb_next = GB_next_marked(gb_prev, "experiment"); break;
        case QUERY_CURRENT_ITEM: gb_next = NULL; break;
    }
    return gb_next;
}

#if defined(WARN_TODO)
#warning move EXP_item_selector to SL/ITEMS
#endif

static struct MutableItemSelector EXP_item_selector = {
    QUERY_ITEM_EXPERIMENTS,
    EXP_select_experiment,
    EXP_get_experiment_id,
    EXP_find_experiment_by_id,
    (AW_CB)experiment_field_selection_list_update_cb,
    -1, // unknown
    CHANGE_KEY_PATH_EXPERIMENTS,
    "experiment",
    "experiments",
    "name",
    EXP_get_first_experiment_data,
    EXP_get_next_experiment_data,
    first_experiment_in_range,
    next_experiment_in_range,
    EXP_get_current_experiment,
    &ORGANISM_get_selector(), GB_get_grandfather,
};

ItemSelector& EXP_get_selector() { return EXP_item_selector; }

static QUERY::DbQuery *GLOBAL_experiment_query = 0;

static AW_window *EXP_create_experiment_window(AW_root *aw_root, AW_CL cl_gb_main);

#if defined(WARN_TODO)
#warning move EXP_create_experiment_query_window to SL/DB_UI
#endif

AW_window *EXP_create_experiment_query_window(AW_root *aw_root, AW_CL cl_gb_main) {
    static AW_window_simple_menu *aws = 0;
    if (!aws) {
        GBDATA *gb_main = (GBDATA*)cl_gb_main;

        aws = new AW_window_simple_menu;
        aws->init(aw_root, "EXPERIMENT_QUERY", "Experiment SEARCH and QUERY");
        aws->create_menu("More functions", "f");
        aws->load_xfig("ad_query.fig");

        QUERY::query_spec awtqs(EXP_get_selector());

        awtqs.gb_main             = gb_main;
        awtqs.species_name        = AWAR_SPECIES_NAME;
        awtqs.tree_name           = AWAR_TREE;
        awtqs.select_bit          = 1;
        awtqs.use_menu            = 1;
        awtqs.ere_pos_fig         = "ere3";
        awtqs.where_pos_fig       = "where3";
        awtqs.by_pos_fig          = "by3";
        awtqs.qbox_pos_fig        = "qbox";
        awtqs.rescan_pos_fig      = 0;
        awtqs.key_pos_fig         = 0;
        awtqs.query_pos_fig       = "content";
        awtqs.result_pos_fig      = "result";
        awtqs.count_pos_fig       = "count";
        awtqs.do_query_pos_fig    = "doquery";
        awtqs.config_pos_fig      = "doconfig";
        awtqs.do_mark_pos_fig     = "domark";
        awtqs.do_unmark_pos_fig   = "dounmark";
        awtqs.do_delete_pos_fig   = "dodelete";
        awtqs.do_set_pos_fig      = "doset";
        awtqs.do_refresh_pos_fig  = "dorefresh";
        awtqs.open_parser_pos_fig = "openparser";
        awtqs.create_view_window  = EXP_create_experiment_window;

        QUERY::DbQuery *query   = create_query_box(aws, &awtqs, "exp");
        GLOBAL_experiment_query = query;

        aws->create_menu("More search",     "s");
        aws->insert_menu_topic("exp_search_equal_fields_within_db", "Search For Equal Fields and Mark Duplicates",              "E", "search_duplicates.hlp", AWM_ALL, (AW_CB)QUERY::search_duplicated_field_content, (AW_CL)query, 0);
        aws->insert_menu_topic("exp_search_equal_words_within_db", "Search For Equal Words Between Fields and Mark Duplicates", "W", "search_duplicates.hlp", AWM_ALL, (AW_CB)QUERY::search_duplicated_field_content, (AW_CL)query, 1);

        aws->button_length(7);

        aws->at("close");
        aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");
        aws->at("help");
        aws->callback(AW_POPUP_HELP, (AW_CL)"experiment_search.hlp");
        aws->create_button("HELP", "HELP", "H");
    }
    return aws;
}

static void experiment_delete_cb(AW_window *aww, AW_CL cl_gb_main) {
    if (aw_ask_sure("Are you sure to delete the experiment")) {
        GBDATA         *gb_main       = (GBDATA*)cl_gb_main;
        GB_transaction  ta(gb_main);
        GB_ERROR        error         = 0;
        GBDATA         *gb_experiment = EXP_get_current_experiment(gb_main, aww->get_root());

        error = gb_experiment ? GB_delete(gb_experiment) : "Please select a experiment first";
        if (error) {
            error = ta.close(error);
            aw_message(error);
        }
    }
}

static void experiment_create_cb(AW_window *aww, AW_CL cl_gb_main) {
    AW_root  *aw_root = aww->get_root();
    char     *dest    = aw_root->awar(AWAR_EXPERIMENT_DEST)->read_string();
    GBDATA   *gb_main = (GBDATA*)cl_gb_main;
    GB_ERROR  error   = GB_begin_transaction(gb_main);

    if (!error) {
        GBDATA *gb_experiment_data = EXP_get_current_experiment_data(gb_main, aw_root);

        if (!gb_experiment_data) {
            error = "Please select an organism";
        }
        else {
            GBDATA *gb_dest = EXP_find_experiment_rel_exp_data(gb_experiment_data, dest);

            if (gb_dest) {
                error  = GBS_global_string("Experiment '%s' already exists", dest);
            }
            else {
                gb_dest             = EXP_find_or_create_experiment_rel_exp_data(gb_experiment_data, dest);
                if (!gb_dest) error = GB_await_error();
                else aww->get_root()->awar(AWAR_EXPERIMENT_NAME)->write_string(dest);
            }
        }
    }

    GB_end_transaction_show_error(gb_main, error, aw_message);
    free(dest);
}

static void experiment_rename_cb(AW_window *aww, AW_CL cl_gb_main) {
    AW_root *aw_root = aww->get_root();
    char    *source  = aw_root->awar(AWAR_EXPERIMENT_NAME)->read_string();
    char    *dest    = aw_root->awar(AWAR_EXPERIMENT_DEST)->read_string();

    if (strcmp(source, dest) != 0) {
        GBDATA   *gb_main = (GBDATA*)cl_gb_main;
        GB_ERROR  error   = GB_begin_transaction(gb_main);

        if (!error) {
            GBDATA *gb_experiment_data = EXP_get_current_experiment_data(gb_main, aww->get_root());

            if (!gb_experiment_data) error = "Please select a species first";
            else {
                GBDATA *gb_source = EXP_find_experiment_rel_exp_data(gb_experiment_data, source);
                GBDATA *gb_dest   = EXP_find_experiment_rel_exp_data(gb_experiment_data, dest);

                if (!gb_source) error   = "Please select an experiment";
                else if (gb_dest) error = GB_export_errorf("Experiment '%s' already exists", dest);
                else {
                    GBDATA *gb_name     = GB_search(gb_source, "name", GB_STRING);
                    if (!gb_name) error = GB_await_error();
                    else {
                        error = GB_write_string(gb_name, dest);
                        if (!error) aww->get_root()->awar(AWAR_EXPERIMENT_NAME)->write_string(dest);
                    }
                }
            }
        }
        error = GB_end_transaction(gb_main, error);
        aww->hide_or_notify(error);
    }

    free(source);
    free(dest);
}

static void experiment_copy_cb(AW_window *aww, AW_CL cl_gb_main) {
    char     *source  = aww->get_root()->awar(AWAR_EXPERIMENT_NAME)->read_string();
    char     *dest    = aww->get_root()->awar(AWAR_EXPERIMENT_DEST)->read_string();
    GBDATA   *gb_main = (GBDATA*)cl_gb_main;
    GB_ERROR  error   = GB_begin_transaction(gb_main);

    if (!error) {
        GBDATA *gb_experiment_data = EXP_get_current_experiment_data(gb_main, aww->get_root());

        if (!gb_experiment_data) {
            error = "Please select a species first.";
        }
        else {
            GBDATA *gb_source = EXP_find_experiment_rel_exp_data(gb_experiment_data, source);
            GBDATA *gb_dest   = EXP_find_experiment_rel_exp_data(gb_experiment_data, dest);

            if (!gb_source) error   = "Please select a experiment";
            else if (gb_dest) error = GB_export_errorf("Experiment '%s' already exists", dest);
            else {
                gb_dest             = GB_create_container(gb_experiment_data, "experiment");
                if (!gb_dest) error = GB_await_error();
                else error          = GB_copy(gb_dest, gb_source);

                if (!error) {
                    error = GBT_write_string(gb_dest, "name", dest);
                    if (!error) aww->get_root()->awar(AWAR_EXPERIMENT_NAME)->write_string(dest);
                }
            }
        }
    }

    error = GB_end_transaction(gb_main, error);
    aww->hide_or_notify(error);

    free(source);
    free(dest);
}

static AW_window *create_experiment_rename_window(AW_root *root, AW_CL cl_gb_main) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "RENAME_EXPERIMENT", "EXPERIMENT RENAME");
    aws->load_xfig("ad_al_si.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the new name\nof the experiment");

    aws->at("input");
    aws->create_input_field(AWAR_EXPERIMENT_DEST, 15);
    aws->at("ok");
    aws->callback(experiment_rename_cb, cl_gb_main);
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}

static AW_window *create_experiment_copy_window(AW_root *root, AW_CL cl_gb_main) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "COPY_EXPERIMENT", "EXPERIMENT COPY");
    aws->load_xfig("ad_al_si.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the name\nof the new experiment");

    aws->at("input");
    aws->create_input_field(AWAR_EXPERIMENT_DEST, 15);

    aws->at("ok");
    aws->callback(experiment_copy_cb, cl_gb_main);
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}

static AW_window *create_experiment_create_window(AW_root *root, AW_CL cl_gb_main) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "CREATE_EXPERIMENT", "EXPERIMENT CREATE");
    aws->load_xfig("ad_al_si.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label"); aws->create_autosize_button(0, "Please enter the name\nof the new experiment");
    aws->at("input"); aws->create_input_field(AWAR_EXPERIMENT_DEST, 15);

    aws->at("ok");
    aws->callback(experiment_create_cb, cl_gb_main);
    aws->create_button("GO", "GO", "G");

    return aws;
}

static void EXP_map_experiment(AW_root *aw_root, AW_CL cl_scanner, AW_CL cl_gb_main) {
    GBDATA         *gb_main       = (GBDATA*)cl_gb_main;
    GB_transaction  ta(gb_main);
    GBDATA         *gb_experiment = EXP_get_current_experiment(gb_main, aw_root);

    if (gb_experiment) map_db_scanner((DbScanner*)cl_scanner, gb_experiment, CHANGE_KEY_PATH_EXPERIMENTS);
}

static void EXP_create_field_items(AW_window *aws, GBDATA *gb_main) {
    static BoundItemSel *bis = new BoundItemSel(gb_main, EXP_get_selector());
    exp_assert(bis->gb_main == gb_main);

    aws->insert_menu_topic("exp_reorder_fields", "Reorder fields ...",    "R", "spaf_reorder.hlp", AD_F_ALL, AW_POPUP, (AW_CL)DBUI::create_fields_reorder_window, (AW_CL)bis);
    aws->insert_menu_topic("exp_delete_field",   "Delete/Hide Field ...", "D", "spaf_delete.hlp",  AD_F_ALL, AW_POPUP, (AW_CL)DBUI::create_field_delete_window, (AW_CL)bis);
    aws->insert_menu_topic("exp_create_field",   "Create fields ...",     "C", "spaf_create.hlp",  AD_F_ALL, AW_POPUP, (AW_CL)DBUI::create_field_create_window, (AW_CL)bis);
    aws->insert_separator();
    aws->insert_menu_topic("exp_unhide_fields", "Show all hidden fields", "S", "scandb.hlp", AD_F_ALL, (AW_CB)experiment_field_selection_list_unhide_all_cb, (AW_CL)gb_main, FIELD_FILTER_NDS);
    aws->insert_separator();
    aws->insert_menu_topic("exp_scan_unknown_fields", "Scan unknown fields",   "u", "scandb.hlp", AD_F_ALL, (AW_CB)experiment_field_selection_list_scan_unknown_cb,  (AW_CL)gb_main, FIELD_FILTER_NDS);
    aws->insert_menu_topic("exp_del_unused_fields",   "Remove unused fields",  "e", "scandb.hlp", AD_F_ALL, (AW_CB)experiment_field_selection_list_delete_unused_cb, (AW_CL)gb_main, FIELD_FILTER_NDS);
    aws->insert_menu_topic("exp_refresh_fields",      "Refresh fields (both)", "f", "scandb.hlp", AD_F_ALL, (AW_CB)experiment_field_selection_list_update_cb,        (AW_CL)gb_main, FIELD_FILTER_NDS);
}

#if defined(WARN_TODO)
#warning move EXP_create_experiment_window to SL/DB_UI
#endif


static AW_window *EXP_create_experiment_window(AW_root *aw_root, AW_CL cl_gb_main) {
    static AW_window_simple_menu *aws = 0;

    if (!aws) {
        GBDATA *gb_main = (GBDATA*)cl_gb_main;

        aws = new AW_window_simple_menu;
        aws->init(aw_root, "EXPERIMENT_INFORMATION", "EXPERIMENT INFORMATION");
        aws->load_xfig("ad_spec.fig");

        aws->button_length(8);

        aws->at("close");
        aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("search");
        aws->callback(AW_POPUP, (AW_CL)EXP_create_experiment_query_window, (AW_CL)gb_main);
        aws->create_button("SEARCH", "SEARCH", "S");

        aws->at("help");
        aws->callback(AW_POPUP_HELP, (AW_CL)"experiment_info.hlp");
        aws->create_button("HELP", "HELP", "H");


        DbScanner *scanner = create_db_scanner(gb_main, aws, "box", 0, "field", "enable", DB_VIEWER, 0, "mark", FIELD_FILTER_NDS, EXP_get_selector());

        aws->create_menu("EXPERIMENT", "E", AD_F_ALL);
        aws->insert_menu_topic("experiment_delete", "Delete",     "D", "spa_delete.hlp", AD_F_ALL, (AW_CB)experiment_delete_cb, (AW_CL)gb_main,                         0);
        aws->insert_menu_topic("experiment_rename", "Rename ...", "R", "spa_rename.hlp", AD_F_ALL, AW_POPUP,                    (AW_CL)create_experiment_rename_window, (AW_CL)gb_main);
        aws->insert_menu_topic("experiment_copy",   "Copy ...",   "y", "spa_copy.hlp",   AD_F_ALL, AW_POPUP,                    (AW_CL)create_experiment_copy_window,   (AW_CL)gb_main);
        aws->insert_menu_topic("experiment_create", "Create ...", "C", "spa_create.hlp", AD_F_ALL, AW_POPUP,                    (AW_CL)create_experiment_create_window, (AW_CL)gb_main);
        aws->insert_separator();

        aws->create_menu("FIELDS", "F", AD_F_ALL);
        EXP_create_field_items(aws, gb_main);

        {
            Awar_Callback_Info    *cb_info     = new Awar_Callback_Info(aws->get_root(), AWAR_EXPERIMENT_NAME, EXP_map_experiment, (AW_CL)scanner, (AW_CL)gb_main); // do not delete!
            AW_detach_information *detach_info = new AW_detach_information(cb_info); // do not delete!
            cb_info->add_callback();

            aws->at("detach");
            aws->callback(DBUI::detach_info_window, (AW_CL)&aws, (AW_CL)cb_info);
            aws->create_button("DETACH", "DETACH", "D");

            detach_info->set_detach_button(aws->get_last_widget());
        }

        aws->show();
        EXP_map_experiment(aws->get_root(), (AW_CL)scanner, (AW_CL)gb_main);
    }
    return aws;
}

void EXP_popup_experiment_window(AW_window *aww, AW_CL cl_gb_main, AW_CL) {
    AW_window *aws = EXP_create_experiment_window(aww->get_root(), cl_gb_main);
    aws->activate();
}

