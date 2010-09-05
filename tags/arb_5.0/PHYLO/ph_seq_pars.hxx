class PH_sequence_pa:  public AP_sequence {
private:
    void    build_table(void);

public:
    char        *sequence;     // AP_BASES
    static char *table;

    PH_sequence_pa(void);
    ~PH_sequence_pa(void);

    AP_sequence *dup(void);                         // used to get the real new element
    void         set(       char *sequence);
    bool         is_set();

    AP_FLOAT combine(const AP_sequence* lefts, const AP_sequence *rights);
    void calc_base_rates(AP_rates *rates,
                         const AP_sequence* lefts, AP_FLOAT leftl,
                         const AP_sequence *rights, AP_FLOAT rightl);
};
