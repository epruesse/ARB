// ================================================================ //
//                                                                  //
//   File      : pt_split.h                                         //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in December 2012   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef PT_SPLIT_H
#define PT_SPLIT_H

#ifndef PROBE_H
#include "probe.h"
#endif
#ifndef PT_COMPLEMENT_H
#include "PT_complement.h"
#endif

class Splits {
    bool valid;
    double split[PT_BASES][PT_BASES];

    static double calc_split(const PT_local *locs, char base, char ref) {
        pt_assert(is_valid_base(base) && is_valid_base(ref));

        int    complement = get_complement(base);
        double max_bind   = locs->bond[(complement-(int)PT_A)*4 + base-(int)PT_A].val;
        double new_bind   = locs->bond[(complement-(int)PT_A)*4 + ref-(int)PT_A].val;

        if (new_bind < locs->split)
            return new_bind-max_bind; // negative values indicate split
        // this mismatch splits the probe in several domains
        return max_bind-new_bind;
    }

public:
    Splits() : valid(false) {}
    Splits(const PT_local *locs) : valid(true) {
        for (int base = PT_QU; base < PT_BASES; ++base) {
            for (int ref = PT_QU; ref < PT_BASES; ++ref) {
                split[base][ref] = calc_split(locs, base, ref);
            }
        }
    }

    double check(char base, char ref) const {
        pt_assert(valid);
        pt_assert(is_valid_base(base) && is_valid_base(ref));
        return split[safeCharIndex(base)][safeCharIndex(ref)];
    }
};

#else
#error pt_split.h included twice
#endif // PT_SPLIT_H
