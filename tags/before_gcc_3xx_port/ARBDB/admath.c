#include <stdlib.h>
#include <stdio.h>
/* #include <malloc.h> */
#include <string.h>
#include <math.h>

/*#include <arbdb.h>*/
#include "adlocal.h"

double GB_log_fak(int n){
    /* returns log(n!) */
    static int max_n = 0;
    static double *res = 0;
    if (n<=1) return 0.0;	/* log 1 = 0 */

    if (n >= max_n){
	double sum = 0;
	int i;
	GB_DELETE(res);
	max_n = n + 100;
	res = (double *)GB_calloc(sizeof(double),max_n);
	for (i=1;i<max_n;i++){
	    sum += log((double)i);
	    res[i] = sum;
	}
    }
    return res[n];
}




