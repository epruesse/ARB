#include "arbdb.h"
#include "arbdbt.h"
#include "stdio.h"
#include "stdlib.h"

#undef _USE_AW_WINDOW
#include "BI_helix.hxx"

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define seq_assert(bed) arb_assert(bed)



void SQ_calc_sequence_structure(GBDATA *gb_main) {


    char *alignment_name;
    const char *path = "demo.arb";
    const char *savetype = "b";

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

	if (gb_ali) {   //!gb_ali
	  GBDATA *gb_quality = GB_search(gb_ali, "quality", GB_CREATE_CONTAINER);
	  read_sequence = GB_find(gb_ali,"data",0,down_level);

	  if (read_sequence) {
	    int count_bases = 0;
	    int count_scores = 0;
	    int count_dots = 0;
	    int percent_scores = 0;
	    int percent_dots = 0;
	    int percent_bases = 0;
	    int sequenceLength = 0;
	    const char *rawSequence = 0;

	    rawSequence = GB_read_char_pntr(read_sequence);
	    sequenceLength = GB_read_count(read_sequence);
	    count_bases = sequenceLength;

	    //claculate physical layout of sequence
	    for (int i = 0; i < sequenceLength; i++) {
	      if (rawSequence[i] == '-') {
		count_bases--;
		count_scores++;
	      }
	      if (rawSequence[i] == '.') {
		count_bases--;
		count_dots++;
	      }
	    }

	    //calculate layout in percent
	    percent_scores = (100 * count_scores) / sequenceLength;
	    percent_dots = (100 * count_dots) / sequenceLength;
	    percent_bases = 100 - (percent_scores + percent_dots);


	    GBDATA *gb_result1 = GB_search(gb_quality, "number_of_bases", GB_INT);
	    seq_assert(gb_result1);
	    GB_write_int(gb_result1, count_bases);

	    GBDATA *gb_result2 = GB_search(gb_quality, "number_of_scores", GB_INT);
	    seq_assert(gb_result2);
	    GB_write_int(gb_result2, count_scores);

	    GBDATA *gb_result3 = GB_search(gb_quality, "number_of_dots", GB_INT);
	    seq_assert(gb_result3);
	    GB_write_int(gb_result3, count_dots);

	    GBDATA *gb_result4 = GB_search(gb_quality, "percent_of_bases", GB_INT);
	    seq_assert(gb_result4);
	    GB_write_int(gb_result4, percent_bases);

	    GBDATA *gb_result5 = GB_search(gb_quality, "percent_of_scores", GB_INT);
	    seq_assert(gb_result5);
	    GB_write_int(gb_result5, percent_scores);

	    GBDATA *gb_result6 = GB_search(gb_quality, "percent_of_dots", GB_INT);
	    seq_assert(gb_result6);
	    GB_write_int(gb_result6, percent_dots);

	  }
	}
      }
    }

    GB_save_as(gb_main, path, savetype);
    GB_pop_transaction(gb_main);

}



void SQ_calc_average_structure(GBDATA *gb_main) {

    char *alignment_name;
    int avg_bases = 0;
    int j = 0;
    const char *path = "demo.arb";
    const char *savetype = "b";

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
	    GBDATA *gb_result1 = GB_search(gb_quality, "number_of_bases", GB_INT);
	    avg_bases = avg_bases + GB_read_int(gb_result1);
	    j++;

	}
      }
    }
    avg_bases = avg_bases / j ;
    //return avg_bases;

    for (gb_species = getFirst(gb_main); gb_species; gb_species = getNext(gb_species) ){

      gb_name = GB_find(gb_species, "name", 0, down_level);

      if (gb_name) {

	GBDATA *gb_ali = GB_find(gb_species,alignment_name,0,down_level);

	if (gb_ali) {
	    int diff = 0;
	    int temp = 0;
	    int diff_percent = 0;

	    GBDATA *gb_quality = GB_search(gb_ali, "quality", GB_FIND);
	    GBDATA *gb_result1 = GB_search(gb_quality, "number_of_bases", GB_INT);

	    temp = GB_read_int(gb_result1);
	    diff = avg_bases - temp;
	    diff_percent = (100*diff) / avg_bases;

	    GBDATA *gb_result2 = GB_search(gb_quality, "diff_from_average", GB_INT);
	    seq_assert(gb_result2);
	    GB_write_int(gb_result2, diff_percent);

	}
      }
    }

    GB_save_as(gb_main, path, savetype);
    GB_pop_transaction(gb_main);

}



