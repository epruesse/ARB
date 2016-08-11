// =============================================================== //
//                                                                 //
//   File      : AP_codon_table.cxx                                //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in January 2010   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "AP_codon_table.hxx"
#include "iupac.h"

#include <arbdb.h>

#include <cctype>
#include <arb_str.h>

#define pn_assert(cond) arb_assert(cond)

#define EMBL_BACTERIAL_TABLE_INDEX 11

// Info about translation codes was taken from
// http://www.ncbi.nlm.nih.gov/Taxonomy/Utils/wprintgc.cgi

static AWT_Codon_Code_Definition AWT_codon_def[AWT_CODON_TABLES+1] =
    {
        //   0000000001111111111222222222233333333334444444444555555555566666
        //   1234567890123456789012345678901234567890123456789012345678901234

        //  "TTTTTTTTTTTTTTTTCCCCCCCCCCCCCCCCAAAAAAAAAAAAAAAAGGGGGGGGGGGGGGGG",  base1
        //  "TTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGG",  base2
        //  "TCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAG"   base3
        {
            " (1) Standard code",
            "FFLLSSSSYY**CC*WLLLLPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG", // The first code in this table has to be 'Standard code'!
            "---M---------------M---------------M----------------------------",
            1 // arb:0
        },
        {
            " (2) Vertebrate mitochondrial code",
            "FFLLSSSSYY**CCWWLLLLPPPPHHQQRRRRIIMMTTTTNNKKSS**VVVVAAAADDEEGGGG",
            "--------------------------------MMMM---------------M------------",
            2 // arb:1
        },
        {
            " (3) Yeast mitochondrial code",
            "FFLLSSSSYY**CCWWTTTTPPPPHHQQRRRRIIMMTTTTNNKKSSRRVVVVAAAADDEEGGGG",
            "----------------------------------MM----------------------------",
            3 // arb:2
        },
        {
            " (4) Mold/Protozoan/Coelenterate mito. + Mycoplasma/Spiroplasma code",
            "FFLLSSSSYY**CCWWLLLLPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG",
            "--MM---------------M------------MMMM---------------M------------",
            4 // arb:3
        },
        {
            " (5) Invertebrate mitochondrial code",
            "FFLLSSSSYY**CCWWLLLLPPPPHHQQRRRRIIMMTTTTNNKKSSSSVVVVAAAADDEEGGGG",
            "---M----------------------------MMMM---------------M------------",
            5 // arb:4
        },
        {
            " (6) Ciliate, Dasycladacean and Hexamita nuclear code",
            "FFLLSSSSYYQQCC*WLLLLPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG",
            "-----------------------------------M----------------------------",
            6 // arb:5
        },
        {
            " (9) Echinoderm and Flatworm mitochondrial code",
            "FFLLSSSSYY**CCWWLLLLPPPPHHQQRRRRIIIMTTTTNNNKSSSSVVVVAAAADDEEGGGG",
            "-----------------------------------M---------------M------------",
            9 // arb:6
        },
        {
            "(10) Euplotid nuclear code",
            "FFLLSSSSYY**CCCWLLLLPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG",
            "-----------------------------------M----------------------------",
            10 // arb:7
        },
        //   0000000001111111111222222222233333333334444444444555555555566666
        //   1234567890123456789012345678901234567890123456789012345678901234

        //  "TTTTTTTTTTTTTTTTCCCCCCCCCCCCCCCCAAAAAAAAAAAAAAAAGGGGGGGGGGGGGGGG",  base1
        //  "TTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGG",  base2
        //  "TCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAG"   base3
        {
            "(11) Bacterial and Plant Plastid code",
            "FFLLSSSSYY**CC*WLLLLPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG",
            "---M---------------M------------MMMM---------------M------------",
            11 // arb:8
        },
        {
            "(12) Alternative Yeast nuclear code",
            "FFLLSSSSYY**CC*WLLLSPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG",
            "-------------------M---------------M----------------------------",
            12 // arb:9
        },
        {
            "(13) Ascidian mitochondrial code",
            "FFLLSSSSYY**CCWWLLLLPPPPHHQQRRRRIIMMTTTTNNKKSSGGVVVVAAAADDEEGGGG",
            "---M------------------------------MM---------------M------------",
            13 // arb:10
        },
        {
            "(14) Alternative Flatworm mitochondrial code",
            "FFLLSSSSYYY*CCWWLLLLPPPPHHQQRRRRIIIMTTTTNNNKSSSSVVVVAAAADDEEGGGG",
            "-----------------------------------M----------------------------",
            14 // arb:11
        },
        {
            "(15) Blepharisma nuclear code",
            "FFLLSSSSYY*QCC*WLLLLPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG",
            "-----------------------------------M----------------------------",
            15 // arb:12
        },
        {
            "(16) Chlorophycean mitochondrial code",
            "FFLLSSSSYY*LCC*WLLLLPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG",
            "-----------------------------------M----------------------------",
            16 // arb:13
        },
        {
            "(21) Trematode mitochondrial code",
            "FFLLSSSSYY**CCWWLLLLPPPPHHQQRRRRIIMMTTTTNNNKSSSSVVVVAAAADDEEGGGG",
            "-----------------------------------M---------------M------------",
            21 // arb:14
        },
        {
            "(22) Scenedesmus obliquus mitochondrial code",
            "FFLLSS*SYY*LCC*WLLLLPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG",
            "-----------------------------------M----------------------------",
            22 // arb:15
        },
        {
            "(23) Thraustochytrium mitochondrial code",
            "FF*LSSSSYY**CC*WLLLLPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG",
            "--------------------------------M--M---------------M------------",
            23 // arb:16
        },

        { 0, 0, 0, 0 } // end of table-marker
    };

