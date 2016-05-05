// =============================================================== //
//                                                                 //
//   File      : di_matr.hxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef DI_MATR_HXX
#define DI_MATR_HXX

#ifndef AP_PRO_A_NUCS_HXX
#include <AP_pro_a_nucs.hxx>
#endif
#ifndef AP_TREE_HXX
#include <AP_Tree.hxx>
#endif
#ifndef AP_MATRIX_HXX
#include <AP_matrix.hxx>
#endif
#ifndef AP_SEQ_DNA_HXX
#include <AP_seq_dna.hxx>
#endif
#ifndef AP_SEQ_SIMPLE_PRO_HXX
#include <AP_seq_simple_pro.hxx>
#endif
#ifndef _GLIBCXX_STRING
#include <string>
#endif

#define di_assert(cond) arb_assert(cond)

#define AWAR_DIST_PREFIX           "dist/"
#define AWAR_DIST_CORR_TRANS       AWAR_DIST_PREFIX "correction/trans"
#define AWAR_DIST_SAVE_MATRIX_BASE "tmp/" AWAR_DIST_PREFIX "save_matrix"

#define AWAR_DIST_DIST_PREFIX AWAR_DIST_PREFIX "dist/"
#define AWAR_DIST_MIN_DIST    AWAR_DIST_DIST_PREFIX "lower"
#define AWAR_DIST_MAX_DIST    AWAR_DIST_DIST_PREFIX "upper"

enum DI_TRANSFORMATION {
    DI_TRANSFORMATION_NONE,
    DI_TRANSFORMATION_SIMILARITY,
    DI_TRANSFORMATION_JUKES_CANTOR,
    DI_TRANSFORMATION_FELSENSTEIN,

    DI_TRANSFORMATION_PAM,
    DI_TRANSFORMATION_CATEGORIES_HALL,
    DI_TRANSFORMATION_CATEGORIES_BARKER,
    DI_TRANSFORMATION_CATEGORIES_CHEMICAL,

    DI_TRANSFORMATION_KIMURA,
    DI_TRANSFORMATION_OLSEN,
    DI_TRANSFORMATION_FELSENSTEIN_VOIGT,
    DI_TRANSFORMATION_OLSEN_VOIGT,
    DI_TRANSFORMATION_ML,

    DI_TRANSFORMATION_FROM_TREE,

    // -------------------- real transformations are above

    DI_TRANSFORMATION_COUNT,         // amount of real transformations
    DI_TRANSFORMATION_NONE_DETECTED, // nothing was auto-detected
};

enum DI_MATRIX_TYPE {
    DI_MATRIX_FULL,
    DI_MATRIX_COMPRESSED
};

class DI_MATRIX;

class DI_ENTRY : virtual Noncopyable {
    DI_MATRIX *phmatrix;
    char      *full_name;

public:
    DI_ENTRY(GBDATA *gbd, DI_MATRIX *phmatrix_);
    DI_ENTRY(const char *name_, DI_MATRIX *phmatrix_);
    ~DI_ENTRY();

    AP_sequence *sequence;

    AP_sequence_parsimony      *get_nucl_seq() { return DOWNCAST(AP_sequence_parsimony*,      sequence); }
    AP_sequence_simple_protein *get_prot_seq() { return DOWNCAST(AP_sequence_simple_protein*, sequence); }

    char *name;
    int   group_nr;                                 // species belongs to group number xxxx
};

enum DI_SAVE_TYPE {
    DI_SAVE_PHYLIP_COMP,
    DI_SAVE_READABLE,
    DI_SAVE_TABBED
};

enum LoadWhat { DI_LOAD_ALL, DI_LOAD_MARKED, DI_LOAD_LIST };

class MatrixOrder : virtual Noncopyable {
    GB_HASH *name2pos; // key = species name, value = order in sort_tree [1..n]
                       // if no sort tree was specified, name2pos is NULL
    int      leafs;    // number of leafs

    bool tree_contains_dups; // unused (if true, matrix sorting works partly wrong)

    void insert_in_hash(TreeNode *tree) {
        if (tree->is_leaf) {
            arb_assert(tree->name);
            if (GBS_write_hash(name2pos, tree->name, ++leafs) != 0) {
                tree_contains_dups = true;
            }
        }
        else {
            insert_in_hash(tree->get_rightson());
            insert_in_hash(tree->get_leftson());
        }
    }

public:
    MatrixOrder(GBDATA *gb_main, GB_CSTR sort_tree_name);
    ~MatrixOrder() { if (name2pos) GBS_free_hash(name2pos); }

