//  ==================================================================== //
//                                                                       //
//    File      : SQ_main.cxx                                            //
//    Purpose   : Entrypoint to Seq. Quality analysis; calls functions   //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July 2003 - February 2004                  //
//  Coded by Kai Bader (baderk@in.tum.de) in 2007 - 2008                 //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //

#include "seq_quality.h"
#include "SQ_functions.h"

#include <awt_filter.hxx>
#include <awt_sel_boxes.hxx>

#include <aw_awars.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>

#include <arb_progress.h>
#include <TreeNode.h>
#include <arb_global_defs.h>
#include <awt_config_manager.hxx>

// --------------------------------------------------------------------------------

#define AWAR_SQ_PERM "seq_quality/"     // saved in properties
#define AWAR_SQ_TEMP "tmp/seq_quality/" // not saved in properties

#define AWAR_SQ_WEIGHT_BASES     AWAR_SQ_PERM "weight_bases"
#define AWAR_SQ_WEIGHT_DEVIATION AWAR_SQ_PERM "weight_deviation"
#define AWAR_SQ_WEIGHT_HELIX     AWAR_SQ_PERM "weight_helix"
#define AWAR_SQ_WEIGHT_CONSENSUS AWAR_SQ_PERM "weight_consensus"
#define AWAR_SQ_WEIGHT_IUPAC     AWAR_SQ_PERM "weight_iupac"
#define AWAR_SQ_WEIGHT_GC        AWAR_SQ_PERM "weight_gc"

#define AWAR_SQ_MARK_ONLY_FLAG AWAR_SQ_PERM "mark_only_flag"
#define AWAR_SQ_MARK_FLAG      AWAR_SQ_PERM "mark_flag"
#define AWAR_SQ_MARK_BELOW     AWAR_SQ_PERM "mark_below"
#define AWAR_SQ_REEVALUATE     AWAR_SQ_PERM "reevaluate"
#define AWAR_SQ_FILTER_NAME    AWAR_SQ_TEMP "filter/name"

void SQ_create_awars(AW_root *aw_root, AW_default aw_def) {
    aw_root->awar_int(AWAR_SQ_WEIGHT_BASES,     5,  aw_def);
    aw_root->awar_int(AWAR_SQ_WEIGHT_DEVIATION, 15, aw_def);
    aw_root->awar_int(AWAR_SQ_WEIGHT_HELIX,     15, aw_def);
    aw_root->awar_int(AWAR_SQ_WEIGHT_CONSENSUS, 50, aw_def);
    aw_root->awar_int(AWAR_SQ_WEIGHT_IUPAC,     5,  aw_def);
    aw_root->awar_int(AWAR_SQ_WEIGHT_GC,        10, aw_def);
    aw_root->awar_int(AWAR_SQ_MARK_ONLY_FLAG,   0,  aw_def);
    aw_root->awar_int(AWAR_SQ_MARK_FLAG,        1,  aw_def);
    aw_root->awar_int(AWAR_SQ_MARK_BELOW,       40, aw_def);
    aw_root->awar_int(AWAR_SQ_REEVALUATE,       0,  aw_def);

    awt_create_filter_awars(aw_root, aw_def, AWAR_SQ_FILTER_NAME, AWAR_DEFAULT_ALIGNMENT);
}

// --------------------------------------------------------------------------------


