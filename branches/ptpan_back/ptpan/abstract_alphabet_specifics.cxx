/*!
 * \file abstract_alphabet_specifics.cxx
 *
 * \date 08.02.2011
 * \author Tilo Eissler
 *
 * Contributors: Chris Hodges, Jörg Böhnel
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "abstract_alphabet_specifics.h"

/*!
 * \brief constructor
 */
AbstractAlphabetSpecifics::AbstractAlphabetSpecifics() :
        compressed_sequence_shortcut_base(), alphabet_size(), max_code_fit_long(), short_edge_max(), power_table(
                NULL), bits_shift_table(NULL), bits_use_table(NULL), bits_mask_table(
                NULL), bits_count_table(), compress_table(), decompress_table(
                NULL), complement_table(NULL), seq_code_valid_table(), mismatch_weights(), ignore_code(
                0), wildcard_code(0), max_dots_in_match(0), dot_code(0), hyphen_code(
                0) {
}

/*!
 * \brief Private copy constructor
 */
AbstractAlphabetSpecifics::AbstractAlphabetSpecifics(
        const AbstractAlphabetSpecifics& /*aas*/) {
}

/*!
 * \brief Private assignment operator
 */
AbstractAlphabetSpecifics& AbstractAlphabetSpecifics::operator=(
        const AbstractAlphabetSpecifics& aas) {
    if (this != &aas) {
        // we don't use it, so we don't implement it!
    }
    return *this;
}

/*!
 * \brief destructor
 */
AbstractAlphabetSpecifics::~AbstractAlphabetSpecifics() {
}

/*!
 * \brief Calculate number of valid characters in given sequence
 *
 * \param srcseq Sequence to count valid characters for
 *
 * \return ULONG
 */
ULONG AbstractAlphabetSpecifics::calcLengthForFilteredSequence(
        CONST_STRPTR srcseq) const {
    ULONG len = 0;
    /* calculate size of compressed sequence */
    while (*srcseq) {
        len += seq_code_valid_table[(INT) *srcseq++];
    }
    return (len);
}

/*!
 * \brief Filter sequence to given buffer: Removes invalid characters
 *
 * \param srcseq Sequence to filter
 * \param filtptr Pointer to output buffer
 *
 * \return Number of valid characters (= length of filtered sequence)
 */
ULONG AbstractAlphabetSpecifics::filterSequenceTo(CONST_STRPTR srcstr,
        STRPTR filtptr) const {
    ULONG len = 0;
    UBYTE code;

    /* now actually filter the sequence */
    while ((code = *srcstr++)) {
        if (seq_code_valid_table[code]) {
            /* add sequence code */
            *filtptr++ = decompress_table[compress_table[code]];
            len++;
        }
    }
    *filtptr = 0;
    return (len);
}

/*!
 * \brief Filter sequence: Removes invalid characters
 *
 * \param srcseq Sequence to filter
 *
 * \return STRPTR New allocated filtered sequence
 */
STRPTR AbstractAlphabetSpecifics::filterSequence(STRPTR srcseq) const {

    ULONG len = calcLengthForFilteredSequence(srcseq);
    STRPTR filtseq = (STRPTR) malloc(len + 1);
    if (!filtseq) {
        return (NULL); /* out of memory */
    }
    /* now actually compress the sequence */
    len = filterSequenceTo(srcseq, filtseq);
    //printf("%ld bytes used.\n", len);

    return (filtseq);
}

/*!
 * \brief Compress sequence to given buffer
 *
 * \param srcseq Sequence to compress
 * \param seqptr Output buffer for decompressed sequence
 *
 * \return ULONG number of characters
 */
