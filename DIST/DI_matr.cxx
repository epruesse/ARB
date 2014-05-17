// =============================================================== //
//                                                                 //
//   File      : DI_matr.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "di_protdist.hxx"
#include "di_clusters.hxx"
#include "dist.hxx"
#include "di_view_matrix.hxx"
#include "di_awars.hxx"

#include <neighbourjoin.hxx>
#include <AP_seq_dna.hxx>
#include <AP_filter.hxx>
#include <BI_helix.hxx>
#include <CT_ctree.hxx>
#include <ColumnStat.hxx>

#include <awt.hxx>
#include <awt_sel_boxes.hxx>
#include <awt_filter.hxx>

#include <aw_preset.hxx>
#include <aw_awars.hxx>
#include <aw_file.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>

#include <gui_aliview.hxx>

#include <climits>
#include <ctime>
#include <cmath>
#include <arb_sort.h>
#include <arb_global_defs.h>
#include <macros.hxx>
#include <ad_cb.h>
#include <awt_TreeAwars.hxx>
#include <arb_defs.h>

// --------------------------------------------------------------------------------

#define AWAR_DIST_BOOTSTRAP_COUNT    AWAR_DIST_PREFIX "bootstrap/count"
#define AWAR_DIST_CANCEL_CHARS       AWAR_DIST_PREFIX "cancel/chars"
#define AWAR_DIST_MATRIX_AUTO_RECALC AWAR_DIST_PREFIX "recalc"

#define AWAR_DIST_COLUMN_STAT_NAME AWAR_DIST_COLUMN_STAT_PREFIX "name"

#define AWAR_DIST_TREE_SORT_NAME        "tmp/" AWAR_DIST_TREE_PREFIX "sort_tree_name"
#define AWAR_DIST_TREE_COMP_NAME        "tmp/" AWAR_DIST_TREE_PREFIX "compr_tree_name"
#define AWAR_DIST_TREE_STD_NAME         AWAR_DIST_TREE_PREFIX "tree_name"
#define AWAR_DIST_MATRIX_AUTO_CALC_TREE AWAR_DIST_TREE_PREFIX "autocalc"

#define AWAR_DIST_SAVE_MATRIX_TYPE     AWAR_DIST_SAVE_MATRIX_BASE "/type"
#define AWAR_DIST_SAVE_MATRIX_FILENAME AWAR_DIST_SAVE_MATRIX_BASE "/file_name"

// --------------------------------------------------------------------------------

DI_GLOBAL_MATRIX GLOBAL_MATRIX;

static MatrixDisplay *matrixDisplay = 0;
static AP_matrix      DI_dna_matrix(AP_MAX);

class BoundWindowCallback : virtual Noncopyable {
    AW_window      *aww;
    WindowCallback  cb;
public:
    BoundWindowCallback(AW_window *aww_, const WindowCallback& cb_)
        : aww(aww_),
          cb(cb_)
    {}
    void operator()() { cb(aww); }
};

static SmartPtr<BoundWindowCallback> recalculate_matrix_cb;
static SmartPtr<BoundWindowCallback> recalculate_tree_cb;

static GB_ERROR last_matrix_calculation_error = NULL;

static void matrix_changed_cb() {
    if (matrixDisplay) {
        matrixDisplay->mark(MatrixDisplay::NEED_SETUP);
        matrixDisplay->update_display();
    }
}

struct RecalcNeeded {
    bool matrix;
    bool tree;

    RecalcNeeded() : matrix(false), tree(false) { }
};

static RecalcNeeded need_recalc;

CONSTEXPR unsigned UPDATE_DELAY = 200;

static unsigned update_cb(AW_root *aw_root);
inline void add_update_cb() {
    AW_root::SINGLETON->add_timed_callback(UPDATE_DELAY, makeTimedCallback(update_cb));
}

inline void matrix_needs_recalc_cb() {
    need_recalc.matrix = true;
    add_update_cb();
}
inline void tree_needs_recalc_cb() {
    need_recalc.tree = true;
    add_update_cb();
}

inline void compressed_matrix_needs_recalc_cb() {
    if (GLOBAL_MATRIX.has_type(DI_MATRIX_COMPRESSED)) {
        matrix_needs_recalc_cb();
    }
}

static unsigned update_cb(AW_root *aw_root) {
    if (need_recalc.matrix) {
        GLOBAL_MATRIX.forget();
        need_recalc.matrix = false; // because it's forgotten

        int matrix_autocalc = aw_root->awar(AWAR_DIST_MATRIX_AUTO_RECALC)->read_int();
        if (matrix_autocalc) {
            bool recalc_now    = true;
            int  tree_autocalc = aw_root->awar(AWAR_DIST_MATRIX_AUTO_CALC_TREE)->read_int();
            if (!tree_autocalc) recalc_now = matrixDisplay ? matrixDisplay->willShow() : false;

            if (recalc_now) {
                di_assert(recalculate_matrix_cb.isSet());
                (*recalculate_matrix_cb)();
                di_assert(need_recalc.tree == true);
            }
        }
        di_assert(need_recalc.matrix == false);
    }

    if (need_recalc.tree) {
        int tree_autocalc = aw_root->awar(AWAR_DIST_MATRIX_AUTO_CALC_TREE)->read_int();
        if (tree_autocalc) {
            di_assert(recalculate_tree_cb.isSet());
            (*recalculate_tree_cb)();
            need_recalc.matrix = false; // otherwise endless loop, e.g. if output-tree is used for sorting
        }
    }

    return 0; // do not call again
}

static void auto_calc_changed_cb(AW_root *aw_root) {
    int matrix_autocalc = aw_root->awar(AWAR_DIST_MATRIX_AUTO_RECALC)->read_int();
    int tree_autocalc   = aw_root->awar(AWAR_DIST_MATRIX_AUTO_CALC_TREE)->read_int();

    if (matrix_autocalc && !GLOBAL_MATRIX.exists()) matrix_needs_recalc_cb();
    if (tree_autocalc && (matrix_autocalc || GLOBAL_MATRIX.exists())) tree_needs_recalc_cb();
}

static AW_window *create_dna_matrix_window(AW_root *aw_root) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root, "SET_DNA_MATRIX", "SET MATRIX");
    aws->auto_increment(50, 50);
    aws->button_length(10);
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE");

    aws->callback(makeHelpCallback("user_matrix.hlp"));
    aws->create_button("HELP", "HELP");

    aws->at_newline();

    DI_dna_matrix.create_input_fields(aws, AWAR_DIST_MATRIX_DNA_BASE);
    aws->window_fit();
    return aws;
}

static void selected_tree_changed_cb() {
    if (AW_root::SINGLETON->awar(AWAR_DIST_CORR_TRANS)->read_int() == DI_TRANSFORMATION_FROM_TREE) {
        matrix_needs_recalc_cb();
    }
}

