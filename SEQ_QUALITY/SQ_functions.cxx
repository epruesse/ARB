#include "arbdb.h"
#include "arbdbt.h"
#include "stdio.h"



char *  SQ_get_raw_sequence(GBDATA *gb_main) {


    char *alignment_name;
    char *rawSequence = 0;
    GBDATA *read_sequence = 0;
    GBDATA *gb_species;
    GBDATA *gb_species_data;


    GB_push_transaction(gb_main);


    gb_species_data = GB_search(gb_main,"species_data",GB_FIND);
    gb_species = GBT_first_species_rel_species_data(gb_species_data);
    alignment_name = GBT_get_default_alignment(gb_main);


    if (gb_species) {
        read_sequence = GBT_read_sequence(gb_species, alignment_name);
    
	if (read_sequence) {
	    rawSequence = GB_read_string(read_sequence);
	}
    }


    return rawSequence;

} 
