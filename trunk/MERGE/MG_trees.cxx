// =============================================================== //
//                                                                 //
//   File      : MG_trees.cxx                                      //
//   Purpose   : Merge trees between databases                     //
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
#include <aw_window.hxx>
#include <arbdbt.h>
#include <arb_global_defs.h>

#define AWAR_TREE_NAME_SRC AWAR_MERGE_TMP_SRC "tree_name"
#define AWAR_TREE_NAME_DST AWAR_MERGE_TMP_DST "tree_name"

void MG_create_trees_awar(AW_root *aw_root, AW_default aw_def) {
    aw_root->awar_string(AWAR_TREE_NAME_SRC, NO_TREE_SELECTED, aw_def);
    aw_root->awar_string(AWAR_TREE_NAME_DST, NO_TREE_SELECTED, aw_def);

    TreeAdmin::create_awars(aw_root, aw_def);
}

static GB_ERROR transfer_tree(const char *tree_name, bool overwrite, const char *behind_name) {
    GB_ERROR  error   = NULL;
    GBDATA   *gb_tree = GBT_find_tree(GLOBAL_gb_src, tree_name);
    if (!gb_tree) {
        if (strcmp(tree_name, NO_TREE_SELECTED) == 0) {
            error = "No tree selected in source DB";
        }
        else {
            error = GBS_global_string("No tree '%s' in source DB", tree_name);
        }
    }
    else {
        GBDATA *gb_behind_tree = NULL;
        if (behind_name && strcmp(behind_name, NO_TREE_SELECTED) != 0) {
            gb_behind_tree = GBT_find_tree(GLOBAL_gb_dst, behind_name);
            if (!gb_behind_tree) {
                error = GBS_global_string("Can't position tree behind '%s' (no such tree)", behind_name);
            }
        }

        if (!error) {
            GBDATA *gb_dest_tree = GBT_find_tree(GLOBAL_gb_dst, tree_name);
            if (gb_dest_tree)  {
                if (!overwrite) {
                    error = GBS_global_string("Tree '%s' already exists in destination DB", tree_name);
                }
                else {
                    error        = GB_delete(gb_dest_tree);
                    gb_dest_tree = NULL;
                }
            }
        }

        if (!error) {
            GBDATA *gb_dest_tree_data     = GBT_get_tree_data(GLOBAL_gb_dst);
            if (!gb_dest_tree_data) error = GB_await_error();
            else {
                GBDATA *gb_dest_tree     = GB_create_container(gb_dest_tree_data, tree_name);
                if (!gb_dest_tree) error = GB_await_error();
                else {
                    error = GB_copy(gb_dest_tree, gb_tree);
                    if (!error) {
                        if (!gb_behind_tree) gb_behind_tree = GBT_find_bottom_tree(GLOBAL_gb_dst);
                        if (gb_behind_tree) error           = GBT_move_tree(gb_dest_tree, GBT_BEHIND, gb_behind_tree);
                    }
                }
            }
        }
    }
    return error;
}

static void MG_transfer_tree(AW_window *aww) {
    AW_root *awr = aww->get_root();

    AW_awar *awar_tree_source = awr->awar(AWAR_TREE_NAME_SRC);
    AW_awar *awar_tree_dest   = awr->awar(AWAR_TREE_NAME_DST);

    char *source_name = awar_tree_source->read_string();
    char *behind_name = awar_tree_dest->read_string();

    GB_ERROR error    = GB_begin_transaction(GLOBAL_gb_dst);
    if (!error) error = GB_begin_transaction(GLOBAL_gb_src);

    if (!error) {
        error = transfer_tree(source_name, false, behind_name);
    }

    error = GB_end_transaction(GLOBAL_gb_dst, error);
    GB_end_transaction_show_error(GLOBAL_gb_src, error, aw_message);

    if (!error) {
        awar_tree_dest->write_string(source_name);
        GB_transaction ta(GLOBAL_gb_src);
        GBDATA *gb_next = GBT_get_next_tree(GBT_find_tree(GLOBAL_gb_src, source_name));
        awar_tree_source->write_string(gb_next ? GBT_get_tree_name(gb_next) : NO_TREE_SELECTED);
    }

    free(behind_name);
    free(source_name);
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
        awt_create_selection_list_on_trees(GLOBAL_gb_src, (AW_window *)aws, AWAR_TREE_NAME_SRC);

        aws->at("trees2");
        awt_create_selection_list_on_trees(GLOBAL_gb_dst, (AW_window *)aws, AWAR_TREE_NAME_DST);

        static TreeAdmin::Spec spec1(GLOBAL_gb_src, AWAR_TREE_NAME_SRC);
        static TreeAdmin::Spec spec2(GLOBAL_gb_dst,  AWAR_TREE_NAME_DST);
    
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

