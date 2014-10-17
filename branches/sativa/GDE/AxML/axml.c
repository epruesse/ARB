#define programName        "AxMl"
#define programVersion     "1.1.0"
#define programVersionInt   10202
#define programDate        "February, 2002"
#define programDateInt      20000212

/*Time measurment */
#include  <sys/times.h>
#include  <sys/types.h>
#include <sys/time.h>
#include <time.h> 
#include <stdlib.h>
#include <string.h>
double accTime = 0.0;
double globTime = 0.0;
struct timeval ttime;

long flopBlocks = 0;

double gettime()
{
  gettimeofday(&ttime , NULL);
  return ttime.tv_sec + ttime.tv_usec * 0.000001;
}
/*Time measurment*/

/* AxMl, a program for estimation of phylogenetic trees from sequences 
 * implementing SEVs (Subtree Equality Vectors) Copyright (C) 2002 by
 * Alexandros P. Stamatakis.
 *  Derived from:
 *  fastDNAml, a program for estimation of phylogenetic trees from sequences.
 *  Copyright (C) 1998, 1999, 2000 by Gary J. Olsen
 *
 *  This program is free software; you may redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *  For any other enquiries write to Gary J. Olsen, Department of Microbiology,
 *  University of Illinois, Urbana, IL  61801, USA
 *
 *  Or send E-mail to gary@phylo.life.uiuc.edu
 *
 *
 *  fastDNAml is based in part on the program dnaml by Joseph Felsenstein.
 *
 *  Copyright notice from dnaml:
 *
 *     version 3.3. (c) Copyright 1986, 1990 by the University of Washington
 *     and Joseph Felsenstein.  Written by Joseph Felsenstein.  Permission is
 *     granted to copy and use this program provided no fee is charged for it
 *     and provided that this copyright notice is not removed.
 *
 *
 *  When publishing work that based on results from fastDNAml please cite:
 *
 *  Felsenstein, J.  1981.  Evolutionary trees from DNA sequences:
 *  A maximum likelihood approach.  J. Mol. Evol. 17: 368-376.
 *
 *  and
 *
 *  Olsen, G. J., Matsuda, H., Hagstrom, R., and Overbeek, R.  1994.
 *  fastDNAml:  A tool for construction of phylogenetic trees of DNA
 *  sequences using maximum likelihood.  Comput. Appl. Biosci. 10: 41-48.
 */

/*  Conversion to C and changes in sequential code by Gary Olsen, 1991-1994
 *
 *  p4 version by Hideo Matsuda and Ross Overbeek, 1991-1993
 */

/*
 *  1.0    March 14, 1992
 *         Initial "release" version
 *
 *  1.0.1  March 18, 1992
 *         Add ntaxa to tree comments
 *         Set minimum branch length on reading tree
 *         Add blanks around operators in treeString (for prolog parsing)
 *         Add program version to treeString comments
 *
 *  1.0.2  April 6, 1992
 *         Improved option line diagnostics
 *         Improved auxiliary line diagnostics
 *         Removed some trailing blanks from output
 *
 *  1.0.3  April 6, 1992
 *         Checkpoint trees that do not need any optimization
 *         Print restart tree likelihood before optimizing
 *         Fix treefile option so that it really toggles
 *
 *  1.0.4  July 13, 1992
 *         Add support for tree fact (instead of true Newick tree) in
 *            processTreeCom, treeReadLen, str_processTreeCom and
 *            str_treeReadLen
 *         Use bit operations in randum
 *         Correct error in bootstrap mask used with weighting mask
 *
 *  1.0.5  August 22, 1992
 *         Fix reading of underscore as first nonblank character in name
 *         Add strchr and strstr functions to source code
 *         Add output treefile name to message "Tree also written ..."
 *
 *  1.0.6  November 20, 1992
 *         Change (! nsites) test in setupTopol to (nsites == 0) for MIPS R4000
 *         Add vectorizing compiler directives for CRAY
 *         Include updates and corrections to parallel code from H. Matsuda
 *
 *  1.0.7  March 25, 1993
 *         Remove translation of underlines in taxon names
 *
 *  1.0.8  April 30, 1993
 *         Remove version number from fastDNAml.h file name
 *
 *  1.0.9  August 12, 1993
 *         Version was never released.
 *         Redefine treefile formats and default:
 *             0  None
 *             1  Newick
 *             2  Prolog
 *             3  PHYLIP (Default)
 *         Remove quote marks and comment from PHYLIP treefile format.
 *
 *  1.1.0  September 3-5, 1993
 *         Arrays of size maxpatterns moved from stack to heap (mallocs) in
 *            evaluateslope, makenewz, and cmpBestTrees.
 *         Correct [maxsites] to [maxpatterns] in temporary array definitions
 *            in Vectorize code of newview and evaluate.  (These should also
 *            get converted to use malloc() at some point.)
 *         Change randum to use 12 bit segments, not 6.  Change its seed
 *            parameter to long *.
 *         Remove the code that took the absolute value of random seeds.
 *         Correct integer divide artifact in setting default transition/
 *            transversion parameter values.
 *         When transition/transversion ratio is "reset", change to small
 *            value, not the program default.
 *         Report the "reset" transition/transversion ratio in the output.
 *         Move global data into analdef, rawDNA, and crunchedDNA structures.
 *         Change names of routines white and digit to whitechar and digitchar.
 *         Convert y[] to yType, which is normally char, but is int if the
 *            Vectorize flag is set.
 *         Split option line reading out of getoptions routine.
 *
 *  1.1.1  September 30, 1994
 *         Incorporate changes made in 1.0.A (Feb. 11, 1994):
 *            Remove processing of quotation marks within comments.
 *            Break label finding into copy to string and find tip.
 *            Generalize tree reading to read trees when names are and are not
 *               already known.
 *            Remove absolute value from randum seed reading.
 *         Include integer version number and program date.
 *         Remove maxsite, maxpatterns and maxsp limitations.
 *         Incorporate code for retaining multiple trees.
 *         Activate code for Hasegawa & Kishino test of tree differences.
 *         Make quick add the default, with Q turning it off.
 *         Make emperical frequencies the option with F turning it off.
 *         Allow a residue frequency option line anywhere in the options.
 *         Increase string length passed to treeString (should be length
 *               checked, but ...)
 *         Introduce (Sept.30) and fix (Oct. 26) bug in bootstrap sampling.
 *         Fix error when user frequencies are last line and start with F.
 *
 *  1.2    September 5, 1997
 *         Move likelihood components into structure.
 *         Change rawDNA to rawdata.
 *         Change crunchedDNA to cruncheddata.
 *         Recast the likelihoods per site into an array of stuctures,
 *               where each stucture (likelivector) includes the likelihoods
 *               of each residue type at the site, and a magnitude scale
 *               factor (exp).  This requires changing the space allocation,
 *               newview, makenewz, evaluate, and sigma.
 *         Change code of newview to rescale likelihoods up by 2**256 when
 *               the largest value falls below 2**-256.  This should solve
 *               floating point underflow for all practical sized trees.
 *               No changes are necessary in makenewz or sigma, since only
 *               relative likelihoods are necessary.
 *
 *  1.2.1  March 9, 1998
 *         Convert likelihood adjustment factor (2**256) to a constant.
 *         Fix vectorized calculation of likelihood (error introduced in 1.2)
 *
 *  1.2.2  December 23, 1998
 *         General code clean-up.
 *         Convert to function definitions with parameter type lists
 *
 *  1.2.2  January 3, 2000
 *         Add copyright and license information
 *         Make this the current release version
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

#ifdef  CRAY
#  define  Vectorize
#endif

#ifdef  Vectorize
#  define maxpatterns  10000  /* maximum number of different site patterns */
#endif

#include <stdio.h>
#include <math.h>
#include "axml.h"  /*  Requires version 1.2  */

#if Master || Slave
#  include "p4.h"
#  include "comm_link.h" 
#endif

/*  Global variables */

xarray       *usedxtip, *freextip;

#ifdef FLOP_BLOCK_COUNTER
unsigned long flopBlock = 0;
#endif

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
#  define  INFILE   Seqf
#  define  OUTFILE  Outf
  FILE  *INFILE, *OUTFILE;
  comm_block comm_slave;
#endif

#if Slave
#  undef   DNAML_STEP
#  define  DNAML_STEP  0
#  define  INFILE   Seqf
#  define  OUTFILE  Outf
  FILE  *INFILE, *OUTFILE;
  comm_block comm_master;
#endif

#if Debug
  FILE *debug;
#endif

#if DNAML_STEP
  int begin_step_time, end_step_time;
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


void  *tipValPtr (nodeptr p)
  { return  (void *) & p->number;
    }


int  cmpTipVal (void *v1, void *v2)
  { /* cmpTipVal */
    int  i1, i2;

    i1 = *((int *) v1);
    i2 = *((int *) v2);
    return  (i1 < i2) ? -1 : ((i1 == i2) ? 0 : 1);
  } /* cmpTipVal */


/*  These are the only routines that need to UNDERSTAND topologies */


topol  *setupTopol (int maxtips, int nsites)
  { /* setupTopol */
    topol   *tpl;

    if (! (tpl = (topol *) Malloc(sizeof(topol))) || 
        ! (tpl->links = (connptr) Malloc((2*maxtips-3) * sizeof(connect))) || 
        (nsites && ! (tpl->log_f
                = (double *) Malloc(nsites * sizeof(double))))) {
      printf("ERROR: Unable to get topology memory");
      tpl = (topol *) NULL;
      }

    else {
      if (nsites == 0)  tpl->log_f = (double *) NULL;
      tpl->likelihood  = unlikely;
      tpl->start       = (node *) NULL;
      tpl->nextlink    = 0;
      tpl->ntips       = 0;
      tpl->nextnode    = 0;
      tpl->opt_level   = 0;     /* degree of branch swapping explored */
      tpl->scrNum      = 0;     /* position in sorted list of scores */
      tpl->tplNum      = 0;     /* position in sorted list of trees */
      tpl->log_f_valid = 0;     /* log_f value sites */
      tpl->prelabeled  = TRUE;
      tpl->smoothed    = FALSE; /* branch optimization converged? */
      }

    return  tpl;
  } /* setupTopol */


void  freeTopol (topol *tpl)
  { /* freeTopol */
    Free(tpl->links);
    if (tpl->log_f)  Free(tpl->log_f);
    Free(tpl);
  } /* freeTopol */


int  saveSubtree (nodeptr p, topol *tpl)
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


nodeptr  minSubtreeTip (nodeptr  p0)
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


nodeptr  minTreeTip (nodeptr  p)
  { /* minTreeTip */
    nodeptr  minp, minpb;

    minp  = minSubtreeTip(p);
    minpb = minSubtreeTip(p->back);
    return cmpTipVal(tipValPtr(minp), tipValPtr(minpb)) < 0 ? minp : minpb;
  } /* minTreeTip */


void saveTree (tree *tr, topol *tpl)
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
    tpl->prelabeled = tr->prelabeled;
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


void copyTopol (topol *tpl1, topol *tpl2)
  { /* copyTopol */
    connptr  r1, r2, r10, r20;
    double  *tpl1_log_f, *tpl2_log_f;
    int  i;

    r10 = tpl1->links;
    r20 = tpl2->links;
    tpl2->nextlink = tpl1->nextlink; 

    r1 = r10;
    r2 = r20;
    i = 2 * tpl1->ntips - 3;
    while (--i >= 0) {
      r2->z = r1->z;
      r2->p = r1->p;
      r2->q = r1->q;
      r2->valptr = r1->valptr;
      r2->descend = r1->descend; 
      r2->sibling = r1->sibling; 
      r1++;
      r2++;
      }

    if (tpl1->log_f_valid && tpl2->log_f) {
      tpl1_log_f = tpl1->log_f;
      tpl2_log_f = tpl2->log_f;
      tpl2->log_f_valid = i = tpl1->log_f_valid;
      while (--i >= 0)  *tpl2_log_f++ = *tpl1_log_f++;
      }
    else {
      tpl2->log_f_valid = 0;
      }

    tpl2->likelihood = tpl1->likelihood;
    tpl2->start      = tpl1->start;
    tpl2->ntips      = tpl1->ntips;
    tpl2->nextnode   = tpl1->nextnode;
    tpl2->opt_level  = tpl1->opt_level;
    tpl2->prelabeled = tpl1->prelabeled;
    tpl2->scrNum     = tpl1->scrNum;
    tpl2->tplNum     = tpl1->tplNum;
    tpl2->smoothed   = tpl1->smoothed;
  } /* copyTopol */


boolean restoreTree (topol *tpl, tree *tr)
  { /* restoreTree */
    void  hookup();
    boolean  initrav();

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
    tr->prelabeled = tpl->prelabeled;
    tr->smoothed   = tpl->smoothed;

    if (tpl_log_f = tpl->log_f) {
      tr_log_f = tr->log_f;
      i = tr->log_f_valid = tpl->log_f_valid;
      while (--i >= 0)  *tr_log_f++ = *tpl_log_f++;
      }
    else {
      tr->log_f_valid = 0;
      }

    return (initrav(tr, tr->start) && initrav(tr, tr->start->back));
  } /* restoreTree */


int initBestTree (bestlist *bt, int newkeep, int numsp, int sites)
  { /* initBestTree */
    int  i, nlogf;


    bt->nkeep = 0;

    if (bt->ninit <= 0) {
      if (! (bt->start = setupTopol(numsp, sites)))  return  0;
      bt->ninit = -1;
      bt->nvalid = 0;
      bt->numtrees = 0;
      bt->best = unlikely;
      bt->improved = FALSE;
      bt->byScore = (topol **) Malloc((newkeep+1) * sizeof(topol *));
      bt->byTopol = (topol **) Malloc((newkeep+1) * sizeof(topol *));
      if (! bt->byScore || ! bt->byTopol) {
        fprintf(stderr, "initBestTree: Malloc failure\n");
        return 0;
        }
      }
    else if (ABS(newkeep) > bt->ninit) {
      if (newkeep <  0) newkeep = -(bt->ninit);
      else newkeep = bt->ninit;
      }

    if (newkeep < 1) {    /*  Use negative newkeep to clear list  */
      newkeep = -newkeep;
      if (newkeep < 1) newkeep = 1;
      bt->nvalid = 0;
      bt->best = unlikely;
      }

    if (bt->nvalid >= newkeep) {
      bt->nvalid = newkeep;
      bt->worst = bt->byScore[newkeep]->likelihood;
      }
    else {
      bt->worst = unlikely;
      }

    for (i = bt->ninit + 1; i <= newkeep; i++) {
      nlogf = (i <= maxlogf) ? sites : 0;
      if (! (bt->byScore[i] = setupTopol(numsp, nlogf)))  break;
      bt->byTopol[i] = bt->byScore[i];
      bt->ninit = i;
      }

    return  (bt->nkeep = MIN(newkeep, bt->ninit));
  } /* initBestTree */



int resetBestTree (bestlist *bt)
  { /* resetBestTree */
    bt->best     = unlikely;
    bt->worst    = unlikely;
    bt->nvalid   = 0;
    bt->improved = FALSE;
  } /* resetBestTree */


boolean  freeBestTree(bestlist *bt)
  { /* freeBestTree */
    while (bt->ninit >= 0)  freeTopol(bt->byScore[(bt->ninit)--]);
    freeTopol(bt->start);
    return TRUE;
  } /* freeBestTree */


/*  Compare two trees, assuming that each is in standard order.  Return
 *  -1 if first preceeds second, 0 if they are identical, or +1 if first
 *  follows second in standard order.  Lower number tips preceed higher
 *  number tips.  A tip preceeds a corresponding internal node.  Internal
 *  nodes are ranked by their lowest number tip.
 */

int  cmpSubtopol (connptr p10, connptr p1, connptr p20, connptr p2)
  { /* cmpSubtopol */
    connptr  p1d, p2d;
    int  cmp;

    if (! p1->descend && ! p2->descend)          /* Two tips */
      return cmpTipVal(p1->valptr, p2->valptr);

    if (! p1->descend) return -1;                /* p1 = tip, p2 = node */
    if (! p2->descend) return  1;                /* p2 = tip, p1 = node */

    p1d = p10 + p1->descend;
    p2d = p20 + p2->descend;
    while (1) {                                  /* Two nodes */
      if (cmp = cmpSubtopol(p10, p1d, p20, p2d))  return cmp; /* Subtrees */
      if (! p1d->sibling && ! p2d->sibling)  return  0; /* Lists done */
      if (! p1d->sibling) return -1;             /* One done, other not */
      if (! p2d->sibling) return  1;             /* One done, other not */
      p1d = p10 + p1d->sibling;                  /* Neither done */
      p2d = p20 + p2d->sibling;
      }
  } /* cmpSubtopol */



int  cmpTopol (void *tpl1, void *tpl2)
  { /* cmpTopol */
    connptr  r1, r2;
    int      cmp;

    r1 = ((topol *) tpl1)->links;
    r2 = ((topol *) tpl2)->links;
    cmp = cmpTipVal(tipValPtr(r1->p), tipValPtr(r2->p));
    if (cmp) return cmp;
    return  cmpSubtopol(r1, r1, r2, r2);
  } /* cmpTopol */



int  cmpTplScore (void *tpl1, void *tpl2)
  { /* cmpTplScore */
    double  l1, l2;

    l1 = ((topol *) tpl1)->likelihood;
    l2 = ((topol *) tpl2)->likelihood;
    return  (l1 > l2) ? -1 : ((l1 == l2) ? 0 : 1);
  } /* cmpTplScore */



/*  Find an item in a sorted list of n items.  If the item is in the list,
 *  return its index.  If it is not in the list, return the negative of the
 *  position into which it should be inserted.
 */

int  findInList (void *item, void *list[], int n, int (* cmpFunc)())
  { /* findInList */
    int  mid, hi, lo, cmp;

    if (n < 1) return  -1;                    /*  No match; first index  */

    lo = 1;
    mid = 0;
    hi = n;
    while (lo < hi) {
      mid = (lo + hi) >> 1;
      cmp = (* cmpFunc)(item, list[mid-1]);
      if (cmp) {
        if (cmp < 0) hi = mid;
        else lo = mid + 1;
        }
      else  return  mid;                        /*  Exact match  */
      }

    if (lo != mid) {
       cmp = (* cmpFunc)(item, list[lo-1]);
       if (cmp == 0) return lo;
       }
    if (cmp > 0) lo++;                         /*  Result of step = 0 test  */
    return  -lo;
  } /* findInList */



int  findTreeInList (bestlist *bt, tree *tr)
  { /* findTreeInList */
    topol  *tpl;

    tpl = bt->byScore[0];
    saveTree(tr, tpl);
    return  findInList((void *) tpl, (void **) (& (bt->byTopol[1])),
                       bt->nvalid, cmpTopol);
  } /* findTreeInList */


int  saveBestTree (bestlist *bt, tree *tr)
  { /* saveBestTree */
    double *tr_log_f, *tpl_log_f;
    topol  *tpl, *reuse;
    int  tplNum, scrNum, reuseScrNum, reuseTplNum, i, oldValid, newValid;

    tplNum = findTreeInList(bt, tr);
    tpl = bt->byScore[0];
    oldValid = newValid = bt->nvalid;

    if (tplNum > 0) {                      /* Topology is in list  */
      reuse = bt->byTopol[tplNum];         /* Matching topol  */
      reuseScrNum = reuse->scrNum;
      reuseTplNum = reuse->tplNum;
      }
                                           /* Good enough to keep? */
    else if (tr->likelihood < bt->worst)  return 0;

    else {                                 /* Topology is not in list */
      tplNum = -tplNum;                    /* Add to list (not replace) */
      if (newValid < bt->nkeep) bt->nvalid = ++newValid;
      reuseScrNum = newValid;              /* Take worst tree */
      reuse = bt->byScore[reuseScrNum];
      reuseTplNum = (newValid > oldValid) ? newValid : reuse->tplNum;
      if (tr->likelihood > bt->start->likelihood) bt->improved = TRUE;
      }

    scrNum = findInList((void *) tpl, (void **) (& (bt->byScore[1])),
                         oldValid, cmpTplScore);
    scrNum = ABS(scrNum);

    if (scrNum < reuseScrNum)
      for (i = reuseScrNum; i > scrNum; i--)
        (bt->byScore[i] = bt->byScore[i-1])->scrNum = i;

    else if (scrNum > reuseScrNum) {
      scrNum--;
      for (i = reuseScrNum; i < scrNum; i++)
        (bt->byScore[i] = bt->byScore[i+1])->scrNum = i;
      }

    if (tplNum < reuseTplNum)
      for (i = reuseTplNum; i > tplNum; i--)
        (bt->byTopol[i] = bt->byTopol[i-1])->tplNum = i;

    else if (tplNum > reuseTplNum) {
      tplNum--;
      for (i = reuseTplNum; i < tplNum; i++)
        (bt->byTopol[i] = bt->byTopol[i+1])->tplNum = i;
      }

    if (tpl_log_f = tpl->log_f) {
      tr_log_f  = tr->log_f;
      i = tpl->log_f_valid = tr->log_f_valid;
      while (--i >= 0)  *tpl_log_f++ = *tr_log_f++;
      }
    else {
      tpl->log_f_valid = 0;
      }

    tpl->scrNum = scrNum;
    tpl->tplNum = tplNum;
    bt->byTopol[tplNum] = bt->byScore[scrNum] = tpl;
    bt->byScore[0] = reuse;

    if (scrNum == 1)  bt->best = tr->likelihood;
    if (newValid == bt->nkeep) bt->worst = bt->byScore[newValid]->likelihood;

    return  scrNum;
  } /* saveBestTree */


