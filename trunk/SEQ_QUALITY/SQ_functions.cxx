//  ==================================================================== //
//                                                                       //
//    File      : SQ_functions.cxx                                       //
//    Purpose   : Implementation of SQ_functions.h                       //
//    Time-stamp: <Tue Dec/16/2003 09:18 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July 2003 - February 2004                  //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //

#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "arbdb.h"
#include "arbdbt.h"
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_preset.hxx>
#include <awt.hxx>

#include "SQ_GroupData.h"
#include "SQ_ambiguities.h"
#include "SQ_helix.h"
#include "SQ_physical_layout.h"

#ifndef __MAP__
#include <map>
#endif

#ifndef SMARTPTR_H
#include <smartptr.h>
#endif

typedef SmartPtr<SQ_GroupData> SQ_GroupDataPtr;
typedef map<string, SQ_GroupDataPtr> SQ_GroupDataDictionary;

static SQ_GroupDataDictionary group_dict;
static int globalcounter  = -1;
static int groupcounter   = -1;
static int globalcounter_notree = 0;
static int pass1_counter_notree = 0;
static int pass2_counter_notree = 0;

enum { CS_CLEAR, CS_PASS1 };

static GB_ERROR no_data_error(GBDATA *gb_species, const char *ali_name) {
    GBDATA     *gb_name = GB_find(gb_species, "name", 0, down_level);
    const char *name    = "<unknown>";
    if (gb_name) name = GB_read_char_pntr(gb_name);
    return GBS_global_string("Species '%s' has no data in alignment '%s'",
                             name, ali_name);
}


GB_ERROR SQ_reset_quality_calcstate(GBDATA *gb_main) {
    GB_push_transaction(gb_main);

    GB_ERROR  error          = 0;
    char     *alignment_name = GBT_get_default_alignment(gb_main);

    for (GBDATA *gb_species = GBT_first_species(gb_main);
         gb_species && !error;
         gb_species = GBT_next_species(gb_species))
    {
        GBDATA *gb_ali = GB_find(gb_species,alignment_name,0,down_level);
        if (!gb_ali) {
            error = no_data_error(gb_species, alignment_name);
        }
        else {
            GBDATA *gb_quality     = GB_search(gb_ali, "quality", GB_CREATE_CONTAINER);
            if (!gb_quality) error = GB_get_error();
            else {
                GBDATA *gb_calcstate     = GB_search(gb_quality, "calcstate", GB_INT);
                if (!gb_calcstate) error = GB_get_error();
                else {
                    GB_write_int(gb_calcstate, CS_CLEAR); // clear calculation state
                }
            }
        }
    }
    free(alignment_name);

    if (error) GB_abort_transaction(gb_main);
    else GB_pop_transaction(gb_main);

    return error;
}


int SQ_get_value(GBDATA *gb_main, const char *option){


    int result = 0;
    char *alignment_name;

    GBDATA *gb_species;
    GBDATA *gb_species_data;
    GBDATA *gb_name;
    GBDATA *(*getFirst)(GBDATA*) = 0;
    GBDATA *(*getNext)(GBDATA*) = 0;


    GB_push_transaction(gb_main);
    gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    alignment_name = GBT_get_default_alignment(gb_main);
    seq_assert(alignment_name);

    if (true /*marked_only*/) {
	getFirst = GBT_first_marked_species;
	getNext = GBT_next_marked_species;
    }

    for (gb_species = getFirst(gb_main); gb_species; gb_species = getNext(gb_species) ){

	gb_name = GB_find(gb_species, "name", 0, down_level);

	if (gb_name) {

	    GBDATA *gb_ali = GB_find(gb_species,alignment_name,0,down_level);

	    if (gb_ali) {
		GBDATA *gb_quality = GB_search(gb_ali, "quality", GB_FIND);
		GBDATA *gb_result1 = GB_search(gb_quality, option, GB_INT);
		result = GB_read_int(gb_result1);
	    }
	}
    }
    free(alignment_name);

    GB_pop_transaction(gb_main);
    return result;

}