void DI_create_matrix_variables(AW_root *aw_root, AW_default def, AW_default db) {
    GB_transaction ta(db);
    DI_dna_matrix.set_descriptions(AP_A, "A");
    DI_dna_matrix.set_descriptions(AP_C, "C");
    DI_dna_matrix.set_descriptions(AP_G, "G");   
    DI_dna_matrix.set_descriptions(AP_T, "TU");  
    DI_dna_matrix.set_descriptions(AP_S, "GAP"); 

    DI_dna_matrix.create_awars(aw_root, AWAR_DIST_MATRIX_DNA_BASE);

    RootCallback matrix_needs_recalc_callback = makeRootCallback(matrix_needs_recalc_cb);
    aw_root->awar_int(AWAR_DIST_MATRIX_DNA_ENABLED, 0)->add_callback(matrix_needs_recalc_callback); // user matrix disabled by default
    {
        GBDATA *gbd = GB_search(AW_ROOT_DEFAULT, AWAR_DIST_MATRIX_DNA_BASE, GB_FIND);
        GB_add_callback(gbd, GB_CB_CHANGED, makeDatabaseCallback(matrix_needs_recalc_cb));
    }

    aw_root->awar_string(AWAR_DIST_WHICH_SPECIES, "marked", def)->add_callback(matrix_needs_recalc_callback);
    {
        char *default_ali = GBT_get_default_alignment(db);
        aw_root->awar_string(AWAR_DIST_ALIGNMENT, "", def)->add_callback(matrix_needs_recalc_callback)->write_string(default_ali);
        free(default_ali);
    }
    aw_root->awar_string(AWAR_DIST_FILTER_ALIGNMENT, "none", def);
    aw_root->awar_string(AWAR_DIST_FILTER_NAME,      "none", def);
    aw_root->awar_string(AWAR_DIST_FILTER_FILTER,    "",     def)->add_callback(matrix_needs_recalc_callback);
    aw_root->awar_int   (AWAR_DIST_FILTER_SIMPLIFY,  0,      def)->add_callback(matrix_needs_recalc_callback);

    aw_root->awar_string(AWAR_DIST_CANCEL_CHARS, ".", def)->add_callback(matrix_needs_recalc_callback);
    aw_root->awar_int(AWAR_DIST_CORR_TRANS, (int)DI_TRANSFORMATION_SIMILARITY, def)->add_callback(matrix_needs_recalc_callback)->set_minmax(0, DI_TRANSFORMATION_COUNT-1);

    aw_root->awar(AWAR_DIST_FILTER_ALIGNMENT)->map(AWAR_DIST_ALIGNMENT);

    AW_create_fileselection_awars(aw_root, AWAR_DIST_SAVE_MATRIX_BASE, ".", "", "infile");
    aw_root->awar_int(AWAR_DIST_SAVE_MATRIX_TYPE, 0, def);

    enum treetype { CURR, SORT, COMPRESS, TREEAWARCOUNT };
    AW_awar *tree_awar[TREEAWARCOUNT] = { NULL, NULL, NULL };

    aw_root->awar_string(AWAR_DIST_TREE_STD_NAME,  "tree_nj", def);
    {
        char *currentTree = aw_root->awar_string(AWAR_TREE, "", db)->read_string();
        tree_awar[CURR]   = aw_root->awar_string(AWAR_DIST_TREE_CURR_NAME, currentTree, def);
        tree_awar[SORT]   = aw_root->awar_string(AWAR_DIST_TREE_SORT_NAME, currentTree, def);
        free(currentTree);
    }
    tree_awar[COMPRESS] = aw_root->awar_string(AWAR_DIST_TREE_COMP_NAME, NO_TREE_SELECTED, def);

    aw_root->awar_int(AWAR_DIST_BOOTSTRAP_COUNT, 1000, def);
    aw_root->awar_int(AWAR_DIST_MATRIX_AUTO_RECALC, 0, def)->add_callback(auto_calc_changed_cb);
    aw_root->awar_int(AWAR_DIST_MATRIX_AUTO_CALC_TREE, 0, def)->add_callback(auto_calc_changed_cb);

    aw_root->awar_float(AWAR_DIST_MIN_DIST, 0.0);
    aw_root->awar_float(AWAR_DIST_MAX_DIST, 0.0);

    aw_root->awar_string(AWAR_SPECIES_NAME, "", db);

    DI_create_cluster_awars(aw_root, def, db);

#if defined(DEBUG)
    AWT_create_db_browser_awars(aw_root, def);
#endif // DEBUG

    {
        GB_push_transaction(db);

        GBDATA *gb_species_data = GBT_get_species_data(db);
        GB_add_callback(gb_species_data, GB_CB_CHANGED, makeDatabaseCallback(matrix_needs_recalc_cb));

        GB_pop_transaction(db);
    }

    AWT_registerTreeAwarCallback(tree_awar[CURR],     makeTreeAwarCallback(selected_tree_changed_cb),          true);
    AWT_registerTreeAwarCallback(tree_awar[SORT],     makeTreeAwarCallback(matrix_needs_recalc_cb),            true);
    AWT_registerTreeAwarCallback(tree_awar[COMPRESS], makeTreeAwarCallback(compressed_matrix_needs_recalc_cb), true);

    auto_calc_changed_cb(aw_root);
}

DI_ENTRY::DI_ENTRY(GBDATA *gbd, DI_MATRIX *phmatri) {
    memset((char *)this, 0, sizeof(DI_ENTRY));
    phmatrix = phmatri;

    GBDATA *gb_ali = GB_entry(gbd, phmatrix->get_aliname());
    if (gb_ali) {
        GBDATA *gb_data = GB_entry(gb_ali, "data");
        if (gb_data) {
            if (phmatrix->is_AA) {
                sequence = sequence_protein = new AP_sequence_simple_protein(phmatrix->get_aliview());
            }
            else {
                sequence = sequence_parsimony = new AP_sequence_parsimony(phmatrix->get_aliview());
            }
            sequence->bind_to_species(gbd);
            sequence->lazy_load_sequence(); // load sequence

            name      = GBT_read_string(gbd, "name");
            full_name = GBT_read_string(gbd, "full_name");
        }
    }
}

DI_ENTRY::DI_ENTRY(char *namei, DI_MATRIX *phmatri)
{
    memset((char *)this, 0, sizeof(DI_ENTRY));
    phmatrix = phmatri;
    this->name = strdup(namei);
}

DI_ENTRY::~DI_ENTRY()
{
    delete sequence_protein;
    delete sequence_parsimony;
    free(name);
    free(full_name);

}

DI_MATRIX::DI_MATRIX(const AliView& aliview_, AW_root *awr) {
    memset((char *)this, 0, sizeof(*this));
    aw_root = awr;
    aliview = new AliView(aliview_);
}

char *DI_MATRIX::unload() {
    for (size_t i=0; i<nentries; i++) {
        delete entries[i];
    }
    freenull(entries);
    nentries = 0;
    return 0;
}

DI_MATRIX::~DI_MATRIX()
{
    unload();
    delete matrix;
    delete aliview;
}

struct TreeOrderedSpecies {
    GBDATA  *gbd;
    int      order_index;

    TreeOrderedSpecies(const MatrixOrder& order, GBDATA *gb_spec)
        : gbd(gb_spec),
          order_index(order.get_index(GBT_read_name(gbd)))
    {}
};

MatrixOrder::MatrixOrder(GBDATA *gb_main, GB_CSTR sort_tree_name)
    : name2pos(NULL),
      leafs(0)
{
    if (sort_tree_name) {
        int       size;
        GBT_TREE *sort_tree = GBT_read_tree_and_size(gb_main, sort_tree_name, GBT_TREE_NodeFactory(), &size);

        if (sort_tree) {
            leafs    = size+1;
            name2pos = GBS_create_hash(leafs, GB_IGNORE_CASE);

            IF_ASSERTION_USED(int leafsLoaded = leafs);
            leafs = 0;
            insert_in_hash(sort_tree);

            arb_assert(leafsLoaded == leafs);
        }
        else {
            GB_clear_error();
        }
    }
}
static int TreeOrderedSpecies_cmp(const void *p1, const void *p2, void *) {
    TreeOrderedSpecies *s1 = (TreeOrderedSpecies*)p1;
    TreeOrderedSpecies *s2 = (TreeOrderedSpecies*)p2;

    return s2->order_index - s1->order_index;
}

void MatrixOrder::applyTo(TreeOrderedSpecies **species_array, size_t array_size) const {
    GB_sort((void**)species_array, 0, array_size, TreeOrderedSpecies_cmp, NULL);
}

GB_ERROR DI_MATRIX::load(LoadWhat what, const MatrixOrder& order, bool show_warnings, GBDATA **species_list) {
    GBDATA     *gb_main = get_gb_main();
    const char *use     = get_aliname();

    GB_transaction ta(gb_main);

    seq_len          = GBT_get_alignment_len(gb_main, use);
    is_AA            = GBT_is_alignment_protein(gb_main, use);
    gb_species_data  = GBT_get_species_data(gb_main);
    entries_mem_size = 1000;

    entries = (DI_ENTRY **)calloc(sizeof(DI_ENTRY*), entries_mem_size);

    nentries = 0;

    size_t no_of_species = -1U;
    switch (what) {
        case DI_LOAD_ALL:
            no_of_species = GBT_get_species_count(gb_main);
            break;
        case DI_LOAD_MARKED:
            no_of_species = GBT_count_marked_species(gb_main);
            break;
        case DI_LOAD_LIST:
            di_assert(species_list);
            for (no_of_species = 0; species_list[no_of_species]; ++no_of_species) ;
            break;
    }

    di_assert(no_of_species != -1U);
    if (no_of_species<2) {
        return GBS_global_string("Not enough input species (%zu)", no_of_species);
    }

    TreeOrderedSpecies *species_to_load[no_of_species];

    {
        size_t i = 0;
        switch (what) {
            case DI_LOAD_ALL: {
                for (GBDATA *gb_species = GBT_first_species_rel_species_data(gb_species_data); gb_species; gb_species = GBT_next_species(gb_species), ++i) {
                    species_to_load[i] = new TreeOrderedSpecies(order, gb_species);
                }
                break;
            }
            case DI_LOAD_MARKED: {
                for (GBDATA *gb_species = GBT_first_marked_species_rel_species_data(gb_species_data); gb_species; gb_species = GBT_next_marked_species(gb_species), ++i) {
                    species_to_load[i] = new TreeOrderedSpecies(order, gb_species);
                }
                break;
            }
            case DI_LOAD_LIST: {
                for (i = 0; species_list[i]; ++i) {
                    species_to_load[i] = new TreeOrderedSpecies(order, species_list[i]);
                }
                break;
            }
        }
        arb_assert(i == no_of_species);
    }

    if (order.defined()) {
        order.applyTo(species_to_load, no_of_species);
        if (show_warnings) {
            int species_not_in_sort_tree = 0;
            for (size_t i = 0; i<no_of_species; ++i) {
                if (!species_to_load[i]->order_index) {
                    species_not_in_sort_tree++;
                }
            }
            if (species_not_in_sort_tree) {
                aw_message(GBS_global_string("Warning: %i of the affected species are not in sort-tree", species_not_in_sort_tree));
            }
        }
    }
    else {
        if (show_warnings) {
            static bool shown = false;
            if (!shown) { // showing once is enough
                aw_message("Warning: No valid tree given to sort matrix (using default database order)");
                shown = true;
            }
        }
    }

    if (no_of_species>entries_mem_size) {
        entries_mem_size = no_of_species;
        realloc_unleaked(entries, sizeof(DI_ENTRY*)*entries_mem_size);
        if (!entries) return "out of memory";
    }

    GB_ERROR     error = NULL;
    arb_progress progress("Preparing sequence data", no_of_species);
    for (size_t i = 0; i<no_of_species && !error; ++i) {
        DI_ENTRY *phentry = new DI_ENTRY(species_to_load[i]->gbd, this);
        if (phentry->sequence) {    // a species found
            arb_assert(nentries<entries_mem_size);
            entries[nentries++] = phentry;
        }
        else {
            delete phentry;
        }
        delete species_to_load[i];
        species_to_load[i] = NULL;

        progress.inc_and_check_user_abort(error);
    }

    return error;
}

