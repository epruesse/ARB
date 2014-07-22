#include "AP_seq_protein.hxx"
#include "AP_parsimony_defaults.hxx"
#include <AP_pro_a_nucs.hxx>

#include <AP_filter.hxx>
#include <ARB_Tree.hxx>

#include <arb_str.h>
#include <climits>


// #define ap_assert(bed) arb_assert(bed)

AP_sequence_protein::AP_sequence_protein(const AliView *aliview)
    : AP_sequence(aliview)
    , seq_prot(NULL)
{
}

AP_sequence_protein::~AP_sequence_protein() {
    delete [] seq_prot;
}

AP_sequence *AP_sequence_protein::dup() const {
    return new AP_sequence_protein(get_aliview());
}

static AP_PROTEINS prot2AP_PROTEIN[26] = {
    APP_A,
    APP_B,
    APP_C,
    APP_D,
    APP_E,
    APP_F,
    APP_G,
    APP_H,
    APP_I,
    APP_ILLEGAL,
    APP_K,
    APP_L,
    APP_M,
    APP_N,
    APP_ILLEGAL,
    APP_P,
    APP_Q,
    APP_R,
    APP_S,
    APP_T,
    APP_ILLEGAL,
    APP_V,
    APP_W,
    APP_X,
    APP_Y,
    APP_Z
};

#define PROTEINS_TO_TEST 22

static AP_PROTEINS prot2test[PROTEINS_TO_TEST] = { // uses same indexing as prot_idx
    APP_STAR,
    APP_A,
    APP_C,
    APP_D,
    APP_E,
    APP_F,
    APP_G,
    APP_H,
    APP_I,
    APP_K,
    APP_L,
    APP_M,
    APP_N,
    APP_P,
    APP_Q,
    APP_R,
    APP_S,
    APP_T,
    APP_V,
    APP_W,
    APP_Y,
    APP_GAP
};

static int prot_idx[PROTEINS_TO_TEST] = { // uses same indexing as prot2test
    // contains indexes for 'AWT_distance_meter->dist'
    0,                          // *
    1,                          // A
    3,                          // C
    4,                          // D
    5,                          // E
    6,                          // F
    7,                          // G
    8,                          // H
    9,                          // I
    10,                         // K
    11,                         // L
    12,                         // M
    13,                         // N
    14,                         // P
    15,                         // Q
    16,                         // R
    17,                         // S
    18,                         // T
    19,                         // V
    20,                         // W
    21,                         // Y
    23                          // gap
};

static char prot_mindist[PROTEINS_TO_TEST][PROTEINS_TO_TEST];
static int  min_mutations_initialized_for_codenr = -1;

static void update_min_mutations(int code_nr, const AWT_distance_meter *distance_meter) {
    if (min_mutations_initialized_for_codenr != code_nr) {
        for (int d = 0; d<PROTEINS_TO_TEST; ++d) {
            int D     = prot_idx[d];
            int D_bit = 1<<D;

            for (int s = 0; s<PROTEINS_TO_TEST; ++s) {
                const AWT_PDP *dist = distance_meter->getDistance(prot_idx[s]);

                int i;
                for (i = 0; i<3; ++i) {
                    if (dist->patd[i] & D_bit) break;
                }

                prot_mindist[s][d] = char(i);
            }
        }

        min_mutations_initialized_for_codenr = code_nr;
    }
}


#if defined(DEBUG)
// #define SHOW_SEQ
#endif // DEBUG

void AP_sequence_protein::set(const char *isequence) {
    AWT_translator *translator = AWT_get_user_translator(get_aliview()->get_gb_main());
    update_min_mutations(translator->CodeNr(), translator->getDistanceMeter());

    size_t sequence_len = get_sequence_length();
    seq_prot            = new AP_PROTEINS[sequence_len+1];

    ap_assert(!get_filter()->does_bootstrap()); // bootstrapping not implemented for protein parsimony

    const AP_filter *filt       = get_filter();
    const uchar     *simplify   = filt->get_simplify_table();
    int              left_bases = sequence_len;
    long             filter_len = filt->get_length();

    ap_assert(filt);

    size_t oidx = 0;               // index for output sequence

    for (int idx = 0; idx<filter_len && left_bases; ++idx) {
        if (filt->use_position(idx)) {
            char        c = toupper(simplify[safeCharIndex(isequence[idx])]);
            AP_PROTEINS p = APP_ILLEGAL;

#if defined(SHOW_SEQ)
            fputc(c, stdout);
#endif // SHOW_SEQ

            if (c >= 'A' && c <= 'Z') p = prot2AP_PROTEIN[c-'A'];
            else if (c == '-')        p = APP_GAP;
            else if (c == '.')        p = APP_X;
            else if (c == '*')        p = APP_STAR;

            if (p == APP_ILLEGAL) {
                GB_warning(GBS_global_string("Illegal sequence character '%c' replaced by gap", c));
                p = APP_GAP;
            }

            seq_prot[oidx++] = p;
            --left_bases;
        }
    }

    ap_assert(oidx == sequence_len);
    seq_prot[sequence_len] = APP_ILLEGAL;

#if defined(SHOW_SEQ)
    fputc('\n', stdout);
#endif // SHOW_SEQ

    mark_sequence_set(true);
}

