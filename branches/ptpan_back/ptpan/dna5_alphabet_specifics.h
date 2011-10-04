/*!
 * \file dna5_alphabet_specifics.h
 *
 * \date 08.02.2011
 * \author Tilo Eissler
 *
 * Contributors: Chris Hodges, Jörg Böhnel
 */

#ifndef DNA5_ALPHABET_SPECIFICS_H_
#define DNA5_ALPHABET_SPECIFICS_H_

// it's a little bit ugly, but so is life!
#define DNA5_ALPHASIZE 5

/* short edge maximum length (maximum length of edge stored implicitly
 and not in the dictionary) */
#define DNA5_SHORTEDGEMAX 6

// How many codes can be stored in a LONG
#ifdef ARB_64
//   -> ALPHASIZE^MAXCODEFITLONG < 2^63
#define DNA5_MAXCODEFITLONG 27
#else
//   -> ALPHASIZE^MAXCODEFITLONG < 2^31
#define DNA5_MAXCODEFITLONG 13
#endif

#include "abstract_alphabet_specifics.h"

/*!
 * \brief TODO
 */
class Dna5AlphabetSpecifics: public AbstractAlphabetSpecifics {
public:
    Dna5AlphabetSpecifics();
    virtual ~Dna5AlphabetSpecifics();

    virtual AbstractAlphabetSpecifics::AlphabetType type() const {
        return DNA5;
    }

    virtual UBYTE
    getNextCharacter(UBYTE* buffer, ULONG &bitpos, ULONG &count) const;

    virtual bool checkUncompressedForWildcard(STRPTR seq) const;

private:
    enum DNA5_SEQCODES {
        N = 0, A = 1, C = 2, G = 3, T = 4, DOT = 5, HYPHEN = 6, IGNORE = 255
    };

    void init();
    void fillTables();
};

#endif /* DNA5_ALPHABET_SPECIFICS_H_ */
