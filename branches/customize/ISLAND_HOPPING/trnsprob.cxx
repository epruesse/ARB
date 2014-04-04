// =============================================================
/*                                                               */
//   File      : trnsprob.c
//   Purpose   :
/*                                                               */
//   Institute of Microbiology (Technical University Munich)
//   http://www.arb-home.de/
/*                                                               */
// =============================================================

#include "trnsprob.h"
#include "defs.h"
#include "mem.h"

#include <arb_simple_assert.h>
#include <stdio.h>

#define ih_assert(bed) arb_assert(bed)
#define EPS 0.00001

//============================================================================

static void identity(double **i) {
    i[T][T]=1; i[T][C]=0; i[T][A]=0; i[T][G]=0;
    i[C][T]=0; i[C][C]=1; i[C][A]=0; i[C][G]=0;
    i[A][T]=0; i[A][C]=0; i[A][A]=1; i[A][G]=0;
    i[G][T]=0; i[G][C]=0; i[G][A]=0; i[G][G]=1;
}

//............................................................................

static void copy(double **i,double **j) {
    i[T][T]=j[T][T]; i[T][C]=j[T][C]; i[T][A]=j[T][A]; i[T][G]=j[T][G];
    i[C][T]=j[C][T]; i[C][C]=j[C][C]; i[C][A]=j[C][A]; i[C][G]=j[C][G];
    i[A][T]=j[A][T]; i[A][C]=j[A][C]; i[A][A]=j[A][A]; i[A][G]=j[A][G];
    i[G][T]=j[G][T]; i[G][C]=j[G][C]; i[G][A]=j[G][A]; i[G][G]=j[G][G];
}

//............................................................................

static void ipol(double **i,double **j,double **k,double f) { double g; g=1.0-f;
    i[T][T]=g*j[T][T]+f*k[T][T]; i[T][C]=g*j[T][C]+f*k[T][C]; i[T][A]=g*j[T][A]+f*k[T][A]; i[T][G]=g*j[T][G]+f*k[T][G];
    i[C][T]=g*j[C][T]+f*k[C][T]; i[C][C]=g*j[C][C]+f*k[C][C]; i[C][A]=g*j[C][A]+f*k[C][A]; i[C][G]=g*j[C][G]+f*k[C][G];
    i[A][T]=g*j[A][T]+f*k[A][T]; i[A][C]=g*j[A][C]+f*k[A][C]; i[A][A]=g*j[A][A]+f*k[A][A]; i[A][G]=g*j[A][G]+f*k[A][G];
    i[G][T]=g*j[G][T]+f*k[G][T]; i[G][C]=g*j[G][C]+f*k[G][C]; i[G][A]=g*j[G][A]+f*k[G][A]; i[G][G]=g*j[G][G]+f*k[G][G];
}

//............................................................................

static void addmul(double **i,double **j,double f) {
    i[T][T]+=j[T][T]*f; i[T][C]+=j[T][C]*f; i[T][A]+=j[T][A]*f; i[T][G]+=j[T][G]*f;
    i[C][T]+=j[C][T]*f; i[C][C]+=j[C][C]*f; i[C][A]+=j[C][A]*f; i[C][G]+=j[C][G]*f;
    i[A][T]+=j[A][T]*f; i[A][C]+=j[A][C]*f; i[A][A]+=j[A][A]*f; i[A][G]+=j[A][G]*f;
    i[G][T]+=j[G][T]*f; i[G][C]+=j[G][C]*f; i[G][A]+=j[G][A]*f; i[G][G]+=j[G][G]*f;
}

//............................................................................

