// =============================================================== //
//                                                                 //
//   File      : ST_ml.cxx                                         //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "st_ml.hxx"
#include "MostLikelySeq.hxx"

#include <ColumnStat.hxx>
#include <AP_filter.hxx>
#include <AP_Tree.hxx>

#include <cctype>
#include <cmath>
#include <algorithm>

AWT_dna_table awt_dna_table;

AWT_dna_table::AWT_dna_table() {
    int i;
    for (i = 0; i < 256; i++) {
        switch (toupper(i)) {
            case 'A':
                char_to_enum_table[i] = ST_A;
                break;
            case 'C':
                char_to_enum_table[i] = ST_C;
                break;
            case 'G':
                char_to_enum_table[i] = ST_G;
                break;
            case 'T':
            case 'U':
                char_to_enum_table[i] = ST_T;
                break;
            case '-':
                char_to_enum_table[i] = ST_GAP;
                break;
            default:
                char_to_enum_table[i] = ST_UNKNOWN;
        }
    }
}

// -----------------------
//      ST_base_vector

void ST_base_vector::setBase(const ST_base_vector& inv_frequencies, char base) {
    base = toupper(base);
    memset((char *) &b[0], 0, sizeof(b));
    const double k = 1.0 / ST_MAX_BASE;
    AWT_dna_base ub = awt_dna_table.char_to_enum(base);
    if (ub != ST_UNKNOWN) {
        b[ub] = 1.0;
    }
    else {
        b[ST_A] = k;
        b[ST_C] = k;
        b[ST_G] = k;
        b[ST_T] = k;
        b[ST_GAP] = k;
    }
    int i;
    for (i = 0; i < ST_MAX_BASE; i++) {
        b[i] *= inv_frequencies.b[i];
    }
    ld_lik = 0;
    lik = 1.0;
}

inline void ST_base_vector::check_overflow()
{
    double sum = 0.0;
    int    i;

    for (i = 0; i < ST_MAX_BASE; i++) {
        sum += b[i];
    }
    if (sum < .00001) {                             // what happend no data, extremely unlikely
        for (i = 0; i < ST_MAX_BASE; i++) {
            b[i] = .25;
        }
        ld_lik -= 5;                                // ???
    }
    else {
        while (sum < 0.25) {
            sum *= 4;
            ld_lik -= 2;
            for (i = 0; i < ST_MAX_BASE; i++) {
                b[i] *= 4;
            }
        }
    }
    if (ld_lik> 10000) {
        printf("overflow\n");
    }
}

inline void ST_base_vector::mult(ST_base_vector *other) {
    const ST_FLOAT *a = &b[0];
    const ST_FLOAT *c = &other->b[0];

    ST_FLOAT a0 = a[0] * c[0];
    ST_FLOAT a1 = a[1] * c[1];
    ST_FLOAT a2 = a[2] * c[2];
    ST_FLOAT a3 = a[3] * c[3];
    ST_FLOAT a4 = a[4] * c[4];

    b[0] = a0;
    b[1] = a1;
    b[2] = a2;
    b[3] = a3;
    b[4] = a4;
    
    ld_lik += other->ld_lik;
    lik    *= other->lik;
}

void ST_base_vector::print() {
    int i;
    for (i = 0; i < ST_MAX_BASE; i++) {
        printf("%.3G ", b[i]);
    }
}


// -----------------------
//      ST_rate_matrix

void ST_rate_matrix::set(double dist, double /* TT_ratio */) {
    int i, j;
    double k = 1.0 / ST_MAX_BASE;
    for (i = 0; i < ST_MAX_BASE; i++) {
        m[i][i] = k + (1.0 - k) * exp(-dist);
        for (j = 0; j < i; j++) {
            m[j][i] = m[i][j] = k - k * exp(-dist);
        }
    }
}