int  startOpt (bestlist *bt, tree *tr)
  { /* startOpt */
    int  scrNum;

    scrNum = saveBestTree(bt, tr);
    copyTopol(bt->byScore[scrNum], bt->start);
    bt->improved = FALSE;
    return  scrNum;
  } /* startOpt */


int  setOptLevel (bestlist *bt, int opt_level)
  { /* setOptLevel */
    int  tplNum, scrNum;

    tplNum = findInList((void *) bt->start, (void **) (&(bt->byTopol[1])),
                        bt->nvalid, cmpTopol);
    if (tplNum > 0) {
      bt->byTopol[tplNum]->opt_level = opt_level;
      scrNum = bt->byTopol[tplNum]->scrNum;
      }
    else {
      scrNum = 0;
      }

    return  scrNum;
  } /* setOptLevel */


int  recallBestTree (bestlist *bt, int rank, tree *tr)
  { /* recallBestTree */
    if (rank < 1)  rank = 1;
    if (rank > bt->nvalid)  rank = bt->nvalid;
    if (rank > 0)  if (! restoreTree(bt->byScore[rank], tr)) return FALSE;
    return  rank;
  } /* recallBestTree */


/*=======================================================================*/
/*                       End of best tree routines                       */
/*=======================================================================*/


#if 0
  void hang(char *msg)
   { printf("Hanging around: %s\n", msg);
     while(1);
     }
#endif


boolean getnums (rawdata *rdta)
    /* input number of species, number of sites */
  { /* getnums */
    printf("\n%s, version %s, %s,\nCopyright (C) 2002 by Alexandros P. Stamatakis\n\n",
            programName,
            programVersion,
            programDate);
    printf("Based in part on Joseph Felsenstein's\n\n");
    printf("   Nucleic acid sequence Maximum Likelihood method, version 3.3\n\n\n");

    if (fscanf(INFILE, "%d %d", & rdta->numsp, & rdta->sites) != 2) {
      printf("ERROR: Problem reading number of species and sites\n");
      return FALSE;
      }
    printf("%d Species, %d Sites\n\n", rdta->numsp, rdta->sites);

    if (rdta->numsp < 4) {
      printf("TOO FEW SPECIES\n");
      return FALSE;
      }

    if (rdta->sites < 1) {
      printf("TOO FEW SITES\n");
      return FALSE;
      }

    return TRUE;
  } /* getnums */


boolean digitchar (int ch) {return (ch >= '0' && ch <= '9'); }


boolean whitechar (int ch)
   { return (ch == ' ' || ch == '\n' || ch == '\t');
     }


void uppercase (int *chptr)
    /* convert character to upper case -- either ASCII or EBCDIC */
  { /* uppercase */
    int  ch;

    ch = *chptr;
    if ((ch >= 'a' && ch <= 'i') || (ch >= 'j' && ch <= 'r')
                                 || (ch >= 's' && ch <= 'z'))
      *chptr = ch + 'A' - 'a';
  } /* uppercase */


int base36 (int ch)
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


int itobase36 (int i)
  { /* itobase36 */
    if      (i <  0) return '?';
    else if (i < 10) return (i      + '0');
    else if (i < 19) return (i - 10 + 'A');
    else if (i < 28) return (i - 19 + 'J');
    else if (i < 36) return (i - 28 + 'S');
    else return '?';
  } /* itobase36 */


int findch (int c)
  { /* findch */
    int ch;

    while ((ch = getc(INFILE)) != EOF && ch != c) ;
    return  ch;
  } /* findch */


#if Master || Slave
int str_findch (char **treestrp, int c)
  { /* str_findch */
    int ch;

    while ((ch = *(*treestrp)++) != NULL && ch != c) ;
    return  ch;
  } /* str_findch */
#endif


boolean inputboot(analdef *adef)
    /* read the bootstrap auxilliary info */
  { /* inputboot */
    if (! adef->boot) {
      printf("ERROR: Unexpected Bootstrap auxiliary data line\n");
      return FALSE;
      }
    else if (fscanf(INFILE, "%ld", & adef->boot) != 1 ||
             findch('\n') == EOF) {
      printf("ERROR: Problem reading boostrap random seed value\n");
      return FALSE;
      }

    return TRUE;
  } /* inputboot */


boolean inputcategories (rawdata *rdta)
    /* read the category rates and the categories for each site */
  { /* inputcategories */
    int  i, j, ch, ci;

    if (rdta->categs >= 0) {
      printf("ERROR: Unexpected Categories auxiliary data line\n");
      return FALSE;
      }
    if (fscanf(INFILE, "%d", & rdta->categs) != 1) {
      printf("ERROR: Problem reading number of rate categories\n");
      return FALSE;
      }
    if (rdta->categs < 1 || rdta->categs > maxcategories) {
      printf("ERROR: Bad number of categories: %d\n", rdta->categs);
      printf("Must be in range 1 - %d\n", maxcategories);
      return FALSE;
      }

    for (j = 1; j <= rdta->categs
           && fscanf(INFILE, "%lf", &(rdta->catrat[j])) == 1; j++) ;

    if ((j <= rdta->categs) || (findch('\n') == EOF)) {
      printf("ERROR: Problem reading rate values\n");
      return FALSE;
      }

    for (i = 1; i <= nmlngth; i++)  (void) getc(INFILE);

    i = 1;
    while (i <= rdta->sites) {
      ch = getc(INFILE);
      ci = base36(ch);
      if (ci >= 0 && ci <= rdta->categs)
        rdta->sitecat[i++] = ci;
      else if (! whitechar(ch)) {
        printf("ERROR: Bad category character (%c) at site %d\n", ch, i);
        return FALSE;
        }
      }

    if (findch('\n') == EOF) {      /* skip to end of line */
      printf("ERROR: Missing newline at end of category data\n");
      return FALSE;
      }

    return TRUE;
  } /* inputcategories */


boolean inputextra (analdef *adef)
  { /* inputextra */
    if (fscanf(INFILE,"%d", & adef->extra) != 1 ||
        findch('\n') == EOF) {
      printf("ERROR: Problem reading extra info value\n");
      return FALSE;
      }

    return TRUE;
  } /* inputextra */


boolean inputfreqs (rawdata *rdta)
  { /* inputfreqs */
    if (fscanf(INFILE, "%lf%lf%lf%lf",
               & rdta->freqa, & rdta->freqc,
               & rdta->freqg, & rdta->freqt) != 4 ||
        findch('\n') == EOF) {
      printf("ERROR: Problem reading user base frequencies data\n");
      return  FALSE;
      }

    rdta->freqread = TRUE;
    return TRUE;
  } /* inputfreqs */


boolean inputglobal (tree *tr)
    /* input the global option information */
  { /* inputglobal */
    int  ch;

    if (tr->global != -2) {
      printf("ERROR: Unexpected Global auxiliary data line\n");
      return FALSE;
      }
    if (fscanf(INFILE, "%d", &(tr->global)) != 1) {
      printf("ERROR: Problem reading rearrangement region size\n");
      return FALSE;
      }
    if (tr->global < 0) {
      printf("WARNING: Global region size too small;\n");
      printf("         value reset to local\n\n");
      tr->global = 1;
      }
    else if (tr->global == 0)  tr->partswap = 0;
    else if (tr->global > tr->mxtips - 3) {
      tr->global = tr->mxtips - 3;
      }

    while ((ch = getc(INFILE)) != '\n') {  /* Scan for second value */
      if (! whitechar(ch)) {
        if (ch != EOF)  (void) ungetc(ch, INFILE);
        if (ch == EOF || fscanf(INFILE, "%d", &(tr->partswap)) != 1
                      || findch('\n') == EOF) {
          printf("ERROR: Problem reading insert swap region size\n");
          return FALSE;
          }
        else if (tr->partswap < 0)  tr->partswap = 1;
        else if (tr->partswap > tr->mxtips - 3) {
          tr->partswap = tr->mxtips - 3;
          }

        if (tr->partswap > tr->global)  tr->global = tr->partswap;
        break;   /*  Break while loop */
        }
      }

    return TRUE;
  } /* inputglobal */


boolean inputjumble (analdef *adef)
  { /* inputjumble */
    if (! adef->jumble) {
      printf("ERROR: Unexpected Jumble auxiliary data line\n");
      return FALSE;
      }
    else if (fscanf(INFILE, "%ld", & adef->jumble) != 1 ||
             findch('\n') == EOF) {
      printf("ERROR: Problem reading jumble random seed value\n");
      return FALSE;
      }
    else if (adef->jumble == 0) {
      printf("WARNING: Jumble random number seed is zero\n\n");
      }

    return TRUE;
  } /* inputjumble */


boolean inputkeep (analdef *adef)
  { /* inputkeep */
    if (fscanf(INFILE, "%d", & adef->nkeep) != 1 ||
        findch('\n') == EOF || adef->nkeep < 1) {
      printf("ERROR: Problem reading number of kept trees\n");
      return FALSE;
      }

    return TRUE;
  } /* inputkeep */


boolean inputoutgroup (analdef *adef, tree *tr)
  { /* inputoutgroup */
    if (! adef->root || tr->outgr > 0) {
      printf("ERROR: Unexpected Outgroup auxiliary data line\n");
      return FALSE;
      }
    else if (fscanf(INFILE, "%d", &(tr->outgr)) != 1 ||
             findch('\n') == EOF) {
      printf("ERROR: Problem reading outgroup number\n");
      return FALSE;
      }
    else if ((tr->outgr < 1) || (tr->outgr > tr->mxtips)) {
      printf("ERROR: Bad outgroup: '%d'\n", tr->outgr);
      return FALSE;
      }

    return TRUE;
  } /* inputoutgroup */


boolean inputratio (rawdata *rdta)
  { /* inputratio */
    if (rdta->ttratio >= 0.0) {
      printf("ERROR: Unexpected Transition/transversion auxiliary data\n");
      return FALSE;
      }
    else if (fscanf(INFILE,"%lf", & rdta->ttratio)!=1 ||
             findch('\n') == EOF) {
      printf("ERROR: Problem reading transition/transversion ratio\n");
      return FALSE;
      }

    return TRUE;
  } /* inputratio */


/*  Y 0 is treeNone   (no tree)
    Y 1 is treeNewick
    Y 2 is treeProlog
    Y 3 is treePHYLIP
 */

boolean inputtreeopt (analdef *adef)
  { /* inputtreeopt */
    if (! adef->trout) {
      printf("ERROR: Unexpected Treefile auxiliary data\n");
      return FALSE;
      }
    else if (fscanf(INFILE,"%d", & adef->trout) != 1 ||
             findch('\n') == EOF) {
      printf("ERROR: Problem reading output tree-type number\n");
      return FALSE;
      }
    else if ((adef->trout < 0) || (adef->trout > treeMaxType)) {
      printf("ERROR: Bad output tree-type number: '%d'\n", adef->trout);
      return FALSE;
      }

    return TRUE;
  } /* inputtreeopt */


boolean inputweights (analdef *adef, rawdata *rdta, cruncheddata *cdta)
    /* input the character weights 0, 1, 2 ... 9, A, B, ... Y, Z */
  { /* inputweights */
    int i, ch, wi;

    if (! adef->userwgt || cdta->wgtsum > 0) {
      printf("ERROR: Unexpected Weights auxiliary data\n");
      return FALSE;
      }

    for (i = 2; i <= nmlngth; i++)  (void) getc(INFILE);
    cdta->wgtsum = 0;
    i = 1;
    while (i <= rdta->sites) {
      ch = getc(INFILE);
      wi = base36(ch);
      if (wi >= 0)
        cdta->wgtsum += rdta->wgt[i++] = wi;
      else if (! whitechar(ch)) {
        printf("ERROR: Bad weight character: '%c'", ch);
        printf("       Weights in dnaml must be a digit or a letter.\n");
        return FALSE;
        }
      }

    if (findch('\n') == EOF) {      /* skip to end of line */
      printf("ERROR: Missing newline at end of weight data\n");
      return FALSE;
      }

    return TRUE;
  } /* inputweights */

boolean inputdepth (tree *tr)
  { 
    if (fscanf(INFILE, "%d", & tr->eqDepth) != 1 ||
        findch('\n') == EOF || tr->eqDepth < 0) {
      printf("Depth (H - option) has to be greater than 0 \n ");
      
      return FALSE;
      }
    fprintf(stderr,"set to: %d\n",  tr->eqDepth);
    return TRUE;
  } 


boolean getoptions (analdef *adef, rawdata *rdta, cruncheddata *cdta, tree *tr)
  { /* getoptions */
    int     ch, i, extranum;

    adef->boot    =           0;  /* Don't bootstrap column weights */
    adef->empf    =        TRUE;  /* Use empirical base frequencies */
    adef->extra   =           0;  /* No extra runtime info unless requested */
    adef->interleaved =    TRUE;  /* By default, data format is interleaved */
    adef->jumble  =       FALSE;  /* Use random addition sequence */
    adef->nkeep   =           0;  /* Keep only the one best tree */
    adef->prdata  =       FALSE;  /* Don't echo data to output stream */
    adef->qadd    =        TRUE;  /* Smooth branches globally in add */
    adef->restart =       FALSE;  /* Restart from user tree */
    adef->root    =       FALSE;  /* User-defined outgroup rooting */
    adef->trout   = treeDefType;  /* Output tree file */
    adef->trprint =        TRUE;  /* Print tree to output stream */
    rdta->categs  =           0;  /* No rate categories */
    rdta->catrat[1] =       1.0;  /* Rate values */
    rdta->freqread =      FALSE;  /* User-defined frequencies not read yet */
    rdta->ttratio =         2.0;  /* Transition/transversion rate ratio */
    tr->global    =          -1;  /* Default search locale for optimum */
    tr->mxtips    = rdta->numsp;
    tr->outgr     =           1;  /* Outgroup number */
    tr->partswap  =           1;  /* Default to swap locally after insert */
    tr->userlen   =       FALSE;  /* User-supplied branch lengths */
    tr->eqDepth   =           0;
    adef->usertree =      FALSE;  /* User-defined tree topologies */
    adef->userwgt =       FALSE;  /* User-defined position weights */
    extranum      =           0;

    while ((ch = getc(INFILE)) != '\n' && ch != EOF) {
      uppercase(& ch);
      switch (ch) {
          case '1' : adef->prdata  = ! adef->prdata; break;
          case '3' : adef->trprint = ! adef->trprint; break;
          case '4' : adef->trout   = treeDefType - adef->trout; break;
          case 'B' : adef->boot    =  1; extranum++; break;
          case 'C' : rdta->categs  = -1; extranum++; break;
          case 'E' : adef->extra   = -1; break;
          case 'F' : adef->empf    = ! adef->empf; break;
          case 'G' : tr->global    = -2; break;
	    //stm
      case 'H' : extranum++;break;  
	    //stm


          case 'I' : adef->interleaved = ! adef->interleaved; break;
          case 'J' : adef->jumble  = 1; extranum++; break;
          case 'K' : extranum++; break;
          case 'L' : tr->userlen   = TRUE; break;
          case 'O' : adef->root    = TRUE; tr->outgr = 0; extranum++; break;
          case 'Q' : adef->qadd    = FALSE; break;
          case 'R' : adef->restart = TRUE; break;
          case 'T' : rdta->ttratio = -1.0; extranum++; break;
          case 'U' : adef->usertree = TRUE; break;
          case 'W' : adef->userwgt = TRUE; cdta->wgtsum = 0; extranum++; break;
          case 'Y' : adef->trout   = treeDefType - adef->trout; break;
          case ' ' : break;
          case '\t': break;
          default  :
              printf("ERROR: Bad option character: '%c'\n", ch);
              return FALSE;
          }
      }

    if (ch == EOF) {
      printf("ERROR: End-of-file in options list\n");
      return FALSE;
      }

    if (adef->usertree && adef->restart) {
      printf("ERROR:  The restart and user-tree options conflict:\n");
      printf("        Restart adds rest of taxa to a starting tree;\n");
      printf("        User-tree does not add any taxa.\n\n");
      return FALSE;
      }

    if (adef->usertree && adef->jumble) {
      printf("WARNING:  The jumble and user-tree options conflict:\n");
      printf("          Jumble adds taxa to a tree in random order;\n");
      printf("          User-tree does not use taxa addition.\n");
      printf("          Jumble option cancelled for this run.\n\n");
      adef->jumble = FALSE;
      }

    if (tr->userlen && tr->global != -1) {
      printf("ERROR:  The global and user-lengths options conflict:\n");
      printf("        Global optimizes a starting tree;\n");
      printf("        User-lengths constrain the starting tree.\n\n");
      return FALSE;
      }

    if (tr->userlen && ! adef->usertree) {
      printf("WARNING:  User lengths required user tree option.\n");
      printf("          User-tree option set for this run.\n\n");
      adef->usertree = TRUE;
      }

    rdta->wgt      = (int *)    Malloc((rdta->sites + 1) * sizeof(int));
    rdta->wgt2     = (int *)    Malloc((rdta->sites + 1) * sizeof(int));
    rdta->sitecat  = (int *)    Malloc((rdta->sites + 1) * sizeof(int));
    cdta->alias    = (int *)    Malloc((rdta->sites + 1) * sizeof(int));
    cdta->aliaswgt = (int *)    Malloc((rdta->sites + 1) * sizeof(int));
    cdta->patcat   = (int *)    Malloc((rdta->sites + 1) * sizeof(int));
    cdta->patrat   = (double *) Malloc((rdta->sites + 1) * sizeof(double));
    cdta->wr       = (double *) Malloc((rdta->sites + 1) * sizeof(double));
    cdta->wr2      = (double *) Malloc((rdta->sites + 1) * sizeof(double));
    if ( ! rdta->wgt || ! rdta->wgt2     || ! rdta->sitecat || ! cdta->alias
                     || ! cdta->aliaswgt || ! cdta->patcat  || ! cdta->patrat
                     || ! cdta->wr       || ! cdta->wr2) {
      fprintf(stderr, "getoptions: Malloc failure\n");
      return 0;
      }

    /*  process lines with auxiliary data */

    while (extranum--) {
      ch = getc(INFILE);
      uppercase(& ch);
      switch (ch) {
        case 'B':  if (! inputboot(adef)) return FALSE; break;
        case 'C':  if (! inputcategories(rdta)) return FALSE; break;
        case 'E':  if (! inputextra(adef)) return FALSE; extranum++; break;
        case 'F':  if (! inputfreqs(rdta)) return FALSE; break;
        case 'G':  if (! inputglobal(tr)) return FALSE; extranum++; break;
	  //stm
      case 'H':    if(!inputdepth(tr)) return FALSE; break;
	//stm
        case 'J':  if (! inputjumble(adef)) return FALSE; break;
        case 'K':  if (! inputkeep(adef)) return FALSE; break;
        case 'O':  if (! inputoutgroup(adef, tr)) return FALSE; break;
        case 'T':  if (! inputratio(rdta)) return FALSE; break;
        case 'W':  if (! inputweights(adef, rdta, cdta)) return FALSE; break;
        case 'Y':  if (! inputtreeopt(adef)) return FALSE; extranum++; break;

        default:
            printf("ERROR: Auxiliary options line starts with '%c'\n", ch);
            return FALSE;
        }
      }

    if (! adef->userwgt) {
      for (i = 1; i <= rdta->sites; i++) rdta->wgt[i] = 1;
      cdta->wgtsum = rdta->sites;
      }

    if (adef->userwgt && cdta->wgtsum < 1) {
      printf("ERROR:  Missing or bad user-supplied weight data.\n");
      return FALSE;
      }

    if (adef->boot) {
      printf("Bootstrap random number seed = %ld\n\n", adef->boot);
      }

    if (adef->jumble) {
      printf("Jumble random number seed = %ld\n\n", adef->jumble);
      }

    if (adef->qadd) {
      printf("Quick add (only local branches initially optimized) in effect\n\n");
      }

    if (rdta->categs > 0) {
      printf("Site category   Rate of change\n\n");
      for (i = 1; i <= rdta->categs; i++)
        printf("           %c%13.3f\n", itobase36(i), rdta->catrat[i]);
      putchar('\n');
      for (i = 1; i <= rdta->sites; i++) {
        if ((rdta->wgt[i] > 0) && (rdta->sitecat[i] < 1)) {
          printf("ERROR: Bad category (%c) at site %d\n",
                  itobase36(rdta->sitecat[i]), i);
          return FALSE;
          }
        }
      }
    else if (rdta->categs < 0) {
      printf("ERROR: Category auxiliary data missing from input\n");
      return FALSE;
      }
    else {                                        /* rdta->categs == 0 */
      for (i = 1; i <= rdta->sites; i++) rdta->sitecat[i] = 1;
      rdta->categs = 1;
      }

    if (tr->outgr < 1) {
      printf("ERROR: Outgroup auxiliary data missing from input\n");
      return FALSE;
      }

    if (rdta->ttratio < 0.0) {
      printf("ERROR: Transition/transversion auxiliary data missing from input\n");
      return FALSE;
      }

    if (tr->global < 0) {
      if (tr->global == -2) tr->global = tr->mxtips - 3;  /* Default global */
      else                  tr->global = adef->usertree ? 0 : 1;/* No global */
      }

    if (adef->restart) {
      printf("Restart option in effect.  ");
      printf("Sequence addition will start from appended tree.\n\n");
      }

    if (adef->usertree && ! tr->global) {
      printf("User-supplied tree topology%swill be used.\n\n",
        tr->userlen ? " and branch lengths " : " ");
      }
    else {
      if (! adef->usertree) {
        printf("Rearrangements of partial trees may cross %d %s.\n",
               tr->partswap, tr->partswap == 1 ? "branch" : "branches");
        }
      printf("Rearrangements of full tree may cross %d %s.\n\n",
             tr->global, tr->global == 1 ? "branch" : "branches");
      }

    if (! adef->usertree && adef->nkeep == 0) adef->nkeep = 1;

    return TRUE;
  } /* getoptions */


