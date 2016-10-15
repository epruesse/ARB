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
#include <arbdbt.h>
#include <arb_global_defs.h>

#define AWAR_TREE_NAME_SRC AWAR_MERGE_TMP_SRC "tree_name"
#define AWAR_TREE_NAME_DST AWAR_MERGE_TMP_DST "tree_name"

#define AWAR_TREE_XFER_WHAT AWAR_MERGE_TMP "xfer_what"
#define AWAR_TREE_OVERWRITE AWAR_MERGE_TMP "overwrite"

enum TREE_XFER_MODE {
    XFER_SELECTED, 
    XFER_ALL, 
    XFER_MISSING, 
    XFER_EXISTING, 
};

void MG_create_trees_awar(AW_root *aw_root, AW_default aw_def) {
    aw_root->awar_string(AWAR_TREE_NAME_SRC, NO_TREE_SELECTED, aw_def);
    aw_root->awar_string(AWAR_TREE_NAME_DST, NO_TREE_SELECTED, aw_def);
    
    aw_root->awar_int(AWAR_TREE_XFER_WHAT, XFER_SELECTED, aw_def);
    aw_root->awar_int(AWAR_TREE_OVERWRITE, 0,             aw_def);

    TreeAdmin::create_awars(aw_root, aw_def, false);
}

static GB_ERROR transfer_tree(const char *tree_name, bool overwrite, const char *behind_name) {
    // transfer tree 'tree_name' from DB1->DB2.
    // 
    // if 'overwrite' is true and tree already exists, overwrite tree w/o changing its position.
    // 
    // if tree does not exist, insert it behind tree 'behind_name'.
    // if tree 'behind_name' does not exist or if 'behind_name' is NULL, insert at end.
    

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
        GBDATA         *gb_at_tree = NULL;
        GBT_ORDER_MODE  next_to    = GBT_BEHIND;

        if (behind_name && strcmp(behind_name, NO_TREE_SELECTED) != 0) {
            gb_at_tree = GBT_find_tree(GLOBAL_gb_dst, behind_name);
            if (!gb_at_tree) {
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
                    // keep position of existing tree
                    GBDATA *gb_tree_infrontof = GBT_tree_infrontof(gb_dest_tree);
                    if (gb_tree_infrontof) {
                        next_to    = GBT_BEHIND;
                        gb_at_tree = gb_tree_infrontof;
                    }
                    else {
                        GBDATA *gb_tree_behind = GBT_tree_behind(gb_dest_tree);
                        next_to    = GBT_INFRONTOF;
                        gb_at_tree = gb_tree_behind;
                    }

                    error = GB_delete(gb_dest_tree);
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
                        if (!gb_at_tree) {
                            gb_at_tree = GBT_find_bottom_tree(GLOBAL_gb_dst);
                            next_to    = GBT_BEHIND;
                        }
                        if (gb_at_tree) {
                            error = GBT_move_tree(gb_dest_tree, next_to, gb_at_tree);
                        }
                    }
                }
            }
        }
    }
    return error;
}

