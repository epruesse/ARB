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

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
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

    size_t *bootstrap; // bootstrap[i] points to random filter positions [0..real_len[

    size_t *filterpos_2_seqpos;                     // filterpos -> sequencepos

#if defined(ASSERTION_USED)
    mutable bool checked_for_validity;
#endif

    void calc_filterpos_2_seqpos();

    void init(size_t size);
    void make_permeable(size_t size);

    size_t bootstrapped_filterpos(size_t bpos) const {
        af_assert(does_bootstrap());
        af_assert(bpos<real_len);
        size_t fpos = bootstrap[bpos];
        af_assert(fpos<real_len);
        return fpos;
    }

public:
    AP_filter(size_t size); // permeable filter (passes all columns)
    AP_filter(const char *filter, const char *zerobases, size_t size);
    AP_filter(const AP_filter& other);
    ~AP_filter();
    DECLARE_ASSIGNMENT_OPERATOR(AP_filter);

    long get_timestamp() const { return update; }
    size_t get_filtered_length() const { return real_len; }
    size_t get_length() const { return filter_len; }

    bool use_position(size_t pos) const {           // returns true if filter is set for position 'pos'
        af_assert(checked_for_validity);
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
    bool does_bootstrap() const { return bootstrap; }

    size_t bootstrapped_seqpos(size_t bpos) const {
        size_t fpos   = bootstrapped_filterpos(bpos);
        size_t spos = (get_filterpos_2_seqpos())[fpos];
        af_assert(spos<filter_len);
        return spos;
    }

    char *to_string() const;                        // convert to 0/1 string

    char *blowup_string(char *filtered_string, char insert) const;

    GB_ERROR is_invalid() const {
        /*! returns error
         * - if filter is based on an empty alignment (i.e. no alignment is selected)
         * - if all positions are filtered out (i.e. filtered sequences will be empty)
         */

#if defined(ASSERTION_USED)
        checked_for_validity = true;
#endif
        if (get_filtered_length()) return NULL;
        if (get_length()) return "Sequence completely filtered out (no columns left)";
        return "No alignment selected";
    }
#if defined(ASSERTION_USED)
    bool was_checked_for_validity() const { return checked_for_validity; }
#endif
};



class AP_weights {
    size_t    len;
    GB_UINT4 *weights;

public:

    AP_weights(const AP_filter *fil); // dummy weights (all columns weighted equal)
    AP_weights(const GB_UINT4 *w, size_t wlen, const AP_filter *fil);
    AP_weights(const AP_weights& other);
    ~AP_weights();
    DECLARE_ASSIGNMENT_OPERATOR(AP_weights);

    GB_UINT4 weight(size_t idx) const {
        af_assert(idx<len);
        return weights[idx];
    }

    size_t length() const { return len; }
};

long AP_timer();

#else
#error AP_filter.hxx included twice
#endif // AP_FILTER_HXX
