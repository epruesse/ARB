/*
 * protml.h   Adachi, J.   1996.07.01
 * Copyright (C) 1992-1996 J. Adachi & M. Hasegawa. All rights reserved.
 */

#define SIMPLE_ARB_ASSERT 
#include "../../INCLUDE/arb_assert.h"
#define mo_assert(cond) arb_assert(cond)

#include "molphy.h"
#include "matrixut.h"
           /* unit time (length) is a number of substitutions per 100 sites */
#define UPPERLIMIT  500.0   /* upper limit on branch length (subsutitutions) */
#define LOWERLIMIT  0.001   /* lower limit on branch length (subsutitutions) */
#define EPSILON     0.001   /* stopping value of error (branch length) */
#define DEPSILON    0.0001  /* stopping value of error (distance) */
#define REPSILON    0.005   /* stopping value of error (local rearrangement) */
#define NUMBOOTS    10000   /* number of bootstrap resamplings */
#define NUMBOOTSR   1000    /* number of bootstrap resamplings (reliability) */
#define MAXIT       50      /* maximum number of iterates of smoothing */
#define TBLRATE     0.3	/* if number of possible trees >  945 (OTUs >  7) */
#define TBLRATE7    2.0	/* if number of possible trees <= 945 (OTUs <= 7) */
#define MAXALTREES  105
#define NUMQLTREES  50
#define MAXQLBUF    9
#define MAXSLBUF    30 /* 20 */
#define RSRATE      0.5
#define QRSRATE     1.15
#define SRSRATE     1.5 /* 1.3 */
#define NUMTBLBIN   20
#define INITCOEFMLE 0.5

#define VARITPM 0

#define MINFREQ    0.005
#define NUMAMI     20
#define NUMNUC     4

#define MTREVNAME   "mtREV24"
#define LLSFILEEXT  ".lls"
#define EPSFILEEXT  ".eps"
#ifndef NUC
#define TPMRADIX    NUMAMI
#define INFOMOL     "Amino"
#define RSRINFILE   "prot.rsr"
#define RSROUTFILE  "p.rsr"
#define TPMOUTFILE  "p.src"
#define TPLFILE     "protml.tpl"
#define TREFILE     "protml.tre"
#define EPSFILE     "protml.eps"
#define LOGFILE     "Protml.log"
#else  /* NUC*/
#define TPMRADIX    NUMNUC
#define INFOMOL     "Nucleic"
#define RSRINFILE   "nuc.rsr"
#define RSROUTFILE  "n.rsr"
#define TPMOUTFILE  "n.src"
#define TPLFILE     "nucml.tpl"
#define TREFILE     "nucml.tre"
#define EPSFILE     "nucml.eps"
#define LOGFILE     "Nucml.log"
#define ALPHABETA   4.0
#define ALPHAYR     1.0
#define BETA12      1.0
#define MINAB       0.5
#define MAXAB       100.0
#define MINAYR      0.1
#define MAXAYR      5.0
#define MINB12      0.1
#define MAXB12      10.0
#endif /* NUC*/

#define MAXCOLUMN  80
#define LINESITES  60
#define BUFLINE    512
#define MAXWORD    32
#define MAXOVER    50
#define MAXLENG    30
#define MIDLIMIT  100.0
#define NMLNGTH    10 /* max. num. of characters in species name with S or I */
#define VERSION    "2.3b3" /* beta 2 */
#define DATE       "July 1 1996"

typedef int       ivectpmty[TPMRADIX];
typedef double    dvectpmty[TPMRADIX];
typedef dvectpmty dmattpmty[TPMRADIX];

typedef struct node {
	struct node *isop;
	struct node *kinp;
	int descen;
	int num;
	double length;
	double lklhdl;
	double reliab;
	ivector paths;
	ivector eprob;
	dmatrix iprob;
} Node;

typedef struct tree {
	Node *rootp;
	Node *firstp;
	Node **ebrnchp;
	Node **ibrnchp;
	double lklhd;
	double varilkl;
	double lklmin;
	double lklmean;
	double aic;
	double aproxl;
	double tblength;
	double tbldis;
	double rssleast;
	int npara;
	int numorder;
	ivector bturn;
} Tree;

