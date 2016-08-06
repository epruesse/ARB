#include "AP_seq_dna.hxx"

#include <arb_mem.h>
#include <AP_pro_a_nucs.hxx>
#include <AP_filter.hxx>
#include <ARB_Tree.hxx>


inline bool hasGap(char c) { return c & AP_GAP; }
inline bool isGap(char c)  { return c == AP_GAP; }

inline bool notHasGap(char c) { return !hasGap(c); }
inline bool notIsGap(char c)  { return !isGap(c); }

// -------------------------------
//      AP_sequence_parsimony

char *AP_sequence_parsimony::table;

AP_sequence_parsimony::AP_sequence_parsimony(const AliView *aliview)
    : AP_sequence(aliview)
    , seq_pars(NULL)
{
}

AP_sequence_parsimony::~AP_sequence_parsimony() {
    free(seq_pars);
}

AP_sequence *AP_sequence_parsimony::dup() const {
    return new AP_sequence_parsimony(get_aliview());
}

void AP_sequence_parsimony::build_table()
{
    table = (char *)AP_create_dna_to_ap_bases();
}



/* --------------------------------------------------------------------------------
 * combine(const AP_sequence *lefts, const AP_sequence *rights)
 * set(char *isequence)
 *
 * for wagner & fitch parsimony algorithm
 *
 * Note: is_set_flag is used by AP_tree_nlen::parsimony_rek()
 *       see ../../PARSIMONY/AP_tree_nlen.cxx@parsimony_rek
*/

// #define SHOW_SEQ

void AP_sequence_parsimony::set(const char *isequence) {
    size_t sequence_len = get_filter()->get_filtered_length();
    ARB_alloc_aligned(seq_pars, sequence_len+1);
    memset(seq_pars, AP_DOT, (size_t)sequence_len+1); // init with dots

    const uchar *simplify = get_filter()->get_simplify_table();
    if (!table) this->build_table();

    const AP_filter *filt = get_filter();
    if (filt->does_bootstrap()) {
        size_t iseqlen = strlen(isequence);

        for (size_t i = 0; i<sequence_len; ++i) {
            size_t pos = filt->bootstrapped_seqpos(i); // random indices (but same for all species)

            ap_assert(pos<iseqlen);
            if (pos >= iseqlen) continue;

            unsigned char c = (unsigned char)isequence[pos];

#if defined(SHOW_SEQ)
            fputc(simplify[c], stdout);
#endif // SHOW_SEQ

            seq_pars[i] = table[simplify[c]];
        }
    }
    else {
        const size_t* base_pos = filt->get_filterpos_2_seqpos();

        for (size_t i = 0; i < sequence_len; ++i) {
            size_t pos = base_pos[i];
            unsigned char c = (unsigned char)isequence[pos];
            seq_pars[i] = table[simplify[c]];

#if defined(SHOW_SEQ)
                fputc(simplify[c], stdout);
#endif // SHOW_SEQ
        }
    }

#if defined(SHOW_SEQ)
    fputc('\n', stdout);
#endif // SHOW_SEQ

    mark_sequence_set(true);
}

void AP_sequence_parsimony::unset() {
    freenull(seq_pars);
    mark_sequence_set(false);
}

/** BELOW CODE CAREFULLY DESIGNED TO ALLOW VECTORIZATION
 *
 * If you mess with it, use "-fopt-info" or "-ftree-vectorizer-verbose=n".
 * Make sure you still see "LOOP VECTORIZED" in the output!
 */

template <class COUNT, class SITE>
static long do_combine(size_t sequence_len,
                       const char * __restrict p1,
                       const char * __restrict p2,
                       char * __restrict p,
                       COUNT count,
                       SITE site) {

    for (size_t idx = 0; idx<sequence_len; ++idx) { // LOOP_VECTORIZED=4 (ok, do_combine is used 4 times)
        char c1 = p1[idx];
        char c2 = p2[idx];

        char c = c1 & c2;
        p[idx] = (c==0)?c1|c2:c;

        count.add(idx, c);
        site.add(idx, c);
    }

    return count.sum;
}

struct count_unweighted {
    long sum;
    count_unweighted():sum(0){}
    void add(size_t, char c) {
        sum += !c;
    }
};

struct count_weighted {
    long sum;
    const GB_UINT4* weights;
    count_weighted(const GB_UINT4* w):sum(0), weights(w){}
    void add(size_t idx, char c) {
        sum += !c * weights[idx];
    }
};

struct count_nothing {
    void add(size_t, char) {}
};

struct count_mutpsite {
    char * sites;
    count_mutpsite(char *s):sites(s){}
    void add(size_t idx, char c) {
        // below code is equal to "if (!c) ++sites[idx]", the difference
        // is that no branch is required and sites[idx] is always
        // written, allowing vectorization.
        //
        // For unknown reasons gcc 4.8.1, 4.9.2 and 5.1.0
        // refuses to vectorize 'c==0?1:0' or '!c'

        sites[idx] += ((c | -c) >> 7 & 1) ^ 1;
    }
};

