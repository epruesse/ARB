#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define st_assert(bed) arb_assert(bed)

enum AWT_dna_base {
    ST_A,
    ST_C,
    ST_G,
    ST_T,
    ST_GAP,
    ST_MAX_BASE,
    ST_UNKNOWN = -1
};

extern class AWT_dna_table {
    char char_to_enum_table[256];
public:
    AWT_dna_base char_to_enum(char i) {
        return (AWT_dna_base)char_to_enum_table[(unsigned char)i];
    }
    AWT_dna_table();
} awt_dna_table;

typedef unsigned char ST_ML_Color;

const int ST_MAX_SEQ_PART = 256;
// should be greater than the editor width
// otherwise extrem performance penalties
const int ST_BUCKET_SIZE = 16;
// at minumum ST_BUCKET_SIZE characters are calculated per call
const int LD_BUCKET_SIZE = 4; // log dualis of ST_BUCKET_SIZE

class ST_base_vector {
public:
    float b[ST_MAX_BASE]; // acgt-
    int ld_lik;
    float lik; // likelihood  = 2^ld_lik * lik * (b[0] + b[1] + b[2] ..)
    void set(char base, ST_base_vector *frequencies);
    inline void mult(ST_base_vector *other);
    void check_overflow();
    void print();
};

class ST_rate_matrix {
    float m[ST_MAX_BASE][ST_MAX_BASE];
public:
    void set(double dist, double TT_ratio);
    inline void mult(ST_base_vector *in, ST_base_vector *out);
    void print();
};

class ST_ML;
class AWT_csp;

/** Note: Because we have only limited memory we split the
 sequence into ST_MAX_SEQ_PART long parts */
class ST_sequence_ml : private AP_sequence {
    friend class ST_ML;
public:

    GBDATA *gb_data; // the sequence
    static ST_base_vector *tmp_out; // len = alignment length

protected:

    ST_ML *st_ml; // link to a global ST object
    ST_base_vector *sequence; // A part of the sequence
    int last_updated;
    ST_ML_Color *color_out;
    int *color_out_valid_till; // color_out is valid up to

public:
    void delete_sequence(); // remove link to database
    void sequence_change(); // sequence has changed in db
    AP_FLOAT combine(const AP_sequence* lefts, const AP_sequence *rights);
    void
            partial_match(const AP_sequence* part, long *overlap, long *penalty) const;
    ST_sequence_ml(AP_tree_root *rooti, ST_ML *st_ml);
    ~ST_sequence_ml();
    AP_sequence *dup(void);

    void set(char *sequence);
    void set_gb(GBDATA *gbd);

    void set_sequence(); // start at st_ml->base

    void go(const ST_sequence_ml *lefts, double leftl,
            const ST_sequence_ml *rights, double rightl);
    void ungo(); // undo go

    void calc_out(ST_sequence_ml *sequence_of_brother, double dist);
    void print();
};

class AW_window;
typedef void (*AW_CB0)(AW_window*);

class ST_ML {
    char *alignment_name;
    friend AP_tree *st_ml_convert_species_name_to_node(ST_ML *st_ml,
            const char *species_name);
    GB_HASH *hash_2_ap_tree; // hash table to get from name to tree_node
    GB_HASH *keep_species_hash; // temporary hash to find
    int refresh_n;
    int *not_valid; // which columns are valid

    ST_sequence_ml *do_tree(AP_tree *node);
    void undo_tree(AP_tree *node); //opposite of do_tree
    void insert_tree_into_hash_rek(AP_tree *node);
    void create_matrizes(double max_disti, int nmatrizes);
    void create_frequencies();
    static long delete_species(const char *key, long val);
public:
    AP_tree_root *tree_root;
    int latest_modification; // last mod;
    int base;
    int to;
    AW_CB0 refresh_func;
    AW_window *aw_window;

    GBDATA *gb_main;
    float *ttratio; // column independent
    ST_base_vector *base_frequencies; // column independent
    ST_base_vector *inv_base_frequencies; // column independent
    float *rates; // column independent
    double max_dist; // max_dist for rate_matrizes
    double step_size; // max_dist/step_size matrizes
    int max_matr;
    ST_rate_matrix *rate_matrizes; // for each distance a new matrix
    long alignment_len;
    AWT_csp *awt_csp;
    void set_modified(int *what = 0);
    void set_refresh(); // set flag for refresh

    ~ST_ML();
    ST_ML(GBDATA *gb_main);
    void print();
    int is_inited;

    GB_ERROR init(const char *tree_name, const char *alignment_name,
            const char *species_names, int marked_only,
            const char *filter_string, AWT_csp *awt_csp);
    // species_names is 0 -> all [marked] species (else species_names is a (char)1 seperated list of species)
    // filter_string==0 -> no filter

    void clear_all(); // delete all caches


    ST_sequence_ml *get_ml_vectors(char *species_name, AP_tree *node,
            int start_ali_pos, int end_ali_pos);
    ST_ML_Color *get_color_string(char *species_name, AP_tree *node,
            int start_ali_pos, int end_ali_pos);

    int update_ml_likelihood(char *result[4], int *latest_update,
            char *species_name, AP_tree *node);

    int refresh_needed();
};