#define MAX_EMBL_TRANSL_TABLE_VALUE 23 // maximum known EMBL transl_table value

int AWT_embl_transl_table_2_arb_code_nr(int embl_code_nr) {
    // returns -1 if embl_code_nr is not known by ARB

    static bool initialized = false;
    static int  arb_code_nr_table[MAX_EMBL_TRANSL_TABLE_VALUE+1];                 // key: embl_code_nr, value: arb_code_nr or -1

    if (!initialized) {
        for (int embl = 0; embl <= MAX_EMBL_TRANSL_TABLE_VALUE; ++embl) {
            arb_code_nr_table[embl] = -1; // illegal table
        }
        for (int arb_code_nr = 0; arb_code_nr < AWT_CODON_TABLES; ++arb_code_nr) {
            arb_code_nr_table[AWT_codon_def[arb_code_nr].embl_feature_transl_table] = arb_code_nr;
        }
        // should be index of 'Bacterial and Plant Plastid code'
        // (otherwise maybe AWAR_PROTEIN_TYPE_bacterial_code_index  is wrong)
        pn_assert(arb_code_nr_table[EMBL_BACTERIAL_TABLE_INDEX] == AWAR_PROTEIN_TYPE_bacterial_code_index);
        pn_assert(arb_code_nr_table[1] == 0); // Standard code has to be on index zero!

        initialized = true;
    }

    if (embl_code_nr<0 || embl_code_nr>MAX_EMBL_TRANSL_TABLE_VALUE) return -1;

    int arb_code_nr = arb_code_nr_table[embl_code_nr];
#ifdef DEBUG
    if (arb_code_nr != -1) {
        pn_assert(arb_code_nr >= 0 && arb_code_nr < AWT_CODON_TABLES);
        pn_assert(AWT_arb_code_nr_2_embl_transl_table(arb_code_nr) == embl_code_nr);
    }
#endif
    return arb_code_nr;
}

int AWT_arb_code_nr_2_embl_transl_table(int arb_code_nr) {
    pn_assert(arb_code_nr >= 0 && arb_code_nr<AWT_CODON_TABLES);
    return AWT_codon_def[arb_code_nr].embl_feature_transl_table;
}


static bool codon_tables_initialized = false;
static char definite_translation[AWT_MAX_CODONS]; // contains 0 if ambiguous, otherwise it contains the definite translation
static char *ambiguous_codons[AWT_MAX_CODONS]; // for each ambiguous codon: contains all translations (each only once)

void AP_initialize_codon_tables() {
    if (codon_tables_initialized) return;

    int codon_nr;
    int code_nr;

    for (codon_nr=0; codon_nr<AWT_MAX_CODONS; codon_nr++) {
        ambiguous_codons[codon_nr] = 0;
    }

    pn_assert(AWT_CODON_TABLES>=1);
    memcpy(definite_translation, AWT_codon_def[0].aa, AWT_MAX_CODONS); // only one translation is really definite

    pn_assert(AWT_codon_def[AWT_CODON_TABLES].aa==NULL); // Error in AWT_codon_def or AWT_CODON_CODES

    for (code_nr=1; code_nr<AWT_CODON_TABLES; code_nr++) {
        const char *translation = AWT_codon_def[code_nr].aa;

        for (codon_nr=0; codon_nr<AWT_MAX_CODONS; codon_nr++) {
            if (definite_translation[codon_nr]!='?') { // is definite till now
                if (definite_translation[codon_nr]!=translation[codon_nr]) { // we found a different translation
                    // create ambiguous_codons:
                    char *amb = ARB_calloc<char>(AWT_MAX_CODONS+1);
                    amb[0] = definite_translation[codon_nr];
                    amb[1] = translation[codon_nr];

                    ambiguous_codons[codon_nr] = amb;
                    definite_translation[codon_nr] = '?';
#if defined(DEBUG) && 0
                    printf("amb[%i]='%s'\n", codon_nr, amb);
#endif
                }
            }
            else { // is ambiguous
                if (strchr(ambiguous_codons[codon_nr], translation[codon_nr])==0) { // not listed in ambiguous codons
                    // append another ambiguous codon:
                    char *amb = ambiguous_codons[codon_nr];
                    amb[strlen(amb)] = translation[codon_nr];
#if defined(DEBUG) && 0
                    printf("amb[%i]='%s'\n", codon_nr, amb);
#endif
                }
            }
        }
    }

    codon_tables_initialized = true;
}

// return 0..3 (ok) or 4 (failure)
inline int dna2idx(char c) {
    switch (c) {
        case 'T': case 't':
        case 'U': case 'u': return 0;
        case 'C': case 'c': return 1;
        case 'A': case 'a': return 2;
        case 'G': case 'g': return 3;
    }
    return 4;
}

inline char idx2dna(int idx) {
    pn_assert(idx>=0 && idx<4);
    return "TCAG"[idx];
}

