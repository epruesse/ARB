// =============================================================== //
//                                                                 //
//   File      : aisc_func_types.h                                 //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in 2000           //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AISC_FUNC_TYPES_H
#define AISC_FUNC_TYPES_H

struct sigcontext;
struct Hs_struct;

#define aisc_talking_func_proto_void(func_name)     void func_name(long arg1, ...)
#define aisc_talking_func_proto_long(func_name)     long func_name(long arg1, ...)
#define aisc_talking_func_proto_longp(func_name)    long* func_name(long arg1, ...)
#define aisc_talking_func_proto_double(func_name)   double func_name(long arg1, ...)

typedef aisc_talking_func_proto_void((*aisc_destroy_callback));
typedef aisc_talking_func_proto_long((*aisc_talking_func_long));
typedef aisc_talking_func_proto_longp((*aisc_talking_func_longp));
typedef aisc_talking_func_proto_double((*aisc_talking_func_double));

#else
#error aisc_func_types.h included twice
#endif // AISC_FUNC_TYPES_H
