// =============================================================== //
//                                                                 //
//   File      : PT_partition.h                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2012   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef PT_PARTITION_H
#define PT_PARTITION_H

#ifndef _GLIBCXX_CMATH
#include <cmath>
#endif

class PrefixIterator {
    // iterates over prefixes of length given to ctor.
    //
    // PT_QU will only occur at end of prefix,
    // i.e. prefixes will be shorter than given length if PT_QU occurs

    PT_BASES low, high;
    int      len;

    char *part;
    int   plen;

    char END() const { return high+1; }
    size_t base_span() const { return high-low+1; }

    void inc() {
        if (plen) {
            do {
                part[plen-1]++;
                if (part[plen-1] == END()) {
                    --plen;
                }
                else {
                    for (int l = plen; l<len; ++l) {
                        part[l] = low;
                        plen    = l+1;
                        if (low == PT_QU) break;
                    }
                    break;
                }
            }
            while (plen);
        }
        else {
            part[0] = END();
        }
    }

public:

    void reset() {
        if (len) {
            for (int l = 0; l<len; ++l) part[l] = low;
        }
        else {
            part[0] = 0;
        }

        plen = (low == PT_QU && len) ? 1 : len;
    }

    PrefixIterator(PT_BASES low_, PT_BASES high_, int len_)
        : low(low_),
          high(high_),
          len(len_),
          part((char*)malloc(len ? len : 1))
    {
        reset();
    }
    PrefixIterator(const PrefixIterator& other)
        : low(other.low),
          high(other.high),
          len(other.len),
          part((char*)malloc(len ? len : 1)),
          plen(other.plen)
    {
        memcpy(part, other.part, plen);
    }
    DECLARE_ASSIGNMENT_OPERATOR(PrefixIterator);
    ~PrefixIterator() {
        free(part);
    }

    const char *prefix() const { return part; }
    size_t length() const { return plen; }

    char *copy() const {
        pt_assert(!done());
        char *result = (char*)malloc(plen+1);
        memcpy(result, part, plen);
        result[plen] = 0;
        return result;
    }

    bool done() const { return part[0] == END(); }

    const PrefixIterator& operator++() { // ++PrefixIterator
        pt_assert(!done());
        inc();
        return *this;
    }

    size_t steps() const {
        size_t count = 1;
        size_t bases = base_span();
        for (int l = 0; l<len; ++l) {
            if (low == PT_QU) {
                // PT_QU branches end instantly
                count = 1+(bases-1)*count;
            }
            else {
                count *= bases;
            }
        }
        return count;
    }

    bool matches_at(const char *probe) const {
        for (int p = 0; p<plen; ++p) {
            if (probe[p] != part[p]) {
                return false;
            }
        }
        return true;
    }
};

// @@@ update STAGE1_INDEX_BYTES_PER_BASE (values are too high)
#ifdef ARB_64
# define STAGE1_INDEX_BYTES_PER_BASE 55
#else
# define STAGE1_INDEX_BYTES_PER_BASE 35
#endif

inline ULONG estimate_memusage_kb(ULONG base_positions) {
    return (STAGE1_INDEX_BYTES_PER_BASE*base_positions)/1024;
}

static double base_probability(char base) {
    pt_assert(base >= PT_QU && base < PT_B_MAX);
    static double bprob[PT_B_MAX] = {
        // @@@ these are fakes; use real base probabilities here
        0.02, // PT_QU
        0.02, // PT_N
        0.24, // PT_A
        0.24, // PT_C
        0.24, // PT_G
        0.24, // PT_T
    };
    return bprob[safeCharIndex(base)];
}

inline double calc_probability(const char *prefix, size_t length) {
    double prob = 1.0;
    for (size_t i = 0; i<length; ++i) {
        prob *= base_probability(prefix[i]);
    }
    return prob;
}

class PrefixProbabilities {
    int     prefixes;
    double *prob;
    double *left_prob;

public:
    PrefixProbabilities(const PrefixIterator& iter) {
        prefixes  = iter.steps();
        prob      = new double[prefixes];
        left_prob = new double[prefixes+1];

        PrefixIterator prefix(iter);
        prefix.reset();

        left_prob[0] = 0.0;

        int i = 0;
        while (!prefix.done()) {
            pt_assert(i<prefixes);
            prob[i]        = calc_probability(prefix.prefix(), prefix.length());
            left_prob[i+1] = left_prob[i]+prob[i];
            ++i;
            ++prefix;
        }
    }
    PrefixProbabilities(const PrefixProbabilities& other)
        : prefixes(other.prefixes)
    {
        prob      = new double[prefixes];
        left_prob = new double[prefixes+1];

        memcpy(prob, other.prob, sizeof(*prob)*prefixes);
        memcpy(left_prob, other.left_prob, sizeof(*prob)*(prefixes+1));
    }
    DECLARE_ASSIGNMENT_OPERATOR(PrefixProbabilities);
    ~PrefixProbabilities() {
        delete [] prob;
        delete [] left_prob;
    }

