// =============================================================== //
//                                                                 //
//   File      : AP_filter.hxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AP_FILTER_HXX
#define AP_FILTER_HXX

#ifndef ARBDBT_H
#include <arbdbt.h>
#endif

#define af_assert(cond) arb_assert(cond)

typedef unsigned char uchar;

enum AWT_FILTER_SIMPLIFY {
    AWT_FILTER_SIMPLIFY_NONE,
    AWT_FILTER_SIMPLIFY_DNA,
    AWT_FILTER_SIMPLIFY_PROTEIN
};



class AP_filter {
public:
    char  *filter_mask;                             // 0 1
    long   filter_len;
    long   real_len;                                // how many 1
    long   update;
    uchar  simplify[256];
    int   *filterpos_2_seqpos;
    int   *bootstrap;                               // if set then sizeof(bootstrap) == real_len; bootstrap[i] points to random original positions [0..filter_len]

    GB_ERROR init(const char *filter,const char *zerobases, long size);
    GB_ERROR init(long size);
    void    calc_filter_2_seq();
    void    enable_bootstrap();
    void    enable_simplify(AWT_FILTER_SIMPLIFY type);
    char    *to_string();   // convert to 0/1 string
    AP_filter(void);
    ~AP_filter(void);
};



class AP_weights {
protected:
    friend class AP_sequence;
    friend class AP_sequence_parsimony;
    friend class AP_sequence_protein;
    friend class AP_sequence_protein_old;

    GB_UINT4 *weights;

public:
    
    long       weight_len;
    AP_filter *filter;
    long       update;
    bool       dummy_weights;   // if true all weights are == 1

    AP_weights(void);
    char *init(const AP_filter *fil); // init weights
    char *init(GB_UINT4 *w, const AP_filter *fil);
    ~AP_weights(void);
};

long AP_timer(void);

#else
#error AP_filter.hxx included twice
#endif // AP_FILTER_HXX