inline int calc_codon_nr(const char *dna) {
    int i1 = dna2idx(dna[0]); if (i1 == 4) return AWT_MAX_CODONS; // is not a codon
    int i2 = dna2idx(dna[1]); if (i2 == 4) return AWT_MAX_CODONS;
    int i3 = dna2idx(dna[2]); if (i3 == 4) return AWT_MAX_CODONS;

    int codon_nr = i1*16 + i2*4 + i3;
    pn_assert(codon_nr>=0 && codon_nr<=AWT_MAX_CODONS);
    return codon_nr;
}

inline void build_codon(int codon_nr, char *to_buffer) {
    pn_assert(codon_nr>=0 && codon_nr<AWT_MAX_CODONS);

    to_buffer[0] = idx2dna((codon_nr>>4)&3);
    to_buffer[1] = idx2dna((codon_nr>>2)&3);
    to_buffer[2] = idx2dna(codon_nr&3);
}

const char* AWT_get_codon_code_name(int code) {
    pn_assert(code>=0 && code<AWT_CODON_TABLES);
    return AWT_codon_def[code].name;
}

static const char *protein_name[26+1] = {
    "Ala", // A
    "Asx", // B (= D or N)
    "Cys", // C
    "Asp", // D
    "Glu", // E
    "Phe", // F
    "Gly", // G
    "His", // H
    "Ile", // I
    0,     // J
    "Lys", // K
    "Leu", // L
    "Met", // M
    "Asn", // N
    0,     // O
    "Pro", // P
    "Gln", // Q
    "Arg", // R
    "Ser", // S
    "Thr", // T
    0,     // U
    "Val", // V
    "Trp", // W
    "Xxx", // X
    "Tyr", // Y
    "Glx", // Z (= E or Q)
    0
};

const char *AP_get_protein_name(char protein) {
    if (protein=='*') return "End";
    if (protein=='-') return "---";

    pn_assert(protein>='A' && protein<='Z');
    pn_assert(protein_name[protein-'A']!=0);
    return protein_name[protein-'A'];
}

#ifdef DEBUG

inline char nextBase(char c) {
    switch (c) {
        case 'T': return 'C';
        case 'C': return 'A';
        case 'A': return 'G';
#if 0
        case 'G': return 0;
#else
        case 'G': return 'M';
        case 'M': return 'R';
        case 'R': return 'W';
        case 'W': return 'S';
        case 'S': return 'Y';
        case 'Y': return 'K';
        case 'K': return 'V';
        case 'V': return 'H';
        case 'H': return 'D';
        case 'D': return 'B';
        case 'B': return 'N';
        case 'N': return 0;
#endif
        default: pn_assert(0);
    }
    return 0;
}

void AWT_dump_codons() {
    const TransTables all_allowed;

    for (char c='*'; c<='Z'; c++) {
        printf("Codons for '%c': ", c);
        bool first_line = true;
        bool found      = false;
        for (char b1='T'; b1; b1=nextBase(b1)) {
            for (char b2='T'; b2; b2=nextBase(b2)) {
                for (char b3='T'; b3; b3=nextBase(b3)) {
                    char dna[4];
                    dna[0]=b1;
                    dna[1]=b2;
                    dna[2]=b3;
                    dna[3]=0;

                    TransTables remaining;
                    if (AWT_is_codon(c, dna, all_allowed, remaining)) {
                        if (!first_line) fputs("\n                ", stdout);
                        first_line = false;
                        printf("%s (%s)", dna, remaining.to_string());
                        found = true;
                    }
                }
            }
        }
        if (!found) fputs("none", stdout);
        fputs("\n", stdout);
        if (c=='*') c='A'-1;
    }
}
#endif

char AWT_is_start_codon(const char *dna, int arb_code_nr) {
    // if dna[0]..dna[2] is defined as start codon for 'arb_code_nr'
    //                  return 'M' (or whatever is defined in tables)
    // return 0 otherwise

    char is_start_codon = 0;
    int  codon_nr       = calc_codon_nr(dna);

    pn_assert(arb_code_nr >= 0 && arb_code_nr<AWT_CODON_TABLES);

    if (codon_nr != AWT_MAX_CODONS) { // dna is a clean codon (it contains no iupac-codes)
        const char *starts = AWT_codon_def[arb_code_nr].starts;

        is_start_codon = starts[codon_nr];
        if (is_start_codon == '-') is_start_codon = 0;
    }

    return is_start_codon;
}

inline bool protMatches(char p1, char p2) {
    /*! return true if p1 matches p2
     * @param p1 "normal" protein (neighter B nor Z)
     * @param p2 any protein (also B or Z)
     * B is a shortcut for Asp(=D) or Asn(=N)
     * Z is a shortcut for Glu(=E) or Gln(=Q)
     */
    pn_assert(p1 != 'B' && p1 != 'Z');
    pn_assert(p1 == toupper(p1));
    pn_assert(p2 == toupper(p2));

    if (p1 == p2) return true;
    if (p2 == 'B') return p1 == 'D' || p1 == 'N';
    if (p2 == 'Z') return p1 == 'E' || p1 == 'Q';
    return false;
}
inline bool containsProtMatching(const char *pstr, char p) {
    /*! return true, if 'pstr' contains any protein that matches 'p'.
     * uses same logic as protMatches()
     */
    pn_assert(p == toupper(p));
    if (p == 'B') return strchr(pstr, 'D') != 0 || strchr(pstr, 'N') != 0;
    if (p == 'Z') return strchr(pstr, 'E') != 0 || strchr(pstr, 'Q') != 0;
    return strchr(pstr, p)                 != 0;
}
inline bool isGap(char c) { return c == '-' || c == '.'; }

