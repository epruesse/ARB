// =============================================================== //
//                                                                 //
//   File      : MG_trees.cxx                                      //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "merge.hxx"
#include <TreeAdmin.h>
#include <awt_sel_boxes.hxx>
#include <aw_root.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <arbdbt.h>

#define AWAR_TREE_NAME1 "tmp/merge1/tree_name"
#define AWAR_TREE_NAME2 "tmp/merge2/tree_name"

void MG_create_trees_awar(AW_root *aw_root, AW_default aw_def) {
    aw_root->awar_string(AWAR_TREE_NAME1, "", aw_def);
    aw_root->awar_string(AWAR_TREE_NAME2, "", aw_def);

    TreeAdmin::create_awars(aw_root, aw_def);
}

static void MG_transfer_tree(AW_window *aww) {
    AW_root *awr    = aww->get_root();
    char    *source = awr->awar(AWAR_TREE_NAME1)->read_string();
    char    *dest   = awr->awar(AWAR_TREE_NAME1)->read_string();

    GB_ERROR error    = GB_begin_transaction(GLOBAL_gb_dest);
    if (!error) error = GB_begin_transaction(GLOBAL_gb_merge);

    if (!error) {
        GBDATA *gb_tree_data1 = GBT_get_tree_data(GLOBAL_gb_merge);
        GBDATA *gb_tree_data2 = GBT_get_tree_data(GLOBAL_gb_dest);

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
    if (!aws) {
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

        static TreeAdmin::Spec spec1(GLOBAL_gb_merge, AWAR_TREE_NAME1);
        static TreeAdmin::Spec spec2(GLOBAL_gb_dest,  AWAR_TREE_NAME2);
    
        aws->at("delete1");
        aws->callback(TreeAdmin::delete_tree_cb, (AW_CL)&spec1);
        aws->create_button("DELETE TREE_DB1", "Delete Tree");

        aws->at("delete2");
        aws->callback(TreeAdmin::delete_tree_cb, (AW_CL)&spec2);
        aws->create_button("DELETE_TREE_DB2", "Delete Tree");

        aws->at("rename1");
        aws->callback(AW_POPUP, (AW_CL)TreeAdmin::create_rename_window, (AW_CL)&spec1); 
        aws->create_button("RENAME_TREE_DB1", "Rename Tree");

        aws->at("rename2");
        aws->callback(AW_POPUP, (AW_CL)TreeAdmin::create_rename_window, (AW_CL)&spec2); 
        aws->create_button("RENAME_TREE_DB2", "Rename Tree");

        aws->at("transfer");
        aws->callback(MG_transfer_tree);
        aws->create_button("TRANSFER_TREE", "Transfer Tree");

        aws->button_length(0);
        aws->shadow_width(1);
        aws->at("icon");
        aws->callback(AW_POPUP_HELP, (AW_CL)"mg_trees.hlp");
        aws->create_button("HELP_MERGE", "#merge/icon.bitmap");
    }
    return aws;
}

