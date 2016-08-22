// =============================================================== //
//                                                                 //
//   File      : AP_pro_a_nucs.hxx                                 //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AP_PRO_A_NUCS_HXX
#define AP_PRO_A_NUCS_HXX

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif


enum AP_BASES {
    AP_A   = 1,
    AP_C   = 2,
    AP_G   = 4,
    AP_T   = 8,
    AP_S   = 16,                // Space (GAP)
    AP_N   = 31,
    AP_MAX = 32
};

struct arb_r2a_pro_2_nucs : virtual Noncopyable {
    struct arb_r2a_pro_2_nucs *next;
    char                       nucbits[3]; // bitsets of nucs

    arb_r2a_pro_2_nucs();
    ~arb_r2a_pro_2_nucs();
};

struct arb_r2a_pro_2_nuc : virtual Noncopyable {
    char single_pro;
    char tri_pro[3];            // null terminated (because of index)
    int  index;                 // < 0x007fffff

    struct arb_r2a_pro_2_nucs *nucs;

    arb_r2a_pro_2_nuc();
    ~arb_r2a_pro_2_nuc();
};

struct AWT_PDP { // distance definition for one protein
    long patd[3];               // proteins at dist
    // every bit in patd[x] represents one protein
    // bit in patd[0] is set = > distance == 0
    // bit in patd[1] is set = > distance <= 1
    // bit in patd[2] is set = > distance <= 2

    char nucbits[3];            // bitsets of nucs
};

class AWT_translator;

class AWT_distance_meter : virtual Noncopyable {
    AWT_PDP *dist_[64];         // sets of proteins with special distance [64 > max_aa

    long transform07[256];       // like dist.patd[1] but for bits 0-7
    long transform815[256];
    long transform1623[256];

public:
    AWT_distance_meter(const AWT_translator *translator);
    ~AWT_distance_meter();

    const AWT_PDP *getDistance(int idx) const { return dist_[idx]; }
    AWT_PDP *getDistance(int idx)  { return dist_[idx]; }
};


class AWT_translator : virtual Noncopyable {
private:
    mutable AWT_distance_meter *distance_meter; // (mutable to allow lazy-evaluation)

    int                code_nr;
    GB_HASH           *t2i_hash;         // hash table trin >> singlepro
    arb_r2a_pro_2_nuc *s2str[256];       // singlecode protein >> dna ...
    long              *pro_2_bitset;     //
    char              *nuc_2_bitset;     // dna to
    unsigned char      index_2_spro[64]; // 64 > max_aa

    int realmax_aa;             // number of real AA + stop codon
    int max_aa;                 // plus ambiguous codes

    void build_table(unsigned char pbase, const char *tri_pro, const char *nuc);
    long *create_pro_to_bits() const;

public:

    AWT_translator(int arb_protein_code_nr);
    ~AWT_translator();

    const AWT_distance_meter *getDistanceMeter() const;
    AWT_distance_meter *getDistanceMeter() {
        return const_cast<AWT_distance_meter*>(const_cast<const AWT_translator*>(this)->getDistanceMeter());
    }

    int CodeNr() const { return code_nr; }
    const GB_HASH *T2iHash() const { return t2i_hash; }
    const arb_r2a_pro_2_nuc *S2str(int index) const { return s2str[index]; }
    const arb_r2a_pro_2_nuc * const *S2strArray() const { return s2str; }
    const long * Pro2Bitset() const { return pro_2_bitset; }
    unsigned char Index2Spro(int index) const { return index_2_spro[index]; }
    int MaxAA() const { return max_aa; }
    int RealmaxAA() const { return realmax_aa; }
};

#define AWAR_PROTEIN_TYPE "nt/protein_codon_type"

char *AP_create_dna_to_ap_bases(); // create dna 2 nuc_bitset

// ------------------------------

int AWT_default_protein_type(GBDATA *gb_main = 0); // returns protein code selected in AWAR_PROTEIN_TYPE

AWT_translator *AWT_get_translator(int code_nr);              // use explicit protein code
AWT_translator *AWT_get_user_translator(GBDATA *gb_main = 0); // uses user setting for protein code from AWAR_PROTEIN_TYPE
// AWAR_PROTEIN_TYPE has to exist; the first call of AWT_get_user_translator needs 'gb_main'!=0

#else
#error AP_pro_a_nucs.hxx included twice
#endif // AP_PRO_A_NUCS_HXX
