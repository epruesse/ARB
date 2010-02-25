// ================================================================ //
//                                                                  //
//   File      : ST_window.cxx                                      //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "st_window.hxx"
#include "st_ml.hxx"
#include "st_quality.hxx"

#include <aw_awars.hxx>
#include <awt.hxx>
#include <awt_item_sel_list.hxx>
#include <ColumnStat.hxx>
#include <awt_filter.hxx>

static void st_ok_cb(AW_window *aww, ST_ML *st_ml) {
    AW_root        *root           = aww->get_root();
    char           *alignment_name = root->awar_string(AWAR_DEFAULT_ALIGNMENT, "-none-", st_ml->get_gb_main())->read_string();
    char           *tree_name      = root->awar_string(AWAR_TREE, "tree_stat", st_ml->get_gb_main())->read_string();
    int             marked_only    = root->awar_int(ST_ML_AWAR_CQ_MARKED_ONLY)->read_int();

    GB_ERROR error = GB_push_transaction(st_ml->get_gb_main());
    if (!error) {
        error = st_ml->init_st_ml(tree_name, alignment_name, NULL, marked_only, st_ml->get_column_statistic(), true);
        if (!error) st_ml->do_refresh();
    }

    error = GB_end_transaction(st_ml->get_gb_main(), error);
    aww->hide_or_notify(error);

    free(tree_name);
    free(alignment_name);
}

AW_window *STAT_create_main_window(AW_root *root, ST_ML *st_ml, AW_CB0 refresh_func, AW_window *refreshed_win) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "COLUMN_STATISTIC", "COLUMN STATISTIC");

    aws->load_xfig("stat_main.fig");

    aws->callback(AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(AW_POPUP_HELP, (AW_CL) "st_ml.hlp");
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    root->awar_string(ST_ML_AWAR_COLUMN_STAT, "");
    root->awar_int(ST_ML_AWAR_CQ_MARKED_ONLY, 1);

    AW_awar *awar_default_alignment = root->awar_string(AWAR_DEFAULT_ALIGNMENT, "-none-", st_ml->get_gb_main());
    root->awar_string(AWAR_TREE, "tree_main", st_ml->get_gb_main());

    root->awar_string(ST_ML_AWAR_ALIGNMENT)->map(AWAR_DEFAULT_ALIGNMENT);

    st_ml->create_column_statistic(root, ST_ML_AWAR_COLUMN_STAT, awar_default_alignment);
    st_ml->set_refresh_callback(refresh_func, refreshed_win);

    aws->at("GO");
    aws->callback((AW_CB1) st_ok_cb, (AW_CL) st_ml);
    aws->create_button("GO", "GO", "G");

    aws->at("awt_csp");
    aws->callback(AW_POPUP, (AW_CL)COLSTAT_create_selection_window, (AW_CL)st_ml->get_column_statistic());
    aws->button_length(20);
    aws->create_button("SELECT_CSP", ST_ML_AWAR_COLUMN_STAT);

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

ST_ML_Color *STAT_get_color_string(ST_ML *st_ml, char *species_name, AP_tree *node, int start_ali_pos, int end_ali_pos) {
    return st_ml->get_color_string(species_name, node, start_ali_pos, end_ali_pos);
}

bool STAT_update_ml_likelihood(ST_ML *st_ml, char *result[4], int& latest_update, const char *species_name, AP_tree *node) {
    /*! @see ST_ML::update_ml_likelihood() */
    return st_ml->update_ml_likelihood(result, latest_update, species_name, node);
}

AP_tree *STAT_find_node_by_name(ST_ML *st_ml, const char *species_name) {
    return st_ml->find_node_by_name(species_name);
}

static void st_check_cb(AW_window *aww, GBDATA *gb_main, ColumnStat *colstat) {
    GB_transaction ta(gb_main);

    AW_root *r = aww->get_root();

    char *alignment_name = r->awar(ST_ML_AWAR_ALIGNMENT)->read_string();
    int   bucket_size    = r->awar(ST_ML_AWAR_CQ_BUCKET_SIZE)->read_int();
    char *tree_name      = r->awar(AWAR_TREE)->read_string();
    char *dest_field     = r->awar(ST_ML_AWAR_CQ_DEST_FIELD)->read_string();
    int   marked_only    = r->awar(ST_ML_AWAR_CQ_MARKED_ONLY)->read_int();

    st_report_enum report = (st_report_enum) r->awar(ST_ML_AWAR_CQ_REPORT)->read_int();
    GB_ERROR       error  = st_ml_check_sequence_quality(gb_main, tree_name, alignment_name,
                                                         colstat, bucket_size, marked_only,
                                                         report, dest_field);
    free(dest_field);
    free(alignment_name);
    free(tree_name);

    error = ta.close(error);
    if (error) aw_message(error);
}

AW_window *STAT_create_quality_check_window(AW_root *root, GBDATA *gb_main) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;
        aws->init(root, "SEQUENCE_QUALITY_CHECK", "Check quality of marked sequences");
        aws->load_xfig("check_quality.fig");

        aws->callback(AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(AW_POPUP_HELP, (AW_CL) "check_quality.hlp");
        aws->at("help");
        aws->create_button("HELP", "HELP", "H");

        AW_awar *awar_default_alignment = root->awar_string(AWAR_DEFAULT_ALIGNMENT, "-none-", gb_main);
        root->awar_int(ST_ML_AWAR_CQ_BUCKET_SIZE, 300);
        root->awar_int(ST_ML_AWAR_CQ_MARKED_ONLY, 0);
        root->awar_string(AWAR_TREE, "tree_main", gb_main);
        root->awar_string(ST_ML_AWAR_CQ_DEST_FIELD, "tmp");
        root->awar_int(ST_ML_AWAR_CQ_REPORT, 0);

        ColumnStat *colstat = new ColumnStat(gb_main, root, ST_ML_AWAR_COLUMN_STAT, awar_default_alignment); // @@@ not freed

        aws->at("which");
        {
            aws->create_option_menu(ST_ML_AWAR_CQ_MARKED_ONLY);
            aws->insert_option("All in tree", "t", 0);
            aws->insert_option("Only marked and in tree", "m", 1);
            aws->update_option_menu();
        }

        aws->at("report");
        {
            aws->create_option_menu(ST_ML_AWAR_CQ_REPORT);
            aws->insert_option("No report", "N", 0);
            aws->insert_option("R. to temporary sequence", "t", 1);
            aws->insert_option("R. to sequence", "s", 2);
            aws->update_option_menu();
        }

        aws->at("awt_csp");
        aws->callback(AW_POPUP, (AW_CL)COLSTAT_create_selection_window, (AW_CL)colstat);
        aws->create_button("SELECT_CSP", ST_ML_AWAR_COLUMN_STAT);

        aws->at("sb");
        aws->create_input_field(ST_ML_AWAR_CQ_BUCKET_SIZE);

        awt_create_selection_list_on_itemfields(gb_main, aws,
                                                ST_ML_AWAR_CQ_DEST_FIELD,
                                                1 << GB_STRING,
                                                "dest",
                                                0,
                                                &AWT_species_selector,
                                                20, 10);

        aws->at("GO");
        aws->callback((AW_CB)st_check_cb, (AW_CL)gb_main, (AW_CL)colstat);
        aws->create_button("GO", "GO", "G");
    }
    return aws;
}
