/*  RAxML-V, a program for sequential and parallel estimation of phylogenetic trees 
 *  from nucleotide sequences implementing SEVs (Subtree Equality Vectors), the lazy subtree
 *  rearrangement heuristics, a simulated annealing search algorithm, and a parallel OpenMP-version
 *  for SMPs. Copyright February 2005 by Alexandros Stamatakis
 *
 *  Partially derived from
 *  fastDNAml, a program for estimation of phylogenetic trees from sequences by Gary J. Olsen
 *  
 *  and 
 *
 *  Programs of the PHYLIP package by Joe Felsenstein.
 *
 *  This program is free software; you may redistribute it and/or modify its
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 * 
 *
 *  For any other enquiries send an Email to Alexandros Stamatakis
 *  stamatak@ics.forth.gr
 *
 *  When publishing work that is based on the results from RAxML-V please cite:
 *  
 *  Alexandros Stamatakis: "An Efficient Program for phylogenetic Inference Using Simulated Annealing". 
 *  Proceedings of IPDPS2005, Denver, Colorado, April 2005.
 *  
 *  AND
 *
 *  Alexandros Stamatakis, Thomas Ludwig, and Harald Meier: "RAxML-III: A Fast Program for Maximum Likelihood-based Inference of Large Phylogenetic Trees". 
 *  In Bioinformatics 21(4):456-463.
 *
 *
 *
 *
 */






#define EQUALITIES 16
#define maxcategories 2

/*  Program constants and parameters  */

#define maxlogf       1024  /* maximum number of user trees */
#define smoothings      32  /* maximum smoothing passes through tree */
#define iterations      10  /* maximum iterations of makenewz per insert */
#define newzpercycle     1  /* iterations of makenewz per tree traversal */
#define nmlngth         100  /* number of characters in species name */
#define deltaz     0.00001  /* test of net branch length change in update */
#define zmin       1.0E-15  /* max branch prop. to -log(zmin) (= 34) */
#define zmax (1.0 - 1.0E-6) /* min branch prop. to 1.0-zmax (= 1.0E-6) */
#define defaultz       0.9  /* value of z assigned as starting point */
#define unlikely  -1.0E300  /* low likelihood for initialization */

/*  These values are used to rescale the lilelihoods at a given site so that
 *  there is no floating point underflow.
 */
#define twotothe256  \
115792089237316195423570985008687907853269984665640564039457584007913129639936.0
                                                     /*  2**256 (exactly)  */
#define minlikelihood  (1.0/twotothe256)





#define badEval        1.0
#define badZ           0.0
#define badRear         -1
#define badSigma      -1.0

#define TRUE             1
#define FALSE            0

#define treeNone         0
#define treeNewick       1
#define treeProlog       2
#define treePHYLIP       3
#define treeMaxType      3
#define treeDefType  treePHYLIP

#define ABS(x)    (((x)<0)   ?  (-(x)) : (x))
#define MIN(x,y)  (((x)<(y)) ?    (x)  : (y))
#define MAX(x,y)  (((x)>(y)) ?    (x)  : (y))
#define LOG(x)    (((x)>0)   ? log(x)  : hang("log domain error"))
#define NINT(x)   ((int) ((x)>0 ? ((x)+0.5) : ((x)-0.5)))

#define programName        "RAxMl-V"
#define programVersion     "2.1"
#define programVersionInt   2105
#define programDate        "MARCH, 2005"
#define programDateInt      200503

#define TREE_INFERENCE 1
#define TREE_EVALUATION 2
#define FAST_MODE 3
#define SIMULATED_ANNEALING 4
#define TREE_RECURSIVE 5

#define M_HKY85 1
#define M_HKY85CAT 2
#define M_GTR 3
#define M_GTRCAT 4


typedef  char  yType;
typedef  int     boolean;
typedef  double  xtype;

typedef  struct  likelihood_vector {
    xtype        a, c, g, t;
    long         exp;
    } likelivector;

typedef struct ratec 
{
  double accumulatedSiteLikelihood;
  double rate;
} rateCategorize;




typedef  struct  xmantyp {
    struct xmantyp  *prev;
    struct xmantyp  *next;
    struct noderec  *owner;
    likelivector    *lv;
    } xarray;






