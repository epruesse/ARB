#ifndef __RNS_H
#define __RNS_H

#ifndef __DEFINES_H
    #include "defines.h"
#endif
#ifndef __BASE_H
    #include "base.h"
#endif
#ifndef __SIMCFG_H
    #include "simcfg.h"
#endif
#ifndef __FRAND_H
    #include "frand.h"
#endif

typedef double SingleProb[BASETYPES];
typedef double DoubleProb[BASETYPES][BASETYPES];

/* /------------------------\ */
/* |  Erzeugung der Ur-RNS  | */
/* \------------------------/ */

extern int        orgLen;
extern double     orgHelixPart;

/* /------------\ */
/* |  Mutation  | */
/* \------------/ */

extern int    timeSteps;
extern Frand  mrpb_Init,
              l2hrpb_Init,
              pairPart,
              mutationRate,
              splitRate,
              helixGcDruck,
              helixGcRate,
              helixAtRate,
              loopGcDruck,
              loopGcRate,
              loopAtRate;
extern double transitionRate,
              transversionRate;

/* /----------------------\ */
/* |  Ausgabefilepointer  | */
/* \----------------------/ */

extern FILE *topo,
            *seq;

/* /----------------\ */
/* |  Eine Species  | */
/* \----------------/ */

typedef struct S_RNS
{
    char *base;    /* Array der Basen */
    int   bases,   /* Anzahl Basen */
          helix,   /* Anzahl Basenpaare in helikalen Bereichen */
          pairing, /* Anzahl paarender Basenpaare (G-C und A-T) */
          laufNr;  /* erhîht sich mit jeder neuen RNS (fÅr Namensvergabe) */

} *RNS;

__PROTOTYPEN__

    RNS  createOriginRNS (void);
    void freeRNS         (RNS rns);
    void splitRNS        (int no_of_father, RNS origin, double age, int steps, int depth);

    void dumpDepths      (void);

__PROTOENDE__

#endif
