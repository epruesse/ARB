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

    PT_base low, high;
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

    PrefixIterator(PT_base low_, PT_base high_, int len_)
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
    size_t max_length() const { return len; }

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
    pt_assert(base >= PT_QU && base < PT_BASES);
    static double bprob[PT_BASES] = {
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

    int find_index_near_leftsum(double wanted_leftsum) const {
        // returned index is in range [0 ... prefixes]

        int    best_idx  = -1;
        double best_diff = 2.0;

        for (int i = 0; i <= prefixes; ++i) {
            double diff = fabs(left_of(i)-wanted_leftsum);
            if (diff<best_diff) {
                best_diff = diff;
                best_idx  = i;
            }
        }

        pt_assert(best_idx >= 0);
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
          start_of_pass(new int[passes+1])
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
          start_of_pass(new int[passes+1])
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
        size_t max_probes = 0;
        for (int p = 1; p <= passes; ++p) {
            size_t probes = estimate_probes_for_pass(p, overall_base_count);
            if (probes>max_probes) max_probes = probes;
        }
        return max_probes;
    }
    size_t estimate_max_kb_for_any_pass(size_t overall_base_count) const {
        return estimate_memusage_kb(estimate_max_probes_for_any_pass(overall_base_count));
    }

    bool contains(const char *probe) const {
        return prefix.isMarked(probe);
    }
};

inline size_t max_probes_for_passes(const PrefixProbabilities& prob, int passes_wanted, size_t overall_base_count) {
    return Partition(prob, passes_wanted).estimate_max_probes_for_any_pass(overall_base_count);
}
inline size_t max_kb_for_passes(const PrefixProbabilities& prob, int passes_wanted, size_t overall_base_count) {
    return estimate_memusage_kb(max_probes_for_passes(prob, passes_wanted, overall_base_count));
}

#else
#error PT_partition.h included twice
#endif // PT_PARTITION_H
