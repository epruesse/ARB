#include "base.h"
#include <ctype.h>

char helixBaseChar[BASECHARS] = { 'A', 'C', 'G', 'T', '=' },
     loopBaseChar[BASECHARS]  = { 'a', 'c', 'g', 't', '-' };

int basesArePairing[BASECHARS][BASECHARS] =
{

/*    A  C  G  T  - */

    { 0, 0, 0, 1, 0 }, /* A */
    { 0, 0, 1, 0, 0 }, /* C */
    { 0, 1, 0, 0, 0 }, /* G */
    { 1, 0, 0, 0, 0 }, /* T */
    { 0, 0, 0, 0, 0 }, /* - */
};

int baseCharType[MAXBASECHAR+1],
    charIsDelete[MAXBASECHAR+1],
    charIsHelical[MAXBASECHAR+1];

/* -------------------------------------------------------------------------- */
/*      void initBaseLookups(void) */
/* ------------------------------------------------------ 25.05.95 02.07 ---- */
void initBaseLookups(void)
{
    int b;

    for (b = 0; b<MAXBASECHAR; b++)
    {
        baseCharType[b]  = -1;
        charIsDelete[b]  = 0;
        charIsHelical[b] = 0;
    }

    for (b = 0; b<BASECHARS; b++)
    {
        int c1 = helixBaseChar[b],
            c2 = loopBaseChar[b];

        assert(c1<=MAXBASECHAR);
        assert(c2<=MAXBASECHAR);

        baseCharType[c1]  = b;
        baseCharType[c2]  = b;
        charIsHelical[c1] = 1;
    }
}
