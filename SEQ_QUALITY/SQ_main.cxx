//  ==================================================================== //
//                                                                       //
//    File      : SQ_main.cxx                                            //
//    Purpose   : Entrypoint to Seq. Quality analysis; calls funktions   //
//    Time-stamp: <Tue Jun/13/2006 19:31 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July 2003 - February 2004                  //
//  Coded by Kai Bader (baderk@in.tum.de) in 2007                        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //


#include <stdio.h>
#include <stdlib.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <awt.hxx>
#include <awt_tree.hxx>
#include "../AWT/awtfilter.hxx"
#include <awt_sel_boxes.hxx>
#include <aw_awars.hxx>

#include "seq_quality.h"
#include "SQ_functions.h"

extern GBDATA *gb_main;

// --------------------------------------------------------------------------------

#define AWAR_SQ_PERM "seq_quality/"     // saved in properties
#define AWAR_SQ_TEMP "tmp/seq_quality/" // not saved in properties

#define AWAR_SQ_WEIGHT_BASES     AWAR_SQ_PERM "weight_bases"
#define AWAR_SQ_WEIGHT_DEVIATION AWAR_SQ_PERM "weight_deviation"
#define AWAR_SQ_WEIGHT_HELIX     AWAR_SQ_PERM "weight_helix"
#define AWAR_SQ_WEIGHT_CONSENSUS AWAR_SQ_PERM "weight_consensus"
#define AWAR_SQ_WEIGHT_IUPAC     AWAR_SQ_PERM "weight_iupac"
#define AWAR_SQ_WEIGHT_GC        AWAR_SQ_PERM "weight_gc"

#define AWAR_SQ_MARK_ONLY_FLAG  AWAR_SQ_PERM "mark_only_flag"
#define AWAR_SQ_MARK_FLAG  AWAR_SQ_PERM "mark_flag"
#define AWAR_SQ_MARK_BELOW AWAR_SQ_PERM "mark_below"
#define AWAR_SQ_REEVALUATE AWAR_SQ_PERM "reevaluate"

#define AWAR_FILTER_PREFIX  AWAR_SQ_TEMP "filter/"
#define AWAR_FILTER_NAME    AWAR_FILTER_PREFIX "name"
#define AWAR_FILTER_FILTER  AWAR_FILTER_PREFIX "filter"
#define AWAR_FILTER_ALI     AWAR_FILTER_PREFIX "alignment"

void SQ_create_awars(AW_root * aw_root, AW_default aw_def)
{
    aw_root->awar_int(AWAR_SQ_WEIGHT_BASES, 5, aw_def);
    aw_root->awar_int(AWAR_SQ_WEIGHT_DEVIATION, 15, aw_def);
    aw_root->awar_int(AWAR_SQ_WEIGHT_HELIX, 15, aw_def);
    aw_root->awar_int(AWAR_SQ_WEIGHT_CONSENSUS, 50, aw_def);
    aw_root->awar_int(AWAR_SQ_WEIGHT_IUPAC, 5, aw_def);
    aw_root->awar_int(AWAR_SQ_WEIGHT_GC, 10, aw_def);
    aw_root->awar_int(AWAR_SQ_MARK_ONLY_FLAG, 0, aw_def);
    aw_root->awar_int(AWAR_SQ_MARK_FLAG, 1, aw_def);
    aw_root->awar_int(AWAR_SQ_MARK_BELOW, 40, aw_def);
    aw_root->awar_int(AWAR_SQ_REEVALUATE, 0, aw_def);
    aw_root->awar_string(AWAR_FILTER_NAME, "none", aw_def);
    aw_root->awar_string(AWAR_FILTER_FILTER, "", aw_def);
    AW_awar *awar_ali = aw_root->awar_string(AWAR_FILTER_ALI, "", aw_def);
    awar_ali->map("presets/use");
}

// --------------------------------------------------------------------------------


