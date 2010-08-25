// =============================================================== //
//                                                                 //
//   File      : aisc_global.h                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in May 2007       //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //
#ifndef AISC_GLOBAL_H
#define AISC_GLOBAL_H

// type mask
#define AISC_TYPE_INT    0x01000000
#define AISC_TYPE_DOUBLE 0x02000000
#define AISC_TYPE_STRING 0x03000000
#define AISC_TYPE_COMMON 0x04000000
#define AISC_TYPE_BYTES  0x05000000

#define AISC_VAR_TYPE_MASK 0xff000000
#define AISC_OBJ_TYPE_MASK 0x00ff0000
#define AISC_ATTR_MASK     0x0000ffff

#define AISC_INDEX       0x1ff0000
#define AISC_NO_ANSWER  -0x7fffffff

#define AISC_COMMON      0


#else
#error aisc_global.h included twice
#endif // AISC_GLOBAL_H

