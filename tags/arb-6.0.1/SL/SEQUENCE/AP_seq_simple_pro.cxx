#include "AP_seq_simple_pro.hxx"
#include <AP_pro_a_nucs.hxx>
#include <AP_filter.hxx>
#include <ARB_Tree.hxx>


// #define ap_assert(bed) arb_assert(bed)

AP_sequence_simple_protein::AP_sequence_simple_protein(const AliView *aliview)
: AP_sequence(aliview)
{
    sequence = 0;
}

AP_sequence_simple_protein::~AP_sequence_simple_protein() {
    delete sequence;
}

AP_sequence *AP_sequence_simple_protein::dup() const {
    return new AP_sequence_simple_protein(get_aliview());
}



void AP_sequence_simple_protein::set(const char *isequence) {
    AWT_translator  *translator = AWT_get_user_translator(get_aliview()->get_gb_main());

    const struct arb_r2a_pro_2_nuc * const *s2str = translator->S2strArray();

    size_t sequence_len = get_sequence_length();
    sequence     = new ap_pro[sequence_len+1];
    memset(sequence, s2str['.']->index, (size_t)(sizeof(ap_pro) * sequence_len));

    const char    *s = isequence;
    ap_pro        *d = sequence;

    const AP_filter *filt      = get_filter();
    const uchar     *simplify  = filt->get_simplify_table();
    int              sindex    = s2str['s']->index;

    if (filt->does_bootstrap()) {
        int iseqlen = strlen(isequence);
        for (int i = filt->get_filtered_length()-1; i>=0; i--) {
            int pos = filt->bootstrapped_seqpos(i);
            if (pos >= iseqlen) continue;
            unsigned char c = s[pos];
            if (! (s2str[c])) {     // unknown character
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
            unsigned char c = s[i];
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
    mark_sequence_set(true);
}

void AP_sequence_simple_protein::unset() {
    delete [] sequence;
    sequence = 0;
    mark_sequence_set(false);
}


AP_FLOAT AP_sequence_simple_protein::combine(const AP_sequence *, const AP_sequence *, char*) {
    ap_assert(0); // should be unused
    return -1.0;
}

void AP_sequence_simple_protein::partial_match(const AP_sequence* /* part */, long * /* overlap */, long * /* penalty */) const {
    ap_assert(0); // should be unused
}

AP_FLOAT AP_sequence_simple_protein::count_weighted_bases() const {
    ap_assert(0); // should be unused
    return -1.0;
}