int SQ_get_value_no_tree(GBDATA *gb_main, const char *option){


    int result = 0;
    char *alignment_name;

    GBDATA *read_sequence = 0;
    GBDATA *gb_species;
    GBDATA *gb_species_data;
    GBDATA *gb_name;
    GBDATA *(*getFirst)(GBDATA*) = 0;
    GBDATA *(*getNext)(GBDATA*) = 0;


    GB_push_transaction(gb_main);
    gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    alignment_name = GBT_get_default_alignment(gb_main);
    seq_assert(alignment_name);

    getFirst = GBT_first_species;
    getNext = GBT_next_species;


    for (gb_species = getFirst(gb_main); gb_species; gb_species = getNext(gb_species) ){
	gb_name = GB_find(gb_species, "name", 0, down_level);
	if (gb_name) {
	    GBDATA *gb_ali = GB_find(gb_species,alignment_name,0,down_level);
	    if (gb_ali) {
		GBDATA *gb_quality = GB_search(gb_ali, "quality", GB_CREATE_CONTAINER);
		if (gb_quality){
		    read_sequence = GB_find(gb_ali,"data",0,down_level);
		    if (read_sequence) {
			GBDATA *gb_result1 = GB_search(gb_quality, option, GB_INT);
			result = GB_read_int(gb_result1);
		    }
		}
	    }
	}
    }
    free(alignment_name);


    GB_pop_transaction(gb_main);
    return result;

}


GB_ERROR SQ_evaluate(GBDATA *gb_main, int weight_bases, int weight_diff_from_average, int weight_helix, int weight_consensus, int weight_iupac, int weight_gc){


    char *alignment_name;

    GBDATA *gb_species;
    GBDATA *gb_species_data;
    GBDATA *gb_name;
    GBDATA *(*getFirst)(GBDATA*) = 0;
    GBDATA *(*getNext)(GBDATA*) = 0;
    GB_ERROR error = 0;


    GB_push_transaction(gb_main);
    gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    alignment_name = GBT_get_default_alignment(gb_main);
    seq_assert(alignment_name);

    if (true /*marked_only*/) {
	getFirst = GBT_first_marked_species;
	getNext = GBT_next_marked_species;
    }

    //DEBUG why SIGSEV???
    //getFirst = GBT_first_species;
    //getNext  = GBT_next_species;


    for (gb_species = getFirst(gb_main);
	 gb_species && !error;
	 gb_species = getNext(gb_species) ){

	gb_name = GB_find(gb_species, "name", 0, down_level);

        if (!gb_name) error = GB_get_error();

	else {

	    GBDATA *gb_ali = GB_find(gb_species,alignment_name,0,down_level);

	    if (!gb_ali) error = GBS_global_string("No such alignment '%s'", alignment_name);

	    else {
		int bases      = 0;
		int dfa        = 0;
		int result     = 0;
		int noh        = 0;
		int cos        = 0;
		int iupv       = 0;
		int gcprop     = 0;
		int value      = 0;

		GBDATA *gb_quality = GB_search(gb_ali, "quality", GB_FIND);

		GBDATA *gb_result1 = GB_search(gb_quality, "percent_of_bases", GB_INT);
		bases = GB_read_int(gb_result1);
		if (bases < 6) result = 1;
		else {
	  	    if (bases < 9) result = 2;
		    else { result = 3;}
		}
		result = result * weight_bases;
		value += result;

		GBDATA *gb_result2 = GB_search(gb_quality, "diff_from_average", GB_INT);
		dfa = GB_read_int(gb_result2);
		if (abs(dfa) < 2) result = 3;
		else {
	  	    if (abs(dfa) < 4) result = 2;
		    else { result = 1;}
		}
		result = result * weight_diff_from_average;
		value += result;

		GBDATA *gb_result3 = GB_search(gb_quality, "number_of_no_helix", GB_INT);
		noh = GB_read_int(gb_result3);
		if (noh < 900) result = 3;
		else {
	  	    if (noh < 1100) result = 2;
		    else { result = 1;}
		}
		result = result * weight_helix;
		value += result;

 		GBDATA *gb_result4 = GB_search(gb_quality, "consensus_evaluated", GB_INT);
 		cos = GB_read_int(gb_result4);
		result = cos;
		result = result * weight_consensus;
		value += result;

		GBDATA *gb_result5 = GB_search(gb_quality, "iupac_value", GB_INT);
		iupv = GB_read_int(gb_result5);
		result = iupv;
		result = result * weight_iupac;
		value += result;

		GBDATA *gb_result6 = GB_search(gb_quality, "GC_proportion", GB_INT);
		gcprop = GB_read_int(gb_result6);
		if (gcprop < 150) result = 1;
		else {
	  	    if (gcprop < 190) result = 2;
		    else { result = 3;}
		}
		result = result * weight_gc;
		value += result;

		/*write the final value of the evaluation*/
		if (value !=0 )	value = value / 6;
		GBDATA *gb_result7 = GB_search(gb_quality, "evaluation", GB_INT);
		seq_assert(gb_result7);
		GB_write_int(gb_result7, value);
		printf("value: %i\n",value);

	    }
	}
    }
    free(alignment_name);

    if (error) GB_abort_transaction(gb_main);
    else GB_pop_transaction(gb_main);

    return error;
}


