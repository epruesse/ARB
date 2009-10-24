// =============================================================== //
//                                                                 //
//   File      : AP_seq_protein.hxx                                //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AP_SEQ_PROTEIN_HXX
#define AP_SEQ_PROTEIN_HXX

#ifndef AP_SEQUENCE_HXX
#include <AP_sequence.hxx>
#endif


enum AP_PROTEINS {
    APP_ILLEGAL = 0,
    APP_A       = (1 <<  0),    // Ala
    APP_C       = (1 <<  1),    // Cys
    APP_D       = (1 <<  2),    // Asp
    APP_E       = (1 <<  3),    // Glu
    APP_F       = (1 <<  4),    // Phe
    APP_G       = (1 <<  5),    // Gly
    APP_H       = (1 <<  6),    // His
    APP_I       = (1 <<  7),    // Ile
    APP_K       = (1 <<  8),    // Lys
    APP_L       = (1 <<  9),    // Leu
    APP_M       = (1 << 10),    // Met
    APP_N       = (1 << 11),    // Asn
    APP_P       = (1 << 12),    // Pro
    APP_Q       = (1 << 13),    // Gln
    APP_R       = (1 << 14),    // Arg
    APP_S       = (1 << 15),    // Ser
    APP_T       = (1 << 16),    // Thr
    APP_V       = (1 << 17),    // Val
    APP_W       = (1 << 18),    // Trp
    APP_Y       = (1 << 19),    // Tyr

    APP_STAR = (1 << 20),        // *
    APP_GAP = (1 << 21),        // space (gap)

    APP_X = (APP_STAR-1),        // Xxx (any real codon)

    APP_B = APP_D | APP_N,      // Asx ( = Asp | Asn )
    APP_Z = APP_E | APP_Q,      // Glx ( = Glu | Gln )
};

class AP_sequence_protein :  public  AP_sequence {
private:
    AP_PROTEINS *sequence;

    AP_sequence_protein(const AP_sequence_protein& other); // copying not allowed
    AP_sequence_protein& operator = (const AP_sequence_protein& other); // assignment not allowed
public:
    AP_sequence_protein(ARB_tree_root *root);
    virtual ~AP_sequence_protein();

    AP_sequence *dup();     // used to get the real new element
    void         set(const char *isequence);
    AP_FLOAT     combine(const AP_sequence * lefts, const AP_sequence *rights) ;
    AP_FLOAT     real_len();
    void partial_match(const AP_sequence* part, long *overlap, long *penalty) const;
};



#else
#error AP_seq_protein.hxx included twice
#endif // AP_SEQ_PROTEIN_HXX