typedef struct fgh{
  double a, c, g, t;
  long exp;
  boolean set;
} homType;




typedef  struct noderec {
  /*Tree divisor*/
  /* tree divisor*/
    double           z, z0;
    struct noderec  *next;
    struct noderec  *back;
    int              number;
    xarray          *x;   
    char             name[nmlngth+1]; /*  Space for null termination  */
    yType           *tip;             /*  Pointer to sequence data  */

  /* AxML modification start*/

  int             *equalityVector; /* subtree equality vector for this node*/  
  homType mlv[EQUALITIES];  
  /* AxML modification end*/
  

    } node, *nodeptr;






typedef struct {
  double sumaq; 
  double sumgq; 
  double sumcq; 
  double sumtq;  
  int exp;
} poutsa;

typedef struct {
  double fx1a, fx1g, fx1c, fx1t, sumag, sumct, sumagct;
  int exp;
} memP;

typedef struct {
  double a, b, c, w;
} mnzMem;


typedef struct {
  double a, b, c, w, dlnLidlz, d2lnLdlz2;
} mnzMem2;



typedef struct {
  double sumfx2rfx2y, fx2r, fx2y;
  double a, c, g, t;
  int exp;
} memQ;

typedef  struct {
  int              numsp;       /* number of species (also tr->mxtips) */
  int              sites;       /* number of input sequence positions */
  yType          **y;           /* sequence data array */
  yType            *y0;  
  

  
  double freqaREV[maxcategories+1], 
    freqcREV[maxcategories+1], 
    freqgREV[maxcategories+1], 
    freqtREV[maxcategories+1], 
    ttratioREV[maxcategories+1];
  boolean matread;
  double EIGV[maxcategories+1][4][4];
  double invfreq[maxcategories+1][4];
  double EIGN[maxcategories+1][4];
  double fracchangeav;
  double initialRates[5];
  double           freqa, freqc, freqg, freqt, ttratio;
  double          freqr, freqy, invfreqr, invfreqy,
                  freqar, freqcy, freqgr, freqty;
  double           xi, xv, fracchange; /* transition/transversion */
/* End of DNA specific values */
    int             *wgt;         /* weight per sequence pos */
    int             *wgt2;        /* weight per pos (booted) */
    int              categs;      /* number of rate categories */
    } rawdata;

typedef  struct {
  int             *alias;       /* site representing a pattern */
  int             *aliaswgt;    /* weight by pattern */
  int             *rateCategory;
  int              endsite;     /* # of sequence patterns */
  int              endsiteNormal;
  int              wgtsum;      /* sum of weights of positions */
  double          *patrat;      /* rates per pattern */
  double          *patratStored;
  double          *wr;          /* weighted rate per pattern */
  double          *wr2;         /* weight*rate**2 per pattern */
} cruncheddata;

typedef  struct {
  double           startLH;
  double endLH;
    double           likelihood;   
    node           **nodep;
    node            *start;
    node            *outgrnode;
    int              mxtips;
    int              ntips;
    int              nextnode;
    int              opt_level;   
    int              outgr;       /* sequence number to use in rooting tree */
  int              NumberOfCategories;
    boolean          prelabeled;  /* the possible tip names are known */
    boolean          smoothed;
    boolean          rooted;
    rawdata         *rdta;        /* raw data structure */
    cruncheddata    *cdta;        /* crunched data structure */
    double t_value;
 
    } tree;





typedef struct subnodetyp {
  int count;
  node **nodeArray;
} subNodes;

typedef struct xyz {
  int difference;
  nodeptr bnode;
} BType;


typedef struct conntyp {
    double           z;           /* branch length */
    node            *p, *q;       /* parent and child sectors */
    void            *valptr;      /* pointer to value of subtree */
    int              descend;     /* pointer to first connect of child */
    int              sibling;     /* next connect from same parent */
    } connect, *connptr;