GB_ERROR SQ_pass1(SQ_GroupData* globalData, GBDATA *gb_main, GBT_TREE* node) {

    char *alignment_name;

    GBDATA *read_sequence = 0;
    GBDATA *gb_species;
    GBDATA *gb_species_data;
    GBDATA *gb_name;
    GB_ERROR error = 0;


    GB_push_transaction(gb_main);
    gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    alignment_name = GBT_get_default_alignment(gb_main);
    seq_assert(alignment_name);
    gb_species = node->gb_node;
    gb_name = GB_find(gb_species, "name", 0, down_level);


    if (!gb_name) error = GB_get_error();

    else {

	GBDATA *gb_ali = GB_find(gb_species,alignment_name,0,down_level);

	if (!gb_ali) {
	    error = no_data_error(gb_species, alignment_name);
	}
	else {
	    GBDATA *gb_quality = GB_search(gb_ali, "quality", GB_CREATE_CONTAINER);

	    if (!gb_quality){
		error = GB_get_error();
	    }
	    read_sequence = GB_find(gb_ali,"data",0,down_level);

	    /*real calculations start here*/
	    if (read_sequence) {
		    int sequenceLength = 0;
		    const char *rawSequence = 0;

		    rawSequence    = GB_read_char_pntr(read_sequence);
		    sequenceLength = GB_read_count(read_sequence);


		    /*calculate physical layout of sequence*/
		    SQ_physical_layout* ps_chan = new SQ_physical_layout();
		    ps_chan->SQ_calc_physical_layout(rawSequence, sequenceLength, gb_quality);

		    /*calculate the average number of bases in group*/
		    globalData->SQ_count_sequences();
		    globalData->SQ_set_avg_bases(ps_chan->SQ_get_number_of_bases());
		    delete ps_chan;

		    /*get values for  ambiguities*/
		    SQ_ambiguities* ambi_chan = new SQ_ambiguities();
		    ambi_chan->SQ_count_ambiguities(rawSequence, sequenceLength, gb_quality);
		    delete ambi_chan;

		    /*claculate the number of strong, weak and no helixes*/
		    SQ_helix* heli_chan = new SQ_helix(sequenceLength);
		    heli_chan->SQ_calc_helix_layout(rawSequence, gb_main, alignment_name, gb_quality);
		    delete heli_chan;

		    /*calculate consensus sequence*/
		    {
                        if (!globalData->SQ_is_initialized()) {
			    globalData->SQ_init_consensus(sequenceLength);
                        }
                        globalData->SQ_add_sequence(rawSequence);
		    }


		}
	    }
    }

    free(alignment_name);

    if (error) GB_abort_transaction(gb_main);
    else GB_pop_transaction(gb_main);

    return error;
}