    double get(int prefix_idx) {
        pt_assert(prefix_idx >= 0 && prefix_idx<prefixes); // index must be in range [0 ... prefixes-1]
        return prob[prefix_idx];
    }

    double sum_left_of(int prefix_idx) const {
        pt_assert(prefix_idx >= 0 && prefix_idx <= prefixes); // index must be in range [0 ... prefixes]
        return left_prob[prefix_idx];
    }

    int find_index_near_leftsum(double wanted_leftsum) {
        // returned index is in range [0 ... prefixes]

        int    best_idx  = -1;
        double best_diff = 2.0;

        for (int i = 0; i <= prefixes; ++i) {
            double diff = fabs(sum_left_of(i)-wanted_leftsum);
            if (diff<best_diff) {
                best_diff = diff;
                best_idx  = i;
            }
        }

        pt_assert(best_idx >= 0);
        return best_idx;
    }
};

class Partitioner {
    PrefixIterator       iter;
    int                  passes;
    PrefixProbabilities  prob;
    int                 *start_of_pass; // contains prefix index

    bool have_zero_prob_passes() {
        for (int p = 1; p <= passes; ++p) {
            if (pass_probability(p) <= 0.0) return true;
        }
        return false;
    }

    void calculate_pass_starts() {
        pt_assert(passes <= steps());
        delete [] start_of_pass;
        start_of_pass = new int[passes+1];

        double prob_per_pass = 1.0/passes;

        start_of_pass[0] = 0;
        for (int p = 1; p < passes; ++p) {
            double pass_prob = p*prob_per_pass;
            start_of_pass[p] = prob.find_index_near_leftsum(pass_prob);
        }
        start_of_pass[passes] = steps();

        // ensure there are no empty passes:
        for (int p = 0; p<passes; ++p) {
            if (start_of_pass[p] >= start_of_pass[p+1]) {
                int nidx = start_of_pass[p]+1;
                if (nidx<steps()) {
                    start_of_pass[p+1] = nidx;
                }
            }
        }
        for (int p = passes-1; p >= 0; --p) {
            if (start_of_pass[p] >= start_of_pass[p+1]) {
                start_of_pass[p] = start_of_pass[p+1]-1;
                pt_assert(start_of_pass[p] >= 0);
            }
        }

        pt_assert(!have_zero_prob_passes());
    }

public:
    Partitioner(int partsize) // @@@ store bps in Partitioner?
        : iter(PT_QU, PT_T, partsize),
          passes(-1),
          prob(iter),
          start_of_pass(NULL)
    {}
    Partitioner(const Partitioner& other)
        : iter(other.iter),
          passes(other.passes),
          prob(other.prob)
    {
        if (other.start_of_pass) {
            start_of_pass = new int[passes+1];
            memcpy(start_of_pass, other.start_of_pass, sizeof(*start_of_pass)*(passes+1));
        }
        else {
            start_of_pass = NULL;
        }
    }
    DECLARE_ASSIGNMENT_OPERATOR(Partitioner);
    ~Partitioner() {
        deselect_passes();
    }

    int steps() const { return iter.steps(); }
    bool done() const { return iter.done(); }
    bool contains(const char *probe) const { return iter.matches_at(probe); }
    bool next() { ++iter; return !done(); }

    int max_allowed_passes() const { return steps(); }

    bool select_passes(int passes_wanted) {
        if (passes_wanted < 1 || passes_wanted > max_allowed_passes()) return false;
        passes = passes_wanted;
        calculate_pass_starts();
        return true;
    }
    void deselect_passes() {
        passes        = -1;
        delete [] start_of_pass; start_of_pass = NULL;
    }

    bool passes_were_selected() const { return passes >= 1; }
    bool legal_pass(int pass) const {
        pt_assert(passes_were_selected());
        return pass >= 1 && pass <= passes;
    }
    int selected_passes() const {
        pt_assert(passes_were_selected());
        return passes;
    }

    double pass_probability(int pass) const {
        pt_assert(legal_pass(pass));

        int pass_idx = pass-1;

        double prev_prob = prob.sum_left_of(start_of_pass[pass_idx]);
        double this_prob = prob.sum_left_of(start_of_pass[pass_idx+1]);

        return this_prob-prev_prob;
    }

    size_t estimate_kb_for_pass(int pass, size_t overall_base_count) const {
        pt_assert(legal_pass(pass));

        size_t this_pass_base_count = size_t(pass_probability(pass)*overall_base_count+0.5);
        return estimate_memusage_kb(this_pass_base_count);
    }

    size_t max_kb_for_passes(int passes_wanted, size_t overall_base_count) {
        ASSERT_RESULT(bool, true, select_passes(passes_wanted));
        size_t max_kb = 0;
        for (int p = 1; p <= passes; ++p) {
            size_t kb = estimate_kb_for_pass(p, overall_base_count);
            if (kb>max_kb) max_kb = kb;
        }
        return max_kb;
    }
};

#else
#error PT_partition.h included twice
#endif // PT_PARTITION_H