void ST_rate_matrix::print() {
    int i, j;
    for (i = 0; i < ST_MAX_BASE; i++) {
        for (j = 0; j < ST_MAX_BASE; j++) {
            printf("%.3G ", m[i][j]);
        }
        printf("\n");
    }
}

inline void ST_rate_matrix::mult(const ST_base_vector *in, ST_base_vector *out) const {
    const ST_FLOAT *pm = &m[0][0];

    for (int i = ST_MAX_BASE - 1; i >= 0; i--) {        // calc revers
        ST_FLOAT sum0 = pm[4] * in->b[0];
        ST_FLOAT sum1 = pm[3] * in->b[1];
        ST_FLOAT sum2 = pm[2] * in->b[2];
        ST_FLOAT sum3 = pm[1] * in->b[3];
        ST_FLOAT sum4 = pm[0] * in->b[4];

        pm        += ST_MAX_BASE;
        out->b[i]  = (sum0 + sum1) + sum4 + (sum2 + sum3);
    }
    
    out->ld_lik = in->ld_lik;
    out->lik    = in->lik;
}

// -----------------------
//      MostLikelySeq

MostLikelySeq::MostLikelySeq(const AliView *aliview, ST_ML *st_ml_)
    : AP_sequence(aliview)
    , st_ml(st_ml_)
    , sequence(new ST_base_vector[ST_MAX_SEQ_PART])
    , up_to_date(false)
    , color_out(NULL)
    , color_out_valid_till(NULL)
{
}

MostLikelySeq::~MostLikelySeq() {
    delete [] sequence;
    delete color_out;
    delete color_out_valid_till;

    unbind_from_species(true);
}

static void st_sequence_callback(GBDATA *, int *cl, GB_CB_TYPE) {
    MostLikelySeq *seq = (MostLikelySeq *) cl;
    seq->sequence_change();
}

static void st_sequence_del_callback(GBDATA *, int *cl, GB_CB_TYPE) {
    MostLikelySeq *seq = (MostLikelySeq *) cl;
    seq->unbind_from_species(false);
}


GB_ERROR MostLikelySeq::bind_to_species(GBDATA *gb_species) {
    GB_ERROR error = AP_sequence::bind_to_species(gb_species);
    if (!error) {
        GBDATA *gb_seq = get_bound_species_data();
        st_assert(gb_seq);

        error             = GB_add_callback(gb_seq, GB_CB_CHANGED, st_sequence_callback, (int *) this);
        if (!error) error = GB_add_callback(gb_seq, GB_CB_DELETE, st_sequence_del_callback, (int *) this);
    }
    return error;
}
void MostLikelySeq::unbind_from_species(bool remove_callbacks) {
    GBDATA *gb_seq = get_bound_species_data();

    if (gb_seq) {  
        if (remove_callbacks) {
            GB_remove_callback(gb_seq, GB_CB_CHANGED, st_sequence_callback, (int *) this);
            GB_remove_callback(gb_seq, GB_CB_DELETE, st_sequence_del_callback, (int *) this);
        }
        AP_sequence::unbind_from_species();
    }
}

void MostLikelySeq::sequence_change() {
    st_ml->clear_all();
}

AP_sequence *MostLikelySeq::dup() const {
    return new MostLikelySeq(get_aliview(), st_ml);
}

void MostLikelySeq::set(const char *) {
    st_assert(0);                                   // hmm why not perform set_sequence() here ?
}

void MostLikelySeq::unset() {
}

void MostLikelySeq::set_sequence() {
    /*! Transform the sequence from character to vector
     * for current range [ST_ML::first_pos .. ST_ML::last_pos]
     */

    GBDATA *gb_data = get_bound_species_data();
    st_assert(gb_data);

    size_t                source_sequence_len = (size_t)GB_read_string_count(gb_data);
    const char           *source_sequence     = GB_read_char_pntr(gb_data) + st_ml->get_first_pos();
    ST_base_vector       *dest                = sequence;
    const ST_base_vector *freq                = st_ml->get_inv_base_frequencies() + st_ml->get_first_pos();

    size_t range_len = st_ml->get_last_pos() - st_ml->get_first_pos();
    size_t data_len  = std::min(range_len, source_sequence_len);
    size_t pos       = 0;

    for (; pos<data_len;  ++pos) dest[pos].setBase(freq[pos], toupper(source_sequence[pos]));
    for (; pos<range_len; ++pos) dest[pos].setBase(freq[pos], '.');

    up_to_date = true;
}