GB_ERROR SQ_pass1_no_tree(SQ_GroupData* globalData, GBDATA *gb_main) {


    char *alignment_name;

    int avg_bases           = 0;
    int worked_on_sequences = 0;

    GBDATA *read_sequence = 0;
    GBDATA *gb_species;
    GBDATA *gb_species_data;
    GBDATA *gb_name;
    GBDATA *(*getFirst)(GBDATA*) = 0;
    GBDATA *(*getNext)(GBDATA*)  = 0;
    GB_ERROR error = 0;



    GB_push_transaction(gb_main);
    gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    alignment_name = GBT_get_default_alignment(gb_main);
    seq_assert(alignment_name);


    getFirst = GBT_first_species;
    getNext  = GBT_next_species;


    /*first pass operations*/
    for (gb_species = getFirst(gb_main);
	 gb_species && !error;
	 gb_species = getNext(gb_species) ){

	gb_name = GB_find(gb_species, "name", 0, down_level);

        if (!gb_name) error = GB_get_error();

	else {

	    GBDATA *gb_ali = GB_find(gb_species,alignment_name,0,down_level);

	    if (!gb_ali) {
		error = no_data_error(gb_species, alignment_name);
	    }
	    else {
		GBDATA *gb_quality = GB_search(gb_ali, "quality", GB_CREATE_CONTAINER);
		if (!gb_quality){
		    error = GB_get_error();
		}
		read_sequence = GB_find(gb_ali,"data",0,down_level);

		/*real calculations start here*/
		if (read_sequence) {
		    int i = 0;
		    int sequenceLength = 0;
		    const char *rawSequence = 0;

		    rawSequence    = GB_read_char_pntr(read_sequence);
		    sequenceLength = GB_read_count(read_sequence);


		    /*calculate physical layout of sequence*/
		    SQ_physical_layout* ps_chan = new SQ_physical_layout();
		    ps_chan->SQ_calc_physical_layout(rawSequence, sequenceLength, gb_quality);
		    i = ps_chan->SQ_get_number_of_bases();
		    avg_bases = avg_bases + i;
		    worked_on_sequences++;
		    delete ps_chan;

		    /*get values for  ambiguities*/
		    SQ_ambiguities* ambi_chan = new SQ_ambiguities();
		    ambi_chan->SQ_count_ambiguities(rawSequence, sequenceLength, gb_quality);
		    delete ambi_chan;

		    /*claculate the number of strong, weak and no helixes*/
		    SQ_helix* heli_chan = new SQ_helix(sequenceLength);
		    heli_chan->SQ_calc_helix_layout(rawSequence, gb_main, alignment_name, gb_quality);
		    delete heli_chan;

		    /*calculate consensus sequence*/
		    {
                        if (!globalData->SQ_is_initialized()) {
			    globalData->SQ_init_consensus(sequenceLength);
                        }
                        globalData->SQ_add_sequence(rawSequence);
		    }
		    pass1_counter_notree++;
		    aw_status(double(globalcounter_notree)/pass1_counter_notree);

		}
	    }
	}
    }
    /*calculate the average number of bases in group*/
    if (worked_on_sequences != 0) {
	avg_bases = avg_bases / worked_on_sequences;
    }
    globalData->SQ_set_avg_bases(avg_bases);
    free(alignment_name);

    if (error) GB_abort_transaction(gb_main);
    else GB_pop_transaction(gb_main);

    return error;
}