void DI_MATRIX::clear(DI_MUT_MATR &hits)
{
    int i, j;
    for (i=0; i<AP_MAX; i++) {
        for (j=0; j<AP_MAX; j++) {
            hits[i][j] = 0;
        }
    }
}

void DI_MATRIX::make_sym(DI_MUT_MATR &hits)
{
    int i, j;
    for (i=AP_A; i<AP_MAX; i*=2) {
        for (j=AP_A; j<=i; j*=2) {
            hits[i][j] = hits[j][i] = hits[i][j] + hits[j][i];
        }
    }
}

void DI_MATRIX::rate_write(DI_MUT_MATR &hits, FILE *out) {
    int i, j;
    for (i=AP_A; i<AP_MAX; i*=2) {
        for (j=AP_A; j<AP_MAX; j*=2) {
            fprintf(out, "%5li ", hits[i][j]);
        }
        fprintf(out, "\n");
    }
}

long *DI_MATRIX::create_helix_filter(BI_helix *helix, const AP_filter *filter) {
    long   *result = (long *)calloc(sizeof(long), filter->get_filtered_length());
    long   *temp   = (long *)calloc(sizeof(long), filter->get_filtered_length());
    int     count  = 0;
    size_t  i;

    for (i=0; i<filter->get_length(); i++) {
        if (filter->use_position(i)) {
            temp[i] = count;
            if (helix->pairtype(i) == HELIX_PAIR) {
                result[count] = helix->opposite_position(i);
            }
            else {
                result[count] = -1;
            }
            count++;
        }
    }
    while (--count >= 0) {
        if (result[count] >= 0) {
            result[count] = temp[result[count]];
        }
    }
    free(temp);
    return result;
}

GB_ERROR DI_MATRIX::calculate_rates(DI_MUT_MATR &hrates, DI_MUT_MATR &nrates, DI_MUT_MATR &pairs, long *filter) {
    if (nentries<=1) {
        return "Not enough species selected to calculate rates";
    }

    arb_progress progress("rates", matrix_halfsize(nentries, false));
    GB_ERROR     error = NULL;

    long s_len = aliview->get_length();

    this->clear(hrates);
    this->clear(nrates);
    this->clear(pairs);

    for (size_t row = 0; row<nentries && !error; row++) {
        for (size_t col=0; col<row; col++) {
            const unsigned char *seq1 = entries[row]->sequence_parsimony->get_usequence();
            const unsigned char *seq2 = entries[col]->sequence_parsimony->get_usequence();
            for (long pos = 0; pos < s_len; pos++) {
                if (filter[pos]>=0) {
                    hrates[*seq1][*seq2]++;
                }
                else {
                    nrates[*seq1][*seq2]++;
                }
                seq1++; seq2++;
            }
            progress.inc_and_check_user_abort(error);
        }
    }
    for (size_t row = 0; row<nentries; row++) {
        const unsigned char *seq1 = entries[row]->sequence_parsimony->get_usequence();
        for (long pos = 0; pos < s_len; pos++) {
            if (filter[pos]>=0) {
                pairs[seq1[pos]][seq1[filter[pos]]]++;
            }
        }
    }
    progress.done();
    return error;
}

// ----------------------------------------------------------------
//      Some test functions to check correlated base correction

GB_ERROR DI_MATRIX::haeschoe(const char *path) {
    static BI_helix *helix = 0;
    if (!helix) helix      = new BI_helix;

    GB_ERROR error = helix->init(get_gb_main());
    if (!error) {
        FILE *out = fopen(path, "w");
        if (!out) {
            GB_export_IO_error("writing", path);
            error = GB_await_error();
        }
        else {
            arb_progress progress("Calculating distance matrix");

            fprintf(out, "Pairs in helical regions:\n");
            long *filter = create_helix_filter(helix, aliview->get_filter());
            
            DI_MUT_MATR temp, temp2, pairs;
            error = calculate_rates(temp, temp2, pairs, filter);
            if (!error) {
                rate_write(pairs, out);
                make_sym(temp); make_sym(temp2);
                fprintf(out, "\nRatematrix helical parts:\n");
                rate_write(temp, out);
                fprintf(out, "\nRatematrix non helical parts:\n");
                rate_write(temp2, out);

                long pos;
                long s_len;

                s_len = aliview->get_length();
                fprintf(out, "\nDistance matrix (Helixdist Helixlen Nonhelixdist Nonhelixlen):");
                fprintf(out, "\n%zu", nentries);

                const int MAXDISTDEBUG = 1000;
                double    distdebug[MAXDISTDEBUG];

                for (pos = 0; pos<MAXDISTDEBUG; pos++) distdebug[pos] = 0.0;

                arb_progress dist_progress("distance", matrix_halfsize(nentries, false));
                for (size_t row = 0; row<nentries && !error; row++) {
                    fprintf (out, "\n%s  ", entries[row]->name);

                    for (size_t col=0; col<row && !error; col++) {
                        const unsigned char *seq1, *seq2;

                        seq1 = entries[row]->sequence_parsimony->get_usequence();
                        seq2 = entries[col]->sequence_parsimony->get_usequence();
                        this->clear(temp);
                        this->clear(temp2);

                        for (pos = 0; pos < s_len; pos++) {
                            if (filter[pos]>=0) temp [*seq1][*seq2]++;
                            else                temp2[*seq1][*seq2]++;
                            seq1++; seq2++;
                        }
                        long hdist = 0, hall2 = 0;
                        int i, j;
                        for (i=AP_A; i<AP_MAX; i*=2) {
                            for (j=AP_A; j<AP_MAX; j*=2) {
                                hall2 += temp[i][j];
                                if (i!=j) hdist += temp[i][j];
                            }
                        }

                        long dist = 0, all = 0;
                        for (i=AP_A; i<=AP_T; i*=2) {
                            for (j=AP_A; j<=AP_T; j*=2) {
                                all += temp2[i][j];
                                if (i!=j) dist += temp2[i][j];
                            }
                        }
                        fprintf(out, "(%4li:%4li %4li:%4li) ", hdist, hall2, dist, all);
                        if (all>100) {
                            distdebug[dist*MAXDISTDEBUG/all] = (double)hdist/(double)hall2;
                        }
                        dist_progress.inc_and_check_user_abort(error);
                    }
                }

                if (!error) {
                    fprintf (out, "\n");
                    fprintf (out, "\nhdist/dist:\n");

                    for (pos = 1; pos<MAXDISTDEBUG; pos++) {
                        if (distdebug[pos]) {
                            fprintf(out, "%4f %5f\n", (double)pos/(double)MAXDISTDEBUG, distdebug[pos]/(double)pos*(double)MAXDISTDEBUG);
                        }
                    }
                }
            }
            
            fclose(out);
        }
    }
    return error;
}

char *DI_MATRIX::calculate_overall_freqs(double rel_frequencies[AP_MAX], char *cancel)
{
    long hits2[AP_MAX];
    long sum   = 0;
    int  i;
    int  pos;
    int  b;
    long s_len = aliview->get_length();

    memset((char *) &hits2[0], 0, sizeof(hits2));
    for (size_t row = 0; row < nentries; row++) {
        const char *seq1 = entries[row]->sequence_parsimony->get_sequence();
        for (pos = 0; pos < s_len; pos++) {
            b = *(seq1++);
            if (cancel[b]) continue;
            hits2[b]++;
        }
    }
    for (i = 0; i < AP_MAX; i++)    sum += hits2[i];
    for (i = 0; i < AP_MAX; i++)    rel_frequencies[i] = hits2[i] / (double) sum;
    return 0;
}

double DI_MATRIX::corr(double dist, double b, double & sigma) {
    const double eps = 0.01;
    double ar = 1.0 - dist/b;
    sigma = 1000.0;
    if (ar< eps) return 3.0;
    sigma = b/ar;
    return - b * log(1-dist/b);
}

