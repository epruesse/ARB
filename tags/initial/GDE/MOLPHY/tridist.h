/*
 * tridist.h   Adachi, J.   1995.09.22
 * Copyright (C) 1993-1995 J. Adachi & M. Hasegawa, All rights reserved.
 */

#include "molphy.h"
#include "matrixut.h"

#define UPPERLIMIT 500.0   /* upper limit on branch length (subsutitutions) */
#define LOWERLIMIT 0.001   /* lower limit on branch length (subsutitutions) */
#define EPSILON    0.001
#define MAXCOLUMN  80
#define BUFLINE    512
#define MAXOVER    50
#define MAXLENG    30
#define NMLNGTH    10 /* max. num. of characters in species name with S or I */
#define SWITCHES   "Hhilm:o:St:T:uvwzZ"
#define VERSION    "1.2.5"
#define DATE       "Oct 30 1995"

#define TREEFEXT   ".tre"
#define EPSFILEEXT ".eps"
#define TPLGYFEXT  ".tpl"
#ifdef NJ
#define TPLGYFILE  "njdist.tpl"
#define EPSFILE    "njdist.eps"
#else  /* NJ */
#define TPLGYFILE  "tridist.tpl"
#define EPSFILE    "tridist.eps"
#endif /* NJ */

typedef struct node {
	struct node *isop;
	struct node *kinp;
	int descen;           /* descendants */
	int num;
	double length;
} Node;

typedef struct tree {
	Node *rootp;
	Node *firstp;
	Node **brnchp;
	double ablength;
	double rssleast;
	int npara;
	imatrix paths;
} Tree;

#ifdef MAIN_MODULE
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN FILE *Epsfp;
EXTERN time_t Ct0;

EXTERN boolean Upgma_optn;    /* u option UPGMA */
EXTERN boolean Least_optn;    /* l option least squares */
EXTERN boolean Outgr_optn;    /* o option */
EXTERN boolean Tfile_optn;    /* T option  Tree file option */
EXTERN boolean Tplgy_optn;    /* t option  topology file option */
EXTERN boolean Seque_optn;    /* S option  PHYLIP Sequential input format */
EXTERN boolean Verbs_optn;    /* v option  Verbose to stderr */
EXTERN boolean Info_optn;     /* i option  get Information */
EXTERN boolean Multi_optn;    /* m option  multiple data sets */
EXTERN boolean Write_optn;    /* w option  output more infomation */
EXTERN boolean Relia_optn;    /* R option  Reliability of a branch */
EXTERN boolean Debug_optn;    /* z option  output debug data */
EXTERN boolean Debug;         /* Z option  output more debug data */

EXTERN char *Epsfile;
EXTERN char *Prog_name;
EXTERN char *Comment;
EXTERN int Numspc;
EXTERN int Numtree;
EXTERN int Cnotree;
EXTERN int Maxbrnch;
EXTERN int Numbrnch;
EXTERN int Numpair;
EXTERN int Numexe;           /* number of executes */
EXTERN int Cnoexe;           /* curent numbering of executes */
EXTERN int Outgroup;
EXTERN double Proportion;

EXTERN Tree *Ctree;
EXTERN char **Identif;
EXTERN char **Sciname;
EXTERN char **Engname;
EXTERN dmatrix Distanmat;
EXTERN dvector Distanvec;
EXTERN dvector Lengths;
EXTERN dmatrix Reliprob;