ULONG AbstractAlphabetSpecifics::compressSequenceTo(CONST_STRPTR srcseq,
        ULONG *seqptr) const {

    ULONG seqcode;
    UBYTE code;

    /* now actually compress the sequence */
    ULONG len = 4;
    UWORD cnt = 0;
    ULONG pval = 0;
    while ((code = *srcseq++)) {
        if (seq_code_valid_table[code]) {
            /* add sequence code */
            seqcode = compress_table[code];
            pval *= alphabet_size;
            pval += seqcode;
            /* check, if storage capacity was reached? */
            if (++cnt == max_code_fit_long) {
                /* write out compressed longword (with eof bit) */
                //printf("[%08lx]", pval | bits_mask_table[cnt]);
                *seqptr++ = (pval << bits_shift_table[max_code_fit_long])
                        | bits_mask_table[max_code_fit_long];
                len += 4;
                cnt = 0;
                pval = 0;
            }
        }
    }

    /* write pending bits (with eof bit) */
    *seqptr = (pval << bits_shift_table[cnt]) | bits_mask_table[cnt];
    //printf("[%08lx]\n", *seqptr);
    return (len);
}

/*!
 * \brief Compress sequence
 *
 * \param srcseq Pointer to sequence to compress
 *
 * \return ULONG* Pointer to compressed sequence
 */
ULONG* AbstractAlphabetSpecifics::compressSequence(CONST_STRPTR srcseq) const {

    ULONG len = calcLengthForFilteredSequence(srcseq);
    //printf("compressing %s (%ld/%ld)...", srcseq, len, (ULONG) strlen(srcseq));

    /* that's all we need: ceil(len/max_code_fit_long) longwords */
    ULONG *compseq = (ULONG *) malloc(
            ((len / max_code_fit_long) + 1) * sizeof(ULONG));
    if (!compseq) {
        return (NULL); /* out of memory */
    }
    /* now actually compress the sequence */
    len = compressSequenceTo(srcseq, compseq);
    //printf("%ld bytes used.\n", len);

    return (compseq);
}

/*!
 * \brief Get length of compressed sequence
 *
 * \param seqptr Pointer to compressed sequence
 *
 * \return ULONG length
 */
ULONG AbstractAlphabetSpecifics::getLengthOfCompressedSequence(
        ULONG *seqptr) const {
    ULONG len = 0;
    UWORD cnt;
    ULONG mask = bits_mask_table[max_code_fit_long];
    do {
        if (*seqptr++ & mask) /* check, if lowest bit is set */
        {
            len += max_code_fit_long;
        } else {
            /* okay, we seem to be at the end of the compressed sequence,
             and we need to find out the actual size */
            --seqptr;
            cnt = max_code_fit_long;
            while (--cnt) {
                if (*seqptr & bits_mask_table[cnt]) /* seems like we found it */
                {
                    len += cnt;
                    break;
                }
            }
            break;
        }
    } while (TRUE);

    return (len);
}

/*!
 * \brief Get size of compressed ULONG (up to max_code_fit_long)
 *
 * \param pval Compressed value
 *
 * \return UWORD length
 */
UWORD AbstractAlphabetSpecifics::getCompressedLongSize(ULONG pval) const {
    UWORD cnt = max_code_fit_long;
    while (!(pval & bits_mask_table[cnt])) /* check, if termination bit is set */
    {
        cnt--;
    }
    return (cnt);
}

/*!
 * \brief Decompress given sequence into given buffer
 *
 * \param seqptr Pointer to sequence to decompress
 * \param tarseq Pointer to output buffer
 *
 * \return ULONG number of decompressed characters
 */
ULONG AbstractAlphabetSpecifics::decompressSequenceTo(ULONG *seqptr,
        STRPTR tarseq) const {
    ULONG len = 0;
    BOOL lastlong;
    UWORD cnt;
    ULONG pval;
    do {
        /* get next longword */
        pval = *seqptr++;
        cnt = getCompressedLongSize(pval);
        pval >>= bits_shift_table[cnt];
        lastlong = (cnt < max_code_fit_long); /* last longword reached? */

        /* unpack compressed longword */
        if (cnt) {
            do {
                *tarseq++ = decompress_table[(pval / power_table[--cnt])
                        % alphabet_size];
                len++;
            } while (cnt);
        }
    } while (!lastlong);
    *tarseq = 0; /* null terminate string */

    return (len);
}

