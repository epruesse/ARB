// =============================================================== //
//                                                                 //
//   File      : PT_family.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "PT_rangeCheck.h"
#include "pt_prototypes.h"

#include <struct_man.h>
#include <PT_server_prototypes.h>
#include "PT_global_defs.h"
#include "PT_complement.h"

#include <arbdbt.h>

#include <algorithm>
#include <vector>
#include <map>

// overloaded functions to avoid problems with type-punning:
inline void aisc_link(dll_public *dll, PT_family_list *family)   { aisc_link(reinterpret_cast<dllpublic_ext*>(dll), reinterpret_cast<dllheader_ext*>(family)); }

struct TraversalHitLimit {
    int id;    // "unique" for each traversal
    int limit; // max hits allowed to each target seq for this traversal

    TraversalHitLimit(int id_, int limit_)
        : id(id_), limit(limit_)
    { pt_assert(limit>0); }
};

class HitCounter {
    int    trav_id;    // current traversal id
    int    trav_hits;  // how many hits have been counted during traversal 'trav_id' ?
    int    count;      // Counter for matches
    double rel_count;  // match_count / (seqlen - oligolen + 1). seqlen depends on RelativeScoreScaling

public:
    HitCounter() : trav_id(-1), trav_hits(0), count(0), rel_count(0.0) {}

    void inc(const TraversalHitLimit& traversal) {
        if (traversal.id != trav_id) { // first hit during this traversal
            trav_id  = traversal.id;
            trav_hits = 1; // reset
            count++;
        }
        else {
            if (trav_hits<traversal.limit) {
                trav_hits++;
                count++;
            }
        }
    }
    void calc_rel_match(int max_poss_matches) {
        rel_count = max_poss_matches>0 ? double(count)/max_poss_matches : 0;
    }

    int cmp_abs(const HitCounter& other) const { return count - other.count; }
    int cmp_rel(const HitCounter& other) const { return double_cmp(rel_count, other.rel_count); }

    int get_match_count() const { return count; }
    const double& get_rel_match_count() const { return rel_count; }
};

class FamilyStat : virtual Noncopyable {
    size_t                size;
    HitCounter           *famstat;
    TraversalHitLimit     trav_info;
    RelativeScoreScaling  scaling;

public:
    FamilyStat(size_t size_, RelativeScoreScaling scaling_)
        : size(size_),
          famstat(new HitCounter[size]),
          trav_info(-1, 1),
          scaling(scaling_)
    {}
    ~FamilyStat() { delete [] famstat; }

    void calc_rel_matches(int oligo_len, int sequence_length)  {
        for (size_t i = 0; i < size; i++) {
            int full_length = 0;
            switch (scaling) {
                case RSS_SOURCE:   full_length = sequence_length; break;
                case RSS_TARGET:   full_length = psg.data[i].get_size(); break;
                case RSS_BOTH_MIN: full_length = std::min(psg.data[i].get_size(), sequence_length); break; 
                case RSS_BOTH_MAX: full_length = std::max(psg.data[i].get_size(), sequence_length); break;
            }
            int max_poss_matches = full_length - oligo_len + 1; // @@@ wrong if target range is used!

            famstat[i].calc_rel_match(max_poss_matches);
        }
    }

    const HitCounter& hits(size_t idx) const { pt_assert(idx<size); return famstat[idx]; }

    void limit_hits_for_next_traversal(int hit_limit) {
        trav_info.id++;
        trav_info.limit = hit_limit;
    }
    void count_match(size_t idx) { famstat[idx].inc(trav_info); }

    int cmp_abs(int a, int b) const { int cmp = famstat[a].cmp_abs(famstat[b]); return cmp ? cmp : a-b; }
    int cmp_rel(int a, int b) const { int cmp = famstat[a].cmp_rel(famstat[b]); return cmp ? cmp : a-b; }
};

class PT_Traversal {
    static Range range;

