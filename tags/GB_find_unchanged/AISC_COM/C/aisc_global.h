// =============================================================== //
//                                                                 //
//   File      : aisc_global.h                                     //
//   Purpose   :                                                   //
//   Time-stamp: <Tue May/22/2007 19:30 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in May 2007       //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //
#ifndef AISC_GLOBAL_H
#define AISC_GLOBAL_H

#define AISC_ATTR_INT    0x1000000
#define AISC_ATTR_DOUBLE 0x2000000
#define AISC_ATTR_STRING 0x3000000
#define AISC_ATTR_COMMON 0x4000000
#define AISC_ATTR_BYTES  0x5000000

#define AISC_INDEX       0x1ff0000
#define AISC_NO_ANSWER  -0x7fffffff

#define AISC_COMMON      0


#else
#error aisc_global.h included twice
#endif // AISC_GLOBAL_H