AP_FLOAT MostLikelySeq::combine(const AP_sequence *, const AP_sequence *, char *) {
    st_assert(0);
    return -1.0;
}

void MostLikelySeq::partial_match(const AP_sequence *, long *, long *) const {
    st_assert(0);                                   // function is expected to be unused
}

void MostLikelySeq::calculate_ancestor(const MostLikelySeq *lefts, double leftl, const MostLikelySeq *rights, double rightl) {
    st_assert(!up_to_date);

    ST_base_vector        hbv;
    double                lc   = leftl / st_ml->get_step_size();
    double                rc   = rightl / st_ml->get_step_size();
    const ST_base_vector *lb   = lefts->sequence;
    const ST_base_vector *rb   = rights->sequence;
    ST_base_vector       *dest = sequence;

    for (size_t pos = st_ml->get_first_pos(); pos < st_ml->get_last_pos(); pos++) {
        st_assert(lb->lik == 1 && rb->lik == 1);

        int distl = (int) (st_ml->get_rate_at(pos) * lc);
        int distr = (int) (st_ml->get_rate_at(pos) * rc);

        st_ml->get_matrix_for(distl).mult(lb, dest);
        st_ml->get_matrix_for(distr).mult(rb, &hbv);
        
        dest->mult(&hbv);
        dest->check_overflow();

        st_assert(dest->lik == 1);

        dest++;
        lb++;
        rb++;
    }

    up_to_date = true;
}

ST_base_vector *MostLikelySeq::tmp_out = 0;

void MostLikelySeq::calc_out(const MostLikelySeq *next_branch, double dist) {
    // result will be in tmp_out

    ST_base_vector *out   = tmp_out + st_ml->get_first_pos();
    double          lc    = dist / st_ml->get_step_size();
    ST_base_vector *lefts = next_branch->sequence;

    for (size_t pos = st_ml->get_first_pos(); pos < st_ml->get_last_pos(); pos++) {
        int distl = (int) (st_ml->get_rate_at(pos) * lc);
        st_ml->get_matrix_for(distl).mult(lefts, out);

        // correct frequencies
        for (int i = ST_A; i < ST_MAX_BASE; i++) {
            out->b[i] *= st_ml->get_base_frequency_at(pos).b[i];
        }
        lefts++;
        out++;
    }
}

void MostLikelySeq::print() {
    const char *data = GB_read_char_pntr(get_bound_species_data());
    for (size_t i = 0; i < ST_MAX_SEQ_PART; i++) {
        printf("POS %3zu  %c     ", i, data[i]);
        printf("\n");
    }
}

AP_FLOAT MostLikelySeq::count_weighted_bases() const {
    st_assert(0);
    return -1.0;
}

// --------------
//      ST_ML

ST_ML::ST_ML(GBDATA *gb_maini) {
    memset((char *) this, 0, sizeof(*this));
    gb_main = gb_maini;
}

ST_ML::~ST_ML() {
    delete tree_root;
    free(alignment_name);
    if (hash_2_ap_tree) GBS_free_hash(hash_2_ap_tree);
    delete not_valid;
    delete [] base_frequencies;
    delete [] inv_base_frequencies;
    delete [] rate_matrices;
    if (!column_stat) {
        // rates and ttratio have been allocated (see ST_ML::init_st_ml)
        delete [] rates;
        delete [] ttratio;
    }
}

void ST_ML::print() {
}

