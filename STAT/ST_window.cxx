#include <stdio.h>
#include <stdlib.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <awt.hxx>
#include <awt_tree.hxx>
#include <awt_csp.hxx>
#include <awt_changekey.hxx>
#include <awt_sel_boxes.hxx>
#include "st_window.hxx"
#include "st_ml.hxx"
#include "st_quality.hxx"

void st_ok_cb(AW_window * aww, ST_ML * st_ml)
{
    AW_root *root = aww->get_root();
    GB_transaction dummy(st_ml->gb_main);
    char *alignment_name =
        root->awar_string(AWAR_DEFAULT_ALIGNMENT, "-none-",
                          st_ml->gb_main)->read_string();
    char *tree_name = root->awar_string(AWAR_TREE, "tree_stat",
                                        st_ml->gb_main)->read_string();
    int marked_only =
        root->awar_int(ST_ML_AWAR_CQ_MARKED_ONLY)->read_int();
    GB_ERROR error =
        st_ml->init(tree_name, alignment_name, (char *) 0, marked_only,
                    (char *) 0, st_ml->awt_csp);

    if (error) {
        aw_message(error);
    } else if (st_ml->refresh_func) {
        st_ml->refresh_func(st_ml->aw_window);
    }
    delete tree_name;
    delete alignment_name;
    aww->hide();
}

AW_window *st_create_main_window(AW_root * root, ST_ML * st_ml,
                                 AW_CB0 refresh_func, AW_window * win)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "ENABLE_ONLINE_STATISTIC",
              "ACTIVATE ONLINE STATISTIC");

    aws->load_xfig("stat_main.fig");

    aws->callback(AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(AW_POPUP_HELP, (AW_CL) "st_ml.hlp");
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    root->awar_string(ST_ML_AWAR_CSP, "");
    root->awar_int(ST_ML_AWAR_CQ_MARKED_ONLY, 1);

    root->awar_string(AWAR_DEFAULT_ALIGNMENT, "-none-", st_ml->gb_main);
    root->awar_string(AWAR_TREE, "tree_main", st_ml->gb_main);

    root->awar_string(ST_ML_AWAR_ALIGNMENT)->map(AWAR_DEFAULT_ALIGNMENT);
    st_ml->awt_csp = new AWT_csp(st_ml->gb_main, root, ST_ML_AWAR_CSP);
    st_ml->refresh_func = refresh_func;
    st_ml->aw_window = win;

    aws->at("GO");
    aws->callback((AW_CB1) st_ok_cb, (AW_CL) st_ml);
    aws->create_button("GO", "GO", "G");

    aws->at("awt_csp");
    aws->callback(AW_POPUP, (AW_CL) create_csp_window,
                  (AW_CL) st_ml->awt_csp);
    aws->button_length(20);
    aws->create_button("SELECT_CSP", ST_ML_AWAR_CSP);

    aws->at("marked");
    aws->create_toggle_field(ST_ML_AWAR_CQ_MARKED_ONLY, "Calculate for ..",
                             "");
    aws->insert_toggle("All species", "A", 0);
    aws->insert_toggle("Marked species", "M", 1);
    aws->update_toggle_field();

    return aws;
}

ST_ML *new_ST_ML(GBDATA * gb_main)
{
    return new ST_ML(gb_main);
}

ST_ML_Color *st_ml_get_color_string(ST_ML * st_ml, char *species_name,
                                    AP_tree * node, int start_ali_pos,
                                    int end_ali_pos)
{
    return st_ml->get_color_string(species_name, node, start_ali_pos,
                                   end_ali_pos);
}

int st_ml_update_ml_likelihood(ST_ML * st_ml, char *result[4],
                               int *latest_update, char *species_name,
                               AP_tree * node)
{
    return st_ml->update_ml_likelihood(result, latest_update, species_name,
                                       node);
}

AP_tree *st_ml_convert_species_name_to_node(ST_ML * st_ml,
                                            const char *species_name)
{
    AP_tree *node;
    if (!st_ml->hash_2_ap_tree)
        return 0;
    node = (AP_tree *) GBS_read_hash(st_ml->hash_2_ap_tree, species_name);
    return node;
}

