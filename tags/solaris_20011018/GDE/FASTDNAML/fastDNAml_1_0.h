/*  fastDNAml_1_0.h  */

#ifndef dnaml_h

/*  Compile time switches for various updates to program:
 *    0 gives original version
 *    1 gives new version
 */

#define ReturnSmoothedView    1  /* Propagate changes back after smooth */
#define BestInsertAverage     1  /* Build three taxon tree analytically */
#define DeleteCheckpointFile  0  /* Remove checkpoint file when done */

#define Debug                 0

/*  Program constants and parameters  */

#define maxsp          500  /* maximum number of species */
#define maxsites      20000  /* maximum number of sites */
#define maxpatterns   10000  /* maximum number of different site patterns */

#define maxcategories   35  /* maximum number of site types */
#define maxkeep          1  /* maximum number of user trees */
#define maxlogf    maxkeep  /* maximum number of user trees */
#define smoothings      32  /* maximum smoothing passes through tree */
#define iterations      10  /* maximum iterations of makenewz per insert */
#define newzpercycle     1  /* iterations of makenewz per tree traversal */
#define nmlngth         10  /* number of characters in species name */
#define deltaz     0.00001  /* test of net branch length change in update */
#define zmin       1.0E-15  /* max branch prop. to -log(zmin) (= 34) */
#define zmax (1.0 - 1.0E-6) /* min branch prop. to 1.0-zmax (= 1.0E-6) */
#define defaultz       0.9  /* value of z assigned as starting point */
#define unlikely  -1.0E300  /* low likelihood for initialization */
#define down             2
#define over            60
#define decimal_point   '.'
#define checkpointname "checkpoint"

#define TRUE             1
#define FALSE            0

#define ABS(x)    (((x)<0)   ? (-(x)) : (x))
#define MIN(x,y)  (((x)<(y)) ?   (x)  : (y))
#define MAX(x,y)  (((x)>(y)) ?   (x)  : (y))
#define LOG(x)    (((x)>0)   ? log(x) : hang("log domain error"))
#define nint(x)   ((int) ((x)>0 ? ((x)+0.5) : ((x)-0.5)))
#define aint(x)   ((double) ((int) (x)))


typedef  int  boolean;
typedef  int  longer[6];

typedef  double  xtype;

typedef  struct  xmantyp {
    struct xmantyp  *prev;
    struct xmantyp  *next;
    struct noderec  *owner;
    xtype           *a, *c, *g, *t;
    } xarray;

typedef  struct noderec {
    double           z, z0;
    struct noderec  *next;
    struct noderec  *back;
    int              number;
    xarray          *x;
    int              xcoord, ycoord, ymin, ymax;
    char             name[nmlngth+1]; /*  Space for null termination  */
    char            *tip;             /*  Pointer to sequence data  */
    } node, *nodeptr;

typedef  struct {
    double           likelihood;
    double           log_f[maxpatterns]; /* info for signif. of trees */
    node            *nodep[2*maxsp];  /* one extra node for tree reading */
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
    boolean          smoothed;
    boolean          rooted;
    boolean          userlen;     /* use user-supplied branch lengths */
    boolean          usertree;    /* use user-supplied trees */
    boolean          userwgt;     /* use user-supplied position weight mask */
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
    boolean          smoothed;    /* branch optimization converged? */
    } topol;

typedef struct {
    double           best;        /* highest score saved */
    double           worst;       /* lowest score saved */
    topol           *start;       /* starting tree for optimization */
    topol           *byScore[maxkeep+1];
    topol           *byTopol[maxkeep+1];
    int              nkeep;       /* maximum topologies to save */
    int              nvalid;      /* number of topologies saved */
    int              ninit;       /* number of topologies initialized */
    int              numtrees;    /* number of alternatives tested */
    boolean          improved;
    } bestlist;

typedef  struct {
    double           tipmax;
    int              tipy;
    } drawdata;

void  exit();

char *strstr();  /* Not needed starting with version 1.0.5 */
char *strchr();  /* Not needed starting with version 1.0.5 */
char *index();   /* Not needed starting with version 1.0.5 */

#if  ANSI
   void *malloc();
#else
   char *malloc();
#endif

char *likelihood_key   = "likelihood";
char *ntaxa_key        = "ntaxa";
char *opt_level_key    = "opt_level";
char *smoothed_key     = "smoothed";

#define dnaml_h
#endif  /* #if undef dnaml_h */
