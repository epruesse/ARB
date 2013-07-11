#ifndef READCFG_H
#define READCFG_H

#ifndef DEFINES_H
#include "defines.h"
#endif

//  decodeFunc bekommt einen Zeiger auf den Text hinter dem
//  Schluesselwort und einen Zeiger auf eine zu setzende lokale Variable
/* */
//  Rueckgabewerte:
/* */
//  0   =   Angabe war falsch (in diesem Fall wird eine ggf. mit der Funktion
//                             setCfgError() gesetzte Fehlermeldung ausgeben)
//  1   =   Angabe war korrekt (Das Schluesselwort darf nicht mehrfach
//                              verwendet werden)
//  2   =   Angabe war korrekt (Das Schluesselwort darf mehrfach
//                              verwendet werden)

typedef int (*decodeFunc)(str afterKeyword, void *varPointer);


typedef struct S_cfgLine
{
    cstr        keyword,
                defaultVal;
    decodeFunc  decode;
    void       *varPointer;
    cstr        description;

} *cfgLine;


#ifdef __cplusplus
extern "C" {
#endif

    // Das 'keyword' des letzten Elements des Arrays 'line' muss NULL sein!

    int  readCfg     (cstr fname, struct S_cfgLine line[]);

    // Optional kann hiermit eine Nachricht ausgegeben werden,
    // falls das Decodieren fehlschlaegt

    void setCfgError (cstr message);

#ifdef __cplusplus
}
#endif

#endif
