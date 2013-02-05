#ifndef RNS_H
#define RNS_H

#ifndef BASE_H
#include "base.h"
#endif
#ifndef SIMCFG_H
#include "simcfg.h"
#endif
#ifndef FRAND_H
#include "frand.h"
#endif

typedef double SingleProb[BASETYPES];
typedef double DoubleProb[BASETYPES][BASETYPES];

// -----------------------------
//      Erzeugung der Ur-RNS

extern int        orgLen;
extern double     orgHelixPart;

// -----------------
//      Mutation

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

// ---------------------------
//      Ausgabefilepointer

extern FILE *topo,
            *seq;

// ---------------------
//      Eine Species

typedef struct S_RNS
{
    char *base;    // Array der Basen
    int   bases,   // Anzahl Basen
          helix,   // Anzahl Basenpaare in helikalen Bereichen
          pairing, // Anzahl paarender Basenpaare (G-C und A-T)
          laufNr;  // erhoeht sich mit jeder neuen RNS (fuer Namensvergabe)

} *RNS;

#ifdef __cplusplus
extern "C" {
#endif

    RNS  createOriginRNS ();
    void freeRNS         (RNS rns);
    void splitRNS        (int no_of_father, RNS origin, double age, int steps, int depth);

    void dumpDepths      ();

#ifdef __cplusplus
}
#endif

#endif
