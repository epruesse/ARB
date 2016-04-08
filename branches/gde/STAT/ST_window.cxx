// ================================================================ //
//                                                                  //
//   File      : ST_window.cxx                                      //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "st_ml.hxx"
#include "st_quality.hxx"

#include <gui_aliview.hxx>
#include <ColumnStat.hxx>

#include <aw_awars.hxx>
#include <aw_root.hxx>
#include <aw_msg.hxx>
#include <item_sel_list.h>
#include <awt_filter.hxx>
#include <awt_config_manager.hxx>
#include <arbdbt.h>
#include <arb_progress.h>

#define ST_ML_AWAR "tmp/st_ml/"

#define ST_ML_AWAR_COLSTAT_PREFIX ST_ML_AWAR "colstat/"
#define ST_ML_AWAR_COLSTAT_NAME   ST_ML_AWAR_COLSTAT_PREFIX "name" 

#define ST_ML_AWAR_FILTER_PREFIX    ST_ML_AWAR "filter/"
#define ST_ML_AWAR_FILTER_ALIGNMENT ST_ML_AWAR_FILTER_PREFIX "alignment"
#define ST_ML_AWAR_FILTER_NAME      ST_ML_AWAR_FILTER_PREFIX "name"
#define ST_ML_AWAR_FILTER_FILTER    ST_ML_AWAR_FILTER_PREFIX "filter"
#define ST_ML_AWAR_FILTER_SIMPLIFY  ST_ML_AWAR_FILTER_PREFIX "simplify"

#define ST_ML_AWAR_CQ_BUCKET_SIZE  ST_ML_AWAR "bucket_size"
#define ST_ML_AWAR_CQ_MARKED_ONLY  ST_ML_AWAR "marked_only"
#define ST_ML_AWAR_CQ_DEST_FIELD   ST_ML_AWAR "dest_field"
#define ST_ML_AWAR_CQ_REPORT       ST_ML_AWAR "report"
#define ST_ML_AWAR_CQ_KEEP_REPORTS ST_ML_AWAR "keep_reports"

static void st_ok_cb(AW_window *aww, ST_ML *st_ml) {
    AW_root *root           = aww->get_root();
    char    *alignment_name = root->awar_string(AWAR_DEFAULT_ALIGNMENT, "-none-", st_ml->get_gb_main())->read_string();
    char    *tree_name      = root->awar_string(AWAR_TREE, "", st_ml->get_gb_main())->read_string();
    int      marked_only    = root->awar_int(ST_ML_AWAR_CQ_MARKED_ONLY)->read_int();

    GB_ERROR error = GB_push_transaction(st_ml->get_gb_main());
    if (!error) {
        error = st_ml->calc_st_ml(tree_name, alignment_name, NULL, marked_only, st_ml->get_column_statistic(), NULL);
        if (!error) st_ml->do_postcalc_callback();
    }

    error = GB_end_transaction(st_ml->get_gb_main(), error);
    aww->hide_or_notify(error);

    free(tree_name);
    free(alignment_name);
}

void STAT_set_postcalc_callback(ST_ML *st_ml, WindowCallbackSimple postcalc_cb, AW_window *cb_win) {
    st_ml->set_postcalc_callback(postcalc_cb, cb_win);
}

AW_window *STAT_create_main_window(AW_root *root, ST_ML *st_ml) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "COLUMN_STATISTIC", "COLUMN STATISTIC");

    aws->load_xfig("stat_main.fig");

    aws->callback(AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(makeHelpCallback("st_ml.hlp"));
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    root->awar_string(ST_ML_AWAR_COLSTAT_NAME, "");
    root->awar_int(ST_ML_AWAR_CQ_MARKED_ONLY, 1);

    AW_awar *awar_default_alignment = root->awar_string(AWAR_DEFAULT_ALIGNMENT, "-none-", st_ml->get_gb_main());
    root->awar_string(AWAR_TREE, "", st_ml->get_gb_main());

    st_ml->create_column_statistic(root, ST_ML_AWAR_COLSTAT_NAME, awar_default_alignment);

    aws->at("GO");
    aws->callback(makeWindowCallback(st_ok_cb, st_ml));
    aws->create_button("GO", "GO", "G");

    aws->at("awt_csp");
    aws->callback(makeCreateWindowCallback(COLSTAT_create_selection_window, st_ml->get_column_statistic()));
    aws->button_length(20);
    aws->create_button("SELECT_CSP", ST_ML_AWAR_COLSTAT_NAME);

    aws->at("marked");
    aws->create_toggle_field(ST_ML_AWAR_CQ_MARKED_ONLY, "Calculate for ..", "");
    aws->insert_toggle("All species", "A", 0);
    aws->insert_toggle("Marked species", "M", 1);
    aws->update_toggle_field();

    return aws;
}