inline GB_ERROR neverTranslatesError(const char *dna, char protein) {
    const char *prot_check = "ABCDEFGHIKLMNPQRSTVWXYZ*";
    if (strchr(prot_check, protein) == 0) {
        return GBS_global_string("'%c' is no valid amino acid", protein);
    }
    return GBS_global_string("'%c%c%c' never translates to '%c'", dna[0], dna[1], dna[2], protein);
}

bool AWT_is_codon(char protein, const char *const dna, const TransTables& allowed, TransTables& remaining, const char **fail_reason_ptr) {
    /*! test if 'dna' codes 'protein'
     * @param protein amino acid
     * @param dna three nucleotides (gaps allowed, e.g. 'A-C' can be tested vs 'X')
     * @param allowed allowed translation tables
     * @param remaining returns the remaining allowed translation tables (only if this functions returns true)
     * @param fail_reason_ptr if not NULL => store reason for failure here (or set it to NULL on success)
     * @return true if dna translates to protein
     */

    pn_assert(allowed.any());
    pn_assert(codon_tables_initialized);

    const char *fail_reason               = 0;
    if (fail_reason_ptr) *fail_reason_ptr = 0;

    bool is_codon        = false;
    int  codon_nr        = calc_codon_nr(dna);
    int  first_iupac_pos = -1;
    int  iupac_positions = 0;
    bool decided         = false;
    bool general_failure = false;

    protein = toupper(protein);

    if (codon_nr==AWT_MAX_CODONS) { // dna is not a clean codon (i.e. it contains iupac-codes or gaps)
        bool too_short = false;
        int  nucs_seen = 0;
        for (int iupac_pos=0; iupac_pos<3 && !too_short && !fail_reason; iupac_pos++) {
            char N = dna[iupac_pos];

            if (!N) too_short = true;
            else if (!isGap(N)) {
                nucs_seen++;
                if (strchr("ACGTU", N) == 0) {
                    if (first_iupac_pos==-1) first_iupac_pos = iupac_pos;
                    iupac_positions++;
                    const char *decoded_iupac = iupac::decode(N, GB_AT_DNA, 0);
                    if (!decoded_iupac[0]) { // no valid IUPAC
                        fail_reason = GBS_global_string("Invalid character '%c' in DNA", N);
                    }
                }
            }
        }

        if (!fail_reason && !nucs_seen) { // got no dna
            fail_reason = "No nucleotides left";
        }
        else if (nucs_seen<3) {
            too_short = true;
        }

        if (fail_reason) {
            decided = true; // fails for all proteins
        }
        else if (too_short) {
            decided  = true;
            if (protein == 'X') {
                is_codon = true;
            }
            else {
                char dna_copy[4];
                strncpy(dna_copy, dna, 3);
                dna_copy[3] = 0;

                fail_reason = GBS_global_string("Not enough nucleotides (got '%s')", dna_copy);
            }
        }
    }

    if (!decided) {
        if (protein == 'X') {
            TransTables  allowed_copy = allowed;
            const char  *prot_check   = "ABCDEFGHIKLMNPQRSTVWYZ*";

            for (int pc = 0; prot_check[pc]; ++pc) {
                if (AWT_is_codon(prot_check[pc], dna, allowed_copy, remaining)) {
                    allowed_copy.forbid(remaining);
                    if (allowed_copy.none()) break;
                }
            }

            if (allowed_copy.any()) {
                is_codon          = true;
                remaining = allowed_copy;
            }
            else {
                fail_reason = neverTranslatesError(dna, protein);
            }
        }
        else if (codon_nr==AWT_MAX_CODONS) { // dna is a codon with one or more IUPAC codes
            pn_assert(iupac_positions);
            const char *decoded_iupac = iupac::decode(dna[first_iupac_pos], GB_AT_DNA, 0);
            pn_assert(decoded_iupac[0]); // already should have been catched above

            char dna_copy[4];
            memcpy(dna_copy, dna, 3);
            dna_copy[3] = 0;

            bool all_are_codons = true;
            bool one_is_codon   = false;

            TransTables allowed_copy = allowed;

            for (int i=0; decoded_iupac[i]; i++) {
                dna_copy[first_iupac_pos] = decoded_iupac[i];
                const char *subfail;
                if (!AWT_is_codon(protein, dna_copy, allowed_copy, remaining, &subfail)) {
                    all_are_codons = false;
                    if (!one_is_codon && ARB_strBeginsWith(subfail, "Not all ")) one_is_codon = true;
                    if (one_is_codon) break;
                }
                else {
                    one_is_codon      = true;
                    allowed_copy = remaining;
                }
            }

            if (all_are_codons) {
                pn_assert(allowed_copy.any());
                remaining = allowed_copy;
                is_codon          = true;
            }
            else {
                remaining.forbidAll();
                dna_copy[first_iupac_pos] = dna[first_iupac_pos];
                if (one_is_codon) {
                    fail_reason = GBS_global_string("Not all IUPAC-combinations of '%s' translate to '%c'", dna_copy, protein); // careful when changing this message (see above)
                }
                else {
                    fail_reason = neverTranslatesError(dna_copy, protein);
                }
            }
        }
        else if (definite_translation[codon_nr]!='?') { // codon has a definite translation (i.e. translates equal for all code-tables)
            int ok = protMatches(definite_translation[codon_nr], protein);

            if (ok) {
                remaining = allowed;
                is_codon          = true;
            }
            else {
                remaining.forbidAll();
                fail_reason     = GBS_global_string("'%c%c%c' translates to '%c', not to '%c'", dna[0], dna[1], dna[2], definite_translation[codon_nr], protein);
                general_failure = true;
            }
        }
        else if (!containsProtMatching(ambiguous_codons[codon_nr], protein)) { // codon does not translate to protein in any code-table
            remaining.forbidAll();
            fail_reason = neverTranslatesError(dna, protein);
            general_failure = true;
        }
        else {
#if defined(ASSERTION_USED)
            bool correct_disallowed_translation = false;
#endif

            // Now codon translates to protein in at least 1 code-table!
            // Check whether protein translates in any of the allowed code-tables:
            for (int code_nr=0; code_nr<AWT_CODON_TABLES; code_nr++) {
                if (protMatches(AWT_codon_def[code_nr].aa[codon_nr], protein)) { // does it translate correct?
                    if (allowed.is_allowed(code_nr)) { // is this code allowed?
                        remaining.allow(code_nr);
                        is_codon = true;
                    }
                    else {
                        remaining.forbid(code_nr); // otherwise forbid code in future
#if defined(ASSERTION_USED)
                        correct_disallowed_translation = true;
#endif
                    }
                }
                else {
                    remaining.forbid(code_nr); // otherwise forbid code in future
                }
            }

            if (!is_codon) {
                pn_assert(correct_disallowed_translation); // should be true because otherwise we shouldn't run into this else-branch
                fail_reason = GBS_global_string("'%c%c%c' does not translate to '%c'", dna[0], dna[1], dna[2], protein);
            }
        }
    }

    if (!is_codon) {
        pn_assert(fail_reason);
        if (fail_reason_ptr) {
            if (!allowed.all() && !general_failure) {
                int one = allowed.explicit_table();
                if (one == -1) {
                    const char *left_tables = allowed.to_string();
                    pn_assert(left_tables[0]); // allowed should never be empty!

                    fail_reason = GBS_global_string("%s (for any of the leftover trans-tables: %s)", fail_reason, left_tables);
                }
                else {
                    fail_reason = GBS_global_string("%s (for trans-table %i)", fail_reason, one);
                }
            }

            *fail_reason_ptr = fail_reason; // set failure-reason if requested
        }
    }
#if defined(ASSERTION_USED)
    else {
        pn_assert(remaining.is_subset_of(allowed));
    }
#endif
    return is_codon;
}

