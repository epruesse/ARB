// ================================================================ //
//                                                                  //
//   File      : iupac.cxx                                          //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "iupac.h"
#include <cctype>

#define awt_assert(bed) arb_assert(bed)

#define IUPAC_EMPTY "" // changed from " " to "" (2009-03-19 -- ralf)
#define ILL_CODE    char(26)

using namespace iupac;

namespace iupac {
#define _____________ {0,0} 

    const Nuc_Group nuc_group[26][2] = {
        { { "A",    1 }, { "A",    1 } }, // A
        { { "CGT",  3 }, { "CGU",  3 } }, // B
        { { "C",    1 }, { "C",    1 } }, // C
        { { "AGT",  3 }, { "AGU",  3 } }, // D
        { _____________, _____________ }, // E
        { _____________, _____________ }, // F
        { { "G",    1 }, { "G",    1 } }, // G
        { { "ACT",  3 }, { "ACU",  3 } }, // H
        { _____________, _____________ }, // I
        { _____________, _____________ }, // J
        { { "GT",   2 }, { "GU",   2 } }, // K
        { _____________, _____________ }, // L
        { { "AC",   2 }, { "AC",   2 } }, // M
        { { "ACGT", 1 }, { "ACGU", 1 } }, // N
        { _____________, _____________ }, // O
        { _____________, _____________ }, // P
        { _____________, _____________ }, // Q
        { { "AG",   2 }, { "AG",   2 } }, // R
        { { "CG",   2 }, { "CG",   2 } }, // S
        { { "T",    1 }, { "U",    1 } }, // T
        { { "T",    1 }, { "U",    1 } }, // U
        { { "ACG",  3 }, { "ACG",  3 } }, // V
        { { "AT",   2 }, { "AU",   2 } }, // W
        { _____________, _____________ }, // X
        { { "CT",   2 }, { "CU",   2 } }, // Y
        { _____________, _____________ }, // Z

        // (In each single string the nucs have to be sorted alphabetically)
    };

#undef _____________

    static const char *aminoGroupMembers[AA_GROUP_COUNT] = {
        "JOUX",   // AA_GROUP_NONE
        "AGPST",  // AA_GROUP_ALPHA
        "BDENQZ", // AA_GROUP_BETA
        "HKR",    // AA_GROUP_GAMMA
        "ILMV",   // AA_GROUP_DELTA
        "FWY",    // AA_GROUP_EPSILON
        "C",      // AA_GROUP_ZETA
    };
    
    static Amino_Group amino_group[26];

    class Setup { // setup static data in this module
        void setup_amino_group() {
            for (int c = 0; c<26; ++c) amino_group[c] = AA_GROUP_ILLEGAL;
            for (Amino_Group g = AA_GROUP_NONE; g<AA_GROUP_COUNT; g = Amino_Group(g+1)) {
                const char *members = aminoGroupMembers[g];
                for (int p = 0; members[p]; ++p) {
                    int c = members[p];
                    awt_assert(c >= 'A' && c <= 'Z');
                    amino_group[c-'A'] = g;
                }
            }
        }
    public:
        Setup() { setup_amino_group(); }
    };
    static Setup perform;

};

Amino_Group iupac::get_amino_group_for(char aa) {
    aa = toupper(aa);
    if (aa<'A' || aa>'Z') { return AA_GROUP_ILLEGAL; }
    return amino_group[aa-'A'];
}

char iupac::get_amino_consensus_char(Amino_Group ag) {
    if (ag>AA_GROUP_NONE && ag<AA_GROUP_ILLEGAL) {
        return aminoGroupMembers[ag][0];
    }
    return '?';
}

static char IUPAC_add[26][26]; // uses T
static int IUPAC_add_initialized = 0;

static void initialize_IUPAC_add()
{
    int c1, c2;

    for (c1=0; c1<26; c1++) {
        const char *decoded1 = nuc_group[c1][0].members;

        if (!decoded1) {
            for (c2=0; c2<=c1; c2++) {
                IUPAC_add[c1][c2] = ILL_CODE;
                IUPAC_add[c2][c1] = ILL_CODE;
            }
        }
        else {
            IUPAC_add[c1][c1] = char(c1);       // add char to same char

            for (c2=0; c2<c1; c2++) {
                if (strchr(decoded1, 'A'+c2)!=0) {      // char is already contained in this IUPAC
                    IUPAC_add[c1][c2] = char(c1);
                }
                else {
                    const char *decoded2 = nuc_group[c2][0].members;

                    if (!decoded2) { // char is illegal
                        IUPAC_add[c1][c2] = ILL_CODE;
                    }
                    else {
#define MAX_MIXED 5
                        char mixed[MAX_MIXED];
                        char *mp = mixed;
                        const char *d1 = decoded1;
                        const char *d2 = decoded2;

                        while (1) {
                            char z1 = *d1;
                            char z2 = *d2;

                            if (!z1 && !z2) break;

                            if (z1==z2) {
                                *mp++ = z1;
                                d1++;
                                d2++;
                            }
                            else if (!z2 || (z1 && z1<z2)) {
                                *mp++ = z1;
                                d1++;
                            }
                            else {
                                awt_assert(!z1 || (z2 && z2<z1));
                                *mp++ = z2;
                                d2++;
                            }
                        }

                        awt_assert((mp-mixed)<MAX_MIXED);
                        *mp++ = 0;

#if !defined(NDEBUG) && 0
                        printf("Mix '%s' + '%s' = '%s'\n", decoded1, decoded2, mixed);
#endif

                        int c3;

                        for (c3=0; c3<26; c3++) {
                            if (nuc_group[c3][0].members && strcmp(mixed, nuc_group[c3][0].members)==0) {
                                IUPAC_add[c1][c2] = char(c3);
                                break;
                            }
                        }

                        if (c3>=26) {
                            IUPAC_add[c1][c2] = 0;
                        }
                    }
                }

                if (IUPAC_add[c1][c2]==('U'-'A')) {
                    IUPAC_add[c1][c2] = 'T'-'A';
                }

                IUPAC_add[c2][c1] = IUPAC_add[c1][c2];
            }
        }

    }
}