ST_ML *STAT_create_ST_ML(GBDATA *gb_main) {
    return new ST_ML(gb_main);
}
void STAT_destroy_ST_ML(ST_ML*& st_ml) {
    delete st_ml;
    st_ml = NULL;
}

ST_ML_Color *STAT_get_color_string(ST_ML *st_ml, char *species_name, AP_tree *node, int start_ali_pos, int end_ali_pos) {
    return st_ml->get_color_string(species_name, node, start_ali_pos, end_ali_pos);
}

bool STAT_update_ml_likelihood(ST_ML *st_ml, char *result[4], int& latest_update, const char *species_name, AP_tree *node) {
    //! @see ST_ML::update_ml_likelihood()
    return st_ml->update_ml_likelihood(result, latest_update, species_name, node);
}

AP_tree *STAT_find_node_by_name(ST_ML *st_ml, const char *species_name) {
    return st_ml->find_node_by_name(species_name);
}

struct st_check_cb_data : public Noncopyable {
    GBDATA         *gb_main;
    ColumnStat     *colstat;
    WeightedFilter *filter;

    st_check_cb_data(GBDATA *gb_main_, AW_root *root, const char *columnStatAwarName, const char *filterAwarName, AW_awar *awar_default_alignment) {
        gb_main = gb_main_;
        colstat = new ColumnStat(gb_main, root, columnStatAwarName, awar_default_alignment);
        filter  = new WeightedFilter(gb_main, root, filterAwarName, NULL, awar_default_alignment);
    }
};

static GB_ERROR st_remove_entries(AW_root *awr, GBDATA *gb_main) {
    GB_ERROR error = NULL;

    char *ali_name   = awr->awar(ST_ML_AWAR_FILTER_ALIGNMENT)->read_string();
    char *dest_field = awr->awar(ST_ML_AWAR_CQ_DEST_FIELD)->read_string();

    {
        GB_transaction ta(gb_main);
        long count = GBT_get_species_count(gb_main);

        arb_progress progress("Removing old reports", count);
        for (GBDATA *gb_species = GBT_first_species(gb_main);
             gb_species && !error;
             gb_species = GBT_next_species(gb_species))
        {
            GBDATA *gb_field    = GB_entry(gb_species, dest_field);
            if (gb_field) error = GB_delete(gb_field);
            if (!error) {
                GBDATA *gb_ali = GB_entry(gb_species, ali_name);
                if (gb_ali) {
                    GBDATA *gb_quality    = GB_entry(gb_ali, "quality");
                    if (gb_quality) error = GB_delete(gb_quality);
                }
            }
            progress.inc_and_check_user_abort(error);
        }
    }

    free(dest_field);
    free(ali_name);

    return error;
}

static void st_remove_entries_cb(AW_window *aww, GBDATA *gb_main) {
    aw_message_if(st_remove_entries(aww->get_root(), gb_main));
}

static void st_check_cb(AW_window *aww, st_check_cb_data *data) {
    arb_progress   glob_progress("Chimera check");
    GB_transaction ta(data->gb_main);

    AW_root *awr = aww->get_root();

    char *ali_name    = awr->awar(ST_ML_AWAR_FILTER_ALIGNMENT)->read_string();
    int   bucket_size = awr->awar(ST_ML_AWAR_CQ_BUCKET_SIZE)->read_int();
    char *tree_name   = awr->awar(AWAR_TREE)->read_string();
    char *dest_field  = awr->awar(ST_ML_AWAR_CQ_DEST_FIELD)->read_string();
    int   marked_only = awr->awar(ST_ML_AWAR_CQ_MARKED_ONLY)->read_int();

    st_report_enum report           = (st_report_enum) awr->awar(ST_ML_AWAR_CQ_REPORT)->read_int();
    bool           keep_old_reports = awr->awar(ST_ML_AWAR_CQ_KEEP_REPORTS)->read_int();

    GB_ERROR error = NULL;
    if (!keep_old_reports) {
        error = st_remove_entries(awr, data->gb_main);
    }
    if (!error) {
        error = st_ml_check_sequence_quality(data->gb_main, tree_name, ali_name, data->colstat, data->filter, bucket_size, marked_only, report, dest_field);
    }

    free(dest_field);
    free(ali_name);
    free(tree_name);

    error = ta.close(error);
    if (error) aw_message(error);
}