static void dot(double **i,double **j,double **k) {
    i[T][T]=j[T][T]*k[T][T]+j[T][C]*k[C][T]+j[T][A]*k[A][T]+j[T][G]*k[G][T];
    i[T][C]=j[T][T]*k[T][C]+j[T][C]*k[C][C]+j[T][A]*k[A][C]+j[T][G]*k[G][C];
    i[T][A]=j[T][T]*k[T][A]+j[T][C]*k[C][A]+j[T][A]*k[A][A]+j[T][G]*k[G][A];
    i[T][G]=j[T][T]*k[T][G]+j[T][C]*k[C][G]+j[T][A]*k[A][G]+j[T][G]*k[G][G];
    i[C][T]=j[C][T]*k[T][T]+j[C][C]*k[C][T]+j[C][A]*k[A][T]+j[C][G]*k[G][T];
    i[C][C]=j[C][T]*k[T][C]+j[C][C]*k[C][C]+j[C][A]*k[A][C]+j[C][G]*k[G][C];
    i[C][A]=j[C][T]*k[T][A]+j[C][C]*k[C][A]+j[C][A]*k[A][A]+j[C][G]*k[G][A];
    i[C][G]=j[C][T]*k[T][G]+j[C][C]*k[C][G]+j[C][A]*k[A][G]+j[C][G]*k[G][G];
    i[A][T]=j[A][T]*k[T][T]+j[A][C]*k[C][T]+j[A][A]*k[A][T]+j[A][G]*k[G][T];
    i[A][C]=j[A][T]*k[T][C]+j[A][C]*k[C][C]+j[A][A]*k[A][C]+j[A][G]*k[G][C];
    i[A][A]=j[A][T]*k[T][A]+j[A][C]*k[C][A]+j[A][A]*k[A][A]+j[A][G]*k[G][A];
    i[A][G]=j[A][T]*k[T][G]+j[A][C]*k[C][G]+j[A][A]*k[A][G]+j[A][G]*k[G][G];
    i[G][T]=j[G][T]*k[T][T]+j[G][C]*k[C][T]+j[G][A]*k[A][T]+j[G][G]*k[G][T];
    i[G][C]=j[G][T]*k[T][C]+j[G][C]*k[C][C]+j[G][A]*k[A][C]+j[G][G]*k[G][C];
    i[G][A]=j[G][T]*k[T][A]+j[G][C]*k[C][A]+j[G][A]*k[A][A]+j[G][G]*k[G][A];
    i[G][G]=j[G][T]*k[T][G]+j[G][C]*k[C][G]+j[G][A]*k[A][G]+j[G][G]*k[G][G];
}

//============================================================================

char encodeBase(char c) {

    switch(c) {
        case 'U': return T;
        case 'T': return T;
        case 'C': return C;
        case 'A': return A;
        case 'G': return G;
        default : return N;
    }

}

//............................................................................

char decodeBase(char c) {

    switch(c) {
        case T: return 'T';
        case C: return 'C';
        case A: return 'A';
        case G: return 'G';
        case N: return '?';
        default : return '-';
    }

}

//============================================================================

void normalizeBaseFreqs(
                        double *F,double fT,double fC,double fA,double fG
                        ) {
    double s;

    s=fT+fC+fA+fG;

    if(s<REAL_MIN) {fT=1.; fC=1.; fA=1.; fG=1.; s=4.;}

    fT/=s; fC/=s; fA/=s; fG/=s;

    if(fT<ATOLPARAM) fT=ATOLPARAM;
    if(fC<ATOLPARAM) fC=ATOLPARAM;
    if(fA<ATOLPARAM) fA=ATOLPARAM;
    if(fG<ATOLPARAM) fG=ATOLPARAM;

    F[T]=fT; F[C]=fC; F[A]=fA; F[G]=fG;

}

//............................................................................

void normalizeRateParams(
                         double *X,double a,double b,double c,double d,double e,double f
                         ) { // TC TA TG CA CG AG
    double s;

    s=a+b+c+d+e+f;

    if(s<REAL_MIN) {a=1.; b=1.; c=1.; d=1.; e=1.; s=6.;}

    a/=s; b/=s; c/=s; d/=s; e/=s;

    if(a<ATOLPARAM) a=ATOLPARAM;
    if(b<ATOLPARAM) b=ATOLPARAM;
    if(c<ATOLPARAM) c=ATOLPARAM;
    if(d<ATOLPARAM) d=ATOLPARAM;
    if(e<ATOLPARAM) e=ATOLPARAM;

    X[0]=a; X[1]=b; X[2]=c; X[3]=d; X[4]=e;

}

//============================================================================

void initTrnsprob(double ****PP) {
    double ***P; short j;

    P=(double***)newVector(128,sizeof(double **));
    for(j=0;j<128;j++) P[j]=(double **)newMatrix(N,N,sizeof(double));

    *PP=P;

}

//............................................................................

void uninitTrnsprob(double ****PP) {
    double ***P; short j;

    P=*PP;

    for(j=0;j<128;j++) freeMatrix(&P[j]);
    freeBlock(PP);

}

