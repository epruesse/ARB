#include "f2c.h"
extern "C" {
double d_sign(doublereal *a, doublereal *b)
{
double x;
x = (*a >= 0 ? *a : - *a);
return( *b >= 0 ? x : -x);
}
}
