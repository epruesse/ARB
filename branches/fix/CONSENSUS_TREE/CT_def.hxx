// ============================================================= //
//                                                               //
//   File      : CT_def.hxx                                      //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in March 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef CT_DEF_HXX
#define CT_DEF_HXX

// debug flags
#if defined(DEBUG)
#define NTREE_DEBUG_FUNCTIONS // debug function for NTREE
// #define DUMP_PART_INIT
// #define DUMP_PART_INSERTION
#endif // DEBUG

inline int NEGATED_double_cmp(double d1, const double d2) {
    /*! returns <0 if d1>d2, >0 if d1<d2 (i.e. this function behaves wrong compared to strcmp) */
    double d = d2-d1;
    return d<0 ? -1 : (d>0 ? 1 : 0);
}

#else
#error CT_def.hxx included twice
#endif // CT_DEF_HXX