// -------------------------------------------------------------------------------- Codon_Group

#if defined(DEBUG)
// #define DUMP_CODON_GROUP_EXPANSION
#endif

class Codon_Group
{
    char codon[64]; // index is calculated with calc_codon_nr

public:
    Codon_Group(char protein, int code_nr);
    ~Codon_Group() {}

    Codon_Group& operator += (const Codon_Group& other);
    int expand(char *to_buffer) const;
};

Codon_Group::Codon_Group(char protein, int code_nr) {
    protein = toupper(protein);
    pn_assert(protein=='*' || isalpha(protein));
    pn_assert(code_nr>=0 && code_nr<AWT_CODON_TABLES);

    const char *amino_table = AWT_codon_def[code_nr].aa;
    for (int i=0; i<AWT_MAX_CODONS; i++) {
        codon[i] = amino_table[i]==protein;
    }
}

Codon_Group& Codon_Group::operator+=(const Codon_Group& other) {
    for (int i=0; i<AWT_MAX_CODONS; i++) {
        codon[i] = codon[i] || other.codon[i];
    }
    return *this;
}

inline int legal_dna_no(int i) { return i>=0 && i<4; }

inline const char *buildMixedCodon(const char *const con1, const char *const con2) {
    int mismatches = 0;
    int mismatch_index = -1;
    static char buf[4];

    for (int i=0; i<3; i++) {
        if (con1[i]!=con2[i]) {
            mismatches++;
            mismatch_index = i;
        }
        else {
            buf[i] = con1[i];
        }
    }

    if (mismatches==1) { // exactly one position differs between codons
        pn_assert(mismatch_index!=-1);
        buf[mismatch_index] = iupac::combine(con1[mismatch_index], con2[mismatch_index], GB_AT_DNA);
        buf[3]              = 0;

        if (memcmp(con1, buf, 3) == 0 ||
            memcmp(con2, buf, 3) == 0)
        {
            return 0;
        }

#if defined(DUMP_CODON_GROUP_EXPANSION)
        printf(" buildMixedCodon('%c%c%c','%c%c%c') == '%s'\n",
               con1[0], con1[1], con1[2],
               con2[0], con2[1], con2[2],
               buf);
#endif

        return buf;
    }
    return 0;
}

