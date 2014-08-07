// =============================================================== //
//                                                                 //
//   File      : AP_seq_dna.hxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AP_SEQ_DNA_HXX
#define AP_SEQ_DNA_HXX

#ifndef AP_SEQUENCE_HXX
#include <AP_sequence.hxx>
#endif

class AP_sequence_parsimony : public AP_sequence { // derived from a Noncopyable
    void build_table();
    AP_FLOAT count_weighted_bases() const OVERRIDE;

    void set(const char *sequence) OVERRIDE;
    void unset() OVERRIDE;

    char *seq_pars;                                 // AP_BASES

public:
    static char *table;

    AP_sequence_parsimony(const AliView *aliview);
    ~AP_sequence_parsimony() OVERRIDE;

    const char *get_sequence() const { lazy_load_sequence(); ap_assert(seq_pars); return seq_pars; }
    const unsigned char *get_usequence() const { return (const unsigned char*)get_sequence(); }

    AP_sequence *dup() const OVERRIDE;                         // used to get the real new element
    AP_FLOAT combine(const AP_sequence *lefts, const AP_sequence *rights, char *mutation_per_site) OVERRIDE;
    void partial_match(const AP_sequence* part, long *overlap, long *penalty) const OVERRIDE;
};


#else
#error AP_seq_dna.hxx included twice
#endif // AP_SEQ_DNA_HXX
