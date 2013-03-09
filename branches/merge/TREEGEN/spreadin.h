#ifndef SPREADIN_H
#define SPREADIN_H

#ifndef DEFINES_H
#include "defines.h"
#endif

// Ein Spreading ist eine Tabelle von Integerwerten, welche den Bereich
// 0 bis RAND_MAX-1 abdeckt.
/* */
// Aus einer Wahrscheinlichkeitstabelle (z.B. loopProb oder helixProb)
// wird hier eine Integertabelle mit Grenzwerten erzeugt um Flieskommarechnung
// zu umgehen.
/* */
// Die Funktion spreadRand() liefert analog zu den Wahrscheinlichkeiten in
// der Wahrscheinlichkeitstabelle verteilte Werte zwischen 0 und der
// Anzahl der EIntraege in der Wahrscheinlichkeitstabelle.

typedef struct S_Spreading
{
    int  values, // Anzahl Werte
        *border; // Die Grenzwerte

} *Spreading;

#ifdef __cplusplus
extern "C" {
#endif

    Spreading   newSpreading    (double *value, int values);
    void        freeSpreading   (Spreading s);

    int         spreadRand      (Spreading s);

#ifdef __cplusplus
}
#endif

#endif