char iupac::encode(const char bases[], GB_alignment_type ali)
{
    if (!IUPAC_add_initialized) {
        initialize_IUPAC_add();
        IUPAC_add_initialized = 1;
    }

    if (ali==GB_AT_AA) {
        return '?';
    }

    int i = 0;
    unsigned char c1;

    while (1) {
        c1 = bases[i++];
        if (!c1) return '-';
        if (isalpha(c1)) break;
    }

    c1 = toupper(c1)-'A';
    unsigned char c = IUPAC_add[c1][c1];

    while (c!=ILL_CODE && bases[i]) {
        if (isalpha(bases[i])) {
            int c2 = toupper(bases[i])-'A';
            c = IUPAC_add[c][c2];
        }
        i++;
    }

    if (c==ILL_CODE) {
        return '-';
    }

    c += 'A';
    if (c=='T' && ali==GB_AT_RNA) c = 'U';
    return c;
}

char iupac::combine(char c1, char c2, GB_alignment_type ali) {
    char buffer[3];
    buffer[0] = c1;
    buffer[1] = c2;
    buffer[2] = 0;
    return iupac::encode(buffer, ali);
}

const char* iupac::decode(char iupac, GB_alignment_type ali, bool decode_amino_iupac_groups) {
    if (!isalpha(iupac)) {
        return IUPAC_EMPTY;
    }

    if (ali==GB_AT_AA) {
        if (decode_amino_iupac_groups) {
            Amino_Group group = get_amino_group_for(iupac);
            if (group == AA_GROUP_NONE) {
                return "";
            }
            else {
                awt_assert(group >= AA_GROUP_NONE && group<AA_GROUP_COUNT);
                return aminoGroupMembers[group];
            }
        }
        return "?";
    }

    const char *decoded = nuc_group[toupper(iupac)-'A'][ali==GB_AT_RNA ? 1 : 0].members;

    return decoded ? decoded : IUPAC_EMPTY;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS

#include <test_unit.h>

void TEST_amino_groups() {
    TEST_EXPECT_EQUAL(decode('x', GB_AT_AA, true), "");
    TEST_EXPECT_EQUAL(decode('A', GB_AT_AA, true), "AGPST");

    // each character (A-Z) shall be member of a group:
    for (char c = 'A'; c <= 'Z'; ++c) {
        Amino_Group group = get_amino_group_for(c);
        TEST_EXPECT_LESS(group, AA_GROUP_COUNT);
        TEST_EXPECT_DIFFERENT(group, AA_GROUP_ILLEGAL);
        TEST_EXPECT_IN_RANGE(group, AA_GROUP_NONE, AA_GROUP_ZETA);

        {
            const char * const members = aminoGroupMembers[group];
            const char * const found   = strchr(members, c);
            TEST_EXPECT_EQUAL(found ? found[0] : 0, c); // char has to be a member of its group
        }

        if (group != AA_GROUP_NONE) {
            const char * const decoded = decode(c, GB_AT_AA, true);
            const char * const found   = strchr(decoded, c);

            TEST_EXPECT_EQUAL(found ? found[0] : 0, c); // check char is
        }
    }

    // no character may be member of two groups
    for (Amino_Group group = AA_GROUP_NONE; group < AA_GROUP_COUNT; group = Amino_Group(group+1)) {
        const char * const member = aminoGroupMembers[group];
        for (int pos = 0; member[pos]; ++pos) {
            Amino_Group groupOfChar = get_amino_group_for(member[pos]);
            TEST_EXPECT_EQUAL(groupOfChar, group);
        }
    }
}

void TEST_nuc_groups() {
    for (int base = 0; base<26; base++) {
        TEST_EXPECT_EQUAL(nuc_group[base][0].count, nuc_group[base][1].count);
        for (int alitype = 0; alitype<2; alitype++) {
            const Nuc_Group& group = nuc_group[base][alitype];
            if (group.members) {
                if ((base+'A') == 'N') {
                    TEST_EXPECT_EQUAL__BROKEN(group.count, strlen(group.members), 1);
                    // @@@ fails because count is 1 for "ACGT" [N]
                    // maybe be expected by resolve_IUPAC_target_string (fix this first)
                    TEST_EXPECT_EQUAL(group.count, 1U); // fixture for behavior
                }
                else {
                    TEST_EXPECT_EQUAL(group.count, strlen(group.members));
                }
                
                for (size_t pos = 1; pos<group.count; ++pos) {
                    TEST_EXPECT_LESS(group.members[pos-1], group.members[pos]);
                }
            }
            else {
                TEST_REJECT(group.count);
            }
        }
    }
}

#endif // UNIT_TESTS