GB_ERROR DI_MATRIX::calculate(AW_root *awr, char *cancel, double /* alpha */, DI_TRANSFORMATION transformation, bool *aborted_flag)
{
    if (transformation == DI_TRANSFORMATION_HAESCH) {
        GB_ERROR error = haeschoe("outfile");
        if (error) return error;
        return "Your matrices have been written to 'outfile'\nSorry I can not make a tree";
    }
    int user_matrix_enabled = awr->awar(AWAR_DIST_MATRIX_DNA_ENABLED)->read_int();
    if (user_matrix_enabled) {  // set transformation Matrix
        switch (transformation) {
            case DI_TRANSFORMATION_NONE:
            case DI_TRANSFORMATION_SIMILARITY:
            case DI_TRANSFORMATION_JUKES_CANTOR:
                break;
            default:
                aw_message("Sorry not implemented:\n"
                           "    This kind of distance correction does not support\n"
                           "    a user defined matrix - it will be ignored");
                user_matrix_enabled = false;
                break;
        }
        DI_dna_matrix.read_awars(awr, AWAR_DIST_MATRIX_DNA_BASE);
        DI_dna_matrix.normize();
    }



    matrix = new AP_smatrix(nentries);

    long   s_len = aliview->get_length();
    long   hits[AP_MAX][AP_MAX];
    size_t i;

    if (nentries<=1) {
        return "Not enough species selected to calculate matrix";
    }
    memset(&cancel_columns[0], 0, 256);

    for (i=0; i<strlen(cancel); i++) {
        int j = cancel[i];
        j = AP_sequence_parsimony::table[j];
        cancel_columns[j] = 1;
    }
    long    columns;
    double b;
    long frequencies[AP_MAX];
    double rel_frequencies[AP_MAX];
    double S_square=0;

    switch (transformation) {
        case DI_TRANSFORMATION_FELSENSTEIN:
            this->calculate_overall_freqs(rel_frequencies, cancel_columns);
            S_square = 0.0;
            for (i=0; i<AP_MAX; i++) S_square += rel_frequencies[i]*rel_frequencies[i];
            break;
        default:    break;
    };

    arb_progress progress("Calculating distance matrix", matrix_halfsize(nentries, true));
    GB_ERROR     error = NULL;
    for (size_t row = 0; row<nentries && !error; row++) {
        for (size_t col=0; col<=row && !error; col++) {
            columns = 0;
            
            const unsigned char *seq1 = entries[row]->sequence_parsimony->get_usequence();
            const unsigned char *seq2 = entries[col]->sequence_parsimony->get_usequence();

            b = 0.0;
            switch (transformation) {
                case  DI_TRANSFORMATION_FROM_TREE:
                    di_assert(0);
                    break;
                case  DI_TRANSFORMATION_JUKES_CANTOR:
                    b = 0.75;
                    // fall-through
                case  DI_TRANSFORMATION_NONE:
                case  DI_TRANSFORMATION_SIMILARITY: {
                    double  dist = 0.0;
                    if (user_matrix_enabled) {
                        memset((char *)hits, 0, sizeof(long) * AP_MAX * AP_MAX);
                        int pos;
                        for (pos = s_len; pos >= 0; pos--) {
                            hits[*(seq1++)][*(seq2++)]++;
                        }
                        int x, y;
                        double diffsum = 0.0;
                        double all_sum = 0.001;
                        for (x = AP_A; x < AP_MAX; x*=2) {
                            for (y = AP_A; y < AP_MAX; y*=2) {
                                if (x==y) {
                                    all_sum += hits[x][y];
                                }
                                else {
                                    diffsum += hits[x][y] * DI_dna_matrix.get(x, y);
                                    all_sum += hits[x][y] * DI_dna_matrix.get(x, y);
                                }
                            }
                        }
                        dist = diffsum / all_sum;
                    }
                    else {
                        for (int pos = s_len; pos >= 0; pos--) {
                            int b1 = *(seq1++);
                            int b2 = *(seq2++);
                            if (cancel_columns[b1]) continue;
                            if (cancel_columns[b2]) continue;
                            columns++;
                            if (b1&b2) continue;
                            dist+=1.0;
                        }
                        if (columns == 0) columns = 1;
                        dist /= columns;
                    }
                    if (transformation==DI_TRANSFORMATION_SIMILARITY) {
                        dist =  (1.0-dist);
                    }
                    else if (b) {
                        double sigma;
                        dist = this->corr(dist, b, sigma);
                    }
                    matrix->set(row, col, dist);
                    break;
                }
                case DI_TRANSFORMATION_KIMURA:
                case DI_TRANSFORMATION_OLSEN:
                case DI_TRANSFORMATION_FELSENSTEIN: {
                    int    pos;
                    double dist = 0.0;
                    long   N, P, Q, M;
                    double p, q;

                    memset((char *)hits, 0, sizeof(long) * AP_MAX * AP_MAX);
                    for (pos = s_len; pos >= 0; pos--) {
                        hits[*(seq1++)][*(seq2++)]++;
                    }
                    switch (transformation) {
                        case DI_TRANSFORMATION_KIMURA:
                            P = hits[AP_A][AP_G] +
                                hits[AP_G][AP_A] +
                                hits[AP_C][AP_T] +
                                hits[AP_T][AP_C];
                            Q = hits[AP_A][AP_C] +
                                hits[AP_A][AP_T] +
                                hits[AP_C][AP_A] +
                                hits[AP_T][AP_A] +
                                hits[AP_G][AP_C] +
                                hits[AP_G][AP_T] +
                                hits[AP_C][AP_G] +
                                hits[AP_T][AP_G];
                            M = hits[AP_A][AP_A] +
                                hits[AP_C][AP_C] +
                                hits[AP_G][AP_G] +
                                hits[AP_T][AP_T];
                            N = P+Q+M;
                            if (N==0) N=1;
                            p = (double)P/(double)N;
                            q = (double)Q/(double)N;
                            dist = - .5 * log(
                                              (1.0-2.0*p-q)*sqrt(1.0-2.0*q)
                                              );
                            break;

                        case DI_TRANSFORMATION_OLSEN:
                        case DI_TRANSFORMATION_FELSENSTEIN:

                            memset((char *)frequencies, 0,
                                   sizeof(long) * AP_MAX);

                            N = 0;
                            M = 0;

                            for (i=0; i<AP_MAX; i++) {
                                if (cancel_columns[i]) continue;
                                unsigned int j;
                                for (j=0; j<i; j++) {
                                    if (cancel_columns[j]) continue;
                                    frequencies[i] +=
                                        hits[i][j]+
                                        hits[j][i];
                                }
                                frequencies[i] += hits[i][i];
                                N += frequencies[i];
                                M += hits[i][i];
                            }
                            if (N==0) N=1;
                            if (transformation == DI_TRANSFORMATION_OLSEN) { // Calc sum square freq individually for each line
                                S_square = 0.0;
                                for (i=0; i<AP_MAX; i++) S_square += frequencies[i]*frequencies[i];
                                b = 1.0 - S_square/((double)N*(double)N);
                            }
                            else {
                                b = 1.0 - S_square;
                            }

                            dist = ((double)(N-M)) / (double) N;
                            double sigma;
                            dist = this->corr(dist, b, sigma);
                            break;

                        default: return "Sorry: Transformation not implemented";
                    }
                    matrix->set(row, col, dist);
                    break;
                }
                default:;
            }   // switch
            progress.inc_and_check_user_abort(error);
        }
    }
    if (aborted_flag && progress.aborted()) *aborted_flag = true;
    return error;
}

GB_ERROR DI_MATRIX::calculate_pro(DI_TRANSFORMATION transformation, bool *aborted_flag) {
    di_cattype whichcat;
    di_codetype whichcode = UNIVERSAL;

    switch (transformation) {
        case DI_TRANSFORMATION_NONE:                whichcat = NONE;       break;
        case DI_TRANSFORMATION_SIMILARITY:          whichcat = SIMILARITY; break;
        case DI_TRANSFORMATION_KIMURA:              whichcat = KIMURA;     break;
        case DI_TRANSFORMATION_PAM:                 whichcat = PAM;        break;
        case DI_TRANSFORMATION_CATEGORIES_HALL:     whichcat = HALL;       break;
        case DI_TRANSFORMATION_CATEGORIES_BARKER:   whichcat = GEORGE;     break;
        case DI_TRANSFORMATION_CATEGORIES_CHEMICAL: whichcat = CHEMICAL;   break;
        default:
            return "This correction is not available for protein data";
    }
    matrix = new AP_smatrix(nentries);

    di_protdist prodist(whichcode, whichcat, nentries, entries, aliview->get_length(), matrix);
    return prodist.makedists(aborted_flag);
}

struct lessCCP { bool operator()(const char *s1, const char *s2) const { return strcmp(s1, s2)<0; } };
typedef std::map<const char*, GBT_TREE*, lessCCP> NamedNodes;

GB_ERROR link_to_tree(NamedNodes& named, GBT_TREE *node) {
    GB_ERROR error = NULL;
    if (node->is_leaf) {
        NamedNodes::iterator found = named.find(node->name);
        if (found != named.end()) {
            if (found->second) {
                error = GBS_global_string("Invalid tree (two nodes named '%s')", node->name);
            }
            else {
                found->second = node;
            }
        }
        // otherwise, we do not care about the node (e.g. because it is not marked)
    }
    else {
        error             = link_to_tree(named, node->get_leftson());
        if (!error) error = link_to_tree(named, node->get_rightson());
    }
    return error;
}