//============================================================================

void getTrnsprob(double **P,double ***PP,double d) {
    long I,J;
    double *Y[N],YT[N],YC[N],YA[N],YG[N];
    double *Z[N],ZT[N],ZC[N],ZA[N],ZG[N];
    double *X[N],XT[N],XC[N],XA[N],XG[N];

    d/=EPS; I=(long)d;

    if( I < 0        ) {copy(P,PP[0]); return;}

    X[T]=XT; X[C]=XC; X[A]=XA; X[G]=XG;

    if( I < 32       ) {ipol(X,PP[I],PP[I==31?33:I+1],d-I); copy(P,X); return;}

    if( I < 1024     ) {
        J=I%32;     ipol(X,PP[J],PP[J==31?33:J+1],d-I);
        dot(P,PP[ I/32    + 32 ],X);
        return;
    }

    Y[T]=YT; Y[C]=YC; Y[A]=YA; Y[G]=YG;

    if( I < 32768   ) {
        J=I%32;     ipol(X,PP[J],PP[J==31?33:J+1],d-I);
        J=I%1024;    dot(Y,PP[ J/32    + 32 ],X);
        dot(P,PP[ I/1024  + 64 ],Y);
        return;
    }

    Z[T]=ZT; Z[C]=ZC; Z[A]=ZA; Z[G]=ZG;

    if( I < 1048576 ) {
        J=I%32;     ipol(X,PP[J],PP[J==31?33:J+1],d-I);
        J=I%1024;    dot(Y,PP[ J/32    + 32 ],X);
        J=I%32768;   dot(Z,PP[ J/1024  + 64 ],Y);
        dot(P,PP[ I/32768 + 96 ],Z);
        return;
    }

    dot(Y,PP[ 31      + 32 ],PP[31]);
    dot(Z,PP[ 31      + 64 ],Y);
    dot(P,PP[ 31      + 96 ],Z);

}

//============================================================================

