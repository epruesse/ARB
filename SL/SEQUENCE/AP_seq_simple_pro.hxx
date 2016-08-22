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

enum aas {
    ALA, ARG, ASN, ASP, CYS, GLN, GLU, GLY, HIS, ILEU, LEU, LYS, MET, PHE, PRO,
    SER, THR, TRP, TYR, VAL, STOP, DEL, ASX, GLX, UNK, QUEST
};

typedef unsigned char ap_pro;   // aas but only one character used

class AP_sequence_simple_protein : public AP_sequence { // derived from a Noncopyable
    ap_pro *sequence;

    AP_FLOAT count_weighted_bases() const OVERRIDE;

    void set(const char *sequence) OVERRIDE;
    void unset() OVERRIDE;

public:

    AP_sequence_simple_protein(const AliView *aliview);
    ~AP_sequence_simple_protein() OVERRIDE;

    AP_sequence *dup() const OVERRIDE;                             // used to get the real new element

    AP_FLOAT combine(const AP_sequence *lefts, const AP_sequence *rights, char *mutation_per_site = 0) OVERRIDE;
    void partial_match(const AP_sequence* part, long *overlap, long *penalty) const OVERRIDE;

    const ap_pro *get_sequence() const { return sequence; }
    ap_pro *get_sequence() { return sequence; }
};


#else
#error AP_seq_simple_pro.hxx included twice
#endif // AP_SEQ_SIMPLE_PRO_HXX
