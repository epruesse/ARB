#ifndef READCFG_H
#define READCFG_H

#ifndef DEFINES_H
#include "defines.h"
#endif

/*  decodeFunc bekommt einen Zeiger auf den Text hinter dem */
/*  Schl�sselwort und einen Zeiger auf eine zu setzende lokale Variable */
/* */
/*  R�ckgabewerte: */
/* */
/*  0   =   Angabe war falsch (in diesem Fall wird eine ggf. mit der Funktion */
/*                             setCfgError() gesetzte Fehlermeldung ausgeben) */
/*  1   =   Angabe war korrekt (Das Schl�sselwort darf nicht mehrfach */
/*                              verwendet werden) */
/*  2   =   Angabe war korrekt (Das Schl�sselwort darf mehrfach */
/*                              verwendet werden) */

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

    /* Das 'keyword' des letzten Elements des Arrays 'line' mu� NULL sein! */

    int  readCfg     (cstr fname, struct S_cfgLine line[]);

    /* Optional kann hiermit eine Nachricht ausgegeben werden, */
    /* falls das Decodieren fehlschl�gt */

    void setCfgError (cstr message);

#ifdef __cplusplus
}
#endif

#endif