typedef struct infotree {
	int npara;
	int bsprob;
	int mlsite;
	double lklhd;
	double varilkl;
	double lklaprox;
	double lklbs;
	double aic;
	double tblength;
	double abdistan;
	char *ltplgy;
} Infotree;

typedef struct infoaltree {
	struct infoaltree *up;
	double lklaprox;
	double tbl;
	double rss;
	char *ltplgy;
} Infoaltree;

typedef struct infoqltree {
	struct infoqltree *up;
	Node *ap;
	double lklaprox;
	double residual;
	dvector lengths;
} Infoqltree;

typedef struct infoaddtree {
	struct infoaddtree *dp;
	double lklaprox;
	int frequency;
	char *ltplgy;
} Infoaddtree;

typedef struct infosltree {
	struct infosltree *up;
	Node *ibp;
	Node *jbp;
	double lklaprox;
	double residual;
	dvector lengths;
} Infosltree;

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


EXTERN FILE *Epsfp;
EXTERN FILE *Tplfp;
EXTERN FILE *Trefp;
EXTERN FILE *Lklfp;
EXTERN FILE *Logfp;
EXTERN FILE *Rtfofp; /* TPM! */
EXTERN FILE *Rtfifp; /* TPM! */
EXTERN FILE *Tpmofp; /* TPM! */
EXTERN time_t Ct0;
EXTERN time_t Ct1;

#define SWITCHES   "aAbCdDefFhHiIjl:LmMn:No:pP:qQrRsSt:TuvVwXzZ"
				/*    Bc  E  G   JkK      O             U  Wx */
EXTERN boolean Aneal_optn;    /* a option  Annealing */
EXTERN boolean Aprox_optn;    /* A option  Approximate likelihood */
EXTERN boolean Boots_optn;    /* b option  no Bootstrap probability */
EXTERN boolean Ctacit_optn;   /* C option  taCiturnity */
EXTERN boolean Dayhf_optn;    /* d option  Dayhoff model */
EXTERN boolean Distn_optn;    /* D option  Distance matrix only */
EXTERN boolean Exhau_optn;    /* e option  Exhaustive search */
EXTERN boolean Frequ_optn;    /* f option  with data frequency */
EXTERN boolean Logdet_optn;   /* F option  */
EXTERN boolean Info_optn;     /* i option  */
EXTERN boolean Inlvd_optn;    /* I option  interleaved input format */
EXTERN boolean Jtt_optn;      /* j option  JTT model */
EXTERN boolean Lklhd_optn;    /* l option  */
EXTERN boolean Logfl_optn;    /* L option  */
EXTERN boolean Mtrev_optn;    /* m option  Mitochondrial model */
EXTERN boolean Mevol_optn;    /* M option  minimum evolution */
EXTERN boolean Njoin_optn;    /* N option  NJ */
EXTERN boolean Outgr_optn;    /* o option  */
EXTERN boolean Poisn_optn;    /* p option  Poisson process */
EXTERN boolean Percnt_optn;   /* P option  Per cent */
EXTERN boolean Quick_optn;    /* q option  quick mode */
EXTERN boolean Quick1_optn;   /* Q option  quick 1 mode */
EXTERN boolean Rrsr_optn;     /* r option  Read RSR, TPM */
EXTERN boolean Relia_optn;    /* R option  Reliability of a branch */
EXTERN boolean Stard_optn;    /* s option  Star Decomposition */
EXTERN boolean Seque_optn;    /* S option  PHYLIP Sequential input format */
EXTERN boolean Tstv_optn;     /* t option  with decimal number */
EXTERN boolean Toptim_optn;   /* t option  without decimal number */
EXTERN boolean Triad_optn;    /* T option  */
EXTERN boolean User_optn;     /* u option  designate user trees */
EXTERN boolean Verbs_optn;    /* v option  Verbose to stderr */
EXTERN boolean Varia_optn;    /* V option  Variance */
EXTERN boolean Write_optn;    /* w option  output sequence infomation */
EXTERN boolean Xreli_optn;    /* X option  Reliability of a branch 2 */
EXTERN boolean Debug_optn;    /* z option  */
EXTERN boolean Debug;         /* Z option  */
EXTERN boolean Const_optn;    /*   option  with constrained_tree */

#ifndef NUC
#else  /* NUC */
EXTERN double AlphaBeta;
EXTERN double AlphaYR;
EXTERN double Beta12;
#endif /* NUC */

