#include "arbdb.h"
#include "arbdbt.h"

#include "seq_quality.h"
#include "SQ_functions.h"

#include <stdio.h>


GB_ERROR SQ_calc_seq_quality(GBDATA *gb_main, const char *tree_name) {
    

    int quality = SQ_get_raw_sequence(gb_main); 
    //debug
    tree_name = 0;

    GB_ERROR error = 0;
    error = GB_export_error("Alignment quality: %i ", quality);
    return error;


}