/*!
 * \brief Decompress given sequence into new sequence
 *
 * \param seqptr Pointer to sequence to decompress
 *
 * \return STRPTR
 */
STRPTR AbstractAlphabetSpecifics::decompressSequence(ULONG *seqptr) const {

    /* first get length */
    ULONG len = getLengthOfCompressedSequence(seqptr);
    /* allocate memory for uncompressed sequence */
    STRPTR tarseq = (STRPTR) malloc(len + 1);
    if (!tarseq) {
        return (NULL); /* out of memory */
    }

    /* decompress sequence */
    decompressSequenceTo(seqptr, tarseq);
    //printf("Decompressed sequence '%s'\n", tarseq);
    return (tarseq);
}

/*!
 * \brief Decompress squence partly to given output buffer
 *
 * \param *seqptr Input sequence
 * \param seqpos Position to start at
 * \param length number of characters to decompress
 * \param tarseq Pointer to output buffer
 *
 * \return ULONG
 */
ULONG AbstractAlphabetSpecifics::decompressSequencePartTo(ULONG *seqptr,
        ULONG seqpos, ULONG length, STRPTR tarseq) const {

    if (!length) /* empty sequence requested? */
    {
        *tarseq = 0;
        return (0);
    }

    UWORD codeoff = seqpos % max_code_fit_long;
    UWORD cnt;
    ULONG len = 0;
    ULONG pval;
    BOOL lastlong;
    /* decompress sequence */
    BOOL first = TRUE;
    seqptr += seqpos / max_code_fit_long;
    do {
        /* get next longword */
        pval = *seqptr++;
        cnt = getCompressedLongSize(pval);
        pval >>= bits_shift_table[cnt];
        lastlong = (cnt < max_code_fit_long); /* last longword reached? */

        if (first) /* do we need to start at a certain offset? */
        {
            if (codeoff > cnt) /* past end of sequence? */
            {
                break;
            }
            cnt -= codeoff;
            first = FALSE;
        }
        /* unpack compressed longword */
        do {
            *tarseq++ = decompress_table[(pval / power_table[--cnt])
                    % alphabet_size];
            len++;
            length--;
        } while (cnt && length);
    } while (length && !lastlong);
    *tarseq = 0; /* null terminate string */

    return (len);
}

/*!
 * \brief Compress given sequence with dots and hyphens and add it to
 *   given PTPanEntry
 *
 * \param seqdata Pointer to sequence data
 * \param struct PTPanEntry *
 */
