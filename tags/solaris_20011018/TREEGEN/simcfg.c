#include "simcfg.h"

#ifndef __READCFG_H
    #include "readcfg.h"
#endif
#ifndef __MATH_H
    #include <math.h>
#endif
#ifndef __STDLIB_H
    #include <stdlib.h>
#endif
#ifndef __STRING_H
    #include <string.h>
#endif
#ifndef __RNS_H
    #include "rns.h"
#endif

extern struct S_cfgLine cfg_lines[];

/* -------------------------------------------------------------------------- */
/*      static int decodeFrand(str setting, void *frandPtr) */
/* ------------------------------------------------------ 20.05.95 18.19 ---- */
static int decodeFrand(str setting, void *frandPtr)
{
    str medium = strtok(setting, " "),
        low    = strtok(NULL, " "),
        high   = strtok(NULL, " ");

    if (medium && low && high)
    {
        double med_val  = atof(medium),
               low_val  = atof(low),
               high_val = atof(high);

        if (med_val<=0.0 || med_val>=1.0)
        {
            setCfgError("Der Mittelwert mu· ZWISCHEN 0.0 und 1.0 liegen");
            return 0;
        }
        else
        {
            double change = fabs(low_val)+fabs(high_val);

            if (change>=med_val || change>=(1.0-med_val))
            {
                setCfgError("Die Summe von nieder- und hochfrequentem Anteil "
                            "ist zu gro· und erreicht eine der Grenzen 0.0 "
                            "bzw. 1.0");

                return 0;
            }
        }

        *((Frand*)frandPtr) = initFrand(med_val, low_val, high_val);

        return 1;
    }

    setCfgError("Syntax lautet: <Mittelwert> <niederfrequenter Anteil> <hochfrequenter Anteil>");

    return 0;
}
/* -------------------------------------------------------------------------- */
/*      static int decodeInt(str setting, void *intPtr) */
/* ------------------------------------------------------ 20.05.95 12.33 ---- */
static int decodeInt(str setting, void *intPtr)
{
    *((int*)intPtr) = atoi(setting);
    return 1;
}
/* -------------------------------------------------------------------------- */
/*      static int decodeProb(str setting, void *doublePtr) */
/* ------------------------------------------------------ 17.05.95 21.55 ---- */
static int decodeProb(str setting, void *doublePtr)
{
    double *dptr = (double*)doublePtr;

    *dptr = atof(setting);

    if (*dptr<0.0 || *dptr>1.0)
    {
        setCfgError("Wahrscheinlichkeit mu· zwischen 0.0 und 1.0 liegen");
        return 0;
    }

    return 1;
}
/* -------------------------------------------------------------------------- */
/*      void readSimCfg(cstr fname) */
/* ------------------------------------------------------ 17.05.95 22:10 ---- */
void readSimCfg(cstr fname)
{
    int lenTeiler,
        stepTeiler;

    if (!readCfg(fname, cfg_lines)) errorf("Fehler beim Lesen von '%s'", fname);

    lenTeiler  = (int)sqrt(orgLen);
    stepTeiler = (int)sqrt(timeSteps);

    mrpb_Init->teiler    = lenTeiler;
    l2hrpb_Init->teiler  = lenTeiler;
    pairPart->teiler     = stepTeiler;
    mutationRate->teiler = stepTeiler;
    splitRate->teiler    = stepTeiler;
    helixGcDruck->teiler = stepTeiler;
    helixGcRate->teiler  = stepTeiler;
    helixAtRate->teiler  = stepTeiler;
    loopGcDruck->teiler  = stepTeiler;
    loopGcRate->teiler   = stepTeiler;
    loopAtRate->teiler   = stepTeiler;
}

static struct S_cfgLine cfg_lines[] =
{

    /* /---------------------------------------\ */
    /* |  Nur zur Initialisierung notwendig :  | */
    /* \---------------------------------------/ */

    { "OriginLen",          "3000",             decodeInt,          &orgLen,            "Anzahl Basen im der Ur-RNS" },
    { "OriginHelixPart",    "0.5",              decodeProb,         &orgHelixPart,      "Anteil helikaler Bereiche (in der Ur-RNS)" },
    { "MutRatePerBase",     "0.5 0.01 0.4",     decodeFrand,        &mrpb_Init,         "Mutationsrate pro Basenposition (wird nur bei Initialisierung verwendet)" },
    { "Loop2HelixRate",     "0.2 0.01 0.1",     decodeFrand,        &l2hrpb_Init,       "Rate pro Basenposition mit der Loop- in Helix-Bereiche Åbergehen und vv. (wird nur bei Initialisierung verwendet)" },
    { "TimeSteps",          "50",               decodeInt,          &timeSteps,         "Anzahl Zeitschritte" },
    { "TransitionRate",     "0.5",              decodeProb,         &transitionRate,    "Transition-Rate" },
    { "TransversionRate",   "0.5",              decodeProb,         &transversionRate,  "Transversion-Rate" },

    /* /-----------------------------------------------------------------\ */
    /* |  Parameter, welche sich wÑhrend des Baumdurchlaufs verÑndern :  | */
    /* \-----------------------------------------------------------------/ */

    { "PairPart",           "0.85 0.1 0.01",    decodeFrand,        &pairPart,          "Gewuenschter Anteil paarender Helix-Bindungen (Mittelwert, Anteil durch niedr. Freq, hohe Freq.)" },
    { "MutationRate",       "0.01 0.005 0.001", decodeFrand,        &mutationRate,      "Mutationsrate" },
    { "SplitProb",          "0.2 0.1 0.01",     decodeFrand,        &splitRate,         "Spaltungsrate" },
    { "Helix-GC-Druck",     "0.72 0.11 0.01",   decodeFrand,        &helixGcDruck,      "Anteil der G-C Bindungen im Helixbereich" },
    { "Helix-GC-Rate",      "0.5 0.001 0.001",  decodeFrand,        &helixGcRate,       "VerhÑltniss G:C im Helixbereich" },
    { "Helix-AT-Rate",      "0.5 0.001 0.001",  decodeFrand,        &helixAtRate,       "VerhÑltniss A:T im Helixbereich" },
    { "Loop-GC-Druck",      "0.62 0.05 0.01",   decodeFrand,        &loopGcDruck,       "Anteil der G-C Bindungen im Loopbereich" },
    { "Loop-GC-Rate",       "0.5 0.001 0.001",  decodeFrand,        &loopGcRate,        "VerhÑltniss G:C im Loopbereich" },
    { "Loop-AT-Rate",       "0.5 0.001 0.001",  decodeFrand,        &loopAtRate,        "VerhÑltniss A:T im Loopbereich" },

/*    { "", "", decode, &, "" }, */

    { NULL }
};

