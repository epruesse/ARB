/*  AxMl.h  */

#define headerName     "AxMl.h"
#define headerVersion  "1.0.0"
#define headerDate     "February, 2002"


#ifndef dnaml_h

/*  Compile time switches for various updates to program:
 *    0 gives original version
 *    1 gives new version
 */

#define ReturnSmoothedView    1  /* Propagate changes back after smooth */
#define BestInsertAverage     1  /* Build three taxon tree analytically */
#define DeleteCheckpointFile  0  /* Remove checkpoint file when done */

#define EQUALITIES 16
char map[EQUALITIES];
char* map2;
#define Debug                 0

/*  Program constants and parameters  */

#define maxlogf       1024  /* maximum number of user trees */
#define maxcategories   35  /* maximum number of site types */

#define smoothings      32  /* maximum smoothing passes through tree */
#define iterations      10  /* maximum iterations of makenewz per insert */
#define newzpercycle     1  /* iterations of makenewz per tree traversal */
#define nmlngth         10  /* number of characters in species name */
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
#define minlikelihood     (1.0/twotothe256)          /*  2**(-256)  */
#define log_minlikelihood (-177.445678223345993274)  /* log(1.0/twotothe256) */

/*  The next two values are used for scaling the tree that is sketched in the
 *  output file.
 */
#define down             2
#define over            60

#define checkpointname "checkpoint"

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

#if  ! Vectorize
  typedef  char  yType;
#else
  typedef  int   yType;
#endif


typedef  int     boolean;
typedef  double  xtype;

typedef  struct  likelihood_vector {
    xtype        a, c, g, t;
    long         exp;
    } likelivector;

typedef  struct  xmantyp {
    struct xmantyp  *prev;
    struct xmantyp  *next;
    struct noderec  *owner;
    likelivector    *lv;
    } xarray;



int equalities = 0;

typedef struct fgh{
  double a, c, g, t;
  long exp;
  boolean set;
}
homType;


typedef  struct noderec {
    double           z, z0;
    struct noderec  *next;
    struct noderec  *back;
    int              number;
    xarray          *x;
    int              xcoord, ycoord, ymin, ymax;
    char             name[nmlngth+1]; /*  Space for null termination  */
    yType           *tip;             /*  Pointer to sequence data  */

  /* AxML modification start*/

  char             *equalityVector; /* subtree equality vector for this node*/  
  homType mlv[EQUALITIES][maxcategories];
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
  double sumfx2rfx2y, fx2r, fx2y;
  double a, c, g, t;
  int exp;
} memQ;

typedef  struct {
    int              numsp;       /* number of species (also tr->mxtips) */
    int              sites;       /* number of input sequence positions */
    yType          **y;           /* sequence data array */
    boolean          freqread;    /* user base frequencies have been read */
/* To do: DNA specific values should get packaged into structure */
    double           freqa, freqc, freqg, freqt,  /* base frequencies */
                        freqr, freqy, invfreqr, invfreqy,
                        freqar, freqcy, freqgr, freqty;
    double           ttratio, xi, xv, fracchange; /* transition/transversion */
/* End of DNA specific values */
    int             *wgt;         /* weight per sequence pos */
    int             *wgt2;        /* weight per pos (booted) */
    int              categs;      /* number of rate categories */
    double           catrat[maxcategories+1]; /* rates per categories */
    int             *sitecat;     /* category per sequence pos */
    } rawdata;

typedef  struct {
    int             *alias;       /* site representing a pattern */
    int             *aliaswgt;    /* weight by pattern */
    int              endsite;     /* # of sequence patterns */
    int              wgtsum;      /* sum of weights of positions */
    int             *patcat;      /* category per pattern */
    double          *patrat;      /* rates per pattern */
    double          *wr;          /* weighted rate per pattern */
    double          *wr2;         /* weight*rate**2 per pattern */
    } cruncheddata;

typedef  struct {
    double           likelihood;
    double          *log_f;       /* info for signif. of trees */
    node           **nodep;
    node            *start;
    node            *outgrnode;
    int              mxtips;
    int              ntips;
    int              nextnode;
    int              opt_level;
    int              log_f_valid; /* log_f value sites */
    int              global;      /* branches to cross in full tree */
    int              partswap;    /* branches to cross in partial tree */
    int              outgr;       /* sequence number to use in rooting tree */
    boolean          prelabeled;  /* the possible tip names are known */
    boolean          smoothed;
    boolean          rooted;
    boolean          userlen;     /* use user-supplied branch lengths */
    rawdata         *rdta;        /* raw data structure */
    cruncheddata    *cdta;        /* crunched data structure */
  //stmatak modif 
  int eqDepth; /*indicates up to which tree depth het. eq. vectors shall be used */
  //stamatk modif
    } tree;

typedef struct conntyp {
    double           z;           /* branch length */
    node            *p, *q;       /* parent and child sectors */
    void            *valptr;      /* pointer to value of subtree */
    int              descend;     /* pointer to first connect of child */
    int              sibling;     /* next connect from same parent */
    } connect, *connptr;

typedef  struct {
    double           likelihood;
    double          *log_f;       /* info for signif. of trees */
    connect         *links;       /* pointer to first connect (start) */
    node            *start;
    int              nextlink;    /* index of next available connect */
                                  /* tr->start = tpl->links->p */
    int              ntips;
    int              nextnode;
    int              opt_level;   /* degree of branch swapping explored */
    int              scrNum;      /* position in sorted list of scores */
    int              tplNum;      /* position in sorted list of trees */
    int              log_f_valid; /* log_f value sites */
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
    long             boot;        /* bootstrap random number seed */
    int              extra;       /* extra output information switch */
    boolean          empf;        /* use empirical base frequencies */
    boolean          interleaved; /* input data are in interleaved format */
    long             jumble;      /* jumble random number seed */
    int              nkeep;       /* number of best trees to keep */
    int              numutrees;   /* number of user trees to read */
    boolean          prdata;      /* echo data to output stream */
    boolean          qadd;        /* test addition without full smoothing */
    boolean          restart;     /* resume addition to partial tree */
    boolean          root;        /* use user-supplied outgroup */
    boolean          trprint;     /* print tree to output stream */
    int              trout;       /* write tree to "treefile" */
    boolean          usertree;    /* use user-supplied trees */
    boolean          userwgt;     /* use user-supplied position weight mask */
    } analdef;

typedef  struct {
    double           tipmax;
    int              tipy;
    } drawdata;

void  exit();

#define  Malloc(x)  malloc((unsigned) (x))       /* BSD */
/* #define  Malloc(x)  malloc((size_t) (x)) */   /* System V */

#define    Free(x)  (void) free((char *) (x))    /* BSD */
/* #define Free(x)  free((void *) (x))      */   /* System V */

char *likelihood_key   = "likelihood";
char *ntaxa_key        = "ntaxa";
char *opt_level_key    = "opt_level";
char *smoothed_key     = "smoothed";

#define dnaml_h
#endif  /* #if undef dnaml_h */
