void SQ_calc_sequence_structure(GBDATA *gb_main, bool marked_only);

void SQ_calc_average_structure(GBDATA *gb_main);

int SQ_get_value(GBDATA *gb_main, const char *option);
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

void SQ_evaluate(GBDATA *gb_main, int weight_bases, int weight_diff_from_average);
    /*
      The "weight_..."  -values are passed to the function "SQ_evaluate()".
      SQ_evaluate() generates the final estimation for the quality of an alignment.
      It takes the values from the different containers, which are generated by the other functions, weights them
      and calculates a final value. The final value is stored in "value_of_evaluation" (see options).
      So, with the "weight_..."  -values one can customise how important a value stored in a contaier becomes
      for the final result.
    */

void SQ_calc_helix_conformance(GBDATA *gb_main);
