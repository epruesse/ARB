// =============================================================== //
//                                                                 //
//   File      : AP_sequence.hxx                                   //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AP_SEQUENCE_HXX
#define AP_SEQUENCE_HXX

#ifndef ALIVIEW_HXX
#include <AliView.hxx>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

#define ap_assert(cond) arb_assert(cond)

typedef double AP_FLOAT;

long AP_timer();

class AP_sequence : virtual Noncopyable {
    mutable AP_FLOAT  cached_wbc;                   // result for weighted_base_count(); <0.0 means "not initialized"
    const AliView    *ali;

    GBDATA *gb_sequence;                            // points to species/ali_xxx/data (or NULL if unbound, e.g. inner nodes in tree)
    bool    has_sequence;                           // true -> sequence was set()
    long    update;

    static long global_combineCount;

protected:
    void mark_sequence_set(bool is_set) {
        if (is_set != has_sequence) {
            update   = is_set ? AP_timer() : 0;
            has_sequence = is_set;
            cached_wbc   = -1.0;
        }
    }

    static void inc_combine_count() { global_combineCount++; }

    virtual AP_FLOAT count_weighted_bases() const = 0;

    virtual void set(const char *sequence) = 0;
    virtual void unset()                   = 0;

    void do_lazy_load() const;

public:
    AP_sequence(const AliView *aliview);
    virtual ~AP_sequence() {}

    virtual AP_sequence *dup() const = 0;                 // used to dup derived class

    virtual AP_FLOAT combine(const AP_sequence* lefts, const AP_sequence *rights, char *mutation_per_site = 0) = 0;
    virtual void partial_match(const AP_sequence* part, long *overlap, long *penalty) const                    = 0;

    static long combine_count() { return global_combineCount; }

    GB_ERROR bind_to_species(GBDATA *gb_species);
    void     unbind_from_species();
    bool is_bound_to_species() const { return gb_sequence != NULL; }
    GBDATA *get_bound_species_data() const { return gb_sequence; }

    void lazy_load_sequence() const {
        if (!has_sequence && is_bound_to_species()) do_lazy_load();
    }
    void ensure_sequence_loaded() const {
        lazy_load_sequence();
        ap_assert(has_sequence);
    }

    bool got_sequence() const { return has_sequence; }
    void forget_sequence() {
        if (has_sequence) {
            unset();
            update       = 0;
            has_sequence = false;
            cached_wbc   = -1.0;
        }
    }

    AP_FLOAT weighted_base_count() const { // returns < 0.0 if no sequence!
        if (cached_wbc<0.0) cached_wbc = count_weighted_bases();
        return cached_wbc;
    }

    size_t get_sequence_length() const { return ali->get_length(); } // filtered length
    const AP_filter *get_filter() const { return ali->get_filter(); }
    const AP_weights *get_weights() const { return ali->get_weights(); }

    const AliView *get_aliview() const { return ali; }
};


#else
#error AP_sequence.hxx included twice
#endif // AP_SEQUENCE_HXX