boolean getbasefreqs (rawdata *rdta)
  { /* getbasefreqs */
    int  ch;

    if (rdta->freqread) return TRUE;

    ch = getc(INFILE);
    if (! ((ch == 'F') || (ch == 'f')))  (void) ungetc(ch, INFILE);

    if (fscanf(INFILE, "%lf%lf%lf%lf",
               & rdta->freqa, & rdta->freqc,
               & rdta->freqg, & rdta->freqt) != 4 ||
        findch('\n') == EOF) {
      printf("ERROR: Problem reading user base frequencies\n");
      return  FALSE;
      }

    return TRUE;
  } /* getbasefreqs */


boolean getyspace (rawdata *rdta)
  { /* getyspace */
    long   size;
    int    i;
    yType *y0;

    if (! (rdta->y = (yType **) Malloc((rdta->numsp + 1) * sizeof(yType *)))) {
      printf("ERROR: Unable to obtain space for data array pointers\n");
      return  FALSE;
      }

    size = 4 * (rdta->sites / 4 + 1);
    if (! (y0 = (yType *) Malloc((rdta->numsp + 1) * size * sizeof(yType)))) {
      printf("ERROR: Unable to obtain space for data array\n");
      return  FALSE;
      }

    for (i = 0; i <= rdta->numsp; i++) {
      rdta->y[i] = y0;
      y0 += size;
      }

    return  TRUE;
  } /* getyspace */



boolean setupTree (tree *tr, int nsites)
  { /* setupTree */
    nodeptr  p0, p, q;
    int      i, j, tips, inter;

    tips  = tr->mxtips;
    inter = tr->mxtips - 1;

    if (!(p0 = (nodeptr) Malloc((tips + 3*inter) * sizeof(node)))) {
      printf("ERROR: Unable to obtain sufficient tree memory\n");
      return  FALSE;
      }


    if (!(tr->nodep = (nodeptr *) Malloc((2*tr->mxtips) * sizeof(nodeptr)))) {
      printf("ERROR: Unable to obtain sufficient tree memory, too\n");
      return  FALSE;
      }

    
    tr->nodep[0] = (node *) NULL;    /* Use as 1-based array */

    for (i = 1; i <= tips; i++) {    /* Set-up tips */
      p = p0++;
      p->x      = (xarray *) NULL;
      p->tip    = (yType *) NULL;
      
      /* AxML modification start*/

      p->equalityVector = (char *) NULL; /*initialize eq vector with zero */
      
      /* AxML modification end*/


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
        p->tip    = (yType *) NULL;
        p->number = i;

	/* AxML modification start*/

	p->equalityVector = (char *) NULL; /*initialize eq vector with zero */	
	/* AxML modification end*/


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
    tr->prelabeled  = TRUE;
    tr->smoothed    = FALSE;
    tr->log_f_valid = 0;

    tr->log_f = (double *) Malloc(nsites * sizeof(double));
    if (! tr->log_f) {
      printf("ERROR: Unable to obtain sufficient tree memory, trey\n");
      return  FALSE;
      }

    return TRUE;
  } /* setupTree */

void freeTreeNode (nodeptr p)   /* Free tree node (sector) associated data */
  { /* freeTreeNode */
    if (p) {
      if (p->x) {
        if (p->x->lv) Free(p->x->lv);
        Free(p->x);
        }
      /* AxML modification start*/
 
      Free(p->equalityVector); /*free equality vector of node*/
      
      /* AxML modification end*/
      }
  } /* freeTree */



/* AxML modification start*/


boolean setupEqualityVector(tree *tr) /* new function for initializing and mapping equality vectors */
{
  int siteNum = tr->cdta->endsite;
  nodeptr upper = tr->nodep[tr->mxtips * 2 - 1];
  nodeptr p = tr->nodep[1];
  char *ev, *end;
  char *tip;   
 
  likelivector *l;
  int i, j;
  homType *mlvP;
  int ref[EQUALITIES];
  int code;
  
  for(; p < upper; p++)
    {      
      if( ! (p->equalityVector = 
	     (ev = (char *)malloc(siteNum * sizeof(char)))))
	{
	  printf("ERROR: Failure to get node equality vector  memory\n");
	  return  FALSE; 
	}      
	 
      end = & ev[siteNum];
      
      if(p->tip)
	{	 
	  int i;
	  tip = p->tip;	 
	
	  for(i = 0; i < equalities; i++)
	    ref[i] = -1;

	  for(i = 0; i < siteNum; i++)
	    {	    
	      ev[i] = map[tip[i]];  	  	     	      
	      if(ref[ev[i]] == -1)
		ref[ev[i]] = 1;
	    }
	  for(i = 0; i < equalities; i++)
	    {	      
	      for(j = 1; j <= tr->rdta->categs; j++)
		{
		  mlvP = &p->mlv[i][j];
		  code = map2[i];
		  //  if(ref[i] == 1)
		  //{
		  mlvP->a =  code       & 1;
		  mlvP->c = (code >> 1) & 1; 	
		  mlvP->g = (code >> 2) & 1;
		  mlvP->t = (code >> 3) & 1;
		  mlvP->exp = 0;
		  mlvP->set = TRUE;
		  //}
		  //else
		  //mlvP->set = FALSE;
		 	 
		}
	    }      
	    
	}
      else
	{	 	
	  for(; ev < end; ev++)
	    {
	      *ev = 0;	     
	    }
	}
    } 
  
  return TRUE;
}

  /* AxML modification end*/







void freeTree (tree *tr)
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

    Free(tr->nodep[1]);       /* Free the actual nodes */
  } /* freeTree */


boolean getdata (analdef *adef, rawdata *rdta, tree *tr)
    /* read sequences */
  { /* getdata */
    int   i, j, k, l, basesread, basesnew, ch;
    int   meaning[256];          /*  meaning of input characters */ 
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
          while (whitechar(ch = getc(INFILE))) {  /*  Skip blank lines */
            if (ch == '\n')  j = 1;  else  j++;
            }

          if (j > nmlngth) {
            printf("ERROR: Blank name for species %d; ", i);
            printf("check number of species,\n");
            printf("       number of sites, and interleave option.\n");
            return  FALSE;
            }

          nameptr = tr->nodep[i]->name;
          for (k = 1; k < j; k++)  *nameptr++ = ' ';

          while (ch != '\n' && ch != EOF) {
            if (whitechar(ch))  ch = ' ';
            *nameptr++ = ch;
            if (++j > nmlngth) break;
            ch = getc(INFILE);
            }

          while (*(--nameptr) == ' ') ;          /*  remove trailing blanks */
          *(++nameptr) = '\0';                   /*  add null termination */

          if (ch == EOF) {
            printf("ERROR: End-of-file in name of species %d\n", i);
            return  FALSE;
            }
          }    /* if (firstpass) */

        j = basesread;
        while ((j < rdta->sites)
               && ((ch = getc(INFILE)) != EOF)
               && ((! adef->interleaved) || (ch != '\n'))) {
          uppercase(& ch);
          if (meaning[ch] || ch == '.') {
            j++;
            if (ch == '.') {
              if (i != 1) ch = rdta->y[1][j];
              else {
                printf("ERROR: Dot (.) found at site %d of sequence 1\n", j);
                return  FALSE;
                }
              }
            rdta->y[i][j] = ch;
            }
          else if (whitechar(ch) || digitchar(ch)) ;
          else {
            printf("ERROR: Bad base (%c) at site %d of sequence %d\n",
                    ch, j, i);
            return  FALSE;
            }
          }

        if (ch == EOF) {
          printf("ERROR: End-of-file at site %d of sequence %d\n", j, i);
          return  FALSE;
          }

        if (! firstpass && (j == basesread)) i--;        /* no data on line */
        else if (i == 1) basesnew = j;
        else if (j != basesnew) {
          printf("ERROR: Sequences out of alignment\n");
          printf("%d (instead of %d) residues read in sequence %d\n",
                  j - basesread, basesnew - basesread, i);
          return  FALSE;
          }

        while (ch != '\n' && ch != EOF) ch = getc(INFILE);  /* flush line */
        }                                                  /* next sequence */
      firstpass = FALSE;
      basesread = basesnew;
      allread = (basesread >= rdta->sites);
      }

    /*  Print listing of sequence alignment */

    if (adef->prdata) {
      j = nmlngth - 5 + ((rdta->sites + ((rdta->sites - 1)/10))/2);
      if (j < nmlngth - 1) j = nmlngth - 1;
      if (j > 37) j = 37;
      printf("Name"); for (i=1;i<=j;i++) putchar(' '); printf("Sequences\n");
      printf("----"); for (i=1;i<=j;i++) putchar(' '); printf("---------\n");
      putchar('\n');

      for (i = 1; i <= rdta->sites; i += 60) {
        l = i + 59;
        if (l > rdta->sites) l = rdta->sites;

        if (adef->userwgt) {
          printf("Weights   ");
          for (j = 11; j <= nmlngth+3; j++) putchar(' ');
          for (k = i; k <= l; k++) {
            putchar(itobase36(rdta->wgt[k]));
            if (((k % 10) == 0) && (k < l)) putchar(' ');
            }
          putchar('\n');
          }

        if (rdta->categs > 1) {
          printf("Categories");
          for (j = 11; j <= nmlngth+3; j++) putchar(' ');
          for (k = i; k <= l; k++) {
            putchar(itobase36(rdta->sitecat[k]));
            if (((k % 10) == 0) && (k < l)) putchar(' ');
            }
          putchar('\n');
          }

        for (j = 1; j <= tr->mxtips; j++) {
          nameptr = tr->nodep[j]->name;
          k = nmlngth+3;
          while (ch = *nameptr++) {putchar(ch); k--;}
          while (--k >= 0) putchar(' ');

          for (k = i; k <= l; k++) {
            ch = rdta->y[j][k];
            if ((j > 1) && (ch == rdta->y[1][k])) ch = '.';
            putchar(ch);
            if (((k % 10) == 0) && (k < l)) putchar(' ');
            }
          putchar('\n');
          }
        putchar('\n');
        }
      }

    for (j = 1; j <= tr->mxtips; j++)    /* Convert characters to meanings */
      for (i = 1; i <= rdta->sites; i++) {
        rdta->y[j][i] = meaning[rdta->y[j][i]];
        }

    return  TRUE;
  } /* getdata */


boolean  getntrees (analdef *adef)
  { /* getntrees */

    if (fscanf(INFILE, "%d", &(adef->numutrees)) != 1 || findch('\n') == EOF) {
      printf("ERROR: Problem reading number of user trees\n");
      return  FALSE;
      }

    if (adef->nkeep == 0) adef->nkeep = adef->numutrees;

    return  TRUE;
  } /* getntrees */


boolean getinput (analdef *adef, rawdata *rdta, cruncheddata *cdta, tree *tr)
  { /* getinput */
    if (! getnums(rdta))                       return FALSE;
    if (! getoptions(adef, rdta, cdta, tr))    return FALSE;
    if (! adef->empf && ! getbasefreqs(rdta))  return FALSE;
    if (! getyspace(rdta))                     return FALSE;
    if (! setupTree(tr, rdta->sites))          return FALSE;
    if (! getdata(adef, rdta, tr))             return FALSE;
    if (adef->usertree && ! getntrees(adef))   return FALSE;

    return TRUE;
  } /* getinput */


void makeboot (analdef *adef, rawdata *rdta, cruncheddata *cdta)
  { /* makeboot */
    int  i, j, nonzero;
    double  randum();

    nonzero = 0;
    for (i = 1; i <= rdta->sites; i++)  if (rdta->wgt[i] > 0) nonzero++;

    for (j = 1; j <= nonzero; j++) cdta->aliaswgt[j] = 0;
    for (j = 1; j <= nonzero; j++)
      cdta->aliaswgt[(int) (nonzero*randum(& adef->boot)) + 1]++;

    j = 0;
    cdta->wgtsum = 0;
    for (i = 1; i <= rdta->sites; i++) {
      if (rdta->wgt[i] > 0)
        cdta->wgtsum += (rdta->wgt2[i] = rdta->wgt[i] * cdta->aliaswgt[++j]);
      else
        rdta->wgt2[i] = 0;
      }
  } /* makeboot */


void sitesort (rawdata *rdta, cruncheddata *cdta)
    /* Shell sort keeping sites with identical residues and weights in
     * the original order (i.e., a stable sort).
     * The index created in cdta->alias is 1 based.  The
     * sitecombcrunch routine packs it to a 0 based index.
     */
  { /* sitesort */
    int  gap, i, j, jj, jg, k, n, nsp;
    int  *index, *category;
    boolean  flip, tied;
    yType  **data;

    index    = cdta->alias;
    category = rdta->sitecat;
    data     = rdta->y;
    n        = rdta->sites;
    nsp      = rdta->numsp;

    for (gap = n / 2; gap > 0; gap /= 2) {
      for (i = gap + 1; i <= n; i++) {
        j = i - gap;

        do {
          jj = index[j];
          jg = index[j+gap];
          flip = (category[jj] >  category[jg]);
          tied = (category[jj] == category[jg]);
          for (k = 1; (k <= nsp) && tied; k++) {
            flip = (data[k][jj] >  data[k][jg]);
            tied = (data[k][jj] == data[k][jg]);
            }
          if (flip) {
            index[j]     = jg;
            index[j+gap] = jj;
            j -= gap;
            }
          } while (flip && (j > 0));

        }  /* for (i ...   */
      }    /* for (gap ... */
  } /* sitesort */


void sitecombcrunch (rawdata *rdta, cruncheddata *cdta)
    /* combine sites that have identical patterns (and nonzero weight) */
  { /* sitecombcrunch */
    int  i, sitei, j, sitej, k;
    boolean  tied;

    i = 0;
    cdta->alias[0] = cdta->alias[1];
    cdta->aliaswgt[0] = 0;

    for (j = 1; j <= rdta->sites; j++) {
      sitei = cdta->alias[i];
      sitej = cdta->alias[j];
      tied = (rdta->sitecat[sitei] == rdta->sitecat[sitej]);

      for (k = 1; tied && (k <= rdta->numsp); k++)
        tied = (rdta->y[k][sitei] == rdta->y[k][sitej]);

      if (tied) {
        cdta->aliaswgt[i] += rdta->wgt2[sitej];
        }
      else {
        if (cdta->aliaswgt[i] > 0) i++;
        cdta->aliaswgt[i] = rdta->wgt2[sitej];
        cdta->alias[i] = sitej;
        }
      }

    cdta->endsite = i;
    if (cdta->aliaswgt[i] > 0) cdta->endsite++;
  } /* sitecombcrunch */


boolean makeweights (analdef *adef, rawdata *rdta, cruncheddata *cdta)
    /* make up weights vector to avoid duplicate computations */
  { /* makeweights */
    int  i;

    if (adef->boot)  makeboot(adef, rdta, cdta);
    else  for (i = 1; i <= rdta->sites; i++)  rdta->wgt2[i] = rdta->wgt[i];

    for (i = 1; i <= rdta->sites; i++)  cdta->alias[i] = i;
    sitesort(rdta, cdta);
    sitecombcrunch(rdta, cdta);

    printf("Total weight of positions in analysis = %d\n", cdta->wgtsum);
    printf("There are %d distinct data patterns (columns)\n\n", cdta->endsite);

    return TRUE;
  } /* makeweights */






boolean makevalues (rawdata *rdta, cruncheddata *cdta)
    /* set up fractional likelihoods at tips */
  { /* makevalues */
    double  temp, wtemp;
    int  i, j;

    for (i = 1; i <= rdta->numsp; i++) {    /* Pack and move tip data */
      for (j = 0; j < cdta->endsite; j++) {
        rdta->y[i-1][j] = rdta->y[i][cdta->alias[j]];
        }
      }

    for (j = 0; j < cdta->endsite; j++) {
      cdta->patcat[j] = i = rdta->sitecat[cdta->alias[j]];
      cdta->patrat[j] = temp = rdta->catrat[i];
      cdta->wr[j]  = wtemp = temp * cdta->aliaswgt[j];
      cdta->wr2[j] = temp * wtemp;
      }

   

    return TRUE;
  } /* makevalues */


boolean empiricalfreqs (rawdata *rdta, cruncheddata *cdta)
    /* Get empirical base frequencies from the data */
  { /* empiricalfreqs */
    double  sum, suma, sumc, sumg, sumt, wj, fa, fc, fg, ft;
    int     i, j, k, code;
    yType  *yptr;

    rdta->freqa = 0.25;
    rdta->freqc = 0.25;
    rdta->freqg = 0.25;
    rdta->freqt = 0.25;
    for (k = 1; k <= 8; k++) {
      suma = 0.0;
      sumc = 0.0;
      sumg = 0.0;
      sumt = 0.0;
      for (i = 0; i < rdta->numsp; i++) {
        yptr = rdta->y[i];
        for (j = 0; j < cdta->endsite; j++) {
          code = *yptr++;
          fa = rdta->freqa * ( code       & 1);
          fc = rdta->freqc * ((code >> 1) & 1);
          fg = rdta->freqg * ((code >> 2) & 1);
          ft = rdta->freqt * ((code >> 3) & 1);
          wj = cdta->aliaswgt[j] / (fa + fc + fg + ft);
          suma += wj * fa;
          sumc += wj * fc;
          sumg += wj * fg;
          sumt += wj * ft;
          }
        }
      sum = suma + sumc + sumg + sumt;
      rdta->freqa = suma / sum;
      rdta->freqc = sumc / sum;
      rdta->freqg = sumg / sum;
      rdta->freqt = sumt / sum;
      }

    return TRUE;
  } /* empiricalfreqs */


