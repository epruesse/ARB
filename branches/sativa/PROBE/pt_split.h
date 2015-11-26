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

inline double get_bond_val(const PT_bond *bond, int probe, int seq) {
    pt_assert(is_std_base(probe));
    pt_assert(is_std_base(seq));
    int idx = ((probe-int(PT_A))*4) + seq-(int)PT_A;
    pt_assert(idx >= 0 && idx < 16);
    return bond[idx].val;
}

class MaxBond {
    double max_bond[PT_BASES];
protected:
    MaxBond() {}
public:
    MaxBond(const PT_bond *bond) {
        for (int base = PT_A; base < PT_BASES; ++base) {
            pt_assert(is_std_base(base));
            max_bond[base] = get_bond_val(bond, get_complement(base), base);
        }
    }

    double get_max_bond(int base) const {
        pt_assert(is_std_base(base));
        return max_bond[base];
    }
};

class MismatchWeights : public MaxBond {
    double weight[PT_BASES][PT_BASES];

    double get_simple_wmismatch(const PT_bond *bonds, char probe, char seq) {
        double max_bind = get_max_bond(probe);
        double new_bind = get_bond_val(bonds, get_complement(probe), seq);
        return max_bind - new_bind;
    }

    void init(const PT_bond *bonds) {
        for (int probe = PT_A; probe < PT_BASES; ++probe) {
            double sum = 0.0;
            for (int seq = PT_A; seq < PT_BASES; ++seq) {
                sum += weight[probe][seq] = get_simple_wmismatch(bonds, probe, seq);
            }
            weight[probe][PT_N] = sum/4.0;
        }
        for (int seq = PT_N; seq < PT_BASES; ++seq) {  // UNCHECKED_LOOP (doesnt matter)
            double sum = 0.0;
            for (int probe = PT_A; probe < PT_BASES; ++probe) {
                sum += weight[probe][seq];
            }
            weight[PT_N][seq] = sum/4.0;
        }

        for (int i = PT_N; i<PT_BASES; ++i) {
            weight[PT_QU][i] = weight[PT_N][i];
            weight[i][PT_QU] = weight[i][PT_N];
        }
        weight[PT_QU][PT_QU] = weight[PT_N][PT_N];
    }

public:
    MismatchWeights(const PT_bond *bonds) : MaxBond(bonds) { init(bonds); }
    double get(int probe, int seq) const {
        pt_assert(is_valid_base(probe) && is_valid_base(seq));
        return weight[probe][seq];
    }

};


class Splits : public MaxBond {
    bool valid;
    double split[PT_BASES][PT_BASES];

    double calc_split(const PT_local *locs, char base, char ref) {
        double max_bind = get_max_bond(base);
        double new_bind = get_bond_val(locs->bond, get_complement(base), ref);

        if (new_bind < locs->split)
            return new_bind-max_bind; // negative values indicate split
        // this mismatch splits the probe in several domains
        return max_bind-new_bind;
    }

public:
    Splits() : valid(false) {}
    Splits(const PT_local *locs) : MaxBond(locs->bond), valid(true) {
        for (int base = PT_A; base < PT_BASES; ++base) {
            for (int ref = PT_A; ref < PT_BASES; ++ref) {
                split[base][ref] = calc_split(locs, base, ref);
            }
        }
    }

    double check(char base, char ref) const {
        pt_assert(valid);

        pt_assert(is_std_base(base));
        pt_assert(is_std_base(ref));

        return split[safeCharIndex(base)][safeCharIndex(ref)];
    }
};

#else
#error pt_split.h included twice
#endif // PT_SPLIT_H
