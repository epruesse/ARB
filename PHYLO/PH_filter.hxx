#ifndef PH_FILTER_HXX
#define PH_FILTER_HXX

enum {
    OPT_START_COL = 0, 
    OPT_STOP_COL = 1, 
    OPT_MIN_HOM = 2, 
    OPT_MAX_HOM = 3, 
    OPT_FILTER_POINT = 4, 
    OPT_FILTER_MINUS = 5, 
    OPT_FILTER_AMBIG = 6, 
    OPT_FILTER_LOWER = 7, 
};

class PH_filter {
public:
    char *filter;               // 0 1
    long  filter_len;
    long  real_len;             // how many 1
    long  update;
    long *options_vector;       // options used to calculate current filter
    //  float   *markerline;       // line to create filter (according to options_vector)
    char *init(char *filter, char *zerobases, long size);
    char *init(long size);

    PH_filter(void);
    ~PH_filter(void);
    float *calculate_column_homology(void);
};

#else
#error PH_filter.hxx included twice
#endif