static void MG_transfer_tree(AW_window *aww) {
    AW_root *awr = aww->get_root();

    TREE_XFER_MODE what = (TREE_XFER_MODE)awr->awar(AWAR_TREE_XFER_WHAT)->read_int();

    AW_awar *awar_tree_source = awr->awar(AWAR_TREE_NAME_SRC);
    AW_awar *awar_tree_dest   = awr->awar(AWAR_TREE_NAME_DST);

    bool overwrite = awr->awar(AWAR_TREE_OVERWRITE)->read_int();

    char *source_name = awar_tree_source->read_string();
    char *behind_name = awar_tree_dest->read_string();

    char *select_dst = NULL;

    GB_ERROR error    = GB_begin_transaction(GLOBAL_gb_dst);
    if (!error) error = GB_begin_transaction(GLOBAL_gb_src);

    int xferd_missing  = 0;
    int xferd_existing = 0;

    if (!error) {
        switch (what) {
            case XFER_SELECTED:
                error = transfer_tree(source_name, overwrite, behind_name);
                if (!error) select_dst = strdup(source_name);
                break;

            case XFER_ALL:
            case XFER_EXISTING:
                for (GBDATA *gb_tree = GBT_find_top_tree(GLOBAL_gb_src); gb_tree && !error; gb_tree = GBT_tree_behind(gb_tree)) {
                    const char *tree_name = GBT_get_tree_name(gb_tree);
                    GBDATA     *gb_exists = GBT_find_tree(GLOBAL_gb_dst, tree_name);

                    if (gb_exists) {
                        xferd_existing++;
                        error = transfer_tree(tree_name, overwrite, NULL);
                    }
                }
                if (what == XFER_EXISTING) break;
                // fall-through for XFER_ALL
            case XFER_MISSING:
                for (GBDATA *gb_tree = GBT_find_top_tree(GLOBAL_gb_src); gb_tree && !error; gb_tree = GBT_tree_behind(gb_tree)) {
                    const char *tree_name = GBT_get_tree_name(gb_tree);
                    GBDATA     *gb_exists = GBT_find_tree(GLOBAL_gb_dst, tree_name);

                    if (!gb_exists) {
                        error = transfer_tree(tree_name, false, behind_name);
                        xferd_missing++;
                        if (!select_dst) select_dst = strdup(tree_name); // select first missing in dest box
                        freedup(behind_name, tree_name);
                    }
                }
                break;
        }
    }

    error = GB_end_transaction(GLOBAL_gb_dst, error);
    GB_end_transaction_show_error(GLOBAL_gb_src, error, aw_message);

    if (!error) {
        if (select_dst) awar_tree_dest->write_string(select_dst);

        if (what == XFER_SELECTED) {
            GB_transaction  ta(GLOBAL_gb_src);
            GBDATA         *gb_next = GBT_find_next_tree(GBT_find_tree(GLOBAL_gb_src, source_name));
            awar_tree_source->write_string(gb_next ? GBT_get_tree_name(gb_next) : NO_TREE_SELECTED);
        }

        if (xferd_existing) {
            if (xferd_missing) aw_message(GBS_global_string("Transferred %i existing and %i missing trees", xferd_existing, xferd_missing));
            else aw_message(GBS_global_string("Transferred %i existing trees", xferd_existing));
        }
        else {
            if (xferd_missing) aw_message(GBS_global_string("Transferred %i missing trees", xferd_missing));
            else {
                if (what != XFER_SELECTED) aw_message("No trees have been transferred");
            }
        }
    }

    free(select_dst);
    free(behind_name);
    free(source_name);
}

AW_window *MG_create_merge_trees_window(AW_root *awr) {
    GB_ERROR error = MG_expect_renamed();
    if (error) {
        aw_message(error);
        return NULL; // deny to open window before user has renamed species
    }

    AW_window_simple *aws = new AW_window_simple;

    aws->init(awr, "MERGE_TREES", "MERGE TREES");
    aws->load_xfig("merge/trees.fig");

    aws->button_length(7);

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("mg_trees.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->at("trees1");
    awt_create_TREE_selection_list(GLOBAL_gb_src, aws, AWAR_TREE_NAME_SRC, true);

    aws->at("trees2");
    awt_create_TREE_selection_list(GLOBAL_gb_dst, aws, AWAR_TREE_NAME_DST, true);

    static TreeAdmin::Spec src_spec(GLOBAL_gb_src, AWAR_TREE_NAME_SRC);
    static TreeAdmin::Spec dst_spec(GLOBAL_gb_dst,  AWAR_TREE_NAME_DST);
    
    aws->button_length(15);

    aws->at("delete1");
    aws->callback(makeWindowCallback(TreeAdmin::delete_tree_cb, &src_spec));
    aws->create_button("DELETE TREE_DB1", "Delete Tree");

    aws->at("delete2");
    aws->callback(makeWindowCallback(TreeAdmin::delete_tree_cb, &dst_spec));
    aws->create_button("DELETE_TREE_DB2", "Delete Tree");

    aws->at("rename1");
    aws->callback(makeCreateWindowCallback(TreeAdmin::create_rename_window, &src_spec));
    aws->create_button("RENAME_TREE_DB1", "Rename Tree");

    aws->at("rename2");
    aws->callback(makeCreateWindowCallback(TreeAdmin::create_rename_window, &dst_spec));
    aws->create_button("RENAME_TREE_DB2", "Rename Tree");

    aws->at("transfer");
    aws->callback(MG_transfer_tree);
    aws->create_autosize_button("TRANSFER_TREE", "Transfer");

    aws->at("xfer_what");
    aws->create_option_menu(AWAR_TREE_XFER_WHAT, true);
    aws->insert_default_option("selected tree",  "s", XFER_SELECTED);
    aws->insert_option        ("all trees",      "a", XFER_ALL);
    aws->insert_option        ("missing trees",  "m", XFER_MISSING);
    aws->insert_option        ("existing trees", "e", XFER_EXISTING);
    aws->update_option_menu();

    aws->at("overwrite");
    aws->label("Overwrite trees?");
    aws->create_toggle(AWAR_TREE_OVERWRITE);

    aws->button_length(0);
    aws->shadow_width(1);
    aws->at("icon");
    aws->callback(makeHelpCallback("mg_trees.hlp"));
    aws->create_button("HELP_MERGE", "#merge/icon.xpm");

    return aws;
}

