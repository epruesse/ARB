#include "arbdb.h"
#include "arbdbt.h"

#include "seq_quality.h"
#include "SQ_functions.h"

#include <stdio.h>


GB_ERROR SQ_calc_seq_quality(GBDATA *gb_main, const char *tree_name) {
    

    char *option = "value_of_evaluation";

    /*
      "option" is variable which is passed to function "SQ_get_value()".
      SQ_get_value() returns the values that are stored in the specific containers used for alignment quality evaluation.
      Right now the options you have are:
      
      number_of_bases
      number_of_spaces
      number_of_dots
      percent_of_bases
      percent_of_spaces
      percent_of_dots
      diff_from_average
      value_of_evaluation
    */

    int weight_bases = 30;
    int weight_diff_from_average = 70;

    /*
      The "weight_..."  -values are passed to the function "SQ_evaluate()".
      SQ_evaluate() generates the final estimation for the quality of an alignment.
      It takes the values from the different containers, which are generated by the other functions, weights them
      and calculates a final value. The final value is stored in "value_of_evaluation" (see options).
      So, with the "weight_..."  -values one can customise how important a value stored in a contaier becomes
      for the final result.
    */

    SQ_calc_sequence_structure(gb_main);
    SQ_calc_average_structure(gb_main);
    SQ_evaluate(gb_main, weight_bases, weight_diff_from_average);

    int value = SQ_get_value(gb_main, option);

    //debug
    tree_name = 0;

    GB_ERROR error = 0;
    error = GB_export_error("Value in container: %i ", value);
    return error;


}
