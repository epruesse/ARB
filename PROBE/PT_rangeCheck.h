// ============================================================= //
//                                                               //
//   File      : PT_rangeCheck.hxx                               //
//   Purpose   : Check whether probe is inside region            //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in March 2011   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef PT_RANGECHECK_HXX
#define PT_RANGECHECK_HXX

#ifndef PROBE_TREE_H
#include "probe_tree.h"
#endif
#ifndef _GLIBCXX_MAP
#include <map>
#endif

typedef std::map<int, int>   apos_cache;
typedef apos_cache::iterator apos_iter;

class Range {
    int start; // -1 or minimum absolute position
    int end;   // -1 or maximum absolute position
    int probe_len; // length of checked probe

    mutable const AbsLoc *curr_match;
    mutable apos_cache    cache;

    int start_pos() const { return curr_match->get_abs_pos(); }
    int min_end_pos() const { return start_pos()+probe_len-1; } // min abs. probe-end-pos

    int calc_max_abs_pos() const;
    int max_abs_pos() const {
        apos_iter found = cache.find(curr_match->get_name());
        if (found != cache.end()) return found->second;

        return cache[curr_match->get_name()] = calc_max_abs_pos();
    }

    bool starts_before_start() const { return start != -1 && start_pos() < start; }
    bool ends_behind_end() const {
        return end != -1 &&
            (min_end_pos() > end || // cheap check
             start > max_abs_pos()); // expensive check
    }

public:
    Range(int start_, int end_, int probe_len_)
        : start(start_),
          end(end_),
          probe_len(probe_len_),
          curr_match(NULL)
    {}
    Range(const Range& other)
        : start(other.start),
          end(other.end),
          probe_len(other.probe_len),
          curr_match(other.curr_match)
    {}
    DECLARE_ASSIGNMENT_OPERATOR(Range);

    bool contains(const AbsLoc& match) const {
        // check if found 'match' of probe is inside the Range
        curr_match  = &match;
        bool inside = !starts_before_start() && !ends_behind_end();
        curr_match  = NULL;
        return inside;
    }
};

#else
#error PT_rangeCheck.hxx included twice
#endif // PT_RANGECHECK_HXX
