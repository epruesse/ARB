//  ==================================================================== //
//                                                                       //
//    File      : global_defs.h                                          //
//    Purpose   : global definitions for PROBE_GROUP and                 //
//                PROBE_GROUP_DESIGN                                     //
//    Note      : include only once in each executable!!!                //
//    Time-stamp: <Fri Sep/12/2003 11:27 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in September 2003        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#ifndef GLOBAL_DEFS_H
#define GLOBAL_DEFS_H


#if defined(DEBUG)
#define SAVE_MODE "a"
// #define SAVE_MODE "bm"
#else
#define SAVE_MODE "bm"
#endif // DEBUG


#else
#error global_defs.h included twice
#endif // GLOBAL_DEFS_H

