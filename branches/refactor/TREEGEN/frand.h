#ifndef FRAND_H
#define FRAND_H

#ifndef DEFINES_H
#include "defines.h"
#endif

typedef struct S_Frand
{
    double val,    /* Interner Wert des Niederfrequenzgenerators */
           alpha,  /* Faktor f�r konstanten Zufallsgenerator */
           beta,   /* Faktor f�r niederfrequenten Zufallsgenerator */
           medium; /* Mittelwert */
    int    teiler; /* Teiler f�r Niederfrequenzgenerator */

} *Frand;

/* Um den Frequenzgenerator an der selben Stelle wiederaufzusetzen, */
/* muss der Wert 'val' gemerkt und spaeter wiedereingesetzt werden. */

#ifdef __cplusplus
extern "C" {
#endif

    Frand  initFrand (double medium, double low, double high);
    void   freeFrand (Frand f);

    double getFrand  (Frand f);

    /* Zufallszahl (0.0 bis 1.0): */

    double randProb  ();

#ifdef __cplusplus
}
#endif

#endif
