// =============================================================== //
//                                                                 //
//   File      : admath.cxx                                        //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <cmath>
#include <ctime>

#include "gb_local.h"

double GB_log_fak(int n) {
    // returns log(n!)
    static int     max_n = 0;
    static double *res   = 0;

    if (n<=1) return 0.0;       // log 1 = 0

    if (n >= max_n) {
        double sum = 0;
        int i;
        freenull(res);
        max_n = n + 100;
        res = (double *)ARB_calloc(sizeof(double), max_n);
        for (i=1; i<max_n; i++) {
            sum += log((double)i);
            res[i] = sum;
        }
    }
    return res[n];
}

// ----------------------------------
//      random number generation

static int      randomSeeded = 0;
static unsigned usedSeed     = 0;

void GB_random_seed(unsigned seed) {
    /*! normally you should not use GB_random_seed.
     * Use it only to reproduce some result (e.g. in unittests)
     */
    usedSeed     = seed;
    srand(seed);
    randomSeeded = 1;
}

int GB_random(int range) {
    // produces a random number in range [0 .. range-1]
    if (!randomSeeded) GB_random_seed(time(0));

#if defined(DEBUG)
    if (range>RAND_MAX) {
        printf("Warning: range to big for random granularity (%i > %i)\n", range, RAND_MAX);
    }
#endif // DEBUG

    return (int)(rand()*((double)range) / (RAND_MAX+1.0));
}