INT AbstractAlphabetSpecifics::compressSequenceWithDotsAndHyphens(
        const char *seqdata, struct PTPanEntry *ps) const {
    // len is the count of characters inserted into pe_RawData
    ps->pe_RawDataSize = 0;

    ULONG bitpos = 0; // ...pe_RawDataSize will be set to len later
    UBYTE* ptr = (UBYTE*) seqdata;

    // prepare compressed sequence shortcuts
    if (ps->pe_SeqDataSize <= 4 * compressed_sequence_shortcut_base) {
        ps->pe_CompressedShortcutsCount = 4;
    } else {
        ps->pe_CompressedShortcutsCount = ps->pe_SeqDataSize
                / compressed_sequence_shortcut_base;
    }

    ps->pe_CompressedShortcuts = (struct PTPanComprSeqShortcuts *) calloc(
            ps->pe_CompressedShortcutsCount,
            sizeof(struct PTPanComprSeqShortcuts));
    if (!ps->pe_CompressedShortcuts) {
        printf(
                "Error: Could not get enough memory for compressed sequence shortcuts\n");
        return -1;
    }
    ULONG chunk_size = ps->pe_SeqDataSize
            / (ps->pe_CompressedShortcutsCount + 1);
    // this is save as the compressed sequence is shorter in all cases
    const LONG buflen = (ps->pe_SeqDataSize) + 1;

    UBYTE* buffer = (UBYTE*) malloc(buflen);
    if (buffer == NULL) {
        printf(
                "Error: Could not get enough memory to compress sequences with dots and hyphens\n");
        return -1;
    }
    ULONG base_count = 0;
    ULONG shortcut_count = 0;
    struct PTPanComprSeqShortcuts *csc;
    ULONG offset_threshold = chunk_size;
    while (*ptr) {
        assert(((bitpos >> 3) + 1 < ps->pe_SeqDataSize));
        assert((bitpos >> 3) + 1 < (ULONG) buflen);
        if (seq_code_valid_table[*ptr]) { // found a valid character
            UBYTE seqcode = compress_table[*ptr];
            bitpos = AbstractAlphabetSpecifics::writeBits(buffer, bitpos,
                    seqcode, 3); // write valid char
            ++ptr;
            ++(ps->pe_RawDataSize);
            ++base_count;
        } else if (*ptr == '.') { // found a '.'
            ULONG count;
            for (count = 0; *ptr == '.'; ++count, ++ptr) {
            } // count all '.'
            base_count += count;
            bitpos = writeManyChars(buffer, bitpos, dot_code, count); // write all '.'
        } else if (*ptr == '-') { // found a '-'
            ULONG count;
            for (count = 0; *ptr == '-'; ++count, ++ptr) {
            } // count all '-'
            base_count += count;
            bitpos = writeManyChars(buffer, bitpos, hyphen_code, count); // write all '-'
        } else { // found an unknown char
            //          printf("Found an unknown char in Species Sequence - ignoring\n");
            bitpos = writeManyChars(buffer, bitpos, hyphen_code, 1); // write one '-'
            ++ptr;
        }
        // add compressed sequence shortcuts ...
        if (base_count > offset_threshold
                && (shortcut_count < ps->pe_CompressedShortcutsCount)) {
            offset_threshold += chunk_size;
            csc =
                    (struct PTPanComprSeqShortcuts *) &ps->pe_CompressedShortcuts[shortcut_count];
            csc->pcs_AbsPosition = base_count;
            csc->pcs_BitPosition = bitpos;
            csc->pcs_RelPosition = ps->pe_RawDataSize;
            shortcut_count++;
        }
    }
    bitpos = AbstractAlphabetSpecifics::writeBits(buffer, bitpos, 0x07, 3); // write end flag (111)
    // fill up last bits after stop bit with 0 to avoid complications with fwrite
    // but DO NOT change bitpos!
    AbstractAlphabetSpecifics::writeBits(buffer, bitpos, 0x0, 8 - (bitpos % 8));

    ps->pe_SeqDataCompressedSize = bitpos;
    ps->pe_SeqDataCompressed = (UBYTE*) realloc(buffer, ((bitpos >> 3) + 1));
    if (ps->pe_SeqDataCompressed == NULL) {
        printf(
                "Error: Could not get enough memory to compress sequences with dots and hyphens\n");
        return -1;
    }
    if (AbstractAlphabetSpecifics::readBits((UBYTE *) ps->pe_SeqDataCompressed,
            bitpos - 3, 3) != 0x07) {
        printf("Error Compressing SeqData (with '.' and '-')\n");
        return -1;
    }

    // TODO must be done by caller!
    // pg->pg_TotalSeqCompressedSize += ((ps->pe_SeqDataCompressedSize >> 3) + 1); // convert from bit to byte
    return 0;
}

/*!
 * \brief Build the sequence complement
 *
 * \warning The value given by the pointer is altered!
 */