static GBT_TREE *findNode(GBT_TREE *node, const char *name) {
    if (node->is_leaf) {
        return strcmp(node->name, name) == 0 ? node : NULL;
    }

    GBT_TREE *found   = findNode(node->get_leftson(), name);
    if (!found) found = findNode(node->get_rightson(), name);
    return found;
}

static GB_ERROR init(NamedNodes& node, GBT_TREE *tree, const DI_ENTRY*const*const entries, size_t nentries) {
    GB_ERROR error = NULL;
    for (size_t n = 0; n<nentries; ++n) {
        node[entries[n]->name] = NULL;
    }
    error = link_to_tree(node, tree);
    if (!error) { // check for missing species (needed but not in tree)
        size_t      missing     = 0;
        const char *exampleName = NULL;

        for (size_t n = 0; n<nentries; ++n) {
            NamedNodes::iterator found = node.find(entries[n]->name);
            if (found == node.end()) {
                ++missing;
                exampleName = entries[n]->name;
            }
            else {
                di_assert(node[entries[n]->name] == findNode(tree, entries[n]->name));
                if (!node[entries[n]->name]) {
                    ++missing;
                    exampleName = entries[n]->name;
                }
            }
        }

        if (missing) {
            error = GBS_global_string("Tree is missing %zu required species (e.g. '%s')", missing, exampleName);
        }
    }
    return error;
}

GB_ERROR DI_MATRIX::extract_from_tree(const char *treename, bool *aborted_flag) {
    GB_ERROR error         = NULL;
    if (nentries<=1) error = "Not enough species selected to calculate matrix";
    else {
        GBT_TREE *tree;
        {
            GB_transaction ta(get_gb_main());
            tree = GBT_read_tree(get_gb_main(), treename, GBT_TREE_NodeFactory());
        }
        if (!tree) error = GB_await_error();
        else {
            arb_progress progress("Extracting distances from tree", matrix_halfsize(nentries, true));
            NamedNodes   node;

            error  = init(node, tree, entries, nentries);
            matrix = new AP_smatrix(nentries);

            for (size_t row = 0; row<nentries && !error; row++) {
                GBT_TREE *rnode = node[entries[row]->name];
                for (size_t col=0; col<=row && !error; col++) {
                    double dist;
                    if (col != row) {
                        GBT_TREE *cnode = node[entries[col]->name];
                        dist  = rnode->intree_distance_to(cnode);
                    }
                    else {
                        dist = 0.0;
                    }
                    matrix->set(row, col, dist);
                    progress.inc_and_check_user_abort(error);
                }
            }
            delete tree;
            if (aborted_flag && progress.aborted()) *aborted_flag = true;
            if (error) progress.done();
        }
    }
    return error;
}

__ATTR__USERESULT static GB_ERROR di_calculate_matrix(AW_root *aw_root, const WeightedFilter *weighted_filter, bool bootstrap_flag, bool show_warnings, bool *aborted_flag) {
    // sets 'aborted_flag' to true, if it is non-NULL and the calculation has been aborted
    GB_ERROR error = NULL;

    if (GLOBAL_MATRIX.exists()) {
        di_assert(!need_recalc.matrix);
    }
    else {
        GB_transaction ta(GLOBAL_gb_main);

        char *use     = aw_root->awar(AWAR_DIST_ALIGNMENT)->read_string();
        long  ali_len = GBT_get_alignment_len(GLOBAL_gb_main, use);

        if (ali_len<=0) {
            error = "Please select a valid alignment";
            GB_clear_error();
        }
        else {
            arb_progress  progress("Calculating matrix");
            char         *cancel  = aw_root->awar(AWAR_DIST_CANCEL_CHARS)->read_string();
            AliView      *aliview = weighted_filter->create_aliview(use, error);

            if (!error) {
                if (bootstrap_flag) aliview->get_filter()->enable_bootstrap();

                char *load_what      = aw_root->awar(AWAR_DIST_WHICH_SPECIES)->read_string();
                char *sort_tree_name = aw_root->awar(AWAR_DIST_TREE_SORT_NAME)->read_string();

                LoadWhat all_flag = (strcmp(load_what, "all") == 0) ? DI_LOAD_ALL : DI_LOAD_MARKED;
                {
                    DI_MATRIX *phm   = new DI_MATRIX(*aliview, aw_root);
                    phm->matrix_type = DI_MATRIX_FULL;

                    static SmartCharPtr          last_sort_tree_name;
                    static SmartPtr<MatrixOrder> last_order;

                    if (last_sort_tree_name.isNull() || !sort_tree_name || strcmp(&*last_sort_tree_name, sort_tree_name) != 0) {
                        last_sort_tree_name = nulldup(sort_tree_name);
                        last_order = new MatrixOrder(GLOBAL_gb_main, sort_tree_name);
                    }
                    di_assert(last_order.isSet());
                    error = phm->load(all_flag, *last_order, show_warnings, NULL);

                    free(sort_tree_name);
                    error = ta.close(error);

                    bool aborted = false;
                    if (!error) {
                        if (progress.aborted()) {
                            phm->unload();
                            error   = "Aborted by user";
                            aborted = true;
                        }
                        else {
                            DI_TRANSFORMATION trans = (DI_TRANSFORMATION)aw_root->awar(AWAR_DIST_CORR_TRANS)->read_int();

                            if (trans == DI_TRANSFORMATION_FROM_TREE) {
                                const char *treename = aw_root->awar(AWAR_DIST_TREE_CURR_NAME)->read_char_pntr();
                                error                = phm->extract_from_tree(treename, &aborted);
                            }
                            else {
                                if (phm->is_AA) error = phm->calculate_pro(trans, &aborted);
                                else error            = phm->calculate(aw_root, cancel, 0.0, trans, &aborted);
                            }
                        }
                    }

                    if (aborted) {
                        di_assert(error);
                        if (aborted_flag) *aborted_flag = true;
                    }
                    if (error) {
                        delete phm;
                        GLOBAL_MATRIX.forget();
                    }
                    else {
                        GLOBAL_MATRIX.replaceBy(phm);
                        tree_needs_recalc_cb();
                        need_recalc.matrix = false;
                    }
                }
                free(load_what);
            }

            free(cancel);
            delete aliview;
        }
        free(use);

        di_assert(contradicted(error, GLOBAL_MATRIX.exists()));
    }
    return error;
}

static void di_mark_by_distance(AW_window *aww, WeightedFilter *weighted_filter) {
    AW_root *aw_root    = aww->get_root();
    double   lowerBound = aw_root->awar(AWAR_DIST_MIN_DIST)->read_float();
    double   upperBound = aw_root->awar(AWAR_DIST_MAX_DIST)->read_float();

    GB_ERROR error = 0;
    if (lowerBound >= upperBound) {
        error = GBS_global_string("Lower bound (%f) has to be smaller than upper bound (%f)", lowerBound, upperBound);
    }
    else if (lowerBound<0.0 || lowerBound > 1.0) {
        error = GBS_global_string("Lower bound (%f) is not in allowed range [0.0 .. 1.0]", lowerBound);
    }
    else if (upperBound<0.0 || upperBound > 1.0) {
        error = GBS_global_string("Upper bound (%f) is not in allowed range [0.0 .. 1.0]", upperBound);
    }
    else {
        GB_transaction ta(GLOBAL_gb_main);

        char *selected = aw_root->awar(AWAR_SPECIES_NAME)->read_string();
        if (!selected[0]) {
            error = "Please select a species";
        }
        else {
            GBDATA *gb_selected = GBT_find_species(GLOBAL_gb_main, selected);
            if (!gb_selected) {
                error = GBS_global_string("Couldn't find species '%s'", selected);
            }
            else {
                char              *use     = aw_root->awar(AWAR_DIST_ALIGNMENT)->read_string();
                char              *cancel  = aw_root->awar(AWAR_DIST_CANCEL_CHARS)->read_string();
                DI_TRANSFORMATION  trans   = (DI_TRANSFORMATION)aw_root->awar(AWAR_DIST_CORR_TRANS)->read_int();
                AliView           *aliview = weighted_filter->create_aliview(use, error);

                if (!error) {
                    DI_MATRIX *prev_global = GLOBAL_MATRIX.swap(NULL);

                    size_t speciesCount   = GBT_get_species_count(GLOBAL_gb_main);
                    bool   markedSelected = false;

                    arb_progress progress("Mark species by distance", speciesCount);
                    MatrixOrder  order(GLOBAL_gb_main, NULL);

                    for (GBDATA *gb_species = GBT_first_species(GLOBAL_gb_main);
                         gb_species && !error;
                         gb_species = GBT_next_species(gb_species))
                    {
                        DI_MATRIX *phm         = new DI_MATRIX(*aliview, aw_root);
                        phm->matrix_type       = DI_MATRIX_FULL;
                        GBDATA *species_pair[] = { gb_selected, gb_species, NULL };

                        error = phm->load(DI_LOAD_LIST, order, false, species_pair);

                        if (phm->nentries == 2) { // if species has no alignment -> nentries<2
                            if (!error) {
                                if (phm->is_AA) {
                                    error = phm->calculate_pro(trans, NULL);
                                }
                                else {
                                    error = phm->calculate(aw_root, cancel, 0.0, trans, NULL);
                                }
                            }

                            if (!error) {
                                double dist_value = phm->matrix->get(0, 1);                         // distance or conformance
                                bool   mark       = (lowerBound <= dist_value && dist_value <= upperBound);
                                GB_write_flag(gb_species, mark);

                                if (!markedSelected) {
                                    dist_value = phm->matrix->get(0, 0);                                     // distance or conformance to self
                                    mark       = (lowerBound <= dist_value && dist_value <= upperBound);
                                    GB_write_flag(gb_selected, mark);

                                    markedSelected = true;
                                }
                            }
                        }

                        delete phm;
                        if (!error) progress.inc_and_check_user_abort(error);
                    }

                    di_assert(!GLOBAL_MATRIX.exists());
                    ASSERT_RESULT(DI_MATRIX*, NULL, GLOBAL_MATRIX.swap(prev_global));

                    if (error) progress.done();
                }

                delete aliview;
                free(cancel);
                free(use);
            }
        }

        free(selected);
        error = ta.close(error);
    }

    if (error) {
        aw_message(error);
    }
}

