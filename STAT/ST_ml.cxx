#include "st_ml.hxx"

#include <awt_csp.hxx>
#include <AP_filter.hxx>

#include <cctype>
#include <cmath>


#define st_assert(bed) arb_assert(bed)

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

void ST_base_vector::set(char base, ST_base_vector * inv_frequencies) {
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
        b[i] *= inv_frequencies->b[i];
    }
    ld_lik = 0;
    lik = 1.0;
}

void ST_base_vector::print() {
    int i;
    for (i = 0; i < ST_MAX_BASE; i++) {
        printf("%.3G ", b[i]);
    }
}

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

inline void ST_base_vector::mult(ST_base_vector * other) {
    float  *a  = &b[0];
    float  *c  = &other->b[0];
    double  a0 = a[0] * c[0];
    double  a1 = a[1] * c[1];
    double  a2 = a[2] * c[2];
    double  a3 = a[3] * c[3];
    double  a4 = a[4] * c[4];

    b[0]    = a0;
    b[1]    = a1;
    b[2]    = a2;
    b[3]    = a3;
    b[4]    = a4;
    ld_lik += other->ld_lik;
    lik    *= other->lik;
}

inline void ST_rate_matrix::mult(ST_base_vector * in, ST_base_vector * out) {
    int    i;
    float *pm = &m[0][0];

    for (i = ST_MAX_BASE - 1; i >= 0; i--) { // calc revers
        double sum0 = pm[4] * in->b[0];
        double sum1 = pm[3] * in->b[1];
        double sum2 = pm[2] * in->b[2];
        double sum3 = pm[1] * in->b[3];
        double sum4 = pm[0] * in->b[4];

        pm        += ST_MAX_BASE;
        out->b[i]  = (sum0 + sum1) + sum4 + (sum2 + sum3);
    }
    out->ld_lik = in->ld_lik;
    out->lik = in->lik;
}

ST_sequence_ml::ST_sequence_ml(const AliView *aliview, ST_ML *st_ml_)
    : AP_sequence(aliview)
    , st_ml(st_ml_)
    , sequence(new ST_base_vector[ST_MAX_SEQ_PART])
    , last_updated(0)
    , color_out(NULL)
    , color_out_valid_till(NULL)
{
}

ST_sequence_ml::~ST_sequence_ml() {
    delete [] sequence;
    delete color_out;
    delete color_out_valid_till;

    unbind_from_species(true);
}

static void st_sequence_callback(GBDATA *, int *cl, GB_CB_TYPE) {
    ST_sequence_ml *seq = (ST_sequence_ml *) cl;
    seq->sequence_change();
}

static void st_sequence_del_callback(GBDATA *, int *cl, GB_CB_TYPE) {
    ST_sequence_ml *seq = (ST_sequence_ml *) cl;
    seq->unbind_from_species(false);
}


GB_ERROR ST_sequence_ml::bind_to_species(GBDATA *gb_species) {
    GB_ERROR error = AP_sequence::bind_to_species(gb_species);
    if (!error) {
        GBDATA *gb_seq = get_bound_species_data();
        st_assert(gb_seq);

        error             = GB_add_callback(gb_seq, GB_CB_CHANGED, st_sequence_callback, (int *) this);
        if (!error) error = GB_add_callback(gb_seq, GB_CB_DELETE, st_sequence_del_callback, (int *) this);
    }
    return error;
}
void ST_sequence_ml::unbind_from_species(bool remove_callbacks) {
    if (remove_callbacks) {
        GBDATA *gb_seq = get_bound_species_data();
        st_assert(gb_seq);
        GB_remove_callback(gb_seq, GB_CB_CHANGED, st_sequence_callback, (int *) this);
        GB_remove_callback(gb_seq, GB_CB_DELETE, st_sequence_del_callback, (int *) this);
    }
    AP_sequence::unbind_from_species();
}

void ST_sequence_ml::sequence_change() {
    st_ml->clear_all();
}

AP_sequence *ST_sequence_ml::dup() const {
    return new ST_sequence_ml(get_aliview(), st_ml);
}

void ST_sequence_ml::set(const char *) {
    st_assert(0);                                   // hmm why not perform set_sequence() here ?
}

void ST_sequence_ml::unset() {
}