GB_ERROR SQ_pass2(SQ_GroupData* globalData, GBDATA *gb_main, GBT_TREE *node) {


    char *alignment_name;

    GBDATA *read_sequence = 0;
    GBDATA *gb_species;
    GBDATA *gb_species_data;
    GBDATA *gb_name;
    GB_ERROR error = 0;


    GB_push_transaction(gb_main);
    gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    alignment_name = GBT_get_default_alignment(gb_main);
    seq_assert(alignment_name);
    gb_species = node->gb_node;
    gb_name = GB_find(gb_species, "name", 0, down_level);

    if (!gb_name) error = GB_get_error();

    else {

	    GBDATA *gb_ali = GB_find(gb_species,alignment_name,0,down_level);

	    if (!gb_ali) {
		error = no_data_error(gb_species, alignment_name);
	    }
	    else {
		GBDATA *gb_quality = GB_search(gb_ali, "quality", GB_CREATE_CONTAINER);
		if (!gb_quality) error = GB_get_error();
		read_sequence = GB_find(gb_ali,"data",0,down_level);

		/*real calculations start here*/
		if (read_sequence) {
		    int sequenceLength      = 0;
		    char temp[10];
		    char cons_dev[1000]  = "<dev>";
		    char cons_conf[1000] = "<conf>";
		    const char *rawSequence = 0;
		    double value1           = 0;
		    double value2           = 0;
		    int evaluation          = 0;
		    int bases               = 0;
		    int avg_bases           = 0;
		    int diff                = 0;
		    int diff_percent        = 0;

		    rawSequence    = GB_read_char_pntr(read_sequence);
		    sequenceLength = GB_read_count(read_sequence);

		    /*
		      calculate the average number of bases in group, and the difference of
		      a single seqeunce in group from it
		    */
		    GBDATA *gb_quality = GB_search(gb_ali, "quality", GB_FIND);
		    GBDATA *gb_result1 = GB_search(gb_quality, "number_of_bases", GB_INT);
		    bases = GB_read_int(gb_result1);
		    avg_bases = globalData->SQ_get_avg_bases();
		    //printf("\n%i avg :",avg_bases);

		    if (avg_bases !=0) {
			diff = bases - avg_bases;
			diff_percent = (100*diff) / avg_bases;
		    }

		    GBDATA *gb_result2 = GB_search(gb_quality, "diff_from_average", GB_INT);
		    seq_assert(gb_result2);
		    GB_write_int(gb_result2, diff_percent);


		    /* get groupnames of visited groups
		       search for name in group dictionary
		       evaluate sequence with group consensus */

		    GBT_TREE *backup = node; //needed?
		    while (backup->father) {
			if(backup->name) {
			    SQ_GroupDataDictionary::iterator GDI = group_dict.find(backup->name);
			    if( GDI != group_dict.end() ) {
			        SQ_GroupDataPtr GD_ptr = GDI->second;
				value1 = GD_ptr->SQ_calc_consensus_conformity(rawSequence);
				value2 = GD_ptr->SQ_calc_consensus_deviation(rawSequence);

				strcat(cons_conf, "<");
				strcat(cons_conf, backup->name);
				strcat(cons_conf, ":");
				sprintf(temp,"%f",value1);
				strcat(cons_conf, temp);
				strcat(cons_conf, ">");

				strcat(cons_dev, "<");
				strcat(cons_dev, backup->name);
				strcat(cons_dev, ":");
				sprintf(temp,"%f",value2);
				strcat(cons_dev, temp);
				strcat(cons_dev, ">");
			    }
			}
			backup = backup->father;
		    }
		    strcat(cons_conf, "</conf>"); //eof character
		    strcat(cons_dev, "</dev>");

		    GBDATA *gb_result3 = GB_search(gb_quality, "consensus_conformity", GB_STRING);
		    seq_assert(gb_result3);
		    GB_write_string(gb_result3, cons_conf);
		    GBDATA *gb_result4 = GB_search(gb_quality, "consensus_deviation", GB_STRING);
		    seq_assert(gb_result4);
		    GB_write_string(gb_result4, cons_dev);

		    //if you parse the upper two values in the evaluate() function cut the following out
		    //for time reasons i do the evaluation here, as i still have the upper two values
		    //---------cut this-----------
		    if (value1 < 0.01) evaluation += 1;
		    else {
		      if (value1 < 0.02) evaluation += 2;
		      else { evaluation += 3;}
		    }
		    if (value2 < 0.0001) evaluation += 1;
		    else {
		      if (value2 < 0.001) evaluation += 2;
		      else { evaluation += 3;}
		    }
		    evaluation = evaluation / 2;
		    GBDATA *gb_result5 = GB_search(gb_quality, "consensus_evaluated", GB_INT);
		    seq_assert(gb_result5);
		    GB_write_int(gb_result5, evaluation);
		    //---------end cut this-------

		}
	    }
    }

    free(alignment_name);

    if (error) GB_abort_transaction(gb_main);
    else GB_pop_transaction(gb_main);

    return error;
}


