#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <awt_tree.hxx>
#include "awt_seq_protein.hxx"
#include "awt_parsimony_defaults.hxx"

#include <inline.h>

#define awt_assert(bed) arb_assert(bed)

extern long global_combineCount;

// start of implementation of class AP_sequence_protein:

AP_sequence_protein::AP_sequence_protein(AP_tree_root *tree_root)
    : AP_sequence(tree_root)
{
    sequence = 0;
}

AP_sequence_protein::~AP_sequence_protein()
{
    if (sequence != 0) delete sequence;
    sequence = 0;
}

AP_sequence *AP_sequence_protein::dup()
{
    return new AP_sequence_protein(root);
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

static AP_PROTEINS prot2test[PROTEINS_TO_TEST] = {
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

static int prot_idx[PROTEINS_TO_TEST] = { // needs same indexing as prot2test; contains indexes for 'awt_pro_a_nucs->dist'
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
static int  min_mutations_initialized_time_stamp = 0;

static void update_min_mutations() {
    for (int d = 0; d<PROTEINS_TO_TEST; ++d) {
        int D     = prot_idx[d];
        int D_bit = 1<<D;

        for (int s = 0; s<PROTEINS_TO_TEST; ++s) {
            int S = prot_idx[s];
            int i;

            for (i = 0; i<3; ++i) {
                if (awt_pro_a_nucs->dist[S]->patd[i] & D_bit) {
                    break;
                }
            }

            prot_mindist[s][d] = char(i);
        }
    }

    min_mutations_initialized_time_stamp = awt_pro_a_nucs->time_stamp;
}


#if defined(DEBUG)
// #define SHOW_SEQ
#endif // DEBUG

void AP_sequence_protein::set(const char *isequence)
{
    if (!awt_pro_a_nucs) awt_pro_a_nucs_gen_dist(this->root->gb_main);
    if (awt_pro_a_nucs->time_stamp != min_mutations_initialized_time_stamp) update_min_mutations();

    sequence_len = root->filter->real_len;
    sequence     = new AP_PROTEINS[sequence_len+1];

    awt_assert(root->filter->bootstrap == 0); // bootstrapping not implemented for protein parsimony

    const uchar *simplify   = root->filter->simplify;
    char        *filt       = root->filter->filter_mask;
    int          left_bases = sequence_len;
    long         filter_len = root->filter->filter_len;

    awt_assert(filt);

    int oidx = 0;               // index for output sequence

    for (int idx = 0; idx<filter_len && left_bases; ++idx) {
        if (filt[idx]) {
            char        c = toupper(simplify[safeCharIndex(isequence[idx])]);
            AP_PROTEINS p = APP_ILLEGAL;

#if defined(SHOW_SEQ)
            fputc(c, stdout);
#endif // SHOW_SEQ


            if (c >= 'A' && c <= 'Z')       p = prot2AP_PROTEIN[c-'A'];
            else if (c == '-' || c == '.')  p = APP_GAP;
            else if (c == '*')              p = APP_STAR;

            if (p == APP_ILLEGAL) {
                aw_message(GBS_global_string("Illegal sequence character '%c' replaced by gap", c));
                p = APP_GAP;
            }

            sequence[oidx++] = p;
            --left_bases;
        }
    }

    awt_assert(oidx == sequence_len);
    sequence[sequence_len] = APP_ILLEGAL;

#if defined(SHOW_SEQ)
    fputc('\n', stdout);
#endif // SHOW_SEQ

    update          = AP_timer();
    is_set_flag     = AP_TRUE;
    cashed_real_len = -1.0;
}

//     for (int idx = 0; idx<sequence_len; ++idx) {
//         char        c = toupper(isequence[idx]);
//         AP_PROTEINS p = APP_ILLEGAL;

//         if (c >= 'A' && c <= 'Z')       p = prot2AP_PROTEIN[c-'A'];
//         else if (c == '-' || c == '.')  p = APP_GAP;
//         else if (c == '*')              p = APP_STAR;

//         if (p == APP_ILLEGAL) {
//             awt_assert(0);
//             p = APP_GAP;
//         }

//         sequence[idx] = p;
//     }



AP_FLOAT AP_sequence_protein::combine(  const AP_sequence * lefts, const    AP_sequence *rights)
{
    // this works similar to AP_sequence_parsimony::combine

    const AP_sequence_protein *left  = (const AP_sequence_protein *)lefts;
    const AP_sequence_protein *right = (const AP_sequence_protein *)rights;

    if (!sequence) {
        sequence_len = root->filter->real_len;
        sequence = new AP_PROTEINS[sequence_len +1];
    }

    const AP_PROTEINS *p1 = left->sequence;
    const AP_PROTEINS *p2 = right->sequence;
    AP_PROTEINS       *p  = sequence;

    GB_UINT4 *w        = 0;
    char     *mutpsite = 0;

    if (mutation_per_site) { // count site specific mutations in mutation_per_site
        w        = root->weights->weights;
        mutpsite = mutation_per_site;
    }
    else if (root->weights->dummy_weights) { // no weights, no mutation_per_site
        ;
    }
    else { // weighted (but don't count mutation_per_site)
        w = root->weights->weights;
    }

    long result = 0;
    awt_assert(min_mutations_initialized_time_stamp == awt_pro_a_nucs->time_stamp); // check if initialized for correct instance of awt_pro_a_nucs

    for (long idx = 0; idx<sequence_len; ++idx) {
        AP_PROTEINS c1 = p1[idx];
        AP_PROTEINS c2 = p2[idx];

        awt_assert(c1 != APP_ILLEGAL);
        awt_assert(c2 != APP_ILLEGAL);

        if ((c1&c2) == 0) { // proteins are distinct
            p[idx] = AP_PROTEINS(c1|c2);     // mix distinct proteins

            int mutations = INT_MAX;

            if (p[idx]&APP_GAP) { // contains a gap
                mutations = 1;  // count first gap as mutation
                // @@@ FIXME:  rethink then line above. maybe it should be 3 mutations ?

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

            awt_assert(mutations >= 0 && mutations <= 3);

            if (mutpsite) mutpsite[idx] += mutations; // count mutations per site (unweighted)
            result                      += mutations * (w ? w[idx] : 1); // count weighted or simple

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

    global_combineCount++;
    this->is_set_flag = AP_TRUE;
    cashed_real_len   = -1.0;

    awt_assert(result >= 0.0);

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
    const AP_PROTEINS         *pf   = sequence;
    const AP_PROTEINS         *pp   = part->sequence;

    GB_UINT4 *w                          = 0;
    if (!root->weights->dummy_weights) w = root->weights->weights;

    long min_end; // minimum of both last non-gap positions

    for (min_end = sequence_len-1; min_end >= 0; --min_end) {
        AP_PROTEINS both = AP_PROTEINS(pf[min_end]|pp[min_end]);
        if (both != APP_GAP) { // last non-gap found
            if (pf[min_end] != APP_GAP) { // occurred in full sequence
                for (; min_end >= 0; --min_end) { // search same in partial sequence
                    if (pp[min_end] != APP_GAP) break;
                }
            }
            else {
                awt_assert(pp[min_end] != APP_GAP); // occurred in partial sequence
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
                    awt_assert(pp[max_start] != APP_GAP); // occurred in partial sequence
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
                    penalty += (w ? w[idx] : 1)*mutations;
                }
            }
            overlap = (min_end-max_start+1)*3;
        }
    }

    *overlapPtr = overlap;
    *penaltyPtr = penalty;
}

AP_FLOAT AP_sequence_protein::real_len()
{
    if (!sequence) return -1.0;
    if (cashed_real_len>=0.0) return cashed_real_len;

    long sum = 0;
    for (long idx = 0; idx<sequence_len; ++idx) {
        if (sequence[idx] != APP_GAP) {
            ++sum;
        }
    }
    cashed_real_len = sum;
    return sum;
}

// -end- of implementation of class AP_sequence_protein.



