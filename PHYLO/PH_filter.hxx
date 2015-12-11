// =========================================================== //
//                                                             //
//   File      : PH_filter.hxx                                 //
//   Purpose   :                                               //
//                                                             //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef PH_FILTER_HXX
#define PH_FILTER_HXX

#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif

enum {
    OPT_START_COL    = 0,
    OPT_STOP_COL     = 1,
    OPT_MIN_HOM      = 2,
    OPT_MAX_HOM      = 3,
    OPT_FILTER_POINT = 4,
    OPT_FILTER_MINUS = 5,
    OPT_FILTER_AMBIG = 6,
    OPT_FILTER_LOWER = 7,

    OPT_COUNT = 8,
};

class PH_filter : virtual Noncopyable {
    char *filter;         // 0 1
    long  filter_len;
    long  real_len;       // how many 1
    long  update;
    long *options_vector; // options used to calculate current filter

public:


    PH_filter();
    ~PH_filter();

    char *init(long size);

    float *calculate_column_homology();
};

#else
#error PH_filter.hxx included twice
#endif

