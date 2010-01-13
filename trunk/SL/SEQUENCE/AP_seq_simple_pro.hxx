// =============================================================== //
//                                                                 //
//   File      : AP_seq_simple_pro.hxx                             //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AP_SEQ_SIMPLE_PRO_HXX
#define AP_SEQ_SIMPLE_PRO_HXX

#ifndef AP_SEQUENCE_HXX
#include <AP_sequence.hxx>
#endif

typedef enum {
    ala, arg, asn, asp, cys, gln, glu, gly, his, ileu, leu, lys, met, phe, pro,
    ser, thr, trp, tyr, val, stop, del, asx, glx, unk, quest
} aas;

typedef unsigned char ap_pro;   // aas but only one character used

class AP_sequence_simple_protein : public AP_sequence {
    ap_pro *sequence;

    AP_FLOAT count_weighted_bases() const;

    void set(const char *sequence);
    void unset();

public:

    AP_sequence_simple_protein(const AliView *aliview);
    ~AP_sequence_simple_protein();

    AP_sequence *dup() const;                             // used to get the real new element

    AP_FLOAT combine(const AP_sequence *lefts, const AP_sequence *rights, char *mutation_per_site = 0);
    void partial_match(const AP_sequence* part, long *overlap, long *penalty) const;

    const ap_pro *get_sequence() const { return sequence; }
    ap_pro *get_sequence() { return sequence; }
};


#else
#error AP_seq_simple_pro.hxx included twice
#endif // AP_SEQ_SIMPLE_PRO_HXX
