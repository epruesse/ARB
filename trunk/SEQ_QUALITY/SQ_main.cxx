#include "arbdb.h"
#include "arbdbt.h"

#include "seq_quality.h"
#include "SQ_functions.h"

#include <stdio.h>


GB_ERROR SQ_calc_seq_quality(GBDATA *gb_main, const char *tree_name) {
    

    //char *rawSeq = SQ_get_raw_sequence(gb_main); 


    GB_ERROR error = 0;
    error = GB_export_error("%s", tree_name);
    return error;


}
