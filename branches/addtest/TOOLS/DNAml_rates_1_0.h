//  DNAml_rates.h

/*  Compile time switches for various updates to program:
 *    0 gives original version
 *    1 gives new version
 */

#ifndef CXXFORWARD_H
#include <cxxforward.h>
#endif
#ifndef ARBDBT_H
#include <arbdbt.h>
#endif

#define Debug 1

//  Program constants and parameters

#define maxsp         10000  // maximum number of species
#define maxsites      8000  // maximum number of sites
#define maxpatterns   8000  // max number of different site patterns
#define maxcategories   35  // maximum number of site types
#define nmlngth         10  // max. number of characters in species name
#define KI_MAX       256.0  // largest rate possible
#define RATE_STEP sqrt(sqrt(2.0)) // initial step size for rate search
#define MAX_ERROR     0.01  // max fractional error in rate calculation
#define MIN_INFO         4  // minimum number of informative sequences
#define UNDEF_CATEGORY   1  // category number to output for undefined rate
#define zmin       1.0E-15  // max branch prop. to -log(zmin) (= 34)
#define zmax (1.0 - 1.0E-6) // min branch prop. to 1.0-zmax (= 1.0E-6)
#define unlikely  -1.0E300  // low likelihood for initialization
#define decimal_point   '.'

// the following macros are already defined in gmacros.h:
// #define ABS(x)    (((x)< 0)  ? -(x) : (x))
// #define MIN(x, y) (((x)<(y)) ?  (x) : (y))
// #define MAX(x, y) (((x)>(y)) ?  (x) : (y))
#define LOG(x)    (((x)> 0) ? log(x) : hang("log domain error"))
#define nint(x)   ((int) ((x)>0 ? ((x)+0.5) : ((x)-0.5)))
#define aint(x)   ((double) ((int) (x)))


typedef double  xtype;
struct          node;
typedef node   *nodeptr;

#if defined(Cxx11)
CONSTEXPR int MAXNODES = leafs_2_nodes(maxsp, ROOTED);
#else // !defined(Cxx11)
const int MAXNODES = 2*maxsp+1;
#endif

struct xarray {
    xarray  *prev;
    xarray  *next;
    nodeptr  owner;
    xtype   *a, *c, *g, *t;
};

struct node {
    double   z;
    nodeptr  next;
    nodeptr  back;
    int      number;
    xarray  *x;
    int      xcoord, ycoord, ymin, ymax;
    char     name[nmlngth+1];             //  Space for null termination
    char    *tip;                         //  Pointer to sequence data
};


struct tree {
    double  likelihood;
    double  log_f[maxpatterns];
    nodeptr nodep[MAXNODES];
    nodeptr start;
    int     mxtips;
    int     ntips;
    int     nextnode;
    int     opt_level;
    bool    smoothed;
    bool    rooted;
};

struct drawdata {
    double tipmax;
    int    tipy;
};