int st_is_inited(ST_ML * st_ml)
{
    return st_ml->is_inited;
}

void st_check_cb(AW_window * aww, GBDATA * gb_main, AWT_csp * awt_csp)
{
    GB_begin_transaction(gb_main);
    AW_root *r = aww->get_root();
    char *alignment_name = r->awar(ST_ML_AWAR_ALIGNMENT)->read_string();
    int bucket_size = r->awar(ST_ML_AWAR_CQ_BUCKET_SIZE)->read_int();
    char *tree_name = r->awar(AWAR_TREE)->read_string();
    char *dest_field = r->awar(ST_ML_AWAR_CQ_DEST_FIELD)->read_string();
    int marked_only = r->awar(ST_ML_AWAR_CQ_MARKED_ONLY)->read_int();
    char *filter_string =
        r->awar(ST_ML_AWAR_CQ_FILTER_FILTER)->read_string();
    st_report_enum report =
        (st_report_enum) r->awar(ST_ML_AWAR_CQ_REPORT)->read_int();
    GB_ERROR error =
        st_ml_check_sequence_quality(gb_main, tree_name, alignment_name,
                                     awt_csp, bucket_size,
                                     marked_only, report,
                                     filter_string, dest_field);
    free(filter_string);
    free(dest_field);
    free(alignment_name);
    free(tree_name);
    if (error) {
        aw_message(error);
        GB_abort_transaction(gb_main);
    } else {
        GB_commit_transaction(gb_main);
    }
}

AW_window *st_create_quality_check_window(AW_root * root, GBDATA * gb_main)
{
    static AW_window_simple *aws = 0;
    if (aws)
        return aws;
    static AWT_csp *awt_csp;
    aws = new AW_window_simple;
    aws->init(root, "SEQUENCE_QUALITY_CHECK",
              "CHECK QUALITY OF MARKED SEQUENCES");

    aws->load_xfig("check_quality.fig");
    //    aws->load_xfig("stat_main.fig");

    aws->callback(AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(AW_POPUP_HELP, (AW_CL) "check_quality.hlp");
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    root->awar_string(ST_ML_AWAR_CSP, "");
    root->awar_string(AWAR_DEFAULT_ALIGNMENT, "-none-", gb_main);
    root->awar_int(ST_ML_AWAR_CQ_BUCKET_SIZE, 300);
    root->awar_int(ST_ML_AWAR_CQ_MARKED_ONLY, 0);
    root->awar_string(AWAR_TREE, "tree_main", gb_main);
    root->awar_string(ST_ML_AWAR_CQ_DEST_FIELD, "tmp");
    root->awar_int(ST_ML_AWAR_CQ_REPORT, 0);

    root->awar_string(ST_ML_AWAR_CQ_FILTER_NAME, "ECOLI");
    root->awar_string(ST_ML_AWAR_CQ_FILTER_ALIGNMENT);
    root->awar_string(ST_ML_AWAR_CQ_FILTER_FILTER);

    root->awar_string(ST_ML_AWAR_ALIGNMENT)->map(AWAR_DEFAULT_ALIGNMENT);
    root->awar_string(ST_ML_AWAR_CQ_FILTER_ALIGNMENT)->
        map(AWAR_DEFAULT_ALIGNMENT);

    awt_csp = new AWT_csp(gb_main, root, ST_ML_AWAR_CSP);
    //AW_CL filter_cl =
    awt_create_select_filter(root, gb_main, ST_ML_AWAR_CQ_FILTER_NAME);

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
    aws->callback(AW_POPUP, (AW_CL) create_csp_window, (AW_CL) awt_csp);
    aws->create_button("SELECT_CSP", ST_ML_AWAR_CSP);

    aws->at("sb");
    aws->create_input_field(ST_ML_AWAR_CQ_BUCKET_SIZE);


    awt_create_selection_list_on_scandb(gb_main, aws,
                                        ST_ML_AWAR_CQ_DEST_FIELD,
                                        1 << GB_STRING, "dest", 0,
                                        &AWT_species_selector, 20, 10);
    aws->at("GO");
    aws->callback((AW_CB) st_check_cb, (AW_CL) gb_main, (AW_CL) awt_csp);
    aws->create_button("GO", "GO", "G");
    return aws;
}