GB_ERROR SQ_pass2_no_tree(SQ_GroupData* globalData, GBDATA *gb_main) {


    char *alignment_name;

    GBDATA *read_sequence = 0;
    GBDATA *gb_species;
    GBDATA *gb_species_data;
    GBDATA *gb_name;
    GBDATA *(*getFirst)(GBDATA*) = 0;
    GBDATA *(*getNext)(GBDATA*) = 0;
    GB_ERROR error = 0;


    GB_push_transaction(gb_main);
    gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    alignment_name = GBT_get_default_alignment(gb_main);
    seq_assert(alignment_name);
    getFirst = GBT_first_species;
    getNext  = GBT_next_species;


    /*second pass operations*/
    for (gb_species = getFirst(gb_main);
	 gb_species && !error;
	 gb_species = getNext(gb_species) ){

	gb_name = GB_find(gb_species, "name", 0, down_level);

        if (!gb_name) error = GB_get_error();

	else {

	    GBDATA *gb_ali = GB_find(gb_species,alignment_name,0,down_level);

	    if (!gb_ali) {
		error = no_data_error(gb_species, alignment_name);
	    }
	    else {
		GBDATA *gb_quality = GB_search(gb_ali, "quality", GB_CREATE_CONTAINER);
		if (!gb_quality) error = GB_get_error();
		read_sequence = GB_find(gb_ali,"data",0,down_level);

		/*real calculations start here*/
		if (read_sequence) {
		    int sequenceLength      = 0;
		    const char *rawSequence = 0;
		    double value            = 0;
		    int bases               = 0;
		    int avg_bases           = 0;
		    int diff                = 0;
		    int diff_percent        = 0;

		    rawSequence    = GB_read_char_pntr(read_sequence);
		    sequenceLength = GB_read_count(read_sequence);

		    /*
		      calculate the average number of bases in group, and the difference of
		      a single seqeunce in group from it
		    */
		    GBDATA *gb_quality = GB_search(gb_ali, "quality", GB_FIND);
		    GBDATA *gb_result1 = GB_search(gb_quality, "number_of_bases", GB_INT);
		    bases = GB_read_int(gb_result1);
		    avg_bases = globalData->SQ_get_avg_bases();

		    if (avg_bases != 0) {
			diff = avg_bases - bases;
			diff_percent = (100*diff) / avg_bases;
		    }

		    GBDATA *gb_result2 = GB_search(gb_quality, "diff_from_average", GB_INT);
		    seq_assert(gb_result2);
		    GB_write_int(gb_result2, diff_percent);
		    //not useful without tree -> new function has to be made
		    value = globalData->SQ_calc_consensus_deviation(rawSequence);
		    pass2_counter_notree++;
		    aw_status(double(globalcounter_notree)/pass2_counter_notree);
		}
	    }
	}
    }
    free(alignment_name);

    if (error) GB_abort_transaction(gb_main);
    else GB_pop_transaction(gb_main);

    return error;
}


