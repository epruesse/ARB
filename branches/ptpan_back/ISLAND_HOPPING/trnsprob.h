// ============================================================= //
//                                                               //
//   File      : trnsprob.h                                      //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef TRNSPROB_H
#define TRNSPROB_H


#define MAXEDGE 1.2

#define ATOLPARAM 0.0001
#define RTOLPARAM 0.001

enum {T,C,A,G,N,N1,N2};
enum {REV,TN93,HKY85};

char encodeBase(char c);
char decodeBase(char c);

void normalizeBaseFreqs(double *F,double fT,double fC,double fA,double fG);
void normalizeRateParams(double *X,double a,double b,double c,double d,double e,double f);

void initTrnsprob(double ****PP);
void uninitTrnsprob(double ****PP);

void getTrnsprob(double **P,double ***PP,double d);

void updateTrnsprob(double ***PP,double *F,double *X,short M);


#else
#error trnsprob.h included twice
#endif // TRNSPROB_H