void AbstractAlphabetSpecifics::complementSequence(STRPTR seqstr) const {
    UBYTE code;
    /* flip A<->T and C<->G */
    while ((code = *seqstr)) {
        *seqstr++ = decompress_table[complement_table[compress_table[code]]];
    }
}

/*!
 * \brief Reverse the given sequence
 *
 * \warning The value given by the pointer is altered!
 */
void AbstractAlphabetSpecifics::reverseSequence(STRPTR seqstr) const {
    char code;
    STRPTR leftptr = seqstr;
    STRPTR rightptr = &seqstr[strlen(seqstr)];

    /* reverse the sequence string */
    while (leftptr < rightptr) {
        code = *leftptr;
        *leftptr++ = *--rightptr;
        *rightptr = code;
    }
}

/*!
 * \brief Convert given type into an integer value
 *
 * \param type (AlphabetType)
 *
 * \return UWORD
 */
UWORD AbstractAlphabetSpecifics::typeAsInt(
        AbstractAlphabetSpecifics::AlphabetType type) {
    UWORD ret_value = 0;
    switch (type) {
    case DNA4:
        ret_value = 1;
        break;
    case DNA5:
        ret_value = 2;
        break;
    case PROTEIN:
        ret_value = 4;
        break;
    default:
        break;
    }
    return ret_value;
}

/*!
 * \brief Convert given type from an integer value to AlphabetType
 *
 * \param type In integer form (UWORD)
 *
 * \return AlphabetType
 */
AbstractAlphabetSpecifics::AlphabetType AbstractAlphabetSpecifics::intAsType(
        UWORD type) {
    switch (type) {
    case 1:
        return DNA4;
        break;
    case 2:
        return DNA5;
        break;
    case 4:
        return PROTEIN;
        break;
    default:
        break;
    }
    return UNKNOWN_TYPE;
}

// private stuff

/*!
 * \brief Private method to write many characters into buffer at bit position
 */
ULONG AbstractAlphabetSpecifics::writeManyChars(UBYTE* buffer, ULONG bitpos,
        BYTE c, ULONG i) const {
    while (i > 0) {
        bitpos = AbstractAlphabetSpecifics::writeBits(buffer, bitpos, c, 3); // code for character
        if (i == 1) {
            bitpos = AbstractAlphabetSpecifics::writeBits(buffer, bitpos, 0x01,
                    1); // 1
            return bitpos;
        }
        if (i == 2) {
            bitpos = AbstractAlphabetSpecifics::writeBits(buffer, bitpos, 0x01,
                    2); // 01
            return bitpos;
        }
        if (i <= 63) {
            bitpos = AbstractAlphabetSpecifics::writeBits(buffer, bitpos, 0x01,
                    3); // 001
            bitpos = AbstractAlphabetSpecifics::writeBits(buffer, bitpos,
                    (i & 0x3f), 6);
            // 6 bit payload (up to 63)
            return bitpos;
        }
        if (i <= 1023) {
            bitpos = AbstractAlphabetSpecifics::writeBits(buffer, bitpos, 0x01,
                    4); // 0001
            bitpos = AbstractAlphabetSpecifics::writeBits(buffer, bitpos,
                    (i & 0x3ff), 10);
            // 10 bit payload (up to 1023)
            return bitpos;
        }
        if (i <= 8191) {
            bitpos = AbstractAlphabetSpecifics::writeBits(buffer, bitpos, 0x00,
                    4); // 0000
            bitpos = AbstractAlphabetSpecifics::writeBits(buffer, bitpos,
                    (i & 0x1fff), 13);
            // 13 bit payload (up to 8191)
            return bitpos;
        }
        bitpos = AbstractAlphabetSpecifics::writeBits(buffer, bitpos, 0x00, 4); // 0000
        bitpos = AbstractAlphabetSpecifics::writeBits(buffer, bitpos, 8191, 13); // 13 bit payload (exactly 8191)
        i -= 8191;
    }
    return bitpos;
}