    const char *oligo;
    int         height;
    int         needed_positions;
    int         accept_mismatches;
    
    FamilyStat& fam_stat;

    void count_match(const AbsLoc& match) const {
        if (range.contains(match)) {
            fam_stat.count_match(match.get_name());
        }
    }

    bool at_end() const { return *oligo == PT_QU; }

    bool too_many_mismatches() const { return accept_mismatches<0; }

    bool did_match() const { return needed_positions <= 0 && !too_many_mismatches(); }
    bool need_match() const { return needed_positions > 0 && !too_many_mismatches(); }
    bool match_possible() const { return need_match() && !at_end(); }

    void match_one_char(char c) {
        pt_assert(match_possible()); // avoid unneeded calls 
        
        if (*oligo++ != c) accept_mismatches--;
        needed_positions--;
        height++;
    }

    void match_rest_and_mark(const ReadableDataLoc& loc) {
        do match_one_char(loc[height]); while (match_possible());
        if (did_match()) count_match(loc);
    }

    void mark_all(POS_TREE2 *pt) const;
    inline void mark_chain_or_leaf(POS_TREE2 *pt) const;

public:

    static void restrictMatchesToRegion(int start, int end, int oligo_len) {
        range  = Range(start, end, oligo_len);
    }
    static void unrestrictMatchesToRegion() {
        range  = Range(-1, -1, -1);
    }

    PT_Traversal(const char *oligo_, int needed_positions_, int accept_mismatches_, FamilyStat& fam_stat_)
        : oligo(oligo_),
          height(0),
          needed_positions(needed_positions_),
          accept_mismatches(accept_mismatches_),
          fam_stat(fam_stat_)
    { }

    void mark_matching(POS_TREE2 *pt) const;

    int operator()(const DataLoc& loc) const {
        //! Increment match_count for matched postree-tips
        if (did_match()) count_match(loc);
        else if (match_possible()) {
            PT_Traversal(*this).match_rest_and_mark(ReadableDataLoc(loc)); // @@@ EXPENSIVE_CONVERSION
        }
        return 0;
    }
    int operator()(const AbsLoc& loc) const {
        //! Increment match_count for matched postree-tips
        if (did_match()) count_match(loc);
        else if (match_possible()) {
            PT_Traversal(*this).match_rest_and_mark(ReadableDataLoc(DataLoc(loc))); // @@@ VERY EXPENSIVE_CONVERSION (2)
        }
        return 0;
    }
};

Range PT_Traversal::range(-1, -1, -1);

inline void PT_Traversal::mark_chain_or_leaf(POS_TREE2 *pt) const {
    pt_assert(pt);
    switch (pt->get_type()) {
        case PT2_LEAF:
            (*this)(DataLoc(pt));
            break;

        case PT2_CHAIN: {
            ChainIteratorStage2 entry(pt);
            while (entry) {
                (*this)(entry.at());
                ++entry;
            }
            break;
        }
        case PT2_NODE:
            pt_assert(0); // not called with chain or leaf
            break;
    }
}

void PT_Traversal::mark_matching(POS_TREE2 *pt) const {
    //! Traverse pos(sub)tree through matching branches and increment 'match_count'
    pt_assert(pt);
    pt_assert(!too_many_mismatches());
    pt_assert(!did_match()); 

    if (pt->is_node()) {
        for (int base = PT_N; base < PT_BASES; base++) {
            POS_TREE2 *pt_son = PT_read_son(pt, (PT_base)base);
            if (pt_son && !at_end()) {
                PT_Traversal sub(*this);
                sub.match_one_char(base);
                if (!sub.too_many_mismatches()) {
                    if (sub.did_match()) sub.mark_all(pt_son);
                    else sub.mark_matching(pt_son);
                }
            }
        }
    }
    else {
        mark_chain_or_leaf(pt);
    }
}

