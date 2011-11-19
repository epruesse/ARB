#include <stdio.h>
#include <memory.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <awt_tree.hxx>
#include "awt_seq_simple_pro.hxx"

#define awt_assert(bed) arb_assert(bed)

AP_sequence_simple_protein::AP_sequence_simple_protein(AP_tree_root *rooti) : AP_sequence(rooti) {
    sequence = 0;
}

AP_sequence_simple_protein::~AP_sequence_simple_protein(void) {
    delete sequence;
    sequence = 0;
}

AP_sequence *AP_sequence_simple_protein::dup(void) {
    return new AP_sequence_simple_protein(root);
}



void AP_sequence_simple_protein::set(const char *isequence) {
    const char    *s;
    unsigned char  c;
    ap_pro        *d;

    AWT_translator *translator = AWT_get_user_translator(this->root->gb_main);

    const struct arb_r2a_pro_2_nuc * const *s2str = translator->S2strArray();
    sequence_len = root->filter->real_len;
    sequence = new ap_pro[sequence_len+1];
    memset(sequence,s2str['.']->index,(size_t)(sizeof(ap_pro) * sequence_len));
    s = isequence;
    d = sequence;
    const uchar *simplify = root->filter->simplify;
    int sindex = s2str['s']->index;
    if (root->filter->bootstrap){
        int iseqlen = strlen(isequence);
        int i;
        for (i=root->filter->real_len-1;i>=0;i--){
            int pos = root->filter->bootstrap[i];
            if (pos >= iseqlen) continue;
            c = s[pos];
            if (! (s2str[c] ) ) {   // unknown character
                continue;
            }
            int ind = s2str[simplify[c]]->index;
            if (ind >= sindex) ind --;
            d[i] = ind;
        }
    }else{
        char *f = root->filter->filter_mask;
        int i = root->filter->filter_len;
        while ( (c = (*s++)) ) {
            if (!i) break;
            i--;
            if (*(f++)) {
                if (! (s2str[c] ) ) {   // unknown character
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

void AP_sequence_simple_protein::partial_match(const AP_sequence* /*part*/, long */*overlap*/, long */*penalty*/) const {
    awt_assert(0); // should be unused
}



