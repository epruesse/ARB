#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <arb_assert.h>
#define awt_assert(bed) arb_assert(bed)

#include "awt_iupac.hxx"

#define IUPAC_EMPTY " "
#define ILL_CODE char(26)

AWT_IUPAC_descriptor AWT_iupac_code[26][2]= {
    {{ "A", 	1 }, { "A", 	1 }},
    {{ "CGT", 	3 }, { "CGU", 	3 }},
    {{ "C",	1 }, { "C",	1 }},
    {{ "AGT", 	3 }, { "AGU", 	3 }},
    {{ 0,	0 }, { 0,	0 }}, // E
    {{ 0,	0 }, { 0,	0 }}, // F
    {{ "G",	1 }, { "G",	1 }},
    {{ "ACT", 	3 }, { "ACU", 	3 }},
    {{ 0,	0 }, { 0,	0 }}, // I
    {{ 0, 	0 }, { 0, 	0 }}, // J
    {{ "GT", 	2 }, { "GU", 	2 }},
    {{ 0, 	0 }, { 0, 	0 }}, // L
    {{ "AC", 	2 }, { "AC", 	2 }},
    {{ "ACGT", 	1 }, { "ACGU", 	1 }}, // N
    {{ 0, 	0 }, { 0, 	0 }}, // O
    {{ 0, 	0 }, { 0, 	0 }}, // P
    {{ 0, 	0 }, { 0, 	0 }}, // Q
    {{ "AG", 	2 }, { "AG", 	2 }},
    {{ "CG", 	2 }, { "CG", 	2 }},
    {{ "T", 	1 }, { "U", 	1 }}, // T
    {{ "T", 	1 }, { "U", 	1 }}, // U
    {{ "ACG", 	3 }, { "ACG", 	3 }},
    {{ "AT", 	2 }, { "AU", 	2 }},
    {{ 0, 	0 }, { 0, 	0 }}, // X
    {{ "CT", 	2 }, { "CU", 	2 }},
    {{ 0, 	0 }, { 0,    	0 }}  // Z

    // (In each single string the nucs have to be sorted alphabetically)
};

const int AWT_iupac_group[26] =  { // this is for amino_acids
    1, // A
    2, // B
    0, // C
    2, // D
    2, // E
    5, // F
    1, // G
    3, // H
    4, // I
    0, // J
    3, // K
    4, // L
    4, // M
    2, // N
    0, // O
    1, // P
    2, // Q
    3, // R
    1, // S
    1, // T
    0, // U
    4, // V
    5, // W
    0, // X
    5, // Y
    2, // Z
};

static char IUPAC_add[26][26]; // uses T
static int IUPAC_add_initialized = 0;

static void initialize_IUPAC_add()
{
    int c1, c2;

    for (c1=0; c1<26; c1++) {
        const char *decoded1 = AWT_iupac_code[c1][0].iupac;

        if (!decoded1) {
            for (c2=0; c2<=c1; c2++) {
                IUPAC_add[c1][c2] = ILL_CODE;
                IUPAC_add[c2][c1] = ILL_CODE;
            }
        }
        else {
            IUPAC_add[c1][c1] = char(c1);	// add char to same char

            for (c2=0; c2<c1; c2++) {
                if (strchr(decoded1, 'A'+c2)!=0) {	// char is already contained in this IUPAC
                    IUPAC_add[c1][c2] = char(c1);
                }
                else {
                    const char *decoded2 = AWT_iupac_code[c2][0].iupac;

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
                            if (AWT_iupac_code[c3][0].iupac && strcmp(mixed, AWT_iupac_code[c3][0].iupac)==0) {
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

char AWT_encode_iupac(const char bases[], GB_alignment_type ali)
{
    if (!IUPAC_add_initialized) {
        initialize_IUPAC_add();
        IUPAC_add_initialized = 1;
    }

    if (ali==GB_AT_AA) {
        return '?';
    }

    int i = 0;
    char c1;

    while (1) {
        c1 = bases[i++];
        if (!c1) return '-';
        if (isalpha(c1)) break;
    }

    c1 = toupper(c1)-'A';
    char c = IUPAC_add[c1][c1];

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

char AWT_iupac_add(char c1, char c2, GB_alignment_type ali) {
    static char buffer[3];
    buffer[0] = c1;
    buffer[1] = c2;
    buffer[2] = 0;
    return AWT_encode_iupac(buffer, ali);
}

const char* AWT_decode_iupac(char iupac, GB_alignment_type ali, int decode_amino_iupac_groups)
{
    if (!isalpha(iupac)) {
        return IUPAC_EMPTY;
    }

    if (ali==GB_AT_AA) {
        if (decode_amino_iupac_groups) {
            int group = AWT_iupac_group[toupper(iupac)-'A'];

            switch (group) {
                case 1: return "AGPST";
                case 2: return "BDENQZ";
                case 3: return "HKR";
                case 4: return "ILMV";
                case 5: return "FWY";
            }
        }
        return "?";
    }

    const char *decoded = AWT_iupac_code[toupper(iupac)-'A'][ali==GB_AT_RNA ? 1 : 0].iupac;

    return decoded ? decoded : IUPAC_EMPTY;
}

