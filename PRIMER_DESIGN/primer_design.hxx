// =============================================================== //
//                                                                 //
//   File      : primer_design.hxx                                 //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Wolfram Foerster in February 2001                    //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef PRIMER_DESIGN_HXX
#define PRIMER_DESIGN_HXX

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif

#define AWAR_PRIMER_DESIGN_LEFT_POS               "primer_design/position/left_pos"
#define AWAR_PRIMER_DESIGN_LEFT_LENGTH            "primer_design/position/left_length"
#define AWAR_PRIMER_DESIGN_RIGHT_POS              "primer_design/position/right_pos"
#define AWAR_PRIMER_DESIGN_RIGHT_LENGTH           "primer_design/position/right_length"
#define AWAR_PRIMER_DESIGN_LENGTH_MIN             "primer_design/length/min"
#define AWAR_PRIMER_DESIGN_LENGTH_MAX             "primer_design/length/max"
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
#define AWAR_PRIMER_DESIGN_APROX_MEM              "primer_design/aprox_mem"

void       create_primer_design_variables(AW_root *aw_root, AW_default aw_def, AW_default global);
AW_window *create_primer_design_window(AW_root *root, GBDATA *gb_main);

#else
#error primer_design.hxx included twice
#endif // PRIMER_DESIGN_HXX
