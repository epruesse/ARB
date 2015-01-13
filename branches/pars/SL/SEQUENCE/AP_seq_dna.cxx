#include "AP_seq_dna.hxx"
#include "AP_parsimony_defaults.hxx"

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
    , seq_pars(0)
{
}

AP_sequence_parsimony::~AP_sequence_parsimony() {
    delete [] seq_pars;
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
    // UNCOVERED(); // covered by TEST_calc_bootstraps
    size_t sequence_len = get_filter()->get_filtered_length();
    seq_pars     = new char[sequence_len+1];
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
        int left_bases = sequence_len;
        int filter_len = filt->get_length();
        int oidx       = 0;                         // for output sequence

        for (int idx = 0; idx<filter_len && left_bases; ++idx) {
            if (filt->use_position(idx)) {
                unsigned char c = (unsigned char)isequence[idx];
                seq_pars[oidx++] = table[simplify[c]];
                --left_bases;
#if defined(SHOW_SEQ)
                fputc(simplify[c], stdout);
#endif                          // SHOW_SEQ
            }
        }
    }

#if defined(SHOW_SEQ)
    fputc('\n', stdout);
#endif // SHOW_SEQ

    mark_sequence_set(true);
}

void AP_sequence_parsimony::unset() {
    delete [] seq_pars; seq_pars = 0;
    mark_sequence_set(false);
}

#if !defined(AP_PARSIMONY_DEFAULTS_HXX)
#error AP_parsimony_defaults.hxx not included
#endif // AP_PARSIMONY_DEFAULTS_HXX

AP_FLOAT AP_sequence_parsimony::combine(const AP_sequence *lefts, const AP_sequence *rights, char *mutation_per_site) {
    // Note: changes done here should also be be applied to AP_seq_protein.cxx@combine_impl

    const AP_sequence_parsimony *left  = (const AP_sequence_parsimony *)lefts;
    const AP_sequence_parsimony *right = (const AP_sequence_parsimony *)rights;

    // UNCOVERED(); // covered by TEST_calc_bootstraps + TEST_tree_quick_add_marked
    size_t sequence_len = get_sequence_length();
    if (seq_pars == 0) {
        seq_pars = new char[sequence_len + 1];
    }

    const char *p1       = left->get_sequence();
    const char *p2       = right->get_sequence();
    char       *p        = seq_pars;
    char       *mutpsite = mutation_per_site;
    long        result   = 0;

    const AP_weights *weights = get_weights();

    for (size_t idx = 0; idx<sequence_len; ++idx) {
        char c1 = p1[idx];
        char c2 = p2[idx];

        ap_assert(c1 != AP_ILLEGAL);
        ap_assert(c2 != AP_ILLEGAL);

        if ((c1&c2) == 0) {    // bases are distinct (that means we count a mismatch)
            p[idx] = c1|c2;     // mix distinct bases

#if !defined(MULTIPLE_GAPS_ARE_MULTIPLE_MUTATIONS)
            // count multiple mutations as 1 mutation
            // (code here is unused)
            if (hasGap(p[idx])) {  // contains a gap
                if (idx>0 && hasGap(p[idx-1])) { // last position also contained gap..
                    if ((hasGap(c1) && hasGap(p1[idx-1])) || // ..in same sequence
                        (hasGap(c2) && hasGap(p2[idx-1])))
                    {
                        if (notHasGap(p1[idx-1]) || notHasGap(p2[idx-1])) { // if one of the sequences had no gap at previous position
                            continue; // skip multiple gaps
                        }
                    }
                }
            }
#endif // MULTIPLE_GAPS_ARE_MULTIPLE_MUTATIONS

            // now count mutation :

            if (mutpsite) mutpsite[idx]++;          // count mutations per site (unweighted)
            result += weights->weight(idx);         // count weighted or simple
        }
        else {
            p[idx] = c1&c2; // store common bases for both subtrees
        }


#if !defined(PROPAGATE_GAPS_UPWARDS)
        // do not propagate mixed gaps upwards (they cause neg. branches)
        // (code here is unused)
        if (hasGap(p[idx])) {
            if (notIsGap(p[idx])) {
                p[idx] = AP_BASES(p[idx]^AP_GAP);
            }
        }
#endif // PROPAGATE_GAPS_UPWARDS

        ap_assert(p[idx] != 0);
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

    // UNCOVERED(); // covered by TEST_tree_add_marked
    const char *pf = get_sequence();
    const char *pp = part->get_sequence();

    const AP_weights *weights = get_weights();

    // @@@ to fix #609 replace "IsGap" by "HasGap" below (also use & instead of | for 'both', 'both' as used here in fact means 'any')

    long min_end; // minimum of both last non-gap positions
    for (min_end = get_sequence_length()-1; min_end >= 0; --min_end) {
        char both = pf[min_end]|pp[min_end];
        if (notIsGap(both)) { // last non-gap found
            if (notIsGap(pf[min_end])) { // occurred in full sequence
                for (; min_end >= 0; --min_end) { // search same in partial sequence
                    if (notIsGap(pp[min_end])) break;
                }
            }
            else {
                ap_assert(notIsGap(pp[min_end])); // occurred in partial sequence
                for (; min_end >= 0; --min_end) { // search same in full sequence
                    if (notIsGap(pf[min_end])) break;
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
            if (notIsGap(both)) { // first non-gap found
                if (notIsGap(pf[max_start])) { // occurred in full sequence
                    for (; max_start <= min_end; ++max_start) { // search same in partial
                        if (notIsGap(pp[max_start])) break;
                    }
                }
                else {
                    ap_assert(notIsGap(pp[max_start])); // occurred in partial sequence
                    for (; max_start <= min_end; ++max_start) { // search same in full
                        if (notIsGap(pf[max_start])) break;
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
        hits = (char*)malloc(256);
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

    // UNCOVERED(); // covered by TEST_calc_bootstraps

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