void ST_ML::create_frequencies() {
    //! Translate characters to base frequencies

    base_frequencies     = new ST_base_vector[alignment_len];
    inv_base_frequencies = new ST_base_vector[alignment_len];

    if (!column_stat) {
        for (size_t i = 0; i < alignment_len; i++) {
            for (int j = ST_A; j < ST_MAX_BASE; j++) {
                base_frequencies[i].b[j]     = 1.0;
                inv_base_frequencies[i].b[j] = 1.0;
            }
            base_frequencies[i].lik     = 1.0;
            inv_base_frequencies[i].lik = 1.0;
        }
    }
    else {
        for (size_t i = 0; i < alignment_len; i++) {
            for (int j = ST_A; j < ST_MAX_BASE; j++) {
                base_frequencies[i].b[j] = 0.01;    // minimal frequency
            }

            static struct {
                unsigned char c;
                AWT_dna_base  b;
            } toCount[] = {
                { 'A', ST_A }, { 'a', ST_A },
                { 'C', ST_C }, { 'c', ST_C },
                { 'G', ST_G }, { 'g', ST_G },
                { 'T', ST_T }, { 't', ST_T },
                { 'U', ST_T }, { 'u', ST_T },
                { '-', ST_GAP },
                { 0, ST_UNKNOWN },
            };

            for (int j = 0; toCount[j].c; ++j) {
                const float *freq = column_stat->get_frequencies(toCount[j].c);
                if (freq) base_frequencies[i].b[toCount[j].b] += freq[i];
            }

            double sum = 0.0;

            for (int j = ST_A; j < ST_MAX_BASE; j++) sum                      += base_frequencies[i].b[j];
            for (int j = ST_A; j < ST_MAX_BASE; j++) base_frequencies[i].b[j] += sum * .01; // smoothen frequencies to avoid crazy values

            double min = base_frequencies[i].b[ST_A];
            sum        = 0.0;
            for (int j = ST_A; j < ST_MAX_BASE; j++) {
                sum += base_frequencies[i].b[j];
                if (base_frequencies[i].b[j] < min) min = base_frequencies[i].b[j];
            }
            for (int j = ST_A; j < ST_MAX_BASE; j++) {
                if (sum > 0.01) {                   // valid column ??
                    base_frequencies[i].b[j] *= ST_MAX_BASE / sum;
                }
                else {
                    base_frequencies[i].b[j] = 1.0; // no
                }
                inv_base_frequencies[i].b[j] = min / base_frequencies[i].b[j];
            }
            base_frequencies[i].lik     = 1.0;
            inv_base_frequencies[i].lik = 1.0;
        }
    }
}

void ST_ML::insert_tree_into_hash_rek(AP_tree *node) {
    node->gr.gc = 0;
    if (node->is_leaf) {
        GBS_write_hash(hash_2_ap_tree, node->name, (long) node);
    }
    else {
        insert_tree_into_hash_rek(node->get_leftson());
        insert_tree_into_hash_rek(node->get_rightson());
    }
}

void ST_ML::create_matrices(double max_disti, int nmatrices) {
    delete [] rate_matrices;
    rate_matrices = new ST_rate_matrix[nmatrices];

    max_dist          = max_disti;
    max_rate_matrices = nmatrices;
    step_size         = max_dist / max_rate_matrices;

    for (int i = 0; i < max_rate_matrices; i++) {
        rate_matrices[i].set((i + 1) * step_size, 0); // ttratio[i]
    }
}

long ST_ML::delete_species(const char *key, long val, void *cd_st_ml) {
    ST_ML *st_ml = (ST_ML*)cd_st_ml;

    if (GBS_read_hash(st_ml->keep_species_hash, key)) {
        return val;
    }
    else {
        AP_tree *leaf   = (AP_tree *) val;
        AP_tree *father = leaf->get_father();
        leaf->remove();
        delete father;                              // also deletes 'this'

        return 0;
    }
}

