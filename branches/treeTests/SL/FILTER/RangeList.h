// ============================================================= //
//                                                               //
//   File      : RangeList.h                                     //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in April 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef RANGELIST_H
#define RANGELIST_H

#ifndef POS_RANGE_H
#include <pos_range.h>
#endif
#ifndef _GLIBCXX_SET
#include <set>
#endif

#define rl_assert(cond) arb_assert(cond)

struct less_DistinctPosRange {
    bool operator()(const PosRange& r1, const PosRange& r2) const {
        // true if r1 is less than r2
        // overlapping and adjacent ranges are never less than each other!

        rl_assert(!r1.is_empty());
        rl_assert(!r2.is_empty());

        return r1.end()<(r2.start()-1); 
    }
};


class RangeList {
    typedef std::set<PosRange, less_DistinctPosRange> range_set;

    range_set ranges;

    void add_combined(const PosRange& range, range_set::const_iterator found);

public:

    typedef range_set::const_iterator         iterator;
    typedef range_set::const_reverse_iterator reverse_iterator;

    size_t size() const { return ranges.size(); }

    iterator begin() const { return ranges.begin(); }
    iterator end() const { return ranges.end(); }

    reverse_iterator rbegin() const { return ranges.rbegin(); }
    reverse_iterator rend() const { return ranges.rend(); }

    RangeList inverse(ExplicitRange versus);

    bool empty() const { return ranges.empty(); }

    void add(const PosRange& range) {
        if (!range.is_empty()) {
            iterator found = ranges.find(range);
            if (found != ranges.end()) add_combined(range, found); // overlapping or adjacent to range
            else ranges.insert(range);
        }
    }
};

RangeList build_RangeList_from_string(const char *SAI_data, const char *set_bytes, bool invert);

#else
#error RangeList.h included twice
#endif // RANGELIST_H
