// =========================================================== //
//                                                             //
//   File      : awt_seq_simple_pro.hxx                        //
//   Purpose   :                                               //
//                                                             //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef AWT_SEQ_SIMPLE_PRO_HXX
#define AWT_SEQ_SIMPLE_PRO_HXX


typedef enum {
    ala, arg, asn, asp, cys, gln, glu, gly, his, ileu, leu, lys, met, phe, pro,
    ser, thr, trp, tyr, val, stop, del, asx, glx, unk, quest
}               aas;

typedef unsigned char ap_pro;   // aas but only one character used

class AP_sequence_simple_protein :  public  AP_sequence {

public:
    ap_pro      *sequence;
    //static char   *table;
    AP_sequence_simple_protein(AP_tree_root *rooti);
    ~AP_sequence_simple_protein(void);
    AP_sequence     *dup(void);     // used to get the real new element
    void set(const char *sequence);
    double combine( const AP_sequence *lefts, const AP_sequence *rights);
    void partial_match(const AP_sequence* part, long *overlap, long *penalty) const;
};

#else
#error awt_seq_simple_pro.hxx included twice
#endif // AWT_SEQ_SIMPLE_PRO_HXX
