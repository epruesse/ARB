#define programName     "fastDNAml"
#define programVersion  "1.0.4"
#define programDate     "July 13, 1992"

/*  Copyright notice from dnaml:
 *
 *  version 3.3. (c) Copyright 1986, 1990 by the University of Washington
 *  and Joseph Felsenstein.  Written by Joseph Felsenstein.  Permission is
 *  granted to copy and use this program provided no fee is charged for it
 *  and provided that this copyright notice is not removed.
 *
 *  Conversion to C and changes in sequential code by Gary Olsen, 1991-1992
 *
 *  p4 version by Hideo Matsuda and Ross Overbeek, 1991-1992
 */

/*
 *  1.0.1  Add ntaxa to tree comments
 *         Set minimum branch length on reading tree
 *         Add blanks around operators in treeString (for prolog parsing)
 *         Add program version to treeString comments
 *
 *  1.0.2  Improved option line diagnostics
 *         Improved auxiliary line diagnostics
 *         Removed some trailing blanks from output
 *
 *  1.0.3  Checkpoint trees that do not need any optimization
 *         Print restart tree likelihood before optimizing
 *         Fix treefile option so that it really toggles
 *
 *  1.0.4  Add support for tree fact (instead of true Newick tree) in
 *            processTreeCom, treeReadLen, str_processTreeCom and
 *            str_treeReadLen
 *         Use bit operations in randum
 *         Correct error in bootstrap mask used with weighting mask
 */

#ifdef Master
#  undef  Master
#  define Master     1
#  define Slave      0
#  define Sequential 0
#else
#  ifdef Slave
#    undef Slave
#    define Master     0
#    define Slave      1
#    define Sequential 0
#  else
#    ifdef Sequential
#      undef Sequential
#    endif
#    define Master     0
#    define Slave      0
#    define Sequential 1
#  endif
#endif

#include <stdio.h>
#include <math.h>
#include "fastDNAml_1_0.h"

#if Master || Slave
#include "p4.h"
#include "comm_link.h" 
#endif

/*  Global variables */

longer  seed;      /*  random number seed with 6 bits per element */

xarray
    *usedxtip, *freextip;

double
    rate[maxcategories+1],    /*  rates for categories */
    ratvalue[maxpatterns],    /*  rates per site */
    wgt_rate[maxpatterns],    /*  weighted rate per site */
    wgt_rate2[maxpatterns];   /*  weighted rate**2 per site */

double
    xi, xv, ttratio,          /*  transition/transversion info */
    freqa, freqc, freqg, freqt,  /*  base frequencies */
    freqr, freqy, invfreqr, invfreqy,
    freqar, freqcy, freqgr, freqty,
    totalwrate,    /*  total of weighted rate values */
    fracchange;    /*  random matching fraquency (in a sense) */

int
    alias[maxsites+1],        /*  site corresponding to pattern */
    aliasweight[maxsites+1],  /*  weight of given pattern */
    category[maxsites+1],     /*  rate category of sequence positions */
    catnumb[maxpatterns],     /*  category number by pattern number */
    weight[maxsites+1];       /*  weight of sequence positions */

int
    categs,        /*  number of rate categories */
    endsite,       /*  number of unique sequence patterns */
    extrainfo,     /*  runtime information switch */
    numsp,         /*  number of species (same as tr->mxtips) */
    nkeep,         /*  number of best trees to keep */
    sites,         /*  number of input sequence positions */
    trout,         /*  write tree to "treefile" */
    weightsum;     /*  sum of weights of positions in analysis */

boolean
    anerror,       /*  error flag */
    bootstrap,     /*  do a set of bootstrap column weights */
    freqsfrom,     /*  use empirical base frequencies */
    interleaved,   /*  input data are in interleaved format */
    jumble,        /*  use random addition order */
    outgropt,      /*  use user-supplied outgroup */
    printdata,     /*  echo data to output stream */
    fastadd,       /*  test addition sites without global smoothing */
    restart,       /*  continue sequential addition to partial tree */
    treeprint;     /*  print tree to output stream */

char
    *y[maxsp+1];   /*  sequence data array */

#if Sequential     /*  Use standard input */
#  undef   DNAML_STEP
#  define  DNAML_STEP  0
#  define  INFILE  stdin
#endif

#if Master
#  define MAX_SEND_AHEAD 400
  char   *best_tr_recv = NULL;     /* these are used for flow control */
  double  best_lk_recv;
  int     send_ahead = 0;          /* number of outstanding sends */

#  ifdef DNAML_STEP
#    define  DNAML_STEP  1
#  endif
#  define  INFILE  Seqf
#  define  OUTFILE Outf
  FILE *INFILE, *OUTFILE;
  comm_block comm_slave;
#endif

#if Slave
#  undef   DNAML_STEP
#  define  DNAML_STEP  0
#  define  INFILE  Seqf
#  define  OUTFILE Outf
  FILE *INFILE, *OUTFILE;
  comm_block comm_master;
#endif

#if DNAML_STEP
  int begin_step_time, end_step_time;
#endif

#if Debug
  FILE *debug;
#endif

#if DNAML_STEP
#  define  REPORT_ADD_SPECS  p4_send(DNAML_ADD_SPECS, DNAML_HOST_ID, NULL, 0)
#  define  REPORT_SEND_TREE  p4_send(DNAML_SEND_TREE, DNAML_HOST_ID, NULL, 0)
#  define  REPORT_RECV_TREE  p4_send(DNAML_RECV_TREE, DNAML_HOST_ID, NULL, 0)
#  define  REPORT_STEP_TIME \
   {\
       char send_buf[80]; \
       end_step_time = p4_clock(); \
       (void) sprintf(send_buf, "%d", end_step_time-begin_step_time); \
       p4_send(DNAML_STEP_TIME, DNAML_HOST_ID, send_buf,strlen(send_buf)+1); \
       begin_step_time = end_step_time; \
   }
#else
#  define  REPORT_ADD_SPECS
#  define  REPORT_SEND_TREE
#  define  REPORT_RECV_TREE
#  define  REPORT_STEP_TIME
#endif

/*=======================================================================*/
/*                              PROGRAM                                  */
/*=======================================================================*/
/*                    Best tree handling for dnaml                       */
/*=======================================================================*/

/*  Tip value comparisons
 *
 *  Use void pointers to hide type from other routines.  Only tipValPtr and
 *  cmpTipVal need to be changed to alter the nature of the values compared
 *  (e.g., names instead of node numbers).
 *
 *    cmpTipVal(tipValPtr(nodeptr p), tipValPtr(nodeptr q)) == -1, 0 or 1.
 *
 *  This provides direct comparison of tip values (for example, for
 *  definition of tr->start).
 */


void  *tipValPtr (p)  nodeptr  p; { return  (void *) & p->number; }


int  cmpTipVal (v1, v2)
    void  *v1, *v2;
  { /* cmpTipVal */
    int  i1, i2;

    i1 = *((int *) v1);
    i2 = *((int *) v2);
    return  (i1 < i2) ? -1 : ((i1 == i2) ? 0 : 1);
  } /* cmpTipVal */


/*  These are the only routines that need to UNDERSTAND topologies */


topol  *setupTopol (maxtips, nsites)
    int     maxtips, nsites;
  { /* setupTopol */
    topol   *tpl;

    if (! (tpl = (topol *) malloc((unsigned) sizeof(topol))) || 
        ! (tpl->links = (connptr) malloc((unsigned) ((2 * maxtips - 3) *
                                                     sizeof(connect)))) || 
        (nsites && ! (tpl->log_f
                = (double *) malloc((unsigned) (nsites * sizeof(double)))))) {
      printf("ERROR: Unable to get topology memory");
      tpl = (topol *) NULL;
      }

    else {
      if (! nsites)  tpl->log_f = (double *) NULL;
      tpl->likelihood  = unlikely;
      tpl->start       = (node *) NULL;
      tpl->nextlink    = 0;
      tpl->ntips       = 0;
      tpl->nextnode    = 0;
      tpl->opt_level   = 0;     /* degree of branch swapping explored */
      tpl->scrNum      = 0;     /* position in sorted list of scores */
      tpl->tplNum      = 0;     /* position in sorted list of trees */
      tpl->log_f_valid = 0;     /* log_f value sites */
      tpl->smoothed    = FALSE; /* branch optimization converged? */
      }

    return  tpl;
  } /* setupTopol */


void  freeTopol (tpl)
    topol  *tpl;
  { /* freeTopol */
    free((char *) tpl->links);
    if (tpl->log_f)  free((char *) tpl->log_f);
    free((char *) tpl);
  } /* freeTopol */


int  saveSubtree (p, tpl)
    nodeptr  p;
    topol   *tpl;
    /*  Save a subtree in a standard order so that earlier branches
     *  from a node contain lower value tips than do second branches from
     *  the node.  This code works with arbitrary furcations in the tree.
     */
  { /* saveSubtree */
    connptr  r, r0;
    nodeptr  q, s;
    int      t, t0, t1;

    r0 = tpl->links;
    r = r0 + (tpl->nextlink)++;
    r->p = p;
    r->q = q = p->back;
    r->z = p->z;
    r->descend = 0;                     /* No children (yet) */

    if (q->tip) {
      r->valptr = tipValPtr(q);         /* Assign value */
      }

    else {                              /* Internal node, look at children */
      s = q->next;                      /* First child */
      do {
        t = saveSubtree(s, tpl);        /* Generate child's subtree */

        t0 = 0;                         /* Merge child into list */
        t1 = r->descend;
        while (t1 && (cmpTipVal(r0[t1].valptr, r0[t].valptr) < 0)) {
          t0 = t1;
          t1 = r0[t1].sibling;
          }
        if (t0) r0[t0].sibling = t;  else  r->descend = t;
        r0[t].sibling = t1;

        s = s->next;                    /* Next child */
        } while (s != q);

      r->valptr = r0[r->descend].valptr;   /* Inherit first child's value */
      }                                 /* End of internal node processing */

    return  r - r0;
  } /* saveSubtree */


nodeptr  minSubtreeTip (p0)
    nodeptr  p0;
  { /* minTreeTip */
    nodeptr  minTip, p, testTip;

    if (p0->tip) return p0;

    p = p0->next;
    minTip = minSubtreeTip(p->back);
    while ((p = p->next) != p0) {
      testTip = minSubtreeTip(p->back);
      if (cmpTipVal(tipValPtr(testTip), tipValPtr(minTip)) < 0)
        minTip = testTip;
      }
    return minTip;
  } /* minTreeTip */


nodeptr  minTreeTip (p)
    nodeptr  p;
  { /* minTreeTip */
    nodeptr  minp, minpb;

    minp  = minSubtreeTip(p);
    minpb = minSubtreeTip(p->back);
    return cmpTipVal(tipValPtr(minp), tipValPtr(minpb)) < 0 ? minp : minpb;
  } /* minTreeTip */


void saveTree (tr, tpl)
    tree   *tr;
    topol  *tpl;
    /*  Save a tree topology in a standard order so that first branches
     *  from a node contain lower value tips than do second branches from
     *  the node.  The root tip should have the lowest value of all.
     */
  { /* saveTree */
    connptr  r;
    double  *tr_log_f, *tpl_log_f;
    int  i;

    tpl->nextlink = 0;                             /* Reset link pointer */
    r = tpl->links + saveSubtree(minTreeTip(tr->start), tpl);  /* Save tree */
    r->sibling = 0;

    tpl->likelihood = tr->likelihood;
    tpl->start      = tr->start;
    tpl->ntips      = tr->ntips;
    tpl->nextnode   = tr->nextnode;
    tpl->opt_level  = tr->opt_level;
    tpl->smoothed   = tr->smoothed;

    if (tpl_log_f = tpl->log_f) {
      tr_log_f  = tr->log_f;
      i = tpl->log_f_valid = tr->log_f_valid;
      while (--i >= 0)  *tpl_log_f++ = *tr_log_f++;
      }
    else {
      tpl->log_f_valid = 0;
      }
  } /* saveTree */


void restoreTree (tpl, tr)
    topol  *tpl;
    tree   *tr;
  { /* restoreTree */
    void  hookup();
    void  initrav();

    connptr  r;
    nodeptr  p, p0;
    double  *tr_log_f, *tpl_log_f;
    int  i;

/*  Clear existing connections */

    for (i = 1; i <= 2*(tr->mxtips) - 2; i++) {  /* Uses p = p->next at tip */
      p0 = p = tr->nodep[i];
      do {
        p->back = (nodeptr) NULL;
        p = p->next;
        } while (p != p0);
      }

/*  Copy connections from topology */

    for (r = tpl->links, i = 0; i < tpl->nextlink; r++, i++) {
      hookup(r->p, r->q, r->z);
      }

    tr->likelihood = tpl->likelihood;
    tr->start      = tpl->start;
    tr->ntips      = tpl->ntips;
    tr->nextnode   = tpl->nextnode;
    tr->opt_level  = tpl->opt_level;
    tr->smoothed   = tpl->smoothed;

    if (tpl_log_f = tpl->log_f) {
      tr_log_f = tr->log_f;
      i = tr->log_f_valid = tpl->log_f_valid;
      while (--i >= 0)  *tr_log_f++ = *tpl_log_f++;
      }
    else {
      tr->log_f_valid = 0;
      }

    initrav(tr->start);
    initrav(tr->start->back);
  } /* restoreTree */


int initBestTree (bt, newKeep, nsites)
    bestlist  *bt;
    int        newKeep, nsites;
  { /* initBestTree */
    int  i, nlogf;

    if (newKeep < 1) newKeep = 1;
    if (newKeep > maxkeep) newKeep = maxkeep;

    bt->best     = unlikely;
    bt->worst    = unlikely;
    bt->nvalid   = 0;
    bt->ninit    = -1;
    bt->numtrees = 0;
    bt->improved = FALSE;
    if (! (bt->start = setupTopol(numsp, nsites)))  return  0;

    for (i = bt->ninit + 1; i <= newKeep; i++) {
      nlogf = (i <= maxlogf) ? nsites : 0;
      if (! (bt->byScore[i] = setupTopol(numsp, nlogf)))  break;
      bt->byTopol[i] = bt->byScore[i];
      bt->ninit = i;
      }

    return  (bt->nkeep = bt->ninit);
  } /* initBestTree */


int resetBestTree (bt)
    bestlist  *bt;
  { /* resetBestTree */
    bt->best = unlikely;
    bt->worst = unlikely;
    bt->nvalid = 0;
    bt->improved = FALSE;
  } /* resetBestTree */


void  freeBestTree(bt)
    bestlist  *bt;
  { /* freeBestTree */
    while (bt->ninit >= 0)  freeTopol(bt->byScore[(bt->ninit)--]);
    freeTopol(bt->start);
  } /* freeBestTree */


int  saveBestTree (bt, tr)
    bestlist  *bt;
    tree      *tr;
  { /* saveBestTree */
    topol  *tpl;
    int  scrNum, tplNum;

    if ((bt->nvalid < bt->nkeep) || (tr->likelihood > bt->worst)) {
      scrNum = 1;
      tplNum = 1;
      tpl = bt->byScore[scrNum];
      saveTree(tr, tpl);
      tpl->scrNum = scrNum;
      tpl->tplNum = tplNum;

      bt->byTopol[tplNum] = tpl;
      bt->nvalid          = 1;
      bt->best            = tr->likelihood;
      bt->worst           = tr->likelihood;
      bt->improved        = TRUE;
      }

    else
      scrNum = 0;

    return  scrNum;
  } /* saveBestTree */


int  startOpt (bt, tr)
    bestlist  *bt;
    tree      *tr;
  { /* startOpt */
    int  scrNum;

    bt->nvalid = 0;
    scrNum = saveBestTree (bt, tr);
    bt->improved = FALSE;
    return  scrNum;
  } /* startOpt */


int  setOptLevel (bt, level)
    bestlist *bt;
    int       level;
  { /* setOptLevel */
    int  scrNum;

    if (! bt->improved) {
      bt->byScore[1]->opt_level = level;
      scrNum = 1;
      }
    else
      scrNum = 0;

    return  scrNum;
  } /* setOptLevel */


int  recallBestTree (bt, rank, tr)
    bestlist  *bt;
    int        rank;
    tree      *tr;
  { /* recallBestTree */
    if (rank < 1)  rank = 1;
    if (rank > bt->nvalid)  rank = bt->nvalid;
    if (rank > 0)  restoreTree(bt->byScore[rank], tr);
    return  rank;
  } /* recallBestTree */

/*=======================================================================*/
/*                       End of best tree routines                       */
/*=======================================================================*/


#if 0
  void hang(msg) char *msg; {printf("Hanging around: %s\n", msg); while(1);}
#endif


void getnums (tr)
    tree  *tr;
    /* input number of species, number of sites */
  { /* getnums */
    printf("\n%s, version %s, %s\n\n",
            programName,
            programVersion,
            programDate);
    printf("Based on Joseph Felsenstein's\n\n");
    printf("Nucleic acid sequence Maximum Likelihood method, ");
    printf("version 3.3\n\n");

    if (fscanf(INFILE, "%d %d", &numsp, &sites) != 2) {
      printf("ERROR: Problem reading number of species and sites\n");
      anerror = TRUE;
      return;
      }
    printf("%d Species, %d Sites\n\n", numsp, sites);

    if (numsp > maxsp) {
      printf("TOO MANY SPECIES: adjust CONSTants\n");
      anerror = TRUE;
      }
    else if (numsp < 4) {
      printf("TOO FEW SPECIES\n");
      anerror = TRUE;
      }

    if (sites > maxsites) {
      printf("TOO MANY SITES: adjust CONSTants\n");
      anerror = TRUE;
      }
    else if (sites < 1) {
      printf("TOO FEW SITES\n");
      anerror = TRUE;
      }

    tr->mxtips = numsp;
  } /* getnums */


