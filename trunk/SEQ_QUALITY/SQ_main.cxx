#include "arbdb.h"
#include "arbdbt.h"

#include "seq_quality.h"
#include "SQ_functions.h"

#include <stdio.h>


GB_ERROR SQ_calc_seq_quality(GBDATA *gb_main, const char *tree_name) {
    

    SQ_calc_sequence_structure(gb_main);
    int spaces = SQ_calc_average_structure(gb_main);
    //debug
    tree_name = 0;

    GB_ERROR error = 0;
    error = GB_export_error("Value in container: %i ", spaces);
    return error;


}
