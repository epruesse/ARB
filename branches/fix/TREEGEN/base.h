#ifndef BASE_H
#define BASE_H

#ifndef DEFINES_H
#include "defines.h"
#endif

enum BaseType {
    BASE_A,
    BASE_C,
    BASE_G,
    BASE_T,
    BASE_DEL

};

#define BASETYPES           4
#define BASECHARS           (BASETYPES+1)
#define BASEQUAD            (BASETYPES*BASETYPES)

#define MAXBASECHAR         ((int)'t')    // vom ASCII-Wert her groesstes Zeichen
#define PROB_NOT_DEF        (-1.0)

#define isDeleted(b)        (charIsDelete[(int)(b)])
#define isHelical(b)        (charIsHelical[(int)(b)])
#define isPairing(b1, b2)   (basesArePairing[(int)(b1)][(int)(b2)])

extern char helixBaseChar[BASECHARS],
            loopBaseChar[BASECHARS];
extern int  basesArePairing[BASECHARS][BASECHARS], // Kombination paarend?
            baseCharType[],
            charIsDelete[],
            charIsHelical[];

#define char2BaseType(c)    ((enum BaseType)baseCharType[(int)c])

#ifdef __cplusplus
extern "C" {
#endif

    void initBaseLookups ();

#ifdef __cplusplus
}
#endif

#endif