boolean digit (ch) int ch; {return (ch >= '0' && ch <= '9'); }


boolean white (ch) int ch; { return (ch == ' ' || ch == '\n' || ch == '\t'); }


void uppercase (chptr)
      int *chptr;
    /* convert character to upper case -- either ASCII or EBCDIC */
  { /* uppercase */
    int  ch;

    ch = *chptr;
    if ((ch >= 'a' && ch <= 'i') || (ch >= 'j' && ch <= 'r')
                                 || (ch >= 's' && ch <= 'z'))
      *chptr = ch + 'A' - 'a';
  } /* uppercase */


int base36 (ch)
    int ch;
  { /* base36 */
    if      (ch >= '0' && ch <= '9') return (ch - '0');
    else if (ch >= 'A' && ch <= 'I') return (ch - 'A' + 10);
    else if (ch >= 'J' && ch <= 'R') return (ch - 'J' + 19);
    else if (ch >= 'S' && ch <= 'Z') return (ch - 'S' + 28);
    else if (ch >= 'a' && ch <= 'i') return (ch - 'a' + 10);
    else if (ch >= 'j' && ch <= 'r') return (ch - 'j' + 19);
    else if (ch >= 's' && ch <= 'z') return (ch - 's' + 28);
    else return -1;
  } /* base36 */


int itobase36 (i)
    int  i;
  { /* itobase36 */
    if      (i <  0) return '?';
    else if (i < 10) return (i      + '0');
    else if (i < 19) return (i - 10 + 'A');
    else if (i < 28) return (i - 19 + 'J');
    else if (i < 36) return (i - 28 + 'S');
    else return '?';
  } /* itobase36 */


int findch (c)
    int  c;
  { /* findch */
    int ch;

    while ((ch = getc(INFILE)) != EOF && ch != c) ;
    return  ch;
  } /* findch */


#if Master || Slave
int str_findch (treestrp, c)
    char **treestrp;
    int  c;
  { /* str_findch */
    int ch;

    while ((ch = *(*treestrp)++) != NULL && ch != c) ;
    return  ch;
  } /* str_findch */
#endif


void inputcategories ()
    /* reads the categories for each site */
  { /* inputcategories */
    int  i, ch, ci;

    for (i = 1; i <= nmlngth; i++)  (void) getc(INFILE);
    i = 1;
    while (i <= sites) {
      ch = getc(INFILE);
      ci = base36(ch);
      if (ci >= 0 && ci <= categs)
        category[i++] = ci;
      else if (! white(ch)) {
        printf("ERROR: Bad category character (%c) at site %d\n", ch, i);
        anerror = TRUE;
        return;
        }
      }

    if (findch('\n') == EOF) {      /* skip to end of line */
      printf("ERROR: Missing newline at end of category data\n");
      anerror = TRUE;
      }
  } /* inputcategories */


void inputweights ()
    /* input the character weights 0, 1, 2 ... 9, A, B, ... Y, Z */
  { /* inputweights */
    int i, ch, wi;

    for (i = 2; i <= nmlngth; i++)  (void) getc(INFILE);
    weightsum = 0;
    i = 1;
    while (i <= sites) {
      ch = getc(INFILE);
      wi = base36(ch);
      if (wi >= 0)
        weightsum += weight[i++] = wi;
      else if (! white(ch)) {
        printf("ERROR: Bad weight character: '%c'", ch);
        printf("       Weights in dnaml must be a digit or a letter.\n");
        anerror = TRUE;
        return;
        }
      }

    if (findch('\n') == EOF) {      /* skip to end of line */
      printf("ERROR: Missing newline at end of weight data\n");
      anerror = TRUE;
      }
  } /* inputweights */


void getoptions (tr)
    tree  *tr;
  { /* getoptions */
    longer  bseed;
    long    inseed, binseed;    /*  random number seed input from user */
    int     ch, i, j, extranum;
    double  randum();

    bootstrap    =  FALSE;  /* Don't bootstrap column weights */
    categs       =      0;  /* Number of rate categories */
    extrainfo    =      0;  /* No extra runtime info unless requested */
    freqsfrom    =  FALSE;  /* Use empirical base frequencies */
    tr->global   =     -1;  /* Default search locale for optimum */
    tr->partswap =      1;  /* Default to swap locally after insert */
    interleaved  =   TRUE;  /* By default, data format is interleaved */
    jumble       =  FALSE;  /* Use random addition sequence */
    nkeep        =      1;  /* Keep only the one best tree */
    tr->outgr    =      1;  /* Outgroup number */
    outgropt     =  FALSE;  /* User-defined outgroup rooting */
    printdata    =  FALSE;  /* Don't echo data to output stream */
    fastadd      = Master;  /* Smooth branches globally in add */
    rate[1]      =    1.0;  /* Rate values */
    restart      =  FALSE;  /* Restart from user tree */
    treeprint    =   TRUE;  /* Print tree to output stream */
    trout        =      0;  /* Output tree file */
    ttratio      =    2.0;  /* Transition/transversion rate ratio */
    tr->userlen  =  FALSE;  /* User-supplied branch lengths */
    tr->usertree =  FALSE;  /* User-defined tree topologies */
    tr->userwgt  =  FALSE;  /* User-defined position weights */
    extranum     =      0;

    while ((ch = getc(INFILE)) != '\n' && ch != EOF) {
      uppercase(& ch);
      switch (ch) {
          case '1' : printdata    = ! printdata; break;
          case '3' : treeprint    = ! treeprint; break;
          case '4' : trout        = 1; break;
          case 'B' : bootstrap    = TRUE; binseed = 0; extranum++; break;
          case 'C' : categs       = -1; extranum++; break;
          case 'E' : extrainfo    = -1; break;
          case 'F' : freqsfrom    = TRUE; break;
          case 'G' : tr->global   = -2; break;
          case 'I' : interleaved  = ! interleaved; break;
          case 'J' : jumble       = TRUE; inseed = 0; extranum++; break;
          case 'K' : extranum++;          break;
          case 'L' : tr->userlen  = TRUE; break;
          case 'O' : outgropt     = TRUE; tr->outgr = 0; extranum++; break;
          case 'Q' : fastadd      = ! Master; break;
          case 'R' : restart      = TRUE; break;
          case 'T' : ttratio      = -1.0; extranum++; break;
          case 'U' : tr->usertree = TRUE; break;
          case 'W' : tr->userwgt  = TRUE; weightsum = 0; extranum++; break;
          case 'Y' : trout        = 1 - trout; break;
          case ' ' : break;
          case '\t': break;
          default  :
              printf("ERROR: Bad option character: '%c'\n", ch);
              anerror = TRUE;
              return;
          }
      }

    if (ch == EOF) {
      printf("ERROR: End-of-file in options list\n");
      anerror = TRUE;
      return;
      }

    if (tr->usertree && restart) {
      printf("ERROR:  The restart and user-tree options conflict:\n");
      printf("        Restart adds rest of taxa to a starting tree;\n");
      printf("        User-tree does not add any taxa.\n\n");
      anerror = TRUE;
      return;
      }
    if (tr->usertree && jumble) {
      printf("WARNING:  The jumble and user-tree options conflict:\n");
      printf("          Jumble adds taxa to a tree in random order;\n");
      printf("          User-tree does not use taxa addition.\n");
      printf("          Jumble ingnored for this run.\n\n");
      jumble = FALSE;
      }
    if (tr->userlen && tr->global != -1) {
      printf("ERROR:  The global and user-lengths options conflict:\n");
      printf("        Global optimizes a starting tree;\n");
      printf("        User-lengths constrain the starting tree.\n\n");
      anerror = TRUE;
      return;
      }
    if (tr->userlen && ! tr->usertree) {
      printf("WARNING:  User lengths required user tree option.\n");
      printf("          User-tree option set for this run.\n\n");
      tr->usertree = TRUE;
      }

    /*  process lines with auxiliary data */

    while (extranum--) {
      ch = getc(INFILE);
      uppercase(& ch);
      switch (ch) {

        case 'B':   /*  Bootstrap  */
            if (! bootstrap || binseed != 0) {
              printf("ERROR: Unexpected Bootstrap auxiliary data line\n");
              anerror = TRUE;
              }
            else if (fscanf(INFILE,"%ld",&binseed)!=1 || findch('\n')==EOF) {
              printf("ERROR: Problem reading boostrap random seed value\n");
              anerror = TRUE;
              }
            break;

        case 'C':   /*  Categories  */
            if (categs >= 0) {
              printf("ERROR: Unexpected Categories auxiliary data line\n");
              anerror = TRUE;
              }
            else if (fscanf(INFILE, "%d", &categs) != 1) {
              printf("ERROR: Problem reading number of rate categories\n");
              anerror = TRUE;
              }
            else if (categs < 1 || categs > maxcategories) {
              printf("ERROR: Bad number of categories: %d\n", categs);
              printf("Must be in range 1 - %d\n", maxcategories);
              anerror = TRUE;
              }
            else {
              for (j = 1; j <= categs
                       && fscanf(INFILE, "%lf", &(rate[j])) == 1; j++) ;

              if ((j <= categs) || (findch('\n') == EOF)) {
                printf("ERROR: Problem reading rate values\n");
                anerror = TRUE;
                }
              else
                inputcategories();
              }
            break;

        case 'E':   /*  Extra output information */
            if (fscanf(INFILE,"%d",&extrainfo) != 1 || findch('\n') == EOF) {
              printf("ERROR: Problem reading extra info value\n");
              anerror = TRUE;
              }
            extranum++;  /*  Don't count this line since it is optional */
            break;

        case 'G':   /*  Global  */
            if (tr->global != -2) {
              printf("ERROR: Unexpected Global auxiliary data line\n");
              anerror = TRUE;
              }
            else if (fscanf(INFILE, "%d", &(tr->global)) != 1) {
              printf("ERROR: Problem reading rearrangement region size\n");
              anerror = TRUE;
              }
            else if (tr->global < 0) {
              printf("WARNING: Global region size too small;\n");
              printf("         value reset to local\n\n");
              tr->global = 1;
              }
            else if (tr->global == 0)  tr->partswap = 0;
            else if (tr->global > tr->mxtips - 3) {
              tr->global = tr->mxtips - 3;
              }

            while ((ch = getc(INFILE)) != '\n') {  /* Scan for second value */
              if (! white(ch)) {
                if (ch != EOF)  (void) ungetc(ch, INFILE);
                if (ch == EOF || fscanf(INFILE, "%d", &(tr->partswap)) != 1
                              || findch('\n') == EOF) {
                  printf("ERROR: Problem reading insert swap region size\n");
                  anerror = TRUE;
                  }
                else if (tr->partswap < 0)  tr->partswap = 1;
                else if (tr->partswap > tr->mxtips - 3) {
                  tr->partswap = tr->mxtips - 3;
                  }

                if (tr->partswap > tr->global)  tr->global = tr->partswap;
                break;   /*  Break while loop */
                }
              }

            extranum++;  /*  Don't count this line since it is optional */
            break;       /*  Break switch statement */

        case 'J':   /*  Jumble  */
            if (! jumble || inseed != 0) {
              printf("ERROR: Unexpected Jumble auxiliary data line\n");
              anerror = TRUE;
              }
            else if (fscanf(INFILE,"%ld",&inseed) != 1 || findch('\n')==EOF) {
              printf("ERROR: Problem reading jumble random seed value\n");
              anerror = TRUE;
              }
            break;

        case 'K':   /*  Keep  */
            if (fscanf(INFILE, "%d", &nkeep) != 1 || findch('\n') == EOF ||
                nkeep < 1) {
              printf("ERROR: Problem reading number of kept trees\n");
              anerror = TRUE;
              }
            else if (nkeep > maxkeep) {
              printf("WARNING: Kept trees lowered from %d to %d\n\n",
                      nkeep, maxkeep);
              nkeep = maxkeep;
              }
            break;

        case 'O':   /*  Outgroup  */
            if (! outgropt || tr->outgr > 0) {
              printf("ERROR: Unexpected Outgroup auxiliary data line\n");
              anerror = TRUE;
              }
            else if (fscanf(INFILE, "%d", &(tr->outgr)) != 1 ||
                findch('\n') == EOF) {
              printf("ERROR: Problem reading outgroup number\n");
              anerror = TRUE;
              }
            else if ((tr->outgr < 1) || (tr->outgr > tr->mxtips)) {
              printf("ERROR: Bad outgroup: '%d'\n", tr->outgr);
              anerror = TRUE;
              }
            break;

        case 'T':   /*  Transition/transversion ratio  */
            if (ttratio >= 0.0) {
              printf("ERROR: Unexpected Transition/transversion auxiliary data\n");
              anerror = TRUE;
              }
            else if (fscanf(INFILE,"%lf",&ttratio)!=1 || findch('\n')==EOF) {
              printf("ERROR: Problem reading transition/transversion ratio\n");
              anerror = TRUE;
              }
            break;

        case 'W':    /*  Weights  */
            if (! tr->userwgt || weightsum > 0) {
              printf("ERROR: Unexpected Weights auxiliary data\n");
              anerror = TRUE;
              }
            else {
              inputweights();
              }
            break;

        case 'Y':    /*  Output tree file  */
            if (! trout) {
              printf("ERROR: Unexpected Treefile auxiliary data\n");
              anerror = TRUE;
              }
            else if (fscanf(INFILE,"%d",&trout) != 1 || findch('\n') == EOF) {
              printf("ERROR: Problem reading output tree-type number\n");
              anerror = TRUE;
              }
            else if ((trout < 0) || (trout > 2)) {
              printf("ERROR: Bad output tree-type number: '%d'\n", trout);
              anerror = TRUE;
              }
            extranum++;  /*  Don't count this line since it is optional */
            break;

        default:
            printf("ERROR: Auxiliary options line starts with '%c'\n", ch);
            anerror = TRUE;
            break;
        }

        if (anerror)  return;
      }

    if (anerror) return;

    if (! tr->userwgt) {
      for (i = 1; i <= sites; i++) weight[i] = 1;
      weightsum = sites;
      }

    if (tr->userwgt && weightsum < 1) {
      printf("ERROR:  Missing or bad user-supplied weight data.\n");
      anerror = TRUE;
      return;
      }

    if (bootstrap) {
      int  nonzero;

      printf("Bootstrap random number seed = %ld\n\n", binseed);

      if (binseed < 0)  binseed = -binseed;
      for (i = 0; i <= 5; i++) {bseed[i] = binseed & 63; binseed >>= 6;}

      nonzero = 0;
      for (i = 1; i <= sites; i++)  if (weight[i] > 0) nonzero++;

      for (j = 1; j <= nonzero; j++) aliasweight[j] = 0;
      for (j = 1; j <= nonzero; j++)
        aliasweight[(int) (nonzero*randum(bseed)) + 1]++;

      j = 0;
      weightsum = 0;
      for (i = 1; i <= sites; i++) {
        if (weight[i] > 0) {
          weightsum += (weight[i] *= aliasweight[++j]);
          }
        }
      }

    if (jumble) {
      printf("Jumble random number seed = %ld\n\n", inseed);
      if (inseed < 0)  inseed = -inseed;
      for (i = 0; i <= 5; i++) {seed[i] = inseed & 63; inseed >>= 6;}
      }

    if (categs > 0) {
      printf("Site category   Rate of change\n\n");
      for (i = 1; i <= categs; i++)
        printf("           %c%13.3f\n", itobase36(i), rate[i]);
      putchar('\n');
      for (i = 1; i <= sites; i++) {
        if ((weight[i] > 0) && (category[i] < 1)) {
          printf("ERROR: Bad category (%c) at site %d\n",
                  itobase36(category[i]), i);
          anerror = TRUE;
          return;
          }
        }
      }
    else if (categs < 0) {
      printf("ERROR: Category auxiliary data missing from input\n");
      anerror = TRUE;
      return;
      }
    else {                                              /* categs == 0 */
      for (i = 1; i <= sites; i++) category[i] = 1;
      categs = 1;
      }

    if (tr->outgr < 1) {
      printf("ERROR: Outgroup auxiliary data missing from input\n");
      anerror = TRUE;
      return;
      }

    if (ttratio < 0.0) {
      printf("ERROR: Transition/transversion auxiliary data missing from input\n");
      anerror = TRUE;
      return;
      }

    if (tr->global < 0) {
      if (tr->global == -2) tr->global = tr->mxtips - 3;  /* Default global */
      else                  tr->global = tr->usertree ? 0 : 1; /* No global */
      }

    if (restart) {
      printf("Restart option in effect.  ");
      printf("Sequence addition will start from appended tree.\n\n");
      }

    if (tr->usertree && ! tr->global) {
      printf("User-supplied tree topology%swill be used.\n\n",
        tr->userlen ? " and branch lengths " : " ");
      }
    else {
      if (! tr->usertree) {
        printf("Rearrangements of partial trees may cross %d %s.\n",
               tr->partswap, tr->partswap == 1 ? "branch" : "branches");
        }
      printf("Rearrangements of full tree may cross %d %s.\n\n",
             tr->global, tr->global == 1 ? "branch" : "branches");
      }
  } /* getoptions */


