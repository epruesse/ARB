class AP_sequence_parsimony : public AP_sequence {
private:
    void    build_table(void);

public:
    char        *sequence;      // AP_BASES
    static char *table;

    AP_sequence_parsimony(void);
    ~AP_sequence_parsimony(void);

    AP_sequence *dup(void);     // used to get the real new element
    void         set(char *sequence);
    AP_FLOAT     combine(const AP_sequence *lefts, const AP_sequence *rights) ;
    AP_FLOAT     real_len(void);
};
