#include "simcfg.h"
#include "readcfg.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "rns.h"

static struct S_cfgLine cfg_lines[];

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
            setCfgError("The average value has to be BETWEEN 0.0 and 1.0");
            return 0;
        }
        else
        {
            double change = fabs(low_val)+fabs(high_val);

            if (change>=med_val || change>=(1.0-med_val))
            {
                setCfgError("The sum of low and high frequent part is too big and "
                            "reaches one of the borders of the allowed range ]0.0, 1.1[");

                return 0;
            }
        }

        *((Frand*)frandPtr) = initFrand(med_val, low_val, high_val);

        return 1;
    }

    setCfgError("Syntax is: <meanvalue> <lowfreqpart> <highfreqpart>");

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
        setCfgError("Probability has to be between 0.0 and 1.0");
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

    if (!readCfg(fname, cfg_lines)) errorf("Error reading config '%s'", fname);

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

    { "OriginLen",          "3000",             decodeInt,          &orgLen,            "Number of base positions in origin species" },
    { "OriginHelixPart",    "0.5",              decodeProb,         &orgHelixPart,      "size of helical part in origin species (0.5 means 50% helix and 50% loop regions)" },
    { "MutRatePerBase",     "0.5 0.01 0.4",     decodeFrand,        &mrpb_Init,         "mutation rate per base position (used for origin only)" },
    { "Loop2HelixRate",     "0.2 0.01 0.1",     decodeFrand,        &l2hrpb_Init,       "loop<->helix conversion rate per base position (used for origin only)" },
    { "TimeSteps",          "50",               decodeInt,          &timeSteps,         "number of time steps" },
    { "TransitionRate",     "0.5",              decodeProb,         &transitionRate,    "transition rate" },
    { "TransversionRate",   "0.5",              decodeProb,         &transversionRate,  "transversion rate" },

    /* /-----------------------------------------------------------------\ */
    /* |  Parameter, welche sich w„hrend des Baumdurchlaufs ver„ndern :  | */
    /* \-----------------------------------------------------------------/ */

    { "PairPart",           "0.85 0.1 0.01",    decodeFrand,        &pairPart,          "part of pairing helix positions (mean value, low frequent part, high frequent part)" },
    { "MutationRate",       "0.01 0.005 0.001", decodeFrand,        &mutationRate,      "mutation rate" },
    { "SplitProb",          "0.2 0.1 0.01",     decodeFrand,        &splitRate,         "split rate (split into two species)" },
    { "Helix-GC-Pressure",  "0.72 0.11 0.01",   decodeFrand,        &helixGcDruck,      "part of G-C bonds in helical regions" },
    { "Helix-GC-Rate",      "0.5 0.001 0.001",  decodeFrand,        &helixGcRate,       "G:C rate in helical regions" },
    { "Helix-AT-Rate",      "0.5 0.001 0.001",  decodeFrand,        &helixAtRate,       "A:T rate in helical regions" },
    { "Loop-GC-Pressure",   "0.62 0.05 0.01",   decodeFrand,        &loopGcDruck,       "part of G-C bonds in loop regions" },
    { "Loop-GC-Rate",       "0.5 0.001 0.001",  decodeFrand,        &loopGcRate,        "G:C rate in loop regions" },
    { "Loop-AT-Rate",       "0.5 0.001 0.001",  decodeFrand,        &loopAtRate,        "A:T rate in loop regions" },

/*    { "", "", decode, &, "" }, */

    { NULL, 0, 0, 0, 0 }
};