static void STAT_create_awars(AW_root *root, GBDATA *gb_main) {
    root->awar_string(AWAR_DEFAULT_ALIGNMENT, "-none-",    gb_main);
    root->awar_string(AWAR_TREE,              "", gb_main);

    root->awar_string(ST_ML_AWAR_COLSTAT_NAME, "none");

    root->awar_string(ST_ML_AWAR_FILTER_ALIGNMENT, "none");
    root->awar_string(ST_ML_AWAR_FILTER_NAME,      "none");
    root->awar_string(ST_ML_AWAR_FILTER_FILTER,    "");
    root->awar_int   (ST_ML_AWAR_FILTER_SIMPLIFY,  0);

    root->awar_int   (ST_ML_AWAR_CQ_BUCKET_SIZE,  300);
    root->awar_int   (ST_ML_AWAR_CQ_MARKED_ONLY,  0);
    root->awar_string(ST_ML_AWAR_CQ_DEST_FIELD,   "tmp");
    root->awar_int   (ST_ML_AWAR_CQ_REPORT,       0);
    root->awar_int   (ST_ML_AWAR_CQ_KEEP_REPORTS, 0);

    root->awar_string(ST_ML_AWAR_FILTER_ALIGNMENT)->map(AWAR_DEFAULT_ALIGNMENT);
}

static AWT_config_mapping_def chimera_config_mapping[] = {
    { ST_ML_AWAR_CQ_MARKED_ONLY,  "marked_only" },
    { ST_ML_AWAR_COLSTAT_NAME,    "colstat" },
    { ST_ML_AWAR_FILTER_NAME,     "filter" },
    { ST_ML_AWAR_CQ_BUCKET_SIZE,  "bucketsize" },
    { ST_ML_AWAR_CQ_DEST_FIELD,   "destfield" },
    { ST_ML_AWAR_CQ_REPORT,       "report" },
    { ST_ML_AWAR_CQ_KEEP_REPORTS, "keepold" },

    { 0, 0 }
};

AW_window *STAT_create_chimera_check_window(AW_root *root, GBDATA *gb_main) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;
        aws->init(root, "CHIMERA_CHECK", "Chimera Check of marked sequences");
        aws->load_xfig("chimera_check.fig");

        STAT_create_awars(root, gb_main);

        aws->callback(AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(makeHelpCallback("chimera_check.hlp"));
        aws->at("help");
        aws->create_button("HELP", "HELP", "H");

        AW_awar *awar_default_alignment = root->awar_string(AWAR_DEFAULT_ALIGNMENT, "-none-", gb_main);
        st_check_cb_data *cb_data = new st_check_cb_data(gb_main, root, ST_ML_AWAR_COLSTAT_NAME, ST_ML_AWAR_FILTER_NAME, awar_default_alignment); // bound to cb (not freed)

        aws->at("which");
        {
            aws->create_option_menu(ST_ML_AWAR_CQ_MARKED_ONLY, true);
            aws->insert_option("All in tree", "t", 0);
            aws->insert_option("Only marked and in tree", "m", 1);
            aws->update_option_menu();
        }

        aws->at("colstat");
        aws->callback(makeCreateWindowCallback(COLSTAT_create_selection_window, cb_data->colstat));
        aws->create_button("SELECT_CSP", ST_ML_AWAR_COLSTAT_NAME);

        
        aws->at("filter");
        aws->callback(makeCreateWindowCallback(awt_create_select_filter_win, cb_data->filter->get_adfiltercbstruct()));
        aws->create_button("SELECT_FILTER", ST_ML_AWAR_FILTER_NAME);
        
        aws->at("sb");
        aws->create_input_field(ST_ML_AWAR_CQ_BUCKET_SIZE);

        create_itemfield_selection_button(aws, FieldSelDef(ST_ML_AWAR_CQ_DEST_FIELD, gb_main, SPECIES_get_selector(), 1 << GB_STRING), "dest"); // @@@ use FIELD_FILTER_STRING_WRITEABLE? check awar-usage!

        aws->at("report");
        {
            aws->create_option_menu(ST_ML_AWAR_CQ_REPORT, true);
            aws->insert_option("No", "N", 0);
            aws->insert_option("to temporary entry", "t", 1);
            aws->insert_option("to permanent entry", "p", 2);
            aws->update_option_menu();
        }

        aws->at("keep");
        aws->create_toggle(ST_ML_AWAR_CQ_KEEP_REPORTS);

        aws->at("del");
        aws->callback(makeWindowCallback(st_remove_entries_cb, gb_main));
        aws->create_button("DEL_ENTRIES", "Remove them now!", "R");

        aws->button_length(10);
        aws->at("GO");
        aws->callback(makeWindowCallback(st_check_cb, cb_data));
        aws->create_button("GO", "GO", "G");

        aws->at("config");
        AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "chimera", chimera_config_mapping);
    }
    return aws;
}