void AP_sequence_protein::unset() {
    delete [] seq_prot;
    seq_prot = 0;
    mark_sequence_set(false);
}

#if !defined(AP_PARSIMONY_DEFAULTS_HXX)
#error AP_parsimony_defaults.hxx not included
#endif // AP_PARSIMONY_DEFAULTS_HXX


AP_FLOAT AP_sequence_protein::combine(const AP_sequence *lefts, const AP_sequence *rights, char *mutation_per_site) {
    // this works similar to AP_sequence_parsimony::combine

    const AP_sequence_protein *left  = (const AP_sequence_protein *)lefts;
    const AP_sequence_protein *right = (const AP_sequence_protein *)rights;

    size_t sequence_len = get_sequence_length();
    if (!seq_prot) seq_prot = new AP_PROTEINS[sequence_len + 1];

    const AP_PROTEINS *p1       = left->get_sequence();
    const AP_PROTEINS *p2       = right->get_sequence();
    AP_PROTEINS       *p        = seq_prot;
    const AP_weights  *weights  = get_weights();
    char              *mutpsite = mutation_per_site;

    long result = 0;
    // check if initialized for correct instance of translator:
    ap_assert(min_mutations_initialized_for_codenr == AWT_get_user_translator()->CodeNr());

    for (size_t idx = 0; idx<sequence_len; ++idx) {
        AP_PROTEINS c1 = p1[idx];
        AP_PROTEINS c2 = p2[idx];

        ap_assert(c1 != APP_ILLEGAL);
        ap_assert(c2 != APP_ILLEGAL);

        if ((c1&c2) == 0) { // proteins are distinct
            p[idx] = AP_PROTEINS(c1|c2);     // mix distinct proteins

            int mutations = INT_MAX;

            if (p[idx]&APP_GAP) { // contains a gap
                mutations = 1;  // count first gap as mutation
                // @@@ FIXME:  rethink the line above. maybe it should be 3 mutations ?

#if !defined(MULTIPLE_GAPS_ARE_MULTIPLE_MUTATIONS)
                // count multiple mutations as 1 mutation
                // this was an experiment (don't use it, it does not work!)
                if (idx>0 && (p[idx-1]&APP_GAP)) { // last position also contained gap..
                    if (((c1&APP_GAP) && (p1[idx-1]&APP_GAP)) || // ..in same sequence
                        ((c2&APP_GAP) && (p2[idx-1]&APP_GAP)))
                    {
                        if (!(p1[idx-1]&APP_GAP) || !(p2[idx-1]&APP_GAP)) { // if one of the sequences had no gap at previous position
                            mutations = 0; // skip multiple gaps
                        }
                    }
                }
#endif // MULTIPLE_GAPS_ARE_MULTIPLE_MUTATIONS
            }
            else {
                for (int t1 = 0; t1<PROTEINS_TO_TEST && mutations>1; ++t1) { // with all proteins to test
                    if (c1&prot2test[t1]) { // if protein is contained in subtree
                        for (int t2 = 0; t2<PROTEINS_TO_TEST; ++t2) {
                            if (c2&prot2test[t2]) {
                                int mut = prot_mindist[t1][t2];
                                if (mut<mutations) {
                                    mutations = mut;
                                    if (mutations < 2) break; // minimum reached -- abort
                                }
                            }
                        }
                    }
                }
            }

            ap_assert(mutations >= 0 && mutations <= 3);

            if (mutpsite) mutpsite[idx] += mutations; // count mutations per site (unweighted)
            result += mutations * weights->weight(idx); // count weighted or simple

        }
        else {
            p[idx] = AP_PROTEINS(c1&c2); // store common proteins for both subtrees
        }

#if !defined(PROPAGATE_GAPS_UPWARDS)
        // do not propagate mixed gaps upwards (they cause neg. branches)
        if (p[idx] & APP_GAP) { // contains gap
            if (p[idx] != APP_GAP) { // is not real gap
                p[idx] = AP_PROTEINS(p[idx]^APP_GAP); //  remove the gap
            }
        }
#endif // PROPAGATE_GAPS_UPWARDS
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

#if defined(DEBUG) && 0
    printf("\nCombine value: %f\n", float(result));
#endif // DEBUG

    inc_combine_count();
    mark_sequence_set(true);

    ap_assert(result >= 0.0);
    return (AP_FLOAT)result;
}

void AP_sequence_protein::partial_match(const AP_sequence* part_, long *overlapPtr, long *penaltyPtr) const {
    // matches the partial sequences 'part_' against 'this'
    // '*penaltyPtr' is set to the number of mismatches (possibly weighted)
    // '*overlapPtr' is set to the number of base positions both sequences overlap
    //          example:
    //          fullseq      'XXX---XXX'        'XXX---XXX'
    //          partialseq   '-XX---XX-'        '---XXX---'
    //          overlap       7                  3
    //
    // algorithm is similar to AP_sequence_protein::combine()

    const AP_sequence_protein *part = (const AP_sequence_protein *)part_;

    const AP_PROTEINS *pf      = get_sequence();
    const AP_PROTEINS *pp      = part->get_sequence();
    const AP_weights  *weights = get_weights();

    long min_end;                                   // minimum of both last non-gap positions
    for (min_end = get_sequence_length()-1; min_end >= 0; --min_end) {
        AP_PROTEINS both = AP_PROTEINS(pf[min_end]|pp[min_end]);
        if (both != APP_GAP) { // last non-gap found
            if (pf[min_end] != APP_GAP) { // occurred in full sequence
                for (; min_end >= 0; --min_end) { // search same in partial sequence
                    if (pp[min_end] != APP_GAP) break;
                }
            }
            else {
                ap_assert(pp[min_end] != APP_GAP); // occurred in partial sequence
                for (; min_end >= 0; --min_end) { // search same in full sequence
                    if (pf[min_end] != APP_GAP) break;
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
            AP_PROTEINS both = AP_PROTEINS(pf[max_start]|pp[max_start]);
            if (both != APP_GAP) { // first non-gap found
                if (pf[max_start] != APP_GAP) { // occurred in full sequence
                    for (; max_start <= min_end; ++max_start) { // search same in partial
                        if (pp[max_start] != APP_GAP) break;
                    }
                }
                else {
                    ap_assert(pp[max_start] != APP_GAP); // occurred in partial sequence
                    for (; max_start <= min_end; ++max_start) { // search same in full
                        if (pf[max_start] != APP_GAP) break;
                    }
                }
                break;
            }
        }

        if (max_start <= min_end) { // if sequences overlap
            for (long idx = max_start; idx <= min_end; ++idx) {
                if ((pf[idx]&pp[idx]) == 0) { // bases are distinct (aka mismatch)
                    int mutations;
                    if ((pf[idx]|pp[idx])&APP_GAP) { // one is a gap
                        mutations = 3;
                    }
                    else {
                        mutations = INT_MAX;
                        for (int t1 = 0; t1<PROTEINS_TO_TEST && mutations>1; ++t1) { // with all proteins to test
                            if (pf[idx] & prot2test[t1]) { // if protein is contained in subtree
                                for (int t2 = 0; t2<PROTEINS_TO_TEST; ++t2) {
                                    if (pp[idx] & prot2test[t2]) {
                                        int mut = prot_mindist[t1][t2];
                                        if (mut<mutations) {
                                            mutations = mut;
                                            if (mutations < 2) break; // minimum reached -- abort
                                        }
                                    }
                                }
                            }
                        }
                    }
                    penalty += weights->weight(idx)*mutations;
                }
            }
            overlap = (min_end-max_start+1)*3;
        }
    }

    *overlapPtr = overlap;
    *penaltyPtr = penalty;
}

AP_FLOAT AP_sequence_protein::count_weighted_bases() const {
    AP_FLOAT           wcount;
    const AP_PROTEINS *sequence = get_sequence();

    if (!sequence) wcount = -1.0;
    else {
        long   sum          = 0;
        size_t sequence_len = get_sequence_length();

        const AP_weights *weights = get_weights();

        for (size_t idx = 0; idx<sequence_len; ++idx) {
            if (sequence[idx] != APP_GAP) {
                sum += weights->weight(idx);
            }
        }
        wcount = sum;
    }
    return wcount;
}