static void sq_calc_seq_quality_cb(AW_window * aww,
                                   AW_CL res_from_awt_create_select_filter)
{
    AW_root *aw_root = aww->get_root();
    GB_ERROR error = 0;
    GBT_TREE *tree = 0;
    AP_tree *ap_tree = 0;
    AP_tree_root *ap_tree_root = 0;
    bool marked_only =
        (aw_root->awar(AWAR_SQ_MARK_ONLY_FLAG)->read_int() > 0);
    char *treename = aw_root->awar(AWAR_TREE)->read_string();   // contains "????" if no tree is selected

    if (treename && strstr(treename, "????") == 0) {
        ap_tree = new AP_tree(0);
        ap_tree_root = new AP_tree_root(gb_main, ap_tree, treename);

        error = ap_tree->load(ap_tree_root, 0, GB_FALSE, GB_FALSE, 0, 0);
        if (error) {
            delete ap_tree;
            delete ap_tree_root;
            aw_message(GBS_global_string
                       ("Cannot read tree '%s' -- group specific calculations skipped.\n   Treating all available sequences as one group!",
                        treename));
            tree = 0;
        } else {
            ap_tree_root->tree = ap_tree;

            GB_push_transaction(gb_main);
            GBT_link_tree((GBT_TREE *) ap_tree_root->tree, gb_main,
                          GB_FALSE, 0, 0);
            GB_pop_transaction(gb_main);

            if (marked_only) {
                error =
                    ap_tree_root->tree->remove_leafs(gb_main,
                                                     AWT_REMOVE_NOT_MARKED
                                                     | AWT_REMOVE_DELETED);
            }
            if (error || !ap_tree_root->tree
                || ((GBT_TREE *) ap_tree_root->tree)->is_leaf) {
                aw_message
                    ("No tree selected -- group specific calculations skipped.");
                tree = 0;
            } else {
                error =
                    ap_tree_root->tree->remove_leafs(gb_main,
                                                     AWT_REMOVE_DELETED);
                tree = ((GBT_TREE *) ap_tree_root->tree);
            }
        }
    } else {
        aw_message
            ("No tree selected -- group specific calculations skipped.");
        tree = 0;
    }

    free(treename);

    if (!error) {
        // error = SQ_reset_quality_calcstate(gb_main);
    }
    // if tree == 0 -> do basic quality calculations that are possible without tree information
    // otherwise    -> use all groups found in tree and compare sequences against the groups they are contained in

    if (!error) {
        struct SQ_weights weights;

        weights.bases = aw_root->awar(AWAR_SQ_WEIGHT_BASES)->read_int();
        weights.diff_from_average =
            aw_root->awar(AWAR_SQ_WEIGHT_DEVIATION)->read_int();
        weights.helix = aw_root->awar(AWAR_SQ_WEIGHT_HELIX)->read_int();
        weights.consensus =
            aw_root->awar(AWAR_SQ_WEIGHT_CONSENSUS)->read_int();
        weights.iupac = aw_root->awar(AWAR_SQ_WEIGHT_IUPAC)->read_int();
        weights.gc = aw_root->awar(AWAR_SQ_WEIGHT_GC)->read_int();

        int mark_flag = aw_root->awar(AWAR_SQ_MARK_FLAG)->read_int();
        int mark_below = aw_root->awar(AWAR_SQ_MARK_BELOW)->read_int();
        int reevaluate = aw_root->awar(AWAR_SQ_REEVALUATE)->read_int();

        // Load and use Sequence-Filter
        AP_filter *filter =
            awt_get_filter(aw_root, res_from_awt_create_select_filter);

        /*
           SQ_evaluate() generates the final estimation for the quality of an alignment.
           It takes the values from the different containers, which are generated by the other functions, weights them
           and calculates a final value. The final value is stored in "value_of_evaluation" (see options).
           With the values stored in "weights" one can customise how important a value stored in a contaier becomes
           for the final result.
         */

        if (tree == 0) {
            if (reevaluate) {
                aw_openstatus("Marking Sequences...");
                SQ_mark_species(gb_main, mark_below, marked_only);
                aw_closestatus();
            } else {
                SQ_GroupData *globalData = new SQ_GroupData_RNA;
                SQ_count_nr_of_species(gb_main);
                aw_openstatus("Calculating pass 1 of 2 ...");
                SQ_pass1_no_tree(globalData, gb_main, filter);
                aw_closestatus();
                aw_openstatus("Calculating pass 2 of 2 ...");
                SQ_pass2_no_tree(globalData, gb_main, filter);
                SQ_evaluate(gb_main, weights);
                aw_closestatus();
                if (mark_flag) {
                    aw_openstatus("Marking Sequences...");
                    SQ_mark_species(gb_main, mark_below, marked_only);
                    aw_closestatus();
                }
                delete globalData;
            }
        } else {
            aw_openstatus("Checking tree for irregularities...");
            SQ_TREE_ERROR check = NONE;
            if ((check = SQ_check_tree_structure(tree)) != NONE) {
                switch (check) {
                case ZOMBIE:
                    aw_message
                        ("Found one or more zombies in the tree.\nPlease remove them or use another tree before running the quality check tool.");
                    break;
                case MISSING_NODE:
                    aw_message
                        ("Missing node(s) or unusable tree structure.\nPlease fix the tree before running the quality check tool.");
                    break;
                default:
                    aw_message
                        ("An error occured while traversing the tree.\nPlease fix the tree before running the quality check tool.");
                    break;
                }
                aw_closestatus();
                return;
            }
            aw_closestatus();

            if (reevaluate) {
                aw_openstatus("Marking Sequences...");
                SQ_count_nr_of_species(gb_main);
                SQ_mark_species(gb_main, mark_below, marked_only);
                aw_closestatus();
            } else {
                aw_openstatus("Calculating pass 1 of 2...");
                SQ_reset_counters(tree);
                SQ_GroupData *globalData = new SQ_GroupData_RNA;
                SQ_calc_and_apply_group_data(tree, gb_main, globalData,
                                             filter);
                aw_closestatus();
                SQ_reset_counters(tree);
                aw_openstatus("Calculating pass 2 of 2...");
                SQ_calc_and_apply_group_data2(tree, gb_main, globalData,
                                              filter);
                SQ_evaluate(gb_main, weights);
                aw_closestatus();
                SQ_reset_counters(tree);
                if (mark_flag) {
                    aw_openstatus("Marking Sequences...");
                    SQ_count_nr_of_species(gb_main);
                    SQ_mark_species(gb_main, mark_below, marked_only);
                    aw_closestatus();
                }
                delete globalData;
            }
        }
    }

    if (error) {
        aw_message(error);
    }

    SQ_clear_group_dictionary();

    // Cleanup trees
    delete ap_tree_root;
    delete ap_tree;
}


// create window for sequence quality calculation (called only once)
AW_window *SQ_create_seq_quality_window(AW_root * aw_root, AW_CL)
{
    AW_window_simple *aws = new AW_window_simple;

    aws->init(aw_root, "CALC_SEQ_QUALITY", "CALCULATE SEQUENCE QUALITY");
    aws->load_xfig("seq_quality.fig");

    aws->at("close");
    aws->callback((AW_CB0) AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL) "seq_quality.hlp");
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
    awt_create_selection_list_on_trees(gb_main, (AW_window *) aws,
                                       AWAR_TREE);

    aws->at("filter");
    AW_CL filtercd = awt_create_select_filter(aws->get_root(), gb_main,
                                              AWAR_FILTER_NAME);
    aws->callback(AW_POPUP, (AW_CL) awt_create_select_filter_win,
                  filtercd);
    aws->create_button("SELECT_FILTER", AWAR_FILTER_NAME);

    aws->at("go");
    aws->callback(sq_calc_seq_quality_cb, filtercd);
    aws->highlight();
    aws->create_button("GO", "GO", "G");

    aws->at("reevaluate");
    aws->label("Re-Evaluate only");
    aws->create_toggle(AWAR_SQ_REEVALUATE);

    return aws;
}
