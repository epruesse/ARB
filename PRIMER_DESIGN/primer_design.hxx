#ifndef PRIMER_DESIGN_HXX
#define PRIMER_DESIGN_HXX

#define AWAR_PRIMER_DESIGN_POS_LEFT_MIN           "primer_design/position/left_min"
#define AWAR_PRIMER_DESIGN_POS_LEFT_MAX           "primer_design/position/left_max"
#define AWAR_PRIMER_DESIGN_POS_RIGHT_MIN          "primer_design/position/right_min"
#define AWAR_PRIMER_DESIGN_POS_RIGHT_MAX          "primer_design/position/right_max"
#define AWAR_PRIMER_DESIGN_LENGTH_MIN             "primer_design/length/min"
#define AWAR_PRIMER_DESIGN_LENGTH_MAX             "primer_design/length/max"
#define AWAR_PRIMER_DESIGN_DIST_USE               "primer_design/distance/use"
#define AWAR_PRIMER_DESIGN_DIST_MIN               "primer_design/distance/min"
#define AWAR_PRIMER_DESIGN_DIST_MAX               "primer_design/distance/max"
#define AWAR_PRIMER_DESIGN_GCRATIO_MIN            "primer_design/gcratio/min"
#define AWAR_PRIMER_DESIGN_GCRATIO_MAX            "primer_design/gcratio/max"
#define AWAR_PRIMER_DESIGN_TEMPERATURE_MIN        "primer_design/temperature/min"
#define AWAR_PRIMER_DESIGN_TEMPERATURE_MAX        "primer_design/temperature/max"
#define AWAR_PRIMER_DESIGN_ALLOWED_MATCH_MIN_DIST "primer_design/allowed_match_min_dist"
#define AWAR_PRIMER_DESIGN_EXPAND_IUPAC           "primer_design/expand_IUPAC"
#define AWAR_PRIMER_DESIGN_MAX_PAIRS              "primer_design/max_pairs"
#define AWAR_PRIMER_DESIGN_GC_FACTOR              "primer_design/GC_factor"
#define AWAR_PRIMER_DESIGN_TEMP_FACTOR            "primer_design/temp_factor"

AW_window *create_primer_design_window( AW_root *root,AW_default def);
void       create_primer_design_variables(AW_root *aw_root, AW_default aw_def, AW_default global);


#else
#error primer_design.hxx included twice
#endif // PRIMER_DESIGN_HXX
