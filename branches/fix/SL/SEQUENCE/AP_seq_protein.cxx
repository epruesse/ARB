#include "AP_seq_protein.hxx"
#include <AP_pro_a_nucs.hxx>

#include <AP_filter.hxx>
#include <ARB_Tree.hxx>

#include <arb_str.h>
#include <climits>

inline bool hasGap(AP_PROTEINS c) { return c & APP_GAP; }
inline bool isGap(AP_PROTEINS c)  { return c == APP_GAP; }

inline bool notHasGap(AP_PROTEINS c) { return !hasGap(c); }
inline bool notIsGap(AP_PROTEINS c)  { return !isGap(c); }

// #define ap_assert(bed) arb_assert(bed)

AP_sequence_protein::AP_sequence_protein(const AliView *aliview)
    : AP_sequence(aliview),
      seq_prot(NULL),
      mut1(NULL),
      mut2(NULL)
{
}

AP_sequence_protein::~AP_sequence_protein() {
    delete [] mut2;
    delete [] mut1;
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
    APP_ILLEGAL, // J
    APP_K,
    APP_L,
    APP_M,
    APP_N,
    APP_ILLEGAL, // O
    APP_P,
    APP_Q,
    APP_R,
    APP_S,
    APP_T,
    APP_ILLEGAL, // U
    APP_V,
    APP_W,
    APP_X,
    APP_Y,
    APP_Z
};

#define PROTEINS_TO_TEST 22 // 26 plus gap and star, minus 3 illegal, 'X', 'B' and 'Z'

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

inline const char *readable_combined_protein(AP_PROTEINS p) {
    if (p == APP_X) { return "X"; }
    if (p == APP_DOT) { return "."; }

    static char buffer[PROTEINS_TO_TEST+1];
    memset(buffer, 0, PROTEINS_TO_TEST+1);
    char        *bp       = buffer;
    const char  *readable = "*ACDEFGHIKLMNPQRSTVWY-"; // same index as prot2test

    for (int b = 0; b<PROTEINS_TO_TEST; ++b) {
        if (p&prot2test[b]) {
            *bp++ = readable[b];
        }
    }
    return buffer;
}

static char prot_mindist[PROTEINS_TO_TEST][PROTEINS_TO_TEST];
static int  min_mutations_initialized_for_codenr = -1;

// OMA = one mutation away
// (speedup for huge table is approx. 4-7%)
#define OMA_SLOW_LOWMEM

#if defined(ASSERTION_USED) && 0
#define OMA_DOUBLE_CHECK
#endif

#if defined(OMA_DOUBLE_CHECK)
# define IMPL_OMA_SLOW_LOWMEM
# define IMPL_OMA_FAST_BIGMEM
#else
# if defined(OMA_SLOW_LOWMEM)
#  define IMPL_OMA_SLOW_LOWMEM
# else
#  define IMPL_OMA_FAST_BIGMEM
# endif
#endif

STATIC_ASSERT(APP_MAX == 4194303);
STATIC_ASSERT(sizeof(AP_PROTEINS) == 4);

#if defined(IMPL_OMA_FAST_BIGMEM)

static AP_PROTEINS one_mutation_away[APP_MAX+1]; // contains all proteins that are <= 1 nuc-mutations away from protein-combination used as index
STATIC_ASSERT(sizeof(one_mutation_away) == 16777216); // ~ 16Mb

#endif

#if defined(IMPL_OMA_SLOW_LOWMEM)

static AP_PROTEINS one_mutation_away_0_7[256]; 
static AP_PROTEINS one_mutation_away_8_15[256];
static AP_PROTEINS one_mutation_away_16_23[256];

inline AP_PROTEINS calcOneMutationAway(AP_PROTEINS p) {
    return AP_PROTEINS(one_mutation_away_0_7  [ p      & 0xff] |
                       one_mutation_away_8_15 [(p>>8)  & 0xff] |
                       one_mutation_away_16_23[(p>>16) & 0xff]);
}

#endif


inline AP_PROTEINS oneMutationAway(AP_PROTEINS p) {
#if defined(OMA_SLOW_LOWMEM)
    return calcOneMutationAway(p);
#else
    return one_mutation_away[p];
#endif
}

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


#if defined(IMPL_OMA_FAST_BIGMEM)
        memset(one_mutation_away, 0, sizeof(one_mutation_away));