void getbasefreqs ()
  { /* getbasefreqs */
    double  suma, sumb;

    if (freqsfrom) printf("Empirical ");
    printf("Base Frequencies:\n\n");

    if (! freqsfrom) {
      if (fscanf(INFILE, "%lf%lf%lf%lf", &freqa, &freqc, &freqg, &freqt) != 4
                                                || findch('\n') == EOF) {
        printf("ERROR: Problem reading user base frequencies\n");
        anerror = TRUE;
        return;
        }
      }

    printf("   A    %10.5f\n", freqa);
    printf("   C    %10.5f\n", freqc);
    printf("   G    %10.5f\n", freqg);
    printf("  T(U)  %10.5f\n\n", freqt);
    freqr = freqa + freqg;
    invfreqr = 1.0/freqr;
    freqar = freqa * invfreqr;
    freqgr = freqg * invfreqr;
    freqy = freqc + freqt;
    invfreqy = 1.0/freqy;
    freqcy = freqc * invfreqy;
    freqty = freqt * invfreqy;
    printf("Transition/transversion ratio = %10.6f\n\n", ttratio);
    suma = ttratio*freqr*freqy - (freqa*freqg + freqc*freqt);
    sumb = freqa*freqgr + freqc*freqty;
    xi = suma/(suma+sumb);
    xv = 1.0 - xi;
    if (xi <= 0.0) {
      printf("WARNING: This transition/transversion ratio\n");
      printf("         is impossible with these base frequencies!\n");
      printf("Transition/transversion parameter reset\n\n");
      xi = 3/5;
      xv = 2/5;
      }
    printf("(Transition/transversion parameter = %10.6f)\n\n", xi/xv);
    fracchange = xi*(2*freqa*freqgr + 2*freqc*freqty)
               + xv*(1.0 - freqa*freqa - freqc*freqc
                         - freqg*freqg - freqt*freqt);
  } /* getbasefreqs */


void getyspace ()
  { /* getyspace */
    long size;
    int  i;
    char *y0;

    size = 4 * (sites/4 + 1);
    if (! (y0 = malloc((unsigned) ((numsp+1) * size * sizeof(char))))) {
      printf("ERROR: Unable to obtain space for data array\n");
      anerror = TRUE;
      return;
      }

    for (i = 0; i <= numsp; i++) {
      y[i] = y0;
      y0 += size;
      }
  } /* getyspace */


void setupTree (tr)
    tree  *tr;
  { /* setupTree */
    nodeptr  p0, p, q;
    int  i, j, tips, inter;

    tips  = tr->mxtips;
    inter = tr->mxtips - 1;

    if (!(p0 = (nodeptr) malloc((unsigned) ((tips + 3*inter) *
                                             sizeof(node))))) {
      printf("ERROR: Unable to obtain sufficient tree memory\n");
      anerror = TRUE;
      return;
      }

    tr->nodep[0] = (node *) NULL;    /* Use as 1-based array */

    for (i = 1; i <= tips; i++) {   /* Set-up tips */
      p = p0++;
      p->x      = (xarray *) NULL;
      p->tip    = (char *) NULL;
      p->number = i;
      p->next   = p;
      p->back   = (node *) NULL;

      tr->nodep[i] = p;
      }

    for (i = tips + 1; i <= tips + inter; i++) { /* Internal nodes */
      q = (node *) NULL;
      for (j = 1; j <= 3; j++) {
        p = p0++;
        p->x      = (xarray *) NULL;
        p->tip    = (char *) NULL;
        p->number = i;
        p->next   = q;
        p->back   = (node *) NULL;
        q = p;
        }
      p->next->next->next = p;
      tr->nodep[i] = p;
      }

    tr->likelihood  = unlikely;
    tr->start       = (node *) NULL;
    tr->outgrnode   = tr->nodep[tr->outgr];
    tr->ntips       = 0;
    tr->nextnode    = 0;
    tr->opt_level   = 0;
    tr->smoothed    = FALSE;
    tr->log_f_valid = 0;
  } /* setupTree */


void freeTreeNode (p)       /* Free tree node (sector) associated data */
    nodeptr  p;
  { /* freeTreeNode */
    if (p) {
      if (p->x) {
        if (p->x->a) free((char *) p->x->a);
        free((char *) p->x);
        }
      }
  } /* freeTreeNode */


void freeTree (tr)
    tree  *tr;
  { /* freeTree */
    nodeptr  p, q;
    int  i, tips, inter;

    tips  = tr->mxtips;
    inter = tr->mxtips - 1;

    for (i = 1; i <= tips; i++) freeTreeNode(tr->nodep[i]);

    for (i = tips + 1; i <= tips + inter; i++) {
      if (p = tr->nodep[i]) {
        if (q = p->next) {
          freeTreeNode(q->next);
          freeTreeNode(q);
          }
        freeTreeNode(p);
        }
      }

    free((char *) tr->nodep[1]);       /* Free the actual nodes */
  } /* freeTree */


void getdata (tr)
    tree  *tr;
    /* read sequences */
  { /* getdata */
    int  i, j, k, l, basesread, basesnew, ch;
    int  meaning[256];          /*  meaning of input characters */ 
    char *nameptr;
    boolean  allread, firstpass;

    for (i = 0; i <= 255; i++) meaning[i] = 0;
    meaning['A'] =  1;
    meaning['B'] = 14;
    meaning['C'] =  2;
    meaning['D'] = 13;
    meaning['G'] =  4;
    meaning['H'] = 11;
    meaning['K'] = 12;
    meaning['M'] =  3;
    meaning['N'] = 15;
    meaning['O'] = 15;
    meaning['R'] =  5;
    meaning['S'] =  6;
    meaning['T'] =  8;
    meaning['U'] =  8;
    meaning['V'] =  7;
    meaning['W'] =  9;
    meaning['X'] = 15;
    meaning['Y'] = 10;
    meaning['?'] = 15;
    meaning['-'] = 15;

    basesread = basesnew = 0;

    allread = FALSE;
    firstpass = TRUE;
    ch = ' ';

    while (! allread) {
      for (i = 1; i <= tr->mxtips; i++) {     /*  Read data line */
        if (firstpass) {                      /*  Read species names */
          j = 1;
          nameptr = tr->nodep[i]->name;
          while (white(ch = getc(INFILE))) {  /*  Skip blank lines */
            if (ch == '\n') {
              j = 1;
              nameptr = tr->nodep[i]->name;
              }
            else if (j++ <= nmlngth) *nameptr++ = ' ';
            }

          if (j > nmlngth) {
            printf("ERROR: Blank name for species %d; ", i);
            printf("check number of species,\n");
            printf("       number of sites, and interleave option.\n");
            anerror = TRUE;
            return;
            }

          if (ch != EOF) {
            *nameptr++ = ch;
            j++;
            while (j <= nmlngth) {
              if ((ch = getc(INFILE)) == '\n' || ch == EOF) break;
              if (ch == '_' || white(ch))  ch = ' ';
              *nameptr++ = ch;
              j++;
              }
            while (j++ <= nmlngth) *nameptr++ = ' ';
            *nameptr = '\0';                      /*  add null termination */
            }

          if (ch == EOF) {
            printf("ERROR: End-of-file in name of species %d\n", i);
            anerror = TRUE;
            return;
            }
          }

        j = basesread;
        while ((j < sites) && ((ch = getc(INFILE)) != EOF)
                           && ((! interleaved) || (ch != '\n'))) {
          uppercase(& ch);
          if (meaning[ch] || ch == '.') {
            j++;
            if (ch == '.') {
              if (i != 1) ch = y[1][j];
              else {
                printf("ERROR: Dot (.) found at site %d of sequence 1\n", j);
                anerror = TRUE;
                return;
                }
              }
            y[i][j] = ch;
            }
          else if (white(ch) || digit(ch)) ;
          else {
            printf("ERROR: Bad base (%c) at site %d of sequence %d\n",
                    ch, j, i);
            anerror = TRUE;
            return;
            }
          }

        if (ch == EOF) {
          printf("ERROR: End-of-file at site %d of sequence %d\n", j, i);
          anerror = TRUE;
          return;
          }

        if (! firstpass && (j == basesread)) i--;        /* no data on line */
        else if (i == 1) basesnew = j;
        else if (j != basesnew) {
          printf("ERROR: Sequences out of alignment\n");
          printf("%d (instead of %d) residues read in sequence %d\n",
                  j - basesread, basesnew - basesread, i);
          anerror = TRUE;
          return;
          }

        while (ch != '\n' && ch != EOF) ch = getc(INFILE);  /* flush line */
        }                                                  /* next sequence */
      firstpass = FALSE;
      basesread = basesnew;
      allread = (basesread >= sites);
      }

    /*  Print listing of sequence alignment */

    if (printdata) {
      j = nmlngth - 5 + ((sites + ((sites-1)/10))/2);
      if (j < nmlngth - 1) j = nmlngth - 1;
      if (j > 37) j = 37;
      printf("Name"); for (i=1;i<=j;i++) putchar(' '); printf("Sequences\n");
      printf("----"); for (i=1;i<=j;i++) putchar(' '); printf("---------\n");
      putchar('\n');

      for (i = 1; i <= sites; i += 60) {
        l = i + 59;
        if (l > sites) l = sites;

        if (tr->userwgt) {
          printf("Weights   ");
          for (j = 11; j <= nmlngth+3; j++) putchar(' ');
          for (k = i; k <= l; k++) {
            putchar(itobase36(weight[k]));
            if (((k % 10) == 0) && ((k % 60) != 0)) putchar(' ');
            }
          putchar('\n');
          }

        if (categs > 1) {
          printf("Categories");
          for (j = 11; j <= nmlngth+3; j++) putchar(' ');
          for (k = i; k <= l; k++) {
            putchar(itobase36(category[k]));
            if (((k % 10) == 0) && ((k % 60) != 0)) putchar(' ');
            }
          putchar('\n');
          }

        for (j = 1; j <= tr->mxtips; j++) {
          printf("%s   ", tr->nodep[j]->name);
          for (k = i; k <= l; k++) {
            ch = y[j][k];
            if ((j > 1) && (ch == y[1][k])) ch = '.';
            putchar(ch);
            if (((k % 10) == 0) && ((k % 60) != 0)) putchar(' ');
            }
          putchar('\n');
          }
        putchar('\n');
        }
      }

    for (j = 1; j <= tr->mxtips; j++)    /* Convert characters to meanings */
      for (i = 1; i <= sites; i++)  y[j][i] = meaning[y[j][i]];

  } /* getdata */


void getinput (tr)
    tree  *tr;
  { /* getinput */
    getnums(tr);                      if (anerror) return;
    getoptions(tr);                   if (anerror) return;
    if (! freqsfrom) getbasefreqs();  if (anerror) return;
    getyspace();                      if (anerror) return;
    setupTree(tr);                    if (anerror) return;
    getdata(tr);                      if (anerror) return;
  } /* getinput */


void sitesort ()
    /* Shell sort keeping sites, weights in same order */
  { /* sitesort */
    int  gap, i, j, jj, jg, k;
    boolean  flip, tied;

    for (gap = sites/2; gap > 0; gap /= 2) {
      for (i = gap + 1; i <= sites; i++) {
        j = i - gap;

        do {
          jj = alias[j];
          jg = alias[j+gap];
          flip = (category[jj] >  category[jg]);
          tied = (category[jj] == category[jg]);
          for (k = 1; (k <= numsp) && tied; k++) {
            flip = (y[k][jj] >  y[k][jg]);
            tied = (y[k][jj] == y[k][jg]);
            }
          if (flip) {
            alias[j]     = jg;
            alias[j+gap] = jj;
            j -= gap;
            }
          } while (flip && (j > 0));

        }  /* for (i ...   */
      }    /* for (gap ... */
  } /* sitesort */


void sitecombcrunch ()
    /* combine sites that have identical patterns (and nonzero weight) */
  { /* sitecombcrunch */
    int  i, sitei, j, sitej, k;
    boolean  tied;

    i = 0;
    alias[0] = alias[1];
    aliasweight[0] = 0;

    for (j = 1; j <= sites; j++) {
      sitei = alias[i];
      sitej = alias[j];
      tied = (category[sitei] == category[sitej]);

      for (k = 1; tied && (k <= numsp); k++)
        tied = (y[k][sitei] == y[k][sitej]);

      if (tied) {
        aliasweight[i] += weight[sitej];
        }
      else {
        if (aliasweight[i] > 0) i++;
        aliasweight[i] = weight[sitej];
        alias[i] = sitej;
        }
      }

    endsite = i;
    if (aliasweight[i] > 0) endsite++;
  } /* sitecombcrunch */


void makeweights ()
    /* make up weights vector to avoid duplicate computations */
  { /* makeweights */
    int  i;

    for (i = 1; i <= sites; i++)  alias[i] = i;
    sitesort();
    sitecombcrunch();
    if (endsite > maxpatterns) {
      printf("ERROR: Too many patterns in data\n");
      printf("       Increase maxpatterns to at least %d\n", endsite);
      anerror = TRUE;
      }
    else {
      printf("Analyzing %d distinct data patterns (columns)\n\n", endsite);
      }
  } /* makeweights */


void makevalues (tr)
    tree  *tr;
    /* set up fractional likelihoods at tips */
  { /* makevalues */
    double  temp;
    int  i, j, k;

    for (i = 1; i <= tr->mxtips; i++) {    /* Pack and move tip data */
      for (j = 0; j < endsite; j++)  y[i-1][j] = y[i][alias[j]];
      tr->nodep[i]->tip = &(y[i-1][0]);
      }

    totalwrate = 0.0;
    for (k = 0; k < endsite; k++) {
      catnumb[k] = i = category[alias[k]];
      ratvalue[k] = temp = rate[i];
      totalwrate += wgt_rate[k] = temp * aliasweight[k];
      wgt_rate2[k] = temp * temp * aliasweight[k];
      }
  } /* makevalues */


void empiricalfreqs (tr)
    tree  *tr;
    /* Get empirical base frequencies from the data */
  { /* empiricalfreqs */
    double  sum, suma, sumc, sumg, sumt, wj, fa, fc, fg, ft;
    int  i, j, k, code;
    char *yptr;

    freqa = 0.25;
    freqc = 0.25;
    freqg = 0.25;
    freqt = 0.25;
    for (k = 1; k <= 8; k++) {
      suma = 0.0;
      sumc = 0.0;
      sumg = 0.0;
      sumt = 0.0;
      for (i = 1; i <= tr->mxtips; i++) {
        yptr = tr->nodep[i]->tip;
        for (j = 0; j < endsite; j++) {
          code = *yptr++;
          fa = freqa * ( code       & 1);
          fc = freqc * ((code >> 1) & 1);
          fg = freqg * ((code >> 2) & 1);
          ft = freqt * ((code >> 3) & 1);
          wj = aliasweight[j] / (fa + fc + fg + ft);
          suma += wj * fa;
          sumc += wj * fc;
          sumg += wj * fg;
          sumt += wj * ft;
          }
        }
      sum = suma + sumc + sumg + sumt;
      freqa = suma / sum;
      freqc = sumc / sum;
      freqg = sumg / sum;
      freqt = sumt / sum;
      }
  } /* empiricalfreqs */


xarray *setupxarray ()
  { /* setupxarray */
    xarray  *x;
    xtype  *data;

    x = (xarray *) malloc((unsigned) sizeof(xarray));
    if (x) {
      data = (xtype *) malloc((unsigned) (4 * endsite * sizeof(xtype)));
      if (data) {
        x->a = data;
        x->c = data += endsite;
        x->g = data += endsite;
        x->t = data +  endsite;
        x->prev = x->next = x;
        x->owner = (node *) NULL;
        }
      else {
        free((char *) x);
        return (xarray *) NULL;
        }
      }
    return x;
  } /* setupxarray */


void linkxarray (req, min, freexptr, usedxptr)
    int  req, min;
    xarray **freexptr, **usedxptr;
    /*  Link a set of xarrays */
  { /* linkxarray */
    xarray  *first, *prev, *x;
    int  i;

    first = prev = (xarray *) NULL;
    i = 0;

    do {
      x = setupxarray();
      if (x) {
        if (! first) first = x;
        else {
          prev->next = x;
          x->prev = prev;
          }
        prev = x;
        i++;
        }
      else {
        printf("ERROR: Failure to get requested xarray memory\n");
        if (i < min) {
          anerror = TRUE;
          return;
          }
        }
      } while ((i < req) && x);

    if (first) {
      first->prev = prev;
      prev->next = first;
      }

    *freexptr = first;
    *usedxptr = (xarray *) NULL;
  } /* linkxarray */


void setupnodex (tr)
    tree  *tr;
  { /* setupnodex */
    nodeptr  p;
    int  i;

    for (i = tr->mxtips + 1; (i <= 2*(tr->mxtips) - 2); i++) {
      p = tr->nodep[i];
      if (anerror = !(p->x = setupxarray())) {
        printf("ERROR: Failure to get internal node xarray memory\n");
        return;
        }
      }
  } /* setupnodex */


