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
#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif
#ifndef _GLIBCXX_ALGORITHM
#include <algorithm>
#endif

class PosRange { // this is a value-class (i.e. all methods apart from ctors have to be const)
    int start_pos;
    int end_pos;

public:
    PosRange() : start_pos(-1), end_pos(-2) {} // default is empty range
    PosRange(int From, int to)
        : start_pos(From<0 ? 0 : From)
    {
        if (to<0) end_pos = -2;
        else {
            if (start_pos>to) { // empty range
                start_pos = -1;
                end_pos = -2;
            }
            else {
                end_pos = to;
                arb_assert(start_pos <= end_pos);
            }
        }
    }
    
    // factory functions
    static PosRange empty() { return PosRange(); }
    static PosRange whole() { return from(0); }
    static PosRange from(int pos) { return PosRange(pos, -1); } static PosRange after(int pos) { return from(pos+1); }
    static PosRange till(int pos) { return PosRange(0, pos); }  static PosRange prior(int pos) { return till(pos-1); }

    int start() const {
        arb_assert(!is_empty());
        return start_pos;
    }
    int end() const {
        arb_assert(size()>0);
        arb_assert(end_pos >= 0); // @@@ remove later
        return end_pos;
    }

    int size() const { return end_pos-start_pos+1; }

    bool is_whole() const { return start_pos == 0 && end_pos<0; }
    bool is_restricted() const { return !is_whole(); }

    bool is_empty() const { return size() == 0; }
    bool is_explicit() const { return size() >= 0; } // includes empty range

    bool operator == (const PosRange& other) const { return start_pos == other.start_pos && end_pos == other.end_pos; }
    bool operator != (const PosRange& other) const { return !(*this == other); }

    void copy_corresponding_part(char *dest, const char *source, size_t source_len) const;
    char *dup_corresponding_part(const char *source, size_t source_len) const;
};

inline PosRange intersection(PosRange r1, PosRange r2) {
    if (r1.is_empty()) return r1;
    if (r2.is_empty()) return r2;

    int start = std::max(r1.start(), r2.start());
    if (r1.is_explicit()) {
        if (r2.is_explicit()) return PosRange(start, std::min(r1.end(), r2.end()));
        return PosRange(start, r1.end());
    }
    if (r2.is_explicit()) return PosRange(start, r2.end());
    return PosRange::from(start);
}

class ExplicitRange : public PosRange {

    // this ctor is private to avoid making ranges repeatedly explicit (if REALLY needed, convert range to PosRange before)
    ExplicitRange(const ExplicitRange& range, int maxlen);

public:
    ExplicitRange(const ExplicitRange& explici) : PosRange(explici) {}
    explicit ExplicitRange(const PosRange& explici) : PosRange(explici) { arb_assert(is_explicit()); }

    ExplicitRange(const PosRange& range, int maxlen)
        : PosRange(range.is_empty() || maxlen <= 0
                   ? PosRange::empty()
                   : PosRange(range.start(), range.is_explicit() ? std::min(range.end(), maxlen-1) : maxlen-1))
    { arb_assert(is_explicit()); }

    ExplicitRange(int start_, int end_)
        : PosRange(start_, end_)
    { arb_assert(is_explicit()); }

    explicit ExplicitRange(int len)
        : PosRange(0, len-1)
    { arb_assert(is_explicit()); }
};

#else
#error pos_range.h included twice
#endif // POS_RANGE_H
