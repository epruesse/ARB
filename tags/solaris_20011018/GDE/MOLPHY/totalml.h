/*
 * totalml.h   Adachi, J.   1994.12.30
 * Copyright (C) 1994 J. Adachi & M. Hasegawa. All rights reserved.
 */

#include "molphy.h"
#include "matrixut.h"

#define NUMBOOTS   10000   /* number of bootstrap resamplings */

#define LLSFILEEXT  ".lls"

#define MAXCOLUMN  80
#define BUFLINE    512
#define SWITCHES   "bCHhivwZz"
#define VERSION    "1.1"
#define DATE       "Dec 30 1994"

#ifdef MAIN_MODULE
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN FILE *llsfp;
EXTERN time_t Ct0;

EXTERN boolean Boots_optn;    /* b option  no Bootstrap probability */
EXTERN boolean Ctacit_optn;   /* C option  taCiturnity */
EXTERN boolean Info_optn;     /* i option  */
EXTERN boolean Verbs_optn;    /* v option  Verbose to stderr */
EXTERN boolean Write_optn;    /* w option  output sequence infomation */
EXTERN boolean Debug;         /* Z option  */
EXTERN boolean Debug_optn;    /* z option  */

EXTERN char *Prog_name;
EXTERN char *Comment;
EXTERN char *Llsfile;
EXTERN int Numtree;
EXTERN int Numseqs;
EXTERN int Allsite;

EXTERN ivector Numsite;

EXTERN dcube Lnlklsite;
EXTERN dmatrix Alnlklsite;
