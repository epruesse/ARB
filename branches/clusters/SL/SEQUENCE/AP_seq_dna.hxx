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

class AP_sequence_parsimony : public AP_sequence {
private:
    void    build_table(void);

public:
    char        *sequence;                          // AP_BASES
    static char *table;

    AP_sequence_parsimony(ARB_tree_root *rooti);
    ~AP_sequence_parsimony(void);

    AP_sequence *dup(void);                         // used to get the real new element
    void         set(const char *sequence);
    AP_FLOAT     combine(   const AP_sequence * lefts, const    AP_sequence *rights) ;
    AP_FLOAT     real_len(void);
    void partial_match(const AP_sequence* part, long *overlap, long *penalty) const;
};


#else
#error AP_seq_dna.hxx included twice
#endif // AP_SEQ_DNA_HXX
