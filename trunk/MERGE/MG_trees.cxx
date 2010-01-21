#include <stdio.h>
#include <stdlib.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <awt.hxx>
#include <awt_sel_boxes.hxx>
#include "merge.hxx"

#define AWAR_TREE_NAME1 "tmp/merge1/tree_name"
#define AWAR_TREE_DEST1 "tmp/merge1/tree_dest"
#define AWAR_TREE_NAME2 "tmp/merge2/tree_name"
#define AWAR_TREE_DEST2 "tmp/merge2/tree_dest"

void MG_create_trees_awar(AW_root *aw_root, AW_default aw_def)
{
    aw_root->awar_string(AWAR_TREE_NAME1, "",   aw_def);
    aw_root->awar_string(AWAR_TREE_DEST1, "",   aw_def);
    aw_root->awar_string(AWAR_TREE_NAME2, "",   aw_def);
    aw_root->awar_string(AWAR_TREE_DEST2, "",   aw_def);
}

void MG_tree_rename_cb(AW_window *aww, GBDATA *gbd, int tree_nr) {
    const char *tsource = tree_nr == 1 ? AWAR_TREE_NAME1 : AWAR_TREE_NAME2;
    const char *tdest   = tree_nr == 1 ? AWAR_TREE_DEST1 : AWAR_TREE_DEST2;
    char       *source  = aww->get_root()->awar(tsource)->read_string();
    char       *dest    = aww->get_root()->awar(tdest)->read_string();

    GB_ERROR error = GBT_check_tree_name(dest);
    if (!error) {
        error = GB_begin_transaction(gbd);

        if (!error) {
            GBDATA *gb_tree_data     = GB_search(gbd, "tree_data", GB_CREATE_CONTAINER);
            if (!gb_tree_data) error = GB_await_error();
            else {
                GBDATA *gb_tree_name = GB_entry(gb_tree_data, source);
                GBDATA *gb_dest_tree = GB_entry(gb_tree_data, dest);

                if (gb_dest_tree) {
                    error = GBS_global_string("Tree '%s' already exists", dest);
                }
                else if (gb_tree_name) {
                    GBDATA *gb_new_tree = GB_create_container(gb_tree_data, dest);
                    error               = GB_copy(gb_new_tree, gb_tree_name);
                    if (!error) error   = GB_delete(gb_tree_name);
                }
                else {
                    error = "Please select a tree first";
                }
            }
        }
        error = GB_end_transaction(gbd, error);
    }
    
    aww->hide_or_notify(error);

    free(source);
    free(dest);
}

AW_window *MG_create_tree_rename_window1(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "MERGE_RENAME_TREE_1", "TREE RENAME 1");
    aws->load_xfig("ad_al_si.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the new name\nof the tree");

    aws->at("input");
    aws->create_input_field(AWAR_TREE_DEST1, 15);

    aws->at("ok");
    aws->callback((AW_CB)MG_tree_rename_cb, (AW_CL)GLOBAL_gb_merge, 1);
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}