inline GB_ERROR tree_size_ok(AP_tree_root *tree_root) {
    GB_ERROR error = NULL;

    AP_tree *root = tree_root->get_root_node();
    if (!root || root->is_leaf) {
        const char *tree_name = tree_root->get_tree_name();
        error = GBS_global_string("Too few species remained in tree '%s'", tree_name);
    }
    return error;
}

GB_ERROR ST_ML::init_st_ml(const char *tree_name, const char *alignment_namei,
                           const char *species_names, int marked_only,
                           ColumnStat *colstat, bool show_status)
{
    /*! this is the real constructor, call only once */

    GB_ERROR error = 0;

    if (is_initialized) {
        error = "Column statistic can't be re-initialized";
    }
    else {
        GB_transaction ta(gb_main);

        if (show_status) aw_openstatus("Activating column statistic");

        column_stat                = colstat;
        GB_ERROR column_stat_error = column_stat->calculate(NULL);

        if (column_stat_error) fprintf(stderr, "Column statistic error: %s (using equal rates/tt-ratio for all columns)\n", column_stat_error);

        alignment_name = strdup(alignment_namei);
        long ali_len  = GBT_get_alignment_len(gb_main, alignment_name);

        if (ali_len<0) {
            error = GB_await_error();
        }
        else if (ali_len<10) {
            error = "alignment too short";
        }
        else {
            alignment_len = ali_len;
            {
                AP_filter      filter(alignment_len); // unfiltered
                AP_weights     weights(&filter);
                AliView       *aliview   = new AliView(gb_main, filter, weights, alignment_name);
                MostLikelySeq *seq_templ = new MostLikelySeq(aliview, this); // @@@ error: never freed! (should be freed when freeing tree_root!)

                tree_root = new AP_tree_root(aliview, AP_tree(0), seq_templ, false);
                // do not delete 'aliview' or 'seq_templ' (they belong to 'tree_root' now)
            }

            if (show_status) aw_status("load tree");
            tree_root->loadFromDB(tree_name);       // tree is not linked!

            if (show_status) aw_status("link tree");
            {
                size_t species_in_tree = count_species_in_tree();
                hash_2_ap_tree         = GBS_create_hash(species_in_tree, GB_MIND_CASE);
            }

            // delete species from tree:
            if (species_names) {                    // keep names
                tree_root->remove_leafs(AWT_REMOVE_DELETED);

                error = tree_size_ok(tree_root);
                if (!error) {
                    char *l, *n;
                    keep_species_hash = GBS_create_hash(GBT_get_species_count(gb_main), GB_MIND_CASE);
                    for (l = (char *) species_names; l; l = n) {
                        n = strchr(l, 1);
                        if (n) *n = 0;
                        GBS_write_hash(keep_species_hash, l, 1);
                        if (n) *(n++) = 1;
                    }

                    insert_tree_into_hash_rek(tree_root->get_root_node());
                    GBS_hash_do_loop(hash_2_ap_tree, delete_species, this);
                    GBS_free_hash(keep_species_hash);
                    keep_species_hash = 0;
                    GBT_link_tree(tree_root->get_root_node()->get_gbt_tree(), gb_main, show_status, 0, 0);
                }
            }
            else {                                  // keep marked/all
                GBT_link_tree(tree_root->get_root_node()->get_gbt_tree(), gb_main, show_status, 0, 0);
                aw_status("clean tree");
                tree_root->remove_leafs((marked_only ? AWT_REMOVE_NOT_MARKED : 0)|AWT_REMOVE_DELETED);

                error = tree_size_ok(tree_root);
                if (!error) insert_tree_into_hash_rek(tree_root->get_root_node());
            }

            if (!error) {
                // calc frequencies

                if (show_status) aw_status("calculating frequencies");

                if (!column_stat_error) {
                    rates   = column_stat->get_rates();
                    ttratio = column_stat->get_ttratio();
                }
                else {
                    float *alloc_rates   = new float[alignment_len];
                    float *alloc_ttratio = new float[alignment_len];

                    for (size_t i = 0; i < alignment_len; i++) {
                        alloc_rates[i]   = 1.0;
                        alloc_ttratio[i] = 2.0;
                    }
                    rates   = alloc_rates;
                    ttratio = alloc_ttratio;

                    column_stat = 0;                // mark rates and ttratio as "allocated" (see ST_ML::~ST_ML)
                }
                create_frequencies();
                latest_modification = GB_read_clock(gb_main); // set update time
                create_matrices(2.0, 1000);

                MostLikelySeq::tmp_out = new ST_base_vector[alignment_len];  // @@@ error: never freed!
                is_initialized = true;
            }
            
            if (error) {
                delete tree_root;               tree_root      = NULL;
                GBS_free_hash(hash_2_ap_tree);  hash_2_ap_tree = NULL;
            }
        }

        if (error) {
            free(alignment_name);
            error = ta.close(error);
        }

        if (show_status) aw_closestatus();
    }
    return error;
}

