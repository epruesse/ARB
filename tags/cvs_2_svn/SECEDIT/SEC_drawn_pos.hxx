// =============================================================== //
//                                                                 //
//   File      : SEC_drawn_pos.hxx                                 //
//   Purpose   : store all drawn positions                         //
//   Time-stamp: <Thu Aug/23/2007 08:45 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2007    //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef SEC_DRAWN_POS_HXX
#define SEC_DRAWN_POS_HXX

#ifndef _CPP_MAP
#include <map>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef AW_POSITION_HXX
#include <aw_position.hxx>
#endif

using namespace AW;

typedef std::map<size_t, Position> PosMap;

class SEC_drawn_positions : Noncopyable {
    PosMap drawnAt;

public:
    void clear() { drawnAt.clear(); }
    void announce(size_t abs, const Position& drawn) { drawnAt[abs] = drawn; }

    bool empty() const { return drawnAt.empty(); }
    const PosMap::const_iterator begin() const { return drawnAt.begin(); }
    const PosMap::const_iterator end() const { return drawnAt.end(); }
    
    const Position *drawn_at(size_t abs) const {
        PosMap::const_iterator found = drawnAt.find(abs);
        return (found == drawnAt.end()) ? 0 : &(found->second);
    }

    const Position& drawn_before(size_t abspos, size_t *before_abs) const {
        sec_assert(!empty());
        PosMap::const_iterator found = drawnAt.lower_bound(abspos); // first pos which is >= abs

        if (found == drawnAt.end() || --found == drawnAt.end()) {
            found = ++drawnAt.rbegin().base();
        }

        if (before_abs) *before_abs = found->first;
        return *&found->second;
    }

    const Position& drawn_after(size_t abspos, size_t *after_abs) const {
        sec_assert(!empty());
        PosMap::const_iterator found = drawnAt.upper_bound(abspos); // first pos which is > abspos
        
        if (found == drawnAt.end()) { // no position drawn behind abs
            found = drawnAt.begin(); // wrap to start
        }
        
        if (after_abs) *after_abs = found->first;
        return *&found->second;
    }

    const Position& drawn_at_or_after(size_t abspos) const {
        const Position *at = drawn_at(abspos);
        return at ? *at : drawn_after(abspos, 0);
    }

};


#else
#error SEC_drawn_pos.hxx included twice
#endif // SEC_DRAWN_POS_HXX
