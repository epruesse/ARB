#include "arbdb.h"
#include "arbdbt.h"
#include "stdio.h"



int SQ_get_raw_sequence(GBDATA *gb_main) {


    int j = 0;
    int k;
    char *alignment_name;
    char *rawSequence = 0;
    int result = 0;
    int abw = 0; //Standardabweichung
    int avr = 0; //average length of Sequence
    char filename[64];
    char working_on_sequence[10000];
    int sequenceLength = 0;
    int sequenceQuality;
    int qualities[100][1];
    GBDATA *read_sequence = 0;
    GBDATA *gb_species;
    GBDATA *gb_species_data;
    GBDATA *gb_name;
    FILE *file1;


    strcpy (filename, "rawsequence.txt");
    GB_push_transaction(gb_main);


    //qualities init
    for (int i = 0; i <=99; i++) {
	qualities [i][0] = 0;
    }


    gb_species_data = GB_search(gb_main,"species_data",GB_FIND);
    alignment_name = GBT_get_default_alignment(gb_main);



    for (gb_species = GBT_first_marked_species(gb_main); gb_species; gb_species = GBT_next_marked_species(gb_species) ){


        gb_name = GB_find(gb_species, "name", 0, down_level);
	if (gb_name) {

	    read_sequence = GBT_read_sequence(gb_species, alignment_name);

	    if (read_sequence) {

		rawSequence = GB_read_string(read_sequence);

		//debug
		sequenceLength = 100;
		sequenceQuality = 101;
		strcpy (working_on_sequence, rawSequence);

		for (int i = 1; i <= sequenceLength; i++) {
		    if (working_on_sequence[i] == '-') {
			sequenceQuality--;
		    }
		}

		qualities[j][0] = sequenceQuality;
		j++;

	    }
	}

    }
    //calculate average
    k = j;
    for (int i = 0; i <= k-1; i++) {
	avr = avr + qualities[i][0];
	if (qualities[i][0] == 0) {
	    k--;
	}
    }
    avr = avr / k;

    //calculate Standardabweichung
    for (int i = 0; i <= j-1; i++) {
	abw = abw + (avr - qualities[i][0]);
	if (qualities[i][0] == 0) {
	    j--;
	}
    }
    abw = abw / j;

    result = avr;
    return result;

} 
