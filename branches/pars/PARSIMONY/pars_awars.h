// =============================================================== //
//                                                                 //
//   File      : pars_awars.h                                      //
//   Purpose   : declares AWARs used in PARSIMONY                  //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in January 2015   //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef PARS_AWARS_H
#define PARS_AWARS_H

#define PREFIX_KL         "genetic/kh/"
#define PREFIX_KL_STATIC  PREFIX_KL "static/"
#define PREFIX_KL_DYNAMIC PREFIX_KL "dynamic/"

#define AWAR_KL_RANDOM_NODES PREFIX_KL "nodes"
#define AWAR_KL_MAXDEPTH     PREFIX_KL "maxdepth"
#define AWAR_KL_INCDEPTH     PREFIX_KL "incdepth"  // @@@ unused!

#define AWAR_KL_STATIC_ENABLED PREFIX_KL_STATIC "enable"
#define AWAR_KL_STATIC_DEPTH0  PREFIX_KL_STATIC "depth0"
#define AWAR_KL_STATIC_DEPTH1  PREFIX_KL_STATIC "depth1"
#define AWAR_KL_STATIC_DEPTH2  PREFIX_KL_STATIC "depth2"
#define AWAR_KL_STATIC_DEPTH3  PREFIX_KL_STATIC "depth3"
#define AWAR_KL_STATIC_DEPTH4  PREFIX_KL_STATIC "depth4"

#define AWAR_KL_DYNAMIC_ENABLED PREFIX_KL_DYNAMIC "enable"
#define AWAR_KL_DYNAMIC_START   PREFIX_KL_DYNAMIC "start"
#define AWAR_KL_DYNAMIC_MAXX    PREFIX_KL_DYNAMIC "maxx"
#define AWAR_KL_DYNAMIC_MAXY    PREFIX_KL_DYNAMIC "maxy"

#define AWAR_KL_FUNCTION_TYPE PREFIX_KL "function_type" // for dynamic reduction (not configurable; experimental?)

#else
#error pars_awars.h included twice
#endif // PARS_AWARS_H
