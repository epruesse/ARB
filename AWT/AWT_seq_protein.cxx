#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <awt_tree.hxx>
#include "awt_seq_protein.hxx"

#define awt_assert(bed) arb_assert(bed)

extern long global_combineCount;

// start of implementation of class AP_sequence_protein:

// ---------------------------------------------------------------------
//      AP_sequence_protein::AP_sequence_protein(AP_tree_root *root)
// ---------------------------------------------------------------------
AP_sequence_protein::AP_sequence_protein(AP_tree_root *root)
    : AP_sequence(root)
{
	sequence = 0;
}

// ----------------------------------------------------
//      AP_sequence_protein::~AP_sequence_protein()
// ----------------------------------------------------
AP_sequence_protein::~AP_sequence_protein()
{
	if (sequence != 0) delete sequence;
	sequence = 0;
}

// ------------------------------------------------
//      AP_sequence *AP_sequence_protein::dup()
// ------------------------------------------------
AP_sequence *AP_sequence_protein::dup()
{
	return (AP_sequence *)new AP_sequence_protein(root);
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

// -------------------------------------------------------
//      void AP_sequence_protein::set(char *isequence)
// -------------------------------------------------------
void AP_sequence_protein::set(char *isequence)
{
	if (!awt_pro_a_nucs) awt_pro_a_nucs_gen_dist(this->root->gb_main);
    if (awt_pro_a_nucs->time_stamp != min_mutations_initialized_time_stamp) update_min_mutations();

	sequence_len = root->filter->real_len;
	sequence     = new AP_PROTEINS[sequence_len+1];

    for (int idx = 0; idx<sequence_len; ++idx) {
        char        c = toupper(isequence[idx]);
        AP_PROTEINS p = APP_ILLEGAL;

        if (c >= 'A' && c <= 'Z')       p = prot2AP_PROTEIN[c-'A'];
        else if (c == '-' || c == '.')  p = APP_GAP;
        else if (c == '*')              p = APP_STAR;

        if (p == APP_ILLEGAL) {
            awt_assert(0);
            p = APP_GAP;
        }

        sequence[idx] = p;
    }
    sequence[sequence_len] = APP_ILLEGAL;

	update          = AP_timer();
	is_set_flag     = AP_TRUE;
	cashed_real_len = -1.0;
}

// ------------------------------------------------------------------------------------------------------
//      AP_FLOAT AP_sequence_protein::combine(	const AP_sequence * lefts, const	AP_sequence *rights)
// ------------------------------------------------------------------------------------------------------
// this works similar to AP_sequence_parsimony::combine

AP_FLOAT AP_sequence_protein::combine(	const AP_sequence * lefts, const	AP_sequence *rights)
{
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
                if (idx>0 && (p[idx-1]&APP_GAP)) { // last position also contained gap..
                    if (((c1&APP_GAP) && (p1[idx-1]&APP_GAP)) || // ..in same sequence
                        ((c2&APP_GAP) && (p2[idx-1]&APP_GAP)))
                    {
                        if (!(p1[idx-1]&APP_GAP) || !(p2[idx-1]&APP_GAP)) { // if one of the sequences had no gap at previous position
                            continue; // skip multiple gaps
                        }
                    }
                }

                // because one subtree contains a gap (and the other does not)
                // we count only one mutation (as in general for gaps):
                mutations = 1;
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
    }

	global_combineCount++;
	this->is_set_flag = AP_TRUE;
	cashed_real_len   = -1.0;

    awt_assert(result >= 0.0);

	return (AP_FLOAT)result;
}

// -------------------------------------------------
//      AP_FLOAT AP_sequence_protein::real_len()
// -------------------------------------------------
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


AP_sequence_protein_old::AP_sequence_protein_old(AP_tree_root *rooti) : AP_sequence(rooti)
	{
	sequence = 0;
}

AP_sequence_protein_old::~AP_sequence_protein_old(void)
{
	if (sequence != 0) delete sequence;
	sequence = 0;
}

AP_sequence *AP_sequence_protein_old::dup(void)
	{
	return (AP_sequence *)new AP_sequence_protein_old(root);
}



