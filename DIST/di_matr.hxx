
#define AWAR_DIST_PREFIX           "dist/"
#define AWAR_DIST_CORR_TRANS       AWAR_DIST_PREFIX "correction/trans"
#define AWAR_DIST_SAVE_MATRIX_BASE "tmp/" AWAR_DIST_PREFIX "save_matrix"

#define AWAR_DIST_DIST_PREFIX AWAR_DIST_PREFIX "dist/"
#define AWAR_DIST_MIN_DIST    AWAR_DIST_DIST_PREFIX "lower"
#define AWAR_DIST_MAX_DIST    AWAR_DIST_DIST_PREFIX "upper"


typedef enum {
    DI_TRANSFORMATION_NONE,
    DI_TRANSFORMATION_SIMILARITY,
    DI_TRANSFORMATION_JUKES_CANTOR,
    DI_TRANSFORMATION_FELSENSTEIN,

    DI_TRANSFORMATION_PAM,
    DI_TRANSFORMATION_CATEGORIES_HALL,
    DI_TRANSFORMATION_CATEGORIES_BARKER,
    DI_TRANSFORMATION_CATEGORIES_CHEMICAL,

    DI_TRANSFORMATION_HAESCH,
    DI_TRANSFORMATION_KIMURA,
    DI_TRANSFORMATION_OLSEN,
    DI_TRANSFORMATION_FELSENSTEIN_VOIGT,
    DI_TRANSFORMATION_OLSEN_VOIGT,
    DI_TRANSFORMATION_ML
} DI_TRANSFORMATION;

enum DI_MATRIX_TYPE {
    DI_MATRIX_FULL,
    DI_MATRIX_COMPRESSED };

class DI_MATRIX;
class AW_root;

class DI_ENTRY {
public:
    DI_ENTRY(GBDATA *gbd,class DI_MATRIX *phmatri);
    DI_ENTRY(char *namei,class DI_MATRIX *phmatri);
    ~DI_ENTRY();

    DI_MATRIX                  *phmatrix;
    AP_sequence                *sequence;
    AP_sequence_parsimony      *sequence_parsimony; // if exist ok
    AP_sequence_simple_protein *sequence_protein;
    long                        seq_len;
    char                       *name;
    char                       *full_name;
    AP_FLOAT                    gc_bias;
    int                         group_nr;           // species belongs to group number xxxx
};

typedef long DI_MUT_MATR[AP_MAX][AP_MAX];

enum DI_SAVE_TYPE {
    DI_SAVE_PHYLIP_COMP,
    DI_SAVE_READABLE,
    DI_SAVE_TABBED
};

class BI_helix;

enum LoadWhat { DI_LOAD_ALL, DI_LOAD_MARKED, DI_LOAD_LIST };

class DI_MATRIX {
private:
    friend class DI_ENTRY;

    GBDATA       *gb_main;
    GBDATA       *gb_species_data;
    char         *use;
    long          seq_len;
    char          cancel_columns[256];
    AP_tree_root *tree_root;
    AW_root      *aw_root;             // only link
    long          entries_mem_size;

public:
    GB_BOOL            is_AA;
    DI_ENTRY         **entries;
    long               nentries;
    static DI_MATRIX  *ROOT;
    AP_smatrix        *matrix;
    DI_MATRIX_TYPE     matrix_type;

    DI_MATRIX(GBDATA *gb_main,AW_root *awr);
    ~DI_MATRIX(void);

    char *load(char *use, AP_filter *filter, AP_weights *weights, LoadWhat what, GB_CSTR sort_tree_name, bool show_warnings, GBDATA **species_list);
    char *unload(void);
    const char *save(char *filename,enum DI_SAVE_TYPE type);

    void    clear(DI_MUT_MATR &hits);
    void    make_sym(DI_MUT_MATR &hits);
    void    rate_write(DI_MUT_MATR &hits,FILE *out);
    long    *create_helix_filter(BI_helix *helix,AP_filter *filter);
    // 0 non helix 1 helix; compressed filter
    GB_ERROR calculate_rates(DI_MUT_MATR &hrates,DI_MUT_MATR &nrates,DI_MUT_MATR &pairs,long *filter);
    GB_ERROR haeschoe(const char *path);
    double  corr(double dist, double b, double & sigma);
    GB_ERROR calculate(AW_root *awr, char *cancel, double alpha, DI_TRANSFORMATION transformation);
    char *calculate_overall_freqs(double rel_frequencies[AP_MAX],char *cancel_columns);
    GB_ERROR calculate_pro(DI_TRANSFORMATION transformation);
    char *analyse(AW_root *aw_root);

    int search_group(GBT_TREE *node,GB_HASH *hash, long *groupcnt,char *groupname, DI_ENTRY **groups);       // @@ OLIVER
    char *compress(GBT_TREE *tree);
};


AW_window *DI_create_save_matrix_window(AW_root *aw_root, char *base_name);
