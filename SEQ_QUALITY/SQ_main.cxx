#include "arbdb.h"
#include "arbdbt.h"

#include "seq_quality.h"
#include "SQ_functions.h"

#include <stdio.h>


GB_ERROR SQ_calc_seq_quality(GBDATA *gb_main, const char *tree_name) {
    

    int percent = SQ_calc_sequence_diff(gb_main); 
    //debug
    tree_name = 0;

    GB_ERROR error = 0;
    error = GB_export_error("Difference of sequence from average in percent: %i ", percent);
    return error;


}