void AP_sequence_protein_old::set(char *isequence)
	{
	register char *s,*f,c;
	register int i;
	register AWT_PDP *d;
	register const uchar *simplify;
	if (!awt_pro_a_nucs) {
		awt_pro_a_nucs_gen_dist(this->root->gb_main);
	}

	register struct arb_r2a_pro_2_nuc **s2str = &awt_pro_a_nucs->s2str[0];
	sequence_len = root->filter->real_len;
	sequence = new AWT_PDP[sequence_len+1];
	memset(sequence,-1,(size_t)(sizeof(AWT_PDP) * sequence_len));
	s = isequence;
	d = sequence;
	f = root->filter->filter_mask;
	simplify = root->filter->simplify;
	i = root->filter->filter_len;
	while ( (c = (*s++)) ) {
		if (!i) break;
		i--;
		if (*(f++)) {
			if (! (s2str[c] ) ) {	// unknown character
				d++;
				continue;
			}
			int index = s2str[simplify[c]]->index;
			*(d++) = *awt_pro_a_nucs->dist[index];
		}
	}
	update = AP_timer();
	is_set_flag = AP_TRUE;
	cashed_real_len = -1.0;
}

AP_FLOAT AP_sequence_protein_old::combine( const AP_sequence *lefts, const AP_sequence *rights) {
	const AP_sequence_protein_old *left  = (const AP_sequence_protein_old *)lefts;
	const AP_sequence_protein_old *right = (const AP_sequence_protein_old *)rights;

	if (sequence == 0)	{
		sequence_len = root->filter->real_len;
		sequence = new AWT_PDP[sequence_len+1];
	}

	AWT_PDP  *p1 = left->sequence;
	AWT_PDP  *p2 = right->sequence;
	AWT_PDP  *p  = sequence;
	GB_UINT4 *w  = root->weights->weights;

	long result   = 0;
    long dash_bit = 1<<(awt_pro_a_nucs->s2str['-']->index); // bit that represents a '-'

    for (long i = 0; i<sequence_len; ++i, ++p, ++p1, ++p2, ++w) {
        long dist_0_one     = p1->patd[0] | p2->patd[0]; // for these proteins 'distance == 0' to p1 OR  p2
        long dist_0_both    = p1->patd[0] & p2->patd[0]; // for these proteins 'distance == 0' to p1 AND p2
        long dist_max1_both = p1->patd[1] & p2->patd[1]; // for these proteins 'distance <= 1' to p1 AND p2
        long dist_max2_both = p1->patd[2] & p2->patd[2]; // for these proteins 'distance <= 2' to p1 AND p2

        long max1_mutations = dist_0_one & dist_max1_both; // for these proteins 1 mutation is needed
        long max2_mutations = (dist_0_one & dist_max2_both) | dist_max1_both; // for these proteins 2 mutations are needed

        if (dist_0_both) {      // mutations == 0
            p->patd[0] = dist_0_both;
            p->patd[1] = max1_mutations;
            p->patd[2] = max2_mutations;
        }
        else {
            long max3_mutations = dist_0_one | (p1->patd[1] & p2->patd[2]) | (p1->patd[2] & p2->patd[1]); // for these proteins 3 mutations are needed
            long min_mutations  = 0;

            if (max1_mutations) { // mutations == 1
                p->patd[0] = max1_mutations;
                p->patd[1] = max2_mutations;
                p->patd[2] = max3_mutations;

                min_mutations = 1;
            }
            else {
                long max4_mutations = p1->patd[1] | p2->patd[1] | dist_max2_both;  // for these proteins 4 mutations are needed

                if (max2_mutations) { // mutations == 2
                    p->patd[0] = max2_mutations;
                    p->patd[1] = max3_mutations;
                    p->patd[2] = max4_mutations;

                    min_mutations = 2;
                }
                else {          // mutations == 3
                    p->patd[0]  = max3_mutations;
                    p->patd[1]  = max4_mutations;
                    p->patd[2]  = p1->patd[2] | p2->patd[2];

                    min_mutations = 3;
                }
            }

            bool ignore_continued_dashes = false;
            if (p->patd[0] & dash_bit) { // contains a gap
                if (i>0 && (p[-1].patd[0] & dash_bit)) { // previous position also contains a gap ..
                    if (((p1->patd[0] & dash_bit) && (p1[-1].patd[0] & dash_bit)) || // .. in same sequence
                        ((p2->patd[0] & dash_bit) && (p2[-1].patd[0] & dash_bit)))
                    {
                        if (!(p1[-1].patd[0] & dash_bit) || !(p2[-1].patd[0] & dash_bit)) { // if one of the sequences had no gap at previous position
                            ignore_continued_dashes = true; // dont count current position
                        }
                    }
                }
            }

            if (!ignore_continued_dashes) result += min_mutations * (*w);
        }

		p->patd[1] |= (awt_pro_a_nucs->transform07  [  p->patd[0]      & 0xff ] |
                       awt_pro_a_nucs->transform815 [ (p->patd[0]>>8)  & 0xff ] |
                       awt_pro_a_nucs->transform1623[ (p->patd[0]>>16) & 0xff ] );

        p->patd[2] |= (awt_pro_a_nucs->transform07  [  p->patd[1]      & 0xff ] |
                       awt_pro_a_nucs->transform815 [ (p->patd[1]>>8)  & 0xff ] |
                       awt_pro_a_nucs->transform1623[ (p->patd[1]>>16) & 0xff ] );
    }

	cashed_real_len = -1.0;
	is_set_flag     = AP_TRUE;

	global_combineCount++;

	return (AP_FLOAT)result;
}

