#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <math.h>
#include <iostream>

#include <arbdb.h>
#include <arbdbt.h>
#include <awt_tree.hxx>
#include "awt_seq_dna.hxx"
#include "awt_parsimony_defaults.hxx"

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define awt_assert(bed) arb_assert(bed)

char *AP_sequence_parsimony::table;


/*****************************


******************************/

AP_sequence_parsimony::AP_sequence_parsimony(AP_tree_root *rooti) : AP_sequence(rooti)
{
    sequence = 0;
}

AP_sequence_parsimony::~AP_sequence_parsimony(void)
{
    if (sequence != 0) delete [] sequence;
    sequence = 0;
}

AP_sequence *AP_sequence_parsimony::dup(void)
{
    return (AP_sequence *)new AP_sequence_parsimony(root);
}

void AP_sequence_parsimony::build_table(void)
{
    table = (char *)AP_create_dna_to_ap_bases();
}



/************************************************************************
combine(const AP_sequence *lefts, const AP_sequence *rights)
set(char *isequence)

for wagner & fitch parsimony algorithm
is_set_flag is used by AP_tree_nlen::parsimony_rek()
*************************************************************************/

// #define SHOW_SEQ

void AP_sequence_parsimony::set(char *isequence)
{
    //     register char *s,*d,*f1,c;
    sequence_len = root->filter->real_len;
    sequence     = new char[sequence_len+1];
    memset(sequence,AP_N,(size_t)sequence_len+1);
    //     s            = isequence;
    //     d            = sequence;
    //     f1           = root->filter->filter_mask;

    const uchar *simplify = root->filter->simplify;
    if (!table) this->build_table();

    if (root->filter->bootstrap) {
        int iseqlen = strlen(isequence);

        for (int i = 0; i<sequence_len; ++i) {
            int pos = root->filter->bootstrap[i]; // enthaelt zufallsfolge

//             printf("i=%i pos=%i sequence_len=%li iseqlen=%i\n", i, pos, sequence_len, iseqlen);

            awt_assert(pos >= 0);
            awt_assert(pos<iseqlen);
            if (pos >= iseqlen) continue;

            char c = isequence[pos];   // @@@ muss ueber mapping tabelle aufgefaltet werden 10/99

#if defined(SHOW_SEQ)
            fputc(simplify[c], stdout);
#endif // SHOW_SEQ

            sequence[i] = table[simplify[c]];

        }

        //         int i;
        //         for (i=root->filter->real_len-1;i>=0;i--){
        //             int pos = root->filter->bootstrap[i]; // enthaelt zufallsfolge
        //             if (pos >= iseqlen) continue;
        //             c = s[pos];
        //             d[i] = table[simplify[c]];
        //         }

    }
    else {
        char *filt       = root->filter->filter_mask;
        int   left_bases = sequence_len;
        int   filter_len = root->filter->filter_len;
        int   oidx       = 0; // for output sequence

        for (int idx = 0; idx<filter_len && left_bases; ++idx) {
            if (filt[idx]) {
                char c           = isequence[idx];
                sequence[oidx++] = table[simplify[c]];
                --left_bases;
#if defined(SHOW_SEQ)
                fputc(simplify[c], stdout);
#endif                          // SHOW_SEQ
            }
        }

        //         while ( (c = (*s++)) ) {
        //             if (!i) break;
        //             if (*(f++)) { // use current position ?
        //                 i--;            // decrement leftover positions
        //                 *(d++) = table[simplify[c]];
        //             }
        //         }
    }

#if defined(SHOW_SEQ)
    fputc('\n', stdout);
#endif // SHOW_SEQ

    update          = AP_timer();
    is_set_flag     = AP_TRUE;
    cashed_real_len = -1.0;
}

long global_combineCount;

AP_FLOAT AP_sequence_parsimony::combine( const AP_sequence *lefts, const AP_sequence *rights) {
    //  register char *p1,*p2,*p;
    //  register char c1,c2;
    //  register long result;

    const AP_sequence_parsimony *left = (const AP_sequence_parsimony *)lefts;
    const AP_sequence_parsimony *right = (const AP_sequence_parsimony *)rights;

    if (sequence == 0)  {
        sequence_len = root->filter->real_len;
        sequence = new char[root->filter->real_len +1];
    }

    const char *p1 = left->sequence;
    const char *p2 = right->sequence;
    char       *p  = sequence;

    //  result         = 0;
    //  char *end_seq1 = p1 + sequence_len;

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

    for (long idx = 0; idx<sequence_len; ++idx) {
        char c1 = p1[idx];
        char c2 = p2[idx];

        awt_assert(c1 != 0); // not a base and not a gap -- what should it be ?
        awt_assert(c2 != 0);

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

            if (mutpsite) mutpsite[idx]++; // count mutations per site (unweighted)
            result += w ? w[idx] : 1; // count weighted or simple
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

        awt_assert(p[idx] != 0);
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

    global_combineCount++;
    this->is_set_flag = AP_TRUE;
    cashed_real_len   = -1.0;

    awt_assert(result >= 0.0);

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

    const char *pf = sequence;
    const char *pp = part->sequence;

    GB_UINT4 *w                          = 0;
    if (!root->weights->dummy_weights) w = root->weights->weights;

    long min_end; // minimum of both last non-gap positions

    for (min_end = sequence_len-1; min_end >= 0; --min_end) {
        char both = pf[min_end]|pp[min_end];
        if (both != AP_S) { // last non-gap found
            if (pf[min_end] != AP_S) { // occurred in full sequence
                for (; min_end >= 0; --min_end) { // search same in partial sequence
                    if (pp[min_end] != AP_S) break;
                }
            }
            else {
                awt_assert(pp[min_end] != AP_S); // occurred in partial sequence
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
                    awt_assert(pp[max_start] != AP_S); // occurred in partial sequence
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
                    penalty += w ? w[idx] : 1;
                }
            }
            overlap = min_end-max_start+1;
        }
    }

    *overlapPtr = overlap;
    *penaltyPtr = penalty;
}



AP_FLOAT AP_sequence_parsimony::real_len(void)  // count all bases
{
    if (!sequence) return -1.0;
    if (cashed_real_len>=0.0) return cashed_real_len;
    char hits[256];
    long i;
    long sum = 0;
    unsigned char *p = (unsigned char*)sequence;
    for (i=0;i<256;i++){    // count ambigous characters half
        hits[i] = 1;
    }
    hits[AP_A] = 2;     // real characters full
    hits[AP_C] = 2;
    hits[AP_G] = 2;
    hits[AP_T] = 2;
    hits[AP_S] = 0;     // count no gaps
    hits[AP_N] = 0;     // no Ns
    GB_UINT4 *w = root->weights->weights;

    for ( i = sequence_len; i ;i-- ) {  // all but no gaps
        sum += hits[*(p++)] * *(w++);
    }
    cashed_real_len = sum * .5;
    return sum * .5;
}
