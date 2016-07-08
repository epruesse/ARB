// ============================================================= //
//                                                               //
//   File      : consensus_config.h                              //
//   Purpose   : Define names for consensus config managers      //
//               (to make sure both use same IDs)                //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in April 2015   //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef CONSENSUS_CONFIG_H
#define CONSENSUS_CONFIG_H

#define CONSENSUS_CONFIG_ID "consensus" // manager id

// CommonEntries:
#define CONSENSUS_CONFIG_COUNTGAPS   "countgaps"
#define CONSENSUS_CONFIG_GAPBOUND    "gapbound"
#define CONSENSUS_CONFIG_GROUP       "group"
#define CONSENSUS_CONFIG_CONSIDBOUND "considbound"
#define CONSENSUS_CONFIG_UPPER       "upper"
#define CONSENSUS_CONFIG_LOWER       "lower"

#else
#error consensus_config.h included twice
#endif // CONSENSUS_CONFIG_H
