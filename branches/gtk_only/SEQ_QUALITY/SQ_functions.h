//  ==================================================================== //
//                                                                       //
//    File      : SQ_functions.h                                         //
//    Purpose   : Functions used for calculation of alignment quality    //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July 2003 - February 2004                  //
//  Coded by Kai Bader (baderk@in.tum.de) in 2007 - 2008                 //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //


#ifndef SQ_FUNCTIONS_H
#define SQ_FUNCTIONS_H

#ifndef SQ_GROUPDATA_H
# include "SQ_GroupData.h"
#endif

#ifndef __MAP__
# include <map>
#endif
#ifndef __STRING__
# include <string>
#endif

#ifndef SMARTPTR_H
#include <smartptr.h>
#endif

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif

class AP_filter;
class arb_progress;
struct GBT_TREE;

typedef SmartPtr < SQ_GroupData> SQ_GroupDataPtr;
typedef std::map < std::string, SQ_GroupDataPtr> SQ_GroupDataDictionary;

GB_ERROR SQ_remove_quality_entries(GBDATA *gb_main);

GB_ERROR SQ_pass1_no_tree(SQ_GroupData * globalData, GBDATA * gb_main, AP_filter * filter, arb_progress& progress);
GB_ERROR SQ_pass2_no_tree(const SQ_GroupData * globalData, GBDATA * gb_main, AP_filter * filter, arb_progress& progress);

int SQ_count_nodes(GBT_TREE *node);

void SQ_calc_and_apply_group_data(GBT_TREE * node, GBDATA * gb_main, SQ_GroupData * data, AP_filter * filter, arb_progress& progress);
void SQ_calc_and_apply_group_data2(GBT_TREE * node, GBDATA * gb_main, const SQ_GroupData * data, AP_filter * filter, arb_progress& progress);

/*
 "option" is variable which is passed to function "SQ_get_value()".
 SQ_get_value() returns the values that are stored in the specific containers used for alignment quality evaluation.
 */

struct SQ_weights {
    int bases;
    int diff_from_average;
    int helix;
    int consensus;
    int iupac;
    int gc;
};

GB_ERROR SQ_evaluate(GBDATA * gb_main, const SQ_weights & weights, bool marked_only);
/*
 The "weight_..."  -values are passed to the function "SQ_evaluate()".
 SQ_evaluate() generates the final estimation for the quality of an alignment.
 It takes the values from the different containers, which are generated by the other functions, weights them
 and calculates a final value. The final value is stored in "value_of_evaluation" (see options).
 So, with the "weight_..."  -values one can customize how important a value stored in a container becomes
 for the final result.
 */

GB_ERROR SQ_mark_species(GBDATA * gb_main, int condition, bool marked_only);

void SQ_clear_group_dictionary();

enum SQ_TREE_ERROR {
    NONE = 0, ZOMBIE = 1, MISSING_NODE = 2
};

SQ_TREE_ERROR SQ_check_tree_structure(GBT_TREE * node);

#else
#error SQ_functions.h included twice
#endif