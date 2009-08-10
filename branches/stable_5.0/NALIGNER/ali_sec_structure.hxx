
/*****************************************************************************
 *
 * STRUCT: ALI_HELIX
 *
 *****************************************************************************/

struct ALI_HELIX {
    unsigned int number_of_bases;
    unsigned int start_low, start_high;
    ALI_HELIX *next_low, *next_high;
    ALI_HELIX *prev_low, *prev_high;
    ALI_HELIX(unsigned int number_of_bases, unsigned int start_low,
              unsigned int start_high);
};

/*****************************************************************************
 *
 * CLASS: ALI_SEC_STRUCTURE
 *
 *****************************************************************************/

class ALI_SEC_STRUCTURE {
    ALI_HELIX *first_helix;
    void printer(ALI_HELIX *node);
public:
    ALI_SEC_STRUCTURE(); 
    ALI_SEC_STRUCTURE(unsigned int number_of_bases, unsigned int start_low,
                      unsigned int start_high);
    ALI_SEC_STRUCTURE(ALI_SEC_STRUCTURE &ref_struct);
    ~ALI_SEC_STRUCTURE();
    ALI_HELIX *copy(ALI_HELIX *ref_node);
    void print(ALI_SEC_STRUCTURE &sec_struct);
    int insert_node(ALI_HELIX *node);
};
