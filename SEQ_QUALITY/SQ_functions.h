//  ==================================================================== //
//                                                                       //
//    File      : SQ_functions.h                                         //
//    Purpose   : Functions used for calculation of alignment quality    //
//    Time-stamp: <Tue Dec/16/2003 09:22 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July - October 2003                        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //
#include <string>

#ifndef SQ_FUNCTIONS_H
#define SQ_FUNCTIONS_H

#ifndef SQ_GROUPADTA_H
#include "SQ_GroupData.h"
#endif

#ifndef __MAP__
#include <map>
#endif

#ifndef SMARTPTR_H
#include <smartptr.h>
#endif

typedef SmartPtr<SQ_GroupData> SQ_GroupDataPtr;
typedef map<string, SQ_GroupDataPtr> SQ_GroupDataDictionary;


GB_ERROR SQ_reset_quality_calcstate(GBDATA *gb_main);

GB_ERROR SQ_pass1(SQ_GroupData* globalData, GBDATA *gb_main, GBT_TREE *node);

GB_ERROR SQ_pass1_no_tree(SQ_GroupData* globalData, GBDATA *gb_main);

GB_ERROR SQ_pass2(SQ_GroupData* globalData, GBDATA *gb_main, GBT_TREE *node);

GB_ERROR SQ_pass2_no_tree(SQ_GroupData* globalData, GBDATA *gb_main);

GB_ERROR SQ_count_nr_of_species(GBDATA *gb_main);

void SQ_reset_counters(GBT_TREE *root); // reset counters used by SQ_calc_and_apply_group_data

void SQ_calc_and_apply_group_data(GBT_TREE *node, GBDATA *gb_main, SQ_GroupData *data);

void SQ_calc_and_apply_group_data2(GBT_TREE *node, GBDATA *gb_main, SQ_GroupData *data);

void create_multi_level_consensus(GBT_TREE *node, SQ_GroupData *data);

int SQ_get_value_no_tree(GBDATA *gb_main, const char *option);

int SQ_get_value(GBDATA *gb_main, const char *option);
    /*
      "option" is variable which is passed to function "SQ_get_value()".
      SQ_get_value() returns the values that are stored in the specific containers used for alignment quality evaluation.
      Right now the options you have are:

      number_of_bases
      number_of_spaces
      number_of_dots
      percent_of_bases
      percent_of_spaces
      percent_of_dots
      diff_from_average
      value_of_evaluation
    */

GB_ERROR SQ_evaluate(GBDATA *gb_main, int weight_bases, int weight_diff_from_average, int weight_helix, int weight_consensus, int weight_iupac);
    /*
      The "weight_..."  -values are passed to the function "SQ_evaluate()".
      SQ_evaluate() generates the final estimation for the quality of an alignment.
      It takes the values from the different containers, which are generated by the other functions, weights them
      and calculates a final value. The final value is stored in "value_of_evaluation" (see options).
      So, with the "weight_..."  -values one can customise how important a value stored in a contaier becomes
      for the final result.
    */

#else
#error SQ_functions.h included twice
#endif
