/*!
 * \file dna5_alphabet_specifics.cxx
 *
 * \date 08.02.2011
 * \author Tilo Eissler
 *
 * Contributors: Chris Hodges, Jörg Böhnel
 */

#include <stdlib.h>
#include <assert.h>

#include "dna5_alphabet_specifics.h"

/*!
 * \brief Constructor
 */
Dna5AlphabetSpecifics::Dna5AlphabetSpecifics() :
        AbstractAlphabetSpecifics() {
    init();
    fillTables();
}

/*!
 * \brief Destructor
 */
Dna5AlphabetSpecifics::~Dna5AlphabetSpecifics() {
    free(power_table);
    free(bits_shift_table);
    free(bits_use_table);
    free(bits_mask_table);
    free(decompress_table);
    free(complement_table);
    MismatchWeights::freeMismatchWeightMatrix(mismatch_weights);
}

/*!
 * \brief TODO
 *
 * Note: count is set in every valid case!
 */
UBYTE Dna5AlphabetSpecifics::getNextCharacter(UBYTE* buffer, ULONG &bitpos,
        ULONG &count) const {
    // return the next character of sequence or 0xff if end flag found
    // increase bitpos by consumed bits
    UBYTE code = AbstractAlphabetSpecifics::readBits(buffer, bitpos, 3); // set count to the number of
    bitpos += 3; // found characters

    switch (code) {
    case N:
    case A:
    case C:
    case G:
    case T:
        // valid character
        count = 1;
        return decompress_table[code];
        break;
    case DOT:
    case HYPHEN: {
        // '.' or '-'
        // skip ... chars
        UBYTE character;
        if (code == HYPHEN) {
            character = '-';
        } else { // if (code == DOT)
            character = '.';
        }

        code = AbstractAlphabetSpecifics::readBits(buffer, bitpos, 4);
        if ((code >> 3) == 0x01) // 1xxx     skip one
                {
            count = 1;
            ++bitpos;
        } else if ((code >> 2) == 0x01) // 01xx     skip two
                {
            count = 2;
            bitpos += 2;
        } else if ((code >> 1) == 0x01) // 001x     skip up to 63
                {
            bitpos += 3;
            count = AbstractAlphabetSpecifics::readBits(buffer, bitpos, 6);
            bitpos += 6;
        } else if ((code) == 0x01) // 0001     skip up to 1023
                {
            bitpos += 4;
            count = AbstractAlphabetSpecifics::readBits(buffer, bitpos, 10);
            bitpos += 10;
        } else if ((code) == 0x00) // 0000     skip up to 8191
                {
            bitpos += 4;
            count = AbstractAlphabetSpecifics::readBits(buffer, bitpos, 13);
            bitpos += 13;

            ULONG tmpbitpos = bitpos; // test if next char is also the same.
            ULONG tmpcount = count;
            UBYTE tmpcode = getNextCharacter(buffer, tmpbitpos, tmpcount);
            if (character == tmpcode) // it is -> the number of same characters
                    { // was splitted (i.e. >8191)
                assert(count == 8191);
                bitpos = tmpbitpos; // consume bits...
                count += tmpcount; // ...and add count
            }
        } else {
            assert(false);
            // shouldn't be possible to get to this line
        }
        return character;
    }
        break;
    case 0x07: // end flag
        return 0xff;
        break;
    default:
        // neither end-flag nor valid char
        // nor '.' nor '-' => something went wrong
        assert(false);
        break;
    }
    return 0xff;
}

/*!
 * \brief TODO
 */
