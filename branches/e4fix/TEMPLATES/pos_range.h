// =============================================================== //
//                                                                 //
//   File      : pos_range.h                                       //
//   Purpose   : range of positions (e.g. part of seq)             //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2011   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef POS_RANGE_H
#define POS_RANGE_H

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

class PosRange {
    int start_pos;
    int end_pos;

public:
    PosRange() : start_pos(0), end_pos(-1) {} // unlimited range (e.g. whole sequence, w/o knowing it's explicit length)
    PosRange(int from, int to) : start_pos(from), end_pos(to) {
        arb_assert(from >= 0);
        arb_assert(from <= to || to == -1);
    }

    int start() const { return start_pos; }
    int end() const {
        arb_assert(end_pos != -1);
        return end_pos;
    }

    size_t size() const { return end()-start()+1; }

    bool is_full_range() const { return start_pos == 0 && end_pos == -1; }
    bool is_restricted() const { return !is_full_range(); }

    void make_explicit(size_t max_length) {
        arb_assert(is_full_range()); // already have specified range
        end_pos = max_length-1;
    }

};


#else
#error pos_range.h included twice
#endif // POS_RANGE_H
