//  ==================================================================== //
//                                                                       //
//    File      : EXP_local.hxx                                          //
//    Purpose   :                                                        //
//    Time-stamp: <Tue Oct/02/2001 17:38 MET Coder@ReallySoft.de>        //
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

#ifndef NDEBUG
# define exp_assert(bed) do { if (!(bed)) *(int *)0 = 0; } while (0)
# ifndef DEBUG
#  error DEBUG is NOT defined - but it has to!
# endif
#else
# ifdef DEBUG
#  error DEBUG is defined - but it should not!
# endif
# define exp_assert(bed)
#endif /* NDEBUG */

// contains the path to the experiment:  "organism_name;experiment_name"
// writing this awar has no effect
#define AWAR_COMBINED_EXPERIMENT_NAME "tmp/experiment/combined_name"

// to create new experiments:
#define AWAR_EXPERIMENT_DEST "tmp/experiment/dest"

AW_window *EXP_create_experiment_window(AW_root *aw_root);

#else
#error EXP_local.hxx included twice
#endif // EXP_LOCAL_HXX

