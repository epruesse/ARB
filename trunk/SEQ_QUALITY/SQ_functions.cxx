#include "arbdb.h"
#include "arbdbt.h"
#include "stdio.h"



int SQ_calc_sequence_diff(GBDATA *gb_main) {


    int j = 0;
    char *alignment_name;
    const char *rawSequence = 0;
    int result = 0;
    int avr = 0; //average length of Sequence
    char working_on_sequence[100000];
    int sequenceLength = 0;
    int sequenceQuality;
    int qualities[100][3];
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
		sequenceQuality = sequenceLength;
		strcpy (working_on_sequence, rawSequence);

		for (int i = 0; i < sequenceLength; i++) {
		    if (working_on_sequence[i] == '-') {
			sequenceQuality--;
		    }
		    if (working_on_sequence[i] == '.') {
			sequenceQuality--;
		    }
		}

		qualities[j][0] = sequenceQuality;
		j++;

	    }
	}

    }


    //calculate average

    for (int i = 0; i < j; i++) {
	avr = avr + qualities[i][0];
    }

    avr = avr / j;


    //calculate differences

    for (int i = 0; i < j; i++) {
	    qualities[i][1] = avr - qualities[i][0];
    }


    //calculate difference from average in %

    for (int i = 0; i < j; i++) {
	    qualities[i][2] = ((100*qualities[i][1]) / avr);
    }


    result = qualities[0][2];
    return result;

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


    return qualities[0][3];

}
