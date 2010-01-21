/*********************************************************************************
 *  Coded by Ralf Westram (coder@reallysoft.de) in 2000                          *
 *  Institute of Microbiology (Technical University Munich)                      *
 *  http://www.mikro.biologie.tu-muenchen.de/                                    *
 *********************************************************************************/

#ifndef AISC_FUNC_TYPES_H
#define AISC_FUNC_TYPES_H

struct sigcontext;

#define aisc_callback_func_proto(func_name)         int func_name(long arg1, ...)
#define aisc_talking_func_proto_long(func_name)     long func_name(long arg1, ...)
#define aisc_talking_func_proto_longp(func_name)    long* func_name(long arg1, ...)
#define aisc_talking_func_proto_double(func_name)   double func_name(long arg1, ...)

typedef aisc_callback_func_proto((*aisc_callback_func));
typedef aisc_talking_func_proto_long((*aisc_talking_func_long));
typedef aisc_talking_func_proto_longp((*aisc_talking_func_longp));
typedef aisc_talking_func_proto_double((*aisc_talking_func_double));

#else
#error aisc_func_types.h included twice
#endif // AISC_FUNC_TYPES_H