static int expandMore(const char *bufferStart, int no_of_condons, char*&to_buffer) {
    int i, j;
    const char *con1, *con2;
    int added = 0;

    for (i=0; i<no_of_condons; i++) {
        con1 = bufferStart+3*i;

        for (j=i+1; j<no_of_condons; j++) {
            con2 = bufferStart+3*j;
            const char *result = buildMixedCodon(con1, con2);
            if (result) {
                to_buffer[0] = 0;
                // do we already have this codon?
                const char *found;
                const char *startSearch = bufferStart;
                for (;;) {
                    found = strstr(startSearch, result);
                    if (!found) break;
                    int pos = (found-bufferStart);
                    if ((pos%3)==0) break; // yes already here!
                    startSearch = found+1; // was misaligned -> try behind
                }

                if (!found) {
                    memmove(to_buffer, result, 3); to_buffer+=3;
                    added++;
                }
            }
        }
    }
    return no_of_condons+added;
}

int Codon_Group::expand(char *to_buffer) const {
    int count = 0;
    int i;
    char *org_to_buffer = to_buffer;

    for (i=0; i<AWT_MAX_CODONS; i++) {
        if (codon[i]) {
            build_codon(i, to_buffer);
            to_buffer += 3;
            count++;
        }
    }

#if defined(DUMP_CODON_GROUP_EXPANSION)
    to_buffer[0] = 0;
    printf("codons = '%s'\n", org_to_buffer);
#endif

    for (;;) {
        int new_count = expandMore(org_to_buffer, count, to_buffer);
        if (new_count==count) break; // nothing expanded -> done
        count = new_count;
#if defined(DUMP_CODON_GROUP_EXPANSION)
        to_buffer[0] = 0;
        printf("codons (expandedMore) = '%s'\n", org_to_buffer);
#endif
    }

    pn_assert(count==(int(to_buffer-org_to_buffer)/3));

    return count;
}

// --------------------------------------------------------------------------------

static Codon_Group *get_Codon_Group(char protein, int code_nr) {
    pn_assert(code_nr>=0 && code_nr<AWT_CODON_TABLES);
    protein = toupper(protein);
    pn_assert(isalpha(protein) || protein=='*');
    pn_assert(codon_tables_initialized);

    Codon_Group *cgroup = 0;

    if (protein=='B') {
        cgroup = new Codon_Group('D', code_nr);
        Codon_Group N('N', code_nr);
        *cgroup += N;
    }
    else if (protein=='Z') {
        cgroup = new Codon_Group('E', code_nr);
        Codon_Group Q('Q', code_nr);
        *cgroup += Q;
    }
    else {
        cgroup = new Codon_Group(protein, code_nr);
    }

    pn_assert(cgroup);

    return cgroup;
}

#define MAX_CODON_LIST_LENGTH (70*3)

