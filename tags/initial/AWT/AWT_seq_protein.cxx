#include <stdio.h>
#include <memory.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <awt_tree.hxx>
#include "awt_seq_protein.hxx"


AP_sequence_protein::AP_sequence_protein(AP_tree_root *rooti) : AP_sequence(rooti)
	{
	sequence = 0;
}

AP_sequence_protein::~AP_sequence_protein(void)
{
	if (sequence != 0) delete sequence;
	sequence = 0;
}

AP_sequence *AP_sequence_protein::dup(void)
	{
	return (AP_sequence *)new AP_sequence_protein(root);
}



void AP_sequence_protein::set(char *isequence)
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

extern long global_combineCount;

AP_FLOAT AP_sequence_protein::combine( const AP_sequence *lefts, const AP_sequence *rights) {
	register AWT_PDP *p1,*p2,*p;
	register long h0,h1,h2,d11,d22;
	register long i;
	GB_UINT4 *w;
	long result;

	AP_sequence_protein *left = (AP_sequence_protein *)lefts;
	AP_sequence_protein *right = (AP_sequence_protein *)rights;
	
	if (sequence == 0)	{
		sequence_len = root->filter->real_len;
		sequence = new AWT_PDP[sequence_len+1];
	}

	p1 = left->sequence;
	p2 = right->sequence;
	p = sequence;
	w = root->weights->weights;
	result = 0;

	for ( i = sequence_len-1; i>=0 ;i-- ) {
		d11 = p2->patd[1] & p1->patd[1];
		d22 = p2->patd[2] & p1->patd[2];

		if ( (h0 = ((h1=p1->patd[0]) & (h2=p2->patd[0]))) ) {	// hit
			p->patd[0] = h0;
			p->patd[1] = (h1 | h2) & d11;
			p->patd[2] = ((h1 | h2) & d22) | d11;
			goto co_end_of_loop;
		}
		if ( (h0 = ( (h1 | h2) & d11 )) ) {
			p->patd[0] = h0;
			p->patd[1] = ((h1 | h2) & d22) | d11;
			p->patd[2] = h1 | h2 | (p2->patd[1] & p1->patd[2]) | (p2->patd[2] & p1->patd[1]);
			result += *w;
			goto co_end_of_loop;
		}
		if ( (h0 = ( ((h1 | h2) & d22) | d11 )) ) {
			result += (*w) * 2;
			p->patd[0] = h0;
			p->patd[1] = h1 | h2 | (p2->patd[1] & p1->patd[2]) | (p2->patd[2] & p1->patd[1]);
			p->patd[2] = p1->patd[1] |p2->patd[1] | d22;
			goto co_end_of_loop;
		}
		{
		    result += (*w) * 3;
		    p->patd[0] = h1 | h2 |	(p2->patd[1] & p1->patd[2]) | (p2->patd[2] & p1->patd[1]);
		    p->patd[1] = p1->patd[1] |p2->patd[1] | d22;
		    p->patd[2] = p1->patd[2] | p2->patd[2];
		}
co_end_of_loop:
		h0 = p->patd[0];
		h1 = h0>>8;
		h2 = h1>>8;
		p->patd[1] |=	awt_pro_a_nucs->transform07[  h0 & 0xff ] |
				awt_pro_a_nucs->transform815[ h1 & 0xff ] |
				awt_pro_a_nucs->transform1623[ h2 & 0xff ];
		h0 = p->patd[1];
		h1 = h0>>8;
		h2 = h1>>8;
		p->patd[2] |=	awt_pro_a_nucs->transform07[  h0 & 0xff ] |
				awt_pro_a_nucs->transform815[ h1 & 0xff ] |
				awt_pro_a_nucs->transform1623[ h2 & 0xff ];


		w++;
		p++;
		p1++;
		p2++;
	}
	cashed_real_len = -1.0;
	global_combineCount++;
	this->is_set_flag = AP_TRUE;
	return (AP_FLOAT)result;
}


AP_FLOAT AP_sequence_protein::real_len(void)	// count all bases
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
