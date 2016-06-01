// ================================================================= //
//                                                                   //
//   File      : iupac.h                                             //
//   Purpose   :                                                     //
//                                                                   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef IUPAC_H
#define IUPAC_H

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif


namespace iupac {
    // --------------
    //      amino

    enum Amino_Group {
        AA_GROUP_NONE = 0,

        AA_GROUP_ALPHA,
        AA_GROUP_BETA,
        AA_GROUP_GAMMA,
        AA_GROUP_DELTA,
        AA_GROUP_EPSILON,
        AA_GROUP_ZETA,

        AA_GROUP_ILLEGAL,

        AA_GROUP_COUNT = AA_GROUP_ILLEGAL, // count of real groups plus none-"group"
    };
    Amino_Group get_amino_group_for(char aa);
    char get_amino_consensus_char(Amino_Group ag);

    // -------------
    //      Nucs

    struct Nuc_Group {
        const char * const members; // contains members of nuc IUPAC group (NULL if no members)
        size_t const       count;   // no of members (1 for 'N'!)
    };

    extern const Nuc_Group nuc_group[26][2]; // second index: [0] uses T, [1] uses U

    inline int to_index(char c) { // calculate index for nuc_group
        arb_assert(c>='A' && c<='Z');
        return c-'A';
    }
    // @@@ create accessor and hide nuc_group

    char combine(char c1, char c2, GB_alignment_type ali); // nucleotides only
    char encode(const char bases[], GB_alignment_type aliType);

    // -------------
    //      Both

    const char* decode(char iupac, GB_alignment_type aliType, bool decode_amino_iupac_groups);
};

#else
#error iupac.h included twice
#endif // IUPAC_H
