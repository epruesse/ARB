#define  __MODUL__
#include "spreadin.h"
#include <stdlib.h>

/* -------------------------------------------------------------------------- */
/*        Spreading newSpreading(double *value, int values) */
/* ------------------------------------------------------ 20.05.95 16:42 ---- */
Spreading newSpreading(double *value, int values)
{
    Spreading s;
    int       v;
    double    val = 0.0,
              sum = 0.0;

    s = malloc(sizeof(struct S_Spreading));
    if (!s) outOfMemory();

    assert(values>0);

    s->values = values;
    s->border = malloc(sizeof(*(s->border))*values);
    if (!s->border) outOfMemory();

    for (v = 0; v<values; v++) sum += value[v];      /* Summe bilden */

    for (v = 0; v<values; v++)
    {
        val          += value[v];
        s->border[v]  = (val/sum)*RAND_MAX;
    }

    s->border[values-1] = RAND_MAX;

/*    for (v = 0; v<values; v++) printf("%f -> %i\n", value[v], s->border[v]); */
/*    printf("---\n"); */

    return s;
}
/* -------------------------------------------------------------------------- */
/*        void freeSpreading(Spreading s) */
/* ------------------------------------------------------ 20.05.95 16:42 ---- */
void freeSpreading(Spreading s)
{
    free(s->border);
    free(s);
}
/* -------------------------------------------------------------------------- */
/*        int spreadRand(Spreading s) */
/* ------------------------------------------------------ 20.05.95 16:42 ---- */
int spreadRand(Spreading s)
{
    int val = rand(),
        l   = 0,
        h   = s->values-1,
        m;

    while (1)
    {
        m = (l+h)/2;

        if (val<=s->border[m])
        {
            if (m==0 || val>s->border[m-1]) break;
            h = m;
        }
        else
        {
            if (m==(h-1))
            {
                if (val<=s->border[h])
                {
                    m = h;
                    break;
                }
            }

            l = m;
        }
    }

    assert(m>=0 && m<s->values);
    assert(val<=s->border[m]);
    assert(m==0 || val>=s->border[m-1]);

    return m;
}