static void sq_calc_seq_quality_cb(AW_window * aww, AW_CL res_from_awt_create_select_filter, AW_CL cl_gb_main) {
    GBDATA   *gb_main     = (GBDATA*)cl_gb_main;
    AW_root  *aw_root     = aww->get_root();
    GB_ERROR  error       = 0;
    TreeNode *tree        = 0;
    bool      marked_only = (aw_root->awar(AWAR_SQ_MARK_ONLY_FLAG)->read_int() > 0);

    arb_progress main_progress("Calculating sequence quality");

    {
        char *treename = aw_root->awar(AWAR_TREE)->read_string();

        if (treename && strcmp(treename, NO_TREE_SELECTED) != 0) {
            error = GB_push_transaction(gb_main);

            if (!error) {
                tree = GBT_read_tree(gb_main, treename, new SimpleRoot);
                if (!tree) error = GB_await_error();
                else {
                    error = GBT_link_tree(tree, gb_main, false, NULL, NULL);
                    if (!error) {
                        GBT_TreeRemoveType mode = marked_only ? GBT_KEEP_MARKED : GBT_REMOVE_ZOMBIES;
                        tree = GBT_remove_leafs(tree, mode, NULL, NULL, NULL);
                        if (!tree || tree->is_leaf) {
                            error = GBS_global_string("Tree contains less than 2 species after removing zombies%s",
                                                      marked_only ? " and non-marked" : "");
                        }
                    }
                }
            }

            error = GB_end_transaction(gb_main, error);
        }
        free(treename);
    }

    // if tree == 0 -> do basic quality calculations that are possible without tree information
    // otherwise    -> use all groups found in tree and compare sequences against the groups they are contained in

    if (!error) {
        struct SQ_weights weights;

        weights.bases = aw_root->awar(AWAR_SQ_WEIGHT_BASES)->read_int();
        weights.diff_from_average = aw_root->awar(AWAR_SQ_WEIGHT_DEVIATION)->read_int();
        weights.helix = aw_root->awar(AWAR_SQ_WEIGHT_HELIX)->read_int();
        weights.consensus = aw_root->awar(AWAR_SQ_WEIGHT_CONSENSUS)->read_int();
        weights.iupac = aw_root->awar(AWAR_SQ_WEIGHT_IUPAC)->read_int();
        weights.gc = aw_root->awar(AWAR_SQ_WEIGHT_GC)->read_int();

        int mark_flag = aw_root->awar(AWAR_SQ_MARK_FLAG)->read_int();
        int mark_below = aw_root->awar(AWAR_SQ_MARK_BELOW)->read_int();
        int reevaluate = aw_root->awar(AWAR_SQ_REEVALUATE)->read_int();

        // Load and use Sequence-Filter
        AP_filter *filter = awt_get_filter((adfiltercbstruct*)res_from_awt_create_select_filter);
        error             = awt_invalid_filter(filter);

        /*
          SQ_evaluate() generates the final estimation for the quality of an alignment.
          It takes the values from the different containers, which are generated by the other functions, weights them
          and calculates a final value. The final value is stored in "value_of_evaluation" (see options).
          With the values stored in "weights" one can customize how important a value stored in a container becomes
          for the final result.
        */

        if (!error) {
            if (tree == 0) {
                if (reevaluate) {
                    SQ_mark_species(gb_main, mark_below, marked_only);
                }
                else {
                    arb_progress  progress(GBT_get_species_count(gb_main)*2);
                    SQ_GroupData *globalData = new SQ_GroupData_RNA;

                    progress.subtitle("pass1");
                    error = SQ_pass1_no_tree(globalData, gb_main, filter, progress);
                    if (!error) {
                        progress.subtitle("pass2");
                        error = SQ_pass2_no_tree(globalData, gb_main, filter, progress);
                        if (!error) {
                            error = SQ_evaluate(gb_main, weights, marked_only);
                            if (mark_flag && !error) {
                                SQ_mark_species(gb_main, mark_below, marked_only);
                            }
                        }
                    }
                    if (error) progress.done();
                    delete globalData;
                }
            }
            else {
                SQ_TREE_ERROR check = SQ_check_tree_structure(tree);
                if (check != NONE) {
                    switch (check) {
                        case ZOMBIE:
                            error = "Found one or more zombies in the tree.\n"
                                "Please remove them or use another tree before running the quality check tool.";
                            break;
                        case MISSING_NODE:
                            error = "Missing node(s) or unusable tree structure.\n"
                                "Please fix the tree before running the quality check tool.";
                            break;
                        default:
                            error = "An error occurred while traversing the tree.\n"
                                "Please fix the tree before running the quality check tool.";
                            break;
                    }
                }
                else if (reevaluate) {
                    SQ_mark_species(gb_main, mark_below, marked_only);
                }
                else {
                    arb_progress progress(SQ_count_nodes(tree)*2);
                    SQ_GroupData *globalData = new SQ_GroupData_RNA;

                    progress.subtitle("pass1");
                    SQ_calc_and_apply_group_data(tree, gb_main, globalData, filter, progress);
                    progress.subtitle("pass2");
                    SQ_calc_and_apply_group_data2(tree, gb_main, globalData, filter, progress);
                    SQ_evaluate(gb_main, weights, marked_only);
                    if (mark_flag) SQ_mark_species(gb_main, mark_below, marked_only);
                    delete globalData;
                }
            }
        }
        awt_destroy_filter(filter);
    }

    if (error) aw_message(error);

    SQ_clear_group_dictionary();
    UNCOVERED();
    destroy(tree);
}