void PT_Traversal::mark_all(POS_TREE2 *pt) const {
    pt_assert(pt);
    pt_assert(!too_many_mismatches());
    pt_assert(did_match());

    if (pt->is_node()) {
        for (int base = PT_N; base < PT_BASES; base++) {
            POS_TREE2 *pt_son = PT_read_son(pt, (PT_base)base);
            if (pt_son) mark_all(pt_son);
        }
    }
    else {
        mark_chain_or_leaf(pt);
    }
}

struct oligo_cmp_abs {
    const FamilyStat& fam_stat;
    oligo_cmp_abs(const FamilyStat& fam_stat_) : fam_stat(fam_stat_) {}
    bool operator()(int a, int b) { return fam_stat.cmp_abs(a, b) > 0; } // biggest scores 1st
};

struct oligo_cmp_rel {
    const FamilyStat& fam_stat;
    oligo_cmp_rel(const FamilyStat& fam_stat_) : fam_stat(fam_stat_) {}
    bool operator()(int a, int b) { return fam_stat.cmp_rel(a, b) > 0; } // biggest scores 1st
};

static int make_PT_family_list(PT_family *ffinder, const FamilyStat& famStat) {
    //!  Make sorted list of family members

    // destroy old list
    while (ffinder->fl) destroy_PT_family_list(ffinder->fl);

    // Sort the data
    std::vector<int> sorted;
    sorted.resize(psg.data_count);

    size_t matching_results = psg.data_count;
    if (ffinder->min_score == 0) { // collect all hits
        for (int i = 0; i < psg.data_count; i++) sorted[i] = i;
    }
    else {
        int j = 0;
        if (ffinder->sort_type == 0) { // filter by absolut score
            double min_score = ffinder->min_score; 
            for (int i = 0; i < psg.data_count; i++) {
                const HitCounter& ps = famStat.hits(i);
                if (ps.get_match_count() >= min_score) {
                    sorted[j++] = i;
                }
            }
        }
        else { // filter by relative score
            double min_score_rel = double(ffinder->min_score)/100.0;
            for (int i = 0; i < psg.data_count; i++) {
                const HitCounter& ps = famStat.hits(i);
                if (ps.get_rel_match_count()>min_score_rel) {
                    sorted[j++] = i;
                }
            }
        }
        matching_results = j;
    }

    bool sort_all = ffinder->sort_max == 0 || ffinder->sort_max >= int(matching_results);

    if (ffinder->sort_type == 0) { // sort by absolut score
        if (sort_all) {
            std::sort(sorted.begin(), sorted.end(), oligo_cmp_abs(famStat));
        }
        else {
            std::partial_sort(sorted.begin(), sorted.begin() + ffinder->sort_max, sorted.begin() + matching_results, oligo_cmp_abs(famStat));
        }
    }
    else { // sort by relative score
        if (sort_all) {
            std::sort(sorted.begin(), sorted.begin() + psg.data_count, oligo_cmp_rel(famStat));
        }
        else {
            std::partial_sort(sorted.begin(), sorted.begin() + ffinder->sort_max, sorted.begin() + matching_results, oligo_cmp_rel(famStat));
        }
    }

    // build new list
    int real_hits = 0;

    int end = (sort_all) ? matching_results : ffinder->sort_max;
    for (int i = 0; i < end; i++) {
        probe_input_data& pid = psg.data[sorted[i]];
        const HitCounter& ps  = famStat.hits(sorted[i]);

        if (ps.get_match_count() != 0) {
            PT_family_list *fl = create_PT_family_list();

            fl->name        = ARB_strdup(pid.get_shortname());
            fl->matches     = ps.get_match_count();
            fl->rel_matches = ps.get_rel_match_count();

            aisc_link(&ffinder->pfl, fl);
            real_hits++;
        }
    }

    ffinder->list_size = real_hits;

    return 0;
}

inline bool contains_ambiguities(char *oligo, int oligo_len) {
    //! Check the oligo for ambiguities
    for (int i = 0; i < oligo_len; i++) {
        if (!is_std_base(oligo[i])) {
            return true;
        }
    }
    return false;
}

