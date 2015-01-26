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

#ifndef PT_PREFIXITER_H
#include "PT_prefixIter.h"
#endif
#ifndef _GLIBCXX_CMATH
#include <cmath>
#endif

// --------------------------------------------------------------------------------
// central settings for memory estimations
// 

#ifdef ARB_64

# define STAGE1_INDEX_BYTES_PER_PASS_OLIGO 3.7  // size of index (for each oligo inserted in one pass)
# define STAGE1_INDEX_BYTES_PER_BASE       0.1  // size of index (for each bp in database)
# define STAGE1_OTHER_BYTES_PER_PASS_OLIGO 1.0  // non-index data (for each oligo inserted in one pass)
# define STAGE1_OTHER_BYTES_PER_BASE       1.15 // non-index data (for each bp in database)

# define STAGE1_INDEX_EXTRA_MB 350 // additional constant memory used by index (+ a bit safety)
# define STAGE1_OTHER_EXTRA_MB 80  // additional constant memory used elsewhere (+ a bit safety)

#else

# define STAGE1_INDEX_BYTES_PER_PASS_OLIGO 3.2  // size of index (for each oligo inserted in one pass)
# define STAGE1_INDEX_BYTES_PER_BASE       0    // size of index (for each bp in database)
# define STAGE1_OTHER_BYTES_PER_PASS_OLIGO 0.7  // non-index data (for each oligo inserted in one pass)
# define STAGE1_OTHER_BYTES_PER_BASE       1    // non-index data (for each bp in database)

# define STAGE1_INDEX_EXTRA_MB 150 // additional constant memory used by index (+ a bit safety)
# define STAGE1_OTHER_EXTRA_MB 50  // additional constant memory used elsewhere (+ a bit safety)

#endif

# define PTSERVER_BIN_MB 20       // binary mem footprint of ptserver (incl. libs, w/o DB) detected using pmap

#define STAGE1_BYTES_PER_PASS_OLIGO (STAGE1_INDEX_BYTES_PER_PASS_OLIGO + STAGE1_OTHER_BYTES_PER_PASS_OLIGO)
#define STAGE1_BYTES_PER_BASE       (STAGE1_INDEX_BYTES_PER_BASE       + STAGE1_OTHER_BYTES_PER_BASE)
#define STAGE1_EXTRA_MB             (STAGE1_INDEX_EXTRA_MB             + STAGE1_OTHER_EXTRA_MB)

inline ULONG estimate_stage1_memusage_kb(ULONG all_bp, ULONG partition_bp) {
    return ULONG(STAGE1_BYTES_PER_PASS_OLIGO * partition_bp / 1024.0 +
                 STAGE1_BYTES_PER_BASE       * all_bp / 1024.0 +
                 STAGE1_EXTRA_MB             * 1024 +
                 0.5);
}

