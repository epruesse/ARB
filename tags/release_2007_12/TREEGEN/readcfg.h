#ifndef __READCFG_H
#define __READCFG_H

#ifndef __DEFINES_H
#include "defines.h"
#endif

/*  decodeFunc bekommt einen Zeiger auf den Text hinter dem */
/*  SchlÅsselwort und einen Zeiger auf eine zu setzende lokale Variable */
/* */
/*  RÅckgabewerte: */
/* */
/*  0   =   Angabe war falsch (in diesem Fall wird eine ggf. mit der Funktion */
/*                             setCfgError() gesetzte Fehlermeldung ausgeben) */
/*  1   =   Angabe war korrekt (Das SchlÅsselwort darf nicht mehrfach */
/*                              verwendet werden) */
/*  2   =   Angabe war korrekt (Das SchlÅsselwort darf mehrfach */
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


__PROTOTYPEN__

    /* Das 'keyword' des letzten Elements des Arrays 'line' mu· NULL sein! */

    int  readCfg     (cstr fname, struct S_cfgLine line[]);

    /* Optional kann hiermit eine Nachricht ausgegeben werden, */
    /* falls das Decodieren fehlschlÑgt */

    void setCfgError (cstr message);

__PROTOENDE__

#endif