static void di_calculate_full_matrix_cb(AW_window *aww, const WeightedFilter *weighted_filter);

static GB_ERROR di_recalc_matrix() {
    // recalculate matrix
    last_matrix_calculation_error = NULL;
    if (need_recalc.matrix && GLOBAL_MATRIX.exists()) {
        GLOBAL_MATRIX.forget();
    }
    di_assert(recalculate_matrix_cb.isSet());
    (*recalculate_matrix_cb)();
    return last_matrix_calculation_error;
}

static void di_view_matrix_cb(AW_window *aww, save_matrix_params *sparam) {
    GB_ERROR error = di_recalc_matrix();
    if (error) return;

    if (!matrixDisplay) matrixDisplay = new MatrixDisplay;

    static AW_window *viewer = 0;
    if (!viewer) viewer = DI_create_view_matrix_window(aww->get_root(), matrixDisplay, sparam);

    matrixDisplay->mark(MatrixDisplay::NEED_SETUP);
    matrixDisplay->update_display();

    GLOBAL_MATRIX.set_changed_cb(matrix_changed_cb);

    viewer->activate();
}

static void di_save_matrix_cb(AW_window *aww) {
    // save the matrix
    GB_ERROR error = di_recalc_matrix();
    if (!error) {
        char              *filename = aww->get_root()->awar(AWAR_DIST_SAVE_MATRIX_FILENAME)->read_string();
        enum DI_SAVE_TYPE  type     = (enum DI_SAVE_TYPE)aww->get_root()->awar(AWAR_DIST_SAVE_MATRIX_TYPE)->read_int();

        GLOBAL_MATRIX.get()->save(filename, type);
        free(filename);
    }
    AW_refresh_fileselection(aww->get_root(), AWAR_DIST_SAVE_MATRIX_BASE);
    aww->hide_or_notify(error);
}

AW_window *DI_create_save_matrix_window(AW_root *aw_root, save_matrix_params *save_params) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;
        aws->init(aw_root, "SAVE_MATRIX", "Save Matrix");
        aws->load_xfig("sel_box_user.fig");

        aws->at("close"); aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE", "CANCEL", "C");


        aws->at("help"); aws->callback(makeHelpCallback("save_matrix.hlp"));
        aws->create_button("HELP", "HELP", "H");

        aws->at("user");
        aws->create_option_menu(AWAR_DIST_SAVE_MATRIX_TYPE, true);
        aws->insert_default_option("Phylip Format (Lower Triangular Matrix)", "P", DI_SAVE_PHYLIP_COMP);
        aws->insert_option("Readable (using NDS)", "R", DI_SAVE_READABLE);
        aws->insert_option("Tabbed (using NDS)", "R", DI_SAVE_TABBED);
        aws->update_option_menu();

        AW_create_standard_fileselection(aws, save_params->awar_base);

        aws->at("save2");
        aws->callback(makeWindowCallback(di_save_matrix_cb));
        aws->create_button("SAVE", "SAVE", "S");

        aws->callback((AW_CB0)AW_POPDOWN);
        aws->at("cancel2");
        aws->create_button("CLOSE", "CANCEL", "C");
    }
    return aws;
}

static AW_window *awt_create_select_cancel_window(AW_root *aw_root) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root, "SELECT_CHARS_TO_CANCEL_COLUMN", "CANCEL SELECT");
    aws->load_xfig("di_cancel.fig");

    aws->at("close"); aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("cancel");
    aws->create_input_field(AWAR_DIST_CANCEL_CHARS, 12);

    return (AW_window *)aws;
}

static const char *enum_trans_to_string[] = {
    "none",
    "similarity",
    "jukes_cantor",
    "felsenstein",

    "pam",
    "hall",
    "barker",
    "chemical",

    "haesch",
    "kimura",
    "olsen",
    "felsenstein voigt",
    "olsen voigt",
    "max ml",

    NULL, // treedist
};

STATIC_ASSERT(ARRAY_ELEMS(enum_trans_to_string) == DI_TRANSFORMATION_COUNT);

static void di_calculate_tree_cb(AW_window *aww, WeightedFilter *weighted_filter, bool bootstrap_flag) {
    recalculate_tree_cb = new BoundWindowCallback(aww, makeWindowCallback(di_calculate_tree_cb, weighted_filter, bootstrap_flag));

    AW_root  *aw_root   = aww->get_root();
    GB_ERROR  error     = 0;
    StrArray *all_names = 0;

    int loop_count      = 0;
    int bootstrap_count = aw_root->awar(AWAR_DIST_BOOTSTRAP_COUNT)->read_int();

    {
        char *tree_name = aw_root->awar(AWAR_DIST_TREE_STD_NAME)->read_string();
        error           = GBT_check_tree_name(tree_name);
        free(tree_name);
    }

    SmartPtr<arb_progress>  progress;
    SmartPtr<ConsensusTree> ctree;

    if (!error) {
        if (bootstrap_flag) {
            if (bootstrap_count) {
                progress = new arb_progress("Calculating bootstrap trees", bootstrap_count+1);
            }
            else {
                progress = new arb_progress("Calculating bootstrap trees (KILL to stop)", INT_MAX);
            }
            progress->auto_subtitles("tree");
        }
        else {
            progress = new arb_progress("Calculating tree");
        }

        if (bootstrap_flag) {
            GLOBAL_MATRIX.forget();
            GLOBAL_MATRIX.set_changed_cb(NULL); // otherwise matrix window will repeatedly pop up/down

            error = di_calculate_matrix(aw_root, weighted_filter, bootstrap_flag, true, NULL);
            if (!error) {
                DI_MATRIX *matr = GLOBAL_MATRIX.get();
                if (!matr) {
                    error = "unexpected error in di_calculate_matrix_cb (data missing)";
                }
                else {
                    all_names = new StrArray;
                    all_names->reserve(matr->nentries+2);

                    for (size_t i=0; i<matr->nentries; i++) {
                        all_names->put(strdup(matr->entries[i]->name));
                    }
                    ctree = new ConsensusTree(*all_names);
                }
            }
        }
    }

    GBT_TREE *tree = 0;
    do {
        if (error) break;

        bool aborted = false;

        if (bootstrap_flag) {
            if (loop_count>0) { // in first loop we already have a valid matrix -> no need to recalculate
                GLOBAL_MATRIX.forget();
            }
        }
        else if (need_recalc.matrix) {
            GLOBAL_MATRIX.forget();
        }

        error = di_calculate_matrix(aw_root, weighted_filter, bootstrap_flag, !bootstrap_flag, &aborted);
        if (error && aborted) {
            error = 0;          // clear error (otherwise no tree will be read below)
            break;              // end of bootstrap
        }

        if (!GLOBAL_MATRIX.exists()) {
            error = "unexpected error in di_calculate_matrix_cb (data missing)";
            break;
        }

        DI_MATRIX  *matr  = GLOBAL_MATRIX.get();
        char      **names = (char **)calloc(sizeof(char *), (size_t)matr->nentries+2);

        for (size_t i=0; i<matr->nentries; i++) {
            names[i] = matr->entries[i]->name;
        }
        di_assert(matr->nentries == matr->matrix->size());
        tree = neighbourjoining(names, *matr->matrix);

        if (bootstrap_flag) {
            error = ctree->insert_tree_weighted(tree, matr->nentries, 1, false);
            delete tree; tree = NULL;
            loop_count++;
            progress->inc();
            if (!bootstrap_count) { // when waiting for kill
                int        t     = time(0);
                static int tlast = 0;

                if (t>tlast) {
                    progress->force_update();
                    tlast = t;
                }
            }
        }
        free(names);
    } while (bootstrap_flag && loop_count != bootstrap_count);

    if (!error) {
        if (bootstrap_flag) {
            tree = ctree->get_consensus_tree(error);
            progress->inc();
            if (!error) {
                error = GBT_is_invalid(tree);
                di_assert(!error);
            }
        }

        if (!error) {
            char *tree_name = aw_root->awar(AWAR_DIST_TREE_STD_NAME)->read_string();
            GB_begin_transaction(GLOBAL_gb_main);
            error = GBT_write_tree(GLOBAL_gb_main, tree_name, tree);

            if (!error) {
                char *filter_name = AWT_get_combined_filter_name(aw_root, "dist");
                int   transr      = aw_root->awar(AWAR_DIST_CORR_TRANS)->read_int();

                const char *comment;
                if (enum_trans_to_string[transr]) {
                    comment = GBS_global_string("PRG=dnadist CORR=%s FILTER=%s PKG=ARB", enum_trans_to_string[transr], filter_name);
                }
                else {
                    di_assert(transr == DI_TRANSFORMATION_FROM_TREE);
                    const char *treename = aw_root->awar(AWAR_DIST_TREE_CURR_NAME)->read_char_pntr();
                    comment = GBS_global_string("PRG=treedist (from '%s') PKG=ARB", treename);
                }

                error = GBT_write_tree_remark(GLOBAL_gb_main, tree_name, comment);
                free(filter_name);
            }
            error = GB_end_transaction(GLOBAL_gb_main, error);
            free(tree_name);
        }
    }

    delete tree;

    // aw_status(); // remove 'abort' flag (@@@ got no equiv for arb_progress yet. really needed?)

    if (bootstrap_flag) {
        if (all_names) delete all_names;
        GLOBAL_MATRIX.forget();
    }
#if defined(DEBUG)
    else {
        di_assert(all_names == 0);
    }
#endif // DEBUG

    progress->done();
    if (error) {
        aw_message(error);
    }
    else {
        need_recalc.tree = false;
        aw_root->awar(AWAR_TREE_REFRESH)->touch();
    }
}