void reportfreqs (analdef *adef, rawdata *rdta)
  { /* reportfreqs */
    double  suma, sumb;

    if (adef->empf) printf("Empirical ");
    printf("Base Frequencies:\n\n");
    printf("   A    %10.5f\n",   rdta->freqa);
    printf("   C    %10.5f\n",   rdta->freqc);
    printf("   G    %10.5f\n",   rdta->freqg);
    printf("  T(U)  %10.5f\n\n", rdta->freqt);
    rdta->freqr = rdta->freqa + rdta->freqg;
    rdta->invfreqr = 1.0/rdta->freqr;
    rdta->freqar = rdta->freqa * rdta->invfreqr;
    rdta->freqgr = rdta->freqg * rdta->invfreqr;
    rdta->freqy = rdta->freqc + rdta->freqt;
    rdta->invfreqy = 1.0/rdta->freqy;
    rdta->freqcy = rdta->freqc * rdta->invfreqy;
    rdta->freqty = rdta->freqt * rdta->invfreqy;
    printf("Transition/transversion ratio = %10.6f\n\n", rdta->ttratio);
    suma = rdta->ttratio*rdta->freqr*rdta->freqy
         - (rdta->freqa*rdta->freqg + rdta->freqc*rdta->freqt);
    sumb = rdta->freqa*rdta->freqgr + rdta->freqc*rdta->freqty;
    rdta->xi = suma/(suma+sumb);
    rdta->xv = 1.0 - rdta->xi;
    if (rdta->xi <= 0.0) {
      printf("WARNING: This transition/transversion ratio\n");
      printf("         is impossible with these base frequencies!\n");
      printf("Transition/transversion parameter reset\n\n");
      rdta->xi = 0.000001;
      rdta->xv = 1.0 - rdta->xi;
      rdta->ttratio = (sumb * rdta->xi / rdta->xv 
                       + rdta->freqa * rdta->freqg
                       + rdta->freqc * rdta->freqt)
                    / (rdta->freqr * rdta->freqy);
      printf("Transition/transversion ratio = %10.6f\n\n", rdta->ttratio);
      }
    printf("(Transition/transversion parameter = %10.6f)\n\n",
            rdta->xi/rdta->xv);
    rdta->fracchange = 2.0 * rdta->xi * (rdta->freqa * rdta->freqgr
                                       + rdta->freqc * rdta->freqty)
                     + rdta->xv * (1.0 - rdta->freqa * rdta->freqa
                                       - rdta->freqc * rdta->freqc
                                       - rdta->freqg * rdta->freqg
                                       - rdta->freqt * rdta->freqt);
  } /* reportfreqs */


boolean linkdata2tree (rawdata *rdta, cruncheddata *cdta, tree *tr)
    /* Link data array to the tree tips */
  { /* linkdata2tree */
    int  i;
    int  j;
    yType *tip;
    int ref[EQUALITIES];
    tr->rdta       = rdta;
    tr->cdta       = cdta;

    for(i = 0; i < EQUALITIES; i++)
      ref[i] = 0;
    equalities = 0;


    for (i = 1; i <= tr->mxtips; i++) {    /* Associate data with tips */
      tr->nodep[i]->tip = &(rdta->y[i-1][0]);
      tip =  tr->nodep[i]->tip;
      for(j = 0; j < tr->cdta->endsite; j++)
	{
	  if(ref[tip[j]] == 0)
	    ref[tip[j]] = 1; 
	}
      }

     for(i = 0; i < EQUALITIES; i++)
       {
	 if(ref[i] == 1) 
	   {
	     map[i] = equalities++;	     	   
	   }
	 else
	   map[i] = -1;
       }
     
     map2 = (char *)malloc(equalities * sizeof(char));

     j = 0;

     for(i = 0; i < EQUALITIES; i++)
       {
	 if(ref[i] == 1)
	   {
	     map2[j] = i;
	     j++;
	   }
       }
  
    

    return TRUE;
  } /* linkdata2tree */


xarray *setupxarray (int npat)
  { /* setupxarray */
    xarray        *x;
    likelivector  *data;

    x = (xarray *) Malloc(sizeof(xarray));
    if (x) {
      data = (likelivector *) Malloc(npat * sizeof(likelivector));
      if (data) {
        x->lv = data;
        x->prev = x->next = x;
        x->owner = (node *) NULL;
        }
      else {
        Free(x);
        return (xarray *) NULL;
        }
      }
    return x;
  } /* setupxarray */


boolean linkxarray (int req, int min, int npat,
                    xarray **freexptr, xarray **usedxptr)
    /*  Link a set of xarrays */
  { /* linkxarray */
    xarray  *first, *prev, *x;
    int  i;

    first = prev = (xarray *) NULL;
    i = 0;

    do {
      x = setupxarray(npat);
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
        if (i < min)  return  FALSE;
        }
      } while ((i < req) && x);

    if (first) {
      first->prev = prev;
      prev->next = first;
      }

    *freexptr = first;
    *usedxptr = (xarray *) NULL;

    return  TRUE;
  } /* linkxarray */


boolean setupnodex (tree *tr)
  { /* setupnodex */
    nodeptr  p;
    int  i;

    for (i = tr->mxtips + 1; (i <= 2*(tr->mxtips) - 2); i++) {
      p = tr->nodep[i];
      if (! (p->x = setupxarray(tr->cdta->endsite))) {
        printf("ERROR: Failure to get internal node xarray memory\n");
        return  FALSE;
        }
      }

    return  TRUE;
  } /* setupnodex */


xarray *getxtip (nodeptr p)
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
      return  (xarray *) NULL;
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


xarray *getxnode (nodeptr p)
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


 /* AxML modification start*/
 /* optimized version of function newview(), implementing homogeneous and */
 /* partially heterogeneous subtree equality vectors */


printhq(long *hq, tree *tr)
{
  int i;
  for(i = 0; i < tr->cdta->endsite; i++)
    fprintf(stderr,"%ld ", hq[i]);
  fprintf(stderr,"\n");
}



boolean newviewOptimized(tree *tr, nodeptr p)        /*  Update likelihoods at node */
  { /* newviewOptimized */
    double   zq, lzq, xvlzq, zr, lzr, xvlzr;
    nodeptr  q, r, realP;
    likelivector *lp, *lq, *lr;
    homType *mlvP;
    int  i, j;
           
    if (p->tip) {             /*  Make sure that data are at tip */
    
      if (p->x) return TRUE;  /*  They are already there */

      if (! getxtip(p)) return FALSE; /*  They are not, so get memory */      
      
      return TRUE;
      }

/*  Internal node needs update */

    q = p->next->back;
    r = p->next->next->back;

    while ((! p->x) || (! q->x) || (! r->x)) {
      /* rekursiver aufruf */
      if (! q->x) if (! newviewOptimized(tr, q))  return FALSE;
      if (! r->x) if (! newviewOptimized(tr, r))  return FALSE;
      if (! p->x) if (! getxnode(p)) return FALSE;
      }



    lp = p->x->lv;

    lq = q->x->lv;
    zq = q->z;
    lzq = (zq > zmin) ? log(zq) : log(zmin);
    xvlzq = tr->rdta->xv * lzq;

    lr = r->x->lv;
    zr = r->z;
    lzr = (zr > zmin) ? log(zr) : log(zmin);
    xvlzr = tr->rdta->xv * lzr;

    { double  zzq, zvq,
	zzr, zvr, rp;
    double  zzqtable[maxcategories+1], zvqtable[maxcategories+1],
              zzrtable[maxcategories+1], zvrtable[maxcategories+1],
             *zzqptr, *zvqptr, *zzrptr, *zvrptr, *rptr;
      double  fxqr, fxqy, fxqn, sumaq, sumgq, sumcq, sumtq,
              fxrr, fxry, fxrn, ki, tempi, tempj;                
      register  char *s1; 
      register  char *s2;
      register char *eqptr;
      int  *cptr;
      int cat;
      homType *mlvR, *mlvQ;
      poutsa calc[EQUALITIES][maxcategories],calcR[EQUALITIES][maxcategories], *calcptr, *calcptrR;
      long ref, ref2, mexp;      
      nodeptr realQ, realR;

      
      rptr   = &(tr->rdta->catrat[1]);
      zzqptr = &(zzqtable[1]);
      zvqptr = &(zvqtable[1]);
      zzrptr = &(zzrtable[1]);
      zvrptr = &(zvrtable[1]);
      /* get equality vectors of p, q, and r */
      
      realP =  tr->nodep[p->number];
    
      
      realQ =  tr->nodep[q->number];
      realR =  tr->nodep[r->number];

      eqptr = realP->equalityVector; 
      s1 = realQ->equalityVector;
      s2 = realR->equalityVector;
                 
    
       for (j = 1; j <= tr->rdta->categs; j++) 
	 {  	  
	   ki = *rptr++;
	   *zzqptr++ = zzq = exp(ki *   lzq);
	   *zvqptr++ = zvq = exp(ki * xvlzq);
	   *zzrptr++ = zzr = exp(ki *   lzr);
	   *zvrptr++ = zvr = exp(ki * xvlzr);	   
   	 	  
	   for(i = 0; i < equalities; i++)
	     {
	       
	       mlvQ = &realQ->mlv[i][j];

	       if(mlvQ->set)
		 {
		   calcptr = &calc[i][j];		 
		   fxqr = tr->rdta->freqa * mlvQ->a + tr->rdta->freqg * mlvQ->g;
		   fxqy = tr->rdta->freqc * mlvQ->c + tr->rdta->freqt * mlvQ->t;
		   fxqn = fxqr + fxqy;
		   tempi = fxqr * tr->rdta->invfreqr;
		   tempj = zvq * (tempi-fxqn) + fxqn;
		   calcptr->sumaq = zzq * (mlvQ->a - tempi) + tempj;
		   calcptr->sumgq = zzq * (mlvQ->g - tempi) + tempj;
		   tempi = fxqy * tr->rdta->invfreqy;
		   tempj = zvq * (tempi-fxqn) + fxqn;
		   calcptr->sumcq = zzq * (mlvQ->c - tempi) + tempj;
		   calcptr->sumtq = zzq * (mlvQ->t - tempi) + tempj;
		   calcptr->exp = mlvQ->exp;
		 }
	       
	       mlvR = &realR->mlv[i][j];

	       if(mlvR->set)
		 {
		   calcptrR = &calcR[i][j];		 
		   fxrr = tr->rdta->freqa * mlvR->a + tr->rdta->freqg * mlvR->g;
		   fxry = tr->rdta->freqc * mlvR->c + tr->rdta->freqt * mlvR->t;
		   fxrn = fxrr + fxry;
		   tempi = fxrr * tr->rdta->invfreqr;
		   tempj = zvr * (tempi-fxrn) + fxrn;
		   calcptrR->sumaq = zzr * (mlvR->a - tempi) + tempj;
		   calcptrR->sumgq = zzr * (mlvR->g - tempi) + tempj;
		   tempi = fxry * tr->rdta->invfreqy;
		   tempj = zvr * (tempi-fxrn) + fxrn;
		   calcptrR->sumcq = zzr * (mlvR->c - tempi) + tempj;
		   calcptrR->sumtq = zzr * (mlvR->t - tempi) + tempj;	     				
		   calcptrR->exp = mlvR->exp;	    	     	
		 }
	       
	       if(mlvR->set && mlvQ->set)
		 {
		   mlvP = &realP->mlv[i][j];

		   mlvP->a = calcptrR->sumaq * calcptr->sumaq;
		   mlvP->g = calcptrR->sumgq * calcptr->sumgq;		    
		   mlvP->c = calcptrR->sumcq * calcptr->sumcq; 
		   mlvP->t = calcptrR->sumtq * calcptr->sumtq;
		   mlvP->exp = calcptrR->exp + calcptr->exp;
		   if (mlvP->a < minlikelihood && mlvP->g < minlikelihood
		       && mlvP->c < minlikelihood && mlvP->t < minlikelihood) {
		     mlvP->a   *= twotothe256;
		     mlvP->g   *= twotothe256;
		     mlvP->c   *= twotothe256;
		     mlvP->t   *= twotothe256;
		     mlvP->exp += 1;
		   }
		   
		   mlvP->set = TRUE;
		 }
	       else
		 mlvP->set = FALSE;
	       
	     }
	 }

       cptr = &(tr->cdta->patcat[0]);

      if(r->tip && q->tip)
	{
	   for (i = 0; i < tr->cdta->endsite; i++) 
	    {
	      ref = *s2++;
	      ref2 = *s1++;
	      cat = *cptr++;

      		  	
	      if(ref2 != ref)
		{
		  *eqptr++ = -1;
		  calcptrR = &calcR[ref][cat];
		  calcptr = &calc[ref2][cat];
		  lp->a = calcptrR->sumaq * calcptr->sumaq;
		  lp->g = calcptrR->sumgq * calcptr->sumgq;		    
		  lp->c = calcptrR->sumcq * calcptr->sumcq; 
		  lp->t = calcptrR->sumtq * calcptr->sumtq;
		  lp->exp = calcptrR->exp + calcptr->exp;
		  if (lp->a < minlikelihood && lp->g < minlikelihood
		      && lp->c < minlikelihood && lp->t < minlikelihood) {
		    lp->a   *= twotothe256;
		    lp->g   *= twotothe256;
		    lp->c   *= twotothe256;
		    lp->t   *= twotothe256;
		    lp->exp += 1;
		  }
		   
		}
	      else
		{
		  *eqptr++ = ref;		    		
		}
	      lp++;
	   	      	    
	    }          
	}
      else
	{
	  	  
	  
	  for (i = 0; i < tr->cdta->endsite; i++) 
	    {
	      ref = *s2++;
	      ref2 = *s1++;
	      cat = *cptr++;
	      
	      if(ref2 >= 0)
		{		  	
		  if(ref2 == ref)
		    {
		      *eqptr++ = ref;		     		  
		      goto incTip;		    		  		 		  	    		
		    }
		  calcptr = &calc[ref2][cat];
		  sumaq = calcptr->sumaq;
		  sumgq = calcptr->sumgq;
		  sumcq = calcptr->sumcq;
		  sumtq = calcptr->sumtq;		 		  	
		  mexp = calcptr->exp;	       
		  
		}	    	  	  	 	 
	      else
		{		
		  zzq = zzqtable[cat];
		  zvq = zvqtable[cat];
		  fxqr = tr->rdta->freqa * lq->a + tr->rdta->freqg * lq->g;
		  fxqy = tr->rdta->freqc * lq->c + tr->rdta->freqt * lq->t;
		  fxqn = fxqr + fxqy;
		  tempi = fxqr * tr->rdta->invfreqr;
		  tempj = zvq * (tempi-fxqn) + fxqn;
		  sumaq = zzq * (lq->a - tempi) + tempj;
		  sumgq = zzq * (lq->g - tempi) + tempj;
		  tempi = fxqy * tr->rdta->invfreqy;
		  tempj = zvq * (tempi-fxqn) + fxqn;
		  sumcq = zzq * (lq->c - tempi) + tempj;
		  sumtq = zzq * (lq->t - tempi) + tempj;
		  mexp = lq->exp;
		}
	      
	      
	      if(ref >= 0)
		{
		  calcptr = &calcR[ref][cat];		 	    
		  lp->a = sumaq * calcptr->sumaq;
		  lp->g = sumgq * calcptr->sumgq;		    
		  lp->c = sumcq * calcptr->sumcq; 
		  lp->t = sumtq * calcptr->sumtq;
		  lp->exp = mexp + calcptr->exp;		  	  
		}
	      else
		{
		  zzr = zzrtable[cat];
		  zvr = zvrtable[cat];
		  fxrr = tr->rdta->freqa * lr->a + tr->rdta->freqg * lr->g;
		  fxry = tr->rdta->freqc * lr->c + tr->rdta->freqt * lr->t;
		  fxrn = fxrr + fxry;
		  tempi = fxrr * tr->rdta->invfreqr;
		  tempj = zvr * (tempi-fxrn) + fxrn;
		  lp->a = sumaq * (zzr * (lr->a - tempi) + tempj);
		  lp->g = sumgq * (zzr * (lr->g - tempi) + tempj);
		  tempi = fxry * tr->rdta->invfreqy;
		  tempj = zvr * (tempi-fxrn) + fxrn;
		  lp->c = sumcq * (zzr * (lr->c - tempi) + tempj);
		  lp->t = sumtq * (zzr * (lr->t - tempi) + tempj);
		  lp->exp = mexp + lr->exp;
		}
	      
	      if (lp->a < minlikelihood && lp->g < minlikelihood
		  && lp->c < minlikelihood && lp->t < minlikelihood) {
		lp->a   *= twotothe256;
		lp->g   *= twotothe256;
		lp->c   *= twotothe256;
		lp->t   *= twotothe256;
		lp->exp += 1;
	      }
	      
	      *eqptr++ = -1;
	    incTip:
	      lp++;
	      lq++;
	      lr++;	 
	    }          
	}
    }
                 

    return TRUE;  
  } /* newviewOptimized */

 /* AxML modification end*/

double evaluate (tree *tr, nodeptr p)
  { /* evaluate */
    double   sum, z, lz, xvlz,
             ki, fx1a, fx1c, fx1g, fx1t, fx1r, fx1y, fx2r, fx2y,
             suma, sumb, sumc, term;


    double   zz, zv;
    double        zztable[maxcategories+1], zvtable[maxcategories+1],
       *zzptr, *zvptr;   
    double       *log_f, *rptr;
    likelivector *lp, *lq;
    homType *mlvP, *mlvQ;
    nodeptr       q, realQ, realP;
    int           i, *wptr, cat, *cptr, j;
    register  char *sQ; 
    register  char *sP;
    long refQ, refP;
    memP calcP[EQUALITIES][maxcategories], *calcptrP;
    memQ calcQ[EQUALITIES][maxcategories], *calcptrQ;

    q = p->back;

    while ((! p->x) || (! q->x)) {
      if (! (p->x)) if (! newviewOptimized(tr, p)) return badEval;
      if (! (q->x)) if (! newviewOptimized(tr, q)) return badEval;
    }

    lp = p->x->lv;
    lq = q->x->lv;

    z = p->z;
    if (z < zmin) z = zmin;
    lz = log(z);
    xvlz = tr->rdta->xv * lz;

    rptr  = &(tr->rdta->catrat[1]);
    zzptr = &(zztable[1]);
    zvptr = &(zvtable[1]);
    
    
    cptr = &(tr->cdta->patcat[0]);  
    wptr = &(tr->cdta->aliaswgt[0]);
    log_f = tr->log_f;
    tr->log_f_valid = tr->cdta->endsite;
    sum = 0.0;
    
    realQ =  tr->nodep[q->number];
    realP =  tr->nodep[p->number];
    sQ = realQ->equalityVector;
    sP = realP->equalityVector; 
    

    for (j = 1; j <= tr->rdta->categs; j++) 
      {  
	ki = *rptr++;
	*zzptr++ = exp(ki *   lz);
	*zvptr++ = exp(ki * xvlz); 

	for(i = 0; i < equalities; i++)
	  { 
	   
	    mlvP = &realP->mlv[i][j];
	    if(mlvP->set)
	      {
		calcptrP = &calcP[i][j];		 
		calcptrP->fx1a = tr->rdta->freqa * mlvP->a;
		calcptrP->fx1g = tr->rdta->freqg * mlvP->g;
		calcptrP->fx1c = tr->rdta->freqc * mlvP->c;
		calcptrP->fx1t = tr->rdta->freqt * mlvP->t;
		calcptrP->sumag = calcptrP->fx1a + calcptrP->fx1g;
		calcptrP->sumct = calcptrP->fx1c + calcptrP->fx1t;
		calcptrP->sumagct = calcptrP->sumag + calcptrP->sumct;	      
		calcptrP->exp = mlvP->exp;
	      }
	    mlvQ = &realQ->mlv[i][j]; 
	    if(mlvQ->set)
	      {
		calcptrQ = &calcQ[i][j];
		calcptrQ->fx2r = tr->rdta->freqa * mlvQ->a + tr->rdta->freqg * mlvQ->g;	     
		calcptrQ->fx2y  = tr->rdta->freqc * mlvQ->c + tr->rdta->freqt * mlvQ->t; 	     
		calcptrQ->sumfx2rfx2y = calcptrQ->fx2r + calcptrQ->fx2y;	      
		calcptrQ->exp = mlvQ->exp;
		calcptrQ->a = mlvQ->a;
		calcptrQ->c = mlvQ->c;
		calcptrQ->g = mlvQ->g;
		calcptrQ->t = mlvQ->t;
	      }
	  
	  }
      }




      for (i = 0; i < tr->cdta->endsite; i++) {
         refQ = *sQ++;
	 refP = *sP++;
	 cat  = *cptr++;
	 zz   = zztable[cat];
	 zv   = zvtable[cat];
	 if(refP >= 0)
	   {
	     if(refQ >= 0)
	       {
		 calcptrP = &calcP[refP][cat];
		 calcptrQ = &calcQ[refQ][cat];
		 suma = calcptrP->fx1a * calcptrQ->a + 
		   calcptrP->fx1c * calcptrQ->c + 
		   calcptrP->fx1g * calcptrQ->g + calcptrP->fx1t * calcptrQ->t;		 		 
		 
		 sumc = calcptrP->sumagct * calcptrQ->sumfx2rfx2y;
		 sumb = calcptrP->sumag * calcptrQ->fx2r * tr->rdta->invfreqr + calcptrP->sumct * calcptrQ->fx2y * tr->rdta->invfreqy;
		 suma -= sumb;
		 sumb -= sumc;
		 term = log(zz * suma + zv * sumb + sumc) + 
		   (calcptrP->exp + calcptrQ->exp)*log(minlikelihood);
		 sum += *wptr++ * term;
		
		 *log_f++ = term;
		 lp++;
		 lq++;
	       }
	     else
	       {
		 calcptrP = &calcP[refP][cat];
		 suma = calcptrP->fx1a * lq->a + calcptrP->fx1c * lq->c + 
		   calcptrP->fx1g * lq->g + calcptrP->fx1t * lq->t;
		 
		 fx2r = tr->rdta->freqa * lq->a + tr->rdta->freqg * lq->g;
		 fx2y = tr->rdta->freqc * lq->c + tr->rdta->freqt * lq->t;
		 
		 sumc = calcptrP->sumagct * (fx2r + fx2y);
		 sumb       = calcptrP->sumag * fx2r * tr->rdta->invfreqr + calcptrP->sumct * fx2y * tr->rdta->invfreqy;
		 suma -= sumb;
		 sumb -= sumc;
		 term = log(zz * suma + zv * sumb + sumc) + 
		   (calcptrP->exp + lq->exp)*log(minlikelihood);
		 sum += *wptr++ * term;
		 
		 *log_f++ = term;
		 lp++;
		 lq++;
	       }
	   }
	 else
	   {
	     if(refQ >= 0)
	       {
		calcptrQ = &calcQ[refQ][cat];
		fx1a = tr->rdta->freqa * lp->a;
		fx1g = tr->rdta->freqg * lp->g;
		fx1c = tr->rdta->freqc * lp->c;
		fx1t = tr->rdta->freqt * lp->t;
		suma = fx1a * calcptrQ->a + fx1c * calcptrQ->c + 
		  fx1g * calcptrQ->g + fx1t * calcptrQ->t;	  
		fx1r = fx1a + fx1g;
		fx1y = fx1c + fx1t;
		sumc = (fx1r + fx1y) * calcptrQ->sumfx2rfx2y; 
		sumb = fx1r * calcptrQ->fx2r * tr->rdta->invfreqr + fx1y * calcptrQ->fx2y * tr->rdta->invfreqy;
		suma -= sumb;
		sumb -= sumc;	
		term = log(zz * suma + zv * sumb + sumc) + 
		  (lp->exp + calcptrQ->exp)*log(minlikelihood);		
		sum += *wptr++ * term;	
		*log_f++ = term;
		lp++;
		lq++;
	       }
	     else
	       {
		 fx1a = tr->rdta->freqa * lp->a;
		 fx1g = tr->rdta->freqg * lp->g;
		 fx1c = tr->rdta->freqc * lp->c;
		 fx1t = tr->rdta->freqt * lp->t;
		 suma = fx1a * lq->a + fx1c * lq->c + fx1g * lq->g + fx1t * lq->t;
		 fx2r = tr->rdta->freqa * lq->a + tr->rdta->freqg * lq->g;
		 fx2y = tr->rdta->freqc * lq->c + tr->rdta->freqt * lq->t;
		 fx1r = fx1a + fx1g;
		 fx1y = fx1c + fx1t;
		 sumc = (fx1r + fx1y) * (fx2r + fx2y);
		 sumb = fx1r * fx2r * tr->rdta->invfreqr + fx1y * fx2y * tr->rdta->invfreqy;
		 suma -= sumb;
		 sumb -= sumc;
		 term = log(zz * suma + zv * sumb + sumc) + 
		   (lp->exp + lq->exp)*log(minlikelihood);
		 sum += *wptr++ * term;		
		 *log_f++ = term;
		 lp++;
		 lq++;
	       }
	   }
        }

     
    tr->likelihood = sum;    
    return  sum;
  } /* evaluate */


