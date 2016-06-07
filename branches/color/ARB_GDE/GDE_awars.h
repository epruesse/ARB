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

#define AWAR_PREFIX_GDE_TEMP "tmp/gde"
#define AWAR_PREFIX_GDE      "gde"

#define AWAR_GDE_CUTOFF_STOPCODON AWAR_PREFIX_GDE "/cutoff_stop_codon"
#define AWAR_GDE_SPECIES          AWAR_PREFIX_GDE "/species"
#define AWAR_GDE_COMPRESSION      AWAR_PREFIX_GDE "/compress"

#else
#error GDE_awars.h included twice
#endif // GDE_AWARS_H
