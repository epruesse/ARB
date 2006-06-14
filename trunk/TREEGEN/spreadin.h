#ifndef __SPREADIN_H
#define __SPREADIN_H

#ifndef __DEFINES_H
#include "defines.h"
#endif

/* Ein Spreading ist eine Tabelle von Integerwerten, welche den Bereich */
/* 0 bis RAND_MAX-1 abdeckt. */
/* */
/* Aus einer Wahrscheinlichkeitstabelle (z.B. loopProb oder helixProb) */
/* wird hier eine Integertabelle mit Grenzwerten erzeugt um Flieskommarechnung */
/* zu umgehen. */
/* */
/* Die Funktion spreadRand() liefert analog zu den Wahrscheinlichkeiten in */
/* der Wahrscheinlichkeitstabelle verteilte Werte zwischen 0 und der */
/* Anzahl der EIntr„ge in der Wahrscheinlichkeitstabelle. */

typedef struct S_Spreading
{
    int  values, /* Anzahl Werte */
        *border; /* Die Grenzwerte */

} *Spreading;

__PROTOTYPEN__

    Spreading   newSpreading    (double *value, int values);
    void        freeSpreading   (Spreading s);

    int         spreadRand      (Spreading s);

__PROTOENDE__

#endif