long countEQ = 0, countNEQ = 0;

double makenewz (tree *tr, nodeptr p, nodeptr q, double z0, int maxiter)
  { /* makenewz */
    likelivector *lp, *lq, *mlv[EQUALITIES][maxcategories];
    double   *abi, *bci, *sumci, *abptr, *bcptr, *sumcptr;
    double   dlnLidlz, dlnLdlz, d2lnLdlz2, z, zprev, zstep, lz, xvlz,
             ki, suma, sumb, sumc, ab, bc, inv_Li, t1, t2,
             fx1a, fx1c, fx1g, fx1t, fx1r, fx1y, fx2r, fx2y;
    double   zzp, zvp;
    double   *wrptr, w, *wr2ptr, *rptr;
    int      i, curvatOK, j , cat, *cptr;
    memP calcP[EQUALITIES][maxcategories], *calcptrP;
    memQ calcQ[EQUALITIES][maxcategories], *calcptrQ;
    homType *mlvP, *mlvQ; 
    register  char *sQ; 
    register  char *sP;
    nodeptr realQ, realP;
    long refQ, refP;
    double   zztable[maxcategories+1], zvtable[maxcategories+1],
            *zzptr, *zvptr;


    while ((! p->x) || (! q->x)) {
      if (! (p->x)) if (! newviewOptimized(tr, p)) return badZ;
      if (! (q->x)) if (! newviewOptimized(tr, q)) return badZ;
      }

    realQ =  tr->nodep[q->number];
    realP =  tr->nodep[p->number];   
    sQ = realQ->equalityVector;
    sP = realP->equalityVector; 
    lp = p->x->lv;
    lq = q->x->lv;

    

    { 
      unsigned scratch_size;
      scratch_size = sizeof(double) * tr->cdta->endsite;
      if ((abi   = (double *) Malloc(scratch_size)) &&
          (bci   = (double *) Malloc(scratch_size)) &&
          (sumci = (double *) Malloc(scratch_size))) ;
      else {
        printf("ERROR: makenewz unable to obtain space for arrays\n");
        return badZ;
        }
      
      }
    abptr   = abi;
    bcptr   = bci;
    sumcptr = sumci;

#   ifdef Vectorize
#     pragma IVDEP
#   endif


    
    for (j = 1; j <= tr->rdta->categs; j++) 
      {  
	for(i = 0; i < equalities; i++)
	  {
	    mlvP = &realP->mlv[i][j];
	    if(mlvP->set)
	      {
		calcptrP = &calcP[i][j];		 
		calcptrP->fx1a = tr->rdta->freqa * mlvP->a;
		calcptrP->fx1g = tr->rdta->freqg * mlvP->g;
		calcptrP->fx1c = tr->rdta->freqc * mlvP->c;
		calcptrP->fx1t = tr->rdta->freqt * mlvP->t;
		calcptrP->sumag = calcptrP->fx1a + calcptrP->fx1g;
		calcptrP->sumct = calcptrP->fx1c + calcptrP->fx1t;
		calcptrP->sumagct = calcptrP->sumag + calcptrP->sumct;	      
	      }
	    mlvQ = &realQ->mlv[i][j]; 
	    if(mlvQ->set)
	      {
		calcptrQ = &calcQ[i][j];
		calcptrQ->fx2r = tr->rdta->freqa * mlvQ->a + tr->rdta->freqg * mlvQ->g;	     
		calcptrQ->fx2y = tr->rdta->freqc * mlvQ->c + tr->rdta->freqt * mlvQ->t; 	     
		calcptrQ->sumfx2rfx2y =  calcptrQ->fx2r +  calcptrQ->fx2y;	     
		calcptrQ->a = mlvQ->a;
		calcptrQ->c = mlvQ->c;
		calcptrQ->g = mlvQ->g;
		calcptrQ->t = mlvQ->t;	      
	      }	     
	  }
      }
    cptr    = &(tr->cdta->patcat[0]);

    for (i = 0; i < tr->cdta->endsite; i++) {
      refQ = *sQ++;
      refP = *sP++;

      cat = *cptr++;
      
      if(refP >= 0)
	{
	  calcptrP = &calcP[refP][cat];

	  if(refQ >= 0)
	    {
	      
	      calcptrQ = &calcQ[refQ][cat];
	      suma = calcptrP->fx1a * calcptrQ->a + calcptrP->fx1c * calcptrQ->c + 
		calcptrP->fx1g * calcptrQ->g + calcptrP->fx1t * calcptrQ->t;
	      	      
	      
	      *sumcptr++ = sumc = calcptrP->sumagct * calcptrQ->sumfx2rfx2y;
	      sumb       = calcptrP->sumag * calcptrQ->fx2r * tr->rdta->invfreqr + calcptrP->sumct * calcptrQ->fx2y * tr->rdta->invfreqy;
	      *abptr++   = suma - sumb;
	      *bcptr++   = sumb - sumc;
	      lp++;
	      lq++;	 
	    }
	  else
	    {	     
	      suma = calcptrP->fx1a * lq->a + calcptrP->fx1c * lq->c + 
		calcptrP->fx1g * lq->g + calcptrP->fx1t * lq->t;
	      
	      fx2r = tr->rdta->freqa * lq->a + tr->rdta->freqg * lq->g;
	      fx2y = tr->rdta->freqc * lq->c + tr->rdta->freqt * lq->t;
	      
	      *sumcptr++ = sumc = calcptrP->sumagct * (fx2r + fx2y);
	      sumb       = calcptrP->sumag * fx2r  * tr->rdta->invfreqr + calcptrP->sumct * fx2y * tr->rdta->invfreqy ;
	      *abptr++   = suma - sumb;
	      *bcptr++   = sumb - sumc;
	      lp++;
	      lq++;	  
	    }
	}
      else
	{
	  if(refQ >= 0)
	    {
	      calcptrQ = &calcQ[refQ][cat];
	      fx1a = tr->rdta->freqa * lp->a;
	      fx1g = tr->rdta->freqg * lp->g;
	      fx1c = tr->rdta->freqc * lp->c;
	      fx1t = tr->rdta->freqt * lp->t;
	      suma = fx1a * calcptrQ->a + fx1c * calcptrQ->c + 
		fx1g * calcptrQ->g + fx1t * calcptrQ->t;	  
	      fx1r = fx1a + fx1g;
	      fx1y = fx1c + fx1t;
	      *sumcptr++ = sumc = (fx1r + fx1y) * calcptrQ->sumfx2rfx2y; 
	      sumb       = fx1r * calcptrQ->fx2r  * tr->rdta->invfreqr + fx1y * calcptrQ->fx2y * tr->rdta->invfreqy;
	      *abptr++   = suma - sumb;
	      *bcptr++   = sumb - sumc;
	      lp++;
	      lq++;
	    }
	  else
	    {

	      fx1a = tr->rdta->freqa * lp->a;
	      fx1g = tr->rdta->freqg * lp->g;
	      fx1c = tr->rdta->freqc * lp->c;
	      fx1t = tr->rdta->freqt * lp->t;
	      suma = fx1a * lq->a + fx1c * lq->c + fx1g * lq->g + fx1t * lq->t;
	      fx2r = tr->rdta->freqa * lq->a + tr->rdta->freqg * lq->g;
	      //nur von q abh
	      fx2y = tr->rdta->freqc * lq->c + tr->rdta->freqt * lq->t; 
	      //nur von q abh
	      fx1r = fx1a + fx1g;
	      fx1y = fx1c + fx1t;
	      *sumcptr++ = sumc = (fx1r + fx1y) * (fx2r + fx2y); 
	      //zweiter term nur von q abh
	      sumb       = fx1r * fx2r * tr->rdta->invfreqr + fx1y * fx2y * tr->rdta->invfreqy;


    
	      *abptr++   = suma - sumb;
	      *bcptr++   = sumb - sumc;
	      lp++;
	      lq++;
	    }
	}

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
        xvlz  = tr->rdta->xv * lz;
        rptr  = &(tr->rdta->catrat[1]);
        zzptr = &(zztable[1]);
        zvptr = &(zvtable[1]);

#       ifdef Vectorize
#         pragma IVDEP
#       endif

        for (i = 1; i <= tr->rdta->categs; i++) {
          ki = *rptr++;
          *zzptr++ = exp(ki *   lz);
          *zvptr++ = exp(ki * xvlz);
          }

        abptr   = abi;
        bcptr   = bci;
        sumcptr = sumci;
        cptr    = &(tr->cdta->patcat[0]);
        wrptr   = &(tr->cdta->wr[0]);
        wr2ptr  = &(tr->cdta->wr2[0]);
        dlnLdlz = 0.0;                 /*  = d(ln(likelihood))/d(lz) */
        d2lnLdlz2 = 0.0;               /*  = d2(ln(likelihood))/d(lz)2 */

#       ifdef Vectorize
#         pragma IVDEP
#       endif

        for (i = 0; i < tr->cdta->endsite; i++) {
          cat    = *cptr++;              /*  ratecategory(i) */
          ab     = *abptr++ * zztable[cat];
          bc     = *bcptr++ * zvtable[cat];
          sumc   = *sumcptr++;
          inv_Li = 1.0/(ab + bc + sumc);
          t1     = ab * inv_Li;
          t2     = tr->rdta->xv * bc * inv_Li;
          dlnLidlz   = t1 + t2;
          dlnLdlz   += *wrptr++  * dlnLidlz;
          d2lnLdlz2 += *wr2ptr++ * (t1 + tr->rdta->xv * t2 - dlnLidlz * dlnLidlz);
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

   
    Free(abi);
    Free(bci);
    Free(sumci);

    
 
    return  z;
  } /* makenewz */


boolean update (tree *tr, nodeptr p)
  { /* update */
    nodeptr  q;
    double   z0, z;

    q = p->back;
    z0 = q->z;
    if ((z = makenewz(tr, p, q, z0, newzpercycle)) == badZ) return FALSE;
    p->z = q->z = z;
    if (ABS(z - z0) > deltaz)  tr->smoothed = FALSE;
    return TRUE;
  } /* update */


boolean smooth (tree *tr, nodeptr p)
  { /* smooth */
    nodeptr  q;

    if (! update(tr, p))               return FALSE; /*  Adjust branch */
    if (! p->tip) {                                  /*  Adjust descendants */
        q = p->next;
        while (q != p) {
          if (! smooth(tr, q->back))   return FALSE;
          q = q->next;
          }

#     if ReturnSmoothedView

	if (! newviewOptimized(tr, p)) return FALSE;
#     endif
      }

    return TRUE;
  } /* smooth */


boolean smoothTree (tree *tr, int maxtimes)
  { /* smoothTree */
    nodeptr  p, q;

    p = tr->start;

    while (--maxtimes >= 0) {
      tr->smoothed = TRUE;
      if (! smooth(tr, p->back))       return FALSE;
      if (! p->tip) {
        q = p->next;
        while (q != p) {
          if (! smooth(tr, q->back))   return FALSE;
          q = q->next;
          }
        }
      if (tr->smoothed)  break;
      }

    return TRUE;
  } /* smoothTree */


boolean localSmooth (tree *tr, nodeptr p, int maxtimes)
  { /* localSmooth -- Smooth branches around p */
    nodeptr  q;

    if (p->tip) return FALSE;            /* Should be an error */

    while (--maxtimes >= 0) {
      tr->smoothed = TRUE;
      q = p;
      do {
        if (! update(tr, q)) return FALSE;
        q = q->next;
        } while (q != p);
      if (tr->smoothed)  break;
      }

    tr->smoothed = FALSE;             /* Only smooth locally */
    return TRUE;
  } /* localSmooth */


void hookup (nodeptr p, nodeptr q, double z)
  { /* hookup */
    p->back = q;
    q->back = p;
    p->z = q->z = z;
  } /* hookup */


/* Insert node p into branch q <-> q->back */


boolean insert (tree *tr, nodeptr p, nodeptr q, boolean glob)
/* glob -- Smooth tree globally? */

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

      if ((zqr = makenewz(tr, q, r, q->z,     iterations)) == badZ) return FALSE;
      if ((zqs = makenewz(tr, q, s, defaultz, iterations)) == badZ) return FALSE;
      if ((zrs = makenewz(tr, r, s, defaultz, iterations)) == badZ) return FALSE;

      
  

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


    if (! newviewOptimized(tr, p)) return FALSE;
    
   

    tr->opt_level = 0;

#   if ! Master         /*  Smoothings are done by slave */
      if (glob) {                                    /* Smooth whole tree */
        if (! smoothTree(tr, smoothings)) return FALSE;
        }
      else {                                         /* Smooth locale of p */
        if (! localSmooth(tr, p, smoothings)) return FALSE;
        }

#   else
      tr->likelihood = unlikely;
#   endif
      
    return  TRUE;
  } /* insert */


nodeptr  removeNode (tree *tr, nodeptr p)

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
      if ((zqr = makenewz(tr, q, r, zqr, iterations)) == badZ) return (node *) NULL;
#   endif
    hookup(q, r, zqr);

    p->next->next->back = p->next->back = (node *) NULL;
    return  q;
  } /* removeNode */


boolean initrav (tree *tr, nodeptr p)
  { /* initrav */
    nodeptr  q;

    if (! p->tip) {
      q = p->next;
      do {
        if (! initrav(tr, q->back))  return FALSE;
        q = q->next;
        } while (q != p);

      if (! newviewOptimized(tr, p)) return FALSE;

      }

    return TRUE;
  } /* initrav */


nodeptr buildNewTip (tree *tr, nodeptr p)
  { /* buildNewTip */
    nodeptr  q;

    q = tr->nodep[(tr->nextnode)++];
    hookup(p, q, defaultz);
    return  q;
  } /* buildNewTip */


boolean buildSimpleTree (tree *tr, int ip, int iq, int ir)
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

    return insert(tr, s, p, FALSE);  /* Smoothing is local to s */
  } /* buildSimpleTree */

#if 0
char * strchr (char *str, int chr)
 { /* strchr */
    int  c;

    while (c = *str)  {if (c == chr) return str; str++;}
    return  (char *) NULL;
 } /* strchr */


char * strstr (char *str1, char *str2)
 { /* strstr */
    char *s1, *s2;
    int  c;

    while (*(s1 = str1)) {
      s2 = str2;
      do {
        if (! (c = *s2++))  return str1;
        } 
        while (*s1++ == c);
      str1++;
      }
    return  (char *) NULL;
 } /* strstr */
#endif

boolean readKeyValue (char *string, char *key, char *format, void *value)
  { /* readKeyValue */

    if (! (string = strstr(string, key)))  return FALSE;
    string += strlen(key);
    if (! (string = strchr(string, '=')))  return FALSE;
    string++;
    return  sscanf(string, format, value);  /* 1 if read, otherwise 0 */
  } /* readKeyValue */


#if Master || Slave

double str_readTreeLikelihood (char *treestr)
  { /* str_readTreeLikelihood */
    double lk1;
    char    *com, *com_end;
    boolean  readKeyValue();

    if ((com = strchr(treestr, '[')) && (com < strchr(treestr, '('))
                                     && (com_end = strchr(com, ']'))) {
      com++;
      *com_end = 0;
      if (readKeyValue(com, likelihood_key, "%lg", (void *) &(lk1))) {
        *com_end = ']';
        return lk1;
        }
      }

    fprintf(stderr, "ERROR reading likelihood in receiveTree\n");
    return  badEval;
  } /* str_readTreeLikelihood */


boolean sendTree (comm_block *comm, tree *tr)
  { /* sendTree */
    char  *treestr;
    char  *treeString();
#   if Master
      void sendTreeNum();
#   endif

    comm->done_flag = tr->likelihood > 0.0;
    if (comm->done_flag)
      write_comm_msg(comm, NULL);

    else {
      treestr = (char *) Malloc((tr->ntips * (nmlngth+32)) + 256);
      if (! treestr) {
        fprintf(stderr, "sendTree: Malloc failure\n");
        return 0;
        }

#     if Master
        if (send_ahead >= MAX_SEND_AHEAD) {
          double new_likelihood;
          int  n_to_get;

          n_to_get = (send_ahead+1)/2;
          sendTreeNum(n_to_get);
          send_ahead -= n_to_get;
          read_comm_msg(& comm_slave, treestr);
          new_likelihood = str_readTreeLikelihood(treestr);
          if (new_likelihood == badEval)  return FALSE;
          if (! best_tr_recv || (new_likelihood > best_lk_recv)) {
            if (best_tr_recv)  Free(best_tr_recv);
            best_tr_recv = Malloc(strlen(treestr) + 1);
            strcpy(best_tr_recv, treestr);
            best_lk_recv = new_likelihood;
            }
          }
        send_ahead++;
#     endif           /*  End #if Master  */

      REPORT_SEND_TREE;
      (void) treeString(treestr, tr, tr->start->back, 1);
      write_comm_msg(comm, treestr);

      Free(treestr);
      }

    return TRUE;
  } /* sendTree */


