// =============================================================== //
//                                                                 //
//   File      : AP_parsimony_defaults.hxx                         //
//   Purpose   : control behavior of parsimony                     //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AP_PARSIMONY_DEFAULTS_HXX
#define AP_PARSIMONY_DEFAULTS_HXX

// --------------------------------------------------------------------------------
//      Defines for PARSIMONY behavior

// When MULTIPLE_GAPS_ARE_MULTIPLE_MUTATIONS is 'undefined' it compiles
// an EXPERIMENTAL VERSION of parsimony that counts multiple
// consecutive gaps as one mutation.
// Our tests show that this method produces bad trees.
//
// Recommended setting is 'defined'.

#define MULTIPLE_GAPS_ARE_MULTIPLE_MUTATIONS


// When PROPAGATE_GAPS_UPWARDS is defined, gaps are propagates upwards
// and they may match against gaps in neighbour sequences.
//
// Recommended setting is 'defined'.

#define PROPAGATE_GAPS_UPWARDS

// --------------------------------------------------------------------------------
// Do not change these settings!
// --------------------------------------------------------------------------------

#else
#error AP_parsimony_defaults.hxx included twice
#endif // AP_PARSIMONY_DEFAULTS_HXX
