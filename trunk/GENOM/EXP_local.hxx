//  ==================================================================== //
//                                                                       //
//    File      : EXP_local.hxx                                          //
//    Purpose   :                                                        //
//    Time-stamp: <Fri Aug/23/2002 19:22 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in September 2001        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#ifndef EXP_LOCAL_HXX
#define EXP_LOCAL_HXX

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define exp_assert(bed) arb_assert(bed)

// contains the path to the experiment:  "organism_name;experiment_name"
// writing this awar has no effect
#define AWAR_COMBINED_EXPERIMENT_NAME "tmp/experiment/combined_name"

// to create new experiments:
#define AWAR_EXPERIMENT_DEST "tmp/experiment/dest"

AW_window *EXP_create_experiment_window(AW_root *aw_root);

#else
#error EXP_local.hxx included twice
#endif // EXP_LOCAL_HXX