    bool defined() const { return leafs; }
    int get_index(const char *name) const {
        // return 1 for lowest and 'leafs' for highest species in sort-tee
        // return 0 for all species missing in sort-tree
        return defined() ? GBS_read_hash(name2pos, name) : -1;
    }
    void applyTo(struct TreeOrderedSpecies **gb_species_array, size_t array_size) const;
};

typedef void (*DI_MATRIX_CB)();

class DI_MATRIX : virtual Noncopyable {
    GBDATA  *gb_species_data;
    long     seq_len;
    char     cancel_columns[256];
    size_t   entries_mem_size;
    AliView *aliview;

    GBDATA *get_gb_main() const { return aliview->get_gb_main(); }
    double  corr(double dist, double b, double & sigma);
    char   *calculate_overall_freqs(double rel_frequencies[AP_MAX], char *cancel_columns);
    int     search_group(TreeNode *node, GB_HASH *hash, size_t& groupcnt, char *groupname, DI_ENTRY **groups);

public:
    // @@@ make members private:
    bool             is_AA;
    DI_ENTRY       **entries;
    size_t           nentries;
    AP_smatrix      *matrix;
    DI_MATRIX_TYPE   matrix_type;

    explicit DI_MATRIX(const AliView& aliview);
    ~DI_MATRIX();

    const char *get_aliname() const { return aliview->get_aliname(); }
    const AliView *get_aliview() const { return aliview; }

    GB_ERROR load(LoadWhat what, const MatrixOrder& order, bool show_warnings, GBDATA **species_list) __ATTR__USERESULT;
    char *unload();
    const char *save(const char *filename, enum DI_SAVE_TYPE type);

    GB_ERROR  calculate(const char *cancel, DI_TRANSFORMATION transformation, bool *aborted_flag, AP_matrix *userdef_matrix);
    GB_ERROR  calculate_pro(DI_TRANSFORMATION transformation, bool *aborted_flag);
    GB_ERROR  extract_from_tree(const char *treename, bool *aborted_flag);

    DI_TRANSFORMATION detect_transformation(std::string& msg);

    char *compress(TreeNode *tree);
};

class DI_GLOBAL_MATRIX : virtual Noncopyable {
    DI_MATRIX    *matrix;
    DI_MATRIX_CB  changed_cb;

    void announce_change() { if (changed_cb) changed_cb(); }

    void forget_no_announce() {
        delete matrix;
        matrix = NULL;
    }

    void set(DI_MATRIX *new_global) { di_assert(!matrix); matrix = new_global; announce_change(); }

public:
    DI_GLOBAL_MATRIX() : matrix(NULL), changed_cb(NULL) {}
    ~DI_GLOBAL_MATRIX() { forget(); }

    DI_MATRIX *get() { return matrix; }
    void forget() {
        if (matrix) {
            forget_no_announce();
            announce_change();
        }
    }
    void replaceBy(DI_MATRIX *new_global) { forget_no_announce(); set(new_global); }

    bool exists() const { return matrix != NULL; }

    void set_changed_cb(DI_MATRIX_CB cb) {
        // announce_change(); // do by caller if really needed
        changed_cb = cb;
    }

    DI_MATRIX *swap(DI_MATRIX *other) {
        DI_MATRIX *prev = matrix;
        matrix          = other;
        announce_change();
        return prev;
    }

    bool has_type(DI_MATRIX_TYPE type) const {
        return matrix && matrix->matrix_type == type;
    }
    void forget_if_not_has_type(DI_MATRIX_TYPE wanted_type) {
        if (matrix && matrix->matrix_type != wanted_type) {
            forget();
        }
    }
};

extern DI_GLOBAL_MATRIX GLOBAL_MATRIX;

class WeightedFilter;
struct save_matrix_params {
    const char           *awar_base;
    const WeightedFilter *weighted_filter;
};

AW_window *DI_create_save_matrix_window(AW_root *aw_root, save_matrix_params *save_params);

#else
#error di_matr.hxx included twice
#endif // DI_MATR_HXX