static void di_autodetect_callback(AW_window *aww) {
    GB_push_transaction(GLOBAL_gb_main);

    GLOBAL_MATRIX.forget();

    AW_root  *aw_root = aww->get_root();
    char     *use     = aw_root->awar(AWAR_DIST_ALIGNMENT)->read_string();
    long      ali_len = GBT_get_alignment_len(GLOBAL_gb_main, use);
    GB_ERROR  error   = NULL;

    if (ali_len<=0) {
        GB_pop_transaction(GLOBAL_gb_main);
        error = "Please select a valid alignment";
        GB_clear_error();
    }
    else {
        arb_progress progress("Analyzing data");

        char *filter_str = aw_root->awar(AWAR_DIST_FILTER_FILTER)->read_string();
        char *cancel     = aw_root->awar(AWAR_DIST_CANCEL_CHARS)->read_string();

        AliView *aliview = NULL;
        {
            AP_filter *ap_filter = NULL;
            long       flen  = strlen(filter_str);

            if (flen == ali_len) {
                ap_filter = new AP_filter(filter_str, "0", ali_len);
            }
            else {
                if (flen) {
                    aw_message("Warning: your filter len is not equal to the alignment len\nfilter got truncated with zeros or cutted");
                    ap_filter = new AP_filter(filter_str, "0", ali_len);
                }
                else {
                    ap_filter = new AP_filter(ali_len); // unfiltered
                }
            }

            error = ap_filter->is_invalid();
            if (!error) {
                AP_weights ap_weights(ap_filter);
                aliview = new AliView(GLOBAL_gb_main, *ap_filter, ap_weights, use);
            }
            delete ap_filter;
        }

        if (error) {
            GB_pop_transaction(GLOBAL_gb_main);
        }
        else {
            DI_MATRIX phm(*aliview, aw_root);

            {
                char *load_what      = aw_root->awar(AWAR_DIST_WHICH_SPECIES)->read_string();
                char *sort_tree_name = aw_root->awar(AWAR_DIST_TREE_SORT_NAME)->read_string();

                LoadWhat all_flag = (strcmp(load_what, "all") == 0) ? DI_LOAD_ALL : DI_LOAD_MARKED;

                GLOBAL_MATRIX.forget();

                MatrixOrder order(GLOBAL_gb_main, sort_tree_name);
                error = phm.load(all_flag, order, true, NULL);

                free(sort_tree_name);
                free(load_what);
            }

            GB_pop_transaction(GLOBAL_gb_main);

            if (!error) {
                progress.subtitle("Search Correction");
                phm.analyse();
            }
        }

        free(cancel);
        delete aliview;

        free(filter_str);
    }

    if (error) aw_message(error);

    free(use);
}

__ATTR__NORETURN static void di_exit(AW_window *aww) {
    if (GLOBAL_gb_main) {
        AW_root *aw_root = aww->get_root();
        shutdown_macro_recording(aw_root);
        aw_root->unlink_awars_from_DB(GLOBAL_gb_main);
        GB_close(GLOBAL_gb_main);
    }
    GLOBAL_MATRIX.set_changed_cb(NULL);
    exit(EXIT_SUCCESS);
}

static void di_calculate_full_matrix_cb(AW_window *aww, const WeightedFilter *weighted_filter) {
    recalculate_matrix_cb = new BoundWindowCallback(aww, makeWindowCallback(di_calculate_full_matrix_cb, weighted_filter));

    GLOBAL_MATRIX.forget_if_not_has_type(DI_MATRIX_FULL);
    GB_ERROR error = di_calculate_matrix(aww->get_root(), weighted_filter, 0, true, NULL);
    aw_message_if(error);
    last_matrix_calculation_error = error;
    if (!error) tree_needs_recalc_cb();
}

static void di_calculate_compressed_matrix_cb(AW_window *aww, WeightedFilter *weighted_filter) {
    recalculate_matrix_cb = new BoundWindowCallback(aww, makeWindowCallback(di_calculate_compressed_matrix_cb, weighted_filter));

    GB_transaction ta(GLOBAL_gb_main);

    AW_root  *aw_root  = aww->get_root();
    char     *treename = aw_root->awar(AWAR_DIST_TREE_COMP_NAME)->read_string();
    GB_ERROR  error    = 0;
    GBT_TREE  *tree    = GBT_read_tree(GLOBAL_gb_main, treename, GBT_TREE_NodeFactory());

    if (!tree) {
        error = GB_await_error();
    }
    else {
        {
            LocallyModify<MatrixDisplay*> skipRefresh(matrixDisplay, NULL); // skip refresh, until matrix has been compressed

            GLOBAL_MATRIX.forget(); // always forget (as tree might have changed)
            error = di_calculate_matrix(aw_root, weighted_filter, 0, true, NULL);
            if (!error && !GLOBAL_MATRIX.exists()) {
                error = "Failed to calculate your matrix (bug?)";
            }
            if (!error) {
                error = GLOBAL_MATRIX.get()->compress(tree);
            }
        }
        delete tree;

        // now force refresh
        if (matrixDisplay) {
            matrixDisplay->mark(MatrixDisplay::NEED_SETUP);
            matrixDisplay->update_display();
        }
    }
    free(treename);
    aw_message_if(error);
    last_matrix_calculation_error = error;
    if (!error) tree_needs_recalc_cb();
}

static void di_define_sort_tree_name_cb(AW_window *aww) {
    AW_root *aw_root = aww->get_root();
    char *tree_name = aw_root->awar(AWAR_DIST_TREE_CURR_NAME)->read_string();
    aw_root->awar(AWAR_DIST_TREE_SORT_NAME)->write_string(tree_name);
    free(tree_name);
}
static void di_define_compression_tree_name_cb(AW_window *aww) {
    AW_root *aw_root = aww->get_root();
    char *tree_name = aw_root->awar(AWAR_DIST_TREE_CURR_NAME)->read_string();
    aw_root->awar(AWAR_DIST_TREE_COMP_NAME)->write_string(tree_name);
    free(tree_name);
}

static void di_define_save_tree_name_cb(AW_window *aww) {
    AW_root *aw_root = aww->get_root();
    char *tree_name = aw_root->awar(AWAR_DIST_TREE_CURR_NAME)->read_string();
    aw_root->awar(AWAR_DIST_TREE_STD_NAME)->write_string(tree_name);
    free(tree_name);
}


AW_window *DI_create_matrix_window(AW_root *aw_root) {
    AW_window_simple_menu *aws = new AW_window_simple_menu;
    aws->init(aw_root, "NEIGHBOUR JOINING", "NEIGHBOUR JOINING [ARB_DIST]");
    aws->load_xfig("di_ge_ma.fig");
    aws->button_length(10);

    aws->at("close");
    aws->callback(di_exit);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("dist.hlp"));
    aws->create_button("HELP", "HELP", "H");


    GB_push_transaction(GLOBAL_gb_main);

#if defined(DEBUG)
    AWT_create_debug_menu(aws);