void Dna5AlphabetSpecifics::init() {

    compressed_sequence_shortcut_base = 65536;
    alphabet_size = DNA5_ALPHASIZE;
    max_code_fit_long = DNA5_MAXCODEFITLONG;
    short_edge_max = DNA5_SHORTEDGEMAX;

    power_table = (ULONG *) malloc((DNA5_MAXCODEFITLONG + 1) * sizeof(ULONG));
    bits_shift_table = (UWORD *) malloc(
            sizeof(UWORD) * (DNA5_MAXCODEFITLONG + 1));
    bits_use_table = (UWORD *) malloc(
            sizeof(UWORD) * (DNA5_MAXCODEFITLONG + 1));
    bits_mask_table = (ULONG *) malloc(
            sizeof(ULONG) * (DNA5_MAXCODEFITLONG + 1));

    decompress_table = (UBYTE *) malloc(DNA5_ALPHASIZE * sizeof(UBYTE));
    complement_table = (UBYTE *) malloc(DNA5_ALPHASIZE * sizeof(UBYTE));

    mismatch_weights = MismatchWeights::createMismatchWeightMatrix(
            alphabet_size);

    ignore_code = IGNORE;
    wildcard_code = N;
}

/*!
 * \brief TODO
 */
void Dna5AlphabetSpecifics::fillTables() {

    /* calculate table of powers of alphabet size */
    ULONG pval = 1;
    ULONG cnt;
    for (cnt = 0, pval = 1; cnt <= DNA5_MAXCODEFITLONG; cnt++) {
        power_table[cnt] = pval;
        pval *= alphabet_size;
    }

    /* calculate table of bits used for code of size n */
    pval = alphabet_size;
    ULONG numbits = 0;
    bits_use_table[0] = 0;
    bits_mask_table[0] = (1UL << ((sizeof(ULONG) * 8 - 1) - numbits));
    for (cnt = 1; cnt <= DNA5_MAXCODEFITLONG; cnt++) {
        while (pval > (1UL << numbits)) {
            numbits++;
        }
        bits_use_table[cnt] = numbits; /* bits required for codesize */
        bits_shift_table[cnt] = (sizeof(ULONG) * 8) - numbits; /* how many bits to shift left */
        bits_mask_table[cnt] = (1UL << ((sizeof(ULONG) * 8 - 1) - numbits));
        pval *= alphabet_size;
    }

    /* bits count table */
    for (cnt = 0; cnt < 256; cnt++) {
        numbits = 0;
        pval = cnt;
        while (pval) {
            numbits += (pval & 1);
            pval >>= 1;
        }
        bits_count_table[cnt] = numbits;
    }

    /* sequence compression tables */
    /* set to N for all other codes */
    for (cnt = 0; cnt < 256; cnt++) {
        compress_table[cnt] = N;
    }
    // now set specific values
    compress_table[(UBYTE) 'a'] = A;
    compress_table[(UBYTE) 'A'] = A;
    compress_table[(UBYTE) 'c'] = C;
    compress_table[(UBYTE) 'C'] = C;
    compress_table[(UBYTE) 'g'] = G;
    compress_table[(UBYTE) 'G'] = G;
    compress_table[(UBYTE) 't'] = T;
    compress_table[(UBYTE) 'T'] = T;
    compress_table[(UBYTE) 'u'] = T;
    compress_table[(UBYTE) 'U'] = T;
    compress_table[(UBYTE) '-'] = IGNORE; /* these chars don't go */
    compress_table[(UBYTE) '.'] = IGNORE; /* into the tree */
    compress_table[0] = IGNORE; /* terminal char, to optimize certain routines */

    /* inverse table for decompressing */
    decompress_table[N] = 'N';
    decompress_table[A] = 'A';
    decompress_table[C] = 'C';
    decompress_table[G] = 'G';
    decompress_table[T] = 'U';

    /* table for creating the complement sequence */
    complement_table[N] = N;
    complement_table[A] = T;
    complement_table[C] = G;
    complement_table[G] = C;
    complement_table[T] = A;

    /* counting table to avoid branches */
    for (cnt = 0; cnt < 256; cnt++) {
        seq_code_valid_table[cnt] = (compress_table[cnt] == IGNORE) ? 0 : 1;
    }

    /* default mismatch weights */
    struct MismatchWeights *mw = mismatch_weights;
    /* format: replace code1 (query) by code2 (database) adds x to the error value */
    /*     N   A   C   G   T (-> query)
     N 0.0 0.1 0.1 0.1 0.1
     A  *  0.0 1.0 1.0 1.0
     C  *  1.0 0.0 1.0 1.0
     G  *  1.0 1.0 0.0 1.0
     T  *  1.0 1.0 1.0 0.0
     ins 2.0 2.0 2.0 2.0 2.0
     del  *  2.0 2.0 2.0 2.0
     */
    /* fill diagonal first (no mismatch) */
    for (cnt = 0; cnt < DNA5_ALPHASIZE; cnt++) {
        mw->mw_Replace[cnt * DNA5_ALPHASIZE + cnt] = 0.0;
    }

    /* N is a joker, but setting it to slightly higher values might be sensible */
    mw->mw_Replace[A * DNA5_ALPHASIZE + N] = 0.0;
    mw->mw_Replace[C * DNA5_ALPHASIZE + N] = 0.0;
    mw->mw_Replace[G * DNA5_ALPHASIZE + N] = 0.0;
    mw->mw_Replace[T * DNA5_ALPHASIZE + N] = 0.0;

    /* replacing N by A, C, G, T will not occur (search string may not contain N) */
    mw->mw_Replace[N * DNA5_ALPHASIZE + A] = 99999.0;
    mw->mw_Replace[N * DNA5_ALPHASIZE + C] = 99999.0;
    mw->mw_Replace[N * DNA5_ALPHASIZE + G] = 99999.0;
    mw->mw_Replace[N * DNA5_ALPHASIZE + T] = 99999.0;

    /* other parts of the matrix (should be symmetrical, but doesn't need to) */
    mw->mw_Replace[A * DNA5_ALPHASIZE + C] = 1.1;
    mw->mw_Replace[C * DNA5_ALPHASIZE + A] = 1.0;

    mw->mw_Replace[A * DNA5_ALPHASIZE + G] = 0.2;
    mw->mw_Replace[G * DNA5_ALPHASIZE + A] = 1.5;

    mw->mw_Replace[A * DNA5_ALPHASIZE + T] = 1.1;
    mw->mw_Replace[T * DNA5_ALPHASIZE + A] = 1.1;

    mw->mw_Replace[C * DNA5_ALPHASIZE + G] = 1.1;
    mw->mw_Replace[G * DNA5_ALPHASIZE + C] = 1.5;

    mw->mw_Replace[C * DNA5_ALPHASIZE + T] = 0.6;
    mw->mw_Replace[T * DNA5_ALPHASIZE + C] = 1.1;

    mw->mw_Replace[G * DNA5_ALPHASIZE + T] = 1.5;
    mw->mw_Replace[T * DNA5_ALPHASIZE + G] = 0.6;

    /* insert operations (to query string) */
    mw->mw_Insert[N] = 1.6;
    mw->mw_Insert[A] = 1.6;
    mw->mw_Insert[C] = 1.6;
    mw->mw_Insert[G] = 1.6;
    mw->mw_Insert[T] = 1.6;

    /* delete operations (from query string) */
    mw->mw_Delete[N] = 99999.0; /* should never happen */
    mw->mw_Delete[A] = 1.6;
    mw->mw_Delete[C] = 1.6;
    mw->mw_Delete[G] = 1.6;
    mw->mw_Delete[T] = 1.6;

    max_dots_in_match = 5;
    dot_code = DOT;
    hyphen_code = HYPHEN;
}

/*!
 * \brief Check uncompressed sequence for wild-card
 *
 * For DNA5, the wildcard is 'N'
 *
 * \param seq Sequence to check
 *
 * \return bool true if there is at least one wildcard
 */
bool Dna5AlphabetSpecifics::checkUncompressedForWildcard(STRPTR seq) const {
    ULONG length = strlen(seq);
    for (ULONG i = 0; i < length; i++) {
        if (seq[i] == 'N') {
            return true;
        }
    }
    return false;
}