boolean  receiveTree (comm_block *comm, tree *tr)
  { /* receiveTree */
    char   *treestr;
    boolean status;
    boolean str_treeReadLen();

    treestr = (char *) Malloc((tr->ntips * (nmlngth+32)) + 256);
    if (! treestr) {
      fprintf(stderr, "receiveTree: Malloc failure\n");
      return 0;
      }

    read_comm_msg(comm, treestr);
    if (comm->done_flag) {
      tr->likelihood = 1.0;
      status = TRUE;
      }

    else {
#     if Master
        if (best_tr_recv) {
          if (str_readTreeLikelihood(treestr) < best_lk_recv) {
            strcpy(treestr, best_tr_recv);  /* Overwrite new tree with best */
            }
          Free(best_tr_recv);
          best_tr_recv = NULL;
          }
#     endif           /*  End #if Master  */

      status = str_treeReadLen(treestr, tr);
      }

    Free(treestr);
    return status;
  } /* receiveTree */


void requestForWork (void)
  { /* requestForWork */
    p4_send(DNAML_REQUEST, DNAML_DISPATCHER_ID, NULL, 0);
  } /* requestForWork */
#endif                  /* End #if Master || Slave  */


#if Master
void sendTreeNum(int n_to_get)
  { /* sendTreeNum */
    char scr[512];

    sprintf(scr, "%d", n_to_get);
    p4_send(DNAML_NUM_TREE, DNAML_MERGER_ID, scr, strlen(scr)+1);
  } /* sendTreeNum */


boolean  getReturnedTrees (tree *tr, bestlist *bt, int n_tree_sent)
 /* n_tree_sent -- number of trees sent to slaves */
  { /* getReturnedTrees */
    void sendTreeNum();
    boolean receiveTree();

    sendTreeNum(send_ahead);
    send_ahead = 0;

    if (! receiveTree(& comm_slave, tr))  return FALSE;
    tr->smoothed = TRUE;
    (void) saveBestTree(bt, tr);

    return TRUE;
  } /* getReturnedTrees */
#endif


void cacheZ (tree *tr)
  { /* cacheZ */
    nodeptr  p;
    int  nodes;

    nodes = tr->mxtips  +  3 * (tr->mxtips - 2);
    p = tr->nodep[1];
    while (nodes-- > 0) {p->z0 = p->z; p++;}
  } /* cacheZ */


void restoreZ (tree *tr)
  { /* restoreZ */
    nodeptr  p;
    int  nodes;

    nodes = tr->mxtips  +  3 * (tr->mxtips - 2);
    p = tr->nodep[1];
    while (nodes-- > 0) {p->z = p->z0; p++;}
  } /* restoreZ */


boolean testInsert (tree *tr, nodeptr p, nodeptr q, bestlist *bt, boolean  fast)
  { /* testInsert */
    double  qz;
    nodeptr  r;

    r = q->back;             /* Save original connection */
    qz = q->z;
    if (! insert(tr, p, q, ! fast)) return FALSE;

#   if ! Master
      if (evaluate(tr, fast ? p->next->next : tr->start) == badEval) return FALSE;
      (void) saveBestTree(bt, tr);
#   else  /* Master */
      tr->likelihood = unlikely;
      if (! sendTree(& comm_slave, tr))  return FALSE;
#   endif

    /* remove p from this branch */

    hookup(q, r, qz);
    p->next->next->back = p->next->back = (nodeptr) NULL;
    if (! fast) {            /* With fast add, other values are still OK */
      restoreZ(tr);          /*   Restore branch lengths */
#     if ! Master            /*   Regenerate x values */
        if (! initrav(tr, p->back))  return FALSE;
        if (! initrav(tr, q))        return FALSE;
        if (! initrav(tr, r))        return FALSE;
#     endif
      }

    return TRUE;
  } /* testInsert */


int addTraverse (tree *tr, nodeptr p, nodeptr q,
                 int mintrav, int maxtrav, bestlist *bt, boolean fast)
  { /* addTraverse */
    int  tested, newtested;

    tested = 0;
    if (--mintrav <= 0) {           /* Moved minimum distance? */
      if (! testInsert(tr, p, q, bt, fast))  return badRear;
      tested++;
      }

    if ((! q->tip) && (--maxtrav > 0)) {    /* Continue traverse? */
      newtested = addTraverse(tr, p, q->next->back,
                              mintrav, maxtrav, bt, fast);
      if (newtested == badRear) return badRear;
      tested += newtested;
      newtested = addTraverse(tr, p, q->next->next->back,
                              mintrav, maxtrav, bt, fast);
      if (newtested == badRear) return badRear;
      tested += newtested;
      }

    return tested;
  } /* addTraverse */


int  rearrange (tree *tr, nodeptr p, int mintrav, int maxtrav, bestlist *bt)
    /* rearranges the tree, globally or locally */
  { /* rearrange */
    double   p1z, p2z, q1z, q2z;
    nodeptr  p1, p2, q, q1, q2;
    int      tested, mintrav2, newtested;

    tested = 0;
    if (maxtrav < 1 || mintrav > maxtrav)  return tested;

/* Moving subtree forward in tree. */

    if (! p->tip) {
      p1 = p->next->back;
      p2 = p->next->next->back;
      if (! p1->tip || ! p2->tip) {
        p1z = p1->z;
        p2z = p2->z;
        if (! removeNode(tr, p)) return badRear;
        cacheZ(tr);
        if (! p1->tip) {
          newtested = addTraverse(tr, p, p1->next->back,
                                  mintrav, maxtrav, bt, FALSE);
          if (newtested == badRear) return badRear;
          tested += newtested;
          newtested = addTraverse(tr, p, p1->next->next->back,
                                  mintrav, maxtrav, bt, FALSE);
          if (newtested == badRear) return badRear;
          tested += newtested;
          }

        if (! p2->tip) {
          newtested = addTraverse(tr, p, p2->next->back,
                                  mintrav, maxtrav, bt, FALSE);
          if (newtested == badRear) return badRear;
          tested += newtested;
          newtested = addTraverse(tr, p, p2->next->next->back,
                                  mintrav, maxtrav, bt, FALSE);
          if (newtested == badRear) return badRear;
          tested += newtested;
          }

        hookup(p->next,       p1, p1z);  /*  Restore original tree */
        hookup(p->next->next, p2, p2z);
        if (! (initrav(tr, tr->start)
            && initrav(tr, tr->start->back))) return badRear;
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
        if (! removeNode(tr, q)) return badRear;
        cacheZ(tr);
        mintrav2 = mintrav > 2 ? mintrav : 2;

        if (! q1->tip) {
          newtested = addTraverse(tr, q, q1->next->back,
                                  mintrav2 , maxtrav, bt, FALSE);
          if (newtested == badRear) return badRear;
          tested += newtested;
          newtested = addTraverse(tr, q, q1->next->next->back,
                                  mintrav2 , maxtrav, bt, FALSE);
          if (newtested == badRear) return badRear;
          tested += newtested;
          }

        if (! q2->tip) {
          newtested = addTraverse(tr, q, q2->next->back,
                                  mintrav2 , maxtrav, bt, FALSE);
          if (newtested == badRear) return badRear;
          tested += newtested;
          newtested = addTraverse(tr, q, q2->next->next->back,
                                  mintrav2 , maxtrav, bt, FALSE);
          if (newtested == badRear) return badRear;
          tested += newtested;
          }

        hookup(q->next,       q1, q1z);  /*  Restore original tree */
        hookup(q->next->next, q2, q2z);
        if (! (initrav(tr, tr->start)
            && initrav(tr, tr->start->back))) return badRear;
        }
      }   /* if (! q->tip && maxtrav > 1) */

/* Move other subtrees */

    if (! p->tip) {
      newtested = rearrange(tr, p->next->back,       mintrav, maxtrav, bt);
      if (newtested == badRear) return badRear;
      tested += newtested;
      newtested = rearrange(tr, p->next->next->back, mintrav, maxtrav, bt);
      if (newtested == badRear) return badRear;
      tested += newtested;
      }

    return  tested;
  } /* rearrange */


FILE *fopen_pid (char *filenm, char *mode, char *name_pid)
  { /* fopen_pid */

    (void) sprintf(name_pid, "%s.%d", filenm, getpid());
    return  fopen(name_pid, mode);
  } /* fopen_pid */


#if DeleteCheckpointFile
void  unlink_pid (char *filenm)
  { /* unlink_pid */
    char scr[512];

    (void) sprintf(scr, "%s.%d", filenm, getpid());
    unlink(scr);
  } /* unlink_pid */
#endif


void  writeCheckpoint (tree *tr)
  { /* writeCheckpoint */
    char   filename[128];
    FILE  *checkpointf;
    void   treeOut();

    checkpointf = fopen_pid(checkpointname, "a", filename);
    if (checkpointf) {
      treeOut(checkpointf, tr, treeNewick);
      (void) fclose(checkpointf);
      }
  } /* writeCheckpoint */


node * findAnyTip(nodeptr p)
  { /* findAnyTip */
    return  p->tip ? p : findAnyTip(p->next->back);
  } /* findAnyTip */


boolean  optimize (tree *tr, int maxtrav, bestlist *bt)
  { /* optimize */
    nodeptr  p;
    int    mintrav, tested;

    if (tr->ntips < 4)  return  TRUE;

    writeCheckpoint(tr);                    /* checkpoint the starting tree */

    if (maxtrav > tr->ntips - 3)  maxtrav = tr->ntips - 3;
    if (maxtrav <= tr->opt_level)  return  TRUE;

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
      if (tested == badRear)  return FALSE;

#     if Master
        if (! getReturnedTrees(tr, bt, tested)) return FALSE;
#     endif

      bt->numtrees += tested;
      (void) setOptLevel(bt, maxtrav);
      if (! recallBestTree(bt, 1, tr)) return FALSE;   /* recover best tree */

      printf("      Tested %d alternative trees\n", tested);
     
      if (bt->improved) {
        printf("      Ln Likelihood =%14.5f\n", tr->likelihood);
        }
      
      writeCheckpoint(tr);                  /* checkpoint the new tree */
      } while (maxtrav > tr->opt_level);

    return TRUE;
  } /* optimize */


void coordinates (tree *tr, nodeptr p, double lengthsum, drawdata *tdptr)
  { /* coordinates */
    /* establishes coordinates of nodes */
    double  x, z;
    nodeptr  q, first, last;

    if (p->tip) {
      p->xcoord = NINT(over * lengthsum);
      p->ymax = p->ymin = p->ycoord = tdptr->tipy;
      tdptr->tipy += down;
      if (lengthsum > tdptr->tipmax) tdptr->tipmax = lengthsum;
      }

    else {
      q = p->next;
      do {
        z = q->z;
        if (z < zmin) z = zmin;
        x = lengthsum - tr->rdta->fracchange * log(z);
        coordinates(tr, q->back, x, tdptr);
        q = q->next;
        } while (p == tr->start->back ? q != p->next : q != p);

      first = p->next->back;
      q = p;
      while (q->next != p) q = q->next;
      last = q->back;
      p->xcoord = NINT(over * lengthsum);
      p->ycoord = (first->ycoord + last->ycoord)/2;
      p->ymin = first->ymin;
      p->ymax = last->ymax;
      }
  } /* coordinates */


void drawline (tree *tr, int i, double scale)
    /* draws one row of the tree diagram by moving up tree */
    /* Modified to handle 1000 taxa, October 16, 1991 */
  { /* drawline */
    nodeptr  p, q, r, first, last;
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
      n = NINT(scale*(q->xcoord - p->xcoord));
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
      printf(" %s", p->name);
      }

    putchar('\n');
  } /* drawline */


void printTree (tree *tr, analdef *adef)
    /* prints out diagram of the tree */
  { /* printTree */
    drawdata  tipdata;
    double  scale;
    int  i, imax;

    if (adef->trprint) {
      putchar('\n');
      tipdata.tipy = 1;
      tipdata.tipmax = 0.0;
      coordinates(tr, tr->start->back, (double) 0.0, & tipdata);
      scale = 1.0 / tipdata.tipmax;
      imax = tipdata.tipy - down;
      for (i = 1; i <= imax; i++)  drawline(tr, i, scale);
      printf("\nRemember: ");
      if (adef->root) printf("(although rooted by outgroup) ");
      printf("this is an unrooted tree!\n\n");
      }
  } /* printTree */


double sigma (tree *tr, nodeptr p, double *sumlrptr)
    /* compute standard deviation */
  { /* sigma */
    likelivector *lp, *lq;
    homType *mlvP, *mlvQ;
    double  slope, sum, sumlr, z, zv, zz, lz,
            rat, suma, sumb, sumc, d2, d, li, temp, abzz, bczv, t3,
            fx1a, fx1c, fx1g, fx1t, fx1r, fx1y, fx2r, fx2y, w;
    double  *rptr;
    nodeptr  q, realQ, realP;
    int  i, *wptr, j, cat, *cptr;
    register  char *sQ; 
    register  char *sP;
    long refQ, refP;
    memP calcP[EQUALITIES][maxcategories], *calcptrP;
    memQ calcQ[EQUALITIES][maxcategories], *calcptrQ;
    
    q = p->back;
    while ((! p->x) || (! q->x)) {
      /*stamatak*/

      if (! (p->x)) if (! newviewOptimized(tr, p)) return -1.0;
      if (! (q->x)) if (! newviewOptimized(tr, q)) return -1.0;
      }

    lp = p->x->lv;
    lq = q->x->lv;

    z = p->z;
    if (z < zmin) z = zmin;
    lz = log(z);
    cptr    = &(tr->cdta->patcat[0]);
    wptr = &(tr->cdta->aliaswgt[0]);
    rptr = &(tr->cdta->patrat[0]);
    sum = sumlr = slope = 0.0;

#   ifdef Vectorize
#     pragma IVDEP
#   endif

    realQ =  tr->nodep[q->number];
    realP =  tr->nodep[p->number];
    sQ = realQ->equalityVector;
    sP = realP->equalityVector; 

     for (j = 1; j <= tr->rdta->categs; j++) 
       {  

	 for(i = 0; i < equalities; i++)
	   {
	     mlvP = &realP->mlv[i][j];
	     if(mlvP->set)
	       {
		 calcptrP = &calcP[i][j];		 
		 calcptrP->fx1a = tr->rdta->freqa * mlvP->a;
		 calcptrP->fx1g = tr->rdta->freqg * mlvP->g;
		 calcptrP->fx1c = tr->rdta->freqc * mlvP->c;
		 calcptrP->fx1t = tr->rdta->freqt * mlvP->t;
		 calcptrP->sumag = calcptrP->fx1a + calcptrP->fx1g;
		 calcptrP->sumct = calcptrP->fx1c + calcptrP->fx1t;
		 calcptrP->sumagct = calcptrP->sumag + calcptrP->sumct;	      
		 calcptrP->exp = mlvP->exp;
	       }
	     mlvQ = &realQ->mlv[i][j]; 
	     if(mlvQ->set)
	       {
		 calcptrQ = &calcQ[i][j];
		 calcptrQ->fx2r = tr->rdta->freqa * mlvQ->a + tr->rdta->freqg * mlvQ->g;	     
		 calcptrQ->fx2y  = tr->rdta->freqc * mlvQ->c + tr->rdta->freqt * mlvQ->t; 	     
		 calcptrQ->sumfx2rfx2y = calcptrQ->fx2r + calcptrQ->fx2y;	     
		 calcptrQ->exp = mlvQ->exp;
		 calcptrQ->a = mlvQ->a;
		 calcptrQ->c = mlvQ->c;
		 calcptrQ->g = mlvQ->g;
		 calcptrQ->t = mlvQ->t;
	       }	     
	   }
       }
    

   
    

    for (i = 0; i < tr->cdta->endsite; i++) {
      rat  = *rptr++;
      cat = *cptr++;
      refQ = *sQ++;
      refP = *sP++;

      zz   = exp(rat                * lz);
      zv   = exp(rat * tr->rdta->xv * lz);

      
      if(refP >= 0)
	{
	  if(refQ >= 0)
	    {
	      calcptrP = &calcP[refP][cat];
	      calcptrQ = &calcQ[refQ][cat];
	      suma = calcptrP->fx1a * calcptrQ->a + 
		calcptrP->fx1c * calcptrQ->c + 
		calcptrP->fx1g * calcptrQ->g + calcptrP->fx1t * calcptrQ->t;
	      	     
	      
	      sumc = calcptrP->sumagct * calcptrQ->sumfx2rfx2y;
	      sumb       = calcptrP->sumag * calcptrQ->fx2r * tr->rdta->invfreqr + calcptrP->sumct * calcptrQ->fx2y * tr->rdta->invfreqy;
		
	    }
	  else
	    {
	      calcptrP = &calcP[refP][cat];
	      suma = calcptrP->fx1a * lq->a + calcptrP->fx1c * lq->c + 
		calcptrP->fx1g * lq->g + calcptrP->fx1t * lq->t;
	      
	      fx2r = tr->rdta->freqa * lq->a + tr->rdta->freqg * lq->g;
	      fx2y = tr->rdta->freqc * lq->c + tr->rdta->freqt * lq->t;
		 
	      sumc = calcptrP->sumagct * (fx2r + fx2y);
	      sumb       = calcptrP->sumag * fx2r * tr->rdta->invfreqr + calcptrP->sumct * fx2y * tr->rdta->invfreqy;
		
	    }
	}
      else
	{
	  if(refQ >= 0)
	    {
	      calcptrQ = &calcQ[refQ][cat];
	      fx1a = tr->rdta->freqa * lp->a;
	      fx1g = tr->rdta->freqg * lp->g;
	      fx1c = tr->rdta->freqc * lp->c;
	      fx1t = tr->rdta->freqt * lp->t;
	      suma = fx1a * calcptrQ->a + fx1c * calcptrQ->c + 
		fx1g * calcptrQ->g + fx1t * calcptrQ->t;	  
	      fx1r = fx1a + fx1g;
	      fx1y = fx1c + fx1t;
	      sumc = (fx1r + fx1y) * calcptrQ->sumfx2rfx2y; 
	      sumb = fx1r * calcptrQ->fx2r * tr->rdta->invfreqr + fx1y * calcptrQ->fx2y * tr->rdta->invfreqy;
	      
	    }
	  else
	    {
	      fx1a = tr->rdta->freqa * lp->a;
	      fx1g = tr->rdta->freqg * lp->g;
	      fx1c = tr->rdta->freqc * lp->c;
	      fx1t = tr->rdta->freqt * lp->t;
	      suma = fx1a * lq->a + fx1c * lq->c + fx1g * lq->g + fx1t * lq->t;
	      fx2r = tr->rdta->freqa * lq->a + tr->rdta->freqg * lq->g;
	      fx2y = tr->rdta->freqc * lq->c + tr->rdta->freqt * lq->t;
	      fx1r = fx1a + fx1g;
	      fx1y = fx1c + fx1t;
	      sumc = (fx1r + fx1y) * (fx2r + fx2y);
	      sumb = fx1r * fx2r * tr->rdta->invfreqr + fx1y * fx2y * tr->rdta->invfreqy;
	      
	    }
	}



      abzz = zz * (suma - sumb);
      bczv = zv * (sumb - sumc);
      li = sumc + abzz + bczv;
      t3 = tr->rdta->xv * bczv;
      d  = abzz + t3;
      d2 = rat * (abzz*(rat-1.0) + t3*(rat * tr->rdta->xv - 1.0));
      
      temp = rat * d / li;
      w = *wptr++;
      slope += w *  temp;
      sum   += w * (temp * temp - d2/li);
      sumlr += w * log(li / (suma + 1.0E-300));
      lp++;
      lq++;
    }

    *sumlrptr = sumlr;
    return (sum > 1.0E-300) ? z*(-slope + sqrt(slope*slope + 3.841*sum))/sum
                            : 1.0;
  } /* sigma */


