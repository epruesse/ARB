// =============================================================== //
//                                                                 //
//   File      : GDE_awars.h                                       //
//   Purpose   : Declare AWARS used by ARB-GDE-interface           //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in January 2008   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GDE_AWARS_H
#define GDE_AWARS_H


#define AWAR_GDE_ALIGNMENT "tmp/gde/alignment"

#define AWAR_GDE_CUTOFF_STOPCODON "gde/cutoff_stop_codon"
#define AWAR_GDE_SPECIES          "gde/species"
#define AWAR_GDE_COMPRESSION      "gde/compress"

#define AWAR_GDE_FILTER_NAME      "gde/filter/name"
#define AWAR_GDE_FILTER_FILTER    "gde/filter/filter"
#define AWAR_GDE_FILTER_ALIGNMENT "gde/filter/alignment"

#else
#error GDE_awars.h included twice
#endif // GDE_AWARS_H
