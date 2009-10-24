#include "AP_seq_simple_pro.hxx"
#include <AP_pro_a_nucs.hxx>
#include <AP_filter.hxx>
#include <ARB_Tree.hxx>


// #define ap_assert(bed) arb_assert(bed)

AP_sequence_simple_protein::AP_sequence_simple_protein(ARB_tree_root *rooti) : AP_sequence(rooti) {
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
    ;
    
    ;

    AWT_translator  *translator = AWT_get_user_translator(this->root->get_gb_main());
    const AP_filter *filt       = root->get_filter();

    const struct arb_r2a_pro_2_nuc * const *s2str = translator->S2strArray();

    sequence_len = filt->get_filtered_length();
    sequence     = new ap_pro[sequence_len+1];
    memset(sequence,s2str['.']->index,(size_t)(sizeof(ap_pro) * sequence_len));
    
    const char    *s = isequence;
    ap_pro        *d = sequence;

    const uchar  *simplify  = filt->get_simplify_table();
    int           sindex    = s2str['s']->index;
    const size_t *bootstrap = filt->get_bootstrap();

    unsigned char  c;
    if (bootstrap){
        int iseqlen = strlen(isequence);
        int i;
        for (i = filt->get_filtered_length()-1; i>=0; i--) {
            int pos = bootstrap[i];
            if (pos >= iseqlen) continue;
            c = s[pos];
            if (! (s2str[c] ) ) {   // unknown character
                continue;
            }
            int ind = s2str[simplify[c]]->index;
            if (ind >= sindex) ind --;
            d[i] = ind;
        }
    }
    else { 
        size_t i, j;
        size_t flen = filt->get_length();
        for (i = j = 0; i < flen; ++i) {
            c = s[i];
            if (!c) break;
            if (filt->use_position(i)) {
                if (s2str[c]) {
                    int ind = s2str[simplify[c]]->index;
                    if (ind >= sindex) ind--;
                    d[j]    = ind;
                }
                j++;
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
    ap_assert(0); // should be unused
}



