//  ==================================================================== //
//                                                                       //
//    File      : SQ_functions.cxx                                       //
//    Purpose   : Implementation of SQ_functions.h                       //
//    Time-stamp: <Mon Oct/06/2003 10:33 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July - October 2003                        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //

#include "arbdb.h"
#include "arbdbt.h"
#include "stdio.h"
#include "stdlib.h"

#include "SQ_consensus.h"
#include "SQ_ambiguities.h"
#include "SQ_helix.h"
#include "SQ_physical_layout.h"
#include "SQ_GroupData.h"

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define seq_assert(bed) arb_assert(bed)

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
    delete alignment_name;

    if (error) GB_abort_transaction(gb_main);
    else GB_pop_transaction(gb_main);

    return error;
}


GB_ERROR SQ_calc_sequence_structure(SQ_GroupData& globalData, GBDATA *gb_main, bool marked_only) {


    char *alignment_name;
    int avg_bases = 0;
    int worked_on_sequences = 0;
    bool pass1_free = true; //semaphor

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


    if (marked_only) {
	getFirst = GBT_first_marked_species;
	getNext  = GBT_next_marked_species;
    }
    else {
	getFirst = GBT_first_species;
	getNext  = GBT_next_species;
    }


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
		if (!gb_quality) error = GB_get_error();

		/*look if first pass wasn't done already*/
		else {
		    GBDATA *gb_calcstate     = GB_search(gb_quality, "calcstate", GB_INT);
		    if (!gb_calcstate) error = GB_get_error();
		    else {
			int sema = GB_read_int(gb_calcstate); // get calculation state;
			if (sema != 1) pass1_free = true;
			else {
			    pass1_free = false;
			}
		    }
		}

		read_sequence = GB_find(gb_ali,"data",0,down_level);

		/*real calculations start here*/
		if (read_sequence && pass1_free) {
		    int i = 0;
		    int sequenceLength = 0;
		    const char *rawSequence = 0;

		    rawSequence    = GB_read_char_pntr(read_sequence);
		    sequenceLength = GB_read_count(read_sequence);


		    /*calculate physical layout of sequence*/
		    SQ_physical_layout ps_chan;
		    ps_chan.SQ_calc_physical_layout(rawSequence, sequenceLength, gb_quality);
		    i = ps_chan.SQ_get_number_of_bases();
		    avg_bases = avg_bases + i;

		    /*get values for  ambiguities*/
		    SQ_ambiguities ambi_chan;
		    ambi_chan.SQ_count_ambiguities(rawSequence, sequenceLength, gb_quality);

		    /*claculate the number of strong, weak and no helixes*/
		    SQ_helix heli_chan(sequenceLength);
		    heli_chan.SQ_calc_helix_layout(rawSequence, gb_main, alignment_name, gb_quality);

		    /*calculate consensus sequence*/
		    {
			bool init;
			int **consensus;
			init = globalData.SQ_is_initialised();
			if (init==false){
			    globalData.SQ_init_consensus(sequenceLength);
			}
			SQ_consensus consens(sequenceLength);
			consens.SQ_calc_consensus(rawSequence);
			consensus = new int *[sequenceLength];
			for (int i=0; i < sequenceLength; i++ ){
			    consensus[i] = new int [7];
			}
 			int *pp;
			for(int i = 0; i < sequenceLength; i++) {
			    for(int j = 0; j < 7; j++) {
				pp = consens.SQ_get_consensus(i,j);
				consensus[i][j] = *pp;
			    }
			}
			for(int i = 0; i < sequenceLength; i++) {
			    for(int j = 0; j < 7; j++) {
				globalData.SQ_add_consensus(consensus[i][j],i,j);
			    }
			}
			delete [] consensus;
		    }



		    worked_on_sequences++;

		    /*set first pass done*/
		    GBDATA *gb_calcstate     = GB_search(gb_quality, "calcstate", GB_INT);
		    if (!gb_calcstate) error = GB_get_error();
		    else {
			GB_write_int(gb_calcstate, CS_PASS1); // set calculation state
		    }
		}
	    }
	}
    }


    /*calculate the average number of bases in group*/
    if (worked_on_sequences != 0) {
	avg_bases = avg_bases / worked_on_sequences;
    }
    //debug
    else {
	worked_on_sequences = 1;
    }



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

	    }
	}
    }
    delete alignment_name;

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
    else {

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
    delete alignment_name;

    GB_pop_transaction(gb_main);
    return result;

}