void ST_sequence_ml::set_sequence() {
    /*! Transform the sequence from character to vector, from st_ml->base to 'to' */

    int         i                   = st_ml->base;
    const char *source_sequence     = 0;
    int         source_sequence_len = 0;

    GBDATA *gb_data = get_bound_species_data();
    if (gb_data) {
        source_sequence_len = (int) GB_read_string_count(gb_data);
        source_sequence = GB_read_char_pntr(gb_data);
    }

    const char *s = source_sequence + st_ml->base;
    ST_base_vector *dest = sequence;
    ST_base_vector *freq = st_ml->inv_base_frequencies + st_ml->base;
    if (st_ml->base < source_sequence_len) {
        for (; i < st_ml->to; i++) {
            int c = *(s++);
            if (!c)
                break; // end of sequence
            dest->set(toupper(c), freq);
            dest++;
            freq++;
        }
    }
    for (; i < st_ml->to; i++) {
        dest->set('.', freq);
        dest++;
        freq++;
    }
}

AP_FLOAT ST_sequence_ml::combine(const AP_sequence *, const AP_sequence *, char *) {
    st_assert(0);
    return -1.0;
}

void ST_sequence_ml::partial_match(const AP_sequence *, long *, long *) const {
    st_assert(0); // function is expected to be unused
}