#endif
#if defined(IMPL_OMA_SLOW_LOWMEM)
        memset(one_mutation_away_0_7,   0, sizeof(one_mutation_away_0_7));
        memset(one_mutation_away_8_15,  0, sizeof(one_mutation_away_8_15));
        memset(one_mutation_away_16_23, 0, sizeof(one_mutation_away_16_23));
#endif

        for (int s = 0; s<PROTEINS_TO_TEST; ++s) {
            AP_PROTEINS oma = APP_ILLEGAL;
            for (int d = 0; d<PROTEINS_TO_TEST; ++d) {
                if (prot_mindist[s][d] == 1) {
                    oma = AP_PROTEINS(oma|prot2test[d]);
                }
            }

            AP_PROTEINS source = prot2test[s];
            oma                = AP_PROTEINS(oma|source);

#if defined(IMPL_OMA_FAST_BIGMEM)
            one_mutation_away[source] = oma;
#endif
#if defined(IMPL_OMA_SLOW_LOWMEM)
            uint32_t idx =  source      & 0xff; if (idx) one_mutation_away_0_7  [idx] = oma;
            idx          = (source>>8)  & 0xff; if (idx) one_mutation_away_8_15 [idx] = oma;
            idx          = (source>>16) & 0xff; if (idx) one_mutation_away_16_23[idx] = oma;
#endif
        }

#if defined(IMPL_OMA_FAST_BIGMEM)
        for (size_t i = 0; i<=APP_MAX; ++i) {
            if (one_mutation_away[i] == APP_ILLEGAL) {
                size_t      j   = i;
                size_t      b   = 1;
                AP_PROTEINS oma = APP_ILLEGAL;

                while (j) {
                    if (j&1) oma = AP_PROTEINS(oma|one_mutation_away[b]);
                    j >>= 1;
                    b <<= 1;
                }

                one_mutation_away[i] = oma;
            }
        }
#endif
#if defined(IMPL_OMA_SLOW_LOWMEM)
        for (int s = 0; s<8; s++) {
            int b = 1<<s;
            for (int i=b+1; i<256; i++) {
                if (i & b) {
                    one_mutation_away_0_7[i]   = AP_PROTEINS(one_mutation_away_0_7[i]   | one_mutation_away_0_7[b]);
                    one_mutation_away_8_15[i]  = AP_PROTEINS(one_mutation_away_8_15[i]  | one_mutation_away_8_15[b]);
                    one_mutation_away_16_23[i] = AP_PROTEINS(one_mutation_away_16_23[i] | one_mutation_away_16_23[b]);
                }
            }
        }
#endif

#if defined(IMPL_OMA_FAST_BIGMEM) && defined(DEBUG)
        for (size_t i = 0; i<=APP_MAX; ++i) {
            if (one_mutation_away[i] == 0) {
                fprintf(stderr, "oma[%s] is zero\n", readable_combined_protein(AP_PROTEINS(i)));
            }
        }
        for (size_t i = 0; i<=APP_MAX; ++i) {
            AP_PROTEINS two_mutations_away = one_mutation_away[one_mutation_away[i]];
            bool        gap                  = hasGap(AP_PROTEINS(i));
            if ((!gap && two_mutations_away != APP_X) || (gap && two_mutations_away != APP_DOT)) {
                // reached for a few amino-acid-combinations: C, F, C|F, K, M, K|M
                // and for APP_ILLEGAL and APP_GAP as below for 3 mutations
                fprintf(stderr, "tma[%s]", readable_combined_protein(AP_PROTEINS(i)));
                fprintf(stderr, "=%s\n", readable_combined_protein(two_mutations_away));
            }
        }
        for (size_t i = 0; i<=APP_MAX; ++i) {
            AP_PROTEINS three_mutations_away = one_mutation_away[one_mutation_away[one_mutation_away[i]]];
            bool        gap                  = hasGap(AP_PROTEINS(i));
            if ((!gap && three_mutations_away != APP_X) || (gap && three_mutations_away != APP_DOT)) {
                // only reached for i==APP_ILLEGAL and i==APP_GAP (result is wrong for latter)
                fprintf(stderr, "3ma[%s]", readable_combined_protein(AP_PROTEINS(i)));
                fprintf(stderr, "=%s\n", readable_combined_protein(three_mutations_away));
            }
        }
#endif

#if defined(OMA_DOUBLE_CHECK)
        for (size_t i = 0; i<=APP_MAX; ++i) {
            AP_PROTEINS p = AP_PROTEINS(i);
            ap_assert(calcOneMutationAway(p) == one_mutation_away[p]);
        }