GB_ERROR SQ_evaluate(GBDATA *gb_main, int weight_bases, int weight_diff_from_average, int weight_helix, int weight_consensus, int weight_iupac){


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
    else {

    }


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
		int coc        = 0;
		int iupv       = 0;
		int gcprop     = 0;

		GBDATA *gb_quality = GB_search(gb_ali, "quality", GB_FIND);

		GBDATA *gb_result1 = GB_search(gb_quality, "percent_of_bases", GB_INT);
		bases = GB_read_int(gb_result1);
		GBDATA *gb_result2 = GB_search(gb_quality, "diff_from_average", GB_INT);
		dfa = GB_read_int(gb_result2);
		GBDATA *gb_result3 = GB_search(gb_quality, "number_of_no_helix", GB_INT);
		noh = GB_read_int(gb_result3);
// 		GBDATA *gb_result4 = GB_search(gb_quality, "consensus_conformity", GB_INT);
// 		coc = GB_read_int(gb_result4);
		GBDATA *gb_result5 = GB_search(gb_quality, "iupac_value", GB_INT);
		iupv = GB_read_int(gb_result5);
		GBDATA *gb_result6 = GB_search(gb_quality, "GC_proportion", GB_INT);
		gcprop = GB_read_int(gb_result6);

		/*this is the equation for the evaluation of the stored values*/
		double gc_proportion = gcprop;    // hack->SQ_physical_layout.h
		gc_proportion = 100 / gcprop;
		//printf("\n debug info:%i %i %i %e \n", bases, dfa, noh, gcprop );

		result = (weight_bases * bases) - (weight_diff_from_average * dfa) - (weight_helix * noh) + (weight_consensus * coc) + (weight_iupac * iupv);
		//result = round(result * gc_proportion); // @@@ runden ?

		/*write the final value of the evaluation*/
		GBDATA *gb_result7 = GB_search(gb_quality, "value_of_evaluation", GB_INT);
		seq_assert(gb_result7);
		GB_write_int(gb_result7, result);

	    }
	}
    }
    delete alignment_name;

    if (error) GB_abort_transaction(gb_main);
    else GB_pop_transaction(gb_main);

    return error;
}



GB_ERROR SQ_pass1(SQ_GroupData& globalData, GBDATA *gb_main) {


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
		    SQ_physical_layout ps_chan;
		    ps_chan.SQ_calc_physical_layout(rawSequence, sequenceLength, gb_quality);
		    i = ps_chan.SQ_get_number_of_bases();
		    avg_bases = avg_bases + i;
		    worked_on_sequences++;

		    /*get values for  ambiguities*/
		    SQ_ambiguities ambi_chan;
		    ambi_chan.SQ_count_ambiguities(rawSequence, sequenceLength, gb_quality);

		    /*claculate the number of strong, weak and no helixes*/
		    SQ_helix heli_chan(sequenceLength);
		    heli_chan.SQ_calc_helix_layout(rawSequence, gb_main, alignment_name, gb_quality);

		    /*calculate consensus sequence*/
		    {
			bool init;
			//int **consensus;
			init = globalData.SQ_is_initialised();
			if (init==false){
			    globalData.SQ_init_consensus(sequenceLength);
			}
			SQ_consensus consens(sequenceLength);
			consens.SQ_calc_consensus(rawSequence);
// 			consensus = new int *[sequenceLength];
// 			for (int i=0; i < sequenceLength; i++ ){
// 			    consensus[i] = new int [7];
// 			}
 			int *pp;
// 			for(int i = 0; i < sequenceLength; i++) {
// 			    for(int j = 0; j < 7; j++) {
// 				pp = consens.SQ_get_consensus(i,j);
// 				consensus[i][j] = *pp;
// 			    }
// 			}
			int p;
			for(int i = 0; i < sequenceLength; i++) {
			    for(int j = 0; j < 7; j++) {
 				pp = consens.SQ_get_consensus(i,j);
 				p = *pp;
				globalData.SQ_add_consensus(p,i,j);
			    }
			}
			//delete [] consensus;
		    }
		}
	    }
	}
    }
    /*calculate the average number of bases in group*/
    if (worked_on_sequences != 0) {
	avg_bases = avg_bases / worked_on_sequences;
    }
    globalData.SQ_set_avg_bases(avg_bases);

    delete alignment_name;

    if (error) GB_abort_transaction(gb_main);
    else GB_pop_transaction(gb_main);

    return error;
}



GB_ERROR SQ_pass2(SQ_GroupData& globalData, GBDATA *gb_main, bool marked_only) {


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


    if (marked_only) {
	getFirst = GBT_first_marked_species;
	getNext  = GBT_next_marked_species;
    }
    else {
	getFirst = GBT_first_species;
	getNext  = GBT_next_species;
    }


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
		    int temp                = 0;
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
		    avg_bases = globalData.SQ_get_avg_bases();

		    diff = avg_bases - bases;
		    diff_percent = (100*diff) / avg_bases;

		    GBDATA *gb_result2 = GB_search(gb_quality, "diff_from_average", GB_INT);
		    seq_assert(gb_result2);
		    GB_write_int(gb_result2, diff_percent);


		    value = globalData.SQ_test_against_consensus(rawSequence);
		    //printf("     %f    ",value);
		}
	    }
	}
    }
    delete alignment_name;

    if (error) GB_abort_transaction(gb_main);
    else GB_pop_transaction(gb_main);

    return error;
}












// void SQ_applyGroupData()  {

// }


// SQ_GroupData *SQ_calc_and_apply_group_data(GBT_TREE *node, GBDATA *gb_main) {

//     if (node->is_leaf){
// 	if (!node->gb_node) return 0;

// 	SQ_GroupData *data = new SQ_GroupData();

//         //  ...

// 	return data;
//     }
//     else {
// 	SQ_GroupData *leftData  = SQ_calc_and_apply_group_data(node->leftson, gb_main);
// 	SQ_GroupData *rightData = SQ_calc_and_apply_group_data(node->rightson, gb_main);

// 	if (!leftData) return rightData;
// 	if (!rightData) return leftData;


// 	SQ_GroupData *data = new SQ_GroupData(*leftData, *rightData);
// 	delete leftData;
// 	delete rightData;

// 	if (node->name) { //  gruppe!
// 	    SQ_applyGroupData();
// 	}

// 	return data;
//     }
// }