xarray *getxtip (p)
    nodeptr  p;
  { /* getxtip */
    xarray  *new;
    boolean  splice;

    if (! p) return (xarray *) NULL;

    splice = FALSE;

    if (p->x) {                  /* array is there; move to tail of list */
      new = p->x;
      if (new == new->prev) ;             /* linked to self; leave it */
      else if (new == usedxtip) usedxtip = usedxtip->next; /* at head */
      else if (new == usedxtip->prev) ;   /* already at tail */
      else {                              /* move to tail of list */
        new->prev->next = new->next;
        new->next->prev = new->prev;
        splice = TRUE;
        }
      }

    else if (freextip) {                 /* take from unused list */
      p->x = new = freextip;
      new->owner = p;
      if (new->prev != new) {            /* not only member of freelist */
        new->prev->next = new->next;
        new->next->prev = new->prev;
        freextip = new->next;
        }
      else
        freextip = (xarray *) NULL;

      splice = TRUE;
      }

    else if (usedxtip) {                 /* take from head of used list */
      usedxtip->owner->x = (xarray *) NULL;
      p->x = new = usedxtip;
      new->owner = p;
      usedxtip = usedxtip->next;
      }

    else {
      printf("ERROR: Unable to locate memory for tip %d.\n", p->number);
      anerror = TRUE;
      exit(1);
      }

    if (splice) {
      if (usedxtip) {                  /* list is not empty */
        usedxtip->prev->next = new;
        new->prev = usedxtip->prev;
        usedxtip->prev = new;
        new->next = usedxtip;
        }
      else
        usedxtip = new->prev = new->next = new;
      }

    return  new;
  } /* getxtip */


xarray *getxnode (p)
    nodeptr  p;
    /* Ensure that internal node p has memory */
  { /* getxnode */
    nodeptr  s;

    if (! (p->x)) {  /*  Move likelihood array on this node to sector p */
      if ((s = p->next)->x || (s = s->next)->x) {
        p->x = s->x;
        s->x = (xarray *) NULL;
        }
      else {
        printf("ERROR: Unable to locate memory at node %d.\n", p->number);
        exit(1);
        }
      }
    return  p->x;
  } /* getxnode */


void newview (p)                      /*  Update likelihoods at node */
    nodeptr  p;
  { /* newview */
    double   z1, lz1, xvlz1, z2, lz2, xvlz2,
             zz1, zv1, fx1r, fx1y, fx1n, suma1, sumg1, sumc1, sumt1,
             zz2, zv2, fx2r, fx2y, fx2n, ki, tempi, tempj;
    nodeptr  q, r;
    xtype   *x1a, *x1c, *x1g, *x1t, *x2a, *x2c, *x2g, *x2t,
            *x3a, *x3c, *x3g, *x3t;
    int  i;

    if (p->tip) {             /*  Make sure that data are at tip */
      int code;
      char *yptr;

      if (p->x) return;       /*  They are already there */

      (void) getxtip(p);      /*  They are not, so get memory */
      x3a = &(p->x->a[0]);    /*  Move tip data to xarray */
      x3c = &(p->x->c[0]);
      x3g = &(p->x->g[0]);
      x3t = &(p->x->t[0]);
      yptr = p->tip;
      for (i = 0; i < endsite; i++) {
        code = *yptr++;
        *x3a++ =  code       & 1;
        *x3c++ = (code >> 1) & 1;
        *x3g++ = (code >> 2) & 1;
        *x3t++ = (code >> 3) & 1;
        }
      return;
      }

/*  Internal node needs update */

    q = p->next->back;
    r = p->next->next->back;

    while ((! p->x) || (! q->x) || (! r->x)) {
      if (! q->x) newview(q);
      if (! r->x) newview(r);
      if (! p->x) (void) getxnode(p);
      }

    x1a = &(q->x->a[0]);
    x1c = &(q->x->c[0]);
    x1g = &(q->x->g[0]);
    x1t = &(q->x->t[0]);
    z1 = q->z;
    lz1 = (z1 > zmin) ? log(z1) : log(zmin);
    xvlz1 = xv * lz1;

    x2a = &(r->x->a[0]);
    x2c = &(r->x->c[0]);
    x2g = &(r->x->g[0]);
    x2t = &(r->x->t[0]);
    z2 = r->z;
    lz2 = (z2 > zmin) ? log(z2) : log(zmin);
    xvlz2 = xv * lz2;

    x3a = &(p->x->a[0]);
    x3c = &(p->x->c[0]);
    x3g = &(p->x->g[0]);
    x3t = &(p->x->t[0]);

    { double  zz1table[maxcategories+1], zv1table[maxcategories+1],
             zz2table[maxcategories+1], zv2table[maxcategories+1],
             *zz1ptr, *zv1ptr, *zz2ptr, *zv2ptr, *rptr;
      int  cat, *cptr;

      rptr   = &(rate[1]);
      zz1ptr = &(zz1table[1]);
      zv1ptr = &(zv1table[1]);
      zz2ptr = &(zz2table[1]);
      zv2ptr = &(zv2table[1]);
      for (i = 1; i <= categs; i++) {   /* exps for each category */
        ki = *rptr++;
        *zz1ptr++ = exp(ki *   lz1);
        *zv1ptr++ = exp(ki * xvlz1);
        *zz2ptr++ = exp(ki *   lz2);
        *zv2ptr++ = exp(ki * xvlz2);
        }

      cptr = &(catnumb[0]);
      for (i = 0; i < endsite; i++) {
        cat = *cptr++;

        zz1 = zz1table[cat];
        zv1 = zv1table[cat];
        fx1r = freqa * *x1a + freqg * *x1g;
        fx1y = freqc * *x1c + freqt * *x1t;
        fx1n = fx1r + fx1y;
        tempi = fx1r * invfreqr;
        tempj = zv1 * (tempi-fx1n) + fx1n;
        suma1 = zz1 * (*x1a++ - tempi) + tempj;
        sumg1 = zz1 * (*x1g++ - tempi) + tempj;
        tempi = fx1y * invfreqy;
        tempj = zv1 * (tempi-fx1n) + fx1n;
        sumc1 = zz1 * (*x1c++ - tempi) + tempj;
        sumt1 = zz1 * (*x1t++ - tempi) + tempj;

        zz2 = zz2table[cat];
        zv2 = zv2table[cat];
        fx2r = freqa * *x2a + freqg * *x2g;
        fx2y = freqc * *x2c + freqt * *x2t;
        fx2n = fx2r + fx2y;
        tempi = fx2r * invfreqr;
        tempj = zv2 * (tempi-fx2n) + fx2n;
        *x3a++ = suma1 * (zz2 * (*x2a++ - tempi) + tempj);
        *x3g++ = sumg1 * (zz2 * (*x2g++ - tempi) + tempj);
        tempi = fx2y * invfreqy;
        tempj = zv2 * (tempi-fx2n) + fx2n;
        *x3c++ = sumc1 * (zz2 * (*x2c++ - tempi) + tempj);
        *x3t++ = sumt1 * (zz2 * (*x2t++ - tempi) + tempj);
        }
      }
  } /* newview */


double evaluate (tr, p)
    tree  *tr;
    nodeptr  p;
  { /* evaluate */
    double   sum, z, lz, xvlz,
             ki, zz, zv, fx1a, fx1c, fx1g, fx1t, fx1r, fx1y, fx2r, fx2y,
             suma, sumb, sumc, term;
    double   zztable[maxcategories+1], zvtable[maxcategories+1],
            *zzptr, *zvptr;
    double  *log_f, *rptr;
    xtype   *x1a, *x1c, *x1g, *x1t, *x2a, *x2c, *x2g, *x2t;
    nodeptr  q;
    int  cat, *cptr, i, *wptr;

    q = p->back;
    while ((! p->x) || (! q->x)) {
      if (! (p->x)) newview(p);
      if (! (q->x)) newview(q);
      }

    x1a = &(p->x->a[0]);
    x1c = &(p->x->c[0]);
    x1g = &(p->x->g[0]);
    x1t = &(p->x->t[0]);

    x2a = &(q->x->a[0]);
    x2c = &(q->x->c[0]);
    x2g = &(q->x->g[0]);
    x2t = &(q->x->t[0]);

    z = p->z;
    if (z < zmin) z = zmin;
    lz = log(z);
    xvlz = xv * lz;

    rptr  = &(rate[1]);
    zzptr = &(zztable[1]);
    zvptr = &(zvtable[1]);
    for (i = 1; i <= categs; i++) {
      ki = *rptr++;
      *zzptr++ = exp(ki *   lz);
      *zvptr++ = exp(ki * xvlz);
      }

    wptr = &(aliasweight[0]);
    cptr = &(catnumb[0]);
    log_f = tr->log_f;
    tr->log_f_valid = TRUE;
    sum = 0.0;

    for (i = 0; i < endsite; i++) {
      cat  = *cptr++;
      zz   = zztable[cat];
      zv   = zvtable[cat];
      fx1a = freqa * *x1a++;
      fx1g = freqg * *x1g++;
      fx1c = freqc * *x1c++;
      fx1t = freqt * *x1t++;
      suma = fx1a * *x2a + fx1c * *x2c + fx1g * *x2g + fx1t * *x2t;
      fx2r = freqa * *x2a++ + freqg * *x2g++;
      fx2y = freqc * *x2c++ + freqt * *x2t++;
      fx1r = fx1a + fx1g;
      fx1y = fx1c + fx1t;
      sumc = (fx1r + fx1y) * (fx2r + fx2y);
      sumb = fx1r * fx2r * invfreqr + fx1y * fx2y * invfreqy;
      suma -= sumb;
      sumb -= sumc;
      term = log(zz * suma + zv * sumb + sumc);
      sum += *wptr++ * term;
      *log_f++ = term;
      }

    tr->likelihood = sum;
    return  sum;
  } /* evaluate */


#if 0                           /*  This is not currently used */
double evaluateslope (p, q, z)  /*  d(L)/d(lz) */
    nodeptr  p, q;
    double  z;
  { /* evaluateslope */
    double   dLdlz, lz, xvlz;
    double   sumai[maxpatterns], sumbi[maxpatterns], sumci[maxpatterns],
            *sumaptr, *sumbptr, *sumcptr;
    double   zztable[maxcategories+1], zvtable[maxcategories+1],
            *zzptr, *zvptr;
    double   ki, fx1a, fx1c, fx1g, fx1t, fx1r, fx1y, fx2r, fx2y,
             suma, sumb, sumc;
    double  *rptr, *wrptr;
    xtype   *x1a, *x1c, *x1g, *x1t, *x2a, *x2c, *x2g, *x2t;
    int  cat, *cptr, i;


    while ((! p->x) || (! q->x)) {
      if (! p->x) newview(p);
      if (! q->x) newview(q);
      }

    x1a = &(p->x->a[0]);
    x1c = &(p->x->c[0]);
    x1g = &(p->x->g[0]);
    x1t = &(p->x->t[0]);

    x2a = &(q->x->a[0]);
    x2c = &(q->x->c[0]);
    x2g = &(q->x->g[0]);
    x2t = &(q->x->t[0]);

    sumaptr = &(sumai[0]);
    sumbptr = &(sumbi[0]);
    sumcptr = &(sumci[0]);

    for (i = 0; i < endsite; i++) {
      fx1a = freqa * *x1a++;
      fx1g = freqg * *x1g++;
      fx1c = freqc * *x1c++;
      fx1t = freqt * *x1t++;
      suma   = fx1a * *x2a + fx1c * *x2c + fx1g * *x2g + fx1t * *x2t;
      fx2r = freqa * *x2a++ + freqg * *x2g++;
      fx2y = freqc * *x2c++ + freqt * *x2t++;
      fx1r = fx1a + fx1g;
      fx1y = fx1c + fx1t;
      *sumcptr++ = sumc = (fx1r + fx1y) * (fx2r + fx2y);
      sumb       = fx1r * fx2r * invfreqr + fx1y * fx2y * invfreqy;
      *sumaptr++ = suma - sumb;
      *sumbptr++ = sumb - sumc;
      }

    if (z < zmin) z = zmin;
    lz = log(z);
    xvlz = xv * lz;

    rptr  = &(rate[1]);
    zzptr = &(zztable[1]);
    zvptr = &(zvtable[1]);
    for (i = 1; i <= categs; i++) {
      ki = *rptr++;
      *zzptr++ = exp(ki *   lz);
      *zvptr++ = exp(ki * xvlz);
      }

    sumaptr = &(sumai[0]);
    sumbptr = &(sumbi[0]);
    sumcptr = &(sumci[0]);
    cptr  = &(catnumb[0]);
    wrptr = &(wgt_rate[0]);
    dLdlz = 0.0;

    for (i = 0; i < endsite; i++) {
      cat    = *cptr++;
      suma   = *sumaptr++ * zztable[cat];
      sumb   = *sumbptr++ * zvtable[cat];
      dLdlz += *wrptr++ * (suma + sumb*xv) / (suma + sumb + *sumcptr++);
      }

    return  dLdlz;
  } /* evaluateslope */
#endif


double makenewz (p, q, z0, maxiter)
    nodeptr  p, q;
    double  z0;
    int  maxiter;
  { /* makenewz */
    double   abi[maxpatterns], bci[maxpatterns], sumci[maxpatterns],
            *abptr, *bcptr, *sumcptr;
    double   dlnLidlz, dlnLdlz, d2lnLdlz2, z, zprev, zstep, lz, xvlz,
             ki, suma, sumb, sumc, ab, bc, inv_Li, t1, t2,
             fx1a, fx1c, fx1g, fx1t, fx1r, fx1y, fx2r, fx2y;
    double   zztable[maxcategories+1], zvtable[maxcategories+1],
            *zzptr, *zvptr;
    double  *rptr, *wrptr, *wr2ptr;
    xtype   *x1a, *x1c, *x1g, *x1t, *x2a, *x2c, *x2g, *x2t;
    int    cat, *cptr, i, curvatOK;


    while ((! p->x) || (! q->x)) {
      if (! (p->x)) newview(p);
      if (! (q->x)) newview(q);
      }

    x1a = &(p->x->a[0]);
    x1c = &(p->x->c[0]);
    x1g = &(p->x->g[0]);
    x1t = &(p->x->t[0]);
    x2a = &(q->x->a[0]);
    x2c = &(q->x->c[0]);
    x2g = &(q->x->g[0]);
    x2t = &(q->x->t[0]);

    abptr = &(abi[0]);
    bcptr = &(bci[0]);
    sumcptr = &(sumci[0]);

    for (i = 0; i < endsite; i++) {
      fx1a = freqa * *x1a++;
      fx1g = freqg * *x1g++;
      fx1c = freqc * *x1c++;
      fx1t = freqt * *x1t++;
      suma = fx1a * *x2a + fx1c * *x2c + fx1g * *x2g + fx1t * *x2t;
      fx2r = freqa * *x2a++ + freqg * *x2g++;
      fx2y = freqc * *x2c++ + freqt * *x2t++;
      fx1r = fx1a + fx1g;
      fx1y = fx1c + fx1t;
      *sumcptr++ = sumc = (fx1r + fx1y) * (fx2r + fx2y);
      sumb       = fx1r * fx2r * invfreqr + fx1y * fx2y * invfreqy;
      *abptr++   = suma - sumb;
      *bcptr++   = sumb - sumc;
      }

    z = z0;
    do {
      zprev = z;
      zstep = (1.0 - zmax) * z + zmin;
      curvatOK = FALSE;

      do {
        if (z < zmin) z = zmin;
        else if (z > zmax) z = zmax;

        lz    = log(z);
        xvlz  = xv * lz;
        rptr  = &(rate[1]);
        zzptr = &(zztable[1]);
        zvptr = &(zvtable[1]);

        for (i = 1; i <= categs; i++) {
          ki = *rptr++;
          *zzptr++ = exp(ki *   lz);
          *zvptr++ = exp(ki * xvlz);
          }

        abptr   = &(abi[0]);
        bcptr   = &(bci[0]);
        sumcptr = &(sumci[0]);
        cptr    = &(catnumb[0]);
        wrptr   = &(wgt_rate[0]);
        wr2ptr  = &(wgt_rate2[0]);
        dlnLdlz = 0.0;                 /*  = d(ln(likelihood))/d(lz) */
        d2lnLdlz2 = 0.0;               /*  = d2(ln(likelihood))/d(lz)2 */

        for (i = 0; i < endsite; i++) {
          cat    = *cptr++;              /*  ratecategory(i) */
          ab     = *abptr++ * zztable[cat];
          bc     = *bcptr++ * zvtable[cat];
          sumc   = *sumcptr++;
          inv_Li = 1.0/(ab + bc + sumc);
          t1     = ab * inv_Li;
          t2     = xv * bc * inv_Li;
          dlnLidlz   = t1 + t2;
          dlnLdlz   += *wrptr++  * dlnLidlz;
          d2lnLdlz2 += *wr2ptr++ * (t1 + xv * t2 - dlnLidlz * dlnLidlz);
          }

        if ((d2lnLdlz2 >= 0.0) && (z < zmax))
          zprev = z = 0.37 * z + 0.63;  /*  Bad curvature, shorten branch */
        else
          curvatOK = TRUE;

        } while (! curvatOK);

      if (d2lnLdlz2 < 0.0) {
        z *= exp(-dlnLdlz / d2lnLdlz2);
        if (z < zmin) z = zmin;
        if (z > 0.25 * zprev + 0.75)    /*  Limit steps toward z = 1.0 */
          z = 0.25 * zprev + 0.75;
        }

      if (z > zmax) z = zmax;

      } while ((--maxiter > 0) && (ABS(z - zprev) > zstep));

    return  z;
  } /* makenewz */


