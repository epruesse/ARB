#include "frand.h"
#include <math.h>
#include <stdlib.h>

static double randval()
{
    //  Liefert einen Zufallswert zwischen -0.5 und +0.5
    double val = rand();

    val /= RAND_MAX;
    val -= 0.5;

    assert(val>=-0.5);
    assert(val <= 0.5);

    return val;
}
static double lowfreqrandval(double *val, int teiler)
{
    //  Liefert einen niederfrequenten Zufallswert zwischen -0.5 und +0.5
    double add = randval()/teiler;

    *val += add;
    if (*val<-0.5 || *val>0.5) *val -= 2*add;

    return *val;
}
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
double getFrand(Frand f)
{
    return f->medium +
           f->alpha  * randval() +
           f->beta   * lowfreqrandval(&(f->val), f->teiler);
}
void freeFrand(Frand f)
{
    free(f);
}
double randProb()
{
    //  Liefert einen Zufallswert zwischen 0.0 und 1.0
    double val = rand();

    val /= RAND_MAX;

    assert(val>=0.0);
    assert(val<=1.0);

    return val;
}

