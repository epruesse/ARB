/*
 * protst.h   Adachi, J.   1996.03.12
 * Copyright (C) 1993-1996 J. Adachi & M. Hasegawa, All rights reserved.
 */

#include "molphy.h"
#include "matrixut.h"

#define NUMAMI     20
#define NUMNUC     4

#ifndef NUC
#define TPMRADIX    NUMAMI
#define INFOMOL     "Amino"
#define LOGFILE     "Protst.log"
#else  /* NUC*/
#define TPMRADIX    NUMNUC
#define INFOMOL     "Nucleic"
#define LOGFILE     "Nucst.log"
#endif /* NUC*/

#define MAXCOLUMN  80
#define LINESITES  60
#define MAXELEMENT 17
#define BUFLINE    512
#define MAXWORD    32
#define MAXOVER    50
#define MAXLENG    30
#define NMLNGTH    10 /* max. num. of characters in species name with S or I */
#define SWITCHES   "ac:hHiILSvwzZ"
#define VERSION    "1.2.1"
#define DATE       "Mar 12 1996"

typedef int       ivectpmty[TPMRADIX];
typedef double    dvectpmty[TPMRADIX];
typedef dvectpmty dmattpmty[TPMRADIX];

extern char *Cacid1[];
extern char *Cacid3[];


#ifdef MAIN_MODULE

#define EXTERN
int Tpmradix = TPMRADIX;
char *Infomol = INFOMOL;

#else

#define EXTERN extern
EXTERN int Tpmradix;
EXTERN char *Infomol;

#endif

EXTERN FILE *Logfp;
EXTERN time_t Ct0;

EXTERN boolean Align_optn;    /* a option  Alignment viewer */
EXTERN boolean Colmn_optn;    /* c option  Column */
EXTERN boolean Ctacit_optn;   /* C option  taCiturnity */
EXTERN boolean Inlvd_optn;    /* I option  interleaved input format */
EXTERN boolean Info_optn;     /* i option  */
EXTERN boolean Logfl_optn;    /* L option  */
EXTERN boolean Multi_optn;    /* m option  multiple data sets */
EXTERN boolean Seque_optn;    /* S option  PHYLIP Sequential input format */
EXTERN boolean Tstv_optn;     /* t option  with decimal number */
EXTERN boolean Toptim_optn;   /* t option  without decimal number */
EXTERN boolean Verbs_optn;    /* v option  Verbose to stderr */
EXTERN boolean Write_optn;    /* w option  output sequence infomation */
EXTERN boolean Debug_optn;    /* z option  */
EXTERN boolean Debug;         /* Z option  */

EXTERN char *Prog_name;
EXTERN int Maxspc;
EXTERN int Numspc;
EXTERN int Maxpair;
EXTERN int Numpair;
EXTERN int Maxsite;
EXTERN int Numsite;
EXTERN int Numptrn;
EXTERN int Linesites;
EXTERN int Maxcolumn;
EXTERN int Maxelement;

EXTERN char **Identif;
EXTERN char **Sciname;
EXTERN char **Engname;
EXTERN cmatrix Seqchar;
EXTERN imatrix Seqconint;
EXTERN ivector Weight;
EXTERN dvectpmty Freqemp;


EXTERN int Numnoindel; /* ST */
EXTERN ivector Insdel; /* ST */
