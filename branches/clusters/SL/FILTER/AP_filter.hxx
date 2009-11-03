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
    AWT_FILTER_SIMPLIFY_NOT_INITIALIZED = -1, 
    AWT_FILTER_SIMPLIFY_NONE            = 0,
    AWT_FILTER_SIMPLIFY_DNA,
    AWT_FILTER_SIMPLIFY_PROTEIN,
};

class AP_filter {
    bool   *filter_mask;                            // true means "use position"
    size_t  filter_len;                             // length of 'filter_mask'
    size_t  real_len;                               // how many 'true's are in 'filter_mask'
    long    update;                                 // timestamp

    uchar               simplify[256];              // base -> simplified base
    AWT_FILTER_SIMPLIFY simplify_type;

    // if not NULL, then sizeof(bootstrap) == real_len
    // bootstrap[i] points to random original positions [0..filter_len[
    size_t *bootstrap;

    size_t *filterpos_2_seqpos;                     // filterpos -> sequencepos

    void calc_filterpos_2_seqpos();
    void resize(size_t newLen);

public:
    AP_filter();
    AP_filter(const AP_filter& other);
    ~AP_filter();

    void init(const char *filter,const char *zerobases, size_t size);
    void init(size_t size);

    long get_timestamp() const { return update; }
    size_t get_filtered_length() const { return real_len; }
    size_t get_length() const { return filter_len; }

    bool use_position(size_t pos) const {           // returns true if filter is set for position 'pos'
        af_assert(pos<filter_len);
        return filter_mask[pos];
    }

    const size_t *get_filterpos_2_seqpos() const { 
        if (!filterpos_2_seqpos) {
            // this is no modification, it's lazy initialization:
            const_cast<AP_filter*>(this)->calc_filterpos_2_seqpos();
        }
        return filterpos_2_seqpos;
    }

    void enable_simplify(AWT_FILTER_SIMPLIFY type); // default is AWT_FILTER_SIMPLIFY_NONE
    const uchar *get_simplify_table() const {
        if (simplify_type == AWT_FILTER_SIMPLIFY_NOT_INITIALIZED) {
            // this is no modification, it's lazy initialization: 
            const_cast<AP_filter*>(this)->enable_simplify(AWT_FILTER_SIMPLIFY_NONE);
        }
        return simplify;
    }

    void enable_bootstrap();
    const size_t *get_bootstrap() const { return bootstrap; }

    char *to_string() const;                              // convert to 0/1 string
};



class AP_weights {
    GB_UINT4 *weights;
    size_t    weight_len;
    size_t    alloc_len;

    long update;

    void resize(size_t newLen);

public:

    AP_weights();
    AP_weights(const AP_weights& other);
    ~AP_weights();

    void init(const AP_filter *fil);                // init weights
    void init(GB_UINT4 *w, size_t wlen, const AP_filter *fil);

    GB_UINT4 weight(size_t idx) const {
        af_assert(idx<weight_len);
        return weights[idx];
    }

    size_t length() const { return weight_len; }
};

long AP_timer();

#else
#error AP_filter.hxx included twice
#endif // AP_FILTER_HXX
