#ifndef __FRAND_H
#define __FRAND_H

#ifndef __DEFINES_H
#include "defines.h"
#endif

typedef struct S_Frand
{
    double val,    /* Interner Wert des Niederfrequenzgenerators */
           alpha,  /* Faktor fr konstanten Zufallsgenerator */
           beta,   /* Faktor fr niederfrequenten Zufallsgenerator */
           medium; /* Mittelwert */
    int    teiler; /* Teiler fr Niederfrequenzgenerator */

} *Frand;

/* Um den Frequenzgenerator an der selben Stelle wiederaufzusetzen, */
/* muá der Wert 'val' gemerkt und sp„ter wiedereingesetzt werden. */

#ifdef __cplusplus
extern "C" {
#endif    

    Frand  initFrand (double medium, double low, double high);
    void   freeFrand (Frand f);

    double getFrand  (Frand f);

    /* Zufallszahl (0.0 bis 1.0): */

    double randProb  (void);

#ifdef __cplusplus
}
#endif    

#endif