AW_window *MG_create_tree_rename_window2(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "MERGE_RENAME_TREE_2", "TREE RENAME 2");
    aws->load_xfig("ad_al_si.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the new name\nof the tree");

    aws->at("input");
    aws->create_input_field(AWAR_TREE_DEST2, 15);

    aws->at("ok");
    aws->callback((AW_CB)MG_tree_rename_cb, (AW_CL)GLOBAL_gb_dest, 2);
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}
void MG_tr_delete_cb(AW_window *aww, GBDATA *gbd, long tree_nr) {
    const char *tsource = tree_nr == 1 ? AWAR_TREE_NAME1 : AWAR_TREE_NAME2;
    char       *source  = aww->get_root()->awar(tsource)->read_string();
    GB_ERROR    error   = GB_begin_transaction(gbd);

    if (!error) {
        GBDATA *gb_tree = GBT_get_tree(gbd, source);

        if (gb_tree) error = GB_delete(gb_tree);
        else {
            if (GB_have_error()) error = GBS_global_string("Could not find tree '%s' (Reason: %s)", source, GB_await_error());
            else error                 = "Please select a tree";
        }
    }
    GB_end_transaction_show_error(gbd, error, aw_message);
    free(source);
}
void MG_transfer_tree(AW_window *aww) {
    AW_root *awr    = aww->get_root();
    char    *source = awr->awar(AWAR_TREE_NAME1)->read_string();
    char    *dest   = awr->awar(AWAR_TREE_NAME1)->read_string();

    GB_ERROR error    = GB_begin_transaction(GLOBAL_gb_dest);
    if (!error) error = GB_begin_transaction(GLOBAL_gb_merge);

    if (!error) {
        GBDATA *gb_tree_data1 = GB_search(GLOBAL_gb_merge, "tree_data", GB_CREATE_CONTAINER);
        GBDATA *gb_tree_data2 = GB_search(GLOBAL_gb_dest, "tree_data", GB_CREATE_CONTAINER);

        if (!gb_tree_data1 || !gb_tree_data2) error = GB_await_error();
        else {
            GBDATA *gb_source     = GB_entry(gb_tree_data1, source);
            GBDATA *gb_dest_tree  = GB_entry(gb_tree_data2, dest);

            if (!gb_source) error = "Please select the tree you want to transfer";
            else if (gb_dest_tree) error = GBS_global_string("Tree '%s' already exists, delete it first", dest);
            else {
                gb_dest_tree             = GB_create_container(gb_tree_data2, dest);
                if (!gb_dest_tree) error = GB_await_error();
                else error               = GB_copy(gb_dest_tree, gb_source);
            }
        }
    }
    
    error = GB_end_transaction(GLOBAL_gb_dest, error);
    GB_end_transaction_show_error(GLOBAL_gb_merge, error, aw_message);

    free(source);
    free(dest);
}

AW_window *MG_merge_trees_cb(AW_root *awr) {
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    aws = new AW_window_simple;
    aws->init(awr, "MERGE_TREES", "MERGE TREES");
    aws->load_xfig("merge/trees.fig");

    aws->button_length(20);

    aws->at("close"); aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"mg_trees.hlp");
    aws->create_button("HELP", "HELP", "H");

    aws->at("trees1");
    awt_create_selection_list_on_trees(GLOBAL_gb_merge, (AW_window *)aws, AWAR_TREE_NAME1);

    aws->at("trees2");
    awt_create_selection_list_on_trees(GLOBAL_gb_dest, (AW_window *)aws, AWAR_TREE_NAME2);

    aws->at("delete1");
    aws->callback((AW_CB)MG_tr_delete_cb, (AW_CL)GLOBAL_gb_merge, 1);
    aws->create_button("DELETE TREE_DB1", "Delete Tree");

    aws->at("delete2");
    aws->callback((AW_CB)MG_tr_delete_cb, (AW_CL)GLOBAL_gb_dest, 2);
    aws->create_button("DELETE_TREE_DB2", "Delete Tree");

    aws->at("rename1");
    aws->callback((AW_CB1)AW_POPUP, (AW_CL)MG_create_tree_rename_window1);
    aws->create_button("RENAME_TREE_DB1", "Rename Tree");

    aws->at("rename2");
    aws->callback((AW_CB1)AW_POPUP, (AW_CL)MG_create_tree_rename_window2);
    aws->create_button("RENAME_TREE_DB2", "Rename Tree");

    aws->at("transfer");
    aws->callback(MG_transfer_tree);
    aws->create_button("TRANSFER_TREE", "Transfer Tree");

    aws->button_length(0);
    aws->shadow_width(1);
    aws->at("icon");
    aws->callback(AW_POPUP_HELP, (AW_CL)"mg_trees.hlp");
    aws->create_button("HELP_MERGE", "#merge/icon.bitmap");

    return (AW_window *)aws;
}