MostLikelySeq *ST_ML::getOrCreate_seq(AP_tree *node) {
    MostLikelySeq *seq = DOWNCAST(MostLikelySeq*, node->get_seq());
    if (!seq) {
        seq = new MostLikelySeq(tree_root->get_aliview(), this); // @@@ why not use dup() ?

        node->set_seq(seq);
        if (node->is_leaf) {
            st_assert(node->gb_node);
            seq->bind_to_species(node->gb_node);
        }
    }
    return seq;
}

const MostLikelySeq *ST_ML::get_mostlikely_sequence(AP_tree *node) {
    /*! go through the tree and calculate the ST_base_vector from bottom to top
     */

    MostLikelySeq *seq = getOrCreate_seq(node);
    if (!seq->is_up_to_date()) {
        if (node->is_leaf) {
            seq->set_sequence();
        }
        else {
            const MostLikelySeq *leftSeq  = get_mostlikely_sequence(node->get_leftson());
            const MostLikelySeq *rightSeq = get_mostlikely_sequence(node->get_rightson());

            seq->calculate_ancestor(leftSeq, node->leftlen, rightSeq, node->rightlen);
        }
    }

    return seq;
}

void ST_ML::clear_all() {
    GB_transaction ta(gb_main);
    undo_tree(tree_root->get_root_node());
    latest_modification = GB_read_clock(gb_main);
}

void ST_ML::undo_tree(AP_tree *node) {
    MostLikelySeq *seq = getOrCreate_seq(node);
    seq->forget_sequence();
    if (!node->is_leaf) {
        undo_tree(node->get_leftson());
        undo_tree(node->get_rightson());
    }
}

#define GET_ML_VECTORS_BUG_WORKAROUND_INCREMENT (ST_MAX_SEQ_PART-1) // workaround bug in get_ml_vectors

MostLikelySeq *ST_ML::get_ml_vectors(const char *species_name, AP_tree *node, size_t start_ali_pos, size_t end_ali_pos) {
    /* result will be in tmp_out
     *
     * assert end_ali_pos - start_ali_pos < ST_MAX_SEQ_PART
     *
     * @@@ CAUTION!!! get_ml_vectors has a bug:
     * it does not calculate the last value, if (end_ali_pos-start_ali_pos+1)==ST_MAX_SEQ_PART
     * (search for GET_ML_VECTORS_BUG_WORKAROUND_INCREMENT)
     *
     * I'm not sure whether this is really a bug! Maybe it's only some misunderstanding about
     * 'end_ali_pos', because it does not mark the last calculated position, but the position
     * behind the last calculated position! @@@ Need to rename it!
     * 
     */

    if (!node) {
        if (!hash_2_ap_tree) return 0;
        node = (AP_tree *) GBS_read_hash(hash_2_ap_tree, species_name);
        if (!node) return 0;
    }

    st_assert(start_ali_pos<end_ali_pos);
    st_assert((end_ali_pos - start_ali_pos + 1) <= ST_MAX_SEQ_PART);

    MostLikelySeq *seq = getOrCreate_seq(node);

    if (start_ali_pos != first_pos || end_ali_pos > last_pos) {
        undo_tree(tree_root->get_root_node());      // undo everything
        first_pos = start_ali_pos;
        last_pos  = end_ali_pos;
    }

    AP_tree *pntr;
    for (pntr = node->get_father(); pntr; pntr = pntr->get_father()) {
        MostLikelySeq *sequ = getOrCreate_seq(pntr);
        if (sequ) sequ->forget_sequence();
    }

    node->set_root();

    const MostLikelySeq *seq_of_brother = get_mostlikely_sequence(node->get_brother());

    seq->calc_out(seq_of_brother, node->father->leftlen + node->father->rightlen);
    return seq;
}