void describe (tree *tr, nodeptr p)
    /* print out information for one branch */
  { /* describe */
    double   z, s, sumlr;
    nodeptr  q;
    char    *nameptr;
    int      k, ch;

    q = p->back;
    printf("%4d          ", q->number - tr->mxtips);
    if (p->tip) {
      nameptr = p->name;
      k = nmlngth;
      while (ch = *nameptr++) {putchar(ch); k--;}
      while (--k >= 0) putchar(' ');
      }
    else {
      printf("%4d", p->number - tr->mxtips);
      for (k = 4; k < nmlngth; k++) putchar(' ');
      }

    z = q->z;
    if (z <= zmin) printf("    infinity");
    else printf("%15.5f", -log(z) * tr->rdta->fracchange);

    s = sigma(tr, q, & sumlr);
    printf("     (");
    if (z + s >= zmax) printf("     zero");
    else printf("%9.5f", (double) -log(z + s) * tr->rdta->fracchange);
    putchar(',');
    if (z - s <= zmin) printf("    infinity");
    else printf("%12.5f", (double) -log(z - s) * tr->rdta->fracchange);
    putchar(')');

    if      (sumlr > 2.995 ) printf(" **");
    else if (sumlr > 1.9205) printf(" *");
    putchar('\n');

    if (! p->tip) {
      describe(tr, p->next->back);
      describe(tr, p->next->next->back);
      }
  } /* describe */


void summarize (tree *tr)
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
    globTime = gettime() - globTime;
    fprintf(stderr, "Acc time global: %f\n", globTime);
    printf("Acc time global: %f\n", globTime);
    fprintf(stderr, "FLOPBS: %ld\n", flopBlocks);   
    fprintf(stderr,"depth: %d\n", tr->eqDepth);
    printf("FLOPPBS: %ld\n", flopBlocks);
    fprintf(stderr,"countEQ: %ld, countNEQ: %ld\n", countEQ, countNEQ);
    
  } /* summarize */


/*===========  This is a problem if tr->start->back is a tip!  ===========*/
/*  All routines should be contrived so that tr->start->back is not a tip */

char *treeString (char *treestr, tree *tr, nodeptr p, int form)
    /* write string with representation of tree */
    /*   form == 1 -> Newick tree */
    /*   form == 2 -> Prolog fact */
    /*   form == 3 -> PHYLIP tree */
  { /* treeString */
    double  x, z;
    char  *nameptr;
    int    c;

    if (p == tr->start->back) {
      if (form != treePHYLIP) {
        if (form == treeProlog) {
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

        (void) sprintf(treestr, "]%s", form == treeProlog ? ", " : " ");
        while (*treestr) treestr++;
        }
      }

    if (p->tip) {
      if (form != treePHYLIP) *treestr++ = '\'';
      nameptr = p->name;
      while (c = *nameptr++) {
        if (form != treePHYLIP) {if (c == '\'') *treestr++ = '\'';}
        else if (c == ' ') {c = '_';}
        *treestr++ = c;
        }
      if (form != treePHYLIP) *treestr++ = '\'';
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
      (void) sprintf(treestr, ":0.0%s\n", (form != treeProlog) ? ";" : ").");
      }
    else {
      z = p->z;
      if (z < zmin) z = zmin;
      x = -log(z) * tr->rdta->fracchange;
      (void) sprintf(treestr, ": %8.6f", x);  /* prolog needs the space */
      }

    while (*treestr) treestr++;     /* move pointer up to null termination */
    return  treestr;
  } /* treeString */


void treeOut (FILE *treefile, tree *tr, int form)
    /* write out file with representation of final tree */
  { /* treeOut */
    int    c;
    char  *cptr, *treestr;

    treestr = (char *) Malloc((tr->ntips * (nmlngth+32)) + 256);
    if (! treestr) {
      fprintf(stderr, "treeOut: Malloc failure\n");
      exit(1);
      }

    (void) treeString(treestr, tr, tr->start->back, form);
    cptr = treestr;
    while (c = *cptr++) putc(c, treefile);

    Free(treestr);
  } /* treeOut */


/*=======================================================================*/
/*                         Read a tree from a file                       */
/*=======================================================================*/


/*  1.0.A  Processing of quotation marks in comment removed
 */

int treeFinishCom (FILE *fp, char **strp)
  { /* treeFinishCom */
    int  ch;

    while ((ch = getc(fp)) != EOF && ch != ']') {
      if (strp != NULL) *(*strp)++ = ch;    /* save character  */
      if (ch == '[') {                      /* nested comment; find its end */
        if ((ch = treeFinishCom(fp, strp)) == EOF)  break;
        if (strp != NULL) *(*strp)++ = ch;  /* save closing ]  */
        }
      }

    if (strp != NULL) **strp = '\0';        /* terminate string  */
    return  ch;
  } /* treeFinishCom */


int treeGetCh (FILE *fp)         /* get next nonblank, noncomment character */
  { /* treeGetCh */
    int  ch;

    while ((ch = getc(fp)) != EOF) {
      if (whitechar(ch)) ;
      else if (ch == '[') {                   /* comment; find its end */
        if ((ch = treeFinishCom(fp, (char **) NULL)) == EOF)  break;
        }
      else  break;
      }

    return  ch;
  } /* treeGetCh */


boolean  treeLabelEnd (int ch)
  { /* treeLabelEnd */
    switch (ch) {
        case EOF:  case '\0':  case '\t':  case '\n':  case ' ':
        case ':':  case ',':   case '(':   case ')':   case '[':
        case ';':
          return TRUE;
        default:
          break;
        }
    return FALSE;
  } /* treeLabelEnd */


boolean  treeGetLabel (FILE *fp, char *lblPtr, int maxlen)
  { /* treeGetLabel */
    int      ch;
    boolean  done, quoted, lblfound;

    if (--maxlen < 0) lblPtr = (char *) NULL;  /* reserves space for '\0' */
    else if (lblPtr == NULL) maxlen = 0;

    ch = getc(fp);
    done = treeLabelEnd(ch);

    lblfound = ! done;
    quoted = (ch == '\'');
    if (quoted && ! done) {ch = getc(fp); done = (ch == EOF);}

    while (! done) {
      if (quoted) {
        if (ch == '\'') {ch = getc(fp); if (ch != '\'') break;}
        }

      else if (treeLabelEnd(ch)) break;

      else if (ch == '_') ch = ' ';  /* unquoted _ goes to space */

      if (--maxlen >= 0) *lblPtr++ = ch;
      ch = getc(fp);
      if (ch == EOF) break;
      }

    if (ch != EOF)  (void) ungetc(ch, fp);

    if (lblPtr != NULL) *lblPtr = '\0';

    return lblfound;
  } /* treeGetLabel */


boolean  treeFlushLabel (FILE *fp)
  { /* treeFlushLabel */
    return  treeGetLabel(fp, (char *) NULL, (int) 0);
  } /* treeFlushLabel */


int  treeFindTipByLabel (char  *str, tree *tr)
                     /*  str -- label string pointer */
  { /* treeFindTipByLabel */
    nodeptr  q;
    char    *nameptr;
    int      ch, i, n;
    boolean  found;

    for (n = 1; n <= tr->mxtips; n++) {
      q = tr->nodep[n];
      if (! (q->back)) {          /*  Only consider unused tips */
        i = 0;
        nameptr = q->name;
        while ((found = (str[i++] == (ch = *nameptr++))) && ch) ;
        if (found) return n;
        }
      }

    printf("ERROR: Cannot find tree species: %s\n", str);

    return  0;
  } /* treeFindTipByLabel */


int  treeFindTipName (FILE *fp, tree *tr)
  { /* treeFindTipName */
    char    *nameptr, str[nmlngth+2];
    int      n;

    if (tr->prelabeled) {
      if (treeGetLabel(fp, str, nmlngth+2))
        n = treeFindTipByLabel(str, tr);
      else
        n = 0;
      }

    else if (tr->ntips < tr->mxtips) {
      n = tr->ntips + 1;
      nameptr = tr->nodep[n]->name;
      if (! treeGetLabel(fp, nameptr, nmlngth+1)) n = 0;
      }

    else {
      n = 0;
      }

    return  n;
  } /* treeFindTipName */


void  treeEchoContext (FILE *fp1, FILE *fp2, int n)
 { /* treeEchoContext */
   int      ch;
   boolean  waswhite;

   waswhite = TRUE;

   while (n > 0 && ((ch = getc(fp1)) != EOF)) {
     if (whitechar(ch)) {
       ch = waswhite ? '\0' : ' ';
       waswhite = TRUE;
       }
     else {
       waswhite = FALSE;
       }

     if (ch > '\0') {putc(ch, fp2); n--;}
     }
 } /* treeEchoContext */


boolean treeProcessLength (FILE *fp, double *dptr)
  { /* treeProcessLength */
    int  ch;

    if ((ch = treeGetCh(fp)) == EOF)  return FALSE;    /*  Skip comments */
    (void) ungetc(ch, fp);

    if (fscanf(fp, "%lf", dptr) != 1) {
      printf("ERROR: treeProcessLength: Problem reading branch length\n");
      treeEchoContext(fp, stdout, 40);
      printf("\n");
      return  FALSE;
      }

    return  TRUE;
  } /* treeProcessLength */


boolean  treeFlushLen (FILE  *fp)
  { /* treeFlushLen */
    double  dummy;
    int     ch;

    if ((ch = treeGetCh(fp)) == ':') return treeProcessLength(fp, & dummy);

    if (ch != EOF) (void) ungetc(ch, fp);
    return TRUE;
  } /* treeFlushLen */


boolean  treeNeedCh (FILE *fp, int c1, char *where)
  { /* treeNeedCh */
    int  c2;

    if ((c2 = treeGetCh(fp)) == c1)  return TRUE;

    printf("ERROR: Expecting '%c' %s tree; found:", c1, where);
    if (c2 == EOF) {
      printf("End-of-File");
      }
    else {
      ungetc(c2, fp);
      treeEchoContext(fp, stdout, 40);
      }
    putchar('\n');
    return FALSE;
  } /* treeNeedCh */


boolean  addElementLen (FILE *fp, tree *tr, nodeptr p)
  { /* addElementLen */
    double   z, branch;
    nodeptr  q;
    int      n, ch;

    if ((ch = treeGetCh(fp)) == '(') {     /*  A new internal node */
      n = (tr->nextnode)++;
      if (n > 2*(tr->mxtips) - 2) {
        if (tr->rooted || n > 2*(tr->mxtips) - 1) {
          printf("ERROR: Too many internal nodes.  Is tree rooted?\n");
          printf("       Deepest splitting should be a trifurcation.\n");
          return FALSE;
          }
        else {
          tr->rooted = TRUE;
          }
        }
      q = tr->nodep[n];
      if (! addElementLen(fp, tr, q->next))        return FALSE;
      if (! treeNeedCh(fp, ',', "in"))             return FALSE;
      if (! addElementLen(fp, tr, q->next->next))  return FALSE;
      if (! treeNeedCh(fp, ')', "in"))             return FALSE;
      (void) treeFlushLabel(fp);
      }

    else {                               /*  A new tip */
      ungetc(ch, fp);
      if ((n = treeFindTipName(fp, tr)) <= 0)          return FALSE;
      q = tr->nodep[n];
      if (tr->start->number > n)  tr->start = q;
      (tr->ntips)++;
      }                                  /* End of tip processing */

    if (tr->userlen) {
      if (! treeNeedCh(fp, ':', "in"))             return FALSE;
      if (! treeProcessLength(fp, & branch))       return FALSE;
      z = exp(-branch / tr->rdta->fracchange);
      if (z > zmax)  z = zmax;
      hookup(p, q, z);
      }
    else {
      if (! treeFlushLen(fp))                       return FALSE;
      hookup(p, q, defaultz);
      }

    return TRUE;
  } /* addElementLen */


int saveTreeCom (char  **comstrp)
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


boolean processTreeCom (FILE *fp, tree *tr)
  { /* processTreeCom */
    int   text_started, functor_read, com_open;

    /*  Accept prefatory "phylip_tree(" or "pseudoNewick("  */

    functor_read = text_started = 0;
    (void) fscanf(fp, " p%nhylip_tree(%n", & text_started, & functor_read);
    if (text_started && ! functor_read) {
      (void) fscanf(fp, "seudoNewick(%n", & functor_read);
      if (! functor_read) {
        printf("Start of tree 'p...' not understood.\n");
        return FALSE;
        }
      }

    com_open = 0;
    (void) fscanf(fp, " [%n", & com_open);

    if (com_open) {                                  /* comment; read it */
      char  com[1024], *com_end;

      com_end = com;
      if (treeFinishCom(fp, & com_end) == EOF) {     /* omits enclosing []s */
        printf("Missing end of tree comment\n");
        return FALSE;
        }

      (void) readKeyValue(com, likelihood_key, "%lg",
                               (void *) &(tr->likelihood));
      (void) readKeyValue(com, opt_level_key,  "%d",
                               (void *) &(tr->opt_level));
      (void) readKeyValue(com, smoothed_key,   "%d",
                               (void *) &(tr->smoothed));

      if (functor_read) (void) fscanf(fp, " ,");   /* remove trailing comma */
      }

    return (functor_read > 0);
  } /* processTreeCom */


nodeptr uprootTree (tree *tr, nodeptr p)
  { /* uprootTree */
    nodeptr  q, r, s, start;
    int      n;

    if (p->tip || p->back) {
      printf("ERROR: Unable to uproot tree.\n");
      printf("       Inappropriate node marked for removal.\n");
      return (nodeptr) NULL;
      }

    n = --(tr->nextnode);               /* last internal node added */
    if (n != tr->mxtips + tr->ntips - 1) {
      printf("ERROR: Unable to uproot tree.  Inconsistent\n");
      printf("       number of tips and nodes for rooted tree.\n");
      return (nodeptr) NULL;
      }

    q = p->next->back;                  /* remove p from tree */
    r = p->next->next->back;
    hookup(q, r, tr->userlen ? (q->z * r->z) : defaultz);

    start = (r->tip || (! q->tip)) ? r : r->next->next->back;

    if (tr->ntips > 2 && p->number != n) {
      q = tr->nodep[n];            /* transfer last node's conections to p */
      r = q->next;
      s = q->next->next;
      hookup(p,             q->back, q->z);   /* move connections to p */
      hookup(p->next,       r->back, r->z);
      hookup(p->next->next, s->back, s->z);
      if (start->number == q->number) start = start->back->back;
      q->back = r->back = s->back = (nodeptr) NULL;
      }
    else {
      p->back = p->next->back = p->next->next->back = (nodeptr) NULL;
      }

    tr->rooted = FALSE;
    return  start;
  } /* uprootTree */


boolean treeReadLen (FILE *fp, tree *tr)
  { /* treeReadLen */
    nodeptr  p;
    int      i, ch;
    boolean  is_fact;

    for (i = 1; i <= tr->mxtips; i++) tr->nodep[i]->back = (node *) NULL;
    tr->start       = tr->nodep[tr->mxtips];
    tr->ntips       = 0;
    tr->nextnode    = tr->mxtips + 1;
    tr->opt_level   = 0;
    tr->log_f_valid = 0;
    tr->smoothed    = FALSE;
    tr->rooted      = FALSE;

    is_fact = processTreeCom(fp, tr);

    p = tr->nodep[(tr->nextnode)++];
    if (! treeNeedCh(fp, '(', "at start of"))       return FALSE;
    if (! addElementLen(fp, tr, p))                 return FALSE;
    if (! treeNeedCh(fp, ',', "in"))                return FALSE;
    if (! addElementLen(fp, tr, p->next))           return FALSE;
    if (! tr->rooted) {
      if ((ch = treeGetCh(fp)) == ',') {        /*  An unrooted format */
        if (! addElementLen(fp, tr, p->next->next)) return FALSE;
        }
      else {                                    /*  A rooted format */
        tr->rooted = TRUE;
        if (ch != EOF)  (void) ungetc(ch, fp);
        }
      }
    else {
      p->next->next->back = (nodeptr) NULL;
      }
    if (! treeNeedCh(fp, ')', "in"))                return FALSE;
    (void) treeFlushLabel(fp);
    if (! treeFlushLen(fp))                         return FALSE;
    if (is_fact) {
      if (! treeNeedCh(fp, ')', "at end of"))       return FALSE;
      if (! treeNeedCh(fp, '.', "at end of"))       return FALSE;
      }
    else {
      if (! treeNeedCh(fp, ';', "at end of"))       return FALSE;
      }

    if (tr->rooted) {
      p->next->next->back = (nodeptr) NULL;
      tr->start = uprootTree(tr, p->next->next);
      if (! tr->start)                              return FALSE;
      }
    else {
      tr->start = p->next->next->back;  /* This is start used by treeString */
      }

    return  (initrav(tr, tr->start) && initrav(tr, tr->start->back));
  } /* treeReadLen */


/*=======================================================================*/
/*                        Read a tree from a string                      */
/*=======================================================================*/


#if Master || Slave
int str_treeFinishCom (char **treestrp, char **strp)
                      /* treestrp -- tree string pointer */
                      /* strp -- comment string pointer */
  { /* str_treeFinishCom */
    int  ch;

    while ((ch = *(*treestrp)++) != NULL && ch != ']') {
      if (strp != NULL) *(*strp)++ = ch;    /* save character  */
      if (ch == '[') {                      /* nested comment; find its end */
        if ((ch = str_treeFinishCom(treestrp)) == NULL)  break;
        if (strp != NULL) *(*strp)++ = ch;  /* save closing ]  */
        }
      }
    if (strp != NULL) **strp = '\0';        /* terminate string  */
    return  ch;
  } /* str_treeFinishCom */


int str_treeGetCh (char **treestrp)
    /* get next nonblank, noncomment character */
  { /* str_treeGetCh */
    int  ch;
    
    while ((ch = *(*treestrp)++) != NULL) {
      if (whitechar(ch)) ;
      else if (ch == '[') {                  /* comment; find its end */
        if ((ch = str_treeFinishCom(treestrp, (char *) NULL)) == NULL)  break;
        }
      else  break;
      }

    return  ch;
  } /* str_treeGetCh */


boolean  str_treeGetLabel (char **treestrp, char *lblPtr, int maxlen)
  { /* str_treeGetLabel */
    int      ch;
    boolean  done, quoted, lblfound;

    if (--maxlen < 0) lblPtr = (char *) NULL;  /* reserves space for '\0' */
    else if (lblPtr == NULL) maxlen = 0;

    ch = *(*treestrp)++;
    done = treeLabelEnd(ch);

    lblfound = ! done;
    quoted = (ch == '\'');
    if (quoted && ! done) {ch = *(*treestrp)++; done = (ch == '\0');}

    while (! done) {
      if (quoted) {
        if (ch == '\'') {ch = *(*treestrp)++; if (ch != '\'') break;}
        }

      else if (treeLabelEnd(ch)) break;

      else if (ch == '_') ch = ' ';  /* unquoted _ goes to space */

      if (--maxlen >= 0) *lblPtr++ = ch;
      ch = *(*treestrp)++;
      if (ch == '\0') break;
      }

    (*treestrp)--;

    if (lblPtr != NULL) *lblPtr = '\0';

    return lblfound;
  } /* str_treeGetLabel */


boolean  str_treeFlushLabel (char **treestrp)
  { /* str_treeFlushLabel */
    return  str_treeGetLabel(treestrp, (char *) NULL, (int) 0);
  } /* str_treeFlushLabel */


int  str_treeFindTipName (char **treestrp, tree *tr)
  { /* str_treeFindTipName */
    nodeptr  q;
    char    *nameptr, str[nmlngth+2];
    int      ch, i, n;

    if (tr->prelabeled) {
      if (str_treeGetLabel(treestrp, str, nmlngth+2))
        n = treeFindTipByLabel(str, tr);
      else
        n = 0;
      }

    else if (tr->ntips < tr->mxtips) {
      n = tr->ntips + 1;
      nameptr = tr->nodep[n]->name;
      if (! str_treeGetLabel(treestrp, nameptr, nmlngth+1)) n = 0;
      }

    else {
      n = 0;
      }

    return  n;
  } /* str_treeFindTipName */


boolean str_treeProcessLength (char **treestrp, double *dptr)
  { /* str_treeProcessLength */
    int     used;

    if(! str_treeGetCh(treestrp))  return FALSE;    /*  Skip comments */
    (*treestrp)--;

    if (sscanf(*treestrp, "%lf%n", dptr, & used) != 1) {
      printf("ERROR: str_treeProcessLength: Problem reading branch length\n");
      printf("%40s\n", *treestrp);
      *dptr = 0.0;
      return FALSE;
      }
    else {
      *treestrp += used;
      }

    return  TRUE;
  } /* str_treeProcessLength */


boolean  str_treeFlushLen (char **treestrp)
  { /* str_treeFlushLen */
    int  ch;

    if ((ch = str_treeGetCh(treestrp)) == ':')
      return str_treeProcessLength(treestrp, (double *) NULL);
    else {
      (*treestrp)--;
      return TRUE;
      }
  } /* str_treeFlushLen */


boolean  str_treeNeedCh (char **treestrp, int c1, char *where)
  { /* str_treeNeedCh */
    int  c2, i;

    if ((c2 = str_treeGetCh(treestrp)) == c1)  return TRUE;

    printf("ERROR: Missing '%c' %s tree; ", c1, where);
    if (c2 == '\0') 
      printf("end-of-string");
    else {
      putchar('"');
      for (i = 24; i-- && (c2 != '\0'); c2 = *(*treestrp)++)  putchar(c2);
      putchar('"');
      }

    printf(" found instead\n");
    return FALSE;
  } /* str_treeNeedCh */


boolean  str_addElementLen (char **treestrp, tree *tr, nodeptr p)
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
          return  FALSE;
          }
        else {
          tr->rooted = TRUE;
          }
        }
      q = tr->nodep[n];
      if (! str_addElementLen(treestrp, tr, q->next))          return FALSE;
      if (! str_treeNeedCh(treestrp, ',', "in"))               return FALSE;
      if (! str_addElementLen(treestrp, tr, q->next->next))    return FALSE;
      if (! str_treeNeedCh(treestrp, ')', "in"))               return FALSE;
      if (! str_treeFlushLabel(treestrp))                      return FALSE;
      }

    else {                           /*  A new tip */
      n = str_treeFindTipName(treestrp, tr, ch);
      if (n <= 0) return FALSE;
      q = tr->nodep[n];
      if (tr->start->number > n)  tr->start = q;
      (tr->ntips)++;
      }                              /* End of tip processing */

    /*  Master and Slave always use lengths */

    if (! str_treeNeedCh(treestrp, ':', "in"))                 return FALSE;
    if (! str_treeProcessLength(treestrp, & branch))           return FALSE;
    z = exp(-branch / tr->rdta->fracchange);
    if (z > zmax)  z = zmax;
    hookup(p, q, z);

    return  TRUE;
  } /* str_addElementLen */