inline void ST_base_vector::check_overflow()
{
    double sum = 0.0;
    int    i;

    for (i = 0; i < ST_MAX_BASE; i++) {
        sum += b[i];
    }
    if (sum < .00001) { // what happend no data, extremely unlikely
        for (i = 0; i < ST_MAX_BASE; i++)
            b[i] = .25;
        ld_lik -= 5; // ???
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

void ST_sequence_ml::ungo() {
    last_updated = 0;
}

void ST_sequence_ml::go(const ST_sequence_ml * lefts, double leftl, const ST_sequence_ml * rights, double rightl) {
    int             pos;
    ST_base_vector  hbv;
    double          lc   = leftl / st_ml->step_size;
    double          rc   = rightl / st_ml->step_size;
    int             maxm = st_ml->max_matr - 1;
    ST_base_vector *lb   = lefts->sequence;
    ST_base_vector *rb   = rights->sequence;
    ST_base_vector *dest = sequence;

    for (pos = st_ml->base; pos < st_ml->to; pos++) {
        if (lb->lik != 1 || rb->lik != 1) {
            printf("error\n");
        }
        int distl = (int) (st_ml->rates[pos] * lc);
        int distr = (int) (st_ml->rates[pos] * rc);
        if (distl < 0)
            distl = -distl;
        if (distl > maxm)
            distl = maxm;
        if (distr < 0)
            distr = -distr;
        if (distr > maxm)
            distr = maxm;
        st_ml->rate_matrices[distl].mult(lb, dest);
        st_ml->rate_matrices[distr].mult(rb, &hbv);
        dest->mult(&hbv);
        dest->check_overflow();
        if (dest->lik != 1) {
            printf("error2\n");
        }
        dest++;
        lb++;
        rb++;
    }
}

ST_base_vector *ST_sequence_ml::tmp_out = 0;

void ST_sequence_ml::calc_out(ST_sequence_ml * next_branch, double dist) {
    // result will be in tmp_out

    int             pos;
    ST_base_vector *out   = tmp_out + st_ml->base;
    double          lc    = dist / st_ml->step_size;
    ST_base_vector *lefts = next_branch->sequence;
    int             maxm  = st_ml->max_matr - 1;

    for (pos = st_ml->base; pos < st_ml->to; pos++) {
        int distl = (int) (st_ml->rates[pos] * lc);
        if (distl < 0)
            distl = -distl;
        if (distl > maxm)
            distl = maxm;
        st_ml->rate_matrices[distl].mult(lefts, out);

        // correct frequencies
        for (int i = ST_A; i < ST_MAX_BASE; i++) {
            out->b[i] *= st_ml->base_frequencies[pos].b[i];
        }
        lefts++;
        out++;
    }
}

void ST_sequence_ml::print() {
    const char *data = GB_read_char_pntr(get_bound_species_data());
    for (int i = 0; i < ST_MAX_SEQ_PART; i++) {
        printf("POS %3i  %c     ", i, data[i]);
        printf("\n");
    }
}

AP_FLOAT ST_sequence_ml::count_weighted_bases() const {
    st_assert(0);
    return -1.0;
}

ST_ML::ST_ML(GBDATA * gb_maini) {
    memset((char *) this, 0, sizeof(*this));
    gb_main = gb_maini;
}

ST_ML::~ST_ML() {
    delete tree_root;
    free(alignment_name);
    if (hash_2_ap_tree) GBS_free_hash(hash_2_ap_tree);
    delete not_valid;
    delete[]base_frequencies;
    delete[]inv_base_frequencies;
    delete[]rate_matrices;
    if (!awt_csp) { // rates and ttratio have been allocated (see ST_ML::init)
        delete rates;
        delete ttratio;
    }
}

void ST_ML::print() {
    ;
}

void ST_ML::create_frequencies() {
    //! Translate characters to base frequencies

    base_frequencies     = new ST_base_vector[alignment_len];
    inv_base_frequencies = new ST_base_vector[alignment_len];

    if (!awt_csp) {
        for (int i = 0; i < alignment_len; i++) {
            for (int j = ST_A; j < ST_MAX_BASE; j++) {
                base_frequencies[i].b[j]     = 1.0;
                inv_base_frequencies[i].b[j] = 1.0;
            }
            base_frequencies[i].lik     = 1.0;
            inv_base_frequencies[i].lik = 1.0;
        }
    }
    else {
        for (int i = 0; i < alignment_len; i++) {
            for (int j = ST_A; j < ST_MAX_BASE; j++) {
                base_frequencies[i].b[j] = 0.01; // minimal frequency
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
                const float *freq = awt_csp->get_frequencies(toCount[j].c);
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
                if (sum > 0.01) { // valid column ??
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

void ST_ML::insert_tree_into_hash_rek(AP_tree * node) {
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
    max_dist = max_disti;
    if (rate_matrices)
        delete rate_matrices;
    rate_matrices = new ST_rate_matrix[nmatrices];
    max_matr = nmatrices;
    step_size = max_dist / max_matr;
    int i;
    for (i = 0; i < max_matr; i++) {
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
        delete father;                              // deletes me also

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

GB_ERROR ST_ML::init(const char *tree_name, const char *alignment_namei,
                     const char *species_names, int marked_only,
                     AWT_csp *awt_cspi, bool show_status)
{
    /*! this is the real constructor, call only once */

    GB_ERROR error = 0;

    if (is_inited) {
        error = "Column statistic can't be re-initialized";
    }
    else {
        GB_transaction ta(gb_main);
        GB_ERROR       awt_csp_error = 0;

        if (show_status) aw_openstatus("Activating column statistic");

        awt_csp       = awt_cspi;
        awt_csp_error = awt_csp->go(NULL);

        if (awt_csp_error) fprintf(stderr, "%s\n", awt_csp_error);

        alignment_name = strdup(alignment_namei);
        alignment_len  = GBT_get_alignment_len(gb_main, alignment_name);

        if (alignment_len < 10) {
            error = GB_await_error();
        }
        else {
            {
                AP_filter       filter(alignment_len);      // unfiltered
                AP_weights      weights(&filter);
                AliView        *aliview   = new AliView(gb_main, filter, weights, alignment_name);
                ST_sequence_ml *seq_templ = new ST_sequence_ml(aliview, this);

                tree_root = new AP_tree_root(aliview, AP_tree(0), seq_templ, false);
                // do not delete 'aliview' or 'seq_templ' (they belong to 'tree_root' now)
            }

            if (show_status) aw_status("load tree");
            tree_root->loadFromDB(tree_name);       // tree is not linked!

            if (show_status) aw_status("link tree");
            hash_2_ap_tree = GBS_create_hash(1000, GB_MIND_CASE); // send species into hash table

            /* delete species */
            if (species_names) { // keep names
                tree_root->remove_leafs(AWT_REMOVE_DELETED);

                error = tree_size_ok(tree_root);
                if (!error) {
                    char *l, *n;
                    keep_species_hash = GBS_create_hash(GBT_get_species_hash_size(gb_main), GB_MIND_CASE);
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
            else { // keep marked
                GBT_link_tree(tree_root->get_root_node()->get_gbt_tree(), gb_main, show_status, 0, 0);
                tree_root->remove_leafs((marked_only ? AWT_REMOVE_NOT_MARKED : 0)|AWT_REMOVE_DELETED);

                error = tree_size_ok(tree_root);
                if (!error) insert_tree_into_hash_rek(tree_root->get_root_node());
            }

            if (!error) {
                /* calc frequencies */
                if (show_status) aw_status("calculating frequencies");

                if (!awt_csp_error) {
                    rates   = awt_csp->get_rates();
                    ttratio = awt_csp->get_ttratio();
                }
                else {
                    float *alloc_rates   = new float[alignment_len];
                    float *alloc_ttratio = new float[alignment_len];

                    for (int i = 0; i < alignment_len; i++) {
                        alloc_rates[i]   = 1.0;
                        alloc_ttratio[i] = 2.0;
                    }
                    rates   = alloc_rates;
                    ttratio = alloc_ttratio;

                    awt_csp = 0;                    // mark rates and ttratio as "allocated"
                }
                create_frequencies();

                /* set update time */

                latest_modification = GB_read_clock(gb_main);

                /* create matrices */
                create_matrices(2.0, 1000);

                ST_sequence_ml::tmp_out = new ST_base_vector[alignment_len];
                is_inited = 1;
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

ST_sequence_ml *ST_ML::getOrCreate_seq(AP_tree *node) {
    ST_sequence_ml *seq = DOWNCAST(ST_sequence_ml*, node->get_seq());
    if (!seq) {
        seq = new ST_sequence_ml(tree_root->get_aliview(), this); // @@@ why not use dup() ?

        node->set_seq(seq);
        if (node->is_leaf) {
            st_assert(node->gb_node);
            seq->bind_to_species(node->gb_node);
        }
    }
    return seq;
}

ST_sequence_ml *ST_ML::do_tree(AP_tree * node) {
    /*! go through the tree and calculate the ST_base_vector from bottom to top
     */
    ST_sequence_ml *seq = getOrCreate_seq(node);
    if (!seq->last_updated) {
        if (node->is_leaf) {
            seq->set_sequence();
        }
        else {
            ST_sequence_ml *ls = do_tree(node->get_leftson());
            ST_sequence_ml *rs = do_tree(node->get_rightson());
            seq->go(ls, node->leftlen, rs, node->rightlen);
        }

        seq->last_updated = 1;
    }

    return seq;
}

void ST_ML::clear_all() {
    GB_transaction dummy(gb_main);
    undo_tree(tree_root->get_root_node());
    latest_modification = GB_read_clock(gb_main);
}

void ST_ML::undo_tree(AP_tree * node) {
    ST_sequence_ml *seq = getOrCreate_seq(node);
    seq->ungo();
    if (!node->is_leaf) {
        undo_tree(node->get_leftson());
        undo_tree(node->get_rightson());
    }
}

ST_sequence_ml *ST_ML::get_ml_vectors(char *species_name, AP_tree * node, int start_ali_pos, int end_ali_pos) {
    /* result will be in tmp_out
     *
     * assert end_ali_pos - start_ali_pos < ST_MAX_SEQ_PART
     *
     * @@@ CAUTION!!! get_ml_vectors has a bug:
     * it does not calculate the last value, if (end_ali_pos-start_ali_pos+1)==ST_MAX_SEQ_PART
     */

    if (!node) {
        if (!hash_2_ap_tree)
            return 0;
        node = (AP_tree *) GBS_read_hash(hash_2_ap_tree, species_name);
        if (!node)
            return 0;
    }

    st_assert((end_ali_pos - start_ali_pos + 1) <= ST_MAX_SEQ_PART);

    ST_sequence_ml *seq = getOrCreate_seq(node);

    if (start_ali_pos != base || end_ali_pos > to) {
        undo_tree(tree_root->get_root_node()); // undo everything
        base = start_ali_pos;
        to = end_ali_pos;
    }

    AP_tree *pntr;
    for (pntr = node->get_father(); pntr; pntr = pntr->get_father()) {
        ST_sequence_ml *sequ = getOrCreate_seq(pntr);
        if (sequ) sequ->ungo();
    }

    node->set_root();

    // get the sequence of my brother
    AP_tree        *brother        = node->get_brother();
    ST_sequence_ml *seq_of_brother = do_tree(brother);

    seq->calc_out(seq_of_brother, node->father->leftlen + node->father->rightlen);
    return seq;
}

int ST_ML::update_ml_likelihood(char *result[4], int *latest_update,
                                char *species_name, AP_tree * node)
// calculates values for 'Detailed column statistics' in ARB_EDIT4
{
    if (*latest_update >= latest_modification)
        return 1;

    // if node isn't given search it using species name:
    if (!node) {
        if (!hash_2_ap_tree)
            return 0;
        node = (AP_tree *) GBS_read_hash(hash_2_ap_tree, species_name);
        if (!node)
            return 0;
    }

    AWT_dna_base adb[4];
    int i;

    if (!result[0]) { // allocate Array-elements for result
        for (i = 0; i < 4; i++) {
            result[i] = (char *) GB_calloc(1, alignment_len + 1); // 0..alignment_len
        }
    }

    for (i = 0; i < 4; i++) {
        adb[i] = awt_dna_table.char_to_enum("ACGU"[i]);
    }

    for (int seq_start = 0; seq_start < alignment_len; seq_start
             += (ST_MAX_SEQ_PART - 1)) {
        // ^^^^^^^^^^^^^^^^^^^ work-around for bug in get_ml_vectors
        int seq_end = alignment_len;

        if ((seq_end - seq_start) >= ST_MAX_SEQ_PART) {
            seq_end = seq_start + (ST_MAX_SEQ_PART - 1);
        }
        get_ml_vectors(0, node, seq_start, seq_end);
    }

    ST_sequence_ml *seq = getOrCreate_seq(node);

    for (int pos = 0; pos < alignment_len; pos++) {
        ST_base_vector & vec = seq->tmp_out[pos];
        double sum = vec.b[ST_A] + vec.b[ST_C] + vec.b[ST_G] + vec.b[ST_T]
            + vec.b[ST_GAP];

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

    *latest_update = latest_modification;
    return 1;
}

ST_ML_Color *ST_ML::get_color_string(char *species_name, AP_tree * node,
                                     int start_ali_pos, int end_ali_pos)
{
    //  (Re-)Calculates the color string of a given node for sequence positions start_ali_pos..end_ali_pos
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

    double          val;
    ST_sequence_ml *seq = getOrCreate_seq(node);
    int             pos;

    if (!seq->color_out) {      // allocate mem for color_out if we not already have it
        seq->color_out = (ST_ML_Color *) GB_calloc(sizeof(ST_ML_Color),
                                                   alignment_len);
        seq->color_out_valid_till = (int *) GB_calloc(sizeof(int),
                                                      (alignment_len >> LD_BUCKET_SIZE) + ST_BUCKET_SIZE);
    }
    // search for first out-dated position:
    for (pos = start_ali_pos; pos <= end_ali_pos; pos += ST_BUCKET_SIZE) {
        if (seq->color_out_valid_till[pos >> LD_BUCKET_SIZE]
            < latest_modification)
            break;
    }
    if (pos > end_ali_pos) { // all positions are up-to-date
        return seq->color_out; // => return existing result
    }

    ST_base_vector *vec;
    int start;
    for (start = start_ali_pos; start <= end_ali_pos; start += (ST_MAX_SEQ_PART
                                                                - 1)) {
        // ^^^^^^^^^^^^^^^^^^^ work-around for bug in get_ml_vectors
        int end = end_ali_pos;
        if (end - start >= ST_MAX_SEQ_PART)
            end = start + (ST_MAX_SEQ_PART - 1);
        get_ml_vectors(0, node, start, end); // calculates tmp_out (see below)
    }

    const char *source_sequence = 0;
    int source_sequence_len = 0;

    GBDATA *gb_data = seq->get_bound_species_data();
    if (gb_data) {
        source_sequence_len = GB_read_string_count(gb_data);
        source_sequence     = GB_read_char_pntr(gb_data);
    }
    // create color string in 'outs':
    ST_ML_Color *outs   = seq->color_out + start_ali_pos;
    vec                 = seq->tmp_out + start_ali_pos; // tmp_out was calculated by get_ml_vectors above
    const char  *source = source_sequence + start_ali_pos;

    for (pos = start_ali_pos; pos <= end_ali_pos; pos++) {

        // search max vec for pos:
        double max = 0;
        double v;
        {
            int b;
            for (b = ST_A; b < ST_MAX_BASE; b++) {
                v = vec->b[b];
                if (v > max)
                    max = v;
            }
        }

        {
            AWT_dna_base b = awt_dna_table.char_to_enum(*source); // convert seq-character to enum AWT_dna_base
            *outs = 0;
            if (b != ST_UNKNOWN) {
                val = max / (0.0001 + vec->b[b]); // calc ratio of max/real base-char
                if (val > 1.0) { // if real base-char is NOT the max-likely base-char
                    *outs = (int) (log(val)); // => insert color
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