// here's the old code of the above for-loop (rewritten Fri Jul 26 17:42:15 2002) :

// 	register long h0,h1,h2,d11,d22;
// 	for ( i = sequence_len-1; i>=0 ;i-- ) {
// 		d11 = p2->patd[1] & p1->patd[1]; // all proteins with 'distance <= 1' to p1 AND p2
// 		d22 = p2->patd[2] & p1->patd[2]; // all proteins with 'distance <= 2' to p1 AND p2

//         h1 = p1->patd[0];       // all proteins with 'no distance' to p1
//         h2 = p2->patd[0];       // all proteins with 'no distance' to p2

//         // h1 & h2                  means both have distance    == 0
//         // h1 | h2                  means one has distance      == 0
//         // (h1 | h2) & d11          means one has distance      == 0 and both have distance <= 1 ( => combined distance <= 1)
//         // ((h1 | h2) & d22) | d11  means (one has distance   == 0 and both have distance <= 2)
//         //                                or both have distance <= 2 ( => combined distance <= 2)


// 		if ( (h0 = (h1 & h2)) ) { // hit (p1 and p2 have zero distance)
// 			p->patd[0] = h0;    // 'no distance' to p1 AND p2 => 'no distance' to combined
// 			p->patd[1] = (h1 | h2) & d11;
//             p->patd[2] = ((h1 | h2) & d22) | d11;
// 			goto co_end_of_loop;
// 		}
// 		if ( (h0 = ( (h1 | h2) & d11 )) ) { // mask proteins with distance == 0 to one and distance <= 1 to both
// 			p->patd[0] = h0;
// 			p->patd[1] = ((h1 | h2) & d22) | d11;
// 			p->patd[2] = h1 | h2 | (p2->patd[1] & p1->patd[2]) | (p2->patd[2] & p1->patd[1]);
// 			result += *w;
// 			goto co_end_of_loop;
// 		}
// 		if ( (h0 = ( ((h1 | h2) & d22) | d11 ))) { // (distance == 0 for one and distance <= 2 for both) or distance <= 1 for both
// 			result     += (*w) * 2;
// 			p->patd[0]  = h0;
// 			p->patd[1]  = h1 | h2 | (p2->patd[1] & p1->patd[2]) | (p2->patd[2] & p1->patd[1]);
// 			p->patd[2]  = p1->patd[1] |p2->patd[1] | d22;
// 			goto co_end_of_loop;
// 		}
// 		{
// 		    result += (*w) * 3;
// 		    p->patd[0] = h1 | h2 |	(p2->patd[1] & p1->patd[2]) | (p2->patd[2] & p1->patd[1]);
// 		    p->patd[1] = p1->patd[1] |p2->patd[1] | d22;
// 		    p->patd[2] = p1->patd[2] | p2->patd[2];
// 		}
// co_end_of_loop:
// 		h0 = p->patd[0];
// 		h1 = h0>>8;
// 		h2 = h1>>8;
// 		p->patd[1] |=	awt_pro_a_nucs->transform07[  h0 & 0xff ] |
// 				awt_pro_a_nucs->transform815[ h1 & 0xff ] |
// 				awt_pro_a_nucs->transform1623[ h2 & 0xff ];
// 		h0 = p->patd[1];
// 		h1 = h0>>8;
// 		h2 = h1>>8;
// 		p->patd[2] |=	awt_pro_a_nucs->transform07[  h0 & 0xff ] |
// 				awt_pro_a_nucs->transform815[ h1 & 0xff ] |
// 				awt_pro_a_nucs->transform1623[ h2 & 0xff ];


// 		w++;
// 		p++;
// 		p1++;
// 		p2++;
// 	}


AP_FLOAT AP_sequence_protein_old::real_len(void)	// count all bases
	{
	if (!sequence) return -1.0;
	if (cashed_real_len>=0.0) return cashed_real_len;
	AP_FLOAT sum = 0.0;
	register long i;
	register AWT_PDP *p = sequence;

	for ( i = sequence_len; i ;i-- ) {	// all but no gaps
		if (p->patd[0] == -1) continue;
		sum +=1.0;
	}
	cashed_real_len = sum;
	return sum;
}