int SQ_count_nr_of_groups(GBT_TREE *node) {
    // counts number of named groups in subtree

    if (node->is_leaf) return 0;

    return
        (node->name != 0) +
        SQ_count_nr_of_groups(node->leftson) +
        SQ_count_nr_of_groups(node->rightson);
}


GB_ERROR SQ_count_nr_of_species(GBDATA *gb_main) {


    char *alignment_name;

    GBDATA *read_sequence = 0;
    GBDATA *gb_species;
    GBDATA *gb_species_data;
    GBDATA *gb_name;
    GBDATA *(*getFirst)(GBDATA*) = 0;
    GBDATA *(*getNext)(GBDATA*) = 0;
    GB_ERROR error = 0;


    GB_push_transaction(gb_main);
    gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    alignment_name = GBT_get_default_alignment(gb_main);
    seq_assert(alignment_name);
    getFirst = GBT_first_species;
    getNext  = GBT_next_species;


    /*second pass operations*/
    for (gb_species = getFirst(gb_main);
	 gb_species && !error;
	 gb_species = getNext(gb_species) ){

	gb_name = GB_find(gb_species, "name", 0, down_level);

        if (!gb_name) error = GB_get_error();

	else {

	    GBDATA *gb_ali = GB_find(gb_species,alignment_name,0,down_level);

	    if (!gb_ali) {
		error = no_data_error(gb_species, alignment_name);
	    }
	    else {
		GBDATA *gb_quality = GB_search(gb_ali, "quality", GB_CREATE_CONTAINER);
		if (!gb_quality) error = GB_get_error();
		read_sequence = GB_find(gb_ali,"data",0,down_level);

		if (read_sequence) {
		    globalcounter_notree++;
		}
	    }
	}
    }
    free(alignment_name);

    if (error) GB_abort_transaction(gb_main);
    else GB_pop_transaction(gb_main);

    return error;
}


void SQ_reset_counters(GBT_TREE *root) {
    globalcounter = 0;
    groupcounter  = SQ_count_nr_of_groups(root);
}


void create_multi_level_consensus(GBT_TREE *node, SQ_GroupData *data) {
    SQ_GroupData *newData  = data->clone();  //save actual consensus
    *newData=*data;
    group_dict[node->name] = newData;        //and link it with an name
}


void SQ_calc_and_apply_group_data(GBT_TREE *node, GBDATA *gb_main, SQ_GroupData *data) {

    if (node->is_leaf){
	if (node->gb_node) {
	    SQ_pass1(data, gb_main, node);
	}
    }

    else {
	GBT_TREE *node1 = node->leftson;
	GBT_TREE *node2 = node->rightson;

	SQ_calc_and_apply_group_data(node1, gb_main, data); //Note: data is not used yet!
	SQ_GroupData *rightData = data->clone();
	SQ_calc_and_apply_group_data(node2, gb_main, rightData);
	data->SQ_add(*rightData);
	delete rightData;

	if (node->name) {        //  group identified
	    create_multi_level_consensus(node, data);
	    globalcounter++;
	    aw_status(double(globalcounter)/groupcounter);
	}
    }
}


void SQ_calc_and_apply_group_data2(GBT_TREE *node, GBDATA *gb_main, SQ_GroupData *data) {

    if (node->is_leaf){
	if (node->gb_node) {
	    SQ_pass2(data, gb_main, node);
	}
    }

    else {
	GBT_TREE *node1 = node->leftson;
	GBT_TREE *node2 = node->rightson;

	if (node1) {
	    SQ_calc_and_apply_group_data2(node1, gb_main, data);
	}
	if (node2) {
	    SQ_calc_and_apply_group_data2(node2, gb_main, data);
	}
	if (node->name) {      //  group identified
	    globalcounter++;
	    aw_status(double(globalcounter)/groupcounter);
	}
    }
}
