//  ==================================================================== //
//                                                                       //
//    File      : global_defs.h                                          //
//    Purpose   : global definitions for PROBE_GROUP and                 //
//                PROBE_GROUP_DESIGN                                     //
//    Note      : include only once in each executable!!!                //
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

struct probe_config_data {
    double min_temperature;
    double max_temperature;
    double min_gccontent;
    double max_gccontent;
    int    max_hairpin_bonds;
    int    ecoli_min_pos;
    int    ecoli_max_pos;

    bool   use_weighted_mismatches;
    // rest is unused if (use_weighted_mismatches == false)
    
    // same as in expert window of probe match/design
    double bonds[4*4];
    double split;
    double dtedge;
    double dt;
};


#else
#error global_defs.h included twice
#endif // GLOBAL_DEFS_H