bool ST_ML::update_ml_likelihood(char *result[4], int& latest_update, const char *species_name, AP_tree *node) {
    /*! calculates values for 'Detailed column statistics' in ARB_EDIT4
     * @return true if calculated with sucess
     *
     * @param result if result[0] is NULL, memory will be allocated and assigned to result[0 .. 3].
     *        You should NOT allocate result yourself, but you can reuse it for multiple calls.
     * @param latest_update has to contain and will be set to the latest statistic modification time
     *        (0 is a good start value)
     */

    st_assert(!!species_name != !!node);            // need node xor species_name

    if (latest_update < latest_modification) {
        if (!node) {                                // if node isn't given search it using species name
            st_assert(hash_2_ap_tree);              // ST_ML was not prepared for search-by-name
            if (hash_2_ap_tree) node = (AP_tree *) GBS_read_hash(hash_2_ap_tree, species_name);
            if (!node) return false;
        }

        AWT_dna_base adb[4];
        int i;

        if (!result[0]) {                           // allocate Array-elements for result
            for (i = 0; i < 4; i++) {
                result[i] = (char *) GB_calloc(1, alignment_len + 1); // [0 .. alignment_len[ + zerobyte 
            }
        }

        for (i = 0; i < 4; i++) {
            adb[i] = awt_dna_table.char_to_enum("ACGU"[i]);
        }

        for (size_t seq_start = 0; seq_start < alignment_len; seq_start += GET_ML_VECTORS_BUG_WORKAROUND_INCREMENT) {
            size_t seq_end = std::min(alignment_len, seq_start+GET_ML_VECTORS_BUG_WORKAROUND_INCREMENT);
            get_ml_vectors(0, node, seq_start, seq_end);
        }

        MostLikelySeq *seq = getOrCreate_seq(node);

        for (size_t pos = 0; pos < alignment_len; pos++) {
            ST_base_vector & vec = seq->tmp_out[pos];
            double           sum = vec.b[ST_A] + vec.b[ST_C] + vec.b[ST_G] + vec.b[ST_T] + vec.b[ST_GAP];

            if (sum == 0) {
                for (i = 0; i < 4; i++) {
                    result[i][pos] = -1;
                }
            }
            else {
                double div = 100.0 / sum;

                for (i = 0; i < 4; i++) {
                    result[i][pos] = char ((vec.b[adb[i]] * div) + 0.5);
                }
            }
        }

        latest_update = latest_modification;
    }
    return true;
}

