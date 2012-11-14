#define AWAR_PVP_SAI "tmp/pos_var_pars/sai"
#define AWAR_PVP_TREE "tmp/pos_var_pars/tree"

AW_window *AP_open_pos_var_pars_window( AW_root *root );

class AP_pos_var {
    GBDATA        *gb_main;
    long           timer;       // for the status box
    long           treesize;    // max value for slider
    GB_UINT4      *frequencies[256]; // count every occurrence
    GB_UINT4      *transitions; // minumum transitions
    GB_UINT4      *transversions; // minimum transversions (dna only)
    unsigned char  char_2_freq[256]; // mapper (~ toupper)
    long           char_2_transition[256]; // a->1 c->2 g->4 ...
    long           char_2_transversion[256]; // y->1 r->2

    long  getsize(GBT_TREE *tree); // size of tree
    long  ali_len;              // max len of alignment
    long  is_dna;
    char *ali_name;
    char *tree_name;

    const char *parsimony(GBT_TREE *tree, GB_UINT4 *bases = 0, GB_UINT4 *ltbases = 0);

public:

    AP_pos_var(GBDATA *gb_main,char *ali_name, long ali_len, int isdna, char *tree_name);
    ~AP_pos_var();

    GB_ERROR retrieve(GBT_TREE *tree);
    GB_ERROR delete_old_sai(const char *sai_name);
    GB_ERROR save_sai(const char *sai_name);
};
