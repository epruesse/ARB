// ============================================================= //
//                                                               //
//   File      : PT_global_defs.h                                //
//   Purpose   : global defines for pt-server and its clients    //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in March 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef PT_GLOBAL_DEFS_H
#define PT_GLOBAL_DEFS_H

enum FF_complement { 
    FF_FORWARD            = 1,
    FF_REVERSE            = 2,
    FF_REVERSE_COMPLEMENT = 4,
    FF_COMPLEMENT         = 8,

    // do NOT change the order here w/o fixing ../PROBE/PT_family.cxx@FF_complement_dep
};

#else
#error PT_global_defs.h included twice
#endif // PT_GLOBAL_DEFS_H