#endif // DEBUG

    aws->create_menu("File", "F", AWM_ALL);
    insert_macro_menu_entry(aws, false);
    aws->insert_menu_topic("quit", "Quit", "Q", "quit.hlp", AWM_ALL, di_exit);

    aws->create_menu("Properties", "P", AWM_ALL);
    aws->insert_menu_topic("frame_props", "Frame ...", "F", "props_frame.hlp", AWM_ALL, AW_preset_window);
    aws->sep______________();
    AW_insert_common_property_menu_entries(aws);
    aws->sep______________();
    aws->insert_menu_topic("save_props", "Save Properties (dist.arb)", "S", "savedef.hlp", AWM_ALL, AW_save_properties);

    aws->insert_help_topic("ARB_DIST help", "D", "dist.hlp", AWM_ALL, makeHelpCallback("dist.hlp"));

    // ------------------
    //      left side

    aws->at("which_species");
    aws->create_option_menu(AWAR_DIST_WHICH_SPECIES, true);
    aws->insert_option("all", "a", "all");
    aws->insert_default_option("marked",   "m", "marked");
    aws->update_option_menu();

    aws->at("which_alignment");
    awt_create_selection_list_on_alignments(GLOBAL_gb_main, (AW_window *)aws, AWAR_DIST_ALIGNMENT, "*=");

    // filter & weights

    AW_awar *awar_dist_alignment    = aws->get_root()->awar_string(AWAR_DIST_ALIGNMENT);
    WeightedFilter *weighted_filter = // do NOT free (bound to callbacks)
        new WeightedFilter(GLOBAL_gb_main, aws->get_root(), AWAR_DIST_FILTER_NAME, AWAR_DIST_COLUMN_STAT_NAME, awar_dist_alignment);

    aws->at("filter_select");
    aws->callback(makeCreateWindowCallback(awt_create_select_filter_win, weighted_filter->get_adfiltercbstruct()));
    aws->create_button("SELECT_FILTER", AWAR_DIST_FILTER_NAME);

    aws->at("weights_select");
    aws->sens_mask(AWM_EXP);
    aws->callback(makeCreateWindowCallback(COLSTAT_create_selection_window, weighted_filter->get_column_stat()));
    aws->create_button("SELECT_COL_STAT", AWAR_DIST_COLUMN_STAT_NAME);
    aws->sens_mask(AWM_ALL);

    aws->at("which_cancel");
    aws->create_input_field(AWAR_DIST_CANCEL_CHARS, 12);

    aws->at("cancel_select");
    aws->callback(awt_create_select_cancel_window);
    aws->create_button("SELECT_CANCEL_CHARS", "Info", "C");

    aws->at("change_matrix");
    aws->callback(create_dna_matrix_window);
    aws->create_button("EDIT_MATRIX", "Edit Matrix");

    aws->at("enable");
    aws->create_toggle(AWAR_DIST_MATRIX_DNA_ENABLED);

    aws->at("which_correction");
    aws->create_option_menu(AWAR_DIST_CORR_TRANS, true);
    aws->insert_option("none",                    "n", (int)DI_TRANSFORMATION_NONE);
    aws->insert_option("similarity",              "n", (int)DI_TRANSFORMATION_SIMILARITY);
    aws->insert_option("jukes-cantor (dna)",      "c", (int)DI_TRANSFORMATION_JUKES_CANTOR);
    aws->insert_option("felsenstein (dna)",       "f", (int)DI_TRANSFORMATION_FELSENSTEIN);
    aws->insert_option("olsen (dna)",             "o", (int)DI_TRANSFORMATION_OLSEN);
    aws->insert_option("felsenstein/voigt (exp)", "1", (int)DI_TRANSFORMATION_FELSENSTEIN_VOIGT);
    aws->insert_option("olsen/voigt (exp)",       "2", (int)DI_TRANSFORMATION_OLSEN_VOIGT);
    aws->insert_option("haesch (exp)",            "f", (int)DI_TRANSFORMATION_HAESCH);
    aws->insert_option("kimura (pro)",            "k", (int)DI_TRANSFORMATION_KIMURA);
    aws->insert_option("PAM (protein)",           "c", (int)DI_TRANSFORMATION_PAM);
    aws->insert_option("Cat. Hall(exp)",          "c", (int)DI_TRANSFORMATION_CATEGORIES_HALL);
    aws->insert_option("Cat. Barker(exp)",        "c", (int)DI_TRANSFORMATION_CATEGORIES_BARKER);
    aws->insert_option("Cat.Chem (exp)",          "c", (int)DI_TRANSFORMATION_CATEGORIES_CHEMICAL);
    aws->insert_option("from selected tree",      "t", (int)DI_TRANSFORMATION_FROM_TREE);
    aws->insert_default_option("unknown",         "u", (int)DI_TRANSFORMATION_NONE);

    aws->update_option_menu();

    aws->at("autodetect");   // auto
    aws->callback(di_autodetect_callback);
    aws->sens_mask(AWM_EXP);
    aws->create_button("AUTODETECT_CORRECTION", "AUTODETECT", "A");
    aws->sens_mask(AWM_ALL);

    // -------------------
    //      right side


    aws->at("mark_distance");
    aws->callback(makeWindowCallback(di_mark_by_distance, weighted_filter));
    aws->create_autosize_button("MARK_BY_DIST", "Mark all species");

    aws->at("mark_lower");
    aws->create_input_field(AWAR_DIST_MIN_DIST, 5);

    aws->at("mark_upper");
    aws->create_input_field(AWAR_DIST_MAX_DIST, 5);

    // -----------------

    // tree selection

    aws->at("tree_list");
    awt_create_selection_list_on_trees(GLOBAL_gb_main, aws, AWAR_DIST_TREE_CURR_NAME, true);

    aws->at("detect_clusters");
    aws->callback(makeCreateWindowCallback(DI_create_cluster_detection_window, weighted_filter));
    aws->create_autosize_button("DETECT_CLUSTERS", "Detect homogenous clusters in tree", "D");

    // matrix calculation

    aws->button_length(18);

    aws->at("calculate");
    aws->callback(makeWindowCallback(di_calculate_full_matrix_cb, weighted_filter));
    aws->create_button("CALC_FULL_MATRIX", "Calculate\nFull Matrix", "F");

    aws->at("compress");
    aws->callback(makeWindowCallback(di_calculate_compressed_matrix_cb, weighted_filter));
    aws->create_button("CALC_COMPRESSED_MATRIX", "Calculate\nCompressed Matrix", "C");

    recalculate_matrix_cb = new BoundWindowCallback(aws, makeWindowCallback(di_calculate_full_matrix_cb, weighted_filter));

    aws->button_length(13);

    {
        save_matrix_params *sparams = new save_matrix_params; // do not free (bound to callbacks)

        sparams->awar_base       = AWAR_DIST_SAVE_MATRIX_BASE;
        sparams->weighted_filter = weighted_filter;

        aws->at("save_matrix");
        aws->callback(makeCreateWindowCallback(DI_create_save_matrix_window, sparams));
        aws->create_button("SAVE_MATRIX", "Save matrix", "M");

        aws->at("view_matrix");
        aws->callback(makeWindowCallback(di_view_matrix_cb, sparams));
        aws->create_button("VIEW_MATRIX", "View matrix", "V");
    }

    aws->button_length(22);
    aws->at("use_compr_tree");
    aws->callback(di_define_compression_tree_name_cb);
    aws->create_button("USE_COMPRESSION_TREE", "Use to compress", "");
    aws->at("use_sort_tree");
    aws->callback(di_define_sort_tree_name_cb);
    aws->create_button("USE_SORT_TREE", "Use to sort", "");

    aws->at("compr_tree_name"); aws->create_input_field(AWAR_DIST_TREE_COMP_NAME, 12);
    aws->at("sort_tree_name");  aws->create_input_field(AWAR_DIST_TREE_SORT_NAME, 12);

    // tree calculation

    aws->button_length(18);

    aws->at("t_calculate");
    aws->callback(makeWindowCallback(di_calculate_tree_cb, weighted_filter, false));
    aws->create_button("CALC_TREE", "Calculate \ntree", "C");

    aws->at("bootstrap");
    aws->callback(makeWindowCallback(di_calculate_tree_cb, weighted_filter, true));
    aws->create_button("CALC_BOOTSTRAP_TREE", "Calculate \nbootstrap tree");

    recalculate_tree_cb = new BoundWindowCallback(aws, makeWindowCallback(di_calculate_tree_cb, weighted_filter, false));

    aws->button_length(22);
    aws->at("use_existing");
    aws->callback(di_define_save_tree_name_cb);
    aws->create_button("USE_NAME", "Use as new tree name", "");

    aws->at("calc_tree_name");
    aws->create_input_field(AWAR_DIST_TREE_STD_NAME, 12);

    aws->at("bcount");
    aws->create_input_field(AWAR_DIST_BOOTSTRAP_COUNT, 7);

    {
        aws->sens_mask(AWM_EXP);

        aws->at("auto_calc_tree");
        aws->label("Auto calculate tree");
        aws->create_toggle(AWAR_DIST_MATRIX_AUTO_CALC_TREE);

        aws->at("auto_recalc");
        aws->label("Auto recalculate");
        aws->create_toggle(AWAR_DIST_MATRIX_AUTO_RECALC);

        aws->sens_mask(AWM_ALL);
    }

    bool disable_autocalc = !ARB_in_expert_mode(aw_root);
    if (disable_autocalc) {
        aw_root->awar(AWAR_DIST_MATRIX_AUTO_RECALC)->write_int(0);
        aw_root->awar(AWAR_DIST_MATRIX_AUTO_CALC_TREE)->write_int(0);
    }

    GB_pop_transaction(GLOBAL_gb_main);
    return aws;
}