typedef  struct {
    double           likelihood;
   
    connect         *links;       /* pointer to first connect (start) */
    node            *start;
    int              nextlink;    /* index of next available connect */
                                  /* tr->start = tpl->links->p */
    int              ntips;
    int              nextnode;
    int              opt_level;   /* degree of branch swapping explored */
    int              scrNum;      /* position in sorted list of scores */
    int              tplNum;      /* position in sorted list of trees */
    
    boolean          prelabeled;  /* the possible tip names are known */
    boolean          smoothed;    /* branch optimization converged? */
    } topol;

typedef struct {
    double           best;        /* highest score saved */
    double           worst;       /* lowest score saved */
    topol           *start;       /* starting tree for optimization */
    topol          **byScore;
    topol          **byTopol;
    int              nkeep;       /* maximum topologies to save */
    int              nvalid;      /* number of topologies saved */
    int              ninit;       /* number of topologies initialized */
    int              numtrees;    /* number of alternatives tested */
    boolean          improved;
    } bestlist;

typedef  struct {
  double timeLimit;
  double t0;
  int categories;
  int repeats; 
  int model;
  int              max_rearrange;
  int              stepwidth;
  int              initial;
  int              mode;
  long             boot;        /* bootstrap random number seed */    
  boolean          interleaved; /* input data are in interleaved format */    
  boolean          restart;     /* resume addition to partial tree */  
  boolean          useWeightFile;
  int              maxSubProblemSize;
} analdef;

xarray *usedxtip, *freextip;
FILE   *INFILE;
double globTime = 0.0;
int treeStringLength;
extern char *optarg;                  
extern int  optind,opterr,optopt;      
int Multiple = 1;
int Thorough = 0;
int branchCounter;
char *seq_data_str = NULL, run_id[40] = "", workdir[2048] = "", 
  seq_file[2048] = "", tree_file[2048]="", weightFileName[2048] = "";
int equalities = 0;
    bestlist *bt;
char *likelihood_key   = "likelihood";
char *ntaxa_key        = "ntaxa";
char *opt_level_key    = "opt_level";
char *smoothed_key     = "smoothed";
int map[EQUALITIES];
int* map2;
FILE *permutationFile, *logFile, *infoFile;
char permFileName[1024], resultFileName[1024], 
  logFileName[1024], checkpointFileName[1024], infoFileName[1024]; 
int checkpoints = 1; 
int buildConsensus = 1; 
int consensusTreeCounter = 0;
double masterTime;
int checkPointCounter = 0;
analdef      *adef;
char *treeString (char *treestr, tree *tr, nodeptr p, int form);
void optimizeRates(tree *tr);
void optimizeTT(tree *tr);
void optimizeRateCategories(tree *tr, int categorized, int _maxCategories);
int optimizeModel(tree *tr, analdef *adef, int finalOptimization);

double str_readTreeLikelihood (char *treestr);
void treeEvaluation(tree *tr, analdef *adef);
void checkTime(analdef *adef, tree *tr);
boolean initrav (tree *tr, nodeptr p);
void hookup (nodeptr p, nodeptr q, double z);


boolean (*newview) (tree *, nodeptr);
double (*evaluate)(tree *, nodeptr);   
double (*makenewz) (tree *, nodeptr, nodeptr, double, int);
double (*evaluatePartial)(tree *, nodeptr, int, double);
boolean (*newviewPartial)(tree *, nodeptr, int, double);
boolean (*initravPartial) (tree *, nodeptr, int, double);


boolean newviewGTR (tree *, nodeptr);
double evaluateGTR (tree *, nodeptr);   
double makenewzGTR (tree *, nodeptr, nodeptr, double, int);

boolean newviewGTRCAT (tree *, nodeptr);
double evaluateGTRCAT (tree *, nodeptr);   
double makenewzGTRCAT (tree *, nodeptr, nodeptr, double, int);

boolean newviewHKY85 (tree *, nodeptr);
double evaluateHKY85 (tree *, nodeptr);   
double makenewzHKY85 (tree *, nodeptr, nodeptr, double, int);

boolean newviewHKY85CAT (tree *, nodeptr);
double evaluateHKY85CAT (tree *, nodeptr);   
double makenewzHKY85CAT (tree *, nodeptr, nodeptr, double, int);

void tred2 (double *a, const int n, const int np, double *d, double *e);