#endif

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

    seq_prot = new AP_PROTEINS[sequence_len+1];
    mut1     = new AP_PROTEINS[sequence_len+1];
    mut2     = new AP_PROTEINS[sequence_len+1];

    ap_assert(!get_filter()->does_bootstrap()); // bootstrapping not implemented for protein parsimony

    const AP_filter *filt       = get_filter();
    const uchar     *simplify   = filt->get_simplify_table();
    int              left_bases = sequence_len;
    long             filter_len = filt->get_length();

    ap_assert(filt);

    size_t oidx = 0;               // index for output sequence

    // check if initialized for correct instance of translator:
    ap_assert(min_mutations_initialized_for_codenr == AWT_get_user_translator()->CodeNr());

    for (int idx = 0; idx<filter_len && left_bases; ++idx) {
        if (filt->use_position(idx)) {
            char        c = toupper(simplify[safeCharIndex(isequence[idx])]);
            AP_PROTEINS p = APP_ILLEGAL;

#if defined(SHOW_SEQ)
            fputc(c, stdout);
#endif // SHOW_SEQ

            if (c >= 'A' && c <= 'Z') p = prot2AP_PROTEIN[c-'A'];
            else if (c == '-')        p = APP_GAP;
            else if (c == '.')        p = APP_DOT;
            else if (c == '*')        p = APP_STAR;

            if (p == APP_ILLEGAL) {
                GB_warning(GBS_global_string("Invalid sequence character '%c' replaced by dot", c));
                p = APP_DOT;
            }

            seq_prot[oidx] = p;
            mut1[oidx]     = oneMutationAway(p);
            mut2[oidx]     = oneMutationAway(mut1[oidx]);

            ++oidx;
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
    delete [] mut2;     mut2     = 0;
    delete [] mut1;     mut1     = 0;
    delete [] seq_prot; seq_prot = 0;
    mark_sequence_set(false);
}

AP_FLOAT AP_sequence_protein::combine(const AP_sequence *lefts, const AP_sequence *rights, char *mutation_per_site) {
    // Note: changes done here should also be be applied to AP_seq_dna.cxx@combine_impl

    // now uses same algorithm as done till [877]

    const AP_sequence_protein *left  = DOWNCAST(const AP_sequence_protein *, lefts);
    const AP_sequence_protein *right = DOWNCAST(const AP_sequence_protein *, rights);

    size_t sequence_len = get_sequence_length();
    if (!seq_prot) {
        seq_prot = new AP_PROTEINS[sequence_len + 1];
        mut1     = new AP_PROTEINS[sequence_len + 1];
        mut2     = new AP_PROTEINS[sequence_len + 1];
    }

    const AP_PROTEINS *p1 = left->get_sequence();
    const AP_PROTEINS *p2 = right->get_sequence();

    const AP_PROTEINS *mut11 = left->get_mut1();
    const AP_PROTEINS *mut21 = left->get_mut2();
    const AP_PROTEINS *mut12 = right->get_mut1();
    const AP_PROTEINS *mut22 = right->get_mut2();

    AP_PROTEINS      *p        = seq_prot;
    const AP_weights *weights  = get_weights();
    char             *mutpsite = mutation_per_site;

    long result = 0;
    // check if initialized for correct instance of translator:
    ap_assert(min_mutations_initialized_for_codenr == AWT_get_user_translator()->CodeNr());

    for (size_t idx = 0; idx<sequence_len; ++idx) {
        AP_PROTEINS c1 = p1[idx];
        AP_PROTEINS c2 = p2[idx];

        AP_PROTEINS onemut1 = mut11[idx];
        AP_PROTEINS onemut2 = mut12[idx];
        AP_PROTEINS twomut1 = mut21[idx];
        AP_PROTEINS twomut2 = mut22[idx];

        ap_assert(c1 != APP_ILLEGAL);
        ap_assert(c2 != APP_ILLEGAL);

        AP_PROTEINS contained_in_both = AP_PROTEINS(c1 & c2);
        AP_PROTEINS contained_in_any  = AP_PROTEINS(c1 | c2);

        AP_PROTEINS reachable_from_both_with_1_mut = AP_PROTEINS(onemut1 & onemut2);
        AP_PROTEINS reachable_from_both_with_2_mut = AP_PROTEINS(twomut1 & twomut2);

        AP_PROTEINS max_cost_1 = AP_PROTEINS(contained_in_any & reachable_from_both_with_1_mut);
        AP_PROTEINS max_cost_2 = AP_PROTEINS((contained_in_any & reachable_from_both_with_2_mut) | reachable_from_both_with_1_mut);

        if (contained_in_both) { // there are common proteins
            p[idx]    = contained_in_both; // store common proteins for both subtrees
            mut1[idx] = max_cost_1;
            mut2[idx] = max_cost_2;
        }
        else { // proteins are distinct
            int mutations = INT_MAX;

            AP_PROTEINS reachable_from_both_with_3_mut = AP_PROTEINS((onemut1 & twomut2) | (onemut2 & twomut1)); // one with 1 mutation, other with 2 mutations

            AP_PROTEINS max_cost_3 = AP_PROTEINS(contained_in_any // = one w/o mutations, other with 3 mutations (=anything, i.e. & APP_DOT, skipped)
                                                 | reachable_from_both_with_3_mut);

            if (max_cost_1) {
                // some proteins can be reached from both subtrees with 1 mutation
                mutations = 1;
                p[idx]    = max_cost_1;
                mut1[idx] = max_cost_2;
                mut2[idx] = max_cost_3;
            }
            else {
                AP_PROTEINS reachable_from_any_with_1_mut = AP_PROTEINS(onemut1 | onemut2);

                AP_PROTEINS max_cost_4 = AP_PROTEINS(reachable_from_any_with_1_mut // one with 1 mutation, other with 3 mutations (=anything, i.e. & APP_DOT, skipped)
                                                     | reachable_from_both_with_2_mut);
                if (max_cost_2) {
                    // some proteins can be reached from both subtrees with 2 mutations
                    mutations = 2;
                    p[idx]    = max_cost_2;
                    mut1[idx] = max_cost_3;
                    mut2[idx] = max_cost_4;
                }
                else {
                    ap_assert(max_cost_3);
                    AP_PROTEINS reachable_from_any_with_2_mut = AP_PROTEINS(twomut1 | twomut2);

                    mutations = 3;
                    p[idx]    = max_cost_3;
                    mut1[idx] = max_cost_4;
                    mut2[idx] = reachable_from_any_with_2_mut; // one with 2 mutations, other with 3 mutations
                }
            }
            ap_assert(mutations >= 1 && mutations <= 3);

            if (mutpsite) mutpsite[idx] += mutations; // count mutations per site (unweighted)
            result += mutations * weights->weight(idx); // count weighted or simple

        }

        AP_PROTEINS calc_mut1 = oneMutationAway(p[idx]);
        mut1[idx]             = AP_PROTEINS(mut1[idx] | calc_mut1);
        AP_PROTEINS calc_mut2 = oneMutationAway(mut1[idx]);
        mut2[idx]             = AP_PROTEINS(mut2[idx] | calc_mut2);
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
    // Note: changes done here should also be be applied to AP_seq_dna.cxx@partial_match_impl

    const AP_sequence_protein *part = (const AP_sequence_protein *)part_;

    const AP_PROTEINS *pf = get_sequence();
    const AP_PROTEINS *pp = part->get_sequence();

    const AP_weights *weights = get_weights();

    long min_end; // minimum of both last non-gap positions
    for (min_end = get_sequence_length()-1; min_end >= 0; --min_end) {
        AP_PROTEINS both = AP_PROTEINS(pf[min_end]|pp[min_end]);
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
            AP_PROTEINS both = AP_PROTEINS(pf[max_start]|pp[max_start]);
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
                    int mutations;
                    if (hasGap(AP_PROTEINS(pf[idx]|pp[idx]))) { // one is a gap
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
            if (notHasGap(sequence[idx])) {
                sum += weights->weight(idx) * 2.0;
            }
            else if /*has gap but */ (notIsGap(sequence[idx])) {
                sum += weights->weight(idx);
            }
        }
        wcount = sum * 0.5;
    }
    return wcount;
}

uint32_t AP_sequence_protein::checksum() const {
    const AP_PROTEINS *seq = get_sequence();
    return GB_checksum(reinterpret_cast<const char *>(seq), sizeof(*seq)*get_sequence_length(), 0, NULL);
}

bool AP_sequence_protein::equals(const AP_sequence_protein *other) const {
    const AP_PROTEINS *seq  = get_sequence();
    const AP_PROTEINS *oseq = other->get_sequence();

    size_t len = get_sequence_length();
    for (size_t p = 0; p<len; ++p) {
        if (seq[p] != oseq[p]) return false;
    }
    return true;
}





