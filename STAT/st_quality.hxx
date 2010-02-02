// =============================================================== //
//                                                                 //
//   File      : st_quality.hxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef ST_QUALITY_HXX
#define ST_QUALITY_HXX

#ifndef ST_WINDOW_HXX
#include "st_window.hxx"
#endif

#define st_assert(cond) arb_assert(cond)

class SummarizedLikelihoods {
    //! contains summarized likelihoods for several columns

    double likelihood_sum;                          //!< sum of likelihoods
    double likelihood2_sum;                         //!< sum of likelihood-squares
    size_t count;                                   //!< number of likelihoods(=columns) added

public:
    SummarizedLikelihoods()
        : likelihood_sum(0.0)
        , likelihood2_sum(0.0)
        , count(0)
    {}

    void add(double likelihood) {
        likelihood_sum  += likelihood;
        likelihood2_sum += likelihood*likelihood;
        count++;
    }

    void add(const SummarizedLikelihoods& other) {
        likelihood_sum  += other.likelihood_sum;
        likelihood2_sum += other.likelihood2_sum;
        count           += other.count;
    }

    size_t get_count() const { return count; }

    double get_mean_likelihood() const { return likelihood_sum / count; }
    double get_mean_likelihood2() const { return likelihood2_sum / count; }
};

class LikelihoodRanges {
    /*! The alignment is splitted into multiple, similar-sized column ranges.
     * For each range the likelihoods get summarized
     */
    size_t                 ranges;
    SummarizedLikelihoods *likelihood;

public:

    LikelihoodRanges(size_t ranges);
    ~LikelihoodRanges() { delete [] likelihood; }

    size_t get_range_count() const { return ranges; }

    void add(size_t range, double probability) {
        st_assert(range < ranges);
        likelihood[range].add(probability);
    }
    void add_relative(double seq_rel_pos, double probability) {
        st_assert(seq_rel_pos>=0.0 && seq_rel_pos<1.0);
        add(seq_rel_pos*ranges, probability);
    }

    SummarizedLikelihoods summarize_all_ranges();
    char *generate_string();

    // int is_bad();                                   // 0 == ok, 1 strange, 2 bad, 3 very bad
};

class ColumnQualityInfo {
public:
    LikelihoodRanges stat_half;                    // two ranges (one for each half)
    LikelihoodRanges stat_five;                    // five ranges
    LikelihoodRanges stat_user;                    // user defined range size
    LikelihoodRanges stat_cons;                    // two ranges (conserved + variable positions) -- @@@ unused 

public:
    ColumnQualityInfo(int seq_len, int bucket_size);

    size_t overall_range_count() {
        return
            stat_half.get_range_count() +
            stat_five.get_range_count() +
            stat_user.get_range_count() +
            stat_cons.get_range_count();
    }
};

GB_ERROR st_ml_check_sequence_quality(GBDATA *gb_main, const char *tree_name,
                                      const char *alignment_name, ColumnStat *colstat, int bucket_size,
                                      int marked_only, st_report_enum report, const char *dest_field);


#else
#error st_quality.hxx included twice
#endif // ST_QUALITY_HXX
