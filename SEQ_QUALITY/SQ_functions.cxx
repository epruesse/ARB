#include "arbdb.h"
#include "arbdbt.h"
#include "stdio.h"
#include "stdlib.h"

#undef _USE_AW_WINDOW
#include "BI_helix.hxx"

#include "SQ_consensus_marked.h"

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define seq_assert(bed) arb_assert(bed)

enum { CS_CLEAR, CS_PASS1 };

GB_ERROR SQ_reset_quality_calcstate(GBDATA *gb_main) {
    GB_push_transaction(gb_main);

    GB_ERROR  error          = 0;
    char     *alignment_name = GBT_get_default_alignment(gb_main);

    for (GBDATA *gb_species = GBT_first_species(gb_main);
         gb_species && !error;
         gb_species = GBT_next_species(gb_species))
    {
        GBDATA *gb_ali     = GB_find(gb_species,alignment_name,0,down_level);
        if (!gb_ali) error = GBS_global_string("No such alignment '%s'", alignment_name);
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

    if (error) GB_abort_transaction(gb_main);
    else GB_pop_transaction(gb_main);

    return error;
}

void SQ_calc_sequence_structure(GBDATA *gb_main, bool marked_only) {


    char *alignment_name;
    int avg_bases = 0;
    int worked_on_sequences = 0;

    GBDATA *read_sequence = 0;
    GBDATA *gb_species;
    GBDATA *gb_species_data;
    GBDATA *gb_name;
    GBDATA *(*getFirst)(GBDATA*) = 0;
    GBDATA *(*getNext)(GBDATA*) = 0;

    BI_PAIR_TYPE pair_type = HELIX_PAIR;
    BI_helix my_helix;


    GB_push_transaction(gb_main);
    gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    alignment_name = GBT_get_default_alignment(gb_main);
    seq_assert(alignment_name);
    my_helix.init(gb_main, alignment_name);
    SQ_consensus_marked cons_mkd;

    if (marked_only) {
	getFirst = GBT_first_marked_species;
	getNext  = GBT_next_marked_species;
    }
    else {
	getFirst = GBT_first_species;
	getNext  = GBT_next_species;
	//SQ_consensus_marked cons_mmkd;//debug : another object for unmarked
    }


    /*first pass operations*/
    for (gb_species = getFirst(gb_main); gb_species; gb_species = getNext(gb_species) ){

	gb_name = GB_find(gb_species, "name", 0, down_level);

	if (gb_name) {

	    GBDATA *gb_ali = GB_find(gb_species,alignment_name,0,down_level);

	    if (gb_ali) {

		GBDATA *gb_quality = GB_search(gb_ali, "quality", GB_CREATE_CONTAINER);
		read_sequence      = GB_find(gb_ali,"data",0,down_level);

		if (read_sequence) {

		    int count_bases = 0;
		    int count_scores = 0;
		    int count_dots = 0;
		    int percent_scores = 0;
		    int percent_dots = 0;
		    int percent_bases = 0;
		    int sequenceLength = 0;

		    int j = 0;
		    int count_helix = 0;
		    int count_weak_helix = 0;
		    int count_no_helix = 0;
		    int temp = 0;
		    char left;
		    char right;

		    const char *rawSequence = 0;

		    rawSequence    = GB_read_char_pntr(read_sequence);
		    sequenceLength = GB_read_count(read_sequence);
		    count_bases    = sequenceLength;

		    for (int i = 0; i < sequenceLength; i++) {

			/*claculate physical layout of sequence*/
			if (rawSequence[i] == '-') {
			    count_bases--;
			    count_scores++;
			}

			if (rawSequence[i] == '.') {
			    count_bases--;
			    count_dots++;
			}

			/*claculate the number of strong, weak and no helixes*/
			pair_type = my_helix.entries[i].pair_type;
			if (pair_type == HELIX_PAIR) {
			    left = rawSequence[i];
			    j = my_helix.entries[i].pair_pos;
			    right = rawSequence[j];
			    temp = my_helix.check_pair(left, right, pair_type);

			    switch(temp){
				case 2:
				    count_helix++;
				    break;
				case 1:
				    count_weak_helix++;
				    break;
				case 0:
				    count_no_helix++;
				    break;
			    }
			}

			/*calculate consensus*/
			cons_mkd.SQ_calc_consensus(rawSequence[i], i);
			//char c = cons_mkd.SQ_get_consensus(i);
			//printf("%c",c);
		    }


		    /*calculate the average number of bases in group*/
		    avg_bases = avg_bases + count_bases;
		    worked_on_sequences++;


		    /*calculate layout in percent*/
		    percent_scores = (100 * count_scores) / sequenceLength;
		    percent_dots   = (100 * count_dots) / sequenceLength;
		    percent_bases  = 100 - (percent_scores + percent_dots);


		    GBDATA *gb_result1 = GB_search(gb_quality, "number_of_helix", GB_INT);
		    seq_assert(gb_result1);
		    GB_write_int(gb_result1, count_helix);

		    GBDATA *gb_result2 = GB_search(gb_quality, "number_of_weak_helix", GB_INT);
		    seq_assert(gb_result2);
		    GB_write_int(gb_result2, count_weak_helix);

		    GBDATA *gb_result3 = GB_search(gb_quality, "number_of_no_helix", GB_INT);
		    seq_assert(gb_result3);
		    GB_write_int(gb_result3, count_no_helix);

		    GBDATA *gb_result4 = GB_search(gb_quality, "number_of_bases", GB_INT);
		    seq_assert(gb_result4);
		    GB_write_int(gb_result4, count_bases);

		    GBDATA *gb_result5 = GB_search(gb_quality, "percent_of_bases", GB_INT);
		    seq_assert(gb_result5);
		    GB_write_int(gb_result5, percent_bases);

		}
	    }
	}
    }

    /*calculate the average number of bases in group*/
    if (worked_on_sequences != 0) {
	avg_bases = avg_bases / worked_on_sequences;
	printf("%i",avg_bases);
    }
    //debug
    else {
	worked_on_sequences = 1;
    }


    /*second pass operations*/
    for (gb_species = getFirst(gb_main); gb_species; gb_species = getNext(gb_species) ){

	gb_name = GB_find(gb_species, "name", 0, down_level);

	if (gb_name) {

	    GBDATA *gb_ali = GB_find(gb_species,alignment_name,0,down_level);

	    if (gb_ali) {
		int diff = 0;
		int temp = 0;
		int diff_percent = 0;

		/*
		  calculate the average number of bases in group, and the difference of
		  a single seqeunce in group from it
		*/
		GBDATA *gb_quality = GB_search(gb_ali, "quality", GB_FIND);
		GBDATA *gb_result1 = GB_search(gb_quality, "number_of_bases", GB_INT);

		temp = GB_read_int(gb_result1);
		diff = avg_bases - temp;
		diff_percent = (100*diff) / avg_bases;

		GBDATA *gb_result2 = GB_search(gb_quality, "diff_from_average", GB_INT);
		seq_assert(gb_result2);
		GB_write_int(gb_result2, diff_percent);


		/*calculate consensus conformity*/
		if (read_sequence) {
		    int sequenceLength      = 0;
		    const char *rawSequence = 0;
		    int cons_conf           = 0;
		    int cons_conf_percent   = 0;
		    rawSequence    = GB_read_char_pntr(read_sequence);
		    sequenceLength = GB_read_count(read_sequence);


		    for (int i = 0; i < sequenceLength; i++) {
			char c;
			c = cons_mkd.SQ_get_consensus(i);
			if (rawSequence[i] == c) {
			    cons_conf++;
			}
		    }
		    cons_conf_percent = (cons_conf * 100) / sequenceLength;

		    GBDATA *gb_result6 = GB_search(gb_quality, "consensus_conformity", GB_INT);
		    seq_assert(gb_result6);
		    GB_write_int(gb_result6, cons_conf_percent);
		}


	    }
	}
    }

    GB_pop_transaction(gb_main);

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

    GB_pop_transaction(gb_main);
    return result;

}



void SQ_evaluate(GBDATA *gb_main, int weight_bases, int weight_diff_from_average, int weight_helix, int weight_consensus){


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
		int bases      = 0;
		int dfa        = 0;
		int result     = 0;
		int nh         = 0;
		int cc         = 0;

		GBDATA *gb_quality = GB_search(gb_ali, "quality", GB_FIND);

		GBDATA *gb_result1 = GB_search(gb_quality, "percent_of_bases", GB_INT);
		bases = GB_read_int(gb_result1);
		GBDATA *gb_result2 = GB_search(gb_quality, "diff_from_average", GB_INT);
		dfa = GB_read_int(gb_result2);
		GBDATA *gb_result3 = GB_search(gb_quality, "number_of_no_helix", GB_INT);
		nh = GB_read_int(gb_result3);
		GBDATA *gb_result4 = GB_search(gb_quality, "consensus_conformity", GB_INT);
		cc = GB_read_int(gb_result4);

		/*this is the equation for the evaluation of the stored values*/
		printf("\n debug info:%i %i %i %i \n", bases, dfa, nh, cc );
		result = (weight_bases * bases) - (weight_diff_from_average * dfa) - (weight_helix * nh) + (weight_consensus * cc);
		result = 2 * result / 45;

		/*write the final value of the evaluation*/
		GBDATA *gb_result5 = GB_search(gb_quality, "value_of_evaluation", GB_INT);
		seq_assert(gb_result5);
		GB_write_int(gb_result5, result);

	    }
	}
    }

// #if defined(DEVEL_JUERGEN)
//     GB_save_as(gb_main, path, savetype);
// #endif // DEVEL_JUERGEN

    GB_pop_transaction(gb_main);

}



void SQ_traverse_through_tree(GBDATA *gb_main, GBT_TREE *node, bool marked_only){

    GBT_TREE *parent = 0;


    if (node->gb_node){
	if (node->is_leaf==true && (!(node->gb_node==0))){
	    parent = node->father;
	    if (parent->name){
		//gb_main = parent->gb_node;
		//gb_main = GB_find(parent->gb_node,"name",0,down_level);
		SQ_calc_sequence_structure(gb_main, marked_only);
	    }
	}
	else {
	    parent = node;
	    if(node = parent->rightson){
		SQ_traverse_through_tree(gb_main, node, marked_only);
	    }
	    if(node = parent->leftson){
		SQ_traverse_through_tree(gb_main, node, marked_only);
	    }
	}
    }
}
