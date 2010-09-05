// =========================================================== //
//                                                             //
//   File      : awt_seq_dna.hxx                               //
//   Purpose   :                                               //
//                                                             //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef AWT_SEQ_DNA_HXX
#define AWT_SEQ_DNA_HXX


class AP_sequence_parsimony :  public  AP_sequence {
private:
    void    build_table(void);

public:
    char        *sequence;                          // AP_BASES
    static char *table;

    AP_sequence_parsimony(AP_tree_root *rooti);
    ~AP_sequence_parsimony(void);

    AP_sequence *dup(void);                         // used to get the real new element
    void         set(const char *sequence);
    AP_FLOAT     combine(   const AP_sequence * lefts, const    AP_sequence *rights) ;
    AP_FLOAT     real_len(void);
    void partial_match(const AP_sequence* part, long *overlap, long *penalty) const;
};


#else
#error awt_seq_dna.hxx included twice
#endif // AWT_SEQ_DNA_HXX