void update (tr, p)
    tree    *tr;
    nodeptr  p;
  { /* update */
    nodeptr  q;
    double   z0, z;

    q = p->back;
    z0 = q->z;
    p->z = q->z = z = makenewz(p, q, z0, newzpercycle);
    if (ABS(z - z0) > deltaz)  tr->smoothed = FALSE;
  } /* update */


void smooth (tr, p)
    tree    *tr;
    nodeptr  p;
  { /* smooth */
    update(tr, p);                       /*  Adjust branch */
    if (! p->tip) {                      /*  Adjust "descendents" */
      smooth(tr, p->next->back);
      smooth(tr, p->next->next->back);

#     if ReturnSmoothedView
        newview(p);
#     endif
      }
  } /* smooth */


void smoothTree (tr, maxtimes)
    tree    *tr;
    int  maxtimes;
  { /* smoothTree */
    nodeptr  p;

    p = tr->start;

    while (--maxtimes >= 0 && ! anerror) {
      tr->smoothed = TRUE;
      smooth(tr, p->back);
      if (! p->tip) {
        smooth(tr, p->next->back);
        smooth(tr, p->next->next->back);
        }
      if (tr->smoothed)  break;
      }
  } /* smoothTree */


void localSmooth (tr, p, maxtimes)    /* Smooth branches around p */
    tree    *tr;
    nodeptr  p;
    int  maxtimes;
  { /* localSmooth */
    nodeptr  pn, pnn;

    if (p->tip) return;               /* Should actually be an error */

    pn  = p->next;
    pnn = pn->next;
    while (--maxtimes >= 0) {
      tr->smoothed = TRUE;
      update(tr, p);     if (anerror) break;
      update(tr, pn);    if (anerror) break;
      update(tr, pnn);   if (anerror) break;
      if (tr->smoothed)  break;
      }
    tr->smoothed = FALSE;             /* Only smooth locally */
  } /* localSmooth */


void hookup (p, q, z)
    nodeptr  p, q;
    double   z;
  { /* hookup */
    p->back = q;
    q->back = p;
    p->z = q->z = z;
  } /* hookup */


void insert (tr, p, q, glob)   /* Insert node p into branch q <-> q->back */
    tree    *tr;
    nodeptr  p, q;
    boolean  glob;             /* Smooth tree globally? */

/*                q
                 /.
             add/ .
               /  .
             pn   .
    s ---- p      .remove
             pnn  .
               \  .
             add\ .
                 \.      pn  = p->next;
                  r      pnn = p->next->next;
 */

  { /* insert */
    nodeptr  r, s;

    r = q->back;
    s = p->back;

#   if BestInsertAverage && ! Master
    { double  zqr, zqs, zrs, lzqr, lzqs, lzrs, lzsum, lzq, lzr, lzs, lzmax;

      zqr = makenewz(q, r, q->z,     iterations);
      zqs = makenewz(q, s, defaultz, iterations);
      zrs = makenewz(r, s, defaultz, iterations);

      lzqr = (zqr > zmin) ? log(zqr) : log(zmin);  /* long branches */
      lzqs = (zqs > zmin) ? log(zqs) : log(zmin);
      lzrs = (zrs > zmin) ? log(zrs) : log(zmin);
      lzsum = 0.5 * (lzqr + lzqs + lzrs);
      lzq = lzsum - lzrs;
      lzr = lzsum - lzqs;
      lzs = lzsum - lzqr;
      lzmax = log(zmax);
      if      (lzq > lzmax) {lzq = lzmax; lzr = lzqr; lzs = lzqs;} /* short */
      else if (lzr > lzmax) {lzr = lzmax; lzq = lzqr; lzs = lzrs;}
      else if (lzs > lzmax) {lzs = lzmax; lzq = lzqs; lzr = lzrs;}

      hookup(p->next,       q, exp(lzq));
      hookup(p->next->next, r, exp(lzr));
      hookup(p,             s, exp(lzs));
      }

#   else
    { double  z;
      z = sqrt(q->z);
      hookup(p->next,       q, z);
      hookup(p->next->next, r, z);
      }

#   endif

    newview(p);         /*  Required so that sector p is valid at update */
    tr->opt_level = 0;

#   if ! Master         /*  Smoothings are done by slave */
      if (! glob)  localSmooth(tr, p, smoothings);  /* Smooth locale of p */
      if (glob)    smoothTree(tr, smoothings);      /* Smooth whole tree */

#   else
      tr->likelihood = unlikely;
#   endif

  } /* insert */


nodeptr  removeNode (tr, p)
    tree    *tr;
    nodeptr  p;

/*                q
                 .|
          remove. |
               .  |
             pn   |
    s ---- p      |add
             pnn  |
               .  |
          remove. |
                 .|      pn  = p->next;
                  r      pnn = p->next->next;
 */

    /* remove p and return where it was */
  { /* removeNode */
    double   zqr;
    nodeptr  q, r;

    q = p->next->back;
    r = p->next->next->back;
    zqr = q->z * r->z;
#   if ! Master
      hookup(q, r, makenewz(q, r, zqr, iterations));
#   else
      hookup(q, r, zqr);
#   endif

    p->next->next->back = p->next->back = (node *) NULL;
    return  q;
  } /* removeNode */


void initrav (p)
    nodeptr  p;
  { /* initrav */
    if (! p->tip) {
      initrav(p->next->back);
      initrav(p->next->next->back);
      newview(p);
      }
  } /* initrav */


nodeptr buildNewTip (tr, p)
    tree  *tr;
    nodeptr  p;
  { /* buildNewTip */
    nodeptr  q;

    q = tr->nodep[(tr->nextnode)++];
    hookup(p, q, defaultz);
    return  q;
  } /* buildNewTip */


void buildSimpleTree (tr, ip, iq, ir)
    tree  *tr;
    int    ip, iq, ir;
  { /* buildSimpleTree */
    /* p, q and r are tips meeting at s */
    nodeptr  p, s;
    int  i;

    i = MIN(ip, iq);
    if (ir < i)  i = ir; 
    tr->start = tr->nodep[i];
    tr->ntips = 3;
    p = tr->nodep[ip];
    hookup(p, tr->nodep[iq], defaultz);
    s = buildNewTip(tr, tr->nodep[ir]);
    insert(tr, s, p, FALSE);            /* Smoothing is local to s */
  } /* buildSimpleTree */


boolean readKeyValue (string, key, format, value)
    char *string, *key, *format;
    void *value;
  { /* readKeyValue */

    if (! (string = strstr(string, key)))  return FALSE;
    string += strlen(key);
    if (! (string = index(string, '=')))  return FALSE;
    string++;
    return  sscanf(string, format, value);  /* 1 if read, otherwise 0 */
  } /* readKeyValue */


#if Master || Slave

double str_readTreeLikelihood (treestr)
    char *treestr;
  { /* str_readTreeLikelihood */
    double lk1;
    char    *com, *com_end;
    boolean  readKeyValue();

    if ((com = index(treestr, '[')) && (com < index(treestr, '('))
                                    && (com_end = index(com, ']'))) {
      com++;
      *com_end = 0;
      if (readKeyValue(com, likelihood_key, "%lg", (void *) &(lk1))) {
        *com_end = ']';
        return lk1;
        }
      }

    fprintf(stderr, "ERROR reading likelihood in receiveTree\n");
    anerror = TRUE;
    return 1.0;
  } /* str_readTreeLikelihood */


void sendTree (comm, tr)
    comm_block *comm;
    tree *tr;
  { /* sendTree */
    char treestr[maxsp*(nmlngth+24)+100], *p1;
    char *treeString();
#   if Master
      void sendTreeNum();
#   endif

    comm->done_flag = tr->likelihood > 0.0;
    if (comm->done_flag)
      write_comm_msg(comm, NULL);

    else {
#     if Master
        if (send_ahead >= MAX_SEND_AHEAD) {
          int  n_to_get;

          n_to_get = (send_ahead+1)/2;
          sendTreeNum(n_to_get);
          send_ahead -= n_to_get;
          read_comm_msg(&comm_slave, treestr);
          new_likelihood = str_readTreeLikelihood(treestr);
          if (! best_tr_recv || (new_likelihood > best_lk_recv)) {
            if (best_tr_recv)  free(best_tr_recv);
            best_tr_recv = malloc((unsigned) (strlen(treestr) + 1));
            strcpy(best_tr_recv, treestr);
            best_lk_recv = new_likelihood;
            }
          }
        send_ahead++;
#     endif           /*  End #if Master  */

      REPORT_SEND_TREE;
      (void) sprintf(treestr, "[%16.14g] ", tr->likelihood);
      p1 = treestr + strlen(treestr);
      (void) treeString(p1, tr, tr->start->back, 1);
      write_comm_msg(comm, treestr);
      }
  } /* sendTree */


void  receiveTree (comm, tr)
    comm_block  *comm;
    tree        *tr;
  { /* receiveTree */
    char treestr[maxsp*(nmlngth+24)+100], *p1;
    void str_treeReadLen();

    read_comm_msg(comm, treestr);
    if (comm->done_flag)
      tr->likelihood = 1.0;

    else {
#     if Master
        if (best_tr_recv) {
          if (str_readTreeLikelihood(treestr) < best_lk_recv) {
            strcpy(treestr, best_tr_recv);  /* Overwrite new tree with best */
            }
          free(best_tr_recv);
          best_tr_recv = NULL;
          }
#     endif           /*  End #if Master  */

      p1 = treestr;
      if (str_findch(&p1, '[') != '['
          || sscanf(p1, "%lg", &(tr->likelihood)) != 1
          || (p1 = index(p1, ']')) == NULL) {
        fprintf(stderr, "ERROR reading likelihood in receiveTree\n");
        anerror = TRUE;
        return;
        }
      p1++;                     /* skip ']' */
      str_treeReadLen(p1, tr);
      }
  } /* receiveTree */


void requestForWork ()
  { /* requestForWork */
    p4_send(DNAML_REQUEST, DNAML_DISPATCHER_ID, NULL, 0);
  } /* requestForWork */
#endif                  /* End #if Master || Slave  */


#if Master
void sendTreeNum(n_to_get)
    int n_to_get;
  { /* sendTreeNum */
    char scr[512];

    sprintf(scr, "%d", n_to_get);
    p4_send(DNAML_NUM_TREE, DNAML_MERGER_ID, scr, strlen(scr)+1);
  } /* sendTreeNum */


void  getReturnedTrees (tr, bt, n_tree_sent)
    tree     *tr;
    bestlist *bt;
    int n_tree_sent; /* number of trees sent to slaves */
  { /* getReturnedTrees */
    void sendTreeNum(), receiveTree();

    sendTreeNum(send_ahead);
    send_ahead = 0;

    receiveTree(&comm_slave, tr);
    tr->smoothed = TRUE;
    (void) saveBestTree(bt, tr);
  } /* getReturnedTrees */
#endif


void cacheZ (tr)
    tree  *tr;
  { /* cacheZ */
    nodeptr  p;
    int  nodes;

    nodes = tr->mxtips  +  3 * (tr->mxtips - 2);
    p = tr->nodep[1];
    while (nodes-- > 0) {p->z0 = p->z; p++;}
  } /* cacheZ */


void restoreZ (tr)
    tree  *tr;
  { /* restoreZ */
    nodeptr  p;
    int  nodes;

    nodes = tr->mxtips  +  3 * (tr->mxtips - 2);
    p = tr->nodep[1];
    while (nodes-- > 0) {p->z = p->z0; p++;}
  } /* restoreZ */


testInsert (tr, p, q, bt, fast)
    tree     *tr;
    nodeptr   p, q;
    bestlist *bt;
    boolean   fast;
  { /* testInsert */
    double  qz;
    nodeptr  r;

    r = q->back;             /* Save original connection */
    qz = q->z;
    insert(tr, p, q, ! fast);

#   if ! Master
      (void) evaluate(tr, fast ? p->next->next : tr->start);
      (void) saveBestTree(bt, tr);
#   else  /* Master */
      tr->likelihood = unlikely;
      sendTree(&comm_slave, tr);
#   endif

    /* remove p from this branch */

    hookup(q, r, qz);
    p->next->next->back = p->next->back = (nodeptr) NULL;
    if (! fast) {            /* With fast add, other values are still OK */
      restoreZ(tr);          /*   Restore branch lengths */
#     if ! Master            /*   Regenerate x values */
        initrav(p->back);
        initrav(q);
        initrav(r);
#     endif
      }
  } /* testInsert */


int addTraverse (tr, p, q, mintrav, maxtrav, bt, fast)
    tree     *tr;
    nodeptr   p, q;
    int       mintrav, maxtrav;
    bestlist *bt;
    boolean   fast;
  { /* addTraverse */
    int  tested;

    tested = 0;
    if (--mintrav <= 0) {           /* Moved minimum distance? */
      testInsert(tr, p, q, bt, fast);
      tested++;
      }

    if ((! q->tip) && (--maxtrav > 0)) {    /* Continue traverse? */
      tested += addTraverse(tr, p, q->next->back,
                            mintrav, maxtrav, bt, fast);
      tested += addTraverse(tr, p, q->next->next->back,
                            mintrav, maxtrav, bt, fast);
      }

    return tested;
  } /* addTraverse */


int  rearrange (tr, p, mintrav, maxtrav, bt)
    tree     *tr;
    nodeptr   p;
    int       mintrav, maxtrav;
    bestlist *bt;
    /* rearranges the tree, globally or locally */
  { /* rearrange */
    double   p1z, p2z, q1z, q2z;
    nodeptr  p1, p2, q, q1, q2;
    int      tested, mintrav2;

    tested = 0;
    if (maxtrav < 1 || mintrav > maxtrav)  return tested;

/* Moving subtree forward in tree. */

    if (! p->tip) {
      p1 = p->next->back;
      p2 = p->next->next->back;
      if (! p1->tip || ! p2->tip) {
        p1z = p1->z;
        p2z = p2->z;
        (void) removeNode(tr, p);
        cacheZ(tr);
        if (! p1->tip) {
          tested += addTraverse(tr, p, p1->next->back,
                                mintrav, maxtrav, bt, FALSE);
          tested += addTraverse(tr, p, p1->next->next->back,
                                mintrav, maxtrav, bt, FALSE);
          }

        if (! p2->tip) {
          tested += addTraverse(tr, p, p2->next->back,
                                mintrav, maxtrav, bt, FALSE);
          tested += addTraverse(tr, p, p2->next->next->back,
                                mintrav, maxtrav, bt, FALSE);
          }

        hookup(p->next,       p1, p1z);  /*  Restore original tree */
        hookup(p->next->next, p2, p2z);
        initrav(tr->start);
        initrav(tr->start->back);
        }
      }   /* if (! p->tip) */

/* Moving subtree backward in tree.  Minimum move is 2 to avoid duplicates */

    q = p->back;
    if (! q->tip && maxtrav > 1) {
      q1 = q->next->back;
      q2 = q->next->next->back;
      if (! q1->tip && (!q1->next->back->tip || !q1->next->next->back->tip) ||
          ! q2->tip && (!q2->next->back->tip || !q2->next->next->back->tip)) {
        q1z = q1->z;
        q2z = q2->z;
        (void) removeNode(tr, q);
        cacheZ(tr);
        mintrav2 = mintrav > 2 ? mintrav : 2;

        if (! q1->tip) {
          tested += addTraverse(tr, q, q1->next->back,
                                mintrav2 , maxtrav, bt, FALSE);
          tested += addTraverse(tr, q, q1->next->next->back,
                                mintrav2 , maxtrav, bt, FALSE);
          }

        if (! q2->tip) {
          tested += addTraverse(tr, q, q2->next->back,
                                mintrav2 , maxtrav, bt, FALSE);
          tested += addTraverse(tr, q, q2->next->next->back,
                                mintrav2 , maxtrav, bt, FALSE);
          }

        hookup(q->next,       q1, q1z);  /*  Restore original tree */
        hookup(q->next->next, q2, q2z);
        initrav(tr->start);
        initrav(tr->start->back);
        }
      }   /* if (! q->tip && maxtrav > 1) */

/* Move other subtrees */

    if (! p->tip) {
      tested += rearrange(tr, p->next->back,       mintrav, maxtrav, bt);
      tested += rearrange(tr, p->next->next->back, mintrav, maxtrav, bt);
      }

    return  tested;
  } /* rearrange */


FILE *fopen_pid (filenm, mode)
    char *filenm, *mode;
  { /* fopen_pid */
    char scr[512];

    (void) sprintf(scr, "%s.%d", filenm, getpid());
    return  fopen(scr, mode);
  } /* fopen_pid */


#if DeleteCheckpointFile
void  unlink_pid (filenm)
    char *filenm;
  { /* unlink_pid */
    char scr[512];

    (void) sprintf(scr, "%s.%d", filenm, getpid());
    unlink(scr);
  } /* unlink_pid */
#endif


void  writeCheckpoint (tr)
    tree  *tr;
  { /* writeCheckpoint */
    FILE  *checkpointf;
    void   treeOut();

    checkpointf = fopen_pid(checkpointname,"a");
    if (checkpointf) {
      treeOut(checkpointf, tr, 1);  /* 1 is for Newick format */
      (void) fclose(checkpointf);
      }
  } /* writeCheckpoint */


