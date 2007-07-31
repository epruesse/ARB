#ifndef __FRAND_H
#define __FRAND_H

#ifndef __DEFINES_H
#include "defines.h"
#endif

typedef struct S_Frand
{
    double val,    /* Interner Wert des Niederfrequenzgenerators */
           alpha,  /* Faktor fÅr konstanten Zufallsgenerator */
           beta,   /* Faktor fÅr niederfrequenten Zufallsgenerator */
           medium; /* Mittelwert */
    int    teiler; /* Teiler fÅr Niederfrequenzgenerator */

} *Frand;

/* Um den Frequenzgenerator an der selben Stelle wiederaufzusetzen, */
/* mu· der Wert 'val' gemerkt und spÑter wiedereingesetzt werden. */

__PROTOTYPEN__

    Frand  initFrand (double medium, double low, double high);
    void   freeFrand (Frand f);

    double getFrand  (Frand f);

    /* Zufallszahl (0.0 bis 1.0): */

    double randProb  (void);

__PROTOENDE__

#endif
