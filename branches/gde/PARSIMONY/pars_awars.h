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

#define PREFIX_OPTI       "optimize/"
#define PREFIX_KL         "kernlin/"
#define PREFIX_KL_STATIC  PREFIX_KL "static/"
#define PREFIX_KL_DYNAMIC PREFIX_KL "dynamic/"
#define PREFIX_RAND       "randomize/"

#define AWAR_OPTI_MARKED_ONLY PREFIX_OPTI "marked"
#define AWAR_OPTI_SKIP_FOLDED PREFIX_OPTI "visible"

#define AWAR_RAND_REPEAT   "tmp/" PREFIX_RAND "repeat"
#define AWAR_RAND_PERCENT  PREFIX_RAND "percent"

#define AWAR_KL_MAXDEPTH PREFIX_KL "maxdepth"
#define AWAR_KL_INCDEPTH PREFIX_KL "inc_depth"

#define AWAR_KL_STATIC_ENABLED PREFIX_KL_STATIC "enable"
#define AWAR_KL_STATIC_DEPTH1  PREFIX_KL_STATIC "depth1"
#define AWAR_KL_STATIC_DEPTH2  PREFIX_KL_STATIC "depth2"
#define AWAR_KL_STATIC_DEPTH3  PREFIX_KL_STATIC "depth3"
#define AWAR_KL_STATIC_DEPTH4  PREFIX_KL_STATIC "depth4"
#define AWAR_KL_STATIC_DEPTH5  PREFIX_KL_STATIC "depth5"

#define AWAR_KL_DYNAMIC_ENABLED PREFIX_KL_DYNAMIC "enable"
#define AWAR_KL_DYNAMIC_START   PREFIX_KL_DYNAMIC "start"
#define AWAR_KL_DYNAMIC_MAXX    PREFIX_KL_DYNAMIC "maxx"
#define AWAR_KL_DYNAMIC_MAXY    PREFIX_KL_DYNAMIC "maxy"

#define AWAR_KL_FUNCTION_TYPE PREFIX_KL "function_type" // threshold function for dynamic reduction (not configurable; experimental?)

// --------------------------------------------------------------------------------

const int CUSTOM_STATIC_PATH_REDUCTION_DEPTH = 5;
const int CUSTOM_DEPTHS                      = CUSTOM_STATIC_PATH_REDUCTION_DEPTH+1; // (+depth[0], which is always 2)

enum KL_DYNAMIC_THRESHOLD_TYPE {
    AP_QUADRAT_START = 5,
    AP_QUADRAT_MAX   = 6
};

enum EdgeSpec {
    // bit-values:
    ANY_EDGE            = 0, // default=0 = take any_edge
    SKIP_UNMARKED_EDGES = 1,
    SKIP_FOLDED_EDGES   = 2, // Note: also skips edges adjacent to folded groups
    SKIP_LEAF_EDGES     = 4,
    SKIP_INNER_EDGES    = 8,
};

struct KL_Settings {
    int maxdepth; // AWAR_KL_MAXDEPTH
    int incdepth; // AWAR_KL_INCDEPTH

    struct {
        bool enabled;              // AWAR_KL_STATIC_ENABLED
        int  depth[CUSTOM_DEPTHS]; // [0]=2, [1..5]=AWAR_KL_STATIC_DEPTH1 .. AWAR_KL_STATIC_DEPTH5
    } Static;
    struct {
        bool                      enabled; // AWAR_KL_DYNAMIC_ENABLED
        KL_DYNAMIC_THRESHOLD_TYPE type;    // AWAR_KL_FUNCTION_TYPE
        int                       start;   // AWAR_KL_DYNAMIC_START
        int                       maxx;    // AWAR_KL_DYNAMIC_MAXX
        int                       maxy;    // AWAR_KL_DYNAMIC_MAXY
    } Dynamic;

    EdgeSpec whichEdges;

    KL_Settings(AW_root *aw_root); // read from AWARs
#if defined(UNIT_TESTS)
    explicit KL_Settings(); // init with unittest defaults
#endif
};


#else
#error pars_awars.h included twice
#endif // PARS_AWARS_H