node * findAnyTip(p)
    nodeptr  p;
  { /* findAnyTip */
    return  p->tip ? p : findAnyTip(p->next->back);
  } /* findAnyTip */


void  optimize (tr, maxtrav, bt)
    tree     *tr;
    int       maxtrav;
    bestlist *bt;
  { /* optimize */
    nodeptr  p;
    int    mintrav, tested;

    if (tr->ntips < 4)  return;

    writeCheckpoint(tr);                    /* checkpoint the starting tree */

    if (maxtrav > tr->ntips - 3)  maxtrav = tr->ntips - 3;
    if (maxtrav <= tr->opt_level)  return;

    printf("      Doing %s rearrangements\n",
             (maxtrav == 1)            ? "local" :
             (maxtrav < tr->ntips - 3) ? "regional" : "global");

    /* loop while tree gets better  */

    do {
      (void) startOpt(bt, tr);
      mintrav = tr->opt_level + 1;

      /* rearrange must start from a tip or it will miss some trees */

      p = findAnyTip(tr->start);
      tested = rearrange(tr, p->back, mintrav, maxtrav, bt);

#     if Master
        getReturnedTrees(tr, bt, tested);
#     endif

      if (anerror)  return;
      bt->numtrees += tested;
      (void) setOptLevel(bt, maxtrav);
      (void) recallBestTree(bt, 1, tr);     /* recover best found tree */

      printf("      Tested %d alternative trees\n", tested);
      if (bt->improved) {
        printf("      Ln Likelihood =%14.5f\n", tr->likelihood);
        }

      writeCheckpoint(tr);                  /* checkpoint the new tree */
      } while (maxtrav > tr->opt_level);

  } /* optimize */


void coordinates (tr, p, lengthsum, tdptr)
    tree     *tr;
    nodeptr   p;
    double    lengthsum;
    drawdata *tdptr;
  { /* coordinates */
    /* establishes coordinates of nodes */
    double  x, z;
    nodeptr  q, first, last;

    if (p->tip) {
      p->xcoord = nint(over * lengthsum);
      p->ymax = p->ymin = p->ycoord = tdptr->tipy;
      tdptr->tipy += down;
      if (lengthsum > tdptr->tipmax) tdptr->tipmax = lengthsum;
      }

    else {
      q = p->next;
      do {
        z = q->z;
        if (z < zmin) z = zmin;
        x = lengthsum - fracchange * log(z);
        coordinates(tr, q->back, x, tdptr);
        q = q->next;
        } while (p == tr->start->back ? q != p->next : q != p);

      first = p->next->back;
      q = p;
      while (q->next != p) q = q->next;
      last = q->back;
      p->xcoord = nint(over * lengthsum);
      p->ycoord = (first->ycoord + last->ycoord)/2;
      p->ymin = first->ymin;
      p->ymax = last->ymax;
      }
  } /* coordinates */


void copyTrimmedName (cp1, cp2)
    char  *cp1, *cp2;
 { /* copyTrimmedName */
   char *ep;

   ep = cp1;
   while (*ep)  ep++;                              /* move forward to end */
   ep--;                                           /* move back to last */
   while (ep >= cp1 && white((int) *(ep)))  ep--;  /* trim white */
   while (cp1 <= ep)  *cp2++ = *cp1++;             /* copy to new end */
   *cp2 = 0;
 } /* copyTrimmedName */


void drawline (tr, i, scale)
    tree   *tr;
    int     i;
    double  scale;
    /* draws one row of the tree diagram by moving up tree */
    /* Modified to handle 1000 taxa, October 16, 1991 */
  { /* drawline */
    nodeptr  p, q, r, first, last;
    char    *nameptr;
    int  n, j, k, l, extra;
    boolean  done;

    p = q = tr->start->back;
    extra = 0;

    if (i == p->ycoord) {
      k = q->number - tr->mxtips;
      for (j = k; j < 1000; j *= 10) putchar('-');
      printf("%d", k);
      extra = 1;
      }
    else printf("   ");

    do {
      if (! p->tip) {
        r = p->next;
        done = FALSE;
        do {
          if ((i >= r->back->ymin) && (i <= r->back->ymax)) {
            q = r->back;
            done = TRUE;
            }
          r = r->next;
          } while (! done && (p == tr->start->back ? r != p->next : r != p));

        first = p->next->back;
        r = p;
        while (r->next != p) r = r->next;
        last = r->back;
        if (p == tr->start->back) last = p->back;
        }

      done = (p->tip) || (p == q);
      n = nint(scale*(q->xcoord - p->xcoord));
      if ((n < 3) && (! q->tip)) n = 3;
      n -= extra;
      extra = 0;

      if ((q->ycoord == i) && (! done)) {
        if (p->ycoord != q->ycoord) putchar('+');
        else                        putchar('-');

        if (! q->tip) {
          k = q->number - tr->mxtips;
          l = n - 3;
          for (j = k; j < 100; j *= 10)  l++;
          for (j = 1; j <= l; j++) putchar('-');
          printf("%d", k);
          extra = 1;
          }
        else for (j = 1; j <= n-1; j++) putchar('-');
        }

      else if (! p->tip) {
        if ((last->ycoord > i) && (first->ycoord < i) && (i != p->ycoord)) {
          putchar('!');
          for (j = 1; j <= n-1; j++) putchar(' ');
          }
        else for (j = 1; j <= n; j++) putchar(' ');
        }

      else
        for (j = 1; j <= n; j++) putchar(' ');

      p = q;
      } while (! done);

    if ((p->ycoord == i) && p->tip) {
      char  trimmed[nmlngth + 1];

      copyTrimmedName(p->name, trimmed);
      printf(" %s", trimmed);
      }

    putchar('\n');
  } /* drawline */


void printTree (tr)
    tree  *tr;
    /* prints out diagram of the tree */
  { /* printTree */
    drawdata  tipdata;
    double  scale;
    int  i, imax;

    if (treeprint) {
      putchar('\n');
      tipdata.tipy = 1;
      tipdata.tipmax = 0.0;
      coordinates(tr, tr->start->back, (double) 0.0, & tipdata);
      scale = 1.0 / tipdata.tipmax;
      imax = tipdata.tipy - down;
      for (i = 1; i <= imax; i++)  drawline(tr, i, scale);
      printf("\nRemember: ");
      if (outgropt) printf("(although rooted by outgroup) ");
      printf("this is an unrooted tree!\n\n");
      }
  } /* printTree */


double sigma (p, sumlrptr)
    nodeptr  p;
    double  *sumlrptr;
    /* compute standard deviation */
  { /* sigma */
    double  slope, sum, sumlr, z, zv, zz, lz,
            rat, suma, sumb, sumc, d2, d, li, temp, abzz, bczv, t3,
            fx1a, fx1c, fx1g, fx1t, fx1r, fx1y, fx2r, fx2y, w;
    double  *rptr;
    xtype   *x1a, *x1c, *x1g, *x1t, *x2a, *x2c, *x2g, *x2t;
    nodeptr  q;
    int  i, *wptr;

    q = p->back;
    while ((! p->x) || (! q->x)) {
      if (! (p->x)) newview(p);
      if (! (q->x)) newview(q);
      }

    x1a = &(p->x->a[0]);
    x1c = &(p->x->c[0]);
    x1g = &(p->x->g[0]);
    x1t = &(p->x->t[0]);

    x2a = &(q->x->a[0]);
    x2c = &(q->x->c[0]);
    x2g = &(q->x->g[0]);
    x2t = &(q->x->t[0]);

    z = p->z;
    if (z < zmin) z = zmin;
    lz = log(z);

    wptr = &(aliasweight[0]);
    rptr = &(ratvalue[0]);
    sum = sumlr = slope = 0.0;

    for (i = 0; i < endsite; i++) {
      rat  = *rptr++;
      zz   = exp(rat    * lz);
      zv   = exp(rat*xv * lz);

      fx1a = freqa * *x1a++;
      fx1g = freqg * *x1g++;
      fx1c = freqc * *x1c++;
      fx1t = freqt * *x1t++;
      fx1r = fx1a + fx1g;
      fx1y = fx1c + fx1t;
      suma = fx1a * *x2a + fx1c * *x2c + fx1g * *x2g + fx1t * *x2t;
      fx2r = freqa * *x2a++ + freqg * *x2g++;
      fx2y = freqc * *x2c++ + freqt * *x2t++;
      sumc = (fx1r + fx1y) * (fx2r + fx2y);
      sumb = fx1r * fx2r * invfreqr + fx1y * fx2y * invfreqy;
      abzz = zz * (suma - sumb);
      bczv = zv * (sumb - sumc);
      li = sumc + abzz + bczv;
      t3 = xv * bczv;
      d  = abzz + t3;
      d2 = rat * (abzz*(rat-1.0) + t3*(rat*xv-1.0));

      temp = rat * d / li;
      w = *wptr++;
      slope += w *  temp;
      sum   += w * (temp * temp - d2/li);
      sumlr += w * log(li/(suma+1.0E-300));
      }

    *sumlrptr = sumlr;
    return (sum > 1.0E-300) ? z*(-slope + sqrt(slope*slope + 3.841*sum))/sum
                            : 1.0;
  } /* sigma */


void describe (tr, p)
    tree    *tr;
    nodeptr  p;
    /* print out information for one branch */
  { /* describe */
    double  z, s, sumlr;
    nodeptr  q;

    q = p->back;
    printf("%4d          ", q->number - tr->mxtips);
    if (p->tip) printf("%s", p->name);
    else        printf("%4d      ", p->number - tr->mxtips);

    z = q->z;
    if (z <= zmin) printf("    infinity");
    else printf("%15.5f", -log(z)*fracchange);

    s = sigma(q, &sumlr);
    printf("     (");
    if (z + s >= zmax) printf("     zero");
    else printf("%9.5f", (double) -log(z + s)*fracchange);
    putchar(',');
    if (z - s <= zmin) printf("    infinity");
    else printf("%12.5f", (double) -log(z - s)*fracchange);
    putchar(')');

    if      (sumlr > 2.995 ) printf(" **");
    else if (sumlr > 1.9205) printf(" *");
    putchar('\n');

    if (! p->tip) {
      describe(tr, p->next->back);
      describe(tr, p->next->next->back);
      }
  } /* describe */


void summarize (tr)
    tree  *tr;
    /* print out branch length information and node numbers */
  { /* summarize */
    printf("Ln Likelihood =%14.5f\n", tr->likelihood);
    putchar('\n');
    printf(" Between        And             Length");
    printf("      Approx. Confidence Limits\n");
    printf(" -------        ---             ------");
    printf("      ------- ---------- ------\n");

    describe(tr, tr->start->back->next->back);
    describe(tr, tr->start->back->next->next->back);
    describe(tr, tr->start);
    putchar('\n');
    printf("     *  = significantly positive, P < 0.05\n");
    printf("     ** = significantly positive, P < 0.01\n\n\n");
  } /* summarize */


/*===========  This is a problem if tr->start->back is a tip!  ===========*/
/*  All routines should be contrived so that tr->start->back is not a tip */

char *treeString (treestr, tr, p, form)
    char  *treestr;
    tree  *tr;
    nodeptr  p;
    int  form;
    /* write string with representation of tree */
    /* form == 1 -> Newick tree */
    /* form == 2 -> Prolog fact */
  { /* treeString */
    double  x, z;
    char  *nameptr;
    int  n, c;

    if (p == tr->start->back) {
      if (form == 2) {
        (void) sprintf(treestr, "phylip_tree(");
        while (*treestr) treestr++;            /* move pointer to null */
        }

      (void) sprintf(treestr, "[&&%s: version = '%s'",
                               programName, programVersion);
      while (*treestr) treestr++;

      (void) sprintf(treestr, ", %s = %15.13g",
                               likelihood_key, tr->likelihood);
      while (*treestr) treestr++;

      (void) sprintf(treestr, ", %s = %d", ntaxa_key, tr->ntips);
      while (*treestr) treestr++;

      (void) sprintf(treestr,", %s = %d", opt_level_key, tr->opt_level);
      while (*treestr) treestr++;

      (void) sprintf(treestr, ", %s = %d", smoothed_key, tr->smoothed);
      while (*treestr) treestr++;

      (void) sprintf(treestr, "]%s", form == 2 ? ", " : " ");
      while (*treestr) treestr++;
      }

    if (p->tip) {
      *treestr++ = '\'';
      n = nmlngth;
      nameptr = p->name + nmlngth - 1;
      while (*nameptr-- == ' ' && n) n--;    /*  Trim trailing spaces */
      nameptr = p->name;
      while (n--) {
        if ((c = *nameptr++) == '\'')  *treestr++ = '\'';
        *treestr++ = c;
        }
      *treestr++ = '\'';
      }

    else {
      *treestr++ = '(';
      treestr = treeString(treestr, tr, p->next->back, form);
      *treestr++ = ',';
      treestr = treeString(treestr, tr, p->next->next->back, form);
      if (p == tr->start->back) {
        *treestr++ = ',';
        treestr = treeString(treestr, tr, p->back, form);
        }
      *treestr++ = ')';
      }

    if (p == tr->start->back) {
      (void) sprintf(treestr, ":0.0%s\n", (form != 2) ? ";" : ").");
      }
    else {
      z = p->z;
      if (z < zmin) z = zmin;
      x = -log(z) * fracchange;
      (void) sprintf(treestr, ": %8.6f", x);  /* prolog needs the space */
      }

    while (*treestr) treestr++;     /* move pointer up to null termination */
    return  treestr;
  } /* treeString */


void treeOut (treefile, tr, form)
    FILE  *treefile;
    tree  *tr;
    int    form;
    /* write out file with representation of final tree */
  { /* treeOut */
    int    c;
    char  *cptr, treestr[maxsp*(nmlngth+24)];

    (void) treeString(treestr, tr, tr->start->back, form);
    cptr = treestr;
    while (c = *cptr++) putc(c, treefile);
  } /* treeOut */


/*=======================================================================*/
/*                         Read a tree from a file                       */
/*=======================================================================*/


int treeFinishCom ()
  { /* treeFinishCom */
    int      ch;
    boolean  inquote;

    inquote = FALSE;
    while ((ch = getc(INFILE)) != EOF && (inquote || ch != ']')) {
      if (ch == '[' && ! inquote) {             /* comment; find its end */
        if ((ch = treeFinishCom()) == EOF)  break;
        }
      else if (ch == '\'') inquote = ! inquote;  /* start or end of quote */
      }

    return  ch;
  } /* treeFinishCom */


int treeGetCh ()
    /* get next nonblank, noncomment character */
  { /* treeGetCh */
    int  ch;

    while ((ch = getc(INFILE)) != EOF) {
      if (white(ch)) ;
      else if (ch == '[') {                   /* comment; find its end */
        if ((ch = treeFinishCom()) == EOF)  break;
        }
      else  break;
      }

    return  ch;
  } /* treeGetCh */


void  treeFlushLabel ()
  { /* treeFlushLabel */
    int      ch;
    boolean  done, quoted;

    if ((ch = treeGetCh()) == EOF)  return;
    done = (ch == ':' || ch == ',' || ch == ')'  || ch == '[' || ch == ';');
    if (! done && (quoted = (ch == '\'')))  ch = getc(INFILE);

    while (! done) {
      if (quoted) {
        if ((ch = findch('\'')) == EOF)  return;      /* find close quote */
        ch = getc(INFILE);                            /* check next char */
        if (ch != '\'') done = TRUE;                  /* not doubled quote */
        }
      else if (ch == ':' || ch == ',' || ch == ')'  || ch == '['
                         || ch == ';' || ch == '\n' || ch == EOF) {
        done = TRUE;
        }
      if (! done)  done = ((ch = getc(INFILE)) == EOF);
      }

    if (ch != EOF)  (void) ungetc(ch, INFILE);
  } /* treeFlushLabel */


int  findTipName (tr, ch)
    tree  *tr;
    int    ch;
  { /* findTipName */
    nodeptr  q;
    char  *nameptr, str[nmlngth+1];
    int  i, n;
    boolean  found, quoted, done;

    if (quoted = (ch == '\''))  ch = getc(INFILE);
    done = FALSE;
    i = 0;

    do {
      if (quoted) {
        if (ch == '\'') {
          ch = getc(INFILE);
          if (ch != '\'') done = TRUE;
          }
        else if (ch == EOF)
          done = TRUE;
        else if (ch == '\n' || ch == '\t')
          ch = ' ';
        }
      else if (ch == ':' || ch == ','  || ch == ')'  || ch == '['
                         || ch == '\n' || ch == EOF)
        done = TRUE;
      else if (ch == '_' || ch == '\t')
        ch = ' ';

      if (! done) {
        if (i < nmlngth)  str[i++] = ch;
        ch = getc(INFILE);
        }
      } while (! done);

    if (ch == EOF) {
      printf("ERROR: End-of-file in tree species name\n");
      return  0;
      }

    (void) ungetc(ch, INFILE);
    while (i < nmlngth)  str[i++] = ' ';     /*  Pad name */

    n = 1;
    do {
      q = tr->nodep[n];
      if (! (q->back)) {          /*  Only consider unused tips */
        i = 0;
        nameptr = q->name;
        do {found = str[i] == *nameptr++;}  while (found && (++i < nmlngth));
        }
      else
        found = FALSE;
      } while ((! found) && (++n <= tr->mxtips));

    if (! found) {
      i = nmlngth;
      do {str[i] = '\0';} while (i-- && (str[i] <= ' '));
      printf("ERROR: Cannot find data for tree species: %s\n", str);
      }

    return  (found ? n : 0);
  } /* findTipName */


