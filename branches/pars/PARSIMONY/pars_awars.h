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

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif

#define PREFIX_KL         "genetic/kh/"
#define PREFIX_KL_STATIC  PREFIX_KL "static/"
#define PREFIX_KL_DYNAMIC PREFIX_KL "dynamic/"

#define AWAR_KL_RANDOM_NODES PREFIX_KL "nodes"
#define AWAR_KL_MAXDEPTH     PREFIX_KL "maxdepth"
#define AWAR_KL_INCDEPTH     PREFIX_KL "inc_depth"

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

#define AWAR_KL_FUNCTION_TYPE PREFIX_KL "function_type" // threshold function for dynamic reduction (not configurable; experimental?)

// --------------------------------------------------------------------------------

const int CUSTOM_STATIC_PATH_REDUCTION_DEPTH = 5;

enum KL_DYNAMIC_THRESHOLD_TYPE {
    AP_QUADRAT_START = 5,
    AP_QUADRAT_MAX   = 6
};

struct KL_Settings {
    double random_nodes; // AWAR_KL_RANDOM_NODES
    int    maxdepth;     // AWAR_KL_MAXDEPTH
    int    incdepth;     // AWAR_KL_INCDEPTH

    struct {
        bool enabled;                                   // AWAR_KL_STATIC_ENABLED
        int  depth[CUSTOM_STATIC_PATH_REDUCTION_DEPTH]; // AWAR_KL_STATIC_DEPTH0 .. AWAR_KL_STATIC_DEPTH4
    } Static;
    struct {
        bool                      enabled; // AWAR_KL_DYNAMIC_ENABLED
        KL_DYNAMIC_THRESHOLD_TYPE type;    // AWAR_KL_FUNCTION_TYPE
        int                       start;   // AWAR_KL_DYNAMIC_START
        int                       maxx;    // AWAR_KL_DYNAMIC_MAXX
        int                       maxy;    // AWAR_KL_DYNAMIC_MAXY
    } Dynamic;

    KL_Settings(AW_root *aw_root); // read from AWARs
#if defined(UNIT_TESTS)
    KL_Settings(GB_alignment_type atype); // init with unittest defaults
#endif
};


#else
#error pars_awars.h included twice
#endif // PARS_AWARS_H