void updateTrnsprob(double ***PP,double *F,double *X,short M) {
    double s,a,b,c,d,e,f;
    double *Q[N],QT[N],QC[N],QA[N],QG[N];
    double *R[N],RT[N],RC[N],RA[N],RG[N];
    double *S[N],ST[N],SC[N],SA[N],SG[N];

    Q[T]=QT; Q[C]=QC; Q[A]=QA; Q[G]=QG;
    R[T]=RT; R[C]=RC; R[A]=RA; R[G]=RG;
    S[T]=ST; S[C]=SC; S[A]=SA; S[G]=SG;

    switch(M) {
        case REV:   a = X[0]; b=X[1]; c=X[2]; d=X[3]; e=X[4]; f=1.0-(a+b+c+d+e); break;
        case TN93:  a = X[0]; b=c=d=e=X[1]; f=1.0-(a+b+c+d+e); break;
        case HKY85: a = f=X[0]; b=c=d=e=0.25*(1.0-(a+f)); break;
        default : ih_assert(0); break;
    }

    s=0.5/(a*F[T]*F[C]+b*F[T]*F[A]+c*F[T]*F[G]+d*F[C]*F[A]+e*F[C]*F[G]+f*F[A]*F[G]);

    Q[T][C]=a*F[C]*s; Q[T][A]=b*F[A]*s; Q[T][G]=c*F[G]*s;
    Q[C][T]=a*F[T]*s;                   Q[C][A]=d*F[A]*s; Q[C][G]=e*F[G]*s;
    Q[A][T]=b*F[T]*s; Q[A][C]=d*F[C]*s;                   Q[A][G]=f*F[G]*s;
    Q[G][T]=c*F[T]*s; Q[G][C]=e*F[C]*s; Q[G][A]=f*F[A]*s;

    Q[T][T] = - (           Q[T][C] + Q[T][A] + Q[T][G] );
    Q[C][C] = - ( Q[C][T] +           Q[C][A] + Q[C][G] );
    Q[A][A] = - ( Q[A][T] + Q[A][C] +           Q[A][G] );
    Q[G][G] = - ( Q[G][T] + Q[G][C] + Q[G][A]           );

    identity(PP[1]);
    addmul(PP[1],Q,EPS);
    dot(R,Q,Q);
    addmul(PP[1],R,EPS*EPS/2.0);
    dot(S,R,Q);
    addmul(PP[1],S,EPS*EPS*EPS/6.0);
    dot(S,R,R);
    addmul(PP[1],S,EPS*EPS*EPS*EPS/24.0);

    identity(PP[ 0]);
    identity(PP[32]);
    identity(PP[64]);
    identity(PP[96]);

    dot(PP[  2      ],PP[  1      ],PP[  1      ]);
    dot(PP[  4      ],PP[  2      ],PP[  2      ]);
    dot(PP[  8      ],PP[  4      ],PP[  4      ]);
    dot(PP[ 16      ],PP[  8      ],PP[  8      ]);
    dot(PP[  1 + 32 ],PP[ 16      ],PP[ 16      ]);
    dot(PP[  2 + 32 ],PP[  1 + 32 ],PP[  1 + 32 ]);
    dot(PP[  4 + 32 ],PP[  2 + 32 ],PP[  2 + 32 ]);
    dot(PP[  8 + 32 ],PP[  4 + 32 ],PP[  4 + 32 ]);
    dot(PP[ 16 + 32 ],PP[  8 + 32 ],PP[  8 + 32 ]);
    dot(PP[  1 + 64 ],PP[ 16 + 32 ],PP[ 16 + 32 ]);
    dot(PP[  2 + 64 ],PP[  1 + 64 ],PP[  1 + 64 ]);
    dot(PP[  4 + 64 ],PP[  2 + 64 ],PP[  2 + 64 ]);
    dot(PP[  8 + 64 ],PP[  4 + 64 ],PP[  4 + 64 ]);
    dot(PP[ 16 + 64 ],PP[  8 + 64 ],PP[  8 + 64 ]);
    dot(PP[  1 + 96 ],PP[ 16 + 64 ],PP[ 16 + 64 ]);
    dot(PP[  2 + 96 ],PP[  1 + 96 ],PP[  1 + 96 ]);
    dot(PP[  4 + 96 ],PP[  2 + 96 ],PP[  2 + 96 ]);
    dot(PP[  8 + 96 ],PP[  4 + 96 ],PP[  4 + 96 ]);
    dot(PP[ 16 + 96 ],PP[  8 + 96 ],PP[  8 + 96 ]);

    dot(PP[  3      ],PP[  2      ],PP[  1      ]);
    dot(PP[  6      ],PP[  3      ],PP[  3      ]);
    dot(PP[ 12      ],PP[  6      ],PP[  6      ]);
    dot(PP[ 24      ],PP[ 12      ],PP[ 12      ]);
    dot(PP[  3 + 32 ],PP[  2 + 32 ],PP[  1 + 32 ]);
    dot(PP[  6 + 32 ],PP[  3 + 32 ],PP[  3 + 32 ]);
    dot(PP[ 12 + 32 ],PP[  6 + 32 ],PP[  6 + 32 ]);
    dot(PP[ 24 + 32 ],PP[ 12 + 32 ],PP[ 12 + 32 ]);
    dot(PP[  3 + 64 ],PP[  2 + 64 ],PP[  1 + 64 ]);
    dot(PP[  6 + 64 ],PP[  3 + 64 ],PP[  3 + 64 ]);
    dot(PP[ 12 + 64 ],PP[  6 + 64 ],PP[  6 + 64 ]);
    dot(PP[ 24 + 64 ],PP[ 12 + 64 ],PP[ 12 + 64 ]);
    dot(PP[  3 + 96 ],PP[  2 + 96 ],PP[  1 + 96 ]);
    dot(PP[  6 + 96 ],PP[  3 + 96 ],PP[  3 + 96 ]);
    dot(PP[ 12 + 96 ],PP[  6 + 96 ],PP[  6 + 96 ]);
    dot(PP[ 24 + 96 ],PP[ 12 + 96 ],PP[ 12 + 96 ]);

    dot(PP[  5      ],PP[  3      ],PP[  2      ]);
    dot(PP[ 10      ],PP[  5      ],PP[  5      ]);
    dot(PP[ 20      ],PP[ 10      ],PP[ 10      ]);
    dot(PP[  5 + 32 ],PP[  3 + 32 ],PP[  2 + 32 ]);
    dot(PP[ 10 + 32 ],PP[  5 + 32 ],PP[  5 + 32 ]);
    dot(PP[ 20 + 32 ],PP[ 10 + 32 ],PP[ 10 + 32 ]);
    dot(PP[  5 + 64 ],PP[  3 + 64 ],PP[  2 + 64 ]);
    dot(PP[ 10 + 64 ],PP[  5 + 64 ],PP[  5 + 64 ]);
    dot(PP[ 20 + 64 ],PP[ 10 + 64 ],PP[ 10 + 64 ]);
    dot(PP[  5 + 96 ],PP[  3 + 96 ],PP[  2 + 96 ]);
    dot(PP[ 10 + 96 ],PP[  5 + 96 ],PP[  5 + 96 ]);
    dot(PP[ 20 + 96 ],PP[ 10 + 96 ],PP[ 10 + 96 ]);

    dot(PP[  7      ],PP[  4      ],PP[  3      ]);
    dot(PP[ 14      ],PP[  7      ],PP[  7      ]);
    dot(PP[ 28      ],PP[ 14      ],PP[ 14      ]);
    dot(PP[  7 + 32 ],PP[  4 + 32 ],PP[  3 + 32 ]);
    dot(PP[ 14 + 32 ],PP[  7 + 32 ],PP[  7 + 32 ]);
    dot(PP[ 28 + 32 ],PP[ 14 + 32 ],PP[ 14 + 32 ]);
    dot(PP[  7 + 64 ],PP[  4 + 64 ],PP[  3 + 64 ]);
    dot(PP[ 14 + 64 ],PP[  7 + 64 ],PP[  7 + 64 ]);
    dot(PP[ 28 + 64 ],PP[ 14 + 64 ],PP[ 14 + 64 ]);
    dot(PP[  7 + 96 ],PP[  4 + 96 ],PP[  3 + 96 ]);
    dot(PP[ 14 + 96 ],PP[  7 + 96 ],PP[  7 + 96 ]);
    dot(PP[ 28 + 96 ],PP[ 14 + 96 ],PP[ 14 + 96 ]);

    dot(PP[  9      ],PP[  5      ],PP[  4      ]);
    dot(PP[ 18      ],PP[  9      ],PP[  9      ]);
    dot(PP[  9 + 32 ],PP[  5 + 32 ],PP[  4 + 32 ]);
    dot(PP[ 18 + 32 ],PP[  9 + 32 ],PP[  9 + 32 ]);
    dot(PP[  9 + 64 ],PP[  5 + 64 ],PP[  4 + 64 ]);
    dot(PP[ 18 + 64 ],PP[  9 + 64 ],PP[  9 + 64 ]);
    dot(PP[  9 + 96 ],PP[  5 + 96 ],PP[  4 + 96 ]);
    dot(PP[ 18 + 96 ],PP[  9 + 96 ],PP[  9 + 96 ]);

    dot(PP[ 11      ],PP[  6      ],PP[  5      ]);
    dot(PP[ 22      ],PP[ 11      ],PP[ 11      ]);
    dot(PP[ 11 + 32 ],PP[  6 + 32 ],PP[  5 + 32 ]);
    dot(PP[ 22 + 32 ],PP[ 11 + 32 ],PP[ 11 + 32 ]);
    dot(PP[ 11 + 64 ],PP[  6 + 64 ],PP[  5 + 64 ]);
    dot(PP[ 22 + 64 ],PP[ 11 + 64 ],PP[ 11 + 64 ]);
    dot(PP[ 11 + 96 ],PP[  6 + 96 ],PP[  5 + 96 ]);
    dot(PP[ 22 + 96 ],PP[ 11 + 96 ],PP[ 11 + 96 ]);

    dot(PP[ 13      ],PP[  7      ],PP[  6      ]);
    dot(PP[ 26      ],PP[ 13      ],PP[ 13      ]);
    dot(PP[ 13 + 32 ],PP[  7 + 32 ],PP[  6 + 32 ]);
    dot(PP[ 26 + 32 ],PP[ 13 + 32 ],PP[ 13 + 32 ]);
    dot(PP[ 13 + 64 ],PP[  7 + 64 ],PP[  6 + 64 ]);
    dot(PP[ 26 + 64 ],PP[ 13 + 64 ],PP[ 13 + 64 ]);
    dot(PP[ 13 + 96 ],PP[  7 + 96 ],PP[  6 + 96 ]);
    dot(PP[ 26 + 96 ],PP[ 13 + 96 ],PP[ 13 + 96 ]);

    dot(PP[ 15      ],PP[  8      ],PP[  7      ]);
    dot(PP[ 30      ],PP[ 15      ],PP[ 15      ]);
    dot(PP[ 15 + 32 ],PP[  8 + 32 ],PP[  7 + 32 ]);
    dot(PP[ 30 + 32 ],PP[ 15 + 32 ],PP[ 15 + 32 ]);
    dot(PP[ 15 + 64 ],PP[  8 + 64 ],PP[  7 + 64 ]);
    dot(PP[ 30 + 64 ],PP[ 15 + 64 ],PP[ 15 + 64 ]);
    dot(PP[ 15 + 96 ],PP[  8 + 96 ],PP[  7 + 96 ]);
    dot(PP[ 30 + 96 ],PP[ 15 + 96 ],PP[ 15 + 96 ]);

    dot(PP[ 17      ],PP[  9      ],PP[  8      ]);
    dot(PP[ 17 + 32 ],PP[  9 + 32 ],PP[  8 + 32 ]);
    dot(PP[ 17 + 64 ],PP[  9 + 64 ],PP[  8 + 64 ]);
    dot(PP[ 17 + 96 ],PP[  9 + 96 ],PP[  8 + 96 ]);

    dot(PP[ 19      ],PP[ 10      ],PP[  9      ]);
    dot(PP[ 19 + 32 ],PP[ 10 + 32 ],PP[  9 + 32 ]);
    dot(PP[ 19 + 64 ],PP[ 10 + 64 ],PP[  9 + 64 ]);
    dot(PP[ 19 + 96 ],PP[ 10 + 96 ],PP[  9 + 96 ]);

    dot(PP[ 21      ],PP[ 11      ],PP[ 10      ]);
    dot(PP[ 21 + 32 ],PP[ 11 + 32 ],PP[ 10 + 32 ]);
    dot(PP[ 21 + 64 ],PP[ 11 + 64 ],PP[ 10 + 64 ]);
    dot(PP[ 21 + 96 ],PP[ 11 + 96 ],PP[ 10 + 96 ]);

    dot(PP[ 23      ],PP[ 12      ],PP[ 11      ]);
    dot(PP[ 23 + 32 ],PP[ 12 + 32 ],PP[ 11 + 32 ]);
    dot(PP[ 23 + 64 ],PP[ 12 + 64 ],PP[ 11 + 64 ]);
    dot(PP[ 23 + 96 ],PP[ 12 + 96 ],PP[ 11 + 96 ]);

    dot(PP[ 25      ],PP[ 13      ],PP[ 12      ]);
    dot(PP[ 25 + 32 ],PP[ 13 + 32 ],PP[ 12 + 32 ]);
    dot(PP[ 25 + 64 ],PP[ 13 + 64 ],PP[ 12 + 64 ]);
    dot(PP[ 25 + 96 ],PP[ 13 + 96 ],PP[ 12 + 96 ]);

    dot(PP[ 27      ],PP[ 14      ],PP[ 13      ]);
    dot(PP[ 27 + 32 ],PP[ 14 + 32 ],PP[ 13 + 32 ]);
    dot(PP[ 27 + 64 ],PP[ 14 + 64 ],PP[ 13 + 64 ]);
    dot(PP[ 27 + 96 ],PP[ 14 + 96 ],PP[ 13 + 96 ]);

    dot(PP[ 29      ],PP[ 15      ],PP[ 14      ]);
    dot(PP[ 29 + 32 ],PP[ 15 + 32 ],PP[ 14 + 32 ]);
    dot(PP[ 29 + 64 ],PP[ 15 + 64 ],PP[ 14 + 64 ]);
    dot(PP[ 29 + 96 ],PP[ 15 + 96 ],PP[ 14 + 96 ]);

    dot(PP[ 31      ],PP[ 16      ],PP[ 15      ]);
    dot(PP[ 31 + 32 ],PP[ 16 + 32 ],PP[ 15 + 32 ]);
    dot(PP[ 31 + 64 ],PP[ 16 + 64 ],PP[ 15 + 64 ]);
    dot(PP[ 31 + 96 ],PP[ 16 + 96 ],PP[ 15 + 96 ]);

}