double processLength ()
  { /* processLength */
    double  branch;
    int     ch;
    char    string[41];

    ch = treeGetCh();                            /*  Skip comments */
    if (ch != EOF)  (void) ungetc(ch, INFILE);

    if (fscanf(INFILE, "%lf", &branch) != 1) {
      printf("ERROR: Problem reading branch length in processLength:\n");
      if (fscanf(INFILE, "%40s", string) == 1)  printf("%s\n", string);
      anerror = TRUE;
      branch = 0.0;
      }

    return  branch;
  } /* processLength */


void  treeFlushLen ()
  { /* treeFlushLen */
    int  ch;

    if ((ch = treeGetCh()) == ':')
      (void) processLength();
    else if (ch != EOF)
      (void) ungetc(ch, INFILE);

  } /* treeFlushLen */


void  treeNeedCh (c1, where)
    int    c1;
    char  *where;
  { /* treeNeedCh */
    int  c2, i;

    if ((c2 = treeGetCh()) == c1)  return;

    printf("ERROR: Missing '%c' %s tree; ", c1, where);
    if (c2 == EOF) 
      printf("End-of-File");
    else {
      putchar('\'');
      for (i = 24; i-- && (c2 != EOF); c2 = getc(INFILE))  putchar(c2);
      putchar('\'');
      }
    printf(" found instead\n");
    anerror = TRUE;
  } /* treeNeedCh */


void  addElementLen (tr, p)
    tree    *tr;
    nodeptr  p;
  { /* addElementLen */
    double   z, branch;
    nodeptr  q;
    int      n, ch;

    if ((ch = treeGetCh()) == '(') {     /*  A new internal node */
      n = (tr->nextnode)++;
      if (n > 2*(tr->mxtips) - 2) {
        if (tr->rooted || n > 2*(tr->mxtips) - 1) {
          printf("ERROR: Too many internal nodes.  Is tree rooted?\n");
          printf("       Deepest splitting should be a trifurcation.\n");
          anerror = TRUE;
          return;
          }
        else {
          tr->rooted = TRUE;
          }
        }
      q = tr->nodep[n];
      addElementLen(tr, q->next);        if (anerror)  return;
      treeNeedCh(',', "in");             if (anerror)  return;
      addElementLen(tr, q->next->next);  if (anerror)  return;
      treeNeedCh(')', "in");             if (anerror)  return;
      treeFlushLabel();                  if (anerror)  return;
      }

    else {                               /*  A new tip */
      n = findTipName(tr, ch);
      if (n <= 0) {anerror = TRUE; return; }
      q = tr->nodep[n];
      if (tr->start->number > n)  tr->start = q;
      (tr->ntips)++;
      }                                  /* End of tip processing */

    if (tr->userlen) {
      treeNeedCh(':', "in");             if (anerror)  return;
      branch = processLength();          if (anerror)  return;
      z = exp(-branch / fracchange);
      if (z > zmax)  z = zmax;
      hookup(p, q, z);
      }
    else {
      treeFlushLen();                    if (anerror)  return;
      hookup(p, q, defaultz);
      }
  } /* addElementLen */


int saveTreeCom (comstrp)
    char  **comstrp;
  { /* saveTreeCom */
    int  ch;
    boolean  inquote;

    inquote = FALSE;
    while ((ch = getc(INFILE)) != EOF && (inquote || ch != ']')) {
      *(*comstrp)++ = ch;                        /* save character  */
      if (ch == '[' && ! inquote) {              /* comment; find its end */
        if ((ch = saveTreeCom(comstrp)) == EOF)  break;
        *(*comstrp)++ = ch;                      /* add ] */
        }
      else if (ch == '\'') inquote = ! inquote;  /* start or end of quote */
      }

    return  ch;
  } /* saveTreeCom */


boolean processTreeCom(tr)
    tree   *tr;
  { /* processTreeCom */
    int   ch, text_started, functor_read, com_open;

    /*  Accept prefatory "phylip_tree(" or "pseudoNewick("  */

    functor_read = text_started = 0;
    fscanf(INFILE, " p%nhylip_tree(%n", &text_started, &functor_read);
    if (text_started && ! functor_read) {
      fscanf(INFILE, "seudoNewick(%n", &functor_read);
      if (! functor_read) {
        printf("Start of tree 'p...' not understood.\n");
        anerror = TRUE;
        return;
        }
      }

    com_open = 0;
    fscanf(INFILE, " [%n", &com_open);

    if (com_open) {                              /* comment; read it */
      char  com[1024], *com_end;

      com_end = com;
      if (saveTreeCom(&com_end) == EOF) {        /* omits enclosing []s */
        printf("Missing end of tree comment.\n");
        anerror = TRUE;
        return;
        }

      *com_end = 0;
      (void) readKeyValue(com, likelihood_key, "%lg",
                               (void *) &(tr->likelihood));
      (void) readKeyValue(com, opt_level_key,  "%d",
                               (void *) &(tr->opt_level));
      (void) readKeyValue(com, smoothed_key,   "%d",
                               (void *) &(tr->smoothed));

      if (functor_read)  fscanf(INFILE, " ,");   /* remove trailing comma */
      }

    return (functor_read > 0);
  } /* processTreeCom */


void uprootTree (tr, p)
    tree   *tr;
    nodeptr p;
  { /* uprootTree */
    nodeptr  q, r, s;
    int  n;

    if (p->tip || p->back) {
      printf("ERROR: Unable to uproot tree.\n");
      printf("       Inappropriate node marked for removal.\n");
      anerror = TRUE;
      return;
      }

    n = --(tr->nextnode);               /* last internal node added */
    if (n != tr->mxtips + tr->ntips - 1) {
      printf("ERROR: Unable to uproot tree.  Inconsistent\n");
      printf("       number of tips and nodes for rooted tree.\n");
      anerror = TRUE;
      return;
      }

    q = p->next->back;                  /* remove p from tree */
    r = p->next->next->back;
    hookup(q, r, tr->userlen ? (q->z * r->z) : defaultz);

    q = tr->nodep[n];
    r = q->next;
    s = q->next->next;
    if (tr->ntips > 2 && p != q && p != r && p != s) {
      hookup(p,             q->back, q->z);   /* move connections to p */
      hookup(p->next,       r->back, r->z);
      hookup(p->next->next, s->back, s->z);
      }

    q->back = r->back = s->back = (nodeptr) NULL;
    tr->rooted = FALSE;
  } /* uprootTree */


void treeReadLen (tr)
    tree  *tr;
  { /* treeReadLen */
    nodeptr  p;
    int  i, ch;
    boolean  is_fact, found;

    for (i = 1; i <= tr->mxtips; i++) tr->nodep[i]->back = (node *) NULL;
    tr->start       = tr->nodep[tr->mxtips];
    tr->ntips       = 0;
    tr->nextnode    = tr->mxtips + 1;
    tr->opt_level   = 0;
    tr->log_f_valid = 0;
    tr->smoothed    = FALSE;
    tr->rooted      = FALSE;

    is_fact = processTreeCom(tr);

    p = tr->nodep[(tr->nextnode)++];
    treeNeedCh('(', "at start of");                  if (anerror)  return;
    addElementLen(tr, p);                            if (anerror)  return;
    treeNeedCh(',', "in");                           if (anerror)  return;
    addElementLen(tr, p->next);                      if (anerror)  return;
    if (! tr->rooted) {
      if ((ch = treeGetCh()) == ',') {        /*  An unrooted format */
        addElementLen(tr, p->next->next);            if (anerror)  return;
        }
      else {                                  /*  A rooted format */
        p->next->next->back = (nodeptr) NULL;
        tr->rooted = TRUE;
        if (ch != EOF)  (void) ungetc(ch, INFILE);
        }
      }
    treeNeedCh(')', "in");                           if (anerror)  return;
    treeFlushLabel();                                if (anerror)  return;
    treeFlushLen();                                  if (anerror)  return;
    if (is_fact) {
      treeNeedCh(')', "at end of");                  if (anerror)  return;
      treeNeedCh('.', "at end of");                  if (anerror)  return;
      }
    else {
      treeNeedCh(';', "at end of");                  if (anerror)  return;
      }

    if (tr->rooted)  uprootTree(tr, p->next->next);  if (anerror)  return;
    tr->start = p->next->next->back;  /* This is start used by treeString */

    initrav(tr->start);
    initrav(tr->start->back);
  } /* treeReadLen */


/*=======================================================================*/
/*                        Read a tree from a string                      */
/*=======================================================================*/


#if Master || Slave
int str_treeFinishCom (treestrp)
    char **treestrp;  /*  tree string pointer */
  { /* str_treeFinishCom */
    int ch;
    boolean  inquote;

    inquote = FALSE;
    while ((ch = *(*treestrp)++) != NULL && (inquote || ch != ']')) {
      if      (ch == '[' && ! inquote) {         /* comment; find its end */
        if ((ch = str_treeFinishCom(treestrp)) == NULL)  break;
        }
      else if (ch == '\'') inquote = ! inquote;  /* start or end of quote */
      }
    return  ch;
  } /* str_treeFinishCom */


int str_treeGetCh (treestrp)
    char **treestrp;  /*  tree string pointer */
    /* get next nonblank, noncomment character */
  { /* str_treeGetCh */
    int  ch;
    
    while ((ch = *(*treestrp)++) != NULL) {
      if (white(ch)) ;
      else if (ch == '[') {                   /* comment; find its end */
        if ((ch = str_treeFinishCom(treestrp)) == NULL)  break;
        }
      else  break;
      }

    return  ch;
  } /* str_treeGetCh */


void  str_treeFlushLabel (treestrp)
    char **treestrp;  /*  tree string pointer */
  { /* str_treeFlushLabel */
    int      ch;
    boolean  done, quoted;

    if ((ch = str_treeGetCh(treestrp)) == NULL)  done = TRUE;
    else {
      done = (ch == ':' || ch == ',' || ch == ')'  || ch == '[' || ch == ';');
      if (! done && (quoted = (ch == '\'')))  ch = *(*treestrp)++;
      }

    while (! done) {
      if (quoted) {
        if ((ch = str_findch(treestrp, '\'')) == NULL)  done = TRUE;
        else {
          ch = *(*treestrp)++;                        /* check next char */
          if (ch != '\'') done = TRUE;                /* not doubled quote */
          }
        }
      else if (ch == ':' || ch == ',' || ch == ')'  || ch == '['
                         || ch == ';' || ch == '\n' || ch == NULL) {
        done = TRUE;
        }
      if (! done)  done = ((ch = *(*treestrp)++) == NULL);
      }

    (*treestrp)--;
  } /* str_treeFlushLabel */


int  str_findTipName (treestrp, tr, ch)
    char **treestrp;  /*  tree string pointer */
    tree  *tr;
    int    ch;
  { /* str_findTipName */
    nodeptr  q;
    char  *nameptr, str[nmlngth+1];
    int  i, n;
    boolean  found, quoted, done;

    i = 0;
    if (quoted = (ch == '\'')) ch = *(*treestrp)++;
    done = FALSE;

    do {
      if (quoted) {
        if (ch == '\'') {
          ch = *(*treestrp)++;
          if (ch != '\'') done = TRUE;
          }
        else if (ch == NULL)
          done = TRUE;
        else if (ch == '\n' || ch == '\t')
          ch = ' ';
        }
      else if (ch == ':' || ch == ','  || ch == ')'  || ch == '['
                         || ch == '\n' || ch == NULL)
        done = TRUE;
      else if (ch == '_' || ch == '\t')
        ch = ' ';

      if (! done) {
        if (i < nmlngth)  str[i++] = ch;
        ch = *(*treestrp)++;
        }
      } while (! done);

    (*treestrp)--;
    if (ch == NULL) {
      printf("ERROR: NULL in tree species name\n");
      return  0;
      }

    while (i < nmlngth)  str[i++] = ' ';     /*  Pad name */

    n = 1;
    do {
      q = tr->nodep[n];
      if (! (q->back)) {          /*  Only consider unused tips */
        i = 0;
        nameptr = q->name;
        do {found = str[i] == *nameptr++;}  while (found && (++i < nmlngth));
        }
      else
        found = FALSE;
      } while ((! found) && (++n <= tr->mxtips));

    if (! found) {
      i = nmlngth;
      do {str[i] = '\0';} while (i-- && (str[i] <= ' '));
      printf("ERROR: Cannot find data for tree species: %s\n", str);
      }

    return  (found ? n : 0);
  } /* str_findTipName */


double str_processLength (treestrp)
    char **treestrp;   /*  tree string ponter */
  { /* str_processLength */
    double  branch;
    int     used;

    (void) str_treeGetCh(treestrp);                /*  Skip comments */
    (*treestrp)--;

    if (sscanf(*treestrp, "%lf%n", &branch, &used) != 1) {
      printf("ERROR: Problem reading branch length in str_processLength:\n");
      printf("%40s\n", *treestrp);
      anerror = TRUE;
      branch = 0.0;
      }
    else {
      *treestrp += used;
      }

    return  branch;
  } /* str_processLength */


void  str_treeFlushLen (treestrp)
    char **treestrp;   /*  tree string ponter */
  { /* str_treeFlushLen */
    int  ch;

    if ((ch = str_treeGetCh(treestrp)) == ':')
      (void) str_processLength(treestrp);
    else
      (*treestrp)--;

  } /* str_treeFlushLen */


void  str_treeNeedCh (treestrp, c1, where)
    char **treestrp;   /*  tree string pointer */
    int    c1;
    char  *where;
  { /* str_treeNeedCh */
    int  c2, i;

    if ((c2 = str_treeGetCh(treestrp)) == c1)  return;

    printf("ERROR: Missing '%c' %s tree; ", c1, where);
    if (c2 == NULL) 
      printf("NULL");
    else {
      putchar('\'');
      for (i = 24; i-- && (c2 != NULL); c2 = *(*treestrp)++)  putchar(c2);
      putchar('\'');
      }
    printf(" found instead\n");
    anerror = TRUE;
  } /* str_treeNeedCh */


void  str_addElementLen (treestrp, tr, p)
    char **treestrp;  /*  tree string pointer */
    tree    *tr;
    nodeptr  p;
  { /* str_addElementLen */
    double   z, branch;
    nodeptr  q;
    int      n, ch;

    if ((ch = str_treeGetCh(treestrp)) == '(') { /*  A new internal node */
      n = (tr->nextnode)++;
      if (n > 2*(tr->mxtips) - 2) {
        if (tr->rooted || n > 2*(tr->mxtips) - 1) {
          printf("ERROR: too many internal nodes.  Is tree rooted?\n");
          printf("Deepest splitting should be a trifurcation.\n");
          anerror = TRUE;
          return;
          }
        else {
          tr->rooted = TRUE;
          }
        }
      q = tr->nodep[n];
      str_addElementLen(treestrp, tr, q->next);        if (anerror)  return;
      str_treeNeedCh(treestrp, ',', "in");             if (anerror)  return;
      str_addElementLen(treestrp, tr, q->next->next);  if (anerror)  return;
      str_treeNeedCh(treestrp, ')', "in");             if (anerror)  return;
      str_treeFlushLabel(treestrp);                    if (anerror)  return;
      }

    else {                           /*  A new tip */
      n = str_findTipName(treestrp, tr, ch);
      if (n <= 0) {anerror = TRUE; return; }
      q = tr->nodep[n];
      if (tr->start->number > n)  tr->start = q;
      (tr->ntips)++;
      }                              /* End of tip processing */

    /*  Master and Slave always use lengths */

    str_treeNeedCh(treestrp, ':', "in");               if (anerror)  return;
    branch = str_processLength(treestrp);              if (anerror)  return;
    z = exp(-branch / fracchange);
    if (z > zmax)  z = zmax;
    hookup(p, q, z);

    return;
  } /* str_addElementLen */


boolean str_processTreeCom(tr, treestrp)
    tree   *tr;
    char  **treestrp;
  { /* str_processTreeCom */
    char  *com, *com_end;
    int  text_started, functor_read, com_open;

    com = *treestrp;

    functor_read = text_started = 0;
    sscanf(com, " p%nhylip_tree(%n", &text_started, &functor_read);
    if (functor_read) {
      com += functor_read;
      }
    else if (text_started) {
      com += text_started;
      sscanf(com, "seudoNewick(%n", &functor_read);
      if (! functor_read) {
        printf("Start of tree 'p...' not understood.\n");
        anerror = TRUE;
        return;
        }
      else {
        com += functor_read;
        }
      }

    com_open = 0;
    sscanf(com, " [%n", &com_open);
    com += com_open;

    if (com_open) {                              /* comment; read it */
	if (!(com_end = index(com, ']'))) {
        printf("Missing end of tree comment.\n");
        anerror = TRUE;
        return;
        }

      *com_end = 0;
      (void) readKeyValue(com, likelihood_key, "%lg",
                               (void *) &(tr->likelihood));
      (void) readKeyValue(com, opt_level_key,  "%d",
                               (void *) &(tr->opt_level));
      (void) readKeyValue(com, smoothed_key,   "%d",
                               (void *) &(tr->smoothed));
      *com_end = ']';
      com_end++;

      if (functor_read) {                          /* remove trailing comma */
        text_started = 0;
        sscanf(com_end, " ,%n", &text_started);
        com_end += text_started;
        }

      *treestrp = com_end;
      }

    return (functor_read > 0);
  } /* str_processTreeCom */


