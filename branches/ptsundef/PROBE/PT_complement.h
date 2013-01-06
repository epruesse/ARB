// =============================================================== //
//                                                                 //
//   File      : PT_complement.h                                   //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in January 2013   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef PT_COMPLEMENT_H
#define PT_COMPLEMENT_H

#ifndef PT_PROTOTYPES_H
#include "pt_prototypes.h"
#endif

class Complement {
    int complement[256];

    static int calc_complement(int base); // DO NOT INLINE! (workaround a gcc-4.4.3-bug; see [9355] for details)

public:
    Complement() { for (int i = 0; i<256; ++i) complement[i] = calc_complement(i); }
    int get(int base) const { return complement[base]; }
};

inline int get_complement(int base) {
    static Complement c;
    pt_assert(base >= 0 && base <= 255);
    return c.get(base);
}

inline void complement_probe(char *Probe, int len) {
    for (int i = 0; i<len; i++) {
        Probe[i] = get_complement(Probe[i]);
    }
}

#else
#error PT_complement.h included twice
#endif // PT_COMPLEMENT_H