EXTERN boolean Converg;
EXTERN boolean Topting;
EXTERN char *Prog_name;
EXTERN char Modelname[64];
EXTERN char *Comment;
EXTERN char *Llsfile;
EXTERN char *Epsfile;
EXTERN int Aneal_mode;
EXTERN int Maxspc;
EXTERN int Numspc;
EXTERN int Maxibrnch;
EXTERN int Numibrnch;
EXTERN int Maxbrnch;
EXTERN int Numbrnch;
EXTERN int Maxpair;
EXTERN int Numpair;
EXTERN int Maxsite;
EXTERN int Numsite;
EXTERN int Numptrn;
EXTERN int Numtree;
EXTERN int Maxaltree;
EXTERN int Numaltree;
EXTERN int Numqltree;
EXTERN int Numaddtree;
EXTERN int Cnotree;
EXTERN int Maxlkltree;
EXTERN int Minaictree;
EXTERN int Mintbltree;
EXTERN int Numexe;
EXTERN int Cnoexe;
EXTERN int Numit;
EXTERN int Linesites;
EXTERN int Outgroup1;
EXTERN int Outgroup2;
EXTERN int Numverbs;
EXTERN int Tblunder;
EXTERN int Tblover;
EXTERN double Epsilon;
EXTERN double Ulimit;
EXTERN double Llimit;
EXTERN double Mlimit;
EXTERN double Numtplgy;
EXTERN double Maxlkl;
EXTERN double Minaic;
EXTERN double Mintbl;
EXTERN double Mintbldm;
EXTERN double Maxtbldm;
EXTERN double Basetbldm;
EXTERN double Proportion;
EXTERN double Percent;
EXTERN double Tblrate;
EXTERN double Tblcoef;
EXTERN ivector Numsites; /* Mar 22 1995 */

EXTERN Tree *Ctree;
EXTERN Infotree *Infotrees;
EXTERN Infoaltree *Infoaltrees;
EXTERN Infoaltree Atail, Ahead;
EXTERN Infoqltree *Infoqltrees;
EXTERN Infoqltree *Qtail, *Qhead;

EXTERN char **Identif;
EXTERN char **Sciname;
EXTERN char **Engname;
EXTERN cmatrix Seqchar;
EXTERN imatrix Seqconint;
EXTERN ivector Weight;
EXTERN dmatrix Distanmat;
EXTERN dvector Distanvec;
EXTERN dvector Brnlength;
EXTERN cvector Strtree;
EXTERN ivector Poolorder;
EXTERN ivector Relistat;
EXTERN imatrix Relinum;
EXTERN dvector Relidiff;
EXTERN dmatrix Reliprob;
EXTERN ivector Tblbin;
EXTERN dvector Relitrif;

#ifdef LIGHT
#define LPMATRIX fmatrix
#define LPVECTOR fvector
#define LPCUBE   fcube
#define NEW_LPMATRIX  new_fmatrix
#define NEW_LPVECTOR  new_fvector
#define NEW_LPCUBE    new_fcube
#define FREE_LPMATRIX free_fmatrix
#define FREE_LPVECTOR free_fvector
#define FREE_LPCUBE   free_fcube
#else  /* LIGHT */
#define LPMATRIX dmatrix
#define LPVECTOR dvector
#define LPCUBE   dcube
#define NEW_LPMATRIX  new_dmatrix
#define NEW_LPVECTOR  new_dvector
#define NEW_LPCUBE    new_dcube
#define FREE_LPMATRIX free_dmatrix
#define FREE_LPVECTOR free_dvector
#define FREE_LPCUBE   free_dcube
#endif /* LIGHT */

EXTERN LPMATRIX Lklptrn;
EXTERN LPVECTOR Alklptrn;

EXTERN dvectpmty Freqemp;
EXTERN dvectpmty Freqtpm;
EXTERN dvectpmty Eval;
EXTERN dvectpmty Evl2;
EXTERN dmattpmty Evec;
EXTERN dmattpmty Ievc;

#ifdef TPM
EXTERN dmattpmty Rtf; /* TPM! */
EXTERN dmattpmty Tpm; /* TPM! */
EXTERN int Itpm; /* TPM! */
EXTERN int Jtpm; /* TPM! */
#endif /* TPM */
