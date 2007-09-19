
#define AWAR_DIST_PREFIX           "dist/"
#define AWAR_DIST_CORR_TRANS       AWAR_DIST_PREFIX "correction/trans"
#define AWAR_DIST_SAVE_MATRIX_BASE "tmp/" AWAR_DIST_PREFIX "save_matrix"

typedef enum {
    PH_TRANSFORMATION_NONE,
    PH_TRANSFORMATION_SIMILARITY,
    PH_TRANSFORMATION_JUKES_CANTOR,
    PH_TRANSFORMATION_FELSENSTEIN,

    PH_TRANSFORMATION_PAM,
    PH_TRANSFORMATION_CATEGORIES_HALL,
    PH_TRANSFORMATION_CATEGORIES_BARKER,
    PH_TRANSFORMATION_CATEGORIES_CHEMICAL,

    PH_TRANSFORMATION_HAESCH,
    PH_TRANSFORMATION_KIMURA,
    PH_TRANSFORMATION_OLSEN,
    PH_TRANSFORMATION_FELSENSTEIN_VOIGT,
    PH_TRANSFORMATION_OLSEN_VOIGT,
    PH_TRANSFORMATION_ML
} PH_TRANSFORMATION;

enum PH_MATRIX_TYPE {
    PH_MATRIX_FULL,
    PH_MATRIX_COMPRESSED };

class PHMATRIX;
class AW_root;

class ph_dummy {
    PHMATRIX *dummy;
};

class PHENTRY {
public:
    PHENTRY(GBDATA *gbd,class PHMATRIX *phmatri);
    PHENTRY(char *namei,class PHMATRIX *phmatri);
    ~PHENTRY();

    PHMATRIX                   *phmatrix;
    AP_sequence                *sequence;
    AP_sequence_parsimony      *sequence_parsimony; // if exist ok
    AP_sequence_simple_protein *sequence_protein;
    long                        seq_len;
    char                       *name;
    char                       *full_name;
    AP_FLOAT                    gc_bias;
    int                         group_nr; // @@ OLIVER species belongs
    // to group number xxxx
};

typedef long PH_MUT_MATR[AP_MAX][AP_MAX];

enum PH_SAVE_TYPE {
    PH_SAVE_PHYLIP_COMP,
    PH_SAVE_READABLE,
    PH_SAVE_TABBED
};

class BI_helix;

class PHMATRIX {
private:
    friend class PHENTRY;
    GBDATA *gb_main;
    GBDATA *gb_species_data;
    char    *use;
    long    seq_len;
    char    cancel_columns[256];
    AP_tree_root *tree_root;

    AW_root *aw_root;       // only link

    long    entries_mem_size;

public:
    GB_BOOL is_AA;
    PHENTRY **entries;
    long    nentries;
    static PHMATRIX *ROOT;
    AP_smatrix *matrix;
    enum PH_MATRIX_TYPE matrix_type;

    PHMATRIX(GBDATA *gb_main,AW_root *awr);
    ~PHMATRIX(void);

    char *load(char *use,AP_filter *filter,AP_weights *weights,AP_smatrix *ratematrix, int all, GB_CSTR sort_tree_name, bool show_warnings);
    char *unload(void);
    const char *save(char *filename,enum PH_SAVE_TYPE type);

    void    clear(PH_MUT_MATR &hits);
    void    make_sym(PH_MUT_MATR &hits);
    void    rate_write(PH_MUT_MATR &hits,FILE *out);
    long    *create_helix_filter(BI_helix *helix,AP_filter *filter);
    // 0 non helix 1 helix; compressed filter
    GB_ERROR calculate_rates(PH_MUT_MATR &hrates,PH_MUT_MATR &nrates,PH_MUT_MATR &pairs,long *filter);
    GB_ERROR haeschoe(const char *path);
    double  corr(double dist, double b, double & sigma);
    GB_ERROR calculate(AW_root *awr, char *cancel, double alpha, PH_TRANSFORMATION transformation);
    char *calculate_overall_freqs(double rel_frequencies[AP_MAX],char *cancel_columns);
    GB_ERROR calculate_pro(PH_TRANSFORMATION transformation);
    char *analyse(AW_root *aw_root);

    int search_group(GBT_TREE *node,GB_HASH *hash, long *groupcnt,char *groupname, PHENTRY **groups);       // @@ OLIVER
    char *compress(GBT_TREE *tree);
};


AW_window *create_save_matrix_window(AW_root *aw_root, char *base_name);