ST_ML_Color *ST_ML::get_color_string(const char *species_name, AP_tree *node, size_t start_ali_pos, size_t end_ali_pos) {
    /*! (Re-)Calculates the color string of a given node for sequence positions [start_ali_pos .. end_ali_pos[
     */
    
    if (!node) {
        // if node isn't given, search it using species name:
        if (!hash_2_ap_tree) return 0;
        node = (AP_tree *) GBS_read_hash(hash_2_ap_tree, species_name);
        if (!node) return 0;
    }

    // align start_ali_pos/end_ali_pos to previous/next pos divisible by ST_BUCKET_SIZE:
    start_ali_pos &= ~(ST_BUCKET_SIZE - 1);
    end_ali_pos    = (end_ali_pos & ~(ST_BUCKET_SIZE - 1)) + ST_BUCKET_SIZE - 1;

    if (end_ali_pos > alignment_len) end_ali_pos = alignment_len;

    double         val;
    MostLikelySeq *seq = getOrCreate_seq(node);
    size_t         pos;

    if (!seq->color_out) {                          // allocate mem for color_out if we not already have it
        seq->color_out = (ST_ML_Color *) GB_calloc(sizeof(ST_ML_Color), alignment_len);
        seq->color_out_valid_till = (int *) GB_calloc(sizeof(int), (alignment_len >> LD_BUCKET_SIZE) + ST_BUCKET_SIZE);
    }
    // search for first out-dated position:
    for (pos = start_ali_pos; pos <= end_ali_pos; pos += ST_BUCKET_SIZE) {
        if (seq->color_out_valid_till[pos >> LD_BUCKET_SIZE] < latest_modification) break;
    }
    if (pos > end_ali_pos) {                        // all positions are up-to-date
        return seq->color_out;                      // => return existing result
    }

    for (size_t start = start_ali_pos; start <= end_ali_pos; start += GET_ML_VECTORS_BUG_WORKAROUND_INCREMENT) {
        int end = std::min(end_ali_pos, start+GET_ML_VECTORS_BUG_WORKAROUND_INCREMENT);
        get_ml_vectors(0, node, start, end);        // calculates tmp_out (see below)
    }

    const char *source_sequence = 0;
    int source_sequence_len = 0;

    GBDATA *gb_data = seq->get_bound_species_data();
    if (gb_data) {
        source_sequence_len = GB_read_string_count(gb_data);
        source_sequence     = GB_read_char_pntr(gb_data);
    }

    // create color string in 'outs':
    ST_ML_Color    *outs   = seq->color_out  + start_ali_pos;
    ST_base_vector *vec    = seq->tmp_out    + start_ali_pos; // tmp_out was calculated by get_ml_vectors above
    const char     *source = source_sequence + start_ali_pos;

    for (pos = start_ali_pos; pos <= end_ali_pos; pos++) {
        // search max vec for pos:

        double max = 0;
        double v;
        {
            int b;
            for (b = ST_A; b < ST_MAX_BASE; b++) {
                v = vec->b[b];
                if (v > max) max = v;
            }
        }

        {
            AWT_dna_base b = awt_dna_table.char_to_enum(*source); // convert seq-character to enum AWT_dna_base
            *outs          = 0;

            if (b != ST_UNKNOWN) {
                val = max / (0.0001 + vec->b[b]);   // calc ratio of max/real base-char
                if (val > 1.0) {                    // if real base-char is NOT the max-likely base-char
                    *outs = (int) (log(val));       // => insert color
                }
            }
        }
        outs++;
        vec++;
        source++;

        seq->color_out_valid_till[pos >> LD_BUCKET_SIZE] = latest_modification;
    }
    return seq->color_out;
}

void ST_ML::create_column_statistic(AW_root *awr, const char *awarname) {
    column_stat = new ColumnStat(get_gb_main(), awr, awarname);
}

const GBT_TREE *ST_ML::get_gbt_tree() const {
    return tree_root->get_root_node()->get_gbt_tree();
}

size_t ST_ML::count_species_in_tree() const {
    ARB_tree_info info;
    tree_root->get_root_node()->calcTreeInfo(info);
    return info.leafs;
}

AP_tree *ST_ML::find_node_by_name(const char *species_name) {
    AP_tree *node = NULL;
    if (hash_2_ap_tree) node = (AP_tree *)GBS_read_hash(hash_2_ap_tree, species_name);
    return node;
}