boolean str_processTreeCom(tree *tr, char **treestrp)
  { /* str_processTreeCom */
    char  *com, *com_end;
    int  text_started, functor_read, com_open;

    com = *treestrp;

    functor_read = text_started = 0;
    sscanf(com, " p%nhylip_tree(%n", & text_started, & functor_read);
    if (functor_read) {
      com += functor_read;
      }
    else if (text_started) {
      com += text_started;
      sscanf(com, "seudoNewick(%n", & functor_read);
      if (! functor_read) {
        printf("Start of tree 'p...' not understood.\n");
        return  FALSE;
        }
      else {
        com += functor_read;
        }
      }

    com_open = 0;
    sscanf(com, " [%n", & com_open);
    com += com_open;

    if (com_open) {                              /* comment; read it */
	if (!(com_end = strchr(com, ']'))) {
        printf("Missing end of tree comment.\n");
        return  FALSE;
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
        sscanf(com_end, " ,%n", & text_started);
        com_end += text_started;
        }

      *treestrp = com_end;
      }

    return (functor_read > 0);
  } /* str_processTreeCom */


boolean str_treeReadLen (char *treestr, tree *tr)
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

    is_fact = str_processTreeCom(tr, & treestr);

    p = tr->nodep[(tr->nextnode)++];
    if (! str_treeNeedCh(& treestr, '(', "at start of"))       return FALSE;
    if (! str_addElementLen(& treestr, tr, p))                 return FALSE;
    if (! str_treeNeedCh(& treestr, ',', "in"))                return FALSE;
    if (! str_addElementLen(& treestr, tr, p->next))           return FALSE;
    if (! tr->rooted) {
      if (str_treeGetCh(& treestr) == ',') {        /*  An unrooted format */
        if (! str_addElementLen(& treestr, tr, p->next->next)) return FALSE;
        }
      else {                                       /*  A rooted format */
        p->next->next->back = (nodeptr) NULL;
        tr->rooted = TRUE;
        treestr--;
        }
      }
    if (! str_treeNeedCh(& treestr, ')', "in"))                return FALSE;
    if (! str_treeFlushLabel(& treestr))                       return FALSE;
    if (! str_treeFlushLen(& treestr))                         return FALSE;
    if (is_fact) {
      if (! str_treeNeedCh(& treestr, ')', "at end of"))       return FALSE;
      if (! str_treeNeedCh(& treestr, '.', "at end of"))       return FALSE;
      }
    else {
      if (! str_treeNeedCh(& treestr, ';', "at end of"))       return FALSE;
      }

    if (tr->rooted)  if (! uprootTree(tr, p->next->next))     return FALSE;
    tr->start = p->next->next->back;  /* This is start used by treeString */

    return  (initrav(tr, tr->start) && initrav(tr, tr->start->back));
  } /* str_treeReadLen */
#endif


boolean treeEvaluate (tree *tr, bestlist *bt)       /* Evaluate a user tree */
  { /* treeEvaluate */

    if (Slave || ! tr->userlen) {
      if (! smoothTree(tr, 4 * smoothings)) return FALSE;
      }

    if (evaluate(tr, tr->start) == badEval)  return FALSE;

#   if ! Slave
      (void) saveBestTree(bt, tr);
#   endif
    return TRUE;
  } /* treeEvaluate */


#if Master || Slave
FILE *freopen_pid (char *filenm, char *mode, FILE *stream)
  { /* freopen_pid */
    char scr[512];

    (void) sprintf(scr, "%s.%d", filenm, getpid());
    return  freopen(scr, mode, stream);
  } /* freopen_pid */
#endif


boolean  showBestTrees (bestlist *bt, tree *tr, analdef *adef, FILE *treefile)
  { /* showBestTrees */
    int     rank;

    for (rank = 1; rank <= bt->nvalid; rank++) {
      if (rank > 1) {
        if (rank != recallBestTree(bt, rank, tr))  break;
        }
      if (evaluate(tr, tr->start) == badEval) return FALSE;
      if (tr->outgrnode->back)  tr->start = tr->outgrnode;
      printTree(tr, adef);
      summarize(tr);
      if (treefile)  treeOut(treefile, tr, adef->trout);
      }

    return TRUE;
  } /* showBestTrees */


boolean cmpBestTrees (bestlist *bt, tree *tr)
  { /* cmpBestTrees */
    double  sum, sum2, sd, temp, wtemp, bestscore;
    double *log_f0, *log_f0_ptr;      /* Save a copy of best log_f */
    double *log_f_ptr;
    int     i, j, num, besttips;

    num = bt->nvalid;
    if ((num <= 1) || (tr->cdta->wgtsum <= 1)) return TRUE;

    if (! (log_f0 = (double *) Malloc(sizeof(double) * tr->cdta->endsite))) {
      printf("ERROR: cmpBestTrees unable to obtain space for log_f0\n");
      return FALSE;
      }

    printf("Tree      Ln L        Diff Ln L       Its S.D.");
    printf("   Significantly worse?\n\n");

    for (i = 1; i <= num; i++) {
      if (i != recallBestTree(bt, i, tr))  break;
      if (! (tr->log_f_valid))  {
        if (evaluate(tr, tr->start) == badEval) return FALSE;
        }

      printf("%3d%14.5f", i, tr->likelihood);
      if (i == 1) {
        printf("  <------ best\n");
        besttips = tr->ntips;
        bestscore = tr->likelihood;
        log_f0_ptr = log_f0;
        log_f_ptr  = tr->log_f;
        for (j = 0; j < tr->cdta->endsite; j++)  *log_f0_ptr++ = *log_f_ptr++;
        }
      else if (tr->ntips != besttips)
        printf("  (different number of species)\n");
      else {
        sum = sum2 = 0.0;
        log_f0_ptr = log_f0;
        log_f_ptr  = tr->log_f;
        for (j = 0; j < tr->cdta->endsite; j++) {
          temp  = *log_f0_ptr++ - *log_f_ptr++;
          wtemp = tr->cdta->aliaswgt[j] * temp;
          sum  += wtemp;
          sum2 += wtemp * temp;
          }
        sd = sqrt( tr->cdta->wgtsum * (sum2 - sum*sum / tr->cdta->wgtsum)
                                    / (tr->cdta->wgtsum - 1) );
        printf("%14.5f%14.4f", tr->likelihood - bestscore, sd);
        printf("           %s\n", (sum > 1.95996 * sd) ? "Yes" : " No");
        }
      }

    Free(log_f0);
    printf("\n\n");

    return TRUE;
  } /* cmpBestTrees */


boolean  makeUserTree (tree *tr, bestlist *bt, analdef *adef)
  { /* makeUserTree */
    char   filename[128];
    FILE  *treefile;
    int    nusertrees, which;

    nusertrees = adef->numutrees;

    printf("User-defined %s:\n\n", (nusertrees == 1) ? "tree" : "trees");

    treefile = adef->trout ? fopen_pid("treefile", "w", filename) : (FILE *) NULL;

    for (which = 1; which <= nusertrees; which++) {
      if (! treeReadLen(INFILE, tr)) return FALSE;
      if (! treeEvaluate(tr, bt))    return FALSE;
      if (tr->global <= 0) {
        if (tr->outgrnode->back)  tr->start = tr->outgrnode;
        printTree(tr, adef);
        summarize(tr);
        if (treefile)  treeOut(treefile, tr, adef->trout);
        }
      else {
        printf("%6d:  Ln Likelihood =%14.5f\n", which, tr->likelihood);
        }
      }

    if (tr->global > 0) {
      putchar('\n');
      if (! recallBestTree(bt, 1, tr))  return FALSE;
      printf("      Ln Likelihood =%14.5f\n", tr->likelihood);
      if (! optimize(tr, tr->global, bt))  return FALSE;
      if (tr->outgrnode->back)  tr->start = tr->outgrnode;
      printTree(tr, adef);
      summarize(tr);
      if (treefile)  treeOut(treefile, tr, adef->trout);
      }

    if (treefile) {
      (void) fclose(treefile);
      printf("Tree also written to %s\n", filename);
      }

    putchar('\n');

    (void) cmpBestTrees(bt, tr);
    return TRUE;
  } /* makeUserTree */


#if Slave
boolean slaveTreeEvaluate (tree *tr, bestlist *bt)
  { /* slaveTreeEvaluate */
    boolean done;

    do {
       requestForWork();
       if (! receiveTree(& comm_master, tr))        return FALSE;
       done = tr->likelihood > 0.0;
       if (! done) {
         if (! treeEvaluate(tr, bt))                return FALSE;
         if (! sendTree(& comm_master, tr))         return FALSE;
         }
       } while (! done);

    return TRUE;
  } /* slaveTreeEvaluate */
#endif


double randum (long  *seed)
    /* random number generator, modified to use 12 bit chunks */
  { /* randum */
    long  sum, mult0, mult1, seed0, seed1, seed2, newseed0, newseed1, newseed2;

    mult0 = 1549;
    seed0 = *seed & 4095;
    sum  = mult0 * seed0;
    newseed0 = sum & 4095;
    sum >>= 12;
    seed1 = (*seed >> 12) & 4095;
    mult1 =  406;
    sum += mult0 * seed1 + mult1 * seed0;
    newseed1 = sum & 4095;
    sum >>= 12;
    seed2 = (*seed >> 24) & 255;
    sum += mult0 * seed2 + mult1 * seed1;
    newseed2 = sum & 255;

    *seed = newseed2 << 24 | newseed1 << 12 | newseed0;
    return  0.00390625 * (newseed2
                          + 0.000244140625 * (newseed1
                                              + 0.000244140625 * newseed0));
  } /* randum */


boolean makeDenovoTree (tree *tr, bestlist *bt, analdef *adef)
  { /* makeDenovoTree */
    char   filename[128];
    FILE  *treefile;
    nodeptr  p;
    int  *enterorder;      /*  random entry order */
    int  i, j, k, nextsp, newsp, maxtrav, tested;

    double randum();


    enterorder = (int *) Malloc(sizeof(int) * (tr->mxtips + 1));
    if (! enterorder) {
       fprintf(stderr, "makeDenovoTree: Malloc failure for enterorder\n");
       return 0;
       }

    if (adef->restart) {
      printf("Restarting from tree with the following sequences:\n");
      tr->userlen = TRUE;
      if (! treeReadLen(INFILE, tr))          return FALSE;
      if (! smoothTree(tr, smoothings))       return FALSE;
      if (evaluate(tr, tr->start) == badEval) return FALSE;
      if (saveBestTree(bt, tr) < 1)           return FALSE;

      for (i = 1, j = tr->ntips; i <= tr->mxtips; i++) { /* find loose tips */
        if (! tr->nodep[i]->back) {
          enterorder[++j] = i;
          }
        else {
          printf("   %s\n", tr->nodep[i]->name);

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

    if (adef->jumble) for (i = tr->ntips + 1; i <= tr->mxtips; i++) {
      j = randum(&(adef->jumble))*(tr->mxtips - tr->ntips) + tr->ntips + 1;
      k = enterorder[j];
      enterorder[j] = enterorder[i];
      enterorder[i] = k;
      }

    bt->numtrees = 1;
    if (tr->ntips < tr->mxtips)  printf("Adding species:\n");

    if (tr->ntips == 0) {
      for (i = 1; i <= 3; i++) {
        printf("   %s\n", tr->nodep[enterorder[i]]->name);
        }
      tr->nextnode = tr->mxtips + 1;
      if (! buildSimpleTree(tr, enterorder[1], enterorder[2], enterorder[3]))
        return FALSE;
      }

    while (tr->ntips < tr->mxtips || tr->opt_level < tr->global) {
      maxtrav = (tr->ntips == tr->mxtips) ? tr->global : tr->partswap;
      if (maxtrav > tr->ntips - 3)  maxtrav = tr->ntips - 3;

      if (tr->opt_level >= maxtrav) {
        nextsp = ++(tr->ntips);
        newsp = enterorder[nextsp];
        p = tr->nodep[newsp];
        printf("   %s\n", p->name);

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
                             1, tr->ntips - 2, bt, adef->qadd);
        if (tested == badRear) return FALSE;
        bt->numtrees += tested;

#       if Master
          getReturnedTrees(tr, bt, tested);
#       endif

        printf("      Tested %d alternative trees\n", tested);
	


        (void) recallBestTree(bt, 1, tr);
        if (! tr->smoothed) {
          if (! smoothTree(tr, smoothings))        return FALSE;
          if (evaluate(tr, tr->start) == badEval)  return FALSE;
          (void) saveBestTree(bt, tr);
          }

        if (tr->ntips == 4)  tr->opt_level = 1;  /* All 4 taxon trees done */
        maxtrav = (tr->ntips == tr->mxtips) ? tr->global : tr->partswap;
        if (maxtrav > tr->ntips - 3)  maxtrav = tr->ntips - 3;
        }

      printf("      Ln Likelihood =%14.5f\n", tr->likelihood);
      if (! optimize(tr, maxtrav, bt)) return FALSE;
      }

    printf("\nExamined %d %s\n", bt->numtrees,
                                 bt->numtrees != 1 ? "trees" : "tree");

    treefile = adef->trout ? fopen_pid("treefile", "w", filename) : (FILE *) NULL;
    (void) showBestTrees(bt, tr, adef, treefile);
    if (treefile) {
      (void) fclose(treefile);
      printf("Tree also written to %s\n\n", filename);
      }

    (void) cmpBestTrees(bt, tr);

#   if DeleteCheckpointFile
      unlink_pid(checkpointname);
#   endif

    Free(enterorder);

    return TRUE;
  } /* makeDenovoTree */

/*==========================================================================*/
/*                             "main" routine                               */
/*==========================================================================*/

#if Sequential
  main ()
#else
  slave ()
#endif
  { /* DNA Maximum Likelihood */
#   if Master
      int starttime, inputtime, endtime;
#   endif

#   if Master || Slave
      int my_id, nprocs, type, from, sz;
      char *msg;
#   endif

    analdef      *adef;
    rawdata      *rdta;
    cruncheddata  *cdta;
    tree         *tr;    /*  current tree */
    bestlist     *bt;    /*  topology of best found tree */

    globTime = gettime();

#   if Debug
      {
        char debugfilename[128];
        debug = fopen_pid("dnaml_debug", "w", debugfilename);
        }
#   endif

#   if Master
      starttime = p4_clock();
      nprocs = p4_num_total_slaves();

      if ((OUTFILE = freopen_pid("master.out", "w", stdout)) == NULL) {
        fprintf(stderr, "Could not open output file\n");
        exit(1);
        }

      /* Receive input file name from host */
      type = DNAML_FILE_NAME;
      from = DNAML_HOST_ID;
      msg  = NULL;
      p4_recv(& type, & from, & msg, & sz);
      if ((INFILE = fopen(msg, "r")) == NULL) {
        fprintf(stderr, "master could not open input file %s\n", msg);
        exit(1);
        }
      p4_msg_free(msg);

      open_link(& comm_slave);
#   endif

#  if DNAML_STEP
      begin_step_time = starttime;
#  endif

#   if Slave
      my_id = p4_get_my_id();
      nprocs = p4_num_total_slaves();

      /* Receive input file name from host */
      type = DNAML_FILE_NAME;
      from = DNAML_HOST_ID;
      msg  = NULL;
      p4_recv(& type, & from, & msg, & sz);
      if ((INFILE = fopen(msg, "r")) == NULL) {
        fprintf(stderr, "slave could not open input file %s\n",msg);
        exit(1);
        }
      p4_msg_free(msg);

#     ifdef P4DEBUG
        if ((OUTFILE = freopen_pid("slave.out", "w", stdout)) == NULL) {
          fprintf(stderr, "Could not open output file\n");
          exit(1);
          }
#     else
        if ((OUTFILE = freopen("/dev/null", "w", stdout)) == NULL) {
          fprintf(stderr, "Could not open output file\n");
          exit(1);
          }
#     endif

      open_link(& comm_master);
#   endif


/*  Get data structure memory  */

    if (! (adef = (analdef *) Malloc(sizeof(analdef)))) {
      printf("ERROR: Unable to get memory for analysis definition\n\n");
      return 1;
      }

    if (! (rdta = (rawdata *) Malloc(sizeof(rawdata)))) {
      printf("ERROR: Unable to get memory for raw DNA\n\n");
      return 1;
      }

    if (! (cdta = (cruncheddata *) Malloc(sizeof(cruncheddata)))) {
      printf("ERROR: Unable to get memory for crunched DNA\n\n");
      return 1;
      }

    if ((tr = (tree *)     Malloc(sizeof(tree))) &&
        (bt = (bestlist *) Malloc(sizeof(bestlist)))) ;
    else {
      printf("ERROR: Unable to get memory for trees\n\n");
      return 1;
      }
    bt->ninit = 0;

    if (! getinput(adef, rdta, cdta, tr))                            return 1;

#   if Master
      inputtime = p4_clock();
      printf("Input time %d milliseconds\n", inputtime - starttime);
      REPORT_STEP_TIME;
#   endif

#   if Slave
      (void) fclose(INFILE);
#   endif

/*  The material below would be a loop over jumbles and/or boots */

    if (! makeweights(adef, rdta, cdta))                             return 1;
    if (! makevalues(rdta, cdta))                                    return 1;
    if (adef->empf && ! empiricalfreqs(rdta, cdta))                  return 1;
    reportfreqs(adef, rdta);
    if (! linkdata2tree(rdta, cdta, tr))                             return 1;

    if (! linkxarray(3, 3, cdta->endsite, & freextip, & usedxtip))   return 1;
    if (! setupnodex(tr))                                            return 1;


    /* AxML modification start*/

    if( ! setupEqualityVector(tr))                                    return 1;

    /* initialize equality vectors */     


    /* AxML modification end*/

#   if Slave
      if (! slaveTreeEvaluate(tr, bt))                               return 1;
#   else
      if (! initBestTree(bt, adef->nkeep, tr->mxtips, tr->cdta->endsite))    return 1;
      if (! adef->usertree) {
        if (! makeDenovoTree(tr, bt, adef))                          return 1;
        }
      else {
        if (! makeUserTree(tr, bt, adef))                            return 1;
        }
      if (! freeBestTree(bt))                                        return 1;
#   endif

/*  Endpoint for jumble and/or boot loop */

#   if Master
      tr->likelihood = 1.0;             /* terminate slaves */
      (void) sendTree(& comm_slave, tr);
#   endif

    freeTree(tr);

#   if Master
      close_link(& comm_slave);
      (void) fclose(INFILE);

      REPORT_STEP_TIME;
      endtime = p4_clock();
      printf("Execution time %d milliseconds\n", endtime - inputtime);
      (void) fclose(OUTFILE);
#   endif

#   if Slave
      close_link(& comm_master);
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
