/*!
 * \file abstract_alphabet_specifics.h
 *
 * \date 08.02.2011
 * \author Tilo Eissler
 *
 * Contributors: Chris Hodges, Jörg Böhnel
 */

#ifndef PTPAN_ABSTRACT_ALPHABET_SPECIFICS_H_
#define PTPAN_ABSTRACT_ALPHABET_SPECIFICS_H_

#include "ptpan.h"

/*!
 * \brief Class ...
 *
 * TODO
 */
class AbstractAlphabetSpecifics {
protected:
    AbstractAlphabetSpecifics();

public:
    enum AlphabetType {
        DNA4, DNA5, PROTEIN, UNKNOWN_TYPE
    };

    virtual ~AbstractAlphabetSpecifics() = 0;

    /*!
     * \brief ...
     */
    static inline ULONG writeBits(UBYTE *adr, ULONG bitpos, ULONG code,
            UWORD len) {
        UBYTE *badr = &adr[bitpos >> 3];
        ULONG newbitpos = bitpos + len;
        UWORD bitfrac = 8 - (bitpos & 7);

        /* initial first byte */
        if (bitfrac == 8) {
            *badr = 0;
        }
        if ((len >= bitfrac) && (bitfrac < 8)) {
            *badr++ |= code >> (len - bitfrac);
            len -= bitfrac;
            bitpos += bitfrac;
        } else {
            if (len < 8) {
                *badr |= code << (bitfrac - len);
                return (newbitpos);
            }
        }
        /* whole bytes */
        while (len > 7) {
            len -= 8;
            *badr++ = code >> len;
        }
        /* last fraction */
        if (len) {
            *badr = code << (8 - len);
        }
        return (newbitpos);
    }

    /*!
     * \brief ...
     */
    static inline ULONG readBits(UBYTE *adr, ULONG bitpos, UWORD len) {
        UBYTE *badr = &adr[bitpos >> 3];
        UWORD bitfrac = bitpos & 7;
        /*printf("BPos: %ld, Len %d, frac %d [%02x %02x %02x %02x %02x]\n",
         bitpos, len, bitfrac,
         badr[0], badr[1], badr[2], badr[3], badr[4]);*/
        /* initial first byte */
        if (len + bitfrac < 8) {
            return (((*badr << bitfrac) & 0xFF) >> (8 - len));
        }
        ULONG res = *badr++ & (0xFF >> bitfrac);
        //printf("First: %lx ", res);
        len -= (8 - bitfrac);
        while (len > 7) {
            res <<= 8;
            res |= *badr++;
            len -= 8;
        }
        //printf("Middle: %lx ", res);
        /* last fraction */
        if (len) {
            res <<= len;
            res |= *badr >> (8 - len);
        }
        //printf("Res: %08lx\n", res);
        return (res);
    }

    virtual AbstractAlphabetSpecifics::AlphabetType type() const = 0;
    virtual UBYTE
    getNextCharacter(UBYTE* buffer, ULONG &bitpos, ULONG &count) const = 0;

    virtual bool checkUncompressedForWildcard(STRPTR seq) const = 0;
    // methods for compressing, decompressing, filtering etc ...
    virtual ULONG calcLengthForFilteredSequence(CONST_STRPTR srcseq) const;
    virtual ULONG compressSequenceTo(CONST_STRPTR srcseq, ULONG *seqptr) const;
    virtual ULONG filterSequenceTo(CONST_STRPTR srcstr, STRPTR filtptr) const;
    virtual STRPTR filterSequence(STRPTR srcseq) const;
    virtual ULONG* compressSequence(CONST_STRPTR srcseq) const;
    virtual ULONG getLengthOfCompressedSequence(ULONG *seqptr) const;
    virtual UWORD getCompressedLongSize(ULONG pval) const;
    virtual ULONG decompressSequenceTo(ULONG *seqptr, STRPTR tarseq) const;
    virtual STRPTR decompressSequence(ULONG *seqptr) const;
    virtual ULONG decompressSequencePartTo(ULONG *seqptr, ULONG seqpos,
            ULONG length, STRPTR tarseq) const;

    virtual INT compressSequenceWithDotsAndHyphens(const char *seqdata,
            struct PTPanEntry *ps) const;

    virtual void complementSequence(STRPTR seqstr) const;
    virtual void reverseSequence(STRPTR seqstr) const;

    static UWORD typeAsInt(AbstractAlphabetSpecifics::AlphabetType type);
    static AbstractAlphabetSpecifics::AlphabetType intAsType(UWORD type);

    ULONG compressed_sequence_shortcut_base;

    // derived classes must fill them up!
    UWORD alphabet_size; // UWORD pg_AlphaSize; /* size of alphabet */
    UWORD max_code_fit_long;
    UWORD short_edge_max;

    /* precalculated Tables for speedup */
    ULONG *power_table; // ULONG pg_PowerTable[max_code_fit_long + 1]; /* stores powers of ALPHASIZE^i */
    UWORD *bits_shift_table; // UWORD pg_BitsShiftTable[max_code_fit_long + 1]; /* how many bits to shift left to get valid code */
    UWORD *bits_use_table; // UWORD pg_BitsUseTable[max_code_fit_long + 1]; /* how many bits are required to store ALPHASIZE^i */
    ULONG *bits_mask_table; // ULONG pg_BitsMaskTable[max_code_fit_long + 1]; /* eof bit mask for i codes */

    UWORD bits_count_table[256]; /* count how many bits are set */

    UBYTE compress_table[256]; /* table to compress char to e.g. { N, A, C, G, T } for DNA5 */
    UBYTE *decompress_table; // UBYTE pg_DecompressTable[ALPHASIZE]; /* inverse mapping of above */
    UBYTE *complement_table; // UBYTE pg_ComplementTable[ALPHASIZE]; /* table with A<->T, C<->G swapped */
    UBYTE seq_code_valid_table[256]; /* 1 for valid char, 0 for invalid (-.) */

    /* mismatch matrix */
    struct MismatchWeights *mismatch_weights; /* matrix containing the default weighted mismatch values */

    // some helpful values for iterations
    UBYTE ignore_code;
    UBYTE wildcard_code;

    // something for the compression algorithm
    ULONG max_dots_in_match;
    UBYTE dot_code;
    UBYTE hyphen_code;

private:
    AbstractAlphabetSpecifics(const AbstractAlphabetSpecifics& aas);
    AbstractAlphabetSpecifics& operator=(const AbstractAlphabetSpecifics& aas);

    virtual ULONG
    writeManyChars(UBYTE* buffer, ULONG bitpos, BYTE c, ULONG i) const;

};

#endif /* PTPAN_ABSTRACT_ALPHABET_SPECIFICS_H_ */
