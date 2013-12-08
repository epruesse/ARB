// =============================================================== //
//                                                                 //
//   File      : st_ml.hxx                                         //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef ST_ML_HXX
#define ST_ML_HXX

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef _GLIBCXX_ALGORITHM
#include <algorithm>
#endif
#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef CB_H
#include <cb.h>
#endif

#define st_assert(cond) arb_assert(cond)

class ColumnStat;
class MostLikelySeq;

class AW_root;
class AW_awar;
class AW_window;

class AP_tree;
class AP_tree_root;
class GBT_TREE;
class AP_filter;
class WeightedFilter;

typedef unsigned char ST_ML_Color;

typedef float ST_FLOAT;
// typedef double ST_FLOAT; // careful, using double has quite an impact on the memory footprint

enum DNA_Base {
    ST_A,
    ST_C,
    ST_G,
    ST_T,
    ST_GAP,
    ST_MAX_BASE,                                    // [can't be changed w/o recoding several parts]
    ST_UNKNOWN = -1
};

struct ST_base_vector {
    ST_FLOAT b[ST_MAX_BASE];                        // acgt-
    int      ld_lik;
    ST_FLOAT lik;                                   // likelihood = 2^ld_lik * lik * (b[0] + b[1] + b[2] ..)

    void setTo(ST_FLOAT freq) {
        for (int base = ST_A; base<ST_MAX_BASE; ++base) {
            b[base] = freq;
        }
    }
    void multiplyWith(ST_FLOAT factor) {
        for (int base = ST_A; base<ST_MAX_BASE; ++base) {
            b[base] *= factor;
        }
    }
    void increaseBy(ST_FLOAT inc) {
        for (int base = ST_A; base<ST_MAX_BASE; ++base) {
            b[base] += inc;
        }
    }
    void makeInverseOf(const ST_base_vector& other, ST_FLOAT other_min_freq) {
        for (int base = ST_A; base<ST_MAX_BASE; ++base) {
            b[base] = other_min_freq / other.b[base];
        }
    }

    void setBase(const ST_base_vector& frequencies, char base);

    void check_overflow();
    void print();

    ST_FLOAT summarize() const { return b[ST_A] + b[ST_C] + b[ST_G] + b[ST_T] + b[ST_GAP]; }
    ST_FLOAT min_frequency() { return std::min(std::min(std::min(b[ST_A], b[ST_C]), std::min(b[ST_G], b[ST_T])), b[ST_GAP]); }
    ST_FLOAT max_frequency() { return std::max(std::max(std::max(b[ST_A], b[ST_C]), std::max(b[ST_G], b[ST_T])), b[ST_GAP]); }

    inline ST_base_vector& operator *= (const ST_base_vector& other);
};

class ST_rate_matrix {
    // symmetric matrix containing only two different values

    ST_FLOAT diag;                                  // value for indices [0][0], [1][1]...
    ST_FLOAT rest;                                  // other indices

public:

    ST_rate_matrix() : diag(0.0), rest(0.0) {}
    void set(double dist, double TT_ratio);
    inline void transform(const ST_base_vector& in, ST_base_vector& out) const;
    void print();
};

class ST_ML : virtual Noncopyable {

    /* Note: Because we have only limited memory we split the
     * sequence into several ST_MAX_SEQ_PART-sized parts during stat-calculation
     */

    char    *alignment_name;
    GB_HASH *hash_2_ap_tree;                        // hash table to get from name to tree_node
    GB_HASH *keep_species_hash;                     // temporary hash to find ??? 
    int      refresh_n;
    int     *not_valid;                             // which columns are valid

    AP_tree_root *tree_root;
    int           latest_modification;              // last mod
    size_t        first_pos;                        // first..
    size_t        last_pos;                         // .. and last alignment position of current slice

    AW_CB0     postcalc_cb;
    AW_window *cb_window;

    GBDATA *gb_main;

    ColumnStat *column_stat;
    // if column_stat == 0 -> rates and ttratio are owned by ST_ML,
    // otherwise they belong to column_stat
    const float *rates;                             // column independent
    const float *ttratio;                           // column independent

    ST_base_vector *base_frequencies;               // column independent
    ST_base_vector *inv_base_frequencies;           // column independent

    double max_dist;                                // max_dist for rate_matrices
    double step_size;                               // max_dist/step_size matrices

    int             max_rate_matrices;              // number of rate_matrices
    ST_rate_matrix *rate_matrices;                  // one matrix for each distance

    bool is_initialized;

    size_t distance_2_matrices_index(int distance) {
        if (distance<0) distance = -distance;       // same matrix for neg/pos distance

        return distance >= max_rate_matrices
            ? (max_rate_matrices-1)
            : size_t(distance);
    }

    MostLikelySeq *getOrCreate_seq(AP_tree *node);

    const MostLikelySeq *get_mostlikely_sequence(AP_tree *node);
    void                 undo_tree(AP_tree *node);  // opposite of get_mostlikely_sequence()

    void insert_tree_into_hash_rek(AP_tree *node);
    void create_matrices(double max_disti, int nmatrices);
    void create_frequencies();
    
    static long delete_species(const char *key, long val, void *cd_st_ml);

public:

    ST_ML(GBDATA *gb_main);
    ~ST_ML();

    void create_column_statistic(AW_root *awr, const char *awarname, AW_awar *awar_default_alignment);
    ColumnStat *get_column_statistic() { return column_stat; }

    GB_ERROR calc_st_ml(const char           *tree_name,
                        const char           *alignment_name,
                        const char           *species_names, // 0 -> all [marked] species (else species_names is a (char)1 separated list of species)
                        int                   marked_only,
                        ColumnStat           *colstat,
                        const WeightedFilter *weighted_filter) __ATTR__USERESULT;

    void cleanup();

    const AP_filter *get_filter() const;
    size_t get_filtered_length() const;
    size_t get_alignment_length() const;

    ST_rate_matrix& get_matrix_for(int distance) {
        return rate_matrices[distance_2_matrices_index(distance)];
    }

    GBDATA *get_gb_main() const { return gb_main; }
    const GBT_TREE *get_gbt_tree() const;
    size_t count_species_in_tree() const;

    AP_tree *find_node_by_name(const char *species_name);

    void set_postcalc_callback(AW_CB0 postcalc_cb_, AW_window *cb_window_) {
        postcalc_cb = postcalc_cb_;
        cb_window   = cb_window_;
    }
    void do_postcalc_callback() { if (postcalc_cb) postcalc_cb(cb_window); }

    size_t get_first_pos() const { return first_pos; }
    size_t get_last_pos() const { return last_pos; }

    double get_step_size() const { return step_size; }

    const ST_base_vector *get_inv_base_frequencies() const { return inv_base_frequencies; }
    const ST_base_vector& get_base_frequency_at(size_t pos) const { return base_frequencies[pos]; }
    float get_rate_at(size_t pos) const { return rates[pos]; }

    void set_modified(int *what = 0);
    void set_refresh();                             // set flag for refresh

    void clear_all();                               // delete all caches

    MostLikelySeq *get_ml_vectors(const char *species_name, AP_tree *node, size_t start_ali_pos, size_t end_ali_pos);
    ST_ML_Color *get_color_string(const char *species_name, AP_tree *node, size_t start_ali_pos, size_t end_ali_pos);

    bool update_ml_likelihood(char *result[4], int& latest_update, const char *species_name, AP_tree *node);

    int refresh_needed();
};

#else
#error st_ml.hxx included twice
#endif // ST_ML_HXX
