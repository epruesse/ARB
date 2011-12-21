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
        res = (double *)GB_calloc(sizeof(double), max_n);
        for (i=1; i<max_n; i++) {
            sum += log((double)i);
            res[i] = sum;
        }
    }
    return res[n];
}

// ----------------------------------
//      random number generation

static int randomSeeded = 0;

int GB_random(int range) {
    // produces a random number in range [0 .. range-1]
    if (!randomSeeded) {
        srand(time(0));
        randomSeeded = 1;
    }

#if defined(DEBUG)
    if (range>RAND_MAX) {
        printf("Warning: range to big for random granularity (%i > %i)\n", range, RAND_MAX);
    }
#endif // DEBUG

    return (int)(rand()*((double)range) / (RAND_MAX+1.0));
}