AP_FLOAT AP_sequence_parsimony::combine(const AP_sequence * lefts,
                                        const AP_sequence * rights,
                                        char *mutation_per_site) {
    const AP_sequence_parsimony *left = (const AP_sequence_parsimony *)lefts;
    const AP_sequence_parsimony *right = (const AP_sequence_parsimony *)rights;

     size_t sequence_len = get_sequence_length();
    if (seq_pars == NULL) {
        ARB_alloc_aligned(seq_pars, sequence_len + 1);
    }

    const char * p1       = left->get_sequence();
    const char * p2       = right->get_sequence();
    long        result   = 0;

    if (get_weights()->is_unweighted()) {
        if (mutation_per_site) {
            result = do_combine(sequence_len, p1, p2, seq_pars,
                                count_unweighted(), count_mutpsite(mutation_per_site));

        }
        else {
            result = do_combine(sequence_len, p1, p2, seq_pars,
                                count_unweighted(), count_nothing());
        }
    }
    else {
        if (mutation_per_site) {
            result = do_combine(sequence_len, p1, p2, seq_pars,
                                count_weighted(get_weights()->get_weights()),
                                count_mutpsite(mutation_per_site));
        }
        else {
            result = do_combine(sequence_len, p1, p2, seq_pars,
                                count_weighted(get_weights()->get_weights()),
                                count_nothing());
        }
    }

#if defined(DEBUG) && 0
#define P1 75
#define P2 90
    printf("Seq1: ");
    for (long idx = P1; idx <= P2; ++idx) printf("%3i ", p1[idx]);
    printf("\nSeq2: ");
    for (long idx = P1; idx <= P2; ++idx) printf("%3i ", p2[idx]);
    printf("\nCombine value: %f\n", float(result));
#undef P1
#undef P2
#endif // DEBUG

    inc_combine_count();
    mark_sequence_set(true);

    ap_assert(result >= 0.0);
    return (AP_FLOAT)result;
}

void AP_sequence_parsimony::partial_match(const AP_sequence* part_, long *overlapPtr, long *penaltyPtr) const {
    // matches the partial sequences 'part_' against 'this'
    // '*penaltyPtr' is set to the number of mismatches (possibly weighted)
    // '*overlapPtr' is set to the number of base positions both sequences overlap
    //          example:
    //          fullseq      'XXX---XXX'        'XXX---XXX'
    //          partialseq   '-XX---XX-'        '---XXX---'
    //          overlap       7                  3
    //
    // algorithm is similar to AP_sequence_parsimony::combine()
    // Note: changes done here should also be be applied to AP_seq_protein.cxx@partial_match_impl

    const AP_sequence_parsimony *part = (const AP_sequence_parsimony *)part_;

    const char *pf = get_sequence();
    const char *pp = part->get_sequence();

    const AP_weights *weights = get_weights();

    long min_end; // minimum of both last non-gap positions
    for (min_end = get_sequence_length()-1; min_end >= 0; --min_end) {
        char both = pf[min_end]|pp[min_end];
        if (notHasGap(both)) { // last non-gap found
            if (notHasGap(pf[min_end])) { // occurred in full sequence
                for (; min_end >= 0; --min_end) { // search same in partial sequence
                    if (notHasGap(pp[min_end])) break;
                }
            }
            else {
                ap_assert(notHasGap(pp[min_end])); // occurred in partial sequence
                for (; min_end >= 0; --min_end) { // search same in full sequence
                    if (notHasGap(pf[min_end])) break;
                }
            }
            break;
        }
    }

    long penalty = 0;
    long overlap = 0;

    if (min_end >= 0) {
        long max_start; // maximum of both first non-gap positions
        for (max_start = 0; max_start <= min_end; ++max_start) {
            char both = pf[max_start]|pp[max_start];
            if (notHasGap(both)) { // first non-gap found
                if (notHasGap(pf[max_start])) { // occurred in full sequence
                    for (; max_start <= min_end; ++max_start) { // search same in partial
                        if (notHasGap(pp[max_start])) break;
                    }
                }
                else {
                    ap_assert(notHasGap(pp[max_start])); // occurred in partial sequence
                    for (; max_start <= min_end; ++max_start) { // search same in full
                        if (notHasGap(pf[max_start])) break;
                    }
                }
                break;
            }
        }

        if (max_start <= min_end) { // if sequences overlap
            for (long idx = max_start; idx <= min_end; ++idx) {
                if ((pf[idx]&pp[idx]) == 0) { // bases are distinct (aka mismatch)
                    penalty += weights->weight(idx);
                }
            }
            overlap = min_end-max_start+1;
        }
    }

    *overlapPtr = overlap;
    *penaltyPtr = penalty;
}

AP_FLOAT AP_sequence_parsimony::count_weighted_bases() const {
    static char *hits = 0;
    if (!hits) {
        hits = (char*)ARB_alloc(256);
        memset(hits, 1, 256); // count ambiguous characters half

        hits[AP_A] = 2; // count real characters full
        hits[AP_C] = 2;
        hits[AP_G] = 2;
        hits[AP_T] = 2;

        hits[AP_GAP] = 0; // don't count gaps
        hits[AP_DOT] = 0; // don't count dots (and other stuff)
    }

    const AP_weights *weights = get_weights();
    const  char      *p       = get_sequence();

    long   sum          = 0;
    size_t sequence_len = get_sequence_length();

    for (size_t i = 0; i<sequence_len; ++i) {
        sum += hits[safeCharIndex(p[i])] * weights->weight(i);
    }

    AP_FLOAT wcount = sum * 0.5;
    return wcount;
}

uint32_t AP_sequence_parsimony::checksum() const {
    const char *seq = get_sequence();
    return GB_checksum(seq, sizeof(*seq)*get_sequence_length(), 0, NULL);
}

bool AP_sequence_parsimony::equals(const AP_sequence_parsimony *other) const {
    const char *seq  = get_sequence();
    const char *oseq = other->get_sequence();

    size_t len = get_sequence_length();
    for (size_t p = 0; p<len; ++p) {
        if (seq[p] != oseq[p]) return false;
    }
    return true;
}