int SQ_calc_insertations(GBDATA *gb_main) {


    int j = 0;
    int temp = 0;
    char *alignment_name;
    const char *rawSequence = 0;
    char working_on_sequence[100000];
    int sequenceLength = 0;
    int spaces = 0;
    int points = 0;
    int qualities[100][5];
    GBDATA *read_sequence = 0;
    GBDATA *gb_species;
    GBDATA *gb_species_data;
    GBDATA *gb_name;

    GB_push_transaction(gb_main);


    //qualities init
    for (int i = 0; i <=99; i++) {
	qualities [i][0] = 0;
	qualities [i][1] = 0;
	qualities [i][2] = 0;
	qualities [i][3] = 0;
	qualities [i][4] = 0;
    }


    gb_species_data = GB_search(gb_main,"species_data",GB_FIND);
    alignment_name = GBT_get_default_alignment(gb_main);



    for (gb_species = GBT_first_marked_species(gb_main); gb_species; gb_species = GBT_next_marked_species(gb_species) ){


        gb_name = GB_find(gb_species, "name", 0, down_level);
	if (gb_name) {

	    read_sequence = GBT_read_sequence(gb_species, alignment_name);

	    if (read_sequence) {

		rawSequence = GB_read_char_pntr(read_sequence);
		sequenceLength = GB_read_count(read_sequence);
		strcpy (working_on_sequence, rawSequence);

		for (int i = 0; i < sequenceLength; i++) {
		    if (working_on_sequence[i] == '-') {
			spaces++;
		    }
		    if (working_on_sequence[i] == '.') {
			points++;
		    }
		}

		qualities[j][0] = sequenceLength - (spaces + points);
		qualities[j][1] = spaces;
		qualities[j][2] = points;
		j++;

	    }
	}

    }


    //calculate insertations in %

    for (int i = 0; i < j; i++) {
	temp = qualities[i][0] + qualities[i][1] + qualities[i][2];
	qualities[i][3] = (100 * qualities[i][1]) / temp; //spaces in %
	qualities[i][4] = (100 * qualities[i][2]) / temp; //points in %
    }

    GB_pop_transaction(gb_main);
    return qualities[0][3];

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



void SQ_evaluate(GBDATA *gb_main, int weight_bases, int weight_diff_from_average){


    char *alignment_name;
    const char *path = "demo.arb";
    const char *savetype = "b";

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
	    int bases = 0;
	    int dfa = 0;
	    int result = 0;

	    GBDATA *gb_quality = GB_search(gb_ali, "quality", GB_FIND);
	    GBDATA *gb_result1 = GB_search(gb_quality, "percent_of_bases", GB_INT);
	    bases = GB_read_int(gb_result1);

	    GBDATA *gb_result2 = GB_search(gb_quality, "diff_from_average", GB_INT);
	    dfa = GB_read_int(gb_result2);
	    result = 100 - ((weight_bases * bases) + (weight_diff_from_average * dfa)) / 100;

	    //write the final value of the evaluation
	    GBDATA *gb_result3 = GB_search(gb_quality, "value_of_evaluation", GB_INT);
	    seq_assert(gb_result3);
	    GB_write_int(gb_result3, result);

	}
      }
    }

    GB_save_as(gb_main, path, savetype);
    GB_pop_transaction(gb_main);

}




void SQ_calc_helix_conformance(GBDATA *gb_main) {


    char *alignment_name;
    const char *path = "demo.arb";
    const char *savetype = "b";

    GBDATA *read_sequence = 0;
    GBDATA *gb_species;
    GBDATA *gb_species_data;
    GBDATA *gb_name;
    GBDATA *(*getFirst)(GBDATA*) = 0;
    GBDATA *(*getNext)(GBDATA*) = 0;

    BI_PAIR_TYPE pair_type = HELIX_PAIR;

    GB_push_transaction(gb_main);
    gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    alignment_name = GBT_get_default_alignment(gb_main);
    seq_assert(alignment_name);
   
    BI_helix my_helix;
    my_helix.init(gb_main, alignment_name);
    


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

	if (gb_ali) {   //!gb_ali
	  GBDATA *gb_quality = GB_search(gb_ali, "quality", GB_CREATE_CONTAINER);
	  read_sequence = GB_find(gb_ali,"data",0,down_level);

	  if (read_sequence) {
	      int count_helix = 0;
	      int count_weak_helix = 0;
	      int count_no_helix = 0;
	      int sequenceLength = 0;
	      int temp = 0;
	      char left;
	      char right;
	      const char *rawSequence = 0;

	      rawSequence = GB_read_char_pntr(read_sequence);
	      sequenceLength = GB_read_count(read_sequence);

	      //claculate physical layout of sequence
	      for (int i = 0; i < sequenceLength; i++) {
		  left = rawSequence[i];
		  i++;
		  right = rawSequence[i];
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

	      GBDATA *gb_result1 = GB_search(gb_quality, "number_of_helix", GB_INT);
	      seq_assert(gb_result1);
	      GB_write_int(gb_result1, count_helix);

	      GBDATA *gb_result2 = GB_search(gb_quality, "number_of_weak_helix", GB_INT);
	      seq_assert(gb_result2);
	      GB_write_int(gb_result2, count_weak_helix);

	      GBDATA *gb_result3 = GB_search(gb_quality, "number_of_no_helix", GB_INT);
	      seq_assert(gb_result3);
	      GB_write_int(gb_result3, count_no_helix);

	  }
	}
      }
    }

    GB_save_as(gb_main, path, savetype);
    GB_pop_transaction(gb_main);

}
