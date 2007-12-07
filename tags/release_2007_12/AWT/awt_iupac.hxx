#ifndef AWT_IUPAC
#define AWT_IUPAC

#ifndef ARBDB_H
#include <arbdb.h>
#endif

struct AWT_IUPAC_descriptor {
    const char *iupac;
    int count; // no of bases, that may be in this code
};

extern AWT_IUPAC_descriptor AWT_iupac_code[26][2]; // second index: [0] uses T, [1] uses U
extern const int AWT_iupac_group[26];

char AWT_encode_iupac(const char bases[], GB_alignment_type aliType);
char AWT_iupac_add(char c1, char c2, GB_alignment_type ali);

const char* AWT_decode_iupac(char iupac, GB_alignment_type aliType, int decode_amino_iupac_groups);

inline int AWT_iupac2index(char c) { // calculate index for AWT_iupac_code
    if (c>='A' && c<='Z') return c-'A';
    return *(char*)0; // exception
}

#else
#error awt_iupac.hxx included twice
#endif // AWT_IUPAC