void str_treeReadLen (treestr, tr)
    char  *treestr;
    tree  *tr;
    /* read string with representation of tree */
  { /* str_treeReadLen */
    nodeptr  p;
    int  i;
    boolean  is_fact, found;

    for (i = 1; i <= tr->mxtips; i++) tr->nodep[i]->back = (node *) NULL;
    tr->start       = tr->nodep[tr->mxtips];
    tr->ntips       = 0;
    tr->nextnode    = tr->mxtips + 1;
    tr->opt_level   = 0;
    tr->log_f_valid = 0;
    tr->smoothed    = Master;
    tr->rooted      = FALSE;

    is_fact = str_processTreeCom(tr, &treestr);

    p = tr->nodep[(tr->nextnode)++];
    str_treeNeedCh(&treestr, '(', "at start of");    if (anerror)  return;
    str_addElementLen(&treestr, tr, p);              if (anerror)  return;
    str_treeNeedCh(&treestr, ',', "in");             if (anerror)  return;
    str_addElementLen(&treestr, tr, p->next);        if (anerror)  return;
    if (! tr->rooted) {
      if (str_treeGetCh(&treestr) == ',') {        /*  An unrooted format */
        str_addElementLen(&treestr, tr, p->next->next);
        if (anerror)  return;
        }
      else {                                       /*  A rooted format */
        p->next->next->back = (nodeptr) NULL;
        tr->rooted = TRUE;
        treestr--;
        }
      }
    str_treeNeedCh(&treestr, ')', "in");             if (anerror)  return;
    str_treeFlushLabel(&treestr);                    if (anerror)  return;
    str_treeFlushLen(&treestr);                      if (anerror)  return;
    if (is_fact) {
      str_treeNeedCh(&treestr, ')', "at end of");    if (anerror)  return;
      str_treeNeedCh(&treestr, '.', "at end of");    if (anerror)  return;
      }
    else {
      str_treeNeedCh(&treestr, ';', "at end of");    if (anerror)  return;
      }

    if (tr->rooted)  uprootTree(tr, p->next->next);  if (anerror)  return;
    tr->start = p->next->next->back;  /* This is start used by treeString */

    initrav(tr->start);
    initrav(tr->start->back);
  } /* str_treeReadLen */
#endif


void treeEvaluate (tr, bt)         /* Evaluate a user tree */
    tree     *tr;
    bestlist *bt;
  { /* treeEvaluate */

    if (Slave || ! tr->userlen)  smoothTree(tr, 4 * smoothings);

    (void) evaluate(tr, tr->start);
    if (anerror)  return;

#   if ! Slave
      (void) saveBestTree(bt, tr);
#   endif
  } /* treeEvaluate */


#if Master || Slave
FILE *freopen_pid (filenm, mode, stream)
    char *filenm, *mode;
    FILE *stream;
  { /* freopen_pid */
    char scr[512];

    (void) sprintf(scr, "%s.%d", filenm, getpid());
    return  freopen(scr, mode, stream);
  } /* freopen_pid */
#endif


void  showBestTrees (bt, tr, treefile)
    bestlist *bt;
    tree     *tr;
    FILE     *treefile;
  { /* showBestTrees */
    int     rank;

    for (rank = 1; rank <= bt->nvalid; rank++) {
      if (rank > 1) {
        if (rank != recallBestTree(bt, rank, tr))  break;
        }
      (void) evaluate(tr, tr->start);
      if (anerror)  return;
      if (tr->outgrnode->back)  tr->start = tr->outgrnode;
      printTree(tr);
      summarize(tr);
      if (treefile)  treeOut(treefile, tr, trout);
      }
  } /* showBestTrees */


void cmpBestTrees (bt, tr)
    bestlist *bt;
    tree     *tr;
  { /* cmpBestTrees */
    double  sum, sum2, sd, temp, bestscore;
    double  log_f0[maxpatterns];      /* Save a copy of best log_f */
    double *log_f_ptr, *log_f0_ptr;
    int     i, j, num, besttips;

    num = bt->nvalid;
    if ((num <= 1) || (weightsum <= 1)) return;

    printf("Tree      Ln L        Diff Ln L       Its S.D.");
    printf("   Significantly worse?\n\n");

    for (i = 1; i <= num; i++) {
      if (i != recallBestTree(bt, i, tr))  break;
      if (! (tr->log_f_valid))  (void) evaluate(tr, tr->start);

      printf("%3d%14.5f", i, tr->likelihood);
      if (i == 1) {
        printf("  <------ best\n");
        besttips = tr->ntips;
        bestscore = tr->likelihood;
        log_f0_ptr = log_f0;
        log_f_ptr  = tr->log_f;
        for (j = 0; j < endsite; j++)  *log_f0_ptr++ = *log_f_ptr++;
        }
      else if (tr->ntips != besttips)
        printf("  (different number of species)\n");
      else {
        sum = sum2 = 0.0;
        log_f0_ptr = log_f0;
        log_f_ptr  = tr->log_f;
        for (j = 0; j < endsite; j++) {
          temp  = *log_f0_ptr++ - *log_f_ptr++;
          sum  += aliasweight[j] * temp;
          sum2 += aliasweight[j] * temp * temp;
          }
        sd = sqrt( weightsum * (sum2 - sum*sum/weightsum) / (weightsum-1) );
        printf("%14.5f%14.4f", tr->likelihood - bestscore, sd);
        printf("           %s\n", (sum > 1.95996 * sd) ? "Yes" : " No");
        }
      }

    printf("\n\n");
  } /* cmpBestTrees */


void makeUserTree (tr, bt)
    tree     *tr;
    bestlist *bt;
  { /* makeUserTree */
    FILE  *treefile;
    int    nusertrees, which;

    if (fscanf(INFILE, "%d", &nusertrees) != 1 || findch('\n') == EOF) {
      printf("ERROR: Problem reading number of user trees\n");
      anerror = TRUE;
      return;
      }

    printf("User-defined %s:\n\n", (nusertrees == 1) ? "tree" : "trees");

    treefile = trout ? fopen_pid("treefile","w") : (FILE *) NULL;

    for (which = 1; which <= nusertrees; which++) {
      treeReadLen(tr);                     if (anerror)  return;
      treeEvaluate(tr, bt);                if (anerror)  return;
      if (tr->global <= 0) {
        if (tr->outgrnode->back)  tr->start = tr->outgrnode;
        printTree(tr);
        summarize(tr);
        if (treefile)  treeOut(treefile, tr, trout);
        }
      else {
        printf("%6d:  Ln Likelihood =%14.5f\n", which, tr->likelihood);
        }
      }

    if (tr->global > 0) {
      putchar('\n');
      if (! recallBestTree(bt, 1, tr)) anerror = TRUE;
      if (anerror)  return;
      printf("      Ln Likelihood =%14.5f\n", tr->likelihood);
      optimize(tr, tr->global, bt);
      if (anerror)  return;
      if (tr->outgrnode->back)  tr->start = tr->outgrnode;
      printTree(tr);
      summarize(tr);
      if (treefile)  treeOut(treefile, tr, trout);
      }

    if (treefile) {
      (void) fclose(treefile);
      printf("Tree also written to file\n");
      }

    putchar('\n');
    if (anerror)  return;

    cmpBestTrees(bt, tr);
  } /* makeUserTree */


#if Slave
void slaveTreeEvaluate (tr, bt)
    tree     *tr;
    bestlist *bt;
  { /* slaveTreeEvaluate */
    boolean done;

    do {
       requestForWork();
       receiveTree(&comm_master, tr);
       done = tr->likelihood > 0.0;
       if (! done) {
         treeEvaluate(tr, bt);         if (anerror) return;
         sendTree(&comm_master, tr);   if (anerror) return;
         }
       } while (! done);
  } /* slaveTreeEvaluate */
#endif


double randum (seed)
    longer  seed;
    /* random number generator */
  { /* randum */
    int  i, j, k, sum, mult[4];
    longer  newseed;
    double  x;

    mult[0] = 13;
    mult[1] = 24;
    mult[2] = 22;
    mult[3] =  6;
    for (i = 0; i <= 5; i++) newseed[i] = 0;
    for (i = 0; i <= 5; i++) {
      sum = newseed[i];
      k = MIN(i,3);
      for (j = 0; j <= k; j++) sum += mult[j] * seed[i-j];
      newseed[i] = sum;
      for (j = i; j <= 4; j++) {
        newseed[j+1] += newseed[j] >> 6;
        newseed[j] &= 63;
        }
      }
    newseed[5] &= 3;
    x = 0.0;
    for (i = 0; i <= 5; i++) x = 0.015625 * x + (seed[i] = newseed[i]);
    return  0.25 * x;
  } /* randum */


void makeDenovoTree (tr, bt)
    tree     *tr;
    bestlist *bt;
  { /* makeDenovoTree */
    FILE  *treefile;
    nodeptr  p;
    int  enterorder[maxsp+1];      /*  random entry order */
    int  i, j, k, nextsp, newsp, maxtrav, tested;

    if (restart) {
      printf("Restarting from tree with the following sequences:\n");
      tr->userlen = TRUE;
      treeReadLen(tr);                        if (anerror)  return;
      smoothTree(tr, smoothings);             if (anerror)  return;
      (void) evaluate(tr, tr->start);         if (anerror)  return;
      (void) saveBestTree(bt, tr);            if (anerror)  return;

      for (i = 1, j = tr->ntips; i <= tr->mxtips; i++) { /* find loose tips */
        if (! tr->nodep[i]->back) enterorder[++j] = i;
        else {
          char  trimmed[nmlngth + 1];

          copyTrimmedName(tr->nodep[i]->name, trimmed);
          printf("   %s\n", trimmed);
#         if Master
            if (i>3) REPORT_ADD_SPECS;
#         endif
          }
        }
      putchar('\n');
      }

    else {                                           /* start from scratch */
      tr->ntips = 0;
      for (i = 1; i <= tr->mxtips; i++) enterorder[i] = i;
      }

    if (jumble) for (i = tr->ntips + 1; i <= tr->mxtips; i++) {
      j = randum(seed)*(tr->mxtips - tr->ntips) + tr->ntips + 1;
      k = enterorder[j];
      enterorder[j] = enterorder[i];
      enterorder[i] = k;
      }

    bt->numtrees = 1;
    if (tr->ntips < tr->mxtips)  printf("Adding species:\n");

    if (tr->ntips == 0) {
      for (i = 1; i <= 3; i++) {
        char  trimmed[nmlngth + 1];

        copyTrimmedName(tr->nodep[enterorder[i]]->name, trimmed);
        printf("   %s\n", trimmed);
        }
      tr->nextnode = tr->mxtips + 1;
      buildSimpleTree(tr, enterorder[1], enterorder[2], enterorder[3]);
      }

    if (anerror)  return;

    while (tr->ntips < tr->mxtips || tr->opt_level < tr->global) {
      maxtrav = (tr->ntips == tr->mxtips) ? tr->global : tr->partswap;
      if (maxtrav > tr->ntips - 3)  maxtrav = tr->ntips - 3;

      if (tr->opt_level >= maxtrav) {
        char  trimmed[nmlngth + 1];

        nextsp = ++(tr->ntips);
        newsp = enterorder[nextsp];
        p = tr->nodep[newsp];

        copyTrimmedName(p->name, trimmed);
        printf("   %s\n", trimmed);

#       if Master
          if (nextsp % DNAML_STEP_TIME_COUNT == 1) {
            REPORT_STEP_TIME;
            }
          REPORT_ADD_SPECS;
#       endif

        (void) buildNewTip(tr, p);

        resetBestTree(bt);
        cacheZ(tr);
        tested = addTraverse(tr, p->back, findAnyTip(tr->start)->back,
                             1, tr->ntips - 2, bt, fastadd);
        bt->numtrees += tested;
        if (anerror)  return;

#       if Master
          getReturnedTrees(tr, bt, tested);
#       endif

        printf("      Tested %d alternative trees\n", tested);

        (void) recallBestTree(bt, 1, tr);
        if (! tr->smoothed) {
          smoothTree(tr, smoothings);         if (anerror)  return;
          (void) evaluate(tr, tr->start);     if (anerror)  return;
          (void) saveBestTree(bt, tr);
          }

        if (tr->ntips == 4)  tr->opt_level = 1;  /* All 4 taxon trees done */
        maxtrav = (tr->ntips == tr->mxtips) ? tr->global : tr->partswap;
        if (maxtrav > tr->ntips - 3)  maxtrav = tr->ntips - 3;
        }

      printf("      Ln Likelihood =%14.5f\n", tr->likelihood);
      optimize(tr, maxtrav, bt);

      if (anerror)  return;
      }

    printf("\nExamined %d %s\n", bt->numtrees,
                                 bt->numtrees != 1 ? "trees" : "tree");

    treefile = trout ? fopen_pid("treefile","w") : (FILE *) NULL;
    showBestTrees(bt, tr, treefile);
    if (treefile) {
      (void) fclose(treefile);
      printf("Tree also written to file\n\n");
      }

    cmpBestTrees(bt, tr);

#   if DeleteCheckpointFile
      unlink_pid(checkpointname);
#   endif
  } /* makeDenovoTree */

/*==========================================================================*/
/*                             "main" routine                               */
/*==========================================================================*/

#if Sequential
  main ()
#else
  void slave ()
#endif
  { /* DNA Maximum Likelihood */
#   if Master
      int starttime, inputtime, endtime;
#   endif

#   if Master || Slave
      int my_id, nprocs, type, from, sz;
      char *msg;
#   endif

    tree      curtree, *tr;   /*  current tree */
    bestlist  bestree, *bt;   /*  topology of best found tree */


#   if Debug
      debug = fopen_pid("dnaml.debug","w");
#   endif

#   if Master
      starttime = p4_clock();
      nprocs = p4_num_total_slaves();

      if ((OUTFILE = freopen_pid("master.out", "w", stdout)) == NULL) {
        fprintf(stderr, "Could not open output file\n");
        exit(1);
        }

      /* Receive input file name from host */
      type = DNAML_FILE_NAME; from = -1;
      p4_recv(&type, &from, &msg, &sz);
      if ((INFILE = fopen(msg, "r")) == NULL) {
        fprintf(stderr, "master could not open input file %s\n", msg);
        exit(1);
        }
      p4_msg_free(msg);

      open_link(&comm_slave);
#   endif

#  if DNAML_STEP
      begin_step_time = starttime;
#  endif

#   if Slave
      my_id = p4_get_my_id();
      nprocs = p4_num_total_slaves();

      /* Receive input file name from host */
      type = DNAML_FILE_NAME; from = -1;
      p4_recv(&type, &from, &msg, &sz);
      if ((INFILE = fopen(msg, "r")) == NULL) {
        fprintf(stderr, "slave could not open input file %s\n",msg);
        exit(1);
        }
      p4_msg_free(msg);

      if ((OUTFILE = freopen("/dev/null", "w", stdout)) == NULL) {
        fprintf(stderr, "Could not open output file\n");
        exit(1);
        }

      open_link(&comm_master);
#   endif

    anerror = FALSE;
    tr = & curtree;
    bt = & bestree;
    bt->ninit = 0;
    getinput(tr);                               if (anerror)  return 1;

#   if Master
      inputtime = p4_clock();
      printf("Input time %d milliseconds\n",inputtime-starttime);
      REPORT_STEP_TIME;
#   endif

#   if Slave
      (void) fclose(INFILE);
#   endif

/*  The material below would be a loop over jumbles and/or boots */

    makeweights();                              if (anerror)  return 1;
    makevalues(tr);                             if (anerror)  return 1;
    if (freqsfrom) {
      empiricalfreqs(tr);                       if (anerror)  return 1;
      getbasefreqs();
      }

    linkxarray(3, 3, & freextip, & usedxtip);   if (anerror)  return 1;
    setupnodex(tr);                             if (anerror)  return 1;

#   if Slave
      slaveTreeEvaluate(tr, bt);
#   else
      (void) initBestTree(bt, nkeep, endsite);      if (anerror)  return 1;
      if (! tr->usertree) {makeDenovoTree(tr, bt);  if (anerror)  return 1;}
      else                {makeUserTree(tr, bt);    if (anerror)  return 1;}
      freeBestTree(bt);                             if (anerror)  return 1;
#   endif

/*  Endpoint for jumble and/or boot loop */

#   if Master
      tr->likelihood = 1.0;             /* terminate slaves */
      sendTree(&comm_slave, tr);
#   endif

    freeTree(tr);

#   if Master
      close_link(&comm_slave);
      (void) fclose(INFILE);

      REPORT_STEP_TIME;
      endtime = p4_clock();
      printf("Execution time %d milliseconds\n", endtime - inputtime);
      (void) fclose(OUTFILE);
#   endif

#   if Slave
      close_link(&comm_master);
      (void) fclose(OUTFILE);
#   endif

#   if Debug
      (void) fclose(debug);
#   endif

#   if Master || Slave
      p4_send(DNAML_DONE, DNAML_HOST_ID, NULL, 0);
#   else
      return 0;
#   endif
  } /* DNA Maximum Likelihood */
