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

class Sampler {
    double sum;                                     //!< sum of added values
    size_t count;                                   //!< number of values added
public:
    Sampler() : sum(0.0) , count(0) {}

    void add(double val) {
        sum += val;
        count++;
    }
    void add(const Sampler& other) {
        sum   += other.sum;
        count += other.count;
    }

    size_t get_count() const { return count; }
    double get_sum() const { return sum; }
    double get_median() const { return sum / count; }
};

class VarianceSampler : public Sampler {
    /*! stores values such that variance can quickly be calculated
     * for algorithm see
     * http://de.wikipedia.org/wiki/Standardabweichung#Berechnung_f.C3.BCr_auflaufende_Messwerte or
     * http://en.wikipedia.org/wiki/Standard_deviation#Rapid_calculation_methods
     */

    double square_sum;                              //!< sum of squares of added values

public:
    VarianceSampler() : square_sum(0.0) {}

    void add(double val) {
        Sampler::add(val);
        square_sum += val*val;
    }

    void add(const VarianceSampler& other) {
        Sampler::add(other);
        square_sum += other.square_sum;
    }

    double get_variance(double median) const { return square_sum/get_count() - median*median; }
    double get_variance() const { return get_variance(get_median()); }
};

class LikelihoodRanges : virtual Noncopyable {
    /*! The alignment is split into multiple, similar-sized column ranges.
     * For each range the likelihoods get summarized
     */
    size_t           ranges;
    VarianceSampler *likelihood;

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

    VarianceSampler summarize_all_ranges();
    char *generate_string(Sampler& t_values);

    // int is_bad();                                   // 0 == ok, 1 strange, 2 bad, 3 very bad
};

struct ColumnQualityInfo {
    LikelihoodRanges stat_half;                    // two ranges (one for each half)
    LikelihoodRanges stat_five;                    // five ranges
    LikelihoodRanges stat_user;                    // user defined range size
    LikelihoodRanges stat_cons;                    // two ranges (conserved + variable positions) -- @@@ unused

    ColumnQualityInfo(int seq_len, int bucket_size);

    size_t overall_range_count() {
        return
            stat_half.get_range_count() +
            stat_five.get_range_count() +
            stat_user.get_range_count() +
            stat_cons.get_range_count();
    }
};

class WeightedFilter;
class ColumnStat;

GB_ERROR st_ml_check_sequence_quality(GBDATA *gb_main, const char *tree_name,
                                      const char *alignment_name, ColumnStat *colstat, const WeightedFilter *weighted_filter, int bucket_size,
                                      int marked_only, st_report_enum report, const char *dest_field); 

#else
#error st_quality.hxx included twice
#endif // ST_QUALITY_HXX
