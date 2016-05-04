#include "AP_seq_dna.hxx"
#include "AP_parsimony_defaults.hxx"

#include <AP_pro_a_nucs.hxx>
#include <AP_filter.hxx>
#include <ARB_Tree.hxx>


char *AP_sequence_parsimony::table;

/*!***************************


******************************/

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

void AP_sequence_parsimony::set(const char *isequence)
{
    size_t sequence_len = get_filter()->get_filtered_length();
    seq_pars     = new char[sequence_len+1];
    memset(seq_pars, AP_N, (size_t)sequence_len+1);

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
    const AP_sequence_parsimony *left = (const AP_sequence_parsimony *)lefts;
    const AP_sequence_parsimony *right = (const AP_sequence_parsimony *)rights;

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

        ap_assert(c1 != 0); // not a base and not a gap -- what should it be ?
        ap_assert(c2 != 0);

        if ((c1&c2) == 0) {    // bases are distinct (that means we count a mismatch)
            p[idx] = c1|c2;     // mix distinct bases

#if !defined(MULTIPLE_GAPS_ARE_MULTIPLE_MUTATIONS)
            // count multiple mutations as 1 mutation
            // this was an experiment (don't use it, it does not work!)
            if (p[idx]&AP_S) {  // contains a gap
                if (idx>0 && (p[idx-1]&AP_S)) { // last position also contained gap..
                    if (((c1&AP_S) && (p1[idx-1]&AP_S)) || // ..in same sequence
                        ((c2&AP_S) && (p2[idx-1]&AP_S)))
                    {
                        if (!(p1[idx-1]&AP_S) || !(p2[idx-1]&AP_S)) { // if one of the sequences had no gap at previous position
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
        // this was an experiment (don't use it, it does not work!)
        if (p[idx] & AP_S) {
            if (p[idx] != AP_S) {
                p[idx] = AP_BASES(p[idx]^AP_S);
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

    const AP_sequence_parsimony *part = (const AP_sequence_parsimony *)part_;

    const char *pf = get_sequence();
    const char *pp = part->get_sequence();

    const AP_weights *weights = get_weights();

    long min_end;                                   // minimum of both last non-gap positions

    for (min_end = get_sequence_length()-1; min_end >= 0; --min_end) {
        char both = pf[min_end]|pp[min_end];
        if (both != AP_S) { // last non-gap found
            if (pf[min_end] != AP_S) { // occurred in full sequence
                for (; min_end >= 0; --min_end) { // search same in partial sequence
                    if (pp[min_end] != AP_S) break;
                }
            }
            else {
                ap_assert(pp[min_end] != AP_S); // occurred in partial sequence
                for (; min_end >= 0; --min_end) { // search same in full sequence
                    if (pf[min_end] != AP_S) break;
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
            if (both != AP_S) { // first non-gap found
                if (pf[max_start] != AP_S) { // occurred in full sequence
                    for (; max_start <= min_end; ++max_start) { // search same in partial
                        if (pp[max_start] != AP_S) break;
                    }
                }
                else {
                    ap_assert(pp[max_start] != AP_S); // occurred in partial sequence
                    for (; max_start <= min_end; ++max_start) { // search same in full
                        if (pf[max_start] != AP_S) break;
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



AP_FLOAT AP_sequence_parsimony::count_weighted_bases() const { // count all bases
    static char *hits = 0;
    if (!hits) {
        hits = (char*)malloc(256);
        memset(hits, 1, 256);                       // count ambiguous characters half

        hits[AP_A] = 2;                             // real characters full
        hits[AP_C] = 2;
        hits[AP_G] = 2;
        hits[AP_T] = 2;
        hits[AP_S] = 0;                             // count no gaps
        hits[AP_N] = 0;                             // no Ns
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