class oligo_comparator {
    int oligo_len;
public:
    oligo_comparator(int len) : oligo_len(len) {}
    bool operator()(const char *p1, const char *p2) const {
        bool isless = false;
        for (int o = 0; o<oligo_len; ++o) {
            if (p1[o] != p2[o]) {
                isless = p1[o]<p2[o];
                break;
            }
        }
        return isless;
    }
};

typedef std::map<const char *, int, oligo_comparator> OligoMap;
typedef OligoMap::const_iterator                      OligoIter;

class OligoRegistry {
    OligoMap oligos;
public:
    OligoRegistry(int oligo_len)
        : oligos(oligo_comparator(oligo_len))
    {}
    void add(const char *seq) {
        OligoMap::iterator found = oligos.find(seq);
        if (found == oligos.end()) oligos[seq] = 1;
        else found->second++;
    }
    OligoIter begin() { return oligos.begin(); }
    OligoIter end() { return oligos.end(); }
};

int find_family(PT_family *ffinder, bytestring *species) {
    //! make sorted list of family members of species

    int oligo_len = ffinder->pr_len;

    if (oligo_len<1) {
        freedup(ffinder->ff_error, "minimum oligo length is 1");
    }
    else {
        int mismatch_nr = ffinder->mis_nr;
        int complement  = ffinder->complement; // any combination of: 1 = forward, 2 = reverse, 4 = reverse-complement, 8 = complement

        char *sequence     = species->data; // sequence data passed by caller
        int   sequence_len = probe_compress_sequence(sequence, species->size-1);

        bool use_all_oligos = ffinder->only_A_probes == 0;

        PT_Traversal::restrictMatchesToRegion(ffinder->range_start, ffinder->range_end, oligo_len);

        FamilyStat famStat(psg.data_count, RelativeScoreScaling(ffinder->rel_scoring));

        char *seq[4];
        int   seq_count = 0;

        // Note: loop-logic depends on order of ../AWTC/awtc_next_neighbours.hxx@FF_complement_dep
        for (int cmode = 1; cmode <= 8; cmode *= 2) {
            switch (cmode) {
                case FF_FORWARD:
                    break;
                case FF_REVERSE:
                case FF_COMPLEMENT:
                    reverse_probe(sequence, sequence_len); // build reverse sequence
                    break;
                case FF_REVERSE_COMPLEMENT:
                    complement_probe(sequence, sequence_len); // build complement sequence
                    break;
            }

            if ((complement&cmode) != 0) {
                char *s = ARB_alloc<char>(sequence_len+1);

                memcpy(s, sequence, sequence_len);
                s[sequence_len] = 0;

                seq[seq_count++] = s;
            }
        }

        OligoRegistry occurring_oligos(oligo_len);

        for (int s = 0; s<seq_count; s++) {
            char *last_oligo = seq[s]+sequence_len-oligo_len;
            for (char *oligo = seq[s]; oligo < last_oligo; ++oligo) {
                if (use_all_oligos || oligo[0] == PT_A) {
                    if (!contains_ambiguities(oligo, oligo_len)) {
                        occurring_oligos.add(oligo);
                    }
                }
            }
        }

        for (OligoIter o = occurring_oligos.begin(); o != occurring_oligos.end(); ++o)  {
            const char *oligo       = o->first;
            int         occur_count = o->second;

            famStat.limit_hits_for_next_traversal(occur_count);
            PT_Traversal(oligo, oligo_len, mismatch_nr, famStat).mark_matching(psg.TREE_ROOT2());
        }

        famStat.calc_rel_matches(ffinder->pr_len, sequence_len);
        make_PT_family_list(ffinder, famStat);

        for (int s = 0; s<seq_count; s++) {
            free(seq[s]);
        }

        PT_Traversal::unrestrictMatchesToRegion();
    }
    free(species->data);
    return 0;
}