static double base_probability(char base) {
    pt_assert(base >= PT_QU && base < PT_BASES);
    static double bprob[PT_BASES] = {
        // probabilities are taken from silva-108-SSU-ref
        0.0014, // PT_QU
        0.0003, // PT_N
        0.2543, // PT_A
        0.2268, // PT_C
        0.3074, // PT_G
        0.2098, // PT_T
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
    int     depth;
    int     prefixes;
    double *left_prob;

public:
    PrefixProbabilities(int depth_)
        : depth(depth_)
    {
        PrefixIterator prefix(PT_QU, PT_T, depth);
        prefixes  = prefix.steps();

        left_prob = new double[prefixes+1];
        left_prob[0] = 0.0;

        double prob[prefixes];

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
        : depth(other.depth),
          prefixes(other.prefixes)
    {
        left_prob = new double[prefixes+1];
        memcpy(left_prob, other.left_prob, sizeof(*left_prob)*(prefixes+1));
    }
    DECLARE_ASSIGNMENT_OPERATOR(PrefixProbabilities);
    ~PrefixProbabilities() { delete [] left_prob; }

    int get_prefix_count() const { return prefixes; }
    int get_depth() const { return depth; }

    double left_of(int prefix_idx) const {
        pt_assert(prefix_idx >= 0 && prefix_idx <= prefixes); // index must be in range [0 ... prefixes]
        return left_prob[prefix_idx];
    }
    double of_range(int first_idx, int last_idx) const {
        pt_assert(first_idx <= last_idx);
        pt_assert(first_idx >= 0 && last_idx<prefixes); // index must be in range [0 ... prefixes-1]

        return left_of(last_idx+1) - left_of(first_idx);
    }
    double of(int prefix_idx) {
        pt_assert(prefix_idx >= 0 && prefix_idx<prefixes); // index must be in range [0 ... prefixes-1]
        return of_range(prefix_idx, prefix_idx);
    }

    int find_index_near_leftsum(double wanted) const {
        // returned index is in range [0 ... prefixes]

        int best_idx;
        if (prefixes == 0) best_idx = 0;
        else {
            int low  = 0;
            int high = prefixes;

            double lol = left_of(low);
            if (lol >= wanted) best_idx = low;
            else {
                double loh = left_of(high);
                if (loh<wanted) best_idx = high;
                else  {
                    while ((low+1)<high) {
                        pt_assert(lol<wanted && wanted<=loh);

                        int mid = (low+high)/2;
                        pt_assert(mid != low && mid != high);

                        double left_of_mid = left_of(mid);
                        if (left_of_mid<wanted) {
                            low = mid;
                            lol = left_of_mid;
                        }
                        else {
                            high = mid;
                            loh  = left_of_mid;
                        }
                    }
                    best_idx = fabs(lol-wanted) < fabs(loh-wanted) ? low : high;
                }
            }
        }
        return best_idx;
    }
};

class DecisionTree : virtual Noncopyable {
    // associate bool-values with probe-prefixes

    bool          decision;
    bool          decided;
    DecisionTree *child[PT_BASES]; // all NULL if decided

    void forgetChilds() {
        for (int i = PT_QU; i<PT_BASES; ++i) {
            delete child[i];
            child[i] = NULL;
        }
    }

public:
    DecisionTree()
        : decided(false)
    {
        for (int i = PT_QU; i<PT_BASES; ++i) {
            child[i] = NULL;
        }
    }
    ~DecisionTree() {
        forgetChilds();
    }

    void setValue(const char *probe, size_t len, bool wanted) {
        if (len>0) {
            DecisionTree*& probe_child  = child[safeCharIndex(*probe)];
            if (!probe_child) probe_child = new DecisionTree;
            probe_child->setValue(probe+1, len-1, wanted);
        }
        else {
            decision = wanted;
            decided  = true;
        }
    }

    bool getValue(const char *probe) const {
        if (decided) return decision;
        return child[safeCharIndex(*probe)]->getValue(probe+1);
    }

    void optimize() {
        if (!decided) {
            child[PT_QU]->optimize();
            bool concordant      = child[PT_QU]->decided;
            bool common_decision = concordant ? child[PT_QU]->decision : false;

            for (int i = PT_QU+1; i<PT_BASES; ++i) {
                child[i]->optimize();
                if (concordant) {
                    if (child[i]->decided) {
                        if (child[i]->decision != common_decision) {
                            concordant = false;
                        }
                    }
                    else {
                        concordant = false;
                    }
                }
            }

            if (concordant) {
                forgetChilds();
                decided  = true;
                decision = common_decision;
            }
        }
    }
};

class MarkedPrefixes {
    int   depth;
    int   max_partitions;
    bool *marked;

    DecisionTree *decision;

    void forget_decision_tree() {
        delete decision;
        decision = NULL;
    }

public:
    MarkedPrefixes(int depth_)
        : depth(depth_),
          max_partitions(PrefixIterator(PT_QU, PT_T, depth).steps()),
          marked(new bool[max_partitions]),
          decision(NULL)
    {
        clearMarks();
    }
    MarkedPrefixes(const MarkedPrefixes& other)
        : depth(other.depth),
          max_partitions(other.max_partitions),
          marked(new bool[max_partitions]),
          decision(NULL)
    {
        memcpy(marked, other.marked, max_partitions*sizeof(*marked));
    }
    DECLARE_ASSIGNMENT_OPERATOR(MarkedPrefixes);
    ~MarkedPrefixes() {
        delete [] marked;
        delete decision;
    }

    void mark(int idx1, int idx2) {
        pt_assert(idx1 <= idx2);
        pt_assert(idx1 >= 0);
        pt_assert(idx2 < max_partitions);

        for (int p = idx1; p <= idx2; ++p) {
            marked[p] = true;
        }
    }
    void clearMarks() {
        for (int p = 0; p<max_partitions; ++p) {
            marked[p] = false;
        }
    }

    void predecide() {
        forget_decision_tree();
        decision = new DecisionTree;

        PrefixIterator iter(PT_QU, PT_T, depth);
        int            idx = 0;

        while (!iter.done()) {
            pt_assert(idx<max_partitions);
            decision->setValue(iter.prefix(), iter.length(), marked[idx]);
            ++iter;
            ++idx;
        }
        decision->optimize();
    }


    bool isMarked(const char *probe) const {
        pt_assert(decision); // predecide() not called
        return decision->getValue(probe);
    }
};

class Partition {
    PrefixProbabilities prob;

    int passes;
    int current_pass;

    MarkedPrefixes prefix;

    int *start_of_pass;                 // contains prefix index

    mutable size_t max_probes_in_any_pass;

    int first_index_of_pass(int pass) const {
        pt_assert(legal_pass(pass));
        return start_of_pass[pass-1];
    }
    int last_index_of_pass(int pass) const {
        pt_assert(legal_pass(pass));
        return start_of_pass[pass]-1;
    }

    bool have_zero_prob_passes() {
        for (int p = 1; p <= passes; ++p) {
            if (pass_probability(p) <= 0.0) return true;
        }
        return false;
    }

    void calculate_pass_starts() {
        pt_assert(passes <= max_allowed_passes());
        delete [] start_of_pass;
        start_of_pass = new int[passes+1];

        double prob_per_pass = 1.0/passes;

        start_of_pass[0] = 0;
        for (int p = 1; p < passes; ++p) {
            double pass_prob = p*prob_per_pass;
            start_of_pass[p] = prob.find_index_near_leftsum(pass_prob);
        }
        start_of_pass[passes] = max_allowed_passes();

        // ensure there are no empty passes:
        for (int p = 0; p<passes; ++p) {
            if (start_of_pass[p] >= start_of_pass[p+1]) {
                int nidx = start_of_pass[p]+1;
                if (nidx<max_allowed_passes()) {
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

    void markPrefixes() {
        pt_assert(!done());

        prefix.clearMarks();
        prefix.mark(first_index_of_pass(current_pass), last_index_of_pass(current_pass));
        prefix.predecide();
    }

    bool legal_pass(int pass) const { return pass >= 1 && pass <= passes; }

public:
    Partition(const PrefixProbabilities& prob_, int passes_)
        : prob(prob_),
          passes(passes_),
          prefix(prob.get_depth()),
          start_of_pass(new int[passes+1]),
          max_probes_in_any_pass(0)
    {
        pt_assert(passes >= 1 && passes <= max_allowed_passes());
        calculate_pass_starts();
        reset();
    }
    Partition(const Partition& other)
        : prob(other.prob),
          passes(other.passes),
          current_pass(other.current_pass),
          prefix(other.prefix),
          start_of_pass(new int[passes+1]),
          max_probes_in_any_pass(other.max_probes_in_any_pass)
    {
        memcpy(start_of_pass, other.start_of_pass, sizeof(*start_of_pass)*(passes+1));
        prefix.predecide();
    }
    DECLARE_ASSIGNMENT_OPERATOR(Partition);
    ~Partition() {
        delete [] start_of_pass;
    }

    double pass_probability(int pass) const {
        return prob.of_range(first_index_of_pass(pass), last_index_of_pass(pass));
    }

    int max_allowed_passes() const { return prob.get_prefix_count(); }
    int number_of_passes() const { return passes; }
    int split_depth() const { return prob.get_depth(); }

    bool done() const { return !legal_pass(current_pass); }
    bool next() {
        ++current_pass;
        if (done()) return false;
        markPrefixes();
        return true;
    }

    void reset() {
        current_pass = 1;
        markPrefixes();
    }

    size_t estimate_probes_for_pass(int pass, size_t overall_base_count) const {
        pt_assert(legal_pass(pass));
        return size_t(pass_probability(pass)*overall_base_count+0.5);
    }
    size_t estimate_max_probes_for_any_pass(size_t overall_base_count) const {
        if (max_probes_in_any_pass == 0) { // lazy eval
            for (int p = 1; p <= passes; ++p) {
                size_t probes = estimate_probes_for_pass(p, overall_base_count);
                if (probes>max_probes_in_any_pass) max_probes_in_any_pass = probes;
            }
            pt_assert(max_probes_in_any_pass);
        }
        return max_probes_in_any_pass;
    }
    size_t estimate_max_kb_for_any_pass(size_t overall_base_count) const {
        return estimate_stage1_memusage_kb(overall_base_count, estimate_max_probes_for_any_pass(overall_base_count));
    }

    bool contains(const char *probe) const {
        return prefix.isMarked(probe);
    }
};

inline size_t max_probes_for_passes(const PrefixProbabilities& prob, int passes_wanted, size_t overall_base_count) {
    return Partition(prob, passes_wanted).estimate_max_probes_for_any_pass(overall_base_count);
}
inline size_t max_kb_for_passes(const PrefixProbabilities& prob, int passes_wanted, size_t overall_base_count) {
    return estimate_stage1_memusage_kb(overall_base_count, max_probes_for_passes(prob, passes_wanted, overall_base_count));
}

#else
#error PT_partition.h included twice
#endif // PT_PARTITION_H
