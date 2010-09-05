#include "frand.h"
#include "defines.h"
#include <math.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------- */
/*      static double randval(void) */
/* ------------------------------------------------------ 16.05.95 20.17 ---- */
/* */
/*  Liefert einen Zufallswert zwischen -0.5 und +0.5 */
/* */
static double randval(void)
{
    double val = rand();

    val /= RAND_MAX;
    val -= 0.5;

    assert(val>=-0.5);
    assert(val<= 0.5);

    return val;
}
/* -------------------------------------------------------------------------- */
/*      static double lowfreqrandval(double *val) */
/* ------------------------------------------------------ 16.05.95 20.17 ---- */
/* */
/*  Liefert einen niederfrequenten Zufallswert zwischen -0.5 und +0.5 */
/* */
static double lowfreqrandval(double *val, int teiler)
{
    double add = randval()/teiler;

    *val += add;
    if (*val<-0.5 || *val>0.5) *val -= 2*add;

    return *val;
}
/* -------------------------------------------------------------------------- */
/*      Frand initFrand(double medium, double low, double high) */
/* ------------------------------------------------------ 17.05.95 15:35 ---- */
Frand initFrand(double medium, double low, double high)
{
    Frand f = (Frand)malloc(sizeof(*f));

    if (!f) outOfMemory();

    f->medium = medium;
    f->alpha  = high*2;
    f->beta   = low*2;
    f->val    = randval();
    f->teiler = 1;

    return f;
}
/* -------------------------------------------------------------------------- */
/*      double getFrand(Frand f) */
/* ------------------------------------------------------ 17.05.95 15:35 ---- */
double getFrand(Frand f)
{
    return f->medium +
           f->alpha  * randval() +
           f->beta   * lowfreqrandval(&(f->val), f->teiler);
}
/* -------------------------------------------------------------------------- */
/*      void freeFrand(Frand f) */
/* ------------------------------------------------------ 17.05.95 15.36 ---- */
void freeFrand(Frand f)
{
    free(f);
}
/* -------------------------------------------------------------------------- */
/*      double randProb(void) */
/* ------------------------------------------------------ 16.05.95 20.17 ---- */
/* */
/*  Liefert einen Zufallswert zwischen 0.0 und 1.0 */
/* */
double randProb(void)
{
    double val = rand();

    val /= RAND_MAX;

    assert(val>=0.0);
    assert(val<=1.0);

    return val;
}