static void sq_remove_quality_entries_cb(AW_window *, AW_CL cl_gb_main) {
    GBDATA *gb_main = (GBDATA*)cl_gb_main;
    SQ_remove_quality_entries(gb_main);
}

static AWT_config_mapping_def seq_quality_config_mapping[] = {
    { AWAR_SQ_WEIGHT_BASES,     "wbases" },
    { AWAR_SQ_WEIGHT_DEVIATION, "wdeviation" },
    { AWAR_SQ_WEIGHT_HELIX,     "whelix" },
    { AWAR_SQ_WEIGHT_CONSENSUS, "wconsens" },
    { AWAR_SQ_WEIGHT_IUPAC,     "wiupac" },
    { AWAR_SQ_WEIGHT_GC,        "wgc" },
    { AWAR_SQ_MARK_ONLY_FLAG,   "onlymarked" },
    { AWAR_SQ_MARK_FLAG,        "markbad" },
    { AWAR_SQ_MARK_BELOW,       "markbelow" },
    { AWAR_SQ_REEVALUATE,       "reeval" },

    { 0, 0 }
};

AW_window *SQ_create_seq_quality_window(AW_root *aw_root, GBDATA *gb_main) {
    // create window for sequence quality calculation (called only once)

    AW_window_simple *aws = new AW_window_simple;

    aws->init(aw_root, "CALC_SEQ_QUALITY", "CALCULATE SEQUENCE QUALITY");
    aws->load_xfig("seq_quality.fig");

    aws->at("close");
    aws->callback((AW_CB0) AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("seq_quality.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->at("base");
    aws->create_input_field(AWAR_SQ_WEIGHT_BASES, 3);

    aws->at("deviation");
    aws->create_input_field(AWAR_SQ_WEIGHT_DEVIATION, 3);

    aws->at("no_helices");
    aws->create_input_field(AWAR_SQ_WEIGHT_HELIX, 3);

    aws->at("consensus");
    aws->create_input_field(AWAR_SQ_WEIGHT_CONSENSUS, 3);

    aws->at("iupac");
    aws->create_input_field(AWAR_SQ_WEIGHT_IUPAC, 3);

    aws->at("gc_proportion");
    aws->create_input_field(AWAR_SQ_WEIGHT_GC, 3);

    aws->at("monly");
    aws->create_toggle(AWAR_SQ_MARK_ONLY_FLAG);

    aws->at("mark");
    aws->create_toggle(AWAR_SQ_MARK_FLAG);

    aws->at("mark_below");
    aws->create_input_field(AWAR_SQ_MARK_BELOW, 3);

    aws->at("tree");
    awt_create_TREE_selection_list(gb_main, aws, AWAR_TREE, true);

    aws->at("filter");
    adfiltercbstruct *adfilter = awt_create_select_filter(aws->get_root(), gb_main, AWAR_SQ_FILTER_NAME);
    aws->callback(makeCreateWindowCallback(awt_create_select_filter_win, adfilter));
    aws->create_button("SELECT_FILTER", AWAR_SQ_FILTER_NAME);

    aws->at("go");
    aws->callback(sq_calc_seq_quality_cb, (AW_CL)adfilter, (AW_CL)gb_main);
    aws->highlight();
    aws->create_button("GO", "GO", "G");

    aws->at("config");
    AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "seq_quality", seq_quality_config_mapping);

    aws->at("reevaluate");
    aws->label("Re-Evaluate only");
    aws->create_toggle(AWAR_SQ_REEVALUATE);

    aws->at("remove");
    aws->callback(sq_remove_quality_entries_cb, (AW_CL)gb_main);
    aws->create_button("Remove", "Remove", "R");

    return aws;
}
