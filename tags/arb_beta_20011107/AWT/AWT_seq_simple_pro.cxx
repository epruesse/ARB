#include <stdio.h>
#include <memory.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <awt_tree.hxx>
#include "awt_seq_simple_pro.hxx"


AP_sequence_simple_protein::AP_sequence_simple_protein(AP_tree_root *rooti) : AP_sequence(rooti)
	{
	sequence = 0;
}

AP_sequence_simple_protein::~AP_sequence_simple_protein(void)
{
	delete sequence;
	sequence = 0;
}

AP_sequence *AP_sequence_simple_protein::dup(void)
	{
	return (AP_sequence *)new AP_sequence_simple_protein(root);
}



void AP_sequence_simple_protein::set(char *isequence)
	{
	register char *s,c;
	register ap_pro *d;
	if (!awt_pro_a_nucs) {
		awt_pro_a_nucs_gen_dist
		    (this->root->gb_main);
	}

	register struct arb_r2a_pro_2_nuc **s2str = &awt_pro_a_nucs->s2str[0];
	sequence_len = root->filter->real_len;
	sequence = new ap_pro[sequence_len+1];
	memset(sequence,s2str['.']->index,(size_t)(sizeof(ap_pro) * sequence_len));
	s = isequence;
	d = sequence;
	register const uchar *simplify = root->filter->simplify;
	int sindex = s2str['s']->index;
	if (root->filter->bootstrap){
	    int iseqlen = strlen(isequence);
	    int i;
	    for (i=root->filter->real_len-1;i>=0;i--){
		int pos = root->filter->bootstrap[i];
		if (pos >= iseqlen) continue;
		c = s[pos];
		if (! (s2str[c] ) ) {	// unknown character
		    continue;
		}
		int ind = s2str[simplify[c]]->index;
		if (ind >= sindex) ind --;
		d[i] = ind;		
	    }
	}else{
	    register char *f = root->filter->filter_mask;
	    register int i = root->filter->filter_len;
	    while ( (c = (*s++)) ) {
		if (!i) break;
		i--;
		if (*(f++)) {
		    if (! (s2str[c] ) ) {	// unknown character
			d++;
			continue;
		    }
		    int ind = s2str[simplify[c]]->index;
		    if (ind >= sindex) ind --;
		    *(d++) = ind;
		}		
	    }
	}
	is_set_flag = AP_TRUE;
	cashed_real_len = -1.0;
}

AP_FLOAT AP_sequence_simple_protein::combine( const AP_sequence *, const AP_sequence *) {
	return 0.0;
}