// get a list of all codons ("xyzxyzxyz...") encoding 'protein' in case we use Codon-Code 'code_nr'
// (includes all completely contained IUPAC-encoded codons at the end of list)
const char *AP_get_codons(char protein, int code_nr) {
    Codon_Group *cgroup = get_Codon_Group(protein, code_nr);

    static char buffer[MAX_CODON_LIST_LENGTH+1];
    int offset = 3*cgroup->expand(buffer);
    pn_assert(offset<MAX_CODON_LIST_LENGTH);
    buffer[offset] = 0;

    delete cgroup;

    return buffer;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

void TEST_codon_check() {
    AP_initialize_codon_tables();

    TEST_EXPECT(protMatches('V', 'V'));
    TEST_EXPECT(protMatches('N', 'B'));
    TEST_EXPECT(protMatches('E', 'Z'));
    TEST_EXPECT(!protMatches('N', 'Z'));
    TEST_EXPECT(!protMatches('V', 'Z'));

    TEST_EXPECT_EQUAL(AP_get_codons('D', 0), "GATGACGAY");
    TEST_EXPECT_EQUAL(AP_get_codons('N', 0), "AATAACAAY");
    TEST_EXPECT_EQUAL(AP_get_codons('B', 0), "AAT" "AAC" "GAT" "GAC" "AAY" "RAT" "RAC" "GAY" "RAY"); // 'B' = 'D' or 'N'

    TEST_EXPECT_EQUAL(AP_get_codons('L', 0),  "TTATTGCTTCTCCTACTG"    "TTRYTAYTGCTYCTWCTKCTMCTSCTRCTHCTBCTDCTVYTRCTN");
    TEST_EXPECT_EQUAL(AP_get_codons('L', 2),  "TTATTG"                "TTR");
    TEST_EXPECT_EQUAL(AP_get_codons('L', 9),  "TTATTGCTTCTCCTAT"      "TRYTACTYCTWCTMCTH");
    TEST_EXPECT_EQUAL(AP_get_codons('L', 13), "TTATTGTAGCTTCTCCTACTG" "TTRYTATWGYTGCTYCTWCTKCTMCTSCTRCTHCTBCTDCTVYTRCTN");
    TEST_EXPECT_EQUAL(AP_get_codons('L', 16), "TTGCTTCTCCTAC"         "TGYTGCTYCTWCTKCTMCTSCTRCTHCTBCTDCTVCTN");

    TEST_EXPECT_EQUAL(AP_get_codons('S', 0),  "TCTTCCTCATCGAGTAGC"       "TCYTCWTCKTCMTCSTCRAGYTCHTCBTCDTCVTCN");
    TEST_EXPECT_EQUAL(AP_get_codons('S', 4),  "TCTTCCTCATCGAGTAGCAGAAGG" "TCYTCWTCKTCMTCSTCRAGYAGWAGKAGMAGSAGRTCHTCBTCDTCVAGHAGBAGDAGVTCNAGN");
    TEST_EXPECT_EQUAL(AP_get_codons('S', 9),  "TCTTCCTCATCGCTGAGTAGC"    "TCYTCWTCKTCMTCSTCRAGYTCHTCBTCDTCVTCN");
    TEST_EXPECT_EQUAL(AP_get_codons('S', 15), "TCTTCCTCGAGTAGC"          "TCYTCKTCSAGYTCB");

    TEST_EXPECT_EQUAL(AP_get_codons('X', 0), ""); // @@@ wrong: TGR->X (or disallow call)

#define ALL_TABLES "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16"
    const TransTables allowed;

    // ---------------------------
    //      test valid codons
    struct test_is_codon {
        char        protein;
        const char *codon;
        const char *tables;
    };
    test_is_codon is_codon[] = {
        { 'P', "CCC", ALL_TABLES },
        { 'P', "CCN", ALL_TABLES },
        { 'R', "CGN", ALL_TABLES },

        { 'D', "GAY", ALL_TABLES },
        { 'N', "AAY", ALL_TABLES },
        { 'B', "AAY", ALL_TABLES }, // translates to 'N', but matches B(=D|N) for realigner
        { 'B', "GAY", ALL_TABLES }, // translates to 'D', but matches B(=D|N) for realigner
        { 'B', "RAY", ALL_TABLES }, // translates to 'D' or to 'N' (i.e. only matches 'B', see failing test for 'RAY' below)
        { 'B', "RAT", ALL_TABLES },

        { 'Q', "CAR", ALL_TABLES },
        { 'E', "GAR", ALL_TABLES },
        { 'Z', "SAR", ALL_TABLES },

        { 'X', "NNN", ALL_TABLES },

        { 'L', "TTR", "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15" },
        { 'L', "YTA", "0,1,3,4,5,6,7,8,9,10,11,12,13,14,15" },
        { 'L', "CTM", "0,1,3,4,5,6,7,8,9,10,11,12,13,14,15,16" },
        { 'L', "CTN", "0,1,3,4,5,6,7,8,10,11,12,13,14,15,16" },
        { 'L', "CTK", "0,1,3,4,5,6,7,8,10,11,12,13,14,15,16" },
        { 'L', "TWG", "13,15" },
        { 'X', "TWG", "0,1,2,3,4,5,6,7,8,9,10,11,12,14,16" }, // all but "13,15"

        { 'S', "AGY", ALL_TABLES },
        { 'S', "TCY", ALL_TABLES },
        { 'S', "TCN", "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,16" }, // all but 15
        { 'S', "AGN", "4,6,11,14" },
        { 'S', "AGR", "4,6,11,14" },

        { 'R', "AGR", "0,2,3,5,7,8,9,12,13,15,16" },
        { 'W', "TGR", "1,2,3,4,6,10,11,14" }, // R=AG
        { 'X', "TGR", "0,5,7,8,9,12,13,15,16" }, // all but "1,2,3,4,6,10,11,14" (e.g. code==0: TGA->* TGG->W => TGR->X?)

        { 'C', "TGW", "7" },
        { 'X', "TGW", "0,1,2,3,4,5,6,8,9,10,11,12,13,14,15,16" }, // all but 7

        { '*', "TRA", "0,8,9,12,13,15,16" },
        { 'X', "TRA", "1,2,3,4,5,6,7,10,11,14" }, // all but "0,8,9,12,13,15,16"

        { '*', "TAR", "0,1,2,3,4,6,7,8,9,10,14,16" },
        { 'Q', "TAR", "5" },
        { 'Z', "TAR", "5" },
        { 'X', "TAR", "11,12,13,15" },

        { 'B', "AAW", "6,11,14" },
        { 'N', "AAW", "6,11,14" },
        { 'X', "AAW", "0,1,2,3,4,5,7,8,9,10,12,13,15,16" }, // all but "6,11,14"

        { 'L', "CTG", "0,1,3,4,5,6,7,8,10,11,12,13,14,15,16" }, // all but 2 and 9
        { 'S', "CTG", "9" },
        { 'T', "CTG", "2" },

        { 'L', "CTR", "0,1,3,4,5,6,7,8,10,11,12,13,14,15,16" }, // all but 2 and 9
        { 'T', "CTR", "2" },
        { 'X', "CTR", "9" },

        { '*', "AGR", "1" },
        { 'G', "AGR", "10" },
        { 'R', "AGR", "0,2,3,5,7,8,9,12,13,15,16" },

        { 'X', "A-C", ALL_TABLES },
        { 'X', ".T.", ALL_TABLES },

        // tests to protect buffer overflows in dna
        { 'X', "CG", ALL_TABLES },
        { 'X', "T",  ALL_TABLES },

        { 0, NULL, NULL}
    };

    for (int c = 0; is_codon[c].protein; ++c) {
        const test_is_codon& C = is_codon[c];
        TEST_ANNOTATE(GBS_global_string("%c <- %s", C.protein, C.codon));

        TransTables  remaining;
        const char  *failure;
        bool         isCodon = AWT_is_codon(C.protein, C.codon, allowed, remaining, &failure);

        TEST_EXPECT_NULL(failure);
        TEST_EXPECT(isCodon);
        TEST_EXPECT_EQUAL(remaining.to_string(), C.tables);
    }

    // -----------------------------
    //      test invalid codons
    struct test_not_codon {
        char        protein;
        const char *codon;
        const char *error;
    };
    test_not_codon not_codon[] = {
        { 'P', "SYK", "Not all IUPAC-combinations of 'SYK' translate to 'P'" }, // correct (possible translations are PAL)
        { 'F', "SYK", "'SYK' never translates to 'F'" },                        // correct failure
        { 'P', "NNN", "Not all IUPAC-combinations of 'NNN' translate to 'P'" }, // correct failure
        { 'D', "RAY", "Not all IUPAC-combinations of 'RAY' translate to 'D'" }, // correct failure
        { 'E', "SAR", "Not all IUPAC-combinations of 'SAR' translate to 'E'" }, // correct failure

        { 'S', "CYT", "'CYT' never translates to 'S'" }, // correct failure

        { 'X', "AGR", "'AGR' never translates to 'X'" },
        { 'J', "RAY", "'J' is no valid amino acid" },
        { 'J', "AAA", "'J' is no valid amino acid" },

        { 'L', "A-C", "Not enough nucleotides (got 'A-C')" }, // correct failure
        { 'V', ".T.", "Not enough nucleotides (got '.T.')" }, // correct failure
        { 'L', "...", "No nucleotides left" },
        { 'J', "...", "No nucleotides left" },

        { 'X', "...", "No nucleotides left" },
        { 'X', "..",  "No nucleotides left" },
        { 'X', "-",   "No nucleotides left" },
        { 'X', "",    "No nucleotides left" },

        // test invalid chars
        { 'X', "AZA", "Invalid character 'Z' in DNA" },
        { 'X', "A@A", "Invalid character '@' in DNA" },
        { 'L', "AZA", "Invalid character 'Z' in DNA" },

        // tests to protect buffer overflows in dna

        { 'A', "--", "No nucleotides left" },
        { 'L', ".",  "No nucleotides left" },
        { 'J', ".",  "No nucleotides left" },
        { 'L', "AT", "Not enough nucleotides (got 'AT')" },
        { 'L', "C",  "Not enough nucleotides (got 'C')" },
        { 'L', "",   "No nucleotides left" },

        { 0, NULL, NULL}
    };
    for (int c = 0; not_codon[c].protein; ++c) {
        const test_not_codon& C = not_codon[c];
        TEST_ANNOTATE(GBS_global_string("%c <- %s", C.protein, C.codon));

        TransTables  remaining;
        const char  *failure;
        bool         isCodon = AWT_is_codon(C.protein, C.codon, allowed, remaining, &failure);

        TEST_EXPECT_EQUAL(failure, C.error);
        TEST_EXPECT(!isCodon);
    }

    // ----------------------------------
    //      test uncombinable codons
    struct test_uncombinable_codons {
        char        protein1;
        const char *codon1;
        const char *tables;
        char        protein2;
        const char *codon2;
        const char *error;
    };
    test_uncombinable_codons uncomb_codons[] = {
        { '*', "TTA", "16",      'E', "SAR", "Not all IUPAC-combinations of 'SAR' translate to 'E' (for trans-table 16)" },
        { '*', "TTA", "16",      'X', "TRA", "'TRA' never translates to 'X' (for trans-table 16)" },
        { 'L', "TAG", "13,15",   'X', "TRA", "'TRA' never translates to 'X' (for any of the leftover trans-tables: 13,15)" },
        { 'L', "TAG", "13,15",   'Q', "TAR", "'TAR' never translates to 'Q' (for any of the leftover trans-tables: 13,15)" },
        { '*', "TTA", "16",      '*', "TCA", "'TCA' does not translate to '*' (for trans-table 16)" },
        { 'N', "AAA", "6,11,14", 'X', "AAW", "'AAW' never translates to 'X' (for any of the leftover trans-tables: 6,11,14)" },
        { 'N', "AAA", "6,11,14", 'K', "AAA", "'AAA' does not translate to 'K' (for any of the leftover trans-tables: 6,11,14)" },

        { 0, NULL, NULL, 0, NULL, NULL}
    };

    for (int c = 0; uncomb_codons[c].protein1; ++c) {
        const test_uncombinable_codons& C = uncomb_codons[c];
        TEST_ANNOTATE(GBS_global_string("%c <- %s + %c <- %s", C.protein1, C.codon1, C.protein2, C.codon2));

        TransTables  remaining;
        const char  *failure;
        bool         isCodon = AWT_is_codon(C.protein1, C.codon1, allowed, remaining, &failure);

        TEST_EXPECT(isCodon);
        TEST_EXPECT_EQUAL(remaining.to_string(), C.tables);

        TransTables  remaining2;
        isCodon = AWT_is_codon(C.protein2, C.codon2, remaining, remaining2, &failure);
        TEST_EXPECT_EQUAL(failure, C.error);
        TEST_REJECT(isCodon);

    }
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
