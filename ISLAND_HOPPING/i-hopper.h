#define BORLAND
/* #define MICROSOFT */
/* #define LINUX */

#ifdef EXTERN
#define EXT extern
#else
#define EXT
#endif

#include "defs.h"

EXT const char *Error;

void Align(
           int nX,char X[],int secX[],char **XX,int nY,char Y[],int secY[],char **YY,
           int freqs,double fT,double fC,double fA,double fG,
           double rTC,double rTA,double rTG,double rCA,double rCG,double rAG,
           double dist,double supp,double gapA,double gapB,double gapC,double thres
           );

#undef EXT


