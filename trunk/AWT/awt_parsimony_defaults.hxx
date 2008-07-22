//  ==================================================================== //
//                                                                       //
//    File      : awt_parsimony_defaults.hxx                             //
//    Purpose   : control behavior of parsimony                          //
//    Time-stamp: <Tue Jul/22/2008 16:25 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in July 2003             //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#ifndef AWT_PARSIMONY_DEFAULTS_HXX
#define AWT_PARSIMONY_DEFAULTS_HXX

// --------------------------------------------------------------------------------
//      Defines for PARSIMONY behavior

// When MULTIPLE_GAPS_ARE_MULTIPLE_MUTATIONS is 'undefined' it compiles
// an EXPERIMENTAL VERSION of parsimony that counts multiple
// consequetive gaps as one mutation.
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
#error awt_parsimony_defaults.hxx included twice
#endif // AWT_PARSIMONY_DEFAULTS_HXX

