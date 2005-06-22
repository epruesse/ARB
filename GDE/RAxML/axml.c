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
 *  Proceedings of IPDPS2005,  Denver, Colorado, April 2005.
 *  
 *  AND
 *
 *  Alexandros Stamatakis, Thomas Ludwig, and Harald Meier: "RAxML-III: A Fast Program for Maximum Likelihood-based Inference of Large Phylogenetic Trees". 
 *  In Bioinformatics 21(4):456-463.
 *
 *
 * 
 */



#ifdef WIN32
   #include <sys/timeb.h>
   #include "getopt.h"
   #include <direct.h>
#else
   #include <sys/times.h>
   #include <sys/types.h>
   #include <sys/time.h>
   #include <unistd.h> 
#endif

#include <math.h>
#include <time.h> 
#include <stdlib.h>
#include <stdio.h>



#include "axml.h"
#ifdef MKL
#include "mkl_vml.h"
#endif







/*  Global variables */


double gettime()
{
#ifdef WIN32
   struct _timeb timebuffer;
   _ftime(&timebuffer);
   return timebuffer.time + timebuffer.millitm * 0.000001;
#else
   struct timeval ttime;
    gettimeofday(&ttime , NULL);
    return ttime.tv_sec + ttime.tv_usec * 0.000001;
#endif
}


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

    if (! (tpl = (topol *) malloc(sizeof(topol))) || 
        ! (tpl->links = (connptr) malloc((2*maxtips-3) * sizeof(connect))))
      {
	printf("ERROR: Unable to get topology memory");
	tpl = (topol *) NULL;
      }
    else 
      {
	tpl->likelihood  = unlikely;
	tpl->start       = (node *) NULL;
	tpl->nextlink    = 0;
	tpl->ntips       = 0;
	tpl->nextnode    = 0;
	tpl->opt_level   = 0;     /* degree of branch swapping explored */
	tpl->scrNum      = 0;     /* position in sorted list of scores */
	tpl->tplNum      = 0;     /* position in sorted list of trees */
	tpl->prelabeled  = TRUE;
	tpl->smoothed    = FALSE; /* branch optimization converged? */
      }

    return  tpl;
  } /* setupTopol */


void  freeTopol (topol *tpl)
  { /* freeTopol */
    free(tpl->links);
    free(tpl);
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

   
  } /* saveTree */


void copyTopol (topol *tpl1, topol *tpl2)
  { /* copyTopol */
    connptr  r1, r2, r10, r20;
   
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


boolean restoreTreeRecursive (topol *tpl, tree *tr)
  { /* restoreTree */
    void  hookup();
    boolean  initrav();

    connptr  r;
    nodeptr  p, p0;
    
    int  i;



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

   
    return TRUE;
    
  } /* restoreTree */




boolean restoreTree (topol *tpl, tree *tr)
  { /* restoreTree */
    /*void  hookup();
      boolean  initrav();*/

    connptr  r;
    nodeptr  p, p0;
    
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
      bt->byScore = (topol **) malloc((newkeep+1) * sizeof(topol *));
      bt->byTopol = (topol **) malloc((newkeep+1) * sizeof(topol *));
      if (! bt->byScore || ! bt->byTopol) {
        printf( "initBestTree: malloc failure\n");
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

   

    tpl->scrNum = scrNum;
    tpl->tplNum = tplNum;
    bt->byTopol[tplNum] = bt->byScore[scrNum] = tpl;
    bt->byScore[0] = reuse;

    if (scrNum == 1)  bt->best = tr->likelihood;
    if (newValid == bt->nkeep) bt->worst = bt->byScore[newValid]->likelihood;

    return  scrNum;
  } /* saveBestTree */




int  recallBestTreeRecursive (bestlist *bt, int rank, tree *tr)
  { /* recallBestTree */
    if (rank < 1)  rank = 1;
    if (rank > bt->nvalid)  rank = bt->nvalid;
    if (rank > 0)  if (! restoreTreeRecursive(bt->byScore[rank], tr)) return FALSE;
    return  rank;
  } /* recallBestTree */



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


void getnums (rawdata *rdta)
    /* input number of species, number of sites */
  { /* getnums */
    

    if (fscanf(INFILE, "%d %d", & rdta->numsp, & rdta->sites) != 2) 
      {
	printf("ERROR: Problem reading number of species and sites\n");
	exit(-1);
      }
   

    if (rdta->numsp < 4) 
      {
	printf("TOO FEW SPECIES\n");
	exit(-1);
      }

    if (rdta->sites < 1) 
      {
	printf("TOO FEW SITES\n");
	exit(-1);
      }

    return;
  } /* getnums */


boolean digitchar (int ch) 
{
  return (ch >= '0' && ch <= '9'); 
}


boolean whitechar (int ch)
{ 
  return (ch == ' ' || ch == '\n' || ch == '\t');
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

void getyspace (rawdata *rdta)
  { /* getyspace */
    long   size;
    int    i;
    yType *y0;

    if (! (rdta->y = (yType **) malloc((rdta->numsp + 1) * sizeof(yType *)))) 
      {
	printf("ERROR: Unable to obtain space for data array pointers\n");
	exit(-1);
      }

    size = 4 * (rdta->sites / 4 + 1);
    if (! (y0 = (yType *) malloc((rdta->numsp + 1) * size * sizeof(yType)))) 
      {
	printf("ERROR: Unable to obtain space for data array\n");
	exit(-1);
      }

    rdta->y0 = y0;

    for (i = 0; i <= rdta->numsp; i++) 
      {
	rdta->y[i] = y0;
	y0 += size;
      }

    return;
  } /* getyspace */


void freeyspace(rawdata *rdta)
{
  long   size;
  int    i;
  yType *y0;
  for (i = 0; i <= rdta->numsp; i++) 
    {
      free(rdta->y[i]);    
    }
}



boolean setupTree (tree *tr, int nsites, analdef *adef)
  { /* setupTree */
    nodeptr  p0, p, q;
    int      i, j, tips, inter;

    tr->outgr     =           1;

   

    

    tips  = tr->mxtips;
    inter = tr->mxtips - 1;

    if (!(p0 = (nodeptr) malloc((tips + 3*inter) * sizeof(node)))) {
      printf("ERROR: Unable to obtain sufficient tree memory\n");
      return  FALSE;
      }


    if (!(tr->nodep = (nodeptr *) malloc((2*tr->mxtips) * sizeof(nodeptr)))) {
      printf("ERROR: Unable to obtain sufficient tree memory, too\n");
      return  FALSE;
      }

    
    tr->nodep[0] = (node *) NULL;    /* Use as 1-based array */

    for (i = 1; i <= tips; i++) {    /* Set-up tips */
      p = p0++;
      p->x      = (xarray *) NULL;
      p->tip    = (yType *) NULL;
      
      /* AxML modification start*/
      p->equalityVector = (int *) NULL; /*initialize eq vector with zero */
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
	p->equalityVector = (int *) NULL; /*initialize eq vector with zero */	
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


    

    return TRUE;
  } /* setupTree */

void freeTreeNode (nodeptr p)   /* Free tree node (sector) associated data */
  { /* freeTreeNode */
    if (p) {
      if (p->x) {
        if (p->x->lv) free(p->x->lv);
        free(p->x);
        }
      /* AxML modification start*/
      free(p->equalityVector); /*free equality vector of node*/
      /* AxML modification end*/
      }
  } /* freeTree */



/* AxML modification start*/





boolean setupEqualityVectorGTR(tree *tr) /* new function for initializing and mapping equality vectors */
{
  int siteNum = tr->cdta->endsite;
  nodeptr upper = tr->nodep[tr->mxtips * 2 - 1];
  nodeptr p = tr->nodep[1];
  int *ev, *end;
  char *tip;   
  likelivector *l;
  int j;
  homType *mlvP;
  int refREV[EQUALITIES];
  int code;
  double (*EIGV)[4] = &(tr->rdta->EIGV[0][0]);
  
  for(; p < upper; p++)
    {      
      if( ! (p->equalityVector = 
	     (ev = (int *)malloc(siteNum * sizeof(int)))))
	{
	  printf("ERROR: Failure to get node equality vector  memory\n");
	  return  FALSE; 
	}      
      
      end = & ev[siteNum];
      
      if(p->tip)
	{	 
	  int i;
	  tip = p->tip;
	  mlvP = tr->nodep[p->number]->mlv;
	  
	  for(i = 0; i < equalities; i++)
	    refREV[i] = -1;

	  for(i = 0; i < siteNum; i++)
	    {	    
	      ev[i] = tip[i] - 1; 	     
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
  
  
  /*freeyspace(tr->rdta);*/

  return TRUE;
}




boolean setupEqualityVectorHKY85(tree *tr) /* new function for initializing and mapping equality vectors */
{
  int siteNum = tr->cdta->endsite;
  nodeptr upper = tr->nodep[tr->mxtips * 2 - 1];
  nodeptr p = tr->nodep[1];
  int *ev, *end;
  char *tip;   
  likelivector *l;
  int i, j;
  homType *mlvP;
  int ref[EQUALITIES];
  int code;
 
 
  


  
  for(; p < upper; p++)
    {      
      if( ! (p->equalityVector = 
	     (ev = (int *)malloc(siteNum * sizeof(int)))))
	{
	  printf("ERROR: Failure to get node equality vector  memory\n");
	  return  FALSE; 
	}      
      
      


      end = & ev[siteNum];
      
      if(p->tip)
	{	 
	  int i;
	  tip = p->tip;
	  mlvP = p->mlv;
	  
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
	      code = map2[i];
	      if(ref[i] == 1)
		{
		  mlvP->a =  code       & 1;
		  mlvP->c = (code >> 1) & 1; 	
		  mlvP->g = (code >> 2) & 1;
		  mlvP->t = (code >> 3) & 1;
		  mlvP->exp = 0;
		  mlvP->set = TRUE;				  		
		}
	      else
		mlvP->set = FALSE;

	      mlvP++;	 
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
  
  
  /*freeyspace(tr->rdta);*/

  return TRUE;
}








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
    

    free(tr->nodep[1]);       /* Free the actual nodes */
  } /* freeTree */


boolean getdata (analdef *adef, rawdata *rdta, tree *tr)
    /* read sequences */
  { /* getdata */
    int   i, j, k, l, basesread, basesnew, ch, my_i;
    int   meaning[256];          /*  meaning of input characters */ 
    char *nameptr;
    boolean  allread, firstpass;
    char buffer[300];

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
    meaning['O'] = 16;
    meaning['R'] =  5;
    meaning['S'] =  6;
    meaning['T'] =  8;
    meaning['U'] =  8;
    meaning['V'] =  7;
    meaning['W'] =  9;
    meaning['X'] = 17;
    meaning['Y'] = 10;
    meaning['?'] = 18;
    meaning['-'] = 19;

    basesread = basesnew = 0;

    allread = FALSE;
    firstpass = TRUE;
    ch = ' ';

    while (! allread) 
      {
	for (i = 1; i <= tr->mxtips; i++) 
	  {   

	    if (firstpass) 
	      {                      
		/*  Read species names */
		ch = getc(INFILE);
		while(ch == ' ' || ch == '\n' || ch == '\t')
		  {
		    ch = getc(INFILE);
		  }

		my_i = 0;

		do 
		  {
		    buffer[my_i] = ch;
		    ch = getc(INFILE);		   
		    my_i++;
		    if(my_i >= nmlngth)
		      {
			printf("ERROR Names are to long, adapt constant nmlngth in\n");
			printf("axml.h AND dnapars.c, current setting %d\n", nmlngth);
			exit(-1);
		      }
		  }
		while(ch !=  ' ' && ch != '\n' && ch != '\t');
		
		buffer[my_i] = '\0';

		strcpy(tr->nodep[i]->name, buffer);		
		/*j = 1;
		  while (whitechar(ch = getc(INFILE))) 
		  { 
		  printf("char %d\n", ch);
		  if (ch == '\n')  
		  j = 1;  
		  else  
		  j++;
		  }
		  
		  if (j > nmlngth) 
		  {
		  printf("ERROR: Blank name for species %d; ", i);
		  printf("check number of species,\n");
		  printf("       number of sites, and interleave option.\n");
		  return  FALSE;
		  }

		  nameptr = tr->nodep[i]->name;
		  for (k = 1; k < j; k++)  *nameptr++ = ' ';
		  
		  while (ch != '\n' && ch != EOF) 
		  {
		  if (whitechar(ch))  ch = ' ';
		  *nameptr++ = ch;
		  if (++j > nmlngth) break;
		  ch = getc(INFILE);
		  }

		  while (*(--nameptr) == ' ') ;  
		  *(++nameptr) = '\0';                   
		  
		  if (ch == EOF) 
		  {
		  printf("ERROR: End-of-file in name of species %d\n", i);
		  return  FALSE;
		  }*/
	      }



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
	      printf("%d (instead of %d) residues read in sequence %d %s\n",
		     j - basesread, basesnew - basesread, i, tr->nodep[i]->name);
	      return  FALSE;
	    }

	    while (ch != '\n' && ch != EOF) ch = getc(INFILE);  /* flush line */
	  }                                                  /* next sequence */
	firstpass = FALSE;
	basesread = basesnew;
	allread = (basesread >= rdta->sites);
      }

    /*  Print listing of sequence alignment */

    for (j = 1; j <= tr->mxtips; j++)    /* Convert characters to meanings */
      for (i = 1; i <= rdta->sites; i++) {
        rdta->y[j][i] = meaning[rdta->y[j][i]];
        }
    
    return  TRUE;
  } /* getdata */





void inputweights (analdef *adef, rawdata *rdta, cruncheddata *cdta)
    /* input the character weights 0, 1, 2 ... 9, A, B, ... Y, Z */
  { /* inputweights */
    int i, ch, w, fres;
    FILE *weightFile;
    int *wv = (int *)malloc(sizeof(int) *  rdta->sites + 1);
  

    weightFile = fopen(weightFileName, "r");
    if (!weightFile)
      {
	printf( "Could not open weight file: %s\n", weightFileName);
	exit(1);
      }
     
    i = 1;
    
    while((fres = fscanf(weightFile,"%d", &w)) != EOF)
      {
	if(!fres)
	  {
	    printf("error reading weight file probably encountered a non-integer weight value\n");
	    exit(-1);
	  }
	wv[i] = w;
	i++;	
      }
    
   
    if(i != (rdta->sites + 1))
      {
	printf("number %d of weights not equal to number %d of alignment columns\n", i, rdta->sites);
	exit(-1);
      }

    cdta->wgtsum = 0;
    for(i = 1; i <= rdta->sites; i++) 
      {     
	rdta->wgt[i] = wv[i];
	cdta->wgtsum += rdta->wgt[i];
      }

    fclose(weightFile);
    free(wv);
  } /* inputweights */







void getinput (analdef *adef, rawdata *rdta, cruncheddata *cdta, tree *tr)
  { /* getinput */
    int i,j;
    char **names;

    getnums(rdta);
    tr->outgr     = 1;
    tr->mxtips    = rdta->numsp;
    rdta->wgt      = (int *)    malloc((rdta->sites + 1) * sizeof(int));
    rdta->wgt2     = (int *)    malloc((rdta->sites + 1) * sizeof(int));
    cdta->alias    = (int *)    malloc((rdta->sites + 1) * sizeof(int));
    cdta->aliaswgt = (int *)    malloc((rdta->sites + 1) * sizeof(int));
    cdta->patrat   = (double *) malloc((rdta->sites + 1) * sizeof(double));
    cdta->patratStored   = (double *) malloc((rdta->sites + 1) * sizeof(double));
    cdta->rateCategory = (int *) malloc((rdta->sites + 1) * sizeof(int));
    cdta->wr       = (double *) malloc((rdta->sites + 1) * sizeof(double));
    cdta->wr2      = (double *) malloc((rdta->sites + 1) * sizeof(double));
    
    if(!adef->useWeightFile)
      {
	for (i = 1; i <= rdta->sites; i++) 
	  rdta->wgt[i] = 1;
	cdta->wgtsum = rdta->sites;   
      }
    else
      {	
	inputweights(adef, rdta, cdta);
      }
    


    getyspace(rdta);
    setupTree(tr, rdta->sites, adef);
    getdata(adef, rdta, tr);
    names = (char **)malloc((tr->mxtips + 1) * sizeof(char *));
         
    for(i = 1; i <= tr->mxtips; i++)
      names[i] = tr->nodep[i]->name;
    if(!adef->restart)
      RAxML_parsimony_init(tr->mxtips, rdta->sites, rdta->y, names);

    for(i = 1; i <= tr->mxtips; i++)
      {	
	for(j = 1; j <= rdta->sites; j++)
	  {	    	   	   
	    if(rdta->y[i][j] > 15) rdta->y[i][j] = 15; 
	  }
      }
    free(names);
    return;
  } /* getinput */




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


void makeboot (analdef *adef, rawdata *rdta, cruncheddata *cdta)
  {
    int  i, j, nonzero;
    double  randum();

    nonzero = 0;
    for (i = 1; i <= rdta->sites; i++)  
      if (rdta->wgt[i] > 0) nonzero++;

    for (j = 1; j <= nonzero; j++) 
      cdta->aliaswgt[j] = 0;
    for (j = 1; j <= nonzero; j++)
      cdta->aliaswgt[(int) (nonzero*randum(& adef->boot)) + 1]++;

    j = 0;
    cdta->wgtsum = 0;
    for (i = 1; i <= rdta->sites; i++) 
      {
	if (rdta->wgt[i] > 0)
	  cdta->wgtsum += (rdta->wgt2[i] = rdta->wgt[i] * cdta->aliaswgt[++j]);
	else
	  rdta->wgt2[i] = 0;
      }
  } 


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
    data     = rdta->y;
    n        = rdta->sites;
    nsp      = rdta->numsp;

    for (gap = n / 2; gap > 0; gap /= 2) {
      for (i = gap + 1; i <= n; i++) {
        j = i - gap;

        do {
          jj = index[j];
          jg = index[j+gap];
          flip = 0;
          tied = 1;
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
    int gaps = 0;
    boolean  tied;

  

    i = 0;
    cdta->alias[0] = cdta->alias[1];
    cdta->aliaswgt[0] = 0;

    for (j = 1; j <= rdta->sites; j++) {
      sitei = cdta->alias[i];
      sitej = cdta->alias[j];
      tied = 1;

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

    cdta->endsite = cdta->endsiteNormal = i;
    if (cdta->aliaswgt[i] > 0) cdta->endsite++;
  } /* sitecombcrunch */


boolean makeweights (analdef *adef, rawdata *rdta, cruncheddata *cdta)
    /* make up weights vector to avoid duplicate computations */
  { /* makeweights */
    int  i;

    if (adef->boot)  
      makeboot(adef, rdta, cdta);
    else 
      {
	for (i = 1; i <= rdta->sites; i++)  
	  rdta->wgt2[i] = rdta->wgt[i];
      }

    for (i = 1; i <= rdta->sites; i++)  
      cdta->alias[i] = i;
    sitesort(rdta, cdta);
    sitecombcrunch(rdta, cdta);
    return TRUE;
  } /* makeweights */






boolean makevalues (rawdata *rdta, cruncheddata *cdta)
    /* set up fractional likelihoods at tips */
  { /* makevalues */
    double  temp, wtemp;
    int  i, j;

    for (i = 1; i <= rdta->numsp; i++) {
      for (j = 0; j < cdta->endsite; j++) 
	{
	  rdta->y[i-1][j] = rdta->y[i][cdta->alias[j]];
        }
      }

   
    for (j = 0; j < cdta->endsite; j++) 
      {
	cdta->patrat[j] = temp = 1.0;
	cdta->patratStored[j] = 1.0;
	cdta->rateCategory[j] = 0;
	cdta->wr[j]  = wtemp = temp * cdta->aliaswgt[j];
	cdta->wr2[j] = temp * wtemp;
      }

    return TRUE;
  } /* makevalues */

char *itobase36_str(i)
     int i;
  { 
    static char buf[4];
    int j = 2;
    strcpy(buf,"  0");
    while(i && j >= 0) {
      buf[j--] = itobase36(i % 36);
      i /= 36;
    }
    return buf;
  }

boolean empiricalfreqs (analdef *adef, rawdata *rdta, cruncheddata *cdta)
    /* Get empirical base frequencies from the data */
  { /* empiricalfreqs */
    double  sum, suma, sumc, sumg, sumt, wj, fa, fc, fg, ft, sumb;
    int     i, j, k, code;
    yType  *yptr;
    
    for (j = 0; j < cdta->endsite; j++) 
      {
	for (k = 1; k <= 8; k++) 
	  {
	     rdta->freqa = 0.25;
	     rdta->freqc = 0.25;
	     rdta->freqg = 0.25;
	     rdta->freqt = 0.25;
	    suma = 0.0;
	    sumc = 0.0;
	    sumg = 0.0;
	    sumt = 0.0;
	    for (i = 0; i < rdta->numsp; i++) 
	      {
		yptr = rdta->y[i];
		
		code = yptr[j];
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

   rdta->freqr = rdta->freqa + rdta->freqg;
    rdta->invfreqr = 1.0/rdta->freqr;
    rdta->freqar = rdta->freqa * rdta->invfreqr;
    rdta->freqgr = rdta->freqg * rdta->invfreqr;
    rdta->freqy = rdta->freqc + rdta->freqt;
    rdta->invfreqy = 1.0/rdta->freqy;
    rdta->freqcy = rdta->freqc * rdta->invfreqy;
    rdta->freqty = rdta->freqt * rdta->invfreqy;
    
    suma = rdta->ttratio*rdta->freqr*rdta->freqy
         - (rdta->freqa*rdta->freqg + rdta->freqc*rdta->freqt);
    sumb = rdta->freqa*rdta->freqgr + rdta->freqc*rdta->freqty;
    rdta->xi = suma/(suma+sumb);
    rdta->xv = 1.0 - rdta->xi;
    if (rdta->xi <= 0.0) {
      /*printf("WARNING: This transition/transversion ratio\n");
      printf("         is impossible with these base frequencies!\n");
      printf("Transition/transversion parameter reset\n\n");*/
      rdta->xi = 0.000001;
      rdta->xv = 1.0 - rdta->xi;
      rdta->ttratio = (sumb * rdta->xi / rdta->xv 
                       + rdta->freqa * rdta->freqg
                       + rdta->freqc * rdta->freqt)
                    / (rdta->freqr * rdta->freqy);
     
      }
   
    rdta->fracchange = 2.0 * rdta->xi * (rdta->freqa * rdta->freqgr
                                       + rdta->freqc * rdta->freqty)
                     + rdta->xv * (1.0 - rdta->freqa * rdta->freqa
                                       - rdta->freqc * rdta->freqc
                                       - rdta->freqg * rdta->freqg
                                       - rdta->freqt * rdta->freqt);
    
    return TRUE;
  } /* empiricalfreqs */






void alterTT(tree *tr, double t_value)
  { /* reportfreqs */
    double  suma, sumb;       
    
    rawdata *rdta = tr->rdta;
    tr->t_value = t_value;
    suma = t_value*rdta->freqr*rdta->freqy
         - (rdta->freqa*rdta->freqg + rdta->freqc*rdta->freqt);
    sumb = rdta->freqa*rdta->freqgr + rdta->freqc*rdta->freqty;
    rdta->xi = suma/(suma+sumb);
    rdta->xv = 1.0 - rdta->xi;
    if (rdta->xi <= 0.0) {
      /*      printf("WARNING: This transition/transversion ratio\n");
      printf("         is impossible with these base frequencies!\n");
      printf("Transition/transversion parameter reset\n\n");*/
      rdta->xi = 0.000001;
      rdta->xv = 1.0 - rdta->xi;
      rdta->ttratio = (sumb * rdta->xi / rdta->xv 
                       + rdta->freqa * rdta->freqg
                       + rdta->freqc * rdta->freqt)
                    / (rdta->freqr * rdta->freqy);
     
      }
   
    rdta->fracchange = 2.0 * rdta->xi * (rdta->freqa * rdta->freqgr
                                       + rdta->freqc * rdta->freqty)
                     + rdta->xv * (1.0 - rdta->freqa * rdta->freqa
                                       - rdta->freqc * rdta->freqc
                                       - rdta->freqg * rdta->freqg
                                       - rdta->freqt * rdta->freqt);
  } 




boolean linkdata2tree (rawdata *rdta, cruncheddata *cdta, tree *tr)
    /* Link data array to the tree tips */
  { /* linkdata2tree */
    int  i;
    int  j;
    yType *tip;
    int ref[EQUALITIES];
    tr->rdta       = rdta;
    tr->cdta       = cdta;
    tr->NumberOfCategories = 1;

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
     
     map2 = (int *)malloc(equalities * sizeof(int));

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

    x = (xarray *) malloc(sizeof(xarray));
    if (x) {
      data = (likelivector *) malloc(npat * sizeof(likelivector));
      if (data) {
        x->lv = data;
        x->prev = x->next = x;
        x->owner = (node *) NULL;
        }
      else {
        free(x);
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


#ifdef OPEN_MP


boolean newviewGTRCAT(tree    *tr, nodeptr  p)
{
  
  if (p->tip) 
    {      
      if (p->x) return TRUE;
      if (! getxtip(p)) return FALSE;  
      {
	int code, i, j, jj;
	double (*EIGV)[4] = &(tr->rdta->EIGV[0][0]);
	likelivector *x1 = p->x->lv;
#pragma omp parallel for private(code, j)     
	for(i = 0; i < tr->cdta->endsite; i++)
	  {	      	  	 
	    code = p->tip[i];
	    x1[i].a = 0;
	    x1[i].c = 0;
	    x1[i].g = 0;
	    x1[i].t = 0;
	    for (j = 0; j < 4; j++) 
	      {
		if ((code >> j) & 1) 
		  {
		    int jj = "0213"[j] - '0';		
		    x1[i].a += EIGV[jj][0];
		    x1[i].c += EIGV[jj][1];
		    x1[i].g += EIGV[jj][2];
		    x1[i].t += EIGV[jj][3];
		  }
	      }		
	      
	    x1[i].exp = 0;	  			  			 	 	 
	} 
      return TRUE; 
      }
    }
 
  { 	
    double  *diagptable_x1, *diagptable_x2, *diagptable_x1_start, *diagptable_x2_start;
    double *EIGN    = &(tr->rdta->EIGN[0][0]);
    double *invfreq = &(tr->rdta->invfreq[0][0]);
    double (*EIGV)[4] = &(tr->rdta->EIGV[0][0]);
    xtype tmp_x1_1, tmp_x1_2, tmp_x1_3,
      tmp_x2_1, tmp_x2_2, tmp_x2_3, 
      ump_x1_1, ump_x1_2, ump_x1_3, ump_x1_0, 
      ump_x2_0, ump_x2_1, ump_x2_2, ump_x2_3, x1px2;
    int cat;
    int  *cptr;	  
    int  i, j, k;
    likelivector   *x1, *x2, *x3;
    double   z1, lz1, z2, lz2, ki;
    nodeptr  q, r;   
    double *rptr;

    rptr   = &(tr->cdta->patrat[0]); 
    cptr  = &(tr->cdta->rateCategory[0]);

    q = p->next->back;
    r = p->next->next->back;
      
    while ((! p->x) || (! q->x) || (! r->x)) {
      if (! q->x) if (! newviewGTRCAT(tr, q)) return FALSE;
      if (! r->x) if (! newviewGTRCAT(tr, r)) return FALSE;
      if (! p->x) if (! getxnode(p)) return FALSE;	
    }

    x1  = q->x->lv;
    z1  = q->z;
    lz1 = (z1 > zmin) ? log(z1) : log(zmin);

    x2  = r->x->lv;
    z2  = r->z;
    lz2 = (z2 > zmin) ? log(z2) : log(zmin);

    x3  = p->x->lv;   
  

    diagptable_x1 = diagptable_x1_start = (double *)malloc(sizeof(double) * tr->NumberOfCategories * 3);
    diagptable_x2 = diagptable_x2_start = (double *)malloc(sizeof(double) * tr->NumberOfCategories * 3);
#pragma omp parallel for private(ki, j, k)
    for(i = 0; i < tr->NumberOfCategories;i++)
      {
	ki = rptr[i];
	
	j = k = 3 * i;
	
	diagptable_x1[j] = exp(EIGN[1] * lz1 * ki);
	diagptable_x1[++j] = exp(EIGN[2] * lz1 * ki);
	diagptable_x1[++j] = exp(EIGN[3] * lz1 * ki);
	
	diagptable_x2[k] = exp(EIGN[1] * lz2 * ki);	
	diagptable_x2[++k] = exp(EIGN[2] * lz2 * ki);
	diagptable_x2[++k] = exp(EIGN[3] * lz2 * ki);		
      }

    
#pragma omp parallel for private(cat, tmp_x1_1, tmp_x1_2,tmp_x1_3,ump_x1_0,ump_x1_1,ump_x1_2,ump_x1_3,ump_x2_0,ump_x2_1,ump_x2_2,ump_x2_3,tmp_x2_1,tmp_x2_2,tmp_x2_3,x1px2)    
    for (i = 0; i < tr->cdta->endsite; i++) 
      {	
	cat = cptr[i];

	diagptable_x1 = &diagptable_x1_start[cat * 3];
	diagptable_x2 = &diagptable_x2_start[cat * 3];

	tmp_x1_1 = x1[i].c * diagptable_x1[0];
	tmp_x1_2 = x1[i].g * diagptable_x1[1];
	tmp_x1_3 = x1[i].t * diagptable_x1[2];
	
	ump_x1_0 = tmp_x1_1 * EIGV[0][1];
	ump_x1_0 += tmp_x1_2 * EIGV[0][2];
	ump_x1_0 += tmp_x1_3 * EIGV[0][3];	      	
	ump_x1_0 *= invfreq[0];
	ump_x1_0 += x1[i].a;
	
	ump_x1_1 = tmp_x1_1 * EIGV[1][1];
	ump_x1_1 += tmp_x1_2 * EIGV[1][2];
	ump_x1_1 += tmp_x1_3 * EIGV[1][3];	      	
	ump_x1_1 *= invfreq[1];
	ump_x1_1 += x1[i].a;
	    
	ump_x1_2 = tmp_x1_1 * EIGV[2][1];
	ump_x1_2 += tmp_x1_2 * EIGV[2][2];
	ump_x1_2 += tmp_x1_3 * EIGV[2][3];	      	
	ump_x1_2 *= invfreq[2];
	ump_x1_2 += x1[i].a;

	ump_x1_3 = tmp_x1_1 * EIGV[3][1];
	ump_x1_3 += tmp_x1_2 * EIGV[3][2];
	ump_x1_3 += tmp_x1_3 * EIGV[3][3];	      	
	ump_x1_3 *= invfreq[3];
	ump_x1_3 += x1[i].a;
	
	tmp_x2_1 = x2[i].c * diagptable_x2[0];
	tmp_x2_2 = x2[i].g * diagptable_x2[1];
	tmp_x2_3 = x2[i].t * diagptable_x2[2];
	 
	 	    	    	     	     
	ump_x2_0 = tmp_x2_1 * EIGV[0][1];
	ump_x2_0 += tmp_x2_2 * EIGV[0][2];
	ump_x2_0 += tmp_x2_3 * EIGV[0][3];		     
	ump_x2_0 *= invfreq[0];
	ump_x2_0 += x2[i].a;
	  
	ump_x2_1 = tmp_x2_1 * EIGV[1][1];
	ump_x2_1 += tmp_x2_2 * EIGV[1][2];
	ump_x2_1 += tmp_x2_3 * EIGV[1][3];		     
	ump_x2_1 *= invfreq[1];
	ump_x2_1 += x2[i].a;	 

	ump_x2_2 = tmp_x2_1 * EIGV[2][1];
	ump_x2_2 += tmp_x2_2 * EIGV[2][2];
	ump_x2_2 += tmp_x2_3 * EIGV[2][3];		     
	ump_x2_2 *= invfreq[2];
	ump_x2_2 += x2[i].a;	  
		   

	ump_x2_3 = tmp_x2_1 * EIGV[3][1] ;
	ump_x2_3 += tmp_x2_2 * EIGV[3][2];
	ump_x2_3 += tmp_x2_3 * EIGV[3][3];		     
	ump_x2_3 *= invfreq[3];
	ump_x2_3 += x2[i].a;	    	  		   	  	
	
	x1px2 = ump_x1_0 * ump_x2_0;
	x3[i].a = x1px2 *  EIGV[0][0];
	x3[i].c = x1px2 *  EIGV[0][1];
	x3[i].g = x1px2 *  EIGV[0][2];
	x3[i].t = x1px2 *  EIGV[0][3]; 
	
	x1px2 = ump_x1_1 * ump_x2_1;
	x3[i].a += x1px2  *  EIGV[1][0];
	x3[i].c += x1px2 *   EIGV[1][1];
	x3[i].g += x1px2 *   EIGV[1][2];
	x3[i].t += x1px2 *   EIGV[1][3];
	
	x1px2 = ump_x1_2 * ump_x2_2;
	x3[i].a += x1px2 *  EIGV[2][0];
	x3[i].c += x1px2 *  EIGV[2][1];
	x3[i].g += x1px2 *  EIGV[2][2];
	x3[i].t += x1px2 *  EIGV[2][3];
	
	x1px2 = ump_x1_3 * ump_x2_3;
	x3[i].a += x1px2 *   EIGV[3][0];
	x3[i].c += x1px2 *   EIGV[3][1];
	x3[i].g += x1px2 *   EIGV[3][2];
	x3[i].t += x1px2 *   EIGV[3][3];
		   
	x3[i].exp = x1[i].exp + x2[i].exp;		  
	
	if (ABS(x3[i].a) < minlikelihood && ABS(x3[i].g) < minlikelihood
	    && ABS(x3[i].c) < minlikelihood && ABS(x3[i].t) < minlikelihood) 
	  {	     
	    x3[i].a   *= twotothe256;
	    x3[i].g   *= twotothe256;
	    x3[i].c   *= twotothe256;
	    x3[i].t   *= twotothe256;
	    x3[i].exp += 1;
	    }
      }     

   
    free(diagptable_x1_start); 
    free(diagptable_x2_start);

    return TRUE;
  }
}





double evaluateGTRCAT (tr, p)
    tree    *tr;
    nodeptr  p;
  { /* evaluate */
    double   sum, z, lz, term, ki;    
    nodeptr  q;
    int     i, j, cat;
    int     *wptr = tr->cdta->aliaswgt, *cptr;
    
    double  *diagptable, *rptr, *diagptable_start;
    double *EIGN = &(tr->rdta->EIGN[0][0]);
    likelivector   *x1, *x2;
       
    q = p->back;
    while ((! p->x) || (! q->x)) {
      if (! (p->x)) if (! newviewGTRCAT(tr, p)) return badEval;
      if (! (q->x)) if (! newviewGTRCAT(tr, q)) return badEval;
    }


    rptr   = &(tr->cdta->patrat[0]); 
    cptr  = &(tr->cdta->rateCategory[0]);

    x1  = p->x->lv;
    x2  = q->x->lv;
    z = p->z;
   
    if (z < zmin) z = zmin;
    lz = log(z);         

    diagptable = diagptable_start = (double *)malloc(sizeof(double) * tr->NumberOfCategories * 3);
#pragma omp parallel for private(ki, j)
    for(i = 0; i <  tr->NumberOfCategories; i++)
      {
	ki = rptr[i];	 
 
	j = 3 * i;

	diagptable[j] = exp(EIGN[1] * lz * ki);
	diagptable[++j] = exp(EIGN[2] * lz * ki);
	diagptable[++j] = exp(EIGN[3] * lz * ki);
      }
    
    sum = 0.0;
#pragma omp parallel for private(cat, term) reduction(+ : sum)    
    for (i = 0; i < tr->cdta->endsite; i++) 
      {	  	 	  
	cat = cptr[i];
	
	diagptable = &diagptable_start[3 * cat];

	term =  x1[i].a * x2[i].a;
	term += x1[i].c * x2[i].c * diagptable[0];
	term += x1[i].g * x2[i].g * diagptable[1];
	term += x1[i].t * x2[i].t * diagptable[2];     
	term = log(term) + (x1[i].exp + x2[i].exp)*log(minlikelihood);
	sum += wptr[i] * term;	
      }
    
   	
      
    free(diagptable_start); 
    
    tr->likelihood = sum;
    return  sum;
  } /* evaluate */



double makenewzGTRCAT (tr, p, q, z0, maxiter)
    tree    *tr;
    nodeptr  p, q;
    double   z0;
    int  maxiter;
  { /* makenewz */
    double   z, zprev, zstep;
    likelivector  *x1, *x2;
    double  *sum0, *sum1, *sum2, *sum3;
    int     i, *cptr, cat;
    double  dlnLdlz = 0.0;                 /*  = d(ln(likelihood))/d(lz) */
    double  d2lnLdlz2 = 0.0;               /*  = d2(ln(likelihood))/d(lz)2 */    
    double d0, *d1, *d2, *d3, ki, *rptr;
    double e0, e1, e2, e3;
    double *EIGN    = &(tr->rdta->EIGN[0][0]);   
    double *wrptr   = &(tr->cdta->wr[0]);
    double *wr2ptr  = &(tr->cdta->wr2[0]);

    while ((! p->x) || (! q->x)) {
      if (! (p->x)) if (! newviewGTRCAT(tr, p)) return badZ;
      if (! (q->x)) if (! newviewGTRCAT(tr, q)) return badZ;
    }

    x1 = p->x->lv;
    x2 = q->x->lv;

   sum0  = (double *)malloc(tr->cdta->endsite * sizeof(double));
   sum1  = (double *)malloc(tr->cdta->endsite * sizeof(double));  
   sum2  = (double *)malloc(tr->cdta->endsite * sizeof(double)); 
   sum3  = (double *)malloc(tr->cdta->endsite * sizeof(double));
    d1 = (double *)malloc(tr->NumberOfCategories * sizeof(double));
    d2 = (double *)malloc(tr->NumberOfCategories * sizeof(double));
    d3 = (double *)malloc(tr->NumberOfCategories * sizeof(double));

#pragma omp parallel for
   for (i = 0; i < tr->cdta->endsite; i++) 
     {     
       sum0[i] = x1[i].a * x2[i].a;
       sum1[i] = x1[i].c * x2[i].c;
       sum2[i] = x1[i].g * x2[i].g;
       sum3[i] = x1[i].t * x2[i].t;		     
     }

   
   rptr   = &(tr->cdta->patrat[0]); 
   cptr  = &(tr->cdta->rateCategory[0]);

    z = z0;
    do {
      int curvatOK = FALSE;

      zprev = z;
      
      zstep = (1.0 - zmax) * z + zmin;

      do {	
	double tmp_0, tmp_1, tmp_2, tmp_3;
	double t, inv_Li, dlnLidlz, d2lnLidlz2; 
	double lz;
        dlnLdlz = 0.0;                 /*  = d(ln(likelihood))/d(lz) */
        d2lnLdlz2 = 0.0;               /*  = d2(ln(likelihood))/d(lz)2 */

        if (z < zmin) z = zmin;
        else if (z > zmax) z = zmax;
        lz    = log(z);
	

#pragma omp parallel for private(ki)	
	for(i = 0; i < tr->NumberOfCategories; i++)
	  {
	    ki = rptr[i];
	    d1[i] = exp (EIGN[1] * lz * ki); 
	    d2[i] = exp (EIGN[2] * lz * ki); 
	    d3[i] = exp (EIGN[3] * lz * ki); 
	  }




#pragma omp parallel for private(cat, inv_Li, t, tmp_0, tmp_1, tmp_2, tmp_3, dlnLidlz, d2lnLidlz2) reduction(+ :  dlnLdlz) reduction(+ : d2lnLdlz2)
        for (i = 0; i < tr->cdta->endsite; i++) 
	  {
	    cat = cptr[i];
	    	 
	    inv_Li = (tmp_0 = sum0[i]);
	    inv_Li += (tmp_1 = sum1[i] * d1[cat]);
	    inv_Li += (tmp_2 = sum2[i] * d2[cat]);
	    inv_Li += (tmp_3 = sum3[i] * d3[cat]);

	    inv_Li = 1.0/inv_Li;
	    
	  
	    t = tmp_1 * EIGN[1];
	    dlnLidlz   = t;
	    d2lnLidlz2 = t * EIGN[1];
	    
	    t = tmp_2 * EIGN[2];
	    dlnLidlz   += t;
	    d2lnLidlz2 += t * EIGN[2];
	    
	    t = tmp_3 * EIGN[3];
	    dlnLidlz   += t;
	    d2lnLidlz2 += t * EIGN[3];
	    
	    dlnLidlz   *= inv_Li;
	    d2lnLidlz2 *= inv_Li;
	    dlnLdlz   += wrptr[i]  * dlnLidlz;
	    d2lnLdlz2 += wr2ptr[i] * (d2lnLidlz2 - dlnLidlz * dlnLidlz);
	  }
	
        if ((d2lnLdlz2 >= 0.0) && (z < zmax))
          zprev = z = 0.37 * z + 0.63;  /*  Bad curvature, shorten branch */
        else
          curvatOK = TRUE;
	
      } while (! curvatOK);

      if (d2lnLdlz2 < 0.0) {
	double tantmp = -dlnLdlz / d2lnLdlz2;  /* prevent overflow */
	if (tantmp < 100) {
	  z *= exp(tantmp);
	  if (z < zmin) z = zmin;
	  if (z > 0.25 * zprev + 0.75)    /*  Limit steps toward z = 1.0 */
	    z = 0.25 * zprev + 0.75;
	} else {
	  z = 0.25 * zprev + 0.75;
	}
      }
      if (z > zmax) z = zmax;

    } while ((--maxiter > 0) && (ABS(z - zprev) > zstep));

    free (sum0);
    free (sum1);
    free (sum2);
    free (sum3);
    free(d1);
    free(d2);
    free(d3);
    return  z;
  } /* makenewz */


boolean newviewHKY85 (tree *tr, nodeptr p)      
  {
    double   zq, lzq, xvlzq, zr, lzr, xvlzr;
    nodeptr  q, r;
    likelivector *lp, *lq, *lr;
    int  i;
   
    if (p->tip) 
      {         
	likelivector *l;
	int           code;
	yType        *yptr;

	if (p->x) return TRUE;

	if (! getxtip(p)) return FALSE;
	l = p->x->lv;           
	yptr = p->tip;   

#pragma omp parallel for private(code)
	for (i = 0; i < tr->cdta->endsite; i++) 
	  {
	   
	    code = yptr[i];
	    l[i].a =  code       & 1;
	    l[i].c = (code >> 1) & 1;
	    l[i].g = (code >> 2) & 1;
	    l[i].t = (code >> 3) & 1;
	    l[i].exp = 0;	
	  }
	return TRUE;
      }

    q = p->next->back;
    r = p->next->next->back;

    while ((! p->x) || (! q->x) || (! r->x)) {
      if (! q->x) if (! newviewHKY85(tr, q))  return FALSE;
      if (! r->x) if (! newviewHKY85(tr, r))  return FALSE;
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

    { 
      double  fxqr, fxqy, fxqn, sumaq, sumgq, sumcq, sumtq,
	fxrr, fxry, fxrn, tempi, tempj,
	zzq, zvq, zzr, zvr;
                           
      zzq = exp(lzq);
      zvq = exp(xvlzq);
      zzr = exp(lzr);
      zvr = exp(xvlzr);
#pragma omp parallel for private(fxqr, fxqy, fxqn, tempi, tempj, sumaq, sumgq, sumcq, sumtq, fxrr, fxry, fxrn)
      for (i = 0; i < tr->cdta->endsite; i++) 
	{         	  
          fxqr = tr->rdta->freqa * lq[i].a + tr->rdta->freqg * lq[i].g;
          fxqy = tr->rdta->freqc * lq[i].c + tr->rdta->freqt * lq[i].t;
          fxqn = fxqr + fxqy;
          tempi = fxqr * tr->rdta->invfreqr;
          tempj = zvq * (tempi-fxqn) + fxqn;
          sumaq = zzq * (lq[i].a - tempi) + tempj;
          sumgq = zzq * (lq[i].g - tempi) + tempj;
          tempi = fxqy * tr->rdta->invfreqy;
          tempj = zvq * (tempi-fxqn) + fxqn;
          sumcq = zzq * (lq[i].c - tempi) + tempj;
          sumtq = zzq * (lq[i].t - tempi) + tempj;

         
          fxrr = tr->rdta->freqa * lr[i].a + tr->rdta->freqg * lr[i].g;
          fxry = tr->rdta->freqc * lr[i].c + tr->rdta->freqt * lr[i].t;
          fxrn = fxrr + fxry;
          tempi = fxrr * tr->rdta->invfreqr;
          tempj = zvr * (tempi-fxrn) + fxrn;
          lp[i].a = sumaq * (zzr * (lr[i].a - tempi) + tempj);
          lp[i].g = sumgq * (zzr * (lr[i].g - tempi) + tempj);
          tempi = fxry * tr->rdta->invfreqy;
          tempj = zvr * (tempi-fxrn) + fxrn;
          lp[i].c = sumcq * (zzr * (lr[i].c - tempi) + tempj);
          lp[i].t = sumtq * (zzr * (lr[i].t - tempi) + tempj);         	 	  
	  lp[i].exp = lq[i].exp + lr[i].exp;

	  if (lp[i].a < minlikelihood && lp[i].g < minlikelihood
	      && lp[i].c < minlikelihood && lp[i].t < minlikelihood) 
	    {	      	     
	      lp[i].a   *= twotothe256;
	      lp[i].g   *= twotothe256;
	      lp[i].c   *= twotothe256;
	      lp[i].t   *= twotothe256;
	      lp[i].exp += 1;
	      
	    }
	}

      return TRUE;
    }
  }




double evaluateHKY85 (tree *tr, nodeptr p)
  { /* evaluate */
    double   sum, z, lz, xvlz,
             fxpa, fxpc, fxpg, fxpt, fxpr, fxpy, fxqr, fxqy,
             suma, sumb, sumc, term, zz, zv;   
    likelivector *lp, *lq;
    nodeptr       q;
    int           i, *wptr;

    q = p->back;

    while ((! p->x) || (! q->x)) 
      {
	if (! (p->x)) if (! newviewHKY85(tr, p)) return badEval;
	if (! (q->x)) if (! newviewHKY85(tr, q)) return badEval;
      }

    lp = p->x->lv;
    lq = q->x->lv;

    z = p->z;
    if (z < zmin) z = zmin;
    lz = log(z);
    xvlz = tr->rdta->xv * lz;
   
    zz = exp(lz);
    zv = exp(xvlz);
      
    wptr = &(tr->cdta->aliaswgt[0]);
       
    sum = 0.0;
#pragma omp parallel for private (fxpa, fxpg, fxpc, fxpt, fxqr, fxqy, fxpr, fxpy, sumc, sumb, suma, term)  reduction(+ : sum)
    for (i = 0; i < tr->cdta->endsite; i++) 
      {       
        fxpa = tr->rdta->freqa * lp[i].a;
        fxpg = tr->rdta->freqg * lp[i].g;
        fxpc = tr->rdta->freqc * lp[i].c;
        fxpt = tr->rdta->freqt * lp[i].t;
	fxqr = tr->rdta->freqa * lq[i].a + tr->rdta->freqg * lq[i].g;
        fxqy = tr->rdta->freqc * lq[i].c + tr->rdta->freqt * lq[i].t;
	fxpr = fxpa + fxpg;
        fxpy = fxpc + fxpt;
        suma = fxpa * lq[i].a + fxpc * lq[i].c + 
	  fxpg * lq[i].g + fxpt * lq[i].t;               
        sumc = (fxpr + fxpy) * (fxqr + fxqy);
        sumb = fxpr * fxqr * tr->rdta->invfreqr + fxpy * fxqy * tr->rdta->invfreqy;

        suma -= sumb;
        sumb -= sumc;

	term = log(zz * suma + zv * sumb + sumc) + (lp[i].exp + lq[i].exp)*log(minlikelihood);

        sum += wptr[i] * term;        

      }
    
  
    tr->likelihood = sum;
    
    return  sum;
  }



double makenewzHKY85 (tree *tr, nodeptr p, nodeptr q, double z0, int maxiter)
  { /* makenewz */
    likelivector *lp, *lq;
    double  *abi, *bci, *sumci, dlnLidlz, dlnLdlz, d2lnLdlz2, 
      z, zprev, zstep, lz, xvlz,
      ki, suma, sumb, sumc, ab, bc, inv_Li, t1, t2,
      fx1a, fx1c, fx1g, fx1t, fx1r, fx1y, fx2r, fx2y, 
      zz, zv, *wrptr, *wr2ptr;
    int      i, curvatOK, scratch_size;


    while ((! p->x) || (! q->x)) 
      {
	if (! (p->x)) if (! newviewHKY85(tr, p)) return badZ;
	if (! (q->x)) if (! newviewHKY85(tr, q)) return badZ;
      }

    lp = p->x->lv;
    lq = q->x->lv;
    scratch_size = sizeof(double) * tr->cdta->endsite;
       
    abi   = (double *) malloc(scratch_size);
    bci   = (double *) malloc(scratch_size);
    sumci = (double *) malloc(scratch_size);      
    
#pragma omp parallel for private (fx1a, fx1g, fx1c, fx1t, suma, fx2r, fx2y, fx1r, fx1y, sumc, sumb)
    for (i = 0; i < tr->cdta->endsite; i++) 
      {
	fx1a = tr->rdta->freqa * lp[i].a;
	fx1g = tr->rdta->freqg * lp[i].g;
	fx1c = tr->rdta->freqc * lp[i].c;
	fx1t = tr->rdta->freqt * lp[i].t;
	suma = fx1a * lq[i].a + fx1c * lq[i].c + fx1g * lq[i].g + fx1t * lq[i].t;
	fx2r = tr->rdta->freqa * lq[i].a + tr->rdta->freqg * lq[i].g;
	fx2y = tr->rdta->freqc * lq[i].c + tr->rdta->freqt * lq[i].t;
	fx1r = fx1a + fx1g;
	fx1y = fx1c + fx1t;
	sumci[i] = sumc = (fx1r + fx1y) * (fx2r + fx2y);
	sumb       = fx1r * fx2r * tr->rdta->invfreqr + fx1y * fx2y * tr->rdta->invfreqy;
	abi[i]   = suma - sumb;
	bci[i]   = sumb - sumc;
      }

    wrptr   = &(tr->cdta->wr[0]);
    wr2ptr  = &(tr->cdta->wr2[0]);
    z = z0;

    do 
      {
	zprev = z;
	zstep = (1.0 - zmax) * z + zmin;
	curvatOK = FALSE;

	do 
	  {
	    if (z < zmin) z = zmin;
	    else if (z > zmax) z = zmax;

	    lz    = log(z);
	    xvlz  = tr->rdta->xv * lz;         
        
	    zz = exp(lz);
	    zv = exp(xvlz);
               
	    dlnLdlz = 0.0;                 
	    d2lnLdlz2 = 0.0;              
#pragma omp parallel for private(ab, bc, sumc, inv_Li, t1, t2, 	dlnLidlz) reduction(+ : dlnLdlz) reduction( + : d2lnLdlz2) 
	    for (i = 0; i < tr->cdta->endsite; i++) 
	      {         
		ab     = abi[i] * zz;
		bc     = bci[i] * zv;
		sumc   = sumci[i];
		inv_Li = 1.0/(ab + bc + sumc);
		t1     = ab * inv_Li;
		t2     = tr->rdta->xv * bc * inv_Li;
		dlnLidlz   = t1 + t2;
		dlnLdlz   += wrptr[i]  * dlnLidlz;
		d2lnLdlz2 += wr2ptr[i] * (t1 + tr->rdta->xv * t2 - dlnLidlz * dlnLidlz);
	      }

	    if ((d2lnLdlz2 >= 0.0) && (z < zmax))
	      zprev = z = 0.37 * z + 0.63;
	    else
	      curvatOK = TRUE;

	  } 
	while (! curvatOK);

	if (d2lnLdlz2 < 0.0) 
	  {
	    z *= exp(-dlnLdlz / d2lnLdlz2);
	    if (z < zmin) z = zmin;
	    if (z > 0.25 * zprev + 0.75)
	      z = 0.25 * zprev + 0.75;
        }

	if (z > zmax) z = zmax;

      } 
    while ((--maxiter > 0) && (ABS(z - zprev) > zstep));

    free(abi);
    free(bci);
    free(sumci);
    


    return  z;
  } 


boolean newviewGTR(tree    *tr, nodeptr  p)
{
  
  if (p->tip) 
    {      
      if (p->x) return TRUE;
      if (! getxtip(p)) return FALSE;  
      {
	int code, i, j, jj;
	double (*EIGV)[4] = &(tr->rdta->EIGV[0][0]);
	likelivector *x1 = p->x->lv;
#pragma omp parallel for private(code, j)
	for(i = 0; i < tr->cdta->endsite; i++)
	  {	      	  	 
	    code = p->tip[i];
	    x1[i].a = 0;
	    x1[i].c = 0;
	    x1[i].g = 0;
	    x1[i].t = 0;
	    for (j = 0; j < 4; j++) 
	      {
		if ((code >> j) & 1) 
		  {
		    int jj = "0213"[j] - '0';		
		    x1[i].a += EIGV[jj][0];
		    x1[i].c += EIGV[jj][1];
		    x1[i].g += EIGV[jj][2];
		    x1[i].t += EIGV[jj][3];
		  }
	      }		
	      
	    x1[i].exp = 0;	  			  			 	 
	} 
      return TRUE; 
      }
    }
  else
    { 	
      double  diagptable_x1[4], diagptable_x2[4];
      double *EIGN    = &(tr->rdta->EIGN[0][0]);
      double *invfreq = &(tr->rdta->invfreq[0][0]);
      double (*EIGV)[4] = &(tr->rdta->EIGV[0][0]);
      xtype tmp_x1_1, tmp_x1_2, tmp_x1_3,
      tmp_x2_1, tmp_x2_2, tmp_x2_3, 
      ump_x1_1, ump_x1_2, ump_x1_3, ump_x1_0, 
      ump_x2_0, ump_x2_1, ump_x2_2, ump_x2_3, x1px2;
    int cat;
    int  *cptr;	  
    int  i, j;
    likelivector   *x1, *x2, *x3;
    double   z1, lz1, z2, lz2, ki;
    nodeptr  q, r;   
    double *eigv, *rptr;

    q = p->next->back;
    r = p->next->next->back;
      
    while ((! p->x) || (! q->x) || (! r->x)) {
      if (! q->x) if (! newviewGTR(tr, q)) return FALSE;
      if (! r->x) if (! newviewGTR(tr, r)) return FALSE;
      if (! p->x) if (! getxnode(p)) return FALSE;	
    }

    x1  = q->x->lv;
    z1  = q->z;
    lz1 = (z1 > zmin) ? log(z1) : log(zmin);

    x2  = r->x->lv;
    z2  = r->z;
    lz2 = (z2 > zmin) ? log(z2) : log(zmin);

    x3  = p->x->lv;   
   
    for (i = 1; i < 4; i++) 
      {
	diagptable_x1[i] = exp (EIGN[i] * lz1);
	diagptable_x2[i] = exp (EIGN[i] * lz2);	    
      }	       
      
  
#pragma omp parallel for private(tmp_x1_1, tmp_x1_2,tmp_x1_3,ump_x1_0,ump_x1_1,ump_x1_2,ump_x1_3,ump_x2_0,ump_x2_1,ump_x2_2,ump_x2_3,tmp_x2_1,tmp_x2_2,tmp_x2_3,x1px2)
    for (i = 0; i < tr->cdta->endsite; i++) 
      {	

	tmp_x1_1 = x1[i].c * diagptable_x1[1];
	tmp_x1_2 = x1[i].g * diagptable_x1[2];
	tmp_x1_3 = x1[i].t * diagptable_x1[3];
	
	ump_x1_0 = tmp_x1_1 * EIGV[0][1];
	ump_x1_0 += tmp_x1_2 * EIGV[0][2];
	ump_x1_0 += tmp_x1_3 * EIGV[0][3];	      	
	ump_x1_0 *= invfreq[0];
	ump_x1_0 += x1[i].a;
	
	ump_x1_1 = tmp_x1_1 * EIGV[1][1];
	ump_x1_1 += tmp_x1_2 * EIGV[1][2];
	ump_x1_1 += tmp_x1_3 * EIGV[1][3];	      	
	ump_x1_1 *= invfreq[1];
	ump_x1_1 += x1[i].a;
	    
	ump_x1_2 = tmp_x1_1 * EIGV[2][1];
	ump_x1_2 += tmp_x1_2 * EIGV[2][2];
	ump_x1_2 += tmp_x1_3 * EIGV[2][3];	      	
	ump_x1_2 *= invfreq[2];
	ump_x1_2 += x1[i].a;

	ump_x1_3 = tmp_x1_1 * EIGV[3][1];
	ump_x1_3 += tmp_x1_2 * EIGV[3][2];
	ump_x1_3 += tmp_x1_3 * EIGV[3][3];	      	
	ump_x1_3 *= invfreq[3];
	ump_x1_3 += x1[i].a;
	
	tmp_x2_1 = x2[i].c * diagptable_x2[1];
	tmp_x2_2 = x2[i].g * diagptable_x2[2];
	tmp_x2_3 = x2[i].t * diagptable_x2[3];
	 
	 	    	    	     	     
	ump_x2_0 = tmp_x2_1 * EIGV[0][1];
	ump_x2_0 += tmp_x2_2 * EIGV[0][2];
	ump_x2_0 += tmp_x2_3 * EIGV[0][3];		     
	ump_x2_0 *= invfreq[0];
	ump_x2_0 += x2[i].a;
	  
	ump_x2_1 = tmp_x2_1 * EIGV[1][1];
	ump_x2_1 += tmp_x2_2 * EIGV[1][2];
	ump_x2_1 += tmp_x2_3 * EIGV[1][3];		     
	ump_x2_1 *= invfreq[1];
	ump_x2_1 += x2[i].a;	 

	ump_x2_2 = tmp_x2_1 * EIGV[2][1];
	ump_x2_2 += tmp_x2_2 * EIGV[2][2];
	ump_x2_2 += tmp_x2_3 * EIGV[2][3];		     
	ump_x2_2 *= invfreq[2];
	ump_x2_2 += x2[i].a;	  
		   

	ump_x2_3 = tmp_x2_1 * EIGV[3][1] ;
	ump_x2_3 += tmp_x2_2 * EIGV[3][2];
	ump_x2_3 += tmp_x2_3 * EIGV[3][3];		     
	ump_x2_3 *= invfreq[3];
	ump_x2_3 += x2[i].a;	    	  		   	  	
	
	x1px2 = ump_x1_0 * ump_x2_0;
	x3[i].a = x1px2 *  EIGV[0][0];
	x3[i].c = x1px2 *  EIGV[0][1];
	x3[i].g = x1px2 *  EIGV[0][2];
	x3[i].t = x1px2 *  EIGV[0][3]; 
	
	x1px2 = ump_x1_1 * ump_x2_1;
	x3[i].a += x1px2  *  EIGV[1][0];
	x3[i].c += x1px2 *   EIGV[1][1];
	x3[i].g += x1px2 *   EIGV[1][2];
	x3[i].t += x1px2 *   EIGV[1][3];
	
	x1px2 = ump_x1_2 * ump_x2_2;
	x3[i].a += x1px2 *  EIGV[2][0];
	x3[i].c += x1px2 *  EIGV[2][1];
	x3[i].g += x1px2 *  EIGV[2][2];
	x3[i].t += x1px2 *  EIGV[2][3];
	
	x1px2 = ump_x1_3 * ump_x2_3;
	x3[i].a += x1px2 *   EIGV[3][0];
	x3[i].c += x1px2 *   EIGV[3][1];
	x3[i].g += x1px2 *   EIGV[3][2];
	x3[i].t += x1px2 *   EIGV[3][3];
		   
	x3[i].exp = x1[i].exp + x2[i].exp;		  
	
	if (ABS(x3[i].a) < minlikelihood && ABS(x3[i].g) < minlikelihood
	    && ABS(x3[i].c) < minlikelihood && ABS(x3[i].t) < minlikelihood) 
	  {	     
	    x3[i].a   *= twotothe256;
	    x3[i].g   *= twotothe256;
	    x3[i].c   *= twotothe256;
	    x3[i].t   *= twotothe256;
	    x3[i].exp += 1;
	    }
      }   
    
   return TRUE; 
    }
}





double evaluateGTR (tr, p)
    tree    *tr;
    nodeptr  p;
  { /* evaluate */
    double   sum, z, lz, term;    
    nodeptr  q;
    int     i, j;
    int     *wptr = tr->cdta->aliaswgt;
    
    double  diagptable[4];
    double *EIGN = &(tr->rdta->EIGN[0][0]);
    likelivector   *x1, *x2;
       
    q = p->back;
    while ((! p->x) || (! q->x)) {
      if (! (p->x)) if (! newviewGTR(tr, p)) return badEval;
      if (! (q->x)) if (! newviewGTR(tr, q)) return badEval;
    }


    x1  = p->x->lv;
    x2  = q->x->lv;
    z = p->z;
   
    if (z < zmin) z = zmin;
    lz = log(z);

    diagptable[0] = 1;
    diagptable[1] = exp (EIGN[1] * lz);
    diagptable[2] = exp (EIGN[2] * lz);
    diagptable[3] = exp (EIGN[3] * lz);
    
    
    
    sum = 0.0;
#pragma omp parallel for private(term) reduction(+ : sum)  
    for (i = 0; i < tr->cdta->endsite; i++) 
      {	  	 	  
	term =  x1[i].a * x2[i].a * diagptable[0];
	term += x1[i].c * x2[i].c * diagptable[1];
	term += x1[i].g * x2[i].g * diagptable[2];
	term += x1[i].t * x2[i].t * diagptable[3];     
	term = log(term) + (x1[i].exp + x2[i].exp)*log(minlikelihood);
	sum += wptr[i] * term;		  
      }
    
    tr->likelihood = sum;
    return  sum;
  } /* evaluate */



double makenewzGTR (tr, p, q, z0, maxiter)
    tree    *tr;
    nodeptr  p, q;
    double   z0;
    int  maxiter;
  { /* makenewz */
    double   z, zprev, zstep;
    likelivector  *x1, *x2;
    double  *sum0, *sum1, *sum2, *sum3;
    int     i;
    double  dlnLdlz = 0.0;                 /*  = d(ln(likelihood))/d(lz) */
    double  d2lnLdlz2 = 0.0;               /*  = d2(ln(likelihood))/d(lz)2 */    
    double d0, d1, d2, d3;
    double e0, e1, e2, e3;

    while ((! p->x) || (! q->x)) {
      if (! (p->x)) if (! newviewGTR(tr, p)) return badZ;
      if (! (q->x)) if (! newviewGTR(tr, q)) return badZ;
    }

    x1 = p->x->lv;
    x2 = q->x->lv;

   sum0  = (double *)malloc(tr->cdta->endsite * sizeof(double));
   sum1  = (double *)malloc(tr->cdta->endsite * sizeof(double));  
   sum2  = (double *)malloc(tr->cdta->endsite * sizeof(double)); 
   sum3  = (double *)malloc(tr->cdta->endsite * sizeof(double));

#pragma omp parallel for
   for (i = 0; i < tr->cdta->endsite; i++) 
     {     
       sum0[i] = x1[i].a * x2[i].a;
       sum1[i] = x1[i].c * x2[i].c;
       sum2[i] = x1[i].g * x2[i].g;
       sum3[i] = x1[i].t * x2[i].t;		     
     }

   
   

    z = z0;
    do {
      int curvatOK = FALSE;

      zprev = z;
      
      zstep = (1.0 - zmax) * z + zmin;

      do {
	double  diagptable[4];
	double *EIGN    = &(tr->rdta->EIGN[0][0]);
	double lz;
        double *wrptr   = &(tr->cdta->wr[0]);
        double *wr2ptr  = &(tr->cdta->wr2[0]);
	double tmp_0, tmp_1, tmp_2, tmp_3;
	double t, inv_Li, dlnLidlz, d2lnLidlz2;
        dlnLdlz = 0.0;                 /*  = d(ln(likelihood))/d(lz) */
        d2lnLdlz2 = 0.0;               /*  = d2(ln(likelihood))/d(lz)2 */

        if (z < zmin) z = zmin;
        else if (z > zmax) z = zmax;
        lz    = log(z);
        
	d1 = exp (EIGN[1] * lz); 
	d2 = exp (EIGN[2] * lz); 
	d3 = exp (EIGN[3] * lz); 

#pragma omp parallel for private(inv_Li, t, tmp_0, tmp_1, tmp_2, tmp_3, dlnLidlz, d2lnLidlz2) reduction(+ :  dlnLdlz) reduction(+ : d2lnLdlz2)
        for (i = 0; i < tr->cdta->endsite; i++) 
	  {
	   
	    	 
	    inv_Li = (tmp_0 = sum0[i]);
	    inv_Li += (tmp_1 = sum1[i] * d1);
	    inv_Li += (tmp_2 = sum2[i] * d2);
	    inv_Li += (tmp_3 = sum3[i] * d3);

	    inv_Li = 1.0/inv_Li;
	    
	  
	    t = tmp_1 * EIGN[1];
	    dlnLidlz   = t;
	    d2lnLidlz2 = t * EIGN[1];
	    
	    t = tmp_2 * EIGN[2];
	    dlnLidlz   += t;
	    d2lnLidlz2 += t * EIGN[2];
	    
	    t = tmp_3 * EIGN[3];
	    dlnLidlz   += t;
	    d2lnLidlz2 += t * EIGN[3];
	    
	    dlnLidlz   *= inv_Li;
	    d2lnLidlz2 *= inv_Li;
	    dlnLdlz   += wrptr[i]  * dlnLidlz;
	    d2lnLdlz2 += wr2ptr[i] * (d2lnLidlz2 - dlnLidlz * dlnLidlz);
	  }
	
        if ((d2lnLdlz2 >= 0.0) && (z < zmax))
          zprev = z = 0.37 * z + 0.63;  /*  Bad curvature, shorten branch */
        else
          curvatOK = TRUE;
	
      } while (! curvatOK);

      if (d2lnLdlz2 < 0.0) {
	double tantmp = -dlnLdlz / d2lnLdlz2;  /* prevent overflow */
	if (tantmp < 100) {
	  z *= exp(tantmp);
	  if (z < zmin) z = zmin;
	  if (z > 0.25 * zprev + 0.75)    /*  Limit steps toward z = 1.0 */
	    z = 0.25 * zprev + 0.75;
	} else {
	  z = 0.25 * zprev + 0.75;
	}
      }
      if (z > zmax) z = zmax;

    } while ((--maxiter > 0) && (ABS(z - zprev) > zstep));

    free (sum0);
    free (sum1);
    free (sum2);
    free (sum3);

    return  z;
  } /* makenewz */



boolean newviewHKY85CAT (tree *tr, nodeptr p)        /*  Update likelihoods at node */
  { /* newview */
    double   zq, lzq, xvlzq, zr, lzr, xvlzr;
    nodeptr  q,r;
    likelivector *lp, *lq, *lr;
    int  i;


    if (p->tip) {             /*  Make sure that data are at tip */
      likelivector *l;
      int           code;
      yType        *yptr;

      if (p->x) return TRUE;  /*  They are already there */

      if (! getxtip(p)) return FALSE; /*  They are not, so get memory */
      l = p->x->lv;           /*  Pointer to first likelihood vector value */
      yptr = p->tip;          /*  Pointer to first nucleotide datum */
      

      /* OpenMP Section start INCREASE */

      

#pragma omp parallel for private(code)
      for (i = 0; i < tr->cdta->endsite; i++) {
        code = yptr[i];
        l[i].a =  code       & 1;
        l[i].c = (code >> 1) & 1;
        l[i].g = (code >> 2) & 1;
        l[i].t = (code >> 3) & 1;
        l[i].exp = 0;

        }
      
        return TRUE;
    }
   
/*  Internal node needs update */

    q = p->next->back;
    r = p->next->next->back;

    

    while ((! p->x) || (! q->x) || (! r->x)) {
      if (! q->x) if (! newviewHKY85CAT(tr, q))  return FALSE;
      if (! r->x) if (! newviewHKY85CAT(tr, r))  return FALSE;
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

    

    { double  *zzqtable, *zvqtable,
              *zzrtable, *zvrtable,
             *zzqptr, *zvqptr, *zzrptr, *zvrptr, *rptr;
      double  fxqr, fxqy, fxqn, sumaq, sumgq, sumcq, sumtq,
              fxrr, fxry, fxrn, ki, tempi, tempj;
      int  *cptr;
      double zzq, zvq, zzr, zvr;
      int cat;

      zzqtable = (double *)malloc(tr->NumberOfCategories * sizeof(double));
      zvqtable = (double *)malloc(tr->NumberOfCategories * sizeof(double));
      zzrtable = (double *)malloc(tr->NumberOfCategories * sizeof(double));
      zvrtable = (double *)malloc(tr->NumberOfCategories * sizeof(double));
    
      rptr   = &(tr->cdta->patrat[0]);
      zzqptr = &(zzqtable[0]);
      zvqptr = &(zvqtable[0]);
      zzrptr = &(zzrtable[0]);
      zvrptr = &(zvrtable[0]);
      cptr  = &(tr->cdta->rateCategory[0]);

      /* OpenMP : INCREASE  small */

     
 	
#pragma omp parallel for private(ki)
      for (i = 0; i < tr->NumberOfCategories; i++) 
	{   

	  ki = tr->cdta->patrat[i];	
	
	  zzqtable[i] = exp(ki *   lzq);
	  zvqtable[i] = exp(ki * xvlzq);
	  zzrtable[i] = exp(ki *   lzr);
	  zvrtable[i] = exp(ki * xvlzr);
	  
	}
     
#pragma omp parallel for private(cat,zzq,zvq,fxqr,fxqy,fxqn,tempi,tempj,sumaq,sumgq,sumcq,sumtq,zzr,zvr,fxrr,fxry,fxrn)
      for (i = 0; i < tr->cdta->endsite; i++) 
	{
	  cat = tr->cdta->rateCategory[i];
	  
	  zzq = zzqtable[cat];
	  zvq = zvqtable[cat];
	  
	  fxqr = tr->rdta->freqa * lq[i].a + tr->rdta->freqg * lq[i].g;
	  fxqy = tr->rdta->freqc * lq[i].c + tr->rdta->freqt * lq[i].t;
	  fxqn = fxqr + fxqy;
	  tempi = fxqr * tr->rdta->invfreqr;
	  tempj = zvq * (tempi-fxqn) + fxqn;
	  sumaq = zzq * (lq[i].a - tempi) + tempj;
	  sumgq = zzq * (lq[i].g - tempi) + tempj;
	  tempi = fxqy * tr->rdta->invfreqy;
	  tempj = zvq * (tempi-fxqn) + fxqn;
	  sumcq = zzq * (lq[i].c - tempi) + tempj;
	  sumtq = zzq * (lq[i].t - tempi) + tempj;
	  
	  zzr = zzrtable[cat];
	  zvr = zvrtable[cat];
	  fxrr = tr->rdta->freqa * lr[i].a + tr->rdta->freqg * lr[i].g;
	  fxry = tr->rdta->freqc * lr[i].c + tr->rdta->freqt * lr[i].t;
	  fxrn = fxrr + fxry;
	  tempi = fxrr * tr->rdta->invfreqr;
	  tempj = zvr * (tempi-fxrn) + fxrn;
	  lp[i].a = sumaq * (zzr * (lr[i].a - tempi) + tempj);
	  lp[i].g = sumgq * (zzr * (lr[i].g - tempi) + tempj);
	  tempi = fxry * tr->rdta->invfreqy;
	  tempj = zvr * (tempi-fxrn) + fxrn;
	  lp[i].c = sumcq * (zzr * (lr[i].c - tempi) + tempj);
	  lp[i].t = sumtq * (zzr * (lr[i].t - tempi) + tempj);
	  lp[i].exp = lq[i].exp + lr[i].exp;
	 	  
	  if (lp[i].a < minlikelihood && lp[i].g < minlikelihood
	      && lp[i].c < minlikelihood && lp[i].t < minlikelihood) 
	    {
	      lp[i].a   *= twotothe256;
	      lp[i].g   *= twotothe256;
	      lp[i].c   *= twotothe256;
	      lp[i].t   *= twotothe256;
	      lp[i].exp += 1;
            }	  
	}

     

      free(zzqtable);
      free(zvqtable);
      free(zzrtable);
      free(zvrtable);
    }
    return TRUE;
  }


/* OpenMP : Parallelize Function */

double evaluateHKY85CAT (tree *tr, nodeptr p)
  { /* evaluate */
    double   sum, z, lz, xvlz,
             ki, fxpa, fxpc, fxpg, fxpt, fxpr, fxpy, fxqr, fxqy,
             suma, sumb, sumc, term;


       double   zz, zv;

    double        *zztable, *zvtable,
                 *zzptr, *zvptr;
    double        *rptr;
    likelivector *lp, *lq;
    nodeptr       q;
    int           cat, *cptr, i, *wptr;


    
    q = p->back;
    while ((! p->x) || (! q->x)) {
      if (! (p->x)) if (! newviewHKY85CAT(tr, p)) return badEval;
      if (! (q->x)) if (! newviewHKY85CAT(tr, q)) return badEval;
      }

    lp = p->x->lv;
    lq = q->x->lv;

    z = p->z;
    if (z < zmin) z = zmin;
    lz = log(z);
    xvlz = tr->rdta->xv * lz;

    zztable = (double *)malloc((tr->NumberOfCategories) * sizeof(double));
    zvtable = (double *)malloc((tr->NumberOfCategories) * sizeof(double));

    rptr   = tr->cdta->patrat;
    cptr  = tr->cdta->rateCategory; 
    zzptr = zztable;
    zvptr = zvtable;

    sum = 0.0;
    wptr = &(tr->cdta->aliaswgt[0]);

    

#pragma omp parallel for private(ki)
    for (i = 0; i < tr->NumberOfCategories; i++) 
      {
	ki = tr->cdta->patrat[i];
	zztable[i] = exp(ki *   lz);
	zvtable[i] = exp(ki * xvlz);
      }




    
#pragma omp parallel for private(cat, zz, zv, fxpa, fxpg, fxpc, fxpt, suma, fxqr, fxqy, fxpr, fxpy, sumb, sumc, term) reduction(+ : sum)
    for (i = 0; i < tr->cdta->endsite; i++) 
      {
	cat  = cptr[i];
	zz   = zztable[cat];
	zv   = zvtable[cat];
	fxpa = tr->rdta->freqa * lp[i].a;
        fxpg = tr->rdta->freqg * lp[i].g;
        fxpc = tr->rdta->freqc * lp[i].c;
        fxpt = tr->rdta->freqt * lp[i].t;
        suma = fxpa * lq[i].a + fxpc * lq[i].c + fxpg * lq[i].g + fxpt * lq[i].t;
        fxqr = tr->rdta->freqa * lq[i].a + tr->rdta->freqg * lq[i].g;
        fxqy = tr->rdta->freqc * lq[i].c + tr->rdta->freqt * lq[i].t;
        fxpr = fxpa + fxpg;
        fxpy = fxpc + fxpt;
        sumc = (fxpr + fxpy) * (fxqr + fxqy);
        sumb = fxpr * fxqr * tr->rdta->invfreqr + fxpy * fxqy * tr->rdta->invfreqy;
        suma -= sumb;
        sumb -= sumc;
        term = log(zz * suma + zv * sumb + sumc) + (lp[i].exp + lq[i].exp)*log(minlikelihood);

        sum += tr->cdta->aliaswgt[i] * term;
      }

    
    free(zztable);
    free(zvtable); 
    
    tr->likelihood = sum;
    
    return  sum;
  }


double makenewzHKY85CAT (tree *tr, nodeptr p, nodeptr q, double z0, int maxiter)
  { /* makenewz */
    likelivector *lp, *lq;
    double  *abi, *bci, *sumci, *abptr, *bcptr, *sumcptr;
    double   dlnLidlz, dlnLdlz, d2lnLdlz2, z, zprev, zstep, lz, xvlz,
             ki, suma, sumb, sumc, ab, bc, inv_Li, t1, t2,
             fx1a, fx1c, fx1g, fx1t, fx1r, fx1y, fx2r, fx2y;
    double   *zztable, *zvtable,
            *zzptr, *zvptr;
    double  *rptr, *wrptr, *wr2ptr;
    int      cat, *cptr, i, curvatOK;

    
    while ((! p->x) || (! q->x)) {
      if (! (p->x)) if (! newviewHKY85CAT(tr, p)) return badZ;
      if (! (q->x)) if (! newviewHKY85CAT(tr, q)) return badZ;
      }

    lp = p->x->lv;
    lq = q->x->lv;

    {
      unsigned scratch_size;
      scratch_size = sizeof(double) * tr->cdta->endsite;
      abi   = (double *) malloc(scratch_size);
      bci   = (double *) malloc(scratch_size);
      sumci = (double *) malloc(scratch_size);      
    }
    
    zztable = (double *)malloc(tr->NumberOfCategories * sizeof(double));
    zvtable = (double *)malloc(tr->NumberOfCategories * sizeof(double));

    abptr   = abi;
    bcptr   = bci;
    sumcptr = sumci;
   
#pragma omp parallel for private(fx1a, fx1g, fx1c, fx1t, suma, fx2r, fx2y, fx1r, fx1y, sumb, sumc)
    for (i = 0; i < tr->cdta->endsite; i++) 
      {
	fx1a = tr->rdta->freqa * lp[i].a;
	fx1g = tr->rdta->freqg * lp[i].g;
	fx1c = tr->rdta->freqc * lp[i].c;
	fx1t = tr->rdta->freqt * lp[i].t;
	suma = fx1a * lq[i].a + fx1c * lq[i].c + fx1g * lq[i].g + fx1t * lq[i].t;
	fx2r = tr->rdta->freqa * lq[i].a + tr->rdta->freqg * lq[i].g;
	fx2y = tr->rdta->freqc * lq[i].c + tr->rdta->freqt * lq[i].t;
	fx1r = fx1a + fx1g;
	fx1y = fx1c + fx1t;
	sumcptr[i] = sumc = (fx1r + fx1y) * (fx2r + fx2y);
	sumb       = fx1r * fx2r * tr->rdta->invfreqr + fx1y * fx2y * tr->rdta->invfreqy;
	abptr[i]   = suma - sumb;
	bcptr[i]   = sumb - sumc;
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
        rptr  = tr->cdta->patrat;
	cptr  = tr->cdta->rateCategory;
        zzptr = zztable;
        zvptr = zvtable;
      	       
        abptr   = abi;
        bcptr   = bci;
        sumcptr = sumci;        
        wrptr   = &(tr->cdta->wr[0]);
        wr2ptr  = &(tr->cdta->wr2[0]);
        dlnLdlz = 0.0;                 /*  = d(ln(likelihood))/d(lz) */
        d2lnLdlz2 = 0.0;               /*  = d2(ln(likelihood))/d(lz)2 */

#pragma omp parallel for private(ki)
        for (i = 0; i < tr->NumberOfCategories; i++) 
	  {
	    ki = tr->cdta->patrat[i];
	    zztable[i] = exp(ki *   lz);
	    zvtable[i] = exp(ki * xvlz);
          }

#pragma omp parallel for private(cat, ab, bc, sumc, inv_Li, t1, t2, dlnLidlz) reduction(+ : dlnLdlz) reduction( + : d2lnLdlz2) 
        for (i = 0; i < tr->cdta->endsite; i++) 
	  {
	    cat    = cptr[i];              /*  ratecategory(i) */
	    ab     = abptr[i] * zztable[cat];
	    bc     = bcptr[i] * zvtable[cat];
	    sumc   = sumcptr[i];
	    inv_Li = 1.0/(ab + bc + sumc);
	    t1     = ab * inv_Li;
	    t2     = tr->rdta->xv * bc * inv_Li;
	    dlnLidlz   = t1 + t2;
	    
	    dlnLdlz   += wrptr[i]  * dlnLidlz;
	    d2lnLdlz2 += wr2ptr[i] * (t1 + tr->rdta->xv * t2 - dlnLidlz * dlnLidlz);
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
  
  
    free(abi);
    free(bci);
    free(sumci);
    free(zztable);
    free(zvtable); 

/* printf("makenewz: %le\n", z); */

    return  z;
  } /* makenewz */
#endif



#ifdef NORMAL

boolean newviewHKY85(tree *tr, nodeptr p)        /*  Update likelihoods at node */
  { /* newview */
    double   zq, lzq, xvlzq, zr, lzr, xvlzr;
    nodeptr  q, r, realP;
    likelivector *lp, *lq, *lr;
    homType *mlvP;
    int  i;
           
    if(p->tip) 
      {
	if (p->x) return TRUE;

	if (! getxtip(p)) return FALSE;

	return TRUE;
      }
      
    q = p->next->back;
    r = p->next->next->back;

    while ((! p->x) || (! q->x) || (! r->x)) 
      {
	if (! q->x) if (! newviewHKY85(tr, q))  return FALSE;
	if (! r->x) if (! newviewHKY85(tr, r))  return FALSE;
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
      double  fxqr, fxqy, fxqn, sumaq, sumgq, sumcq, sumtq,
              fxrr, fxry, fxrn, ki, tempi, tempj;                
      register  int *s1; 
      register  int *s2;
      register int *eqptr;
      
      homType *mlvR, *mlvQ;
      poutsa calc[EQUALITIES],calcR[EQUALITIES], *calcptr, *calcptrR;
      long ref, ref2, mexp;      
      nodeptr realQ, realR;
    
      /* get equality vectors of p, q, and r */
      
      realP =  tr->nodep[p->number];
    
      mlvP = realP->mlv;
      realQ =  tr->nodep[q->number];
      realR =  tr->nodep[r->number];


      eqptr = realP->equalityVector; 
      s1 = realQ->equalityVector;
      s2 = realR->equalityVector; 
                 
      zzq = exp(lzq);
      zvq = exp(xvlzq);
      zzr = exp(lzr);        
      zvr = exp(xvlzr); 
      
      mlvQ = realQ->mlv;
      mlvR = realR->mlv;


     
           	 	  
      for(i = 0; i < equalities; i++)
	{
	  
	  if(mlvQ->set)
	    {
	      calcptr = &calc[i];		 
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
	  if(mlvR->set)
	    {
	      calcptrR = &calcR[i];		 
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
	  
	  mlvQ++;
	  mlvR++;
	  mlvP++;
	}


      if(r->tip && q->tip)
	{

	 

	   for (i = 0; i < tr->cdta->endsite; i++) 
	    {
	      ref = *s2++;
	      ref2 = *s1++;
	     	      	      		  	
	      if(ref2 != ref)
		{
		  *eqptr++ = -1;
		  calcptrR = &calcR[ref];
		  calcptr = &calc[ref2];
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
		  lp++;   
		}
	      else
		{
		  *eqptr++ = ref;		    		
		}
	   	      	    
	    }          
	}
      else
	{
	  	  

	 
	  
	  for (i = 0; i < tr->cdta->endsite; i++) 
	    {
	      ref = *s2++;
	      ref2 = *s1++;
	      
	      if(ref2 >= 0)
		{		  	
		  if(ref2 == ref)
		    {
		      *eqptr++ = ref;		     		  
		      goto incTip;		    		  		 		  	    		
		    }
		  calcptr = &calc[ref2];
		  sumaq = calcptr->sumaq;
		  sumgq = calcptr->sumgq;
		  sumcq = calcptr->sumcq;
		  sumtq = calcptr->sumtq;		 		  	
		  mexp = calcptr->exp;	       		  
		}	    	  	  	 	 
	      else
		{		
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
		  lq++;
		}
	      	      
	      if(ref >= 0)
		{
		  calcptr = &calcR[ref];		 	    
		  lp->a = sumaq * calcptr->sumaq;
		  lp->g = sumgq * calcptr->sumgq;		    
		  lp->c = sumcq * calcptr->sumcq; 
		  lp->t = sumtq * calcptr->sumtq;
		  lp->exp = mexp + calcptr->exp;		  	  
		}
	      else
		{
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
		  lr++;
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
	      lp++;
	    incTip:	 ;
	    }      
	
    
	}
    }
    
   
    

    return TRUE;  
  } /* newview */

 /* AxML modification end*/





double evaluateHKY85(tree *tr, nodeptr p)
  { /* evaluate */
    double   sum, z, lz, xvlz,
             ki, fx1a, fx1c, fx1g, fx1t, fx1r, fx1y, fx2r, fx2y,
             suma, sumb, sumc, term, zzp, zvp;           
    likelivector *lp, *lq;
    homType *mlvP, *mlvQ;
    nodeptr       q, realQ, realP;
    int           i, *wptr;
    register  int *sQ; 
    register  int *sP;
    long refQ, refP;
    memP calcP[EQUALITIES], *calcptrP;
    memQ calcQ[EQUALITIES], *calcptrQ;

    q = p->back;

    while ((! p->x) || (! q->x)) {
      if (! (p->x)) if (! newviewHKY85(tr, p)) return badEval;
      if (! (q->x)) if (! newviewHKY85(tr, q)) return badEval;
    }

    lp = p->x->lv;
    lq = q->x->lv;

    z = p->z;
    if (z < zmin) z = zmin;
    lz = log(z);
    xvlz = tr->rdta->xv * lz;

    
    zzp = exp(lz);
    zvp = exp(xvlz);
      
    wptr = &(tr->cdta->aliaswgt[0]);
   
    sum = 0.0;
    
    realQ =  tr->nodep[q->number];
    realP =  tr->nodep[p->number];
    mlvP = realP->mlv;
    mlvQ = realQ->mlv;
    sQ = realQ->equalityVector;
    sP = realP->equalityVector; 

    

    for(i = 0; i < equalities; i++)
	{
	 
	  if(mlvP->set)
	    {
	      calcptrP = &calcP[i];		 
	      calcptrP->fx1a = tr->rdta->freqa * mlvP->a;
	      calcptrP->fx1g = tr->rdta->freqg * mlvP->g;
	      calcptrP->fx1c = tr->rdta->freqc * mlvP->c;
	      calcptrP->fx1t = tr->rdta->freqt * mlvP->t;
	      calcptrP->sumag = calcptrP->fx1a + calcptrP->fx1g;
	      calcptrP->sumct = calcptrP->fx1c + calcptrP->fx1t;
	      calcptrP->sumagct = calcptrP->sumag + calcptrP->sumct;	      
	      calcptrP->exp = mlvP->exp;
	    }
	 
	  if(mlvQ->set)
	    {
	      calcptrQ = &calcQ[i];
	      calcptrQ->fx2r = tr->rdta->freqa * mlvQ->a + tr->rdta->freqg * mlvQ->g;	     
	      calcptrQ->fx2y  = tr->rdta->freqc * mlvQ->c + tr->rdta->freqt * mlvQ->t; 	     
	      calcptrQ->sumfx2rfx2y = calcptrQ->fx2r + calcptrQ->fx2y;	      
	      calcptrQ->exp = mlvQ->exp;
	      calcptrQ->a = mlvQ->a;
	      calcptrQ->c = mlvQ->c;
	      calcptrQ->g = mlvQ->g;
	      calcptrQ->t = mlvQ->t;
	    }
	  
	  mlvQ++;	 
	  mlvP++;
	}




      for (i = 0; i < tr->cdta->endsite; i++) {
	    refQ = *sQ++;
	    refP = *sP++;

	    if(refP >= 0)
	      {
		if(refQ >= 0)
		  {
		    calcptrP = &calcP[refP];
		    calcptrQ = &calcQ[refQ];
		    suma = calcptrP->fx1a * calcptrQ->a + 
		      calcptrP->fx1c * calcptrQ->c + 
		      calcptrP->fx1g * calcptrQ->g + calcptrP->fx1t * calcptrQ->t;		 		 
		    
		    sumc = calcptrP->sumagct * calcptrQ->sumfx2rfx2y;
		    sumb = calcptrP->sumag * calcptrQ->fx2r * tr->rdta->invfreqr + calcptrP->sumct * calcptrQ->fx2y * tr->rdta->invfreqy;
		    suma -= sumb;
		    sumb -= sumc;
		    term = log(zzp * suma + zvp * sumb + sumc) + 
		      (calcptrP->exp + calcptrQ->exp)*log(minlikelihood);
		    sum += *wptr++ * term;
		    
		   
		  }
		else
		  {
		    calcptrP = &calcP[refP];
		    suma = calcptrP->fx1a * lq->a + calcptrP->fx1c * lq->c + 
		      calcptrP->fx1g * lq->g + calcptrP->fx1t * lq->t;
		    
		    fx2r = tr->rdta->freqa * lq->a + tr->rdta->freqg * lq->g;
		    fx2y = tr->rdta->freqc * lq->c + tr->rdta->freqt * lq->t;
		    
		    sumc = calcptrP->sumagct * (fx2r + fx2y);
		    sumb       = calcptrP->sumag * fx2r * tr->rdta->invfreqr + calcptrP->sumct * fx2y * tr->rdta->invfreqy;
		    suma -= sumb;
		    sumb -= sumc;
		    term = log(zzp * suma + zvp * sumb + sumc) + 
		      (calcptrP->exp + lq->exp)*log(minlikelihood);
		    sum += *wptr++ * term;
		    
		   
		    lq++;
		  }
	      }
	    else
	      {
		if(refQ >= 0)
		  {
		    calcptrQ = &calcQ[refQ];
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
		    term = log(zzp * suma + zvp * sumb + sumc) + 
		      (lp->exp + calcptrQ->exp)*log(minlikelihood);		
		    sum += *wptr++ * term;	
		    
		    lp++;
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
		    term = log(zzp * suma + zvp * sumb + sumc) + 
		      (lp->exp + lq->exp)*log(minlikelihood);
		    sum += *wptr++ * term;		
		   
		    lp++;
		    lq++;
		  }
	      }	  
	   
      }

     
    tr->likelihood = sum; 
    return  sum;
   
  } /* evaluate */


double makenewzHKY85 (tree *tr, nodeptr p, nodeptr q, double z0, int maxiter)
{
  likelivector *lp, *lq;
    double  *abi, *bci, *sumci, *abptr, *bcptr, *sumcptr;
    double   dlnLidlz, dlnLdlz, d2lnLdlz2, z, zprev, zstep, lz, xvlz,
             ki, suma, sumb, sumc, ab, bc, inv_Li, t1, t2,
             fx1a, fx1c, fx1g, fx1t, fx1r, fx1y, fx2r, fx2y;
    double  zz, zv;
    double   *wrptr, *wr2ptr;
    int      i, curvatOK;
    homType *mlvP, *mlvQ;
    nodeptr realQ, realP;
    register  int *sQ; 
    register  int *sP;
    long refQ, refP;

    while ((! p->x) || (! q->x)) {
      if (! (p->x)) if (! newviewHKY85(tr, p)) return badZ;
      if (! (q->x)) if (! newviewHKY85(tr, q)) return badZ;
      }

    lp = p->x->lv;
    lq = q->x->lv;

    { 
      unsigned scratch_size;
      scratch_size = sizeof(double) * tr->cdta->endsite;
      abi   = (double *) malloc(scratch_size);
      bci   = (double *) malloc(scratch_size);
      sumci = (double *) malloc(scratch_size);      
    }
    
    
    

    abptr   = abi;
    bcptr   = bci;
    sumcptr = sumci;
    realQ =  tr->nodep[q->number];
    realP =  tr->nodep[p->number];
    mlvP = realP->mlv;
    mlvQ = realQ->mlv;
    sQ = realQ->equalityVector;
    sP = realP->equalityVector;


    for (i = 0; i < tr->cdta->endsite; i++) {
      refQ = *sQ++;
      refP = *sP++;
      if(refP >= 0)
	{
	  if(refQ >= 0)
	    {
	      mlvP = &realP->mlv[refP];
	      mlvQ = &realQ->mlv[refQ];
	      fx1a = tr->rdta->freqa * mlvP->a;
	      fx1g = tr->rdta->freqg * mlvP->g;
	      fx1c = tr->rdta->freqc * mlvP->c;
	      fx1t = tr->rdta->freqt * mlvP->t;
	      suma = fx1a * mlvQ->a + fx1c * mlvQ->c + fx1g * mlvQ->g + fx1t * mlvQ->t;
	      fx2r = tr->rdta->freqa * mlvQ->a + tr->rdta->freqg * mlvQ->g;
	      fx2y = tr->rdta->freqc * mlvQ->c + tr->rdta->freqt * mlvQ->t;
	      fx1r = fx1a + fx1g;
	      fx1y = fx1c + fx1t;
	      *sumcptr++ = sumc = (fx1r + fx1y) * (fx2r + fx2y);
	      sumb       = fx1r * fx2r * tr->rdta->invfreqr + fx1y * fx2y * tr->rdta->invfreqy;
	      *abptr++   = suma - sumb;
	      *bcptr++   = sumb - sumc;

		  
	    }
	  else
	    {
	      mlvP = &realP->mlv[refP];
	      fx1a = tr->rdta->freqa * mlvP->a;
	      fx1g = tr->rdta->freqg * mlvP->g;
	      fx1c = tr->rdta->freqc * mlvP->c;
	      fx1t = tr->rdta->freqt * mlvP->t;
	      suma = fx1a * lq->a + fx1c * lq->c + fx1g * lq->g + fx1t * lq->t;
	      fx2r = tr->rdta->freqa * lq->a + tr->rdta->freqg * lq->g;
	      fx2y = tr->rdta->freqc * lq->c + tr->rdta->freqt * lq->t;
	      fx1r = fx1a + fx1g;
	      fx1y = fx1c + fx1t;
	      *sumcptr++ = sumc = (fx1r + fx1y) * (fx2r + fx2y);
	      sumb       = fx1r * fx2r * tr->rdta->invfreqr + fx1y * fx2y * tr->rdta->invfreqy;
	      *abptr++   = suma - sumb;
	      *bcptr++   = sumb - sumc;
    		    		   
	      lq++;
	    }
	}
      else
	{
	  if(refQ >= 0)
	    {
	      mlvQ = &realQ->mlv[refQ];
	
	      fx1a = tr->rdta->freqa * lp->a;
	      fx1g = tr->rdta->freqg * lp->g;
	      fx1c = tr->rdta->freqc * lp->c;
	      fx1t = tr->rdta->freqt * lp->t;
	      suma = fx1a * mlvQ->a + fx1c * mlvQ->c + fx1g * mlvQ->g + fx1t * mlvQ->t;
	      fx2r = tr->rdta->freqa * mlvQ->a + tr->rdta->freqg * mlvQ->g;
	      fx2y = tr->rdta->freqc * mlvQ->c + tr->rdta->freqt * mlvQ->t;
	      fx1r = fx1a + fx1g;
	      fx1y = fx1c + fx1t;
	      *sumcptr++ = sumc = (fx1r + fx1y) * (fx2r + fx2y);
	      sumb       = fx1r * fx2r * tr->rdta->invfreqr + fx1y * fx2y * tr->rdta->invfreqy;
	      *abptr++   = suma - sumb;
	      *bcptr++   = sumb - sumc;
	      
	      lp++;
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
	      *sumcptr++ = sumc = (fx1r + fx1y) * (fx2r + fx2y);
	      sumb       = fx1r * fx2r * tr->rdta->invfreqr + 
		fx1y * fx2y * tr->rdta->invfreqy;
	      *abptr++   = suma - sumb;
	      *bcptr++   = sumb - sumc;
	      
	      lq++;
	      lp++;
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
        
	zz = exp(lz);
	zv = exp(xvlz);
          
	abptr   = abi;
	bcptr   = bci;
	sumcptr = sumci;        
	wrptr   = &(tr->cdta->wr[0]);
	wr2ptr  = &(tr->cdta->wr2[0]);
	dlnLdlz = 0.0;                 /*  = d(ln(likelihood))/d(lz) */
	d2lnLdlz2 = 0.0;               /*  = d2(ln(likelihood))/d(lz)2 */



        for (i = 0; i < tr->cdta->endsite; i++) {
         
          ab     = *abptr++ * zz;
          bc     = *bcptr++ * zv;
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

    free(abi);
    free(bci);
    free(sumci);
    

    

/* printf("makenewz: %le\n", z); */

    return  z;
}








boolean newviewGTR(tree    *tr, nodeptr  p)
{
  
  if (p->tip) 
    {
      
      if (p->x) return TRUE;
      if (! getxtip(p)) return FALSE;
      {
	homType *mlvP;
	int code, i, j, jj;
	double (*EIGV)[4] = &(tr->rdta->EIGV[0][0]);
	int refREV[EQUALITIES];
	int *ev = p->equalityVector;
                 
      mlvP = tr->nodep[p->number]->mlv;
     
      for(i = 0; i < EQUALITIES; i++)
	{	      	  	 
	  mlvP->a = 0;
	  mlvP->c = 0;
	  mlvP->g = 0;
	  mlvP->t = 0;
	  code = i + 1;
	  for (j = 0; j < 4; j++) 
	    {
	      if ((code >> j) & 1) 
		{
		  int jj = "0213"[j] - '0';		
		  mlvP->a += EIGV[jj][0];
		  mlvP->c += EIGV[jj][1];
		  mlvP->g += EIGV[jj][2];
		  mlvP->t += EIGV[jj][3];
		}
	    }		
	      
	  mlvP->exp = 0;	  			  			 
	  mlvP++;	 
	} 
      return TRUE; 
      }
    }
  
  { 	
    double  diagptable_x1[4], diagptable_x2[4];
    double *EIGN    = &(tr->rdta->EIGN[0][0]);
    double *invfreq = &(tr->rdta->invfreq[0][0]);
    double (*EIGV)[4] = &(tr->rdta->EIGV[0][0]);
    xtype tmp_x1_1, tmp_x1_2, tmp_x1_3,
      tmp_x2_1, tmp_x2_2, tmp_x2_3, 
      ump_x1_1, ump_x1_2, ump_x1_3, ump_x1_0, 
      ump_x2_0, ump_x2_1, ump_x2_2, ump_x2_3, x1px2;
    homType *mlvR, *mlvQ, *mlvP;
    poutsa calcQ[EQUALITIES],calcR[EQUALITIES], *calcptr, *calcptrR;
    long ref, ref2, mexp;      
    nodeptr realQ, realR, realP;	  
    int  i;
    likelivector   *x1, *x2, *x3;
    double   z1, lz1, z2, lz2;
    nodeptr  q, r;
    register int *eqptr, *s1, *s2;
    double *eigv;
    q = p->next->back;
    r = p->next->next->back;
      
    while ((! p->x) || (! q->x) || (! r->x)) {
      if (! q->x) if (! newviewGTR(tr, q)) return FALSE;
      if (! r->x) if (! newviewGTR(tr, r)) return FALSE;
      if (! p->x) if (! getxnode(p)) return FALSE;	
    }

    x1  = q->x->lv;
    z1  = q->z;
    lz1 = (z1 > zmin) ? log(z1) : log(zmin);

    x2  = r->x->lv;
    z2  = r->z;
    lz2 = (z2 > zmin) ? log(z2) : log(zmin);

    x3  = p->x->lv;
      
    realP =  tr->nodep[p->number];
    realQ =  tr->nodep[q->number];
    realR =  tr->nodep[r->number];

    mlvP = realP->mlv;
    mlvQ = realQ->mlv;
    mlvR = realR->mlv;

    eqptr = realP->equalityVector; 
    s1 = realQ->equalityVector;
    s2 = realR->equalityVector; 
     	  
    for (i = 1; i < 4; i++) 
      {
	diagptable_x1[i] = exp (EIGN[i] * lz1);
	diagptable_x2[i] = exp (EIGN[i] * lz2);	    
      }	       
    
    for(i = 0; i < EQUALITIES; i++)
      {	  
	calcptr = &calcQ[i];		 
	tmp_x1_1 = mlvQ->c * diagptable_x1[1];
	tmp_x1_2 = mlvQ->g * diagptable_x1[2];
	tmp_x1_3 = mlvQ->t * diagptable_x1[3];
	
	calcptr->sumaq = tmp_x1_1 * EIGV[0][1];
	calcptr->sumaq += tmp_x1_2 * EIGV[0][2];
	calcptr->sumaq += tmp_x1_3 * EIGV[0][3];	      	
	calcptr->sumaq *= invfreq[0];
	calcptr->sumaq += mlvQ->a;
	
	calcptr->sumcq = tmp_x1_1 * EIGV[1][1];
	calcptr->sumcq += tmp_x1_2 * EIGV[1][2];
	calcptr->sumcq += tmp_x1_3 * EIGV[1][3];	      	
	calcptr->sumcq *= invfreq[1];
	calcptr->sumcq += mlvQ->a;
	
	calcptr->sumgq = tmp_x1_1 * EIGV[2][1];
	calcptr->sumgq += tmp_x1_2 * EIGV[2][2];
	calcptr->sumgq += tmp_x1_3 * EIGV[2][3];	      	
	calcptr->sumgq *= invfreq[2];
	calcptr->sumgq += mlvQ->a;
	
	calcptr->sumtq = tmp_x1_1 * EIGV[3][1];
	calcptr->sumtq += tmp_x1_2 * EIGV[3][2];
	calcptr->sumtq += tmp_x1_3 * EIGV[3][3];	      	
	calcptr->sumtq *= invfreq[3];
	calcptr->sumtq += mlvQ->a;	  
	
	calcptr->exp = mlvQ->exp;
	
	calcptrR = &calcR[i];
	tmp_x2_1 = mlvR->c * diagptable_x2[1];
	tmp_x2_2 = mlvR->g * diagptable_x2[2];
	tmp_x2_3 = mlvR->t * diagptable_x2[3];
	
	calcptrR->sumaq = tmp_x2_1 * EIGV[0][1];
	calcptrR->sumaq += tmp_x2_2 * EIGV[0][2];
	calcptrR->sumaq += tmp_x2_3 * EIGV[0][3];		     
	calcptrR->sumaq *= invfreq[0];
	calcptrR->sumaq += mlvR->a;
	
	calcptrR->sumcq = tmp_x2_1 * EIGV[1][1];
	calcptrR->sumcq += tmp_x2_2 * EIGV[1][2];
	calcptrR->sumcq += tmp_x2_3 * EIGV[1][3];		     
	calcptrR->sumcq *= invfreq[1];
	calcptrR->sumcq += mlvR->a;	 
	
	calcptrR->sumgq = tmp_x2_1 * EIGV[2][1];
	calcptrR->sumgq += tmp_x2_2 * EIGV[2][2];
	calcptrR->sumgq += tmp_x2_3 * EIGV[2][3];		     
	calcptrR->sumgq *= invfreq[2];
	calcptrR->sumgq += mlvR->a;	  
	
	calcptrR->sumtq  = tmp_x2_1 * EIGV[3][1] ;
	calcptrR->sumtq  += tmp_x2_2 * EIGV[3][2];
	calcptrR->sumtq  += tmp_x2_3 * EIGV[3][3];		     
	calcptrR->sumtq  *= invfreq[3];
	calcptrR->sumtq  += mlvR->a;	    	  	      
	
	calcptrR->exp = mlvR->exp;
	



	mlvP->a = calcptrR->sumaq * calcptr->sumaq * EIGV[0][0];
	mlvP->c = calcptrR->sumaq * calcptr->sumaq * EIGV[0][1];
	mlvP->g = calcptrR->sumaq * calcptr->sumaq * EIGV[0][2];
	mlvP->t = calcptrR->sumaq * calcptr->sumaq * EIGV[0][3]; 
	
	mlvP->a += calcptrR->sumcq * calcptr->sumcq * EIGV[1][0];
	mlvP->c += calcptrR->sumcq * calcptr->sumcq * EIGV[1][1];
	mlvP->g += calcptrR->sumcq * calcptr->sumcq * EIGV[1][2];
	mlvP->t += calcptrR->sumcq * calcptr->sumcq * EIGV[1][3];
	
	mlvP->a += calcptrR->sumgq  * calcptr->sumgq * EIGV[2][0];
	mlvP->c += calcptrR->sumgq  * calcptr->sumgq * EIGV[2][1];
	mlvP->g += calcptrR->sumgq  * calcptr->sumgq * EIGV[2][2];
	mlvP->t += calcptrR->sumgq  * calcptr->sumgq * EIGV[2][3];
	
	mlvP->a += calcptrR->sumtq * calcptr->sumtq *  EIGV[3][0];
	mlvP->c += calcptrR->sumtq * calcptr->sumtq *  EIGV[3][1];
	mlvP->g += calcptrR->sumtq * calcptr->sumtq *  EIGV[3][2];
	mlvP->t += calcptrR->sumtq * calcptr->sumtq *  EIGV[3][3];		     
	
	mlvP->exp = calcptr->exp + calcptrR->exp;	
	if (ABS(mlvP->a) < minlikelihood && ABS(mlvP->g) < minlikelihood
	    && ABS(mlvP->c) < minlikelihood && ABS(mlvP->t) < minlikelihood) 
	  {	     
	    mlvP->a   *= twotothe256;
	    mlvP->g   *= twotothe256;
	    mlvP->c   *= twotothe256;
	    mlvP->t   *= twotothe256;
	    mlvP->exp += 1;
	  }
	mlvQ++;
	mlvR++;
	mlvP++;
      }

    if(r->tip && q->tip)
	{	 
	  for (i = 0; i < tr->cdta->endsite; i++) 
	    {
	      ref = *s2++;
	      ref2 = *s1++;
	     	      	      		  	
	      if(ref2 != ref)
		{
		  *eqptr++ = -1;
		  calcptrR = &calcR[ref];
		  calcptr = &calcQ[ref2];
		  eigv = &EIGV[0][0];
		  x1px2 = calcptrR->sumaq * calcptr->sumaq;
		  x3->a =  x1px2 * *eigv++;/*EIGV[0][0];*/
		  x3->c =  x1px2 * *eigv++;/*EIGV[0][1];*/
		  x3->g =  x1px2 * *eigv++;/*EIGV[0][2];*/
		  x3->t =  x1px2 * *eigv++;/*EIGV[0][3];*/
	    
		  x1px2 = calcptrR->sumcq * calcptr->sumcq;
		  x3->a +=  x1px2 * *eigv++;/*EIGV[1][0];*/
		  x3->c +=  x1px2 * *eigv++;/*EIGV[1][1];*/
		  x3->g +=  x1px2 * *eigv++;/*EIGV[1][2];*/
		  x3->t +=  x1px2 * *eigv++;/*EIGV[1][3];*/
	    
		  x1px2 = calcptrR->sumgq * calcptr->sumgq;
		  x3->a += x1px2 * *eigv++;/*EIGV[2][0];*/
		  x3->c += x1px2 * *eigv++;/*EIGV[2][1];*/
		  x3->g += x1px2 * *eigv++;/*EIGV[2][2];*/
		  x3->t += x1px2 * *eigv++;/*EIGV[2][3];*/
	    
		  x1px2 = calcptrR->sumtq * calcptr->sumtq;
		  x3->a += x1px2  *  *eigv++;/*EIGV[3][0];*/
		  x3->c += x1px2  *  *eigv++;/*EIGV[3][1];*/
		  x3->g += x1px2  *  *eigv++;/*EIGV[3][2];*/
		  x3->t += x1px2  *  *eigv++;/*EIGV[3][3];*/		     
		  
		  x3->exp = calcptrR->exp + calcptr->exp;
		  if (ABS(x3->a) < minlikelihood && ABS(x3->g) < minlikelihood
		      && ABS(x3->c) < minlikelihood && ABS(x3->t) < minlikelihood) 
		    {	     
		      x3->a   *= twotothe256;
		      x3->g   *= twotothe256;
		      x3->c   *= twotothe256;
		      x3->t   *= twotothe256;
		      x3->exp += 1;
		    }
		  x3++;   
		}
	      else
		{
		  *eqptr++ = ref;		    		
		}
	   	      	    
	    }          
	}
      else
	{
	  for (i = 0; i < tr->cdta->endsite; i++) 
	    {
	      ref = *s2++;
	      ref2 = *s1++;
	      
	      if(ref2 >= 0)
		{		  	
		  if(ref2 == ref)
		    {
		      *eqptr++ = ref;		     		  
		      goto incTip;		    		  		 		  	    		
		    }
		  calcptr = &calcQ[ref2];
		  ump_x1_0 = calcptr->sumaq;
		  ump_x1_1 = calcptr->sumcq;
		  ump_x1_2 = calcptr->sumgq;
		  ump_x1_3 = calcptr->sumtq;		 		  		       		  		
		  mexp = calcptr->exp;
		}	    	  	  	 	 
	      else
		{		
		  tmp_x1_1 = x1->c * diagptable_x1[1];
		  tmp_x1_2 = x1->g * diagptable_x1[2];
		  tmp_x1_3 = x1->t * diagptable_x1[3];
		  
		  ump_x1_0 = tmp_x1_1 * EIGV[0][1];
		  ump_x1_0 += tmp_x1_2 * EIGV[0][2];
		  ump_x1_0 += tmp_x1_3 * EIGV[0][3];	      	
		  ump_x1_0 *= invfreq[0];
		  ump_x1_0 += x1->a;
		  
		  ump_x1_1 = tmp_x1_1 * EIGV[1][1];
		  ump_x1_1 += tmp_x1_2 * EIGV[1][2];
		  ump_x1_1 += tmp_x1_3 * EIGV[1][3];	      	
		  ump_x1_1 *= invfreq[1];
		  ump_x1_1 += x1->a;
		  
		  ump_x1_2 = tmp_x1_1 * EIGV[2][1];
		  ump_x1_2 += tmp_x1_2 * EIGV[2][2];
		  ump_x1_2 += tmp_x1_3 * EIGV[2][3];	      	
		  ump_x1_2 *= invfreq[2];
		  ump_x1_2 += x1->a;

		  ump_x1_3 = tmp_x1_1 * EIGV[3][1];
		  ump_x1_3 += tmp_x1_2 * EIGV[3][2];
		  ump_x1_3 += tmp_x1_3 * EIGV[3][3];	      	
		  ump_x1_3 *= invfreq[3];
		  ump_x1_3 += x1->a;
		  mexp = x1->exp;
		  x1++;
		}
	      	      
	      if(ref >= 0)
		{
		  calcptr = &calcR[ref];		
		  eigv = &EIGV[0][0];
		  x3->a = ump_x1_0 * calcptr->sumaq * *eigv++;/*EIGV[0][0];*/
		  x3->c = ump_x1_0 * calcptr->sumaq *  *eigv++;/*EIGV[0][1];*/
		  x3->g = ump_x1_0 * calcptr->sumaq *  *eigv++;/*EIGV[0][2];*/
		  x3->t = ump_x1_0 * calcptr->sumaq *  *eigv++;/*EIGV[0][3]; */
		  
		  x3->a += ump_x1_1 * calcptr->sumcq *  *eigv++;/*EIGV[1][0];*/
		  x3->c += ump_x1_1 * calcptr->sumcq *  *eigv++;/*EIGV[1][1];*/
		  x3->g += ump_x1_1 * calcptr->sumcq *  *eigv++;/*EIGV[1][2];*/
		  x3->t += ump_x1_1 * calcptr->sumcq *  *eigv++;/*EIGV[1][3];*/
		  
		  x3->a += ump_x1_2 * calcptr->sumgq *  *eigv++;/*EIGV[2][0];*/
		  x3->c += ump_x1_2 * calcptr->sumgq *  *eigv++;/*EIGV[2][1];*/
		  x3->g += ump_x1_2 * calcptr->sumgq *  *eigv++;/*EIGV[2][2];*/
		  x3->t += ump_x1_2 * calcptr->sumgq *  *eigv++;/*EIGV[2][3];*/
		  
		  x3->a += ump_x1_3 *  calcptr->sumtq *  *eigv++;/*EIGV[3][0];*/
		  x3->c += ump_x1_3 *  calcptr->sumtq *  *eigv++;/*EIGV[3][1];*/
		  x3->g += ump_x1_3 *  calcptr->sumtq *  *eigv++;/*EIGV[3][2];*/
		  x3->t += ump_x1_3 *  calcptr->sumtq *  *eigv++;/*EIGV[3][3];*/
		 
		  x3->exp = mexp + calcptr->exp;
		}
	      else
		{
		   tmp_x2_1 = x2->c * diagptable_x2[1];
		   tmp_x2_2 = x2->g * diagptable_x2[2];
		   tmp_x2_3 = x2->t * diagptable_x2[3];
	 
	 	    	    	     	     
		   ump_x2_0 = tmp_x2_1 * EIGV[0][1];
		   ump_x2_0 += tmp_x2_2 * EIGV[0][2];
		   ump_x2_0 += tmp_x2_3 * EIGV[0][3];		     
		   ump_x2_0 *= invfreq[0];
		   ump_x2_0 += x2->a;
	  
		   ump_x2_1 = tmp_x2_1 * EIGV[1][1];
		   ump_x2_1 += tmp_x2_2 * EIGV[1][2];
		   ump_x2_1 += tmp_x2_3 * EIGV[1][3];		     
		   ump_x2_1 *= invfreq[1];
		   ump_x2_1 += x2->a;	 

		   ump_x2_2 = tmp_x2_1 * EIGV[2][1];
		   ump_x2_2 += tmp_x2_2 * EIGV[2][2];
		   ump_x2_2 += tmp_x2_3 * EIGV[2][3];		     
		   ump_x2_2 *= invfreq[2];
		   ump_x2_2 += x2->a;	  
		   

		   ump_x2_3 = tmp_x2_1 * EIGV[3][1] ;
		   ump_x2_3 += tmp_x2_2 * EIGV[3][2];
		   ump_x2_3 += tmp_x2_3 * EIGV[3][3];		     
		   ump_x2_3 *= invfreq[3];
		   ump_x2_3 += x2->a;	    	  		   	  
		   eigv = &EIGV[0][0];

		   x1px2 = ump_x1_0 * ump_x2_0;
		   x3->a = x1px2 *  *eigv++;/*EIGV[0][0];*/
		   x3->c = x1px2 *  *eigv++;/*EIGV[0][1];*/
		   x3->g = x1px2 *  *eigv++;/*EIGV[0][2];*/
		   x3->t = x1px2 *  *eigv++;/*EIGV[0][3]; */

		   x1px2 = ump_x1_1 * ump_x2_1;
		   x3->a += x1px2  *  *eigv++;/*EIGV[1][0];*/
		   x3->c += x1px2 *  *eigv++;/*EIGV[1][1];*/
		   x3->g += x1px2 *  *eigv++;/*EIGV[1][2];*/
		   x3->t += x1px2 *  *eigv++;/*EIGV[1][3];*/
	   
		   x1px2 = ump_x1_2 * ump_x2_2;
		   x3->a += x1px2 *  *eigv++;/*EIGV[2][0];*/
		   x3->c += x1px2*  *eigv++;/*EIGV[2][1];*/
		   x3->g += x1px2 *  *eigv++;/*EIGV[2][2];*/
		   x3->t += x1px2 *  *eigv++;/*EIGV[2][3];*/

		   x1px2 = ump_x1_3 * ump_x2_3;
		   x3->a += x1px2 *   *eigv++;/*EIGV[3][0];*/
		   x3->c += x1px2 *   *eigv++;/*EIGV[3][1];*/
		   x3->g += x1px2 *   *eigv++;/*EIGV[3][2];*/
		   x3->t += x1px2 *   *eigv++;/*EIGV[3][3];*/
		   
		   x3->exp = mexp + x2->exp;
		   x2++;
		}
	      	    
	      if (ABS(x3->a) < minlikelihood && ABS(x3->g) < minlikelihood
		  && ABS(x3->c) < minlikelihood && ABS(x3->t) < minlikelihood) 
		{	     
		  x3->a   *= twotothe256;
		  x3->g   *= twotothe256;
		  x3->c   *= twotothe256;
		  x3->t   *= twotothe256;
		  x3->exp += 1;
		}


 	      
	      *eqptr++ = -1;
	      x3++;
	    incTip:	 ;
	    }    	  	 
	}
    return TRUE;
  }
}





double evaluateGTR (tr, p)
    tree    *tr;
    nodeptr  p;
  { /* evaluate */
    double   sum, z, lz, term;    
    nodeptr  q;
    int     i, j;
    int     *wptr = tr->cdta->aliaswgt;
    
    double  diagptable[4];
    double *EIGN = &(tr->rdta->EIGN[0][0]);
    likelivector   *x1, *x2;
    homType *mlvP, *mlvQ;
    nodeptr realQ, realP;
    register  int *sQ; 
    register  int *sP;
    long refQ, refP;    
    q = p->back;
    while ((! p->x) || (! q->x)) {
      if (! (p->x)) if (! newviewGTR(tr, p)) return badEval;
      if (! (q->x)) if (! newviewGTR(tr, q)) return badEval;
    }


    x1  = p->x->lv;
    x2  = q->x->lv;
    z = p->z;
   
    if (z < zmin) z = zmin;
    lz = log(z);

    diagptable[0] = 1;
    diagptable[1] = exp (EIGN[1] * lz);
    diagptable[2] = exp (EIGN[2] * lz);
    diagptable[3] = exp (EIGN[3] * lz);
    
    
    
    sum = 0.0;
    realQ =  tr->nodep[q->number];
    realP =  tr->nodep[p->number];
    mlvP = realP->mlv;
    mlvQ = realQ->mlv;
    sQ = realQ->equalityVector;
    sP = realP->equalityVector;

   

    for (i = 0; i < tr->cdta->endsite; i++) {
	    refQ = *sQ++;
	    refP = *sP++;

	    

	    if(refP >= 0)
	      {
		if(refQ >= 0)
		  {
		    mlvP = &realP->mlv[refP];
		    mlvQ = &realQ->mlv[refQ];
		    
		    term =  mlvP->a * mlvQ->a * diagptable[0];
		    term += mlvP->c * mlvQ->c * diagptable[1];
		    term += mlvP->g * mlvQ->g * diagptable[2];
		    term += mlvP->t * mlvQ->t * diagptable[3];     
		    term = log(term) + (mlvQ->exp + mlvP->exp)*log(minlikelihood);
		    sum += *wptr++ * term;
		  }
		else
		  {
		    mlvP = &realP->mlv[refP];
		    term =  mlvP->a * x2->a * diagptable[0];
		    term += mlvP->c * x2->c * diagptable[1];
		    term += mlvP->g * x2->g * diagptable[2];
		    term += mlvP->t * x2->t * diagptable[3];     
		    term = log(term) + (mlvP->exp + x2->exp)*log(minlikelihood);
		   
		    sum += *wptr++ * term;      		    		   
		    x2++;
		  }
	      }
	    else
	      {
		if(refQ >= 0)
		  {
		    mlvQ = &realQ->mlv[refQ];
		    term =  x1->a * mlvQ->a * diagptable[0];
		    term += x1->c * mlvQ->c * diagptable[1];
		    term += x1->g * mlvQ->g * diagptable[2];
		    term += x1->t * mlvQ->t * diagptable[3];     
		    term = log(term) + (x1->exp + mlvQ->exp)*log(minlikelihood); 
		    sum += *wptr++ * term;		   
		    x1++;
		  }
		else
		  {
		    term =  x1->a * x2->a * diagptable[0];
		    term += x1->c * x2->c * diagptable[1];
		    term += x1->g * x2->g * diagptable[2];
		    term += x1->t * x2->t * diagptable[3];     
		    term = log(term) + (x1->exp + x2->exp)*log(minlikelihood);
		    sum += *wptr++ * term;		  
		    x1++;
		    x2++;
		  }
	      }	    
    }
    
    tr->likelihood = sum;
    return  sum;
  } /* evaluate */


double makenewzGTR (tr, p, q, z0, maxiter)
    tree    *tr;
    nodeptr  p, q;
    double   z0;
    int  maxiter;
  { /* makenewz */
    double   z, zprev, zstep;
    likelivector  *x1, *x2;
    double  *sumtable, *sum;
    int     i;
    double  dlnLdlz = 0.0;                 /*  = d(ln(likelihood))/d(lz) */
    double  d2lnLdlz2 = 0.0;               /*  = d2(ln(likelihood))/d(lz)2 */
    homType *mlvP, *mlvQ;
    nodeptr realQ, realP;
    register  int *sQ; 
    register  int *sP;
    long refQ, refP;
    double d0, d1, d2, d3;
    double e0, e1, e2, e3;

    while ((! p->x) || (! q->x)) {
      if (! (p->x)) if (! newviewGTR(tr, p)) return badZ;
      if (! (q->x)) if (! newviewGTR(tr, q)) return badZ;
    }

    x1 = p->x->lv;
    x2 = q->x->lv;

    sum = sumtable = (double *)malloc(4 * tr->cdta->endsite * 
				      sizeof(double));     
    realQ =  tr->nodep[q->number];
    realP =  tr->nodep[p->number];
    mlvP = realP->mlv;
    mlvQ = realQ->mlv;
    sQ = realQ->equalityVector;
    sP = realP->equalityVector;

    for (i = 0; i < tr->cdta->endsite; i++) 
      {     
	refQ = *sQ++;
	refP = *sP++;


	if(refP >= 0)
	      {
		if(refQ >= 0)
		  {
		    mlvP = &realP->mlv[refP];
		    mlvQ = &realQ->mlv[refQ];
		    *sum++ = mlvP->a * mlvQ->a;
		    *sum++ = mlvP->c * mlvQ->c;
		    *sum++ = mlvP->g * mlvQ->g;
		    *sum++ = mlvP->t * mlvQ->t;
		  }
		else
		  {
		    mlvP = &realP->mlv[refP];
		    *sum++ = mlvP->a * x2->a;
		    *sum++ = mlvP->c * x2->c;
		    *sum++ = mlvP->g * x2->g;
		    *sum++ = mlvP->t * x2->t;
    		    		   
		    x2++;
		  }
	      }
	    else
	      {
		if(refQ >= 0)
		  {
		    mlvQ = &realQ->mlv[refQ];
		    *sum++ = x1->a * mlvQ->a;
		    *sum++ = x1->c * mlvQ->c;
		    *sum++ = x1->g * mlvQ->g;
		    *sum++ = x1->t * mlvQ->t;

		    x1++;
		  }
		else
		  {
		    *sum++ = x1->a * x2->a;
		    *sum++ = x1->c * x2->c;
		    *sum++ = x1->g * x2->g;
		    *sum++ = x1->t * x2->t;
		    
		    x1++;
		    x2++;
		  }
	      }

      }

   
   

    z = z0;
    do {
      int curvatOK = FALSE;

      zprev = z;
      
      zstep = (1.0 - zmax) * z + zmin;

      do {
	double  diagptable[4];
	double *EIGN    = &(tr->rdta->EIGN[0][0]);
	double lz;
        double *wrptr   = &(tr->cdta->wr[0]);
        double *wr2ptr  = &(tr->cdta->wr2[0]);

        dlnLdlz = 0.0;                 /*  = d(ln(likelihood))/d(lz) */
        d2lnLdlz2 = 0.0;               /*  = d2(ln(likelihood))/d(lz)2 */

        if (z < zmin) z = zmin;
        else if (z > zmax) z = zmax;
        lz    = log(z);
        
	d1 = exp (EIGN[1] * lz); 
	d2 = exp (EIGN[2] * lz); 
	d3 = exp (EIGN[3] * lz); 
	
	sum = sumtable;
        for (i = 0; i < tr->cdta->endsite; i++) 
	  {
	    double tmp_0, tmp_1, tmp_2, tmp_3;
	    double t, inv_Li, dlnLidlz = 0, d2lnLidlz2 = 0;
	    	 
	    inv_Li = (tmp_0 = *sum++);
	    inv_Li += (tmp_1 = *sum++ * d1);
	    inv_Li += (tmp_2 = *sum++ * d2);
	    inv_Li += (tmp_3 = *sum++ * d3);

	    inv_Li = 1.0/inv_Li;
	    
	  
	    t = tmp_1 * EIGN[1];
	    dlnLidlz   += t;
	    d2lnLidlz2 += t * EIGN[1];
	    
	    t = tmp_2 * EIGN[2];
	    dlnLidlz   += t;
	    d2lnLidlz2 += t * EIGN[2];
	    
	    t = tmp_3 * EIGN[3];
	    dlnLidlz   += t;
	    d2lnLidlz2 += t * EIGN[3];
	    
	    dlnLidlz   *= inv_Li;
	    d2lnLidlz2 *= inv_Li;
	    dlnLdlz   += *wrptr++  * dlnLidlz;
	    d2lnLdlz2 += *wr2ptr++ * (d2lnLidlz2 - dlnLidlz * dlnLidlz);
	  }
	
        if ((d2lnLdlz2 >= 0.0) && (z < zmax))
          zprev = z = 0.37 * z + 0.63;  /*  Bad curvature, shorten branch */
        else
          curvatOK = TRUE;
	
      } while (! curvatOK);

      if (d2lnLdlz2 < 0.0) {
	double tantmp = -dlnLdlz / d2lnLdlz2;  /* prevent overflow */
	if (tantmp < 100) {
	  z *= exp(tantmp);
	  if (z < zmin) z = zmin;
	  if (z > 0.25 * zprev + 0.75)    /*  Limit steps toward z = 1.0 */
	    z = 0.25 * zprev + 0.75;
	} else {
	  z = 0.25 * zprev + 0.75;
	}
      }
      if (z > zmax) z = zmax;

    } while ((--maxiter > 0) && (ABS(z - zprev) > zstep));

    free (sumtable);

    return  z;
  } /* makenewz */




boolean newviewHKY85CAT (tree *tr, nodeptr p)        /*  Update likelihoods at node */
  { /* newview */
    double   zq, lzq, xvlzq, zr, lzr, xvlzr, tip_t;
    nodeptr  q, r;
    likelivector *lp, *lq, *lr;
    int  i;

    

    if (p->tip) {             /*  Make sure that data are at tip */      
      likelivector *l;
      int           code;
      yType        *yptr;
      
      if (p->x) 
	{	 
	  return TRUE;  
	}

      if (! getxtip(p)) return FALSE; /*  They are not, so get memory */
      l = p->x->lv;           /*  Pointer to first likelihood vector value */
      yptr = p->tip;          /*  Pointer to first nucleotide datum */
      for (i = 0; i < tr->cdta->endsite; i++) {
        code = *yptr++;
        l->a =  code       & 1;
        l->c = (code >> 1) & 1;
        l->g = (code >> 2) & 1;
        l->t = (code >> 3) & 1;
        l->exp = 0;

	
        l++;
        }
    
      return TRUE;
      }

/*  Internal node needs update */

    q = p->next->back;
    r = p->next->next->back;

    

    while ((! p->x) || (! q->x) || (! r->x)) {
      if (! q->x) if (! newviewHKY85CAT(tr, q))  return FALSE;
      if (! r->x) if (! newviewHKY85CAT(tr, r))  return FALSE;
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

    { double  *zzqtable, *zvqtable,
              *zzrtable, *zvrtable,
             *zzqptr, *zvqptr, *zzrptr, *zvrptr, *rptr;
      double  fxqr, fxqy, fxqn, sumaq, sumgq, sumcq, sumtq,
              fxrr, fxry, fxrn, ki, tempi, tempj;
      int  *cptr;
      double zzq, zvq, zzr, zvr;
      int cat;

      zzqtable = (double *)malloc(tr->NumberOfCategories * sizeof(double));
      zvqtable = (double *)malloc(tr->NumberOfCategories * sizeof(double));
      zzrtable = (double *)malloc(tr->NumberOfCategories * sizeof(double));
      zvrtable = (double *)malloc(tr->NumberOfCategories * sizeof(double));
     
    
      rptr   = &(tr->cdta->patrat[0]);
      zzqptr = &(zzqtable[0]);
      zvqptr = &(zvqtable[0]);
      zzrptr = &(zzrtable[0]);
      zvrptr = &(zvrtable[0]);
      cptr  = &(tr->cdta->rateCategory[0]);

     

      for (i = 0; i < tr->NumberOfCategories; i++) {   
        ki = *rptr++;	
	
        *zzqptr++ = exp(ki *   lzq);
        *zvqptr++ = exp(ki * xvlzq);
        *zzrptr++ = exp(ki *   lzr);
        *zvrptr++ = exp(ki * xvlzr);
        }

    
     

        for (i = 0; i < tr->cdta->endsite; i++) {
          cat = *cptr++;
	  
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
          lp->exp = lq->exp + lr->exp;
	 
	  

          if (lp->a < minlikelihood && lp->g < minlikelihood
           && lp->c < minlikelihood && lp->t < minlikelihood) {
            lp->a   *= twotothe256;
            lp->g   *= twotothe256;
            lp->c   *= twotothe256;
            lp->t   *= twotothe256;
            lp->exp += 1;
            }
          lp++;
          lq++;
          lr++;
          }

	free(zzqtable);
	free(zvqtable);
	free(zzrtable);
	free(zvrtable);


      return TRUE;
      }
  } /* newview */





double evaluateHKY85CAT (tree *tr, nodeptr p)
  { /* evaluate */
    double   sum, z, lz, xvlz,
             ki, fxpa, fxpc, fxpg, fxpt, fxpr, fxpy, fxqr, fxqy,
             suma, sumb, sumc, term;


       double   zz, zv;

    double        *zztable, *zvtable,
                 *zzptr, *zvptr;
    double        *rptr;
    likelivector *lp, *lq;
    nodeptr       q;
    int           cat, *cptr, i, *wptr;

    q = p->back;
    while ((! p->x) || (! q->x)) {
      if (! (p->x)) if (! newviewHKY85CAT(tr, p)) return badEval;
      if (! (q->x)) if (! newviewHKY85CAT(tr, q)) return badEval;
      }

    lp = p->x->lv;
    lq = q->x->lv;

    z = p->z;
    if (z < zmin) z = zmin;
    lz = log(z);
    xvlz = tr->rdta->xv * lz;

    zztable = (double *)malloc(tr->NumberOfCategories * sizeof(double));
    zvtable = (double *)malloc(tr->NumberOfCategories * sizeof(double));

    rptr   = tr->cdta->patrat;
    cptr  = tr->cdta->rateCategory; 
    zzptr = zztable;
    zvptr = zvtable;

    for (i = 0; i < tr->NumberOfCategories; i++) {
      ki = *rptr++;
      *zzptr++ = exp(ki *   lz);
      *zvptr++ = exp(ki * xvlz);
      }

    wptr = &(tr->cdta->aliaswgt[0]);
    
   
    sum = 0.0;


      for (i = 0; i < tr->cdta->endsite; i++) {
        cat  = *cptr++;
        zz   = zztable[cat];
        zv   = zvtable[cat];
        fxpa = tr->rdta->freqa * lp->a;
        fxpg = tr->rdta->freqg * lp->g;
        fxpc = tr->rdta->freqc * lp->c;
        fxpt = tr->rdta->freqt * lp->t;
        suma = fxpa * lq->a + fxpc * lq->c + fxpg * lq->g + fxpt * lq->t;
        fxqr = tr->rdta->freqa * lq->a + tr->rdta->freqg * lq->g;
        fxqy = tr->rdta->freqc * lq->c + tr->rdta->freqt * lq->t;
        fxpr = fxpa + fxpg;
        fxpy = fxpc + fxpt;
        sumc = (fxpr + fxpy) * (fxqr + fxqy);
        sumb = fxpr * fxqr * tr->rdta->invfreqr + fxpy * fxqy * tr->rdta->invfreqy;
        suma -= sumb;
        sumb -= sumc;
        term = log(zz * suma + zv * sumb + sumc) + (lp->exp + lq->exp)*log(minlikelihood);

        sum += *wptr++ * term;
        
        lp++;
        lq++;
        }

      free(zztable);
      free(zvtable); 


    tr->likelihood = sum;
    return  sum;
  } /* evaluate */



double makenewzHKY85CAT (tree *tr, nodeptr p, nodeptr q, double z0, int maxiter)
  { /* makenewz */
    likelivector *lp, *lq;
    double  *abi, *bci, *sumci, *abptr, *bcptr, *sumcptr;
    double   dlnLidlz, dlnLdlz, d2lnLdlz2, z, zprev, zstep, lz, xvlz,
             ki, suma, sumb, sumc, ab, bc, inv_Li, t1, t2,
             fx1a, fx1c, fx1g, fx1t, fx1r, fx1y, fx2r, fx2y;
    double   *zztable, *zvtable,
            *zzptr, *zvptr;
    double  *rptr, *wrptr, *wr2ptr;
    int      cat, *cptr, i, curvatOK;


    while ((! p->x) || (! q->x)) {
      if (! (p->x)) if (! newviewHKY85CAT(tr, p)) return badZ;
      if (! (q->x)) if (! newviewHKY85CAT(tr, q)) return badZ;
      }

    lp = p->x->lv;
    lq = q->x->lv;

    { 
      unsigned scratch_size;
      scratch_size = sizeof(double) * tr->cdta->endsite;
      abi   = (double *) malloc(scratch_size);
      bci   = (double *) malloc(scratch_size);
      sumci = (double *) malloc(scratch_size);      
    }
    
    zztable = (double *)malloc(tr->NumberOfCategories * sizeof(double));
    zvtable = (double *)malloc(tr->NumberOfCategories * sizeof(double));

    abptr   = abi;
    bcptr   = bci;
    sumcptr = sumci;
    


    for (i = 0; i < tr->cdta->endsite; i++) {
      fx1a = tr->rdta->freqa * lp->a;
      fx1g = tr->rdta->freqg * lp->g;
      fx1c = tr->rdta->freqc * lp->c;
      fx1t = tr->rdta->freqt * lp->t;
      suma = fx1a * lq->a + fx1c * lq->c + fx1g * lq->g + fx1t * lq->t;
      fx2r = tr->rdta->freqa * lq->a + tr->rdta->freqg * lq->g;
      fx2y = tr->rdta->freqc * lq->c + tr->rdta->freqt * lq->t;
      fx1r = fx1a + fx1g;
      fx1y = fx1c + fx1t;
      *sumcptr++ = sumc = (fx1r + fx1y) * (fx2r + fx2y);
      sumb       = fx1r * fx2r * tr->rdta->invfreqr + fx1y * fx2y * tr->rdta->invfreqy;
      *abptr++   = suma - sumb;
      *bcptr++   = sumb - sumc;
      lp++;
      lq++;
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
        rptr  = tr->cdta->patrat;
	cptr  = tr->cdta->rateCategory;
        zzptr = zztable;
        zvptr = zvtable;



        for (i = 0; i < tr->NumberOfCategories; i++) {
          ki = *rptr++;
          *zzptr++ = exp(ki *   lz);
          *zvptr++ = exp(ki * xvlz);
          }

        abptr   = abi;
        bcptr   = bci;
        sumcptr = sumci;        
        wrptr   = &(tr->cdta->wr[0]);
        wr2ptr  = &(tr->cdta->wr2[0]);
        dlnLdlz = 0.0;                 /*  = d(ln(likelihood))/d(lz) */
        d2lnLdlz2 = 0.0;               /*  = d2(ln(likelihood))/d(lz)2 */



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

    free(abi);
    free(bci);
    free(sumci);
    free(zztable);
    free(zvtable); 

/* printf("makenewz: %le\n", z); */

    return  z;
  } /* makenewz */

boolean newviewGTRCAT(tree    *tr, nodeptr  p)
{
  
  if (p->tip) 
    {      
      if (p->x) return TRUE;
      if (! getxtip(p)) return FALSE;  
      {
	int code, i, j, jj;
	double (*EIGV)[4] = &(tr->rdta->EIGV[0][0]);
	likelivector *x1 = p->x->lv;
     
	for(i = 0; i < tr->cdta->endsite; i++)
	  {	      	  	 
	    code = p->tip[i];
	    x1->a = 0;
	    x1->c = 0;
	    x1->g = 0;
	    x1->t = 0;
	    for (j = 0; j < 4; j++) 
	      {
		if ((code >> j) & 1) 
		  {
		    int jj = "0213"[j] - '0';		
		    x1->a += EIGV[jj][0];
		    x1->c += EIGV[jj][1];
		    x1->g += EIGV[jj][2];
		    x1->t += EIGV[jj][3];
		  }
	      }		
	      
	    x1->exp = 0;	  			  			 
	    x1++;	 
	} 
      return TRUE; 
      }
    }
  
  { 	
    double  *diagptable_x1, *diagptable_x2, *diagptable_x1_start, *diagptable_x2_start;
    double *EIGN    = &(tr->rdta->EIGN[0][0]);
    double *invfreq = &(tr->rdta->invfreq[0][0]);
    double (*EIGV)[4] = &(tr->rdta->EIGV[0][0]);
    xtype tmp_x1_1, tmp_x1_2, tmp_x1_3,
      tmp_x2_1, tmp_x2_2, tmp_x2_3, 
      ump_x1_1, ump_x1_2, ump_x1_3, ump_x1_0, 
      ump_x2_0, ump_x2_1, ump_x2_2, ump_x2_3, x1px2;
    int cat;
    int  *cptr;	  
    int  i, j;
    likelivector   *x1, *x2, *x3;
    double   z1, lz1, z2, lz2, ki;
    nodeptr  q, r;   
    double *eigv, *rptr;

    rptr   = &(tr->cdta->patrat[0]); 
    cptr  = &(tr->cdta->rateCategory[0]);

    q = p->next->back;
    r = p->next->next->back;
      
    while ((! p->x) || (! q->x) || (! r->x)) {
      if (! q->x) if (! newviewGTRCAT(tr, q)) return FALSE;
      if (! r->x) if (! newviewGTRCAT(tr, r)) return FALSE;
      if (! p->x) if (! getxnode(p)) return FALSE;	
    }

    x1  = q->x->lv;
    z1  = q->z;
    lz1 = (z1 > zmin) ? log(z1) : log(zmin);

    x2  = r->x->lv;
    z2  = r->z;
    lz2 = (z2 > zmin) ? log(z2) : log(zmin);

    x3  = p->x->lv;   
   
    
    diagptable_x1 = diagptable_x1_start = (double *)malloc(sizeof(double) * tr->NumberOfCategories * 3);
    diagptable_x2 = diagptable_x2_start = (double *)malloc(sizeof(double) * tr->NumberOfCategories * 3);
    
    
    for(i = 0; i < tr->NumberOfCategories;i++)
      {
	ki = *rptr++;
	
	*diagptable_x1++ = exp (EIGN[1] * ki * lz1);
	*diagptable_x1++ = exp (EIGN[2] * ki * lz1);
	*diagptable_x1++ = exp (EIGN[3] * ki * lz1);
	*diagptable_x2++ = exp (EIGN[1] * ki * lz2);
	*diagptable_x2++ = exp (EIGN[2] * ki * lz2);
	*diagptable_x2++ = exp (EIGN[3] * ki * lz2);		
      }

    
    for (i = 0; i < tr->cdta->endsite; i++) 
      {	
	cat = *cptr++;

	diagptable_x1 = &diagptable_x1_start[cat * 3];
	diagptable_x2 = &diagptable_x2_start[cat * 3];

	tmp_x1_1 = x1->c * *diagptable_x1++;
	tmp_x1_2 = x1->g * *diagptable_x1++;
	tmp_x1_3 = x1->t * *diagptable_x1++;
	
	ump_x1_0 = tmp_x1_1 * EIGV[0][1];
	ump_x1_0 += tmp_x1_2 * EIGV[0][2];
	ump_x1_0 += tmp_x1_3 * EIGV[0][3];	      	
	ump_x1_0 *= invfreq[0];
	ump_x1_0 += x1->a;
	
	ump_x1_1 = tmp_x1_1 * EIGV[1][1];
	ump_x1_1 += tmp_x1_2 * EIGV[1][2];
	ump_x1_1 += tmp_x1_3 * EIGV[1][3];	      	
	ump_x1_1 *= invfreq[1];
	ump_x1_1 += x1->a;
	    
	ump_x1_2 = tmp_x1_1 * EIGV[2][1];
	ump_x1_2 += tmp_x1_2 * EIGV[2][2];
	ump_x1_2 += tmp_x1_3 * EIGV[2][3];	      	
	ump_x1_2 *= invfreq[2];
	ump_x1_2 += x1->a;

	ump_x1_3 = tmp_x1_1 * EIGV[3][1];
	ump_x1_3 += tmp_x1_2 * EIGV[3][2];
	ump_x1_3 += tmp_x1_3 * EIGV[3][3];	      	
	ump_x1_3 *= invfreq[3];
	ump_x1_3 += x1->a;
	
	tmp_x2_1 = x2->c * *diagptable_x2++;
	tmp_x2_2 = x2->g * *diagptable_x2++;
	tmp_x2_3 = x2->t * *diagptable_x2++;
	 
	 	    	    	     	     
	ump_x2_0 = tmp_x2_1 * EIGV[0][1];
	ump_x2_0 += tmp_x2_2 * EIGV[0][2];
	ump_x2_0 += tmp_x2_3 * EIGV[0][3];		     
	ump_x2_0 *= invfreq[0];
	ump_x2_0 += x2->a;
	  
	ump_x2_1 = tmp_x2_1 * EIGV[1][1];
	ump_x2_1 += tmp_x2_2 * EIGV[1][2];
	ump_x2_1 += tmp_x2_3 * EIGV[1][3];		     
	ump_x2_1 *= invfreq[1];
	ump_x2_1 += x2->a;	 

	ump_x2_2 = tmp_x2_1 * EIGV[2][1];
	ump_x2_2 += tmp_x2_2 * EIGV[2][2];
	ump_x2_2 += tmp_x2_3 * EIGV[2][3];		     
	ump_x2_2 *= invfreq[2];
	ump_x2_2 += x2->a;	  
		   

	ump_x2_3 = tmp_x2_1 * EIGV[3][1] ;
	ump_x2_3 += tmp_x2_2 * EIGV[3][2];
	ump_x2_3 += tmp_x2_3 * EIGV[3][3];		     
	ump_x2_3 *= invfreq[3];
	ump_x2_3 += x2->a;	    	  		   	  
	eigv = &EIGV[0][0];
	
	x1px2 = ump_x1_0 * ump_x2_0;
	x3->a = x1px2 *  *eigv++;/*EIGV[0][0];*/
	x3->c = x1px2 *  *eigv++;/*EIGV[0][1];*/
	x3->g = x1px2 *  *eigv++;/*EIGV[0][2];*/
	x3->t = x1px2 *  *eigv++;/*EIGV[0][3]; */
	
	x1px2 = ump_x1_1 * ump_x2_1;
	x3->a += x1px2  *  *eigv++;/*EIGV[1][0];*/
	x3->c += x1px2 *  *eigv++;/*EIGV[1][1];*/
	x3->g += x1px2 *  *eigv++;/*EIGV[1][2];*/
	x3->t += x1px2 *  *eigv++;/*EIGV[1][3];*/
	
	x1px2 = ump_x1_2 * ump_x2_2;
	x3->a += x1px2 *  *eigv++;/*EIGV[2][0];*/
	x3->c += x1px2*  *eigv++;/*EIGV[2][1];*/
	x3->g += x1px2 *  *eigv++;/*EIGV[2][2];*/
	x3->t += x1px2 *  *eigv++;/*EIGV[2][3];*/
	
	x1px2 = ump_x1_3 * ump_x2_3;
	x3->a += x1px2 *   *eigv++;/*EIGV[3][0];*/
	x3->c += x1px2 *   *eigv++;/*EIGV[3][1];*/
	x3->g += x1px2 *   *eigv++;/*EIGV[3][2];*/
	x3->t += x1px2 *   *eigv++;/*EIGV[3][3];*/
		   
	x3->exp = x1->exp + x2->exp;		  
	
	if (ABS(x3->a) < minlikelihood && ABS(x3->g) < minlikelihood
	    && ABS(x3->c) < minlikelihood && ABS(x3->t) < minlikelihood) 
	  {	     
	    x3->a   *= twotothe256;
	    x3->g   *= twotothe256;
	    x3->c   *= twotothe256;
	    x3->t   *= twotothe256;
	    x3->exp += 1;
	  }
	 	
	x1++;
	x2++;
	x3++;
      
      }
         
    free(diagptable_x1_start); 
    free(diagptable_x2_start);
    return TRUE;
  }
}





double evaluateGTRCAT (tr, p)
    tree    *tr;
    nodeptr  p;
  { /* evaluate */
    double   sum, z, lz, term, ki;    
    nodeptr  q;
    int     i, j, cat;
    int     *wptr = tr->cdta->aliaswgt, *cptr;
    
    double  *diagptable, *rptr, *diagptable_start;
    double *EIGN = &(tr->rdta->EIGN[0][0]);
    likelivector   *x1, *x2;
       
    q = p->back;
    while ((! p->x) || (! q->x)) {
      if (! (p->x)) if (! newviewGTRCAT(tr, p)) return badEval;
      if (! (q->x)) if (! newviewGTRCAT(tr, q)) return badEval;
    }


    rptr   = &(tr->cdta->patrat[0]); 
    cptr  = &(tr->cdta->rateCategory[0]);

    x1  = p->x->lv;
    x2  = q->x->lv;
    z = p->z;
   
    if (z < zmin) z = zmin;
    lz = log(z);

   
    
    diagptable = diagptable_start = (double *)malloc(sizeof(double) * tr->NumberOfCategories * 3);
    for(i = 0; i <  tr->NumberOfCategories;i++)
      {
	ki = *rptr++;	 
	*diagptable++ = exp (EIGN[1] * ki * lz);
	*diagptable++ = exp (EIGN[2] * ki * lz);
	*diagptable++ = exp (EIGN[3] * ki * lz);
      }
	
    
    sum = 0.0;
  
    for (i = 0; i < tr->cdta->endsite; i++) 
      {	  	 	  
	cat = *cptr++;
	diagptable = &diagptable_start[3 * cat];

	term =  x1->a * x2->a;
	term += x1->c * x2->c * *diagptable++;
	term += x1->g * x2->g * *diagptable++;
	term += x1->t * x2->t * *diagptable++;     
	term = log(term) + (x1->exp + x2->exp)*log(minlikelihood);
	sum += *wptr++ * term;		  
	x1++;
	x2++;
      }
    
      
    free(diagptable_start); 
    
    tr->likelihood = sum;
    return  sum;
  } /* evaluate */


double makenewzGTRCAT (tr, p, q, z0, maxiter)
    tree    *tr;
    nodeptr  p, q;
    double   z0;
    int  maxiter;
  { /* makenewz */
    double   z, zprev, zstep;
    likelivector  *x1, *x2;
    double  *sumtable, *sum;
    int     i, *cptr, cat;
    double  dlnLdlz = 0.0;                 /*  = d(ln(likelihood))/d(lz) */
    double  d2lnLdlz2 = 0.0;               /*  = d2(ln(likelihood))/d(lz)2 */    
    double *d1, *d2, *d3, *rptr;
    double ki;
    
    while ((! p->x) || (! q->x)) {
      if (! (p->x)) if (! newviewGTRCAT(tr, p)) return badZ;
      if (! (q->x)) if (! newviewGTRCAT(tr, q)) return badZ;
    }

    x1 = p->x->lv;
    x2 = q->x->lv;

    sum = sumtable = (double *)malloc(4 * tr->cdta->endsite * 
				      sizeof(double));
    d1 = (double *)malloc(tr->NumberOfCategories * sizeof(double));
    d2 = (double *)malloc(tr->NumberOfCategories * sizeof(double));
    d3 = (double *)malloc(tr->NumberOfCategories * sizeof(double));
 

    for (i = 0; i < tr->cdta->endsite; i++) 
      {     
	*sum++ = x1->a * x2->a;
	*sum++ = x1->c * x2->c;
	*sum++ = x1->g * x2->g;
	*sum++ = x1->t * x2->t;		    
	x1++;
	x2++;		 
      }

   
   
    z = z0;
    do {
      int curvatOK = FALSE;

      zprev = z;
      
      zstep = (1.0 - zmax) * z + zmin;

      do {	
	double *EIGN    = &(tr->rdta->EIGN[0][0]);
	double lz;
        double *wrptr   = &(tr->cdta->wr[0]);
        double *wr2ptr  = &(tr->cdta->wr2[0]);

        dlnLdlz = 0.0;                 /*  = d(ln(likelihood))/d(lz) */
        d2lnLdlz2 = 0.0;               /*  = d2(ln(likelihood))/d(lz)2 */

        if (z < zmin) z = zmin;
        else if (z > zmax) z = zmax;
        lz    = log(z);
        
	rptr   = &(tr->cdta->patrat[0]); 
	cptr  = &(tr->cdta->rateCategory[0]);
	sum = sumtable;
	
	for(i = 0; i < tr->NumberOfCategories; i++)
	  {
	    ki = *rptr++;
	    d1[i] = exp (EIGN[1] * lz * ki); 
	    d2[i] = exp (EIGN[2] * lz * ki); 
	    d3[i] = exp (EIGN[3] * lz * ki); 
	  }


        for (i = 0; i < tr->cdta->endsite; i++) 
	  {
	    double tmp_0, tmp_1, tmp_2, tmp_3;
	    double t, inv_Li, dlnLidlz = 0, d2lnLidlz2 = 0;
	    	
	    cat = *cptr++;
	    inv_Li = (tmp_0 = *sum++);
	    inv_Li += (tmp_1 = *sum++ * d1[cat]);
	    inv_Li += (tmp_2 = *sum++ * d2[cat]);
	    inv_Li += (tmp_3 = *sum++ * d3[cat]);

	    inv_Li = 1.0/inv_Li;
	    
	  
	    t = tmp_1 * EIGN[1];
	    dlnLidlz   += t;
	    d2lnLidlz2 += t * EIGN[1];
	    
	    t = tmp_2 * EIGN[2];
	    dlnLidlz   += t;
	    d2lnLidlz2 += t * EIGN[2];
	    
	    t = tmp_3 * EIGN[3];
	    dlnLidlz   += t;
	    d2lnLidlz2 += t * EIGN[3];
	    
	    dlnLidlz   *= inv_Li;
	    d2lnLidlz2 *= inv_Li;
	    dlnLdlz   += *wrptr++  * dlnLidlz;
	    d2lnLdlz2 += *wr2ptr++ * (d2lnLidlz2 - dlnLidlz * dlnLidlz);
	  }
	
        if ((d2lnLdlz2 >= 0.0) && (z < zmax))
          zprev = z = 0.37 * z + 0.63;  /*  Bad curvature, shorten branch */
        else
          curvatOK = TRUE;
	
      } while (! curvatOK);

      if (d2lnLdlz2 < 0.0) {
	double tantmp = -dlnLdlz / d2lnLdlz2;  /* prevent overflow */
	if (tantmp < 100) {
	  z *= exp(tantmp);
	  if (z < zmin) z = zmin;
	  if (z > 0.25 * zprev + 0.75)    /*  Limit steps toward z = 1.0 */
	    z = 0.25 * zprev + 0.75;
	} else {
	  z = 0.25 * zprev + 0.75;
	}
      }
      if (z > zmax) z = zmax;

    } while ((--maxiter > 0) && (ABS(z - zprev) > zstep));

    free (sumtable);
    free(d1);
    free(d2);
    free(d3);
    return  z;
  } /* makenewz */




#endif



#ifdef MKL

boolean newviewHKY85(tree *tr, nodeptr p)        /*  Update likelihoods at node */
  { /* newview */
    double   zq, lzq, xvlzq, zr, lzr, xvlzr;
    nodeptr  q, r, realP;
    likelivector *lp, *lq, *lr;
    homType *mlvP;
    int  i;
           
    if(p->tip) 
      {
	if (p->x) return TRUE;

	if (! getxtip(p)) return FALSE;

	return TRUE;
      }
      
    q = p->next->back;
    r = p->next->next->back;

    while ((! p->x) || (! q->x) || (! r->x)) 
      {
	if (! q->x) if (! newviewHKY85(tr, q))  return FALSE;
	if (! r->x) if (! newviewHKY85(tr, r))  return FALSE;
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
      double  fxqr, fxqy, fxqn, sumaq, sumgq, sumcq, sumtq,
              fxrr, fxry, fxrn, ki, tempi, tempj;                
      register  int *s1; 
      register  int *s2;
      register int *eqptr;
      
      homType *mlvR, *mlvQ;
      poutsa calc[EQUALITIES],calcR[EQUALITIES], *calcptr, *calcptrR;
      long ref, ref2, mexp;      
      nodeptr realQ, realR;
    
      /* get equality vectors of p, q, and r */
      
      realP =  tr->nodep[p->number];
    
      mlvP = realP->mlv;
      realQ =  tr->nodep[q->number];
      realR =  tr->nodep[r->number];


      eqptr = realP->equalityVector; 
      s1 = realQ->equalityVector;
      s2 = realR->equalityVector; 
                 
      zzq = exp(lzq);
      zvq = exp(xvlzq);
      zzr = exp(lzr);        
      zvr = exp(xvlzr); 
      
      mlvQ = realQ->mlv;
      mlvR = realR->mlv;


     
           	 	  
      for(i = 0; i < equalities; i++)
	{
	  
	  if(mlvQ->set)
	    {
	      calcptr = &calc[i];		 
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
	  if(mlvR->set)
	    {
	      calcptrR = &calcR[i];		 
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
	  
	  mlvQ++;
	  mlvR++;
	  mlvP++;
	}


      if(r->tip && q->tip)
	{

	 

	   for (i = 0; i < tr->cdta->endsite; i++) 
	    {
	      ref = *s2++;
	      ref2 = *s1++;
	     	      	      		  	
	      if(ref2 != ref)
		{
		  *eqptr++ = -1;
		  calcptrR = &calcR[ref];
		  calcptr = &calc[ref2];
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
		  lp++;   
		}
	      else
		{
		  *eqptr++ = ref;		    		
		}
	   	      	    
	    }          
	}
      else
	{
	  	  

	 
	  
	  for (i = 0; i < tr->cdta->endsite; i++) 
	    {
	      ref = *s2++;
	      ref2 = *s1++;
	      
	      if(ref2 >= 0)
		{		  	
		  if(ref2 == ref)
		    {
		      *eqptr++ = ref;		     		  
		      goto incTip;		    		  		 		  	    		
		    }
		  calcptr = &calc[ref2];
		  sumaq = calcptr->sumaq;
		  sumgq = calcptr->sumgq;
		  sumcq = calcptr->sumcq;
		  sumtq = calcptr->sumtq;		 		  	
		  mexp = calcptr->exp;	       		  
		}	    	  	  	 	 
	      else
		{		
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
		  lq++;
		}
	      	      
	      if(ref >= 0)
		{
		  calcptr = &calcR[ref];		 	    
		  lp->a = sumaq * calcptr->sumaq;
		  lp->g = sumgq * calcptr->sumgq;		    
		  lp->c = sumcq * calcptr->sumcq; 
		  lp->t = sumtq * calcptr->sumtq;
		  lp->exp = mexp + calcptr->exp;		  	  
		}
	      else
		{
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
		  lr++;
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
	      lp++;
	    incTip:	 ;
	    }      
	
    
	}
    }
    
   
    

    return TRUE;  
  } /* newview */

 /* AxML modification end*/





double evaluateHKY85(tree *tr, nodeptr p)
  { /* evaluate */
    double   sum, z, lz, xvlz,
             ki, fx1a, fx1c, fx1g, fx1t, fx1r, fx1y, fx2r, fx2y,
             suma, sumb, sumc, term, zzp, zvp;           
    likelivector *lp, *lq;
    homType *mlvP, *mlvQ;
    nodeptr       q, realQ, realP;
    int           i, *wptr;
    register  int *sQ; 
    register  int *sP;
    long refQ, refP;
    memP calcP[EQUALITIES], *calcptrP;
    memQ calcQ[EQUALITIES], *calcptrQ;

    q = p->back;

    while ((! p->x) || (! q->x)) {
      if (! (p->x)) if (! newviewHKY85(tr, p)) return badEval;
      if (! (q->x)) if (! newviewHKY85(tr, q)) return badEval;
    }

    lp = p->x->lv;
    lq = q->x->lv;

    z = p->z;
    if (z < zmin) z = zmin;
    lz = log(z);
    xvlz = tr->rdta->xv * lz;

    
    zzp = exp(lz);
    zvp = exp(xvlz);
      
    wptr = &(tr->cdta->aliaswgt[0]);
   
    sum = 0.0;
    
    realQ =  tr->nodep[q->number];
    realP =  tr->nodep[p->number];
    mlvP = realP->mlv;
    mlvQ = realQ->mlv;
    sQ = realQ->equalityVector;
    sP = realP->equalityVector; 

    

    for(i = 0; i < equalities; i++)
	{
	 
	  if(mlvP->set)
	    {
	      calcptrP = &calcP[i];		 
	      calcptrP->fx1a = tr->rdta->freqa * mlvP->a;
	      calcptrP->fx1g = tr->rdta->freqg * mlvP->g;
	      calcptrP->fx1c = tr->rdta->freqc * mlvP->c;
	      calcptrP->fx1t = tr->rdta->freqt * mlvP->t;
	      calcptrP->sumag = calcptrP->fx1a + calcptrP->fx1g;
	      calcptrP->sumct = calcptrP->fx1c + calcptrP->fx1t;
	      calcptrP->sumagct = calcptrP->sumag + calcptrP->sumct;	      
	      calcptrP->exp = mlvP->exp;
	    }
	 
	  if(mlvQ->set)
	    {
	      calcptrQ = &calcQ[i];
	      calcptrQ->fx2r = tr->rdta->freqa * mlvQ->a + tr->rdta->freqg * mlvQ->g;	     
	      calcptrQ->fx2y  = tr->rdta->freqc * mlvQ->c + tr->rdta->freqt * mlvQ->t; 	     
	      calcptrQ->sumfx2rfx2y = calcptrQ->fx2r + calcptrQ->fx2y;	      
	      calcptrQ->exp = mlvQ->exp;
	      calcptrQ->a = mlvQ->a;
	      calcptrQ->c = mlvQ->c;
	      calcptrQ->g = mlvQ->g;
	      calcptrQ->t = mlvQ->t;
	    }
	  
	  mlvQ++;	 
	  mlvP++;
	}




      for (i = 0; i < tr->cdta->endsite; i++) {
	    refQ = *sQ++;
	    refP = *sP++;

	    if(refP >= 0)
	      {
		if(refQ >= 0)
		  {
		    calcptrP = &calcP[refP];
		    calcptrQ = &calcQ[refQ];
		    suma = calcptrP->fx1a * calcptrQ->a + 
		      calcptrP->fx1c * calcptrQ->c + 
		      calcptrP->fx1g * calcptrQ->g + calcptrP->fx1t * calcptrQ->t;		 		 
		    
		    sumc = calcptrP->sumagct * calcptrQ->sumfx2rfx2y;
		    sumb = calcptrP->sumag * calcptrQ->fx2r * tr->rdta->invfreqr + calcptrP->sumct * calcptrQ->fx2y * tr->rdta->invfreqy;
		    suma -= sumb;
		    sumb -= sumc;
		    term = log(zzp * suma + zvp * sumb + sumc) + 
		      (calcptrP->exp + calcptrQ->exp)*log(minlikelihood);
		    sum += *wptr++ * term;
		    
		   
		  }
		else
		  {
		    calcptrP = &calcP[refP];
		    suma = calcptrP->fx1a * lq->a + calcptrP->fx1c * lq->c + 
		      calcptrP->fx1g * lq->g + calcptrP->fx1t * lq->t;
		    
		    fx2r = tr->rdta->freqa * lq->a + tr->rdta->freqg * lq->g;
		    fx2y = tr->rdta->freqc * lq->c + tr->rdta->freqt * lq->t;
		    
		    sumc = calcptrP->sumagct * (fx2r + fx2y);
		    sumb       = calcptrP->sumag * fx2r * tr->rdta->invfreqr + calcptrP->sumct * fx2y * tr->rdta->invfreqy;
		    suma -= sumb;
		    sumb -= sumc;
		    term = log(zzp * suma + zvp * sumb + sumc) + 
		      (calcptrP->exp + lq->exp)*log(minlikelihood);
		    sum += *wptr++ * term;
		    
		   
		    lq++;
		  }
	      }
	    else
	      {
		if(refQ >= 0)
		  {
		    calcptrQ = &calcQ[refQ];
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
		    term = log(zzp * suma + zvp * sumb + sumc) + 
		      (lp->exp + calcptrQ->exp)*log(minlikelihood);		
		    sum += *wptr++ * term;	
		    
		    lp++;
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
		    term = log(zzp * suma + zvp * sumb + sumc) + 
		      (lp->exp + lq->exp)*log(minlikelihood);
		    sum += *wptr++ * term;		
		   
		    lp++;
		    lq++;
		  }
	      }	  
	   
      }

     
    tr->likelihood = sum; 
    return  sum;
   
  } /* evaluate */


double makenewzHKY85 (tree *tr, nodeptr p, nodeptr q, double z0, int maxiter)
{
  likelivector *lp, *lq;
    double  *abi, *bci, *sumci, *abptr, *bcptr, *sumcptr;
    double   dlnLidlz, dlnLdlz, d2lnLdlz2, z, zprev, zstep, lz, xvlz,
             ki, suma, sumb, sumc, ab, bc, inv_Li, t1, t2,
             fx1a, fx1c, fx1g, fx1t, fx1r, fx1y, fx2r, fx2y;
    double  zz, zv;
    double   *wrptr, *wr2ptr;
    int      i, curvatOK;
    homType *mlvP, *mlvQ;
    nodeptr realQ, realP;
    register  int *sQ; 
    register  int *sP;
    long refQ, refP;

    while ((! p->x) || (! q->x)) {
      if (! (p->x)) if (! newviewHKY85(tr, p)) return badZ;
      if (! (q->x)) if (! newviewHKY85(tr, q)) return badZ;
      }

    lp = p->x->lv;
    lq = q->x->lv;

    { 
      unsigned scratch_size;
      scratch_size = sizeof(double) * tr->cdta->endsite;
      abi   = (double *) malloc(scratch_size);
      bci   = (double *) malloc(scratch_size);
      sumci = (double *) malloc(scratch_size);      
    }
    
    
    

    abptr   = abi;
    bcptr   = bci;
    sumcptr = sumci;
    realQ =  tr->nodep[q->number];
    realP =  tr->nodep[p->number];
    mlvP = realP->mlv;
    mlvQ = realQ->mlv;
    sQ = realQ->equalityVector;
    sP = realP->equalityVector;


    for (i = 0; i < tr->cdta->endsite; i++) {
      refQ = *sQ++;
      refP = *sP++;
      if(refP >= 0)
	{
	  if(refQ >= 0)
	    {
	      mlvP = &realP->mlv[refP];
	      mlvQ = &realQ->mlv[refQ];
	      fx1a = tr->rdta->freqa * mlvP->a;
	      fx1g = tr->rdta->freqg * mlvP->g;
	      fx1c = tr->rdta->freqc * mlvP->c;
	      fx1t = tr->rdta->freqt * mlvP->t;
	      suma = fx1a * mlvQ->a + fx1c * mlvQ->c + fx1g * mlvQ->g + fx1t * mlvQ->t;
	      fx2r = tr->rdta->freqa * mlvQ->a + tr->rdta->freqg * mlvQ->g;
	      fx2y = tr->rdta->freqc * mlvQ->c + tr->rdta->freqt * mlvQ->t;
	      fx1r = fx1a + fx1g;
	      fx1y = fx1c + fx1t;
	      *sumcptr++ = sumc = (fx1r + fx1y) * (fx2r + fx2y);
	      sumb       = fx1r * fx2r * tr->rdta->invfreqr + fx1y * fx2y * tr->rdta->invfreqy;
	      *abptr++   = suma - sumb;
	      *bcptr++   = sumb - sumc;

		  
	    }
	  else
	    {
	      mlvP = &realP->mlv[refP];
	      fx1a = tr->rdta->freqa * mlvP->a;
	      fx1g = tr->rdta->freqg * mlvP->g;
	      fx1c = tr->rdta->freqc * mlvP->c;
	      fx1t = tr->rdta->freqt * mlvP->t;
	      suma = fx1a * lq->a + fx1c * lq->c + fx1g * lq->g + fx1t * lq->t;
	      fx2r = tr->rdta->freqa * lq->a + tr->rdta->freqg * lq->g;
	      fx2y = tr->rdta->freqc * lq->c + tr->rdta->freqt * lq->t;
	      fx1r = fx1a + fx1g;
	      fx1y = fx1c + fx1t;
	      *sumcptr++ = sumc = (fx1r + fx1y) * (fx2r + fx2y);
	      sumb       = fx1r * fx2r * tr->rdta->invfreqr + fx1y * fx2y * tr->rdta->invfreqy;
	      *abptr++   = suma - sumb;
	      *bcptr++   = sumb - sumc;
    		    		   
	      lq++;
	    }
	}
      else
	{
	  if(refQ >= 0)
	    {
	      mlvQ = &realQ->mlv[refQ];
	
	      fx1a = tr->rdta->freqa * lp->a;
	      fx1g = tr->rdta->freqg * lp->g;
	      fx1c = tr->rdta->freqc * lp->c;
	      fx1t = tr->rdta->freqt * lp->t;
	      suma = fx1a * mlvQ->a + fx1c * mlvQ->c + fx1g * mlvQ->g + fx1t * mlvQ->t;
	      fx2r = tr->rdta->freqa * mlvQ->a + tr->rdta->freqg * mlvQ->g;
	      fx2y = tr->rdta->freqc * mlvQ->c + tr->rdta->freqt * mlvQ->t;
	      fx1r = fx1a + fx1g;
	      fx1y = fx1c + fx1t;
	      *sumcptr++ = sumc = (fx1r + fx1y) * (fx2r + fx2y);
	      sumb       = fx1r * fx2r * tr->rdta->invfreqr + fx1y * fx2y * tr->rdta->invfreqy;
	      *abptr++   = suma - sumb;
	      *bcptr++   = sumb - sumc;
	      
	      lp++;
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
	      *sumcptr++ = sumc = (fx1r + fx1y) * (fx2r + fx2y);
	      sumb       = fx1r * fx2r * tr->rdta->invfreqr + 
		fx1y * fx2y * tr->rdta->invfreqy;
	      *abptr++   = suma - sumb;
	      *bcptr++   = sumb - sumc;
	      
	      lq++;
	      lp++;
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
        
	zz = exp(lz);
	zv = exp(xvlz);
          
	abptr   = abi;
	bcptr   = bci;
	sumcptr = sumci;        
	wrptr   = &(tr->cdta->wr[0]);
	wr2ptr  = &(tr->cdta->wr2[0]);
	dlnLdlz = 0.0;                 /*  = d(ln(likelihood))/d(lz) */
	d2lnLdlz2 = 0.0;               /*  = d2(ln(likelihood))/d(lz)2 */



        for (i = 0; i < tr->cdta->endsite; i++) {
         
          ab     = *abptr++ * zz;
          bc     = *bcptr++ * zv;
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

    free(abi);
    free(bci);
    free(sumci);
    

    

/* printf("makenewz: %le\n", z); */

    return  z;
}








boolean newviewGTR(tree    *tr, nodeptr  p)
{
  
  if (p->tip) 
    {
      
      if (p->x) return TRUE;
      if (! getxtip(p)) return FALSE;  
      {
	homType *mlvP;
	int code, i, j, jj;
	double (*EIGV)[4] = &(tr->rdta->EIGV[0][0]);
	int refREV[EQUALITIES];
	int *ev = p->equalityVector;
                 
      mlvP = tr->nodep[p->number]->mlv;
     
      for(i = 0; i < EQUALITIES; i++)
	{	      	  	 
	  mlvP->a = 0;
	  mlvP->c = 0;
	  mlvP->g = 0;
	  mlvP->t = 0;
	  code = i + 1;
	  for (j = 0; j < 4; j++) 
	    {
	      if ((code >> j) & 1) 
		{
		  int jj = "0213"[j] - '0';		
		  mlvP->a += EIGV[jj][0];
		  mlvP->c += EIGV[jj][1];
		  mlvP->g += EIGV[jj][2];
		  mlvP->t += EIGV[jj][3];
		}
	    }		
	      
	  mlvP->exp = 0;	  			  			 
	  mlvP++;	 
	} 
      return TRUE; 
      }
    }
  
  { 	
    double  diagptable_x1[4], diagptable_x2[4];
    double *EIGN    = &(tr->rdta->EIGN[0][0]);
    double *invfreq = &(tr->rdta->invfreq[0][0]);
    double (*EIGV)[4] = &(tr->rdta->EIGV[0][0]);
    xtype tmp_x1_1, tmp_x1_2, tmp_x1_3,
      tmp_x2_1, tmp_x2_2, tmp_x2_3, 
      ump_x1_1, ump_x1_2, ump_x1_3, ump_x1_0, 
      ump_x2_0, ump_x2_1, ump_x2_2, ump_x2_3, x1px2;
    homType *mlvR, *mlvQ, *mlvP;
    poutsa calcQ[EQUALITIES],calcR[EQUALITIES], *calcptr, *calcptrR;
    long ref, ref2, mexp;      
    nodeptr realQ, realR, realP;	  
    int  i;
    likelivector   *x1, *x2, *x3;
    double   z1, lz1, z2, lz2;
    nodeptr  q, r;
    register int *eqptr, *s1, *s2;
    double *eigv;
    q = p->next->back;
    r = p->next->next->back;
      
    while ((! p->x) || (! q->x) || (! r->x)) {
      if (! q->x) if (! newviewGTR(tr, q)) return FALSE;
      if (! r->x) if (! newviewGTR(tr, r)) return FALSE;
      if (! p->x) if (! getxnode(p)) return FALSE;	
    }

    x1  = q->x->lv;
    z1  = q->z;
    lz1 = (z1 > zmin) ? log(z1) : log(zmin);

    x2  = r->x->lv;
    z2  = r->z;
    lz2 = (z2 > zmin) ? log(z2) : log(zmin);

    x3  = p->x->lv;
      
    realP =  tr->nodep[p->number];
    realQ =  tr->nodep[q->number];
    realR =  tr->nodep[r->number];

    mlvP = realP->mlv;
    mlvQ = realQ->mlv;
    mlvR = realR->mlv;

    eqptr = realP->equalityVector; 
    s1 = realQ->equalityVector;
    s2 = realR->equalityVector; 
     	  
    for (i = 1; i < 4; i++) 
      {
	diagptable_x1[i] = exp (EIGN[i] * lz1);
	diagptable_x2[i] = exp (EIGN[i] * lz2);	    
      }	       
    
    for(i = 0; i < EQUALITIES; i++)
      {	  
	calcptr = &calcQ[i];		 
	tmp_x1_1 = mlvQ->c * diagptable_x1[1];
	tmp_x1_2 = mlvQ->g * diagptable_x1[2];
	tmp_x1_3 = mlvQ->t * diagptable_x1[3];
	
	calcptr->sumaq = tmp_x1_1 * EIGV[0][1];
	calcptr->sumaq += tmp_x1_2 * EIGV[0][2];
	calcptr->sumaq += tmp_x1_3 * EIGV[0][3];	      	
	calcptr->sumaq *= invfreq[0];
	calcptr->sumaq += mlvQ->a;
	
	calcptr->sumcq = tmp_x1_1 * EIGV[1][1];
	calcptr->sumcq += tmp_x1_2 * EIGV[1][2];
	calcptr->sumcq += tmp_x1_3 * EIGV[1][3];	      	
	calcptr->sumcq *= invfreq[1];
	calcptr->sumcq += mlvQ->a;
	
	calcptr->sumgq = tmp_x1_1 * EIGV[2][1];
	calcptr->sumgq += tmp_x1_2 * EIGV[2][2];
	calcptr->sumgq += tmp_x1_3 * EIGV[2][3];	      	
	calcptr->sumgq *= invfreq[2];
	calcptr->sumgq += mlvQ->a;
	
	calcptr->sumtq = tmp_x1_1 * EIGV[3][1];
	calcptr->sumtq += tmp_x1_2 * EIGV[3][2];
	calcptr->sumtq += tmp_x1_3 * EIGV[3][3];	      	
	calcptr->sumtq *= invfreq[3];
	calcptr->sumtq += mlvQ->a;	  
	
	calcptr->exp = mlvQ->exp;
	
	calcptrR = &calcR[i];
	tmp_x2_1 = mlvR->c * diagptable_x2[1];
	tmp_x2_2 = mlvR->g * diagptable_x2[2];
	tmp_x2_3 = mlvR->t * diagptable_x2[3];
	
	calcptrR->sumaq = tmp_x2_1 * EIGV[0][1];
	calcptrR->sumaq += tmp_x2_2 * EIGV[0][2];
	calcptrR->sumaq += tmp_x2_3 * EIGV[0][3];		     
	calcptrR->sumaq *= invfreq[0];
	calcptrR->sumaq += mlvR->a;
	
	calcptrR->sumcq = tmp_x2_1 * EIGV[1][1];
	calcptrR->sumcq += tmp_x2_2 * EIGV[1][2];
	calcptrR->sumcq += tmp_x2_3 * EIGV[1][3];		     
	calcptrR->sumcq *= invfreq[1];
	calcptrR->sumcq += mlvR->a;	 
	
	calcptrR->sumgq = tmp_x2_1 * EIGV[2][1];
	calcptrR->sumgq += tmp_x2_2 * EIGV[2][2];
	calcptrR->sumgq += tmp_x2_3 * EIGV[2][3];		     
	calcptrR->sumgq *= invfreq[2];
	calcptrR->sumgq += mlvR->a;	  
	
	calcptrR->sumtq  = tmp_x2_1 * EIGV[3][1] ;
	calcptrR->sumtq  += tmp_x2_2 * EIGV[3][2];
	calcptrR->sumtq  += tmp_x2_3 * EIGV[3][3];		     
	calcptrR->sumtq  *= invfreq[3];
	calcptrR->sumtq  += mlvR->a;	    	  	      
	
	calcptrR->exp = mlvR->exp;
	



	mlvP->a = calcptrR->sumaq * calcptr->sumaq * EIGV[0][0];
	mlvP->c = calcptrR->sumaq * calcptr->sumaq * EIGV[0][1];
	mlvP->g = calcptrR->sumaq * calcptr->sumaq * EIGV[0][2];
	mlvP->t = calcptrR->sumaq * calcptr->sumaq * EIGV[0][3]; 
	
	mlvP->a += calcptrR->sumcq * calcptr->sumcq * EIGV[1][0];
	mlvP->c += calcptrR->sumcq * calcptr->sumcq * EIGV[1][1];
	mlvP->g += calcptrR->sumcq * calcptr->sumcq * EIGV[1][2];
	mlvP->t += calcptrR->sumcq * calcptr->sumcq * EIGV[1][3];
	
	mlvP->a += calcptrR->sumgq  * calcptr->sumgq * EIGV[2][0];
	mlvP->c += calcptrR->sumgq  * calcptr->sumgq * EIGV[2][1];
	mlvP->g += calcptrR->sumgq  * calcptr->sumgq * EIGV[2][2];
	mlvP->t += calcptrR->sumgq  * calcptr->sumgq * EIGV[2][3];
	
	mlvP->a += calcptrR->sumtq * calcptr->sumtq *  EIGV[3][0];
	mlvP->c += calcptrR->sumtq * calcptr->sumtq *  EIGV[3][1];
	mlvP->g += calcptrR->sumtq * calcptr->sumtq *  EIGV[3][2];
	mlvP->t += calcptrR->sumtq * calcptr->sumtq *  EIGV[3][3];		     
	
	mlvP->exp = calcptr->exp + calcptrR->exp;	
	if (ABS(mlvP->a) < minlikelihood && ABS(mlvP->g) < minlikelihood
	    && ABS(mlvP->c) < minlikelihood && ABS(mlvP->t) < minlikelihood) 
	  {	     
	    mlvP->a   *= twotothe256;
	    mlvP->g   *= twotothe256;
	    mlvP->c   *= twotothe256;
	    mlvP->t   *= twotothe256;
	    mlvP->exp += 1;
	  }
	mlvQ++;
	mlvR++;
	mlvP++;
      }

    if(r->tip && q->tip)
	{	 
	  for (i = 0; i < tr->cdta->endsite; i++) 
	    {
	      ref = *s2++;
	      ref2 = *s1++;
	     	      	      		  	
	      if(ref2 != ref)
		{
		  *eqptr++ = -1;
		  calcptrR = &calcR[ref];
		  calcptr = &calcQ[ref2];
		  eigv = &EIGV[0][0];
		  x1px2 = calcptrR->sumaq * calcptr->sumaq;
		  x3->a =  x1px2 * *eigv++;/*EIGV[0][0];*/
		  x3->c =  x1px2 * *eigv++;/*EIGV[0][1];*/
		  x3->g =  x1px2 * *eigv++;/*EIGV[0][2];*/
		  x3->t =  x1px2 * *eigv++;/*EIGV[0][3];*/
	    
		  x1px2 = calcptrR->sumcq * calcptr->sumcq;
		  x3->a +=  x1px2 * *eigv++;/*EIGV[1][0];*/
		  x3->c +=  x1px2 * *eigv++;/*EIGV[1][1];*/
		  x3->g +=  x1px2 * *eigv++;/*EIGV[1][2];*/
		  x3->t +=  x1px2 * *eigv++;/*EIGV[1][3];*/
	    
		  x1px2 = calcptrR->sumgq * calcptr->sumgq;
		  x3->a += x1px2 * *eigv++;/*EIGV[2][0];*/
		  x3->c += x1px2 * *eigv++;/*EIGV[2][1];*/
		  x3->g += x1px2 * *eigv++;/*EIGV[2][2];*/
		  x3->t += x1px2 * *eigv++;/*EIGV[2][3];*/
	    
		  x1px2 = calcptrR->sumtq * calcptr->sumtq;
		  x3->a += x1px2  *  *eigv++;/*EIGV[3][0];*/
		  x3->c += x1px2  *  *eigv++;/*EIGV[3][1];*/
		  x3->g += x1px2  *  *eigv++;/*EIGV[3][2];*/
		  x3->t += x1px2  *  *eigv++;/*EIGV[3][3];*/		     
		  
		  x3->exp = calcptrR->exp + calcptr->exp;
		  if (ABS(x3->a) < minlikelihood && ABS(x3->g) < minlikelihood
		      && ABS(x3->c) < minlikelihood && ABS(x3->t) < minlikelihood) 
		    {	     
		      x3->a   *= twotothe256;
		      x3->g   *= twotothe256;
		      x3->c   *= twotothe256;
		      x3->t   *= twotothe256;
		      x3->exp += 1;
		    }
		  x3++;   
		}
	      else
		{
		  *eqptr++ = ref;		    		
		}
	   	      	    
	    }          
	}
      else
	{
	  for (i = 0; i < tr->cdta->endsite; i++) 
	    {
	      ref = *s2++;
	      ref2 = *s1++;
	      
	      if(ref2 >= 0)
		{		  	
		  if(ref2 == ref)
		    {
		      *eqptr++ = ref;		     		  
		      goto incTip;		    		  		 		  	    		
		    }
		  calcptr = &calcQ[ref2];
		  ump_x1_0 = calcptr->sumaq;
		  ump_x1_1 = calcptr->sumcq;
		  ump_x1_2 = calcptr->sumgq;
		  ump_x1_3 = calcptr->sumtq;		 		  		       		  		
		  mexp = calcptr->exp;
		}	    	  	  	 	 
	      else
		{		
		  tmp_x1_1 = x1->c * diagptable_x1[1];
		  tmp_x1_2 = x1->g * diagptable_x1[2];
		  tmp_x1_3 = x1->t * diagptable_x1[3];
		  
		  ump_x1_0 = tmp_x1_1 * EIGV[0][1];
		  ump_x1_0 += tmp_x1_2 * EIGV[0][2];
		  ump_x1_0 += tmp_x1_3 * EIGV[0][3];	      	
		  ump_x1_0 *= invfreq[0];
		  ump_x1_0 += x1->a;
		  
		  ump_x1_1 = tmp_x1_1 * EIGV[1][1];
		  ump_x1_1 += tmp_x1_2 * EIGV[1][2];
		  ump_x1_1 += tmp_x1_3 * EIGV[1][3];	      	
		  ump_x1_1 *= invfreq[1];
		  ump_x1_1 += x1->a;
		  
		  ump_x1_2 = tmp_x1_1 * EIGV[2][1];
		  ump_x1_2 += tmp_x1_2 * EIGV[2][2];
		  ump_x1_2 += tmp_x1_3 * EIGV[2][3];	      	
		  ump_x1_2 *= invfreq[2];
		  ump_x1_2 += x1->a;

		  ump_x1_3 = tmp_x1_1 * EIGV[3][1];
		  ump_x1_3 += tmp_x1_2 * EIGV[3][2];
		  ump_x1_3 += tmp_x1_3 * EIGV[3][3];	      	
		  ump_x1_3 *= invfreq[3];
		  ump_x1_3 += x1->a;
		  mexp = x1->exp;
		  x1++;
		}
	      	      
	      if(ref >= 0)
		{
		  calcptr = &calcR[ref];		
		  eigv = &EIGV[0][0];
		  x3->a = ump_x1_0 * calcptr->sumaq * *eigv++;/*EIGV[0][0];*/
		  x3->c = ump_x1_0 * calcptr->sumaq *  *eigv++;/*EIGV[0][1];*/
		  x3->g = ump_x1_0 * calcptr->sumaq *  *eigv++;/*EIGV[0][2];*/
		  x3->t = ump_x1_0 * calcptr->sumaq *  *eigv++;/*EIGV[0][3]; */
		  
		  x3->a += ump_x1_1 * calcptr->sumcq *  *eigv++;/*EIGV[1][0];*/
		  x3->c += ump_x1_1 * calcptr->sumcq *  *eigv++;/*EIGV[1][1];*/
		  x3->g += ump_x1_1 * calcptr->sumcq *  *eigv++;/*EIGV[1][2];*/
		  x3->t += ump_x1_1 * calcptr->sumcq *  *eigv++;/*EIGV[1][3];*/
		  
		  x3->a += ump_x1_2 * calcptr->sumgq *  *eigv++;/*EIGV[2][0];*/
		  x3->c += ump_x1_2 * calcptr->sumgq *  *eigv++;/*EIGV[2][1];*/
		  x3->g += ump_x1_2 * calcptr->sumgq *  *eigv++;/*EIGV[2][2];*/
		  x3->t += ump_x1_2 * calcptr->sumgq *  *eigv++;/*EIGV[2][3];*/
		  
		  x3->a += ump_x1_3 *  calcptr->sumtq *  *eigv++;/*EIGV[3][0];*/
		  x3->c += ump_x1_3 *  calcptr->sumtq *  *eigv++;/*EIGV[3][1];*/
		  x3->g += ump_x1_3 *  calcptr->sumtq *  *eigv++;/*EIGV[3][2];*/
		  x3->t += ump_x1_3 *  calcptr->sumtq *  *eigv++;/*EIGV[3][3];*/
		 
		  x3->exp = mexp + calcptr->exp;
		}
	      else
		{
		   tmp_x2_1 = x2->c * diagptable_x2[1];
		   tmp_x2_2 = x2->g * diagptable_x2[2];
		   tmp_x2_3 = x2->t * diagptable_x2[3];
	 
	 	    	    	     	     
		   ump_x2_0 = tmp_x2_1 * EIGV[0][1];
		   ump_x2_0 += tmp_x2_2 * EIGV[0][2];
		   ump_x2_0 += tmp_x2_3 * EIGV[0][3];		     
		   ump_x2_0 *= invfreq[0];
		   ump_x2_0 += x2->a;
	  
		   ump_x2_1 = tmp_x2_1 * EIGV[1][1];
		   ump_x2_1 += tmp_x2_2 * EIGV[1][2];
		   ump_x2_1 += tmp_x2_3 * EIGV[1][3];		     
		   ump_x2_1 *= invfreq[1];
		   ump_x2_1 += x2->a;	 

		   ump_x2_2 = tmp_x2_1 * EIGV[2][1];
		   ump_x2_2 += tmp_x2_2 * EIGV[2][2];
		   ump_x2_2 += tmp_x2_3 * EIGV[2][3];		     
		   ump_x2_2 *= invfreq[2];
		   ump_x2_2 += x2->a;	  
		   

		   ump_x2_3 = tmp_x2_1 * EIGV[3][1] ;
		   ump_x2_3 += tmp_x2_2 * EIGV[3][2];
		   ump_x2_3 += tmp_x2_3 * EIGV[3][3];		     
		   ump_x2_3 *= invfreq[3];
		   ump_x2_3 += x2->a;	    	  		   	  
		   eigv = &EIGV[0][0];

		   x1px2 = ump_x1_0 * ump_x2_0;
		   x3->a = x1px2 *  *eigv++;/*EIGV[0][0];*/
		   x3->c = x1px2 *  *eigv++;/*EIGV[0][1];*/
		   x3->g = x1px2 *  *eigv++;/*EIGV[0][2];*/
		   x3->t = x1px2 *  *eigv++;/*EIGV[0][3]; */

		   x1px2 = ump_x1_1 * ump_x2_1;
		   x3->a += x1px2  *  *eigv++;/*EIGV[1][0];*/
		   x3->c += x1px2 *  *eigv++;/*EIGV[1][1];*/
		   x3->g += x1px2 *  *eigv++;/*EIGV[1][2];*/
		   x3->t += x1px2 *  *eigv++;/*EIGV[1][3];*/
	   
		   x1px2 = ump_x1_2 * ump_x2_2;
		   x3->a += x1px2 *  *eigv++;/*EIGV[2][0];*/
		   x3->c += x1px2*  *eigv++;/*EIGV[2][1];*/
		   x3->g += x1px2 *  *eigv++;/*EIGV[2][2];*/
		   x3->t += x1px2 *  *eigv++;/*EIGV[2][3];*/

		   x1px2 = ump_x1_3 * ump_x2_3;
		   x3->a += x1px2 *   *eigv++;/*EIGV[3][0];*/
		   x3->c += x1px2 *   *eigv++;/*EIGV[3][1];*/
		   x3->g += x1px2 *   *eigv++;/*EIGV[3][2];*/
		   x3->t += x1px2 *   *eigv++;/*EIGV[3][3];*/
		   
		   x3->exp = mexp + x2->exp;
		   x2++;
		}
	      	    
	      if (ABS(x3->a) < minlikelihood && ABS(x3->g) < minlikelihood
		  && ABS(x3->c) < minlikelihood && ABS(x3->t) < minlikelihood) 
		{	     
		  x3->a   *= twotothe256;
		  x3->g   *= twotothe256;
		  x3->c   *= twotothe256;
		  x3->t   *= twotothe256;
		  x3->exp += 1;
		}


 	      
	      *eqptr++ = -1;
	      x3++;
	    incTip:	 ;
	    }    
	  
	  return TRUE;
	}
  }
}





double evaluateGTR (tr, p)
    tree    *tr;
    nodeptr  p;
  { /* evaluate */
    double   sum, z, lz, term;    
    nodeptr  q;
    int     i, j;
    int     *wptr = tr->cdta->aliaswgt;
    
    double  diagptable[4];
    double *EIGN = &(tr->rdta->EIGN[0][0]);
    likelivector   *x1, *x2;
    homType *mlvP, *mlvQ;
    nodeptr realQ, realP;
    register  int *sQ; 
    register  int *sP;
    long refQ, refP;    
    q = p->back;
    while ((! p->x) || (! q->x)) {
      if (! (p->x)) if (! newviewGTR(tr, p)) return badEval;
      if (! (q->x)) if (! newviewGTR(tr, q)) return badEval;
    }


    x1  = p->x->lv;
    x2  = q->x->lv;
    z = p->z;
   
    if (z < zmin) z = zmin;
    lz = log(z);

    diagptable[0] = 1;
    diagptable[1] = exp (EIGN[1] * lz);
    diagptable[2] = exp (EIGN[2] * lz);
    diagptable[3] = exp (EIGN[3] * lz);
    
    
    
    sum = 0.0;
    realQ =  tr->nodep[q->number];
    realP =  tr->nodep[p->number];
    mlvP = realP->mlv;
    mlvQ = realQ->mlv;
    sQ = realQ->equalityVector;
    sP = realP->equalityVector;

   

    for (i = 0; i < tr->cdta->endsite; i++) {
	    refQ = *sQ++;
	    refP = *sP++;

	    

	    if(refP >= 0)
	      {
		if(refQ >= 0)
		  {
		    mlvP = &realP->mlv[refP];
		    mlvQ = &realQ->mlv[refQ];
		    
		    term =  mlvP->a * mlvQ->a * diagptable[0];
		    term += mlvP->c * mlvQ->c * diagptable[1];
		    term += mlvP->g * mlvQ->g * diagptable[2];
		    term += mlvP->t * mlvQ->t * diagptable[3];     
		    term = log(term) + (mlvQ->exp + mlvP->exp)*log(minlikelihood);
		    sum += *wptr++ * term;
		  }
		else
		  {
		    mlvP = &realP->mlv[refP];
		    term =  mlvP->a * x2->a * diagptable[0];
		    term += mlvP->c * x2->c * diagptable[1];
		    term += mlvP->g * x2->g * diagptable[2];
		    term += mlvP->t * x2->t * diagptable[3];     
		    term = log(term) + (mlvP->exp + x2->exp)*log(minlikelihood);
		   
		    sum += *wptr++ * term;      		    		   
		    x2++;
		  }
	      }
	    else
	      {
		if(refQ >= 0)
		  {
		    mlvQ = &realQ->mlv[refQ];
		    term =  x1->a * mlvQ->a * diagptable[0];
		    term += x1->c * mlvQ->c * diagptable[1];
		    term += x1->g * mlvQ->g * diagptable[2];
		    term += x1->t * mlvQ->t * diagptable[3];     
		    term = log(term) + (x1->exp + mlvQ->exp)*log(minlikelihood); 
		    sum += *wptr++ * term;		   
		    x1++;
		  }
		else
		  {
		    term =  x1->a * x2->a * diagptable[0];
		    term += x1->c * x2->c * diagptable[1];
		    term += x1->g * x2->g * diagptable[2];
		    term += x1->t * x2->t * diagptable[3];     
		    term = log(term) + (x1->exp + x2->exp)*log(minlikelihood);
		    sum += *wptr++ * term;		  
		    x1++;
		    x2++;
		  }
	      }	    
    }
    
    tr->likelihood = sum;
    return  sum;
  } /* evaluate */


double makenewzGTR (tr, p, q, z0, maxiter)
    tree    *tr;
    nodeptr  p, q;
    double   z0;
    int  maxiter;
  { /* makenewz */
    double   z, zprev, zstep;
    likelivector  *x1, *x2;
    double  *sumtable, *sum;
    int     i;
    double  dlnLdlz = 0.0;                 /*  = d(ln(likelihood))/d(lz) */
    double  d2lnLdlz2 = 0.0;               /*  = d2(ln(likelihood))/d(lz)2 */
    homType *mlvP, *mlvQ;
    nodeptr realQ, realP;
    register  int *sQ; 
    register  int *sP;
    long refQ, refP;
    double d0, d1, d2, d3;
    double e0, e1, e2, e3;

    while ((! p->x) || (! q->x)) {
      if (! (p->x)) if (! newviewGTR(tr, p)) return badZ;
      if (! (q->x)) if (! newviewGTR(tr, q)) return badZ;
    }

    x1 = p->x->lv;
    x2 = q->x->lv;

    sum = sumtable = (double *)malloc(4 * tr->cdta->endsite * 
				      sizeof(double));     
    realQ =  tr->nodep[q->number];
    realP =  tr->nodep[p->number];
    mlvP = realP->mlv;
    mlvQ = realQ->mlv;
    sQ = realQ->equalityVector;
    sP = realP->equalityVector;

    for (i = 0; i < tr->cdta->endsite; i++) 
      {     
	refQ = *sQ++;
	refP = *sP++;


	if(refP >= 0)
	      {
		if(refQ >= 0)
		  {
		    mlvP = &realP->mlv[refP];
		    mlvQ = &realQ->mlv[refQ];
		    *sum++ = mlvP->a * mlvQ->a;
		    *sum++ = mlvP->c * mlvQ->c;
		    *sum++ = mlvP->g * mlvQ->g;
		    *sum++ = mlvP->t * mlvQ->t;
		  }
		else
		  {
		    mlvP = &realP->mlv[refP];
		    *sum++ = mlvP->a * x2->a;
		    *sum++ = mlvP->c * x2->c;
		    *sum++ = mlvP->g * x2->g;
		    *sum++ = mlvP->t * x2->t;
    		    		   
		    x2++;
		  }
	      }
	    else
	      {
		if(refQ >= 0)
		  {
		    mlvQ = &realQ->mlv[refQ];
		    *sum++ = x1->a * mlvQ->a;
		    *sum++ = x1->c * mlvQ->c;
		    *sum++ = x1->g * mlvQ->g;
		    *sum++ = x1->t * mlvQ->t;

		    x1++;
		  }
		else
		  {
		    *sum++ = x1->a * x2->a;
		    *sum++ = x1->c * x2->c;
		    *sum++ = x1->g * x2->g;
		    *sum++ = x1->t * x2->t;
		    
		    x1++;
		    x2++;
		  }
	      }

      }

   
   

    z = z0;
    do {
      int curvatOK = FALSE;

      zprev = z;
      
      zstep = (1.0 - zmax) * z + zmin;

      do {
	double  diagptable[4];
	double *EIGN    = &(tr->rdta->EIGN[0][0]);
	double lz;
        double *wrptr   = &(tr->cdta->wr[0]);
        double *wr2ptr  = &(tr->cdta->wr2[0]);

        dlnLdlz = 0.0;                 /*  = d(ln(likelihood))/d(lz) */
        d2lnLdlz2 = 0.0;               /*  = d2(ln(likelihood))/d(lz)2 */

        if (z < zmin) z = zmin;
        else if (z > zmax) z = zmax;
        lz    = log(z);
        
	d1 = exp (EIGN[1] * lz); 
	d2 = exp (EIGN[2] * lz); 
	d3 = exp (EIGN[3] * lz); 
	
	sum = sumtable;
        for (i = 0; i < tr->cdta->endsite; i++) 
	  {
	    double tmp_0, tmp_1, tmp_2, tmp_3;
	    double t, inv_Li, dlnLidlz = 0, d2lnLidlz2 = 0;
	    	 
	    inv_Li = (tmp_0 = *sum++);
	    inv_Li += (tmp_1 = *sum++ * d1);
	    inv_Li += (tmp_2 = *sum++ * d2);
	    inv_Li += (tmp_3 = *sum++ * d3);

	    inv_Li = 1.0/inv_Li;
	    
	  
	    t = tmp_1 * EIGN[1];
	    dlnLidlz   += t;
	    d2lnLidlz2 += t * EIGN[1];
	    
	    t = tmp_2 * EIGN[2];
	    dlnLidlz   += t;
	    d2lnLidlz2 += t * EIGN[2];
	    
	    t = tmp_3 * EIGN[3];
	    dlnLidlz   += t;
	    d2lnLidlz2 += t * EIGN[3];
	    
	    dlnLidlz   *= inv_Li;
	    d2lnLidlz2 *= inv_Li;
	    dlnLdlz   += *wrptr++  * dlnLidlz;
	    d2lnLdlz2 += *wr2ptr++ * (d2lnLidlz2 - dlnLidlz * dlnLidlz);
	  }
	
        if ((d2lnLdlz2 >= 0.0) && (z < zmax))
          zprev = z = 0.37 * z + 0.63;  /*  Bad curvature, shorten branch */
        else
          curvatOK = TRUE;
	
      } while (! curvatOK);

      if (d2lnLdlz2 < 0.0) {
	double tantmp = -dlnLdlz / d2lnLdlz2;  /* prevent overflow */
	if (tantmp < 100) {
	  z *= exp(tantmp);
	  if (z < zmin) z = zmin;
	  if (z > 0.25 * zprev + 0.75)    /*  Limit steps toward z = 1.0 */
	    z = 0.25 * zprev + 0.75;
	} else {
	  z = 0.25 * zprev + 0.75;
	}
      }
      if (z > zmax) z = zmax;

    } while ((--maxiter > 0) && (ABS(z - zprev) > zstep));

    free (sumtable);

    return  z;
  } /* makenewz */



boolean newviewHKY85CAT (tree *tr, nodeptr p)        /*  Update likelihoods at node */
  { /* newview */
    double   zq, lzq, xvlzq, zr, lzr, xvlzr;
    nodeptr  q, r;
    likelivector *lp, *lq, *lr;
    int  i;

    

    if (p->tip) {             /*  Make sure that data are at tip */
      likelivector *l;
      int           code;
      yType        *yptr;

      if (p->x) return TRUE;  /*  They are already there */

      if (! getxtip(p)) return FALSE; /*  They are not, so get memory */
      l = p->x->lv;           /*  Pointer to first likelihood vector value */
      yptr = p->tip;          /*  Pointer to first nucleotide datum */
      for (i = 0; i < tr->cdta->endsite; i++) {
        code = *yptr++;
        l->a =  code       & 1;
        l->c = (code >> 1) & 1;
        l->g = (code >> 2) & 1;
        l->t = (code >> 3) & 1;
        l->exp = 0;

	
        l++;
        }
      return TRUE;
      }

/*  Internal node needs update */

    q = p->next->back;
    r = p->next->next->back;

    

    while ((! p->x) || (! q->x) || (! r->x)) {
      if (! q->x) if (! newviewHKY85CAT(tr, q))  return FALSE;
      if (! r->x) if (! newviewHKY85CAT(tr, r))  return FALSE;
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

    { double  *zzqtable, *zvqtable,
              *zzrtable, *zvrtable,
             *zzqptr, *zvqptr, *zzrptr, *zvrptr, *rptr;
      double  fxqr, fxqy, fxqn, sumaq, sumgq, sumcq, sumtq,
              fxrr, fxry, fxrn, ki, tempi, tempj;
      int  *cptr;
      double zzq, zvq, zzr, zvr;
      int cat;

      zzqtable = (double *)malloc(tr->NumberOfCategories * sizeof(double));
      zvqtable = (double *)malloc(tr->NumberOfCategories * sizeof(double));
      zzrtable = (double *)malloc(tr->NumberOfCategories * sizeof(double));
      zvrtable = (double *)malloc(tr->NumberOfCategories * sizeof(double));
     
    
      rptr   = &(tr->cdta->patrat[0]);
      zzqptr = &(zzqtable[0]);
      zvqptr = &(zvqtable[0]);
      zzrptr = &(zzrtable[0]);
      zvrptr = &(zvrtable[0]);
      cptr  = &(tr->cdta->rateCategory[0]);

     

      for (i = 0; i < tr->NumberOfCategories; i++) {   
        ki = *rptr++;	
	
        *zzqptr++ = ki *   lzq;
        *zvqptr++ = ki * xvlzq;
        *zzrptr++ = ki *   lzr;
        *zvrptr++ = ki * xvlzr;
        }

      zzqptr = &(zzqtable[0]);
      zvqptr = &(zvqtable[0]);
      zzrptr = &(zzrtable[0]);
      zvrptr = &(zvrtable[0]);
      
      vdExp (tr->NumberOfCategories, zzqptr, zzqptr);
      vdExp (tr->NumberOfCategories, zvqptr, zvqptr);
      vdExp (tr->NumberOfCategories, zzrptr, zzrptr);
      vdExp (tr->NumberOfCategories, zvrptr, zvrptr);

    
     

        for (i = 0; i < tr->cdta->endsite; i++) {
          cat = *cptr++;
	  
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
          lp->exp = lq->exp + lr->exp;
	 
	  

          if (lp->a < minlikelihood && lp->g < minlikelihood
           && lp->c < minlikelihood && lp->t < minlikelihood) {
            lp->a   *= twotothe256;
            lp->g   *= twotothe256;
            lp->c   *= twotothe256;
            lp->t   *= twotothe256;
            lp->exp += 1;
            }
          lp++;
          lq++;
          lr++;
          }

	free(zzqtable);
	free(zvqtable);
	free(zzrtable);
	free(zvrtable);


      return TRUE;
      }
  } /* newview */


double evaluateHKY85CAT (tree *tr, nodeptr p)
  { /* evaluate */
    double   sum, z, lz, xvlz,
             ki, fxpa, fxpc, fxpg, fxpt, fxpr, fxpy, fxqr, fxqy,
             suma, sumb, sumc, term;
    double *tv1, *tv2, *tv1a, *tv2a;

       double   zz, zv;

    double        *zztable, *zvtable,
                 *zzptr, *zvptr;
    double        *rptr;
    likelivector *lp, *lq;
    nodeptr       q;
    int           cat, *cptr, i, *wptr;

    q = p->back;
    while ((! p->x) || (! q->x)) {
      if (! (p->x)) if (! newviewHKY85CAT(tr, p)) return badEval;
      if (! (q->x)) if (! newviewHKY85CAT(tr, q)) return badEval;
      }

    lp = p->x->lv;
    lq = q->x->lv;

    z = p->z;
    if (z < zmin) z = zmin;
    lz = log(z);
    xvlz = tr->rdta->xv * lz;

    zztable = (double *)malloc(tr->NumberOfCategories * sizeof(double));
    zvtable = (double *)malloc(tr->NumberOfCategories * sizeof(double));

     tv1 = tv1a = (double *)malloc(sizeof(double) * tr->cdta->endsite);
     tv2 = tv2a = (double *)malloc(sizeof(double) * tr->cdta->endsite);

    rptr   = tr->cdta->patrat;
    cptr  = tr->cdta->rateCategory; 
    zzptr = zztable;
    zvptr = zvtable;

    for (i = 0; i < tr->NumberOfCategories; i++) {
      ki = *rptr++;
      *zzptr++ = ki *   lz;
      *zvptr++ = ki * xvlz;
      }
    
    zzptr = zztable;
    zvptr = zvtable;
    vdExp (tr->NumberOfCategories, zzptr, zzptr);
    vdExp (tr->NumberOfCategories, zvptr, zvptr);
    
    wptr = &(tr->cdta->aliaswgt[0]);
    
   
    sum = 0.0;


      for (i = 0; i < tr->cdta->endsite; i++) {
        cat  = *cptr++;
        zz   = zztable[cat];
        zv   = zvtable[cat];
        fxpa = tr->rdta->freqa * lp->a;
        fxpg = tr->rdta->freqg * lp->g;
        fxpc = tr->rdta->freqc * lp->c;
        fxpt = tr->rdta->freqt * lp->t;
        suma = fxpa * lq->a + fxpc * lq->c + fxpg * lq->g + fxpt * lq->t;
        fxqr = tr->rdta->freqa * lq->a + tr->rdta->freqg * lq->g;
        fxqy = tr->rdta->freqc * lq->c + tr->rdta->freqt * lq->t;
        fxpr = fxpa + fxpg;
        fxpy = fxpc + fxpt;
        sumc = (fxpr + fxpy) * (fxqr + fxqy);
        sumb = fxpr * fxqr * tr->rdta->invfreqr + fxpy * fxqy * tr->rdta->invfreqy;
        suma -= sumb;
        sumb -= sumc;
        /*term = log(zz * suma + zv * sumb + sumc) + (lp->exp + lq->exp)*log(minlikelihood);

        sum += *wptr++ * term;*/
	*tv1++ = zz * suma + zv * sumb + sumc;
	*tv2++ = (lp->exp + lq->exp)*log(minlikelihood);
        lp++;
        lq++;
        }

       tv1 = tv1a;
       tv2 = tv2a;
       vdLn (tr->cdta->endsite, tv1, tv1);

        for(i = 0; i < tr->cdta->endsite; i++)
	{
	  sum += *wptr++ * *tv1++ + *tv2++;
	  /*wptr++;
	  tv1++;
	  tv2++;*/
	}

      free(zztable);
      free(zvtable); 
      free(tv1a);
      free(tv2a);

    tr->likelihood = sum;
    return  sum;
  } /* evaluate */



double makenewzHKY85CAT (tree *tr, nodeptr p, nodeptr q, double z0, int maxiter)
  { /* makenewz */
    likelivector *lp, *lq;
    double  *abi, *bci, *sumci, *abptr, *bcptr, *sumcptr;
    double   dlnLidlz, dlnLdlz, d2lnLdlz2, z, zprev, zstep, lz, xvlz,
             ki, suma, sumb, sumc, ab, bc, inv_Li, t1, t2,
             fx1a, fx1c, fx1g, fx1t, fx1r, fx1y, fx2r, fx2y;
    double   *zztable, *zvtable,
            *zzptr, *zvptr;
    double  *rptr, *wrptr, *wr2ptr;
    int      cat, *cptr, i, curvatOK;


    while ((! p->x) || (! q->x)) {
      if (! (p->x)) if (! newviewHKY85CAT(tr, p)) return badZ;
      if (! (q->x)) if (! newviewHKY85CAT(tr, q)) return badZ;
      }

    lp = p->x->lv;
    lq = q->x->lv;

    { 
      unsigned scratch_size;
      scratch_size = sizeof(double) * tr->cdta->endsite;
      abi   = (double *) malloc(scratch_size);
      bci   = (double *) malloc(scratch_size);
      sumci = (double *) malloc(scratch_size);      
    }
    
    zztable = (double *)malloc(tr->NumberOfCategories * sizeof(double));
    zvtable = (double *)malloc(tr->NumberOfCategories * sizeof(double));

    abptr   = abi;
    bcptr   = bci;
    sumcptr = sumci;
    


    for (i = 0; i < tr->cdta->endsite; i++) {
      fx1a = tr->rdta->freqa * lp->a;
      fx1g = tr->rdta->freqg * lp->g;
      fx1c = tr->rdta->freqc * lp->c;
      fx1t = tr->rdta->freqt * lp->t;
      suma = fx1a * lq->a + fx1c * lq->c + fx1g * lq->g + fx1t * lq->t;
      fx2r = tr->rdta->freqa * lq->a + tr->rdta->freqg * lq->g;
      fx2y = tr->rdta->freqc * lq->c + tr->rdta->freqt * lq->t;
      fx1r = fx1a + fx1g;
      fx1y = fx1c + fx1t;
      *sumcptr++ = sumc = (fx1r + fx1y) * (fx2r + fx2y);
      sumb       = fx1r * fx2r * tr->rdta->invfreqr + fx1y * fx2y * tr->rdta->invfreqy;
      *abptr++   = suma - sumb;
      *bcptr++   = sumb - sumc;
      lp++;
      lq++;
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
        rptr  = tr->cdta->patrat;
	cptr  = tr->cdta->rateCategory;
        zzptr = zztable;
        zvptr = zvtable;



        for (i = 0; i < tr->NumberOfCategories; i++) {
          ki = *rptr++;
          *zzptr++ = ki *   lz;
          *zvptr++ = ki * xvlz;
          }
	zzptr = zztable;
        zvptr = zvtable;
	vdExp (tr->NumberOfCategories, zzptr, zzptr);
	vdExp (tr->NumberOfCategories, zvptr, zvptr);

        abptr   = abi;
        bcptr   = bci;
        sumcptr = sumci;        
        wrptr   = &(tr->cdta->wr[0]);
        wr2ptr  = &(tr->cdta->wr2[0]);
        dlnLdlz = 0.0;                 /*  = d(ln(likelihood))/d(lz) */
        d2lnLdlz2 = 0.0;               /*  = d2(ln(likelihood))/d(lz)2 */



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

    free(abi);
    free(bci);
    free(sumci);
    free(zztable);
    free(zvtable); 

/* printf("makenewz: %le\n", z); */

    return  z;
  } /* makenewz */


boolean newviewGTRCAT(tree    *tr, nodeptr  p)
{
  
  if (p->tip) 
    {      
      if (p->x) return TRUE;
      if (! getxtip(p)) return FALSE;  
      {
	int code, i, j, jj;
	double (*EIGV)[4] = &(tr->rdta->EIGV[0][0]);
	likelivector *x1 = p->x->lv;
     
	for(i = 0; i < tr->cdta->endsite; i++)
	  {	      	  	 
	    code = p->tip[i];
	    x1->a = 0;
	    x1->c = 0;
	    x1->g = 0;
	    x1->t = 0;
	    for (j = 0; j < 4; j++) 
	      {
		if ((code >> j) & 1) 
		  {
		    int jj = "0213"[j] - '0';		
		    x1->a += EIGV[jj][0];
		    x1->c += EIGV[jj][1];
		    x1->g += EIGV[jj][2];
		    x1->t += EIGV[jj][3];
		  }
	      }		
	      
	    x1->exp = 0;	  			  			 
	    x1++;	 
	} 
      return TRUE; 
      }
    }
  
  { 	
    double  *diagptable_x1, *diagptable_x2, *diagptable_x1_start, *diagptable_x2_start;
    double *EIGN    = &(tr->rdta->EIGN[0][0]);
    double *invfreq = &(tr->rdta->invfreq[0][0]);
    double (*EIGV)[4] = &(tr->rdta->EIGV[0][0]);
    xtype tmp_x1_1, tmp_x1_2, tmp_x1_3,
      tmp_x2_1, tmp_x2_2, tmp_x2_3, 
      ump_x1_1, ump_x1_2, ump_x1_3, ump_x1_0, 
      ump_x2_0, ump_x2_1, ump_x2_2, ump_x2_3, x1px2;
    int cat;
    int  *cptr;	  
    int  i, j;
    likelivector   *x1, *x2, *x3;
    double   z1, lz1, z2, lz2, ki;
    nodeptr  q, r;   
    double *eigv, *rptr;

    rptr   = &(tr->cdta->patrat[0]); 
    cptr  = &(tr->cdta->rateCategory[0]);

    q = p->next->back;
    r = p->next->next->back;
      
    while ((! p->x) || (! q->x) || (! r->x)) {
      if (! q->x) if (! newviewGTRCAT(tr, q)) return FALSE;
      if (! r->x) if (! newviewGTRCAT(tr, r)) return FALSE;
      if (! p->x) if (! getxnode(p)) return FALSE;	
    }

    x1  = q->x->lv;
    z1  = q->z;
    lz1 = (z1 > zmin) ? log(z1) : log(zmin);

    x2  = r->x->lv;
    z2  = r->z;
    lz2 = (z2 > zmin) ? log(z2) : log(zmin);

    x3  = p->x->lv;   
   
    
    diagptable_x1 = diagptable_x1_start = (double *)malloc(sizeof(double) * tr->NumberOfCategories * 3);
    diagptable_x2 = diagptable_x2_start = (double *)malloc(sizeof(double) * tr->NumberOfCategories * 3);
    
    
    for(i = 0; i < tr->NumberOfCategories;i++)
      {
	ki = *rptr++;
	
	*diagptable_x1++ = EIGN[1] * lz1 * ki;
	*diagptable_x1++ = EIGN[2] * lz1 * ki;
	*diagptable_x1++ = EIGN[3] * lz1 * ki;
	*diagptable_x2++ = EIGN[1] * lz2 * ki;
	*diagptable_x2++ = EIGN[2] * lz2 * ki;
	*diagptable_x2++ = EIGN[3] * lz2 * ki;		
      }
    
    vdExp (tr->NumberOfCategories * 3, diagptable_x1_start, diagptable_x1_start);
    vdExp (tr->NumberOfCategories * 3, diagptable_x2_start, diagptable_x2_start);

    
    for (i = 0; i < tr->cdta->endsite; i++) 
      {	
	cat = *cptr++;

	diagptable_x1 = &diagptable_x1_start[cat * 3];
	diagptable_x2 = &diagptable_x2_start[cat * 3];

	tmp_x1_1 = x1->c * *diagptable_x1++;
	tmp_x1_2 = x1->g * *diagptable_x1++;
	tmp_x1_3 = x1->t * *diagptable_x1++;
	
	ump_x1_0 = tmp_x1_1 * EIGV[0][1];
	ump_x1_0 += tmp_x1_2 * EIGV[0][2];
	ump_x1_0 += tmp_x1_3 * EIGV[0][3];	      	
	ump_x1_0 *= invfreq[0];
	ump_x1_0 += x1->a;
	
	ump_x1_1 = tmp_x1_1 * EIGV[1][1];
	ump_x1_1 += tmp_x1_2 * EIGV[1][2];
	ump_x1_1 += tmp_x1_3 * EIGV[1][3];	      	
	ump_x1_1 *= invfreq[1];
	ump_x1_1 += x1->a;
	    
	ump_x1_2 = tmp_x1_1 * EIGV[2][1];
	ump_x1_2 += tmp_x1_2 * EIGV[2][2];
	ump_x1_2 += tmp_x1_3 * EIGV[2][3];	      	
	ump_x1_2 *= invfreq[2];
	ump_x1_2 += x1->a;

	ump_x1_3 = tmp_x1_1 * EIGV[3][1];
	ump_x1_3 += tmp_x1_2 * EIGV[3][2];
	ump_x1_3 += tmp_x1_3 * EIGV[3][3];	      	
	ump_x1_3 *= invfreq[3];
	ump_x1_3 += x1->a;
	
	tmp_x2_1 = x2->c * *diagptable_x2++;
	tmp_x2_2 = x2->g * *diagptable_x2++;
	tmp_x2_3 = x2->t * *diagptable_x2++;
	 
	 	    	    	     	     
	ump_x2_0 = tmp_x2_1 * EIGV[0][1];
	ump_x2_0 += tmp_x2_2 * EIGV[0][2];
	ump_x2_0 += tmp_x2_3 * EIGV[0][3];		     
	ump_x2_0 *= invfreq[0];
	ump_x2_0 += x2->a;
	  
	ump_x2_1 = tmp_x2_1 * EIGV[1][1];
	ump_x2_1 += tmp_x2_2 * EIGV[1][2];
	ump_x2_1 += tmp_x2_3 * EIGV[1][3];		     
	ump_x2_1 *= invfreq[1];
	ump_x2_1 += x2->a;	 

	ump_x2_2 = tmp_x2_1 * EIGV[2][1];
	ump_x2_2 += tmp_x2_2 * EIGV[2][2];
	ump_x2_2 += tmp_x2_3 * EIGV[2][3];		     
	ump_x2_2 *= invfreq[2];
	ump_x2_2 += x2->a;	  
		   

	ump_x2_3 = tmp_x2_1 * EIGV[3][1] ;
	ump_x2_3 += tmp_x2_2 * EIGV[3][2];
	ump_x2_3 += tmp_x2_3 * EIGV[3][3];		     
	ump_x2_3 *= invfreq[3];
	ump_x2_3 += x2->a;	    	  		   	  
	eigv = &EIGV[0][0];
	
	x1px2 = ump_x1_0 * ump_x2_0;
	x3->a = x1px2 *  *eigv++;/*EIGV[0][0];*/
	x3->c = x1px2 *  *eigv++;/*EIGV[0][1];*/
	x3->g = x1px2 *  *eigv++;/*EIGV[0][2];*/
	x3->t = x1px2 *  *eigv++;/*EIGV[0][3]; */
	
	x1px2 = ump_x1_1 * ump_x2_1;
	x3->a += x1px2  *  *eigv++;/*EIGV[1][0];*/
	x3->c += x1px2 *  *eigv++;/*EIGV[1][1];*/
	x3->g += x1px2 *  *eigv++;/*EIGV[1][2];*/
	x3->t += x1px2 *  *eigv++;/*EIGV[1][3];*/
	
	x1px2 = ump_x1_2 * ump_x2_2;
	x3->a += x1px2 *  *eigv++;/*EIGV[2][0];*/
	x3->c += x1px2*  *eigv++;/*EIGV[2][1];*/
	x3->g += x1px2 *  *eigv++;/*EIGV[2][2];*/
	x3->t += x1px2 *  *eigv++;/*EIGV[2][3];*/
	
	x1px2 = ump_x1_3 * ump_x2_3;
	x3->a += x1px2 *   *eigv++;/*EIGV[3][0];*/
	x3->c += x1px2 *   *eigv++;/*EIGV[3][1];*/
	x3->g += x1px2 *   *eigv++;/*EIGV[3][2];*/
	x3->t += x1px2 *   *eigv++;/*EIGV[3][3];*/
		   
	x3->exp = x1->exp + x2->exp;		  
	
	if (ABS(x3->a) < minlikelihood && ABS(x3->g) < minlikelihood
	    && ABS(x3->c) < minlikelihood && ABS(x3->t) < minlikelihood) 
	  {	     
	    x3->a   *= twotothe256;
	    x3->g   *= twotothe256;
	    x3->c   *= twotothe256;
	    x3->t   *= twotothe256;
	    x3->exp += 1;
	  }
	 	
	x1++;
	x2++;
	x3++;
      
      }
         
    free(diagptable_x1_start); 
    free(diagptable_x2_start);
  }
}





double evaluateGTRCAT (tr, p)
    tree    *tr;
    nodeptr  p;
  { /* evaluate */
    double   sum, z, lz, term, ki;    
    nodeptr  q;
    int     i, j, cat;
    int     *wptr = tr->cdta->aliaswgt, *cptr;
    
    double  *diagptable, *rptr, *diagptable_start;
    double *EIGN = &(tr->rdta->EIGN[0][0]);
    likelivector   *x1, *x2;
       
    q = p->back;
    while ((! p->x) || (! q->x)) {
      if (! (p->x)) if (! newviewGTRCAT(tr, p)) return badEval;
      if (! (q->x)) if (! newviewGTRCAT(tr, q)) return badEval;
    }


    rptr   = &(tr->cdta->patrat[0]); 
    cptr  = &(tr->cdta->rateCategory[0]);

    x1  = p->x->lv;
    x2  = q->x->lv;
    z = p->z;
   
    if (z < zmin) z = zmin;
    lz = log(z);

   
    
    diagptable = diagptable_start = (double *)malloc(sizeof(double) * tr->NumberOfCategories * 3);
    for(i = 0; i <  tr->NumberOfCategories;i++)
      {
	ki = *rptr++;	 
	*diagptable++ = EIGN[1] * lz * ki;
	*diagptable++ = EIGN[2] * lz * ki;
	*diagptable++ = EIGN[3] * lz * ki;
      }
	
    vdExp (tr->NumberOfCategories * 3, diagptable_start, diagptable_start);

    sum = 0.0;
  
    for (i = 0; i < tr->cdta->endsite; i++) 
      {	  	 	  
	cat = *cptr++;
	diagptable = &diagptable_start[3 * cat];

	term =  x1->a * x2->a;
	term += x1->c * x2->c * *diagptable++;
	term += x1->g * x2->g * *diagptable++;
	term += x1->t * x2->t * *diagptable++;     
	term = log(term) + (x1->exp + x2->exp)*log(minlikelihood);
	sum += *wptr++ * term;		  
	x1++;
	x2++;
      }
    
      
    free(diagptable_start); 
    
    tr->likelihood = sum;
    return  sum;
  } /* evaluate */



double makenewzGTRCAT (tr, p, q, z0, maxiter)
    tree    *tr;
    nodeptr  p, q;
    double   z0;
    int  maxiter;
  { /* makenewz */
    double   z, zprev, zstep;
    likelivector  *x1, *x2;
    double  *sumtable, *sum;
    int     i, *cptr, cat;
    double  dlnLdlz = 0.0;                 /*  = d(ln(likelihood))/d(lz) */
    double  d2lnLdlz2 = 0.0;               /*  = d2(ln(likelihood))/d(lz)2 */    
    double *d1, *d2, *d3, *rptr;
    double ki;
    
    while ((! p->x) || (! q->x)) {
      if (! (p->x)) if (! newviewGTRCAT(tr, p)) return badZ;
      if (! (q->x)) if (! newviewGTRCAT(tr, q)) return badZ;
    }

    x1 = p->x->lv;
    x2 = q->x->lv;

    sum = sumtable = (double *)malloc(4 * tr->cdta->endsite * 
				      sizeof(double));
    d1 = (double *)malloc(tr->NumberOfCategories * sizeof(double));
    d2 = (double *)malloc(tr->NumberOfCategories * sizeof(double));
    d3 = (double *)malloc(tr->NumberOfCategories * sizeof(double));
 

    for (i = 0; i < tr->cdta->endsite; i++) 
      {     
	*sum++ = x1->a * x2->a;
	*sum++ = x1->c * x2->c;
	*sum++ = x1->g * x2->g;
	*sum++ = x1->t * x2->t;		    
	x1++;
	x2++;		 
      }

   
   
    z = z0;
    do {
      int curvatOK = FALSE;

      zprev = z;
      
      zstep = (1.0 - zmax) * z + zmin;

      do {	
	double *EIGN    = &(tr->rdta->EIGN[0][0]);
	double lz;
        double *wrptr   = &(tr->cdta->wr[0]);
        double *wr2ptr  = &(tr->cdta->wr2[0]);

        dlnLdlz = 0.0;                 /*  = d(ln(likelihood))/d(lz) */
        d2lnLdlz2 = 0.0;               /*  = d2(ln(likelihood))/d(lz)2 */

        if (z < zmin) z = zmin;
        else if (z > zmax) z = zmax;
        lz    = log(z);
        
	rptr   = &(tr->cdta->patrat[0]); 
	cptr  = &(tr->cdta->rateCategory[0]);
	sum = sumtable;
	
	for(i = 0; i < tr->NumberOfCategories; i++)
	  {
	    ki = *rptr++;
	    d1[i] = EIGN[1] * lz * ki; 
	    d2[i] = EIGN[2] * lz * ki; 
	    d3[i] = EIGN[3] * lz * ki; 
	  }
	vdExp (tr->NumberOfCategories, d1, d1);
	vdExp (tr->NumberOfCategories, d2, d2);
	vdExp (tr->NumberOfCategories, d3, d3);
	


        for (i = 0; i < tr->cdta->endsite; i++) 
	  {
	    double tmp_0, tmp_1, tmp_2, tmp_3;
	    double t, inv_Li, dlnLidlz = 0, d2lnLidlz2 = 0;
	    	
	    cat = *cptr++;
	    inv_Li = (tmp_0 = *sum++);
	    inv_Li += (tmp_1 = *sum++ * d1[cat]);
	    inv_Li += (tmp_2 = *sum++ * d2[cat]);
	    inv_Li += (tmp_3 = *sum++ * d3[cat]);

	    inv_Li = 1.0/inv_Li;
	    
	  
	    t = tmp_1 * EIGN[1];
	    dlnLidlz   += t;
	    d2lnLidlz2 += t * EIGN[1];
	    
	    t = tmp_2 * EIGN[2];
	    dlnLidlz   += t;
	    d2lnLidlz2 += t * EIGN[2];
	    
	    t = tmp_3 * EIGN[3];
	    dlnLidlz   += t;
	    d2lnLidlz2 += t * EIGN[3];
	    
	    dlnLidlz   *= inv_Li;
	    d2lnLidlz2 *= inv_Li;
	    dlnLdlz   += *wrptr++  * dlnLidlz;
	    d2lnLdlz2 += *wr2ptr++ * (d2lnLidlz2 - dlnLidlz * dlnLidlz);
	  }
	
        if ((d2lnLdlz2 >= 0.0) && (z < zmax))
          zprev = z = 0.37 * z + 0.63;  /*  Bad curvature, shorten branch */
        else
          curvatOK = TRUE;
	
      } while (! curvatOK);

      if (d2lnLdlz2 < 0.0) {
	double tantmp = -dlnLdlz / d2lnLdlz2;  /* prevent overflow */
	if (tantmp < 100) {
	  z *= exp(tantmp);
	  if (z < zmin) z = zmin;
	  if (z > 0.25 * zprev + 0.75)    /*  Limit steps toward z = 1.0 */
	    z = 0.25 * zprev + 0.75;
	} else {
	  z = 0.25 * zprev + 0.75;
	}
      }
      if (z > zmax) z = zmax;

    } while ((--maxiter > 0) && (ABS(z - zprev) > zstep));

    free (sumtable);
    free(d1);
    free(d2);
    free(d3);
    return  z;
  } /* makenewz */




#endif












boolean newviewPartialHKY85CAT(tree *tr, nodeptr p, int i, double ki) 
{ 
  double   zq, lzq, xvlzq, zr, lzr, xvlzr;
  nodeptr  q, r;
  likelivector *lp, *lq, *lr;
  
  if (p->tip) {             /*  Make sure that data are at tip */
    likelivector *l;
    int           code;
    yType        *yptr;
    
    if (p->x) return TRUE;  /*  They are already there */
    
    if (! getxtip(p)) return FALSE; /*  They are not, so get memory */
    
    l = &p->x->lv[i];           /*  Pointer to first likelihood vector value */
      yptr = p->tip;          /*  Pointer to first nucleotide datum */
      code = yptr[i];
      l->a =  code       & 1;
      l->c = (code >> 1) & 1;
      l->g = (code >> 2) & 1;
      l->t = (code >> 3) & 1;
      l->exp = 0;
      return TRUE;
  }
  
  /*  Internal node needs update */
  
  q = p->next->back;
  r = p->next->next->back;
  
  
  
  while ((! p->x) || (! q->x) || (! r->x)) {
    if (! q->x) if (! newviewPartialHKY85CAT(tr, q, i, ki))  return FALSE;
    if (! r->x) if (! newviewPartialHKY85CAT(tr, r, i, ki))  return FALSE;
    /*if (! p->x) if (! getxnode(p)) return FALSE;*/
  }
  
  lp = &p->x->lv[i];
  
  lq = &q->x->lv[i];
  zq = q->z;
  lzq = (zq > zmin) ? log(zq) : log(zmin);
  xvlzq = tr->rdta->xv * lzq;
    
  lr = &r->x->lv[i];
  zr = r->z;
  lzr = (zr > zmin) ? log(zr) : log(zmin);
  xvlzr = tr->rdta->xv * lzr;
  
  { 
    double  *rptr;
    double  fxqr, fxqy, fxqn, sumaq, sumgq, sumcq, sumtq,
      fxrr, fxry, fxrn, tempi, tempj;
    double zzq, zvq, zzr, zvr;

    zzq = exp(ki *   lzq);
    zvq = exp(ki * xvlzq);
    zzr = exp(ki *   lzr);
    zvr = exp(ki * xvlzr);
        
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
    lp->exp = lq->exp + lr->exp;
	
	
	
    if (lp->a < minlikelihood && lp->g < minlikelihood
	&& lp->c < minlikelihood && lp->t < minlikelihood) {
      lp->a   *= twotothe256;
      lp->g   *= twotothe256;
      lp->c   *= twotothe256;
      lp->t   *= twotothe256;
      lp->exp += 1;
    }
    
    return TRUE;
      }
  } /* newview */



double evaluatePartialHKY85CAT (tree *tr, nodeptr p, int i, double ki)
  { /* evaluate */
    double   sum, z, lz, xvlz,
             fxpa, fxpc, fxpg, fxpt, fxpr, fxpy, fxqr, fxqy,
             suma, sumb, sumc, term;

    double partial;
    double   zz, zv;

    double        *rptr;
    likelivector *lp, *lq;
    nodeptr       q;
    int           *wptr;

    q = p->back;
    while ((! p->x) || (! q->x)) {
      if (! (p->x)) if (! newviewPartialHKY85CAT(tr, p, i, ki)) return badEval;
       if (! (q->x)) if (! newviewPartialHKY85CAT(tr, q, i, ki)) return badEval;
      }

    lp = &p->x->lv[i];
    lq = &q->x->lv[i];

    z = p->z;
    if (z < zmin) z = zmin;
    lz = log(z);
    xvlz = tr->rdta->xv * lz;

    rptr  = &(tr->cdta->patrat[0]);    
    wptr = &(tr->cdta->aliaswgt[0]);

    zz   = exp(ki *   lz);
    zv   = exp(ki * xvlz);

    fxpa = tr->rdta->freqa * lp->a;
    fxpg = tr->rdta->freqg * lp->g;
    fxpc = tr->rdta->freqc * lp->c;
    fxpt = tr->rdta->freqt * lp->t;
    suma = fxpa * lq->a + fxpc * lq->c + fxpg * lq->g + fxpt * lq->t;
    fxqr = tr->rdta->freqa * lq->a + tr->rdta->freqg * lq->g;
    fxqy = tr->rdta->freqc * lq->c + tr->rdta->freqt * lq->t;
    fxpr = fxpa + fxpg;
    fxpy = fxpc + fxpt;
    sumc = (fxpr + fxpy) * (fxqr + fxqy);
    sumb = fxpr * fxqr * tr->rdta->invfreqr + fxpy * fxqy * tr->rdta->invfreqy;
    suma -= sumb;
    sumb -= sumc;
    term = log(zz * suma + zv * sumb + sumc) + (lp->exp + lq->exp)*log(minlikelihood);

    partial = wptr[i] * term;
    return partial;

  } /* evaluate */



boolean newviewPartialGTRCAT(tree *tr, nodeptr p, int i, double ki) 
{
 if (p->tip) 
    {      
      if (p->x) return TRUE;

      if (! getxtip(p)) return FALSE; 
      {
	int code, j, jj;
	double (*EIGV)[4] = &(tr->rdta->EIGV[0][0]);
	likelivector *x1 =&(p->x->lv[i]);
     
	code = p->tip[i];
	x1->a = 0;
	x1->c = 0;
	x1->g = 0;
	x1->t = 0;
	for (j = 0; j < 4; j++) 
	  {
	    if ((code >> j) & 1) 
	      {
		int jj = "0213"[j] - '0';		
		x1->a += EIGV[jj][0];
		x1->c += EIGV[jj][1];
		x1->g += EIGV[jj][2];
		x1->t += EIGV[jj][3];
	      }
	  }		
	      
	x1->exp = 0;	  			  			 
		 
	return TRUE; 
	}
      return TRUE; 
    }
  
  { 	
    double  *diagptable_x1, *diagptable_x2;
    double *EIGN    = &(tr->rdta->EIGN[0][0]);
    double *invfreq = &(tr->rdta->invfreq[0][0]);
    double (*EIGV)[4] = &(tr->rdta->EIGV[0][0]);
    xtype tmp_x1_1, tmp_x1_2, tmp_x1_3,
      tmp_x2_1, tmp_x2_2, tmp_x2_3, 
      ump_x1_1, ump_x1_2, ump_x1_3, ump_x1_0, 
      ump_x2_0, ump_x2_1, ump_x2_2, ump_x2_3, x1px2;
    int cat;
    int  *cptr;	  
    int  j;
    likelivector   *x1, *x2, *x3;
    double   z1, lz1, z2, lz2;
    nodeptr  q, r;   
    double *eigv, *rptr;

    q = p->next->back;
    r = p->next->next->back;
      
    while ((! p->x) || (! q->x) || (! r->x)) {
      if (! q->x) if (! newviewPartialGTRCAT(tr, q, i, ki)) return FALSE;
      if (! r->x) if (! newviewPartialGTRCAT(tr, r, i, ki)) return FALSE;
      if (! p->x) if (! getxnode(p)) return FALSE;
    }

    x1  = &(q->x->lv[i]);
    z1  = q->z;
    lz1 = (z1 > zmin) ? log(z1) : log(zmin);

    x2  = &(r->x->lv[i]);
    z2  = r->z;
    lz2 = (z2 > zmin) ? log(z2) : log(zmin);

    x3  = &(p->x->lv[i]);   
    	
    diagptable_x1 = (double *)malloc(sizeof(double) * 4);
    diagptable_x2 = (double *)malloc(sizeof(double) * 4);

    for (j = 1; j < 4; j++) 
      {	    
	diagptable_x1[j] = exp (EIGN[j] * lz1 * ki);
	diagptable_x2[j] = exp (EIGN[j] * lz2 * ki);	    
      }	       
      
    
    

    tmp_x1_1 = x1->c * diagptable_x1[1];
    tmp_x1_2 = x1->g * diagptable_x1[2];
    tmp_x1_3 = x1->t * diagptable_x1[3];
	
    ump_x1_0 = tmp_x1_1 * EIGV[0][1];
    ump_x1_0 += tmp_x1_2 * EIGV[0][2];
    ump_x1_0 += tmp_x1_3 * EIGV[0][3];	      	
    ump_x1_0 *= invfreq[0];
    ump_x1_0 += x1->a;
	
    ump_x1_1 = tmp_x1_1 * EIGV[1][1];
    ump_x1_1 += tmp_x1_2 * EIGV[1][2];
    ump_x1_1 += tmp_x1_3 * EIGV[1][3];	      	
    ump_x1_1 *= invfreq[1];
    ump_x1_1 += x1->a;
	    
    ump_x1_2 = tmp_x1_1 * EIGV[2][1];
    ump_x1_2 += tmp_x1_2 * EIGV[2][2];
    ump_x1_2 += tmp_x1_3 * EIGV[2][3];	      	
    ump_x1_2 *= invfreq[2];
    ump_x1_2 += x1->a;
    
    ump_x1_3 = tmp_x1_1 * EIGV[3][1];
    ump_x1_3 += tmp_x1_2 * EIGV[3][2];
    ump_x1_3 += tmp_x1_3 * EIGV[3][3];	      	
    ump_x1_3 *= invfreq[3];
    ump_x1_3 += x1->a;
	
    tmp_x2_1 = x2->c * diagptable_x2[1];
    tmp_x2_2 = x2->g * diagptable_x2[2];
    tmp_x2_3 = x2->t * diagptable_x2[3];
    
	 	    	    	     	     
    ump_x2_0 = tmp_x2_1 * EIGV[0][1];
    ump_x2_0 += tmp_x2_2 * EIGV[0][2];
    ump_x2_0 += tmp_x2_3 * EIGV[0][3];		     
    ump_x2_0 *= invfreq[0];
    ump_x2_0 += x2->a;
    
    ump_x2_1 = tmp_x2_1 * EIGV[1][1];
    ump_x2_1 += tmp_x2_2 * EIGV[1][2];
    ump_x2_1 += tmp_x2_3 * EIGV[1][3];		     
    ump_x2_1 *= invfreq[1];
    ump_x2_1 += x2->a;	 

    ump_x2_2 = tmp_x2_1 * EIGV[2][1];
    ump_x2_2 += tmp_x2_2 * EIGV[2][2];
    ump_x2_2 += tmp_x2_3 * EIGV[2][3];		     
    ump_x2_2 *= invfreq[2];
    ump_x2_2 += x2->a;	  
		   

    ump_x2_3 = tmp_x2_1 * EIGV[3][1] ;
    ump_x2_3 += tmp_x2_2 * EIGV[3][2];
    ump_x2_3 += tmp_x2_3 * EIGV[3][3];		     
    ump_x2_3 *= invfreq[3];
    ump_x2_3 += x2->a;	    	  		   	  
    eigv = &EIGV[0][0];
	
    x1px2 = ump_x1_0 * ump_x2_0;
    x3->a = x1px2 *  *eigv++;/*EIGV[0][0];*/
    x3->c = x1px2 *  *eigv++;/*EIGV[0][1];*/
    x3->g = x1px2 *  *eigv++;/*EIGV[0][2];*/
    x3->t = x1px2 *  *eigv++;/*EIGV[0][3]; */
    
    x1px2 = ump_x1_1 * ump_x2_1;
    x3->a += x1px2  *  *eigv++;/*EIGV[1][0];*/
    x3->c += x1px2 *  *eigv++;/*EIGV[1][1];*/
    x3->g += x1px2 *  *eigv++;/*EIGV[1][2];*/
    x3->t += x1px2 *  *eigv++;/*EIGV[1][3];*/
	
    x1px2 = ump_x1_2 * ump_x2_2;
    x3->a += x1px2 *  *eigv++;/*EIGV[2][0];*/
    x3->c += x1px2*  *eigv++;/*EIGV[2][1];*/
    x3->g += x1px2 *  *eigv++;/*EIGV[2][2];*/
    x3->t += x1px2 *  *eigv++;/*EIGV[2][3];*/
	
    x1px2 = ump_x1_3 * ump_x2_3;
    x3->a += x1px2 *   *eigv++;/*EIGV[3][0];*/
    x3->c += x1px2 *   *eigv++;/*EIGV[3][1];*/
    x3->g += x1px2 *   *eigv++;/*EIGV[3][2];*/
    x3->t += x1px2 *   *eigv++;/*EIGV[3][3];*/
		   
    x3->exp = x1->exp + x2->exp;		  
    
    if (ABS(x3->a) < minlikelihood && ABS(x3->g) < minlikelihood
	&& ABS(x3->c) < minlikelihood && ABS(x3->t) < minlikelihood) 
      {	     
	x3->a   *= twotothe256;
	x3->g   *= twotothe256;
	x3->c   *= twotothe256;
	x3->t   *= twotothe256;
	x3->exp += 1;
      }
	           
    free(diagptable_x1); 
    free(diagptable_x2);

    return TRUE;

  } 
}


double evaluatePartialGTRCAT (tree *tr, nodeptr p, int i, double ki)
{
  double   sum, z, lz, term, k;    
    nodeptr  q;
    int     j, cat;
    int     *wptr = tr->cdta->aliaswgt, *cptr;
    
    double  diagptable[4], *rptr;
    double *EIGN = &(tr->rdta->EIGN[0][0]);
    likelivector   *x1, *x2;
       
    q = p->back;
    while ((! p->x) || (! q->x)) 
      {
	if (! (p->x)) if (! newviewPartialGTRCAT(tr, p, i, ki)) return badEval;
	if (! (q->x)) if (! newviewPartialGTRCAT(tr, q, i, ki)) return badEval;
      }

    rptr   = &(tr->cdta->patrat[0]); 
    cptr  = &(tr->cdta->rateCategory[0]);

    x1  = &(p->x->lv[i]);
    x2  = &(q->x->lv[i]);
    z = p->z;
   
    if (z < zmin) z = zmin;
    lz = log(z);

    diagptable[0] = 1;
    diagptable[1] = exp (EIGN[1] * lz * ki);
    diagptable[2] = exp (EIGN[2] * lz * ki);
    diagptable[3] = exp (EIGN[3] * lz * ki);
    
   
	
    
    sum = 0.0;
  
    term =  x1->a * x2->a * diagptable[0];
    term += x1->c * x2->c * diagptable[1];
    term += x1->g * x2->g * diagptable[2];
    term += x1->t * x2->t * diagptable[3];     
    term = log(term) + (x1->exp + x2->exp)*log(minlikelihood);
    sum = wptr[i] * term;		  
       
    tr->likelihood = sum;
    return  sum;
}




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
    if (! p->tip) 
      {                                  /*  Adjust descendants */
        q = p->next;
        while (q != p) 
	  {
	    if (! smooth(tr, q->back))   return FALSE;
	    q = q->next;
          }	
	if (! newview(tr, p)) return FALSE;	
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

    if(Thorough)
    { 
      double  zqr, zqs, zrs, lzqr, lzqs, lzrs, lzsum, lzq, lzr, lzs, lzmax;      
     
      if ((zqr = makenewz(tr, q, r, q->z, iterations)) == badZ) return FALSE; 
      
      if ((zqs = makenewz(tr, q, s, defaultz, iterations)) == badZ) return FALSE;      
       
      if ((zrs = makenewz(tr, r, s, defaultz, iterations)) == badZ) return FALSE;
      
      
      
  

      lzqr = (zqr > zmin) ? log(zqr) : log(zmin); 
      lzqs = (zqs > zmin) ? log(zqs) : log(zmin);
      lzrs = (zrs > zmin) ? log(zrs) : log(zmin);
      lzsum = 0.5 * (lzqr + lzqs + lzrs);

      lzq = lzsum - lzrs;
      lzr = lzsum - lzqs;
      lzs = lzsum - lzqr;
      lzmax = log(zmax);

      if      (lzq > lzmax) {lzq = lzmax; lzr = lzqr; lzs = lzqs;} 
      else if (lzr > lzmax) {lzr = lzmax; lzq = lzqr; lzs = lzrs;}
      else if (lzs > lzmax) {lzs = lzmax; lzq = lzqs; lzr = lzrs;}
      
     
      hookup(p->next,       q, exp(lzq));
      hookup(p->next->next, r, exp(lzr));
      hookup(p,             s, exp(lzs));       
    
      }
    else
      { 
	
	double  z;
	z = sqrt(q->z);

	hookup(p->next,       q, z);
	hookup(p->next->next, r, z);
      }

   
    
    if (! newview(tr, p)) return FALSE;
    
   
    tr->opt_level = 0;

    if(Thorough)
    {
      if (! localSmooth(tr, p, smoothings)) return FALSE;	
    }               
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
   

    
    if ((zqr = makenewz(tr, q, r, zqr, iterations)) == badZ) 
      return (node *) NULL;      

    

    hookup(q, r, zqr);

    p->next->next->back = p->next->back = (node *) NULL;
     
    return  q;
  } /* removeNode */


boolean initrav (tree *tr, nodeptr p)
  { /* initrav */
    nodeptr  q;

    if (! p->tip) {
      
      q = p->next;
      
      do 
	{
	  if (! initrav(tr, q->back))  return FALSE;	
	  q = q->next;	
        } while (q != p);
      
      
      if (! newview(tr, p)) return FALSE;
	      
      }

    return TRUE;
  } /* initrav */



boolean initravPartialHKY85CAT (tree *tr, nodeptr p, int i, double ki)
  { /* initrav */
    nodeptr  q;

    if (! p->tip) {
      q = p->next;
      do {
        if (! initravPartialHKY85CAT(tr, q->back, i, ki))  return FALSE;
        q = q->next;
        } while (q != p);

      if (! newviewPartialHKY85CAT(tr, p, i, ki)) return FALSE;

      }

    return TRUE;
  } /* initrav */

boolean initravPartialGTRCAT (tree *tr, nodeptr p, int i, double ki)
  { /* initrav */
    nodeptr  q;

    if (! p->tip) {
      q = p->next;
      do {
        if (! initravPartialGTRCAT(tr, q->back, i, ki))  return FALSE;
        q = q->next;
        } while (q != p);

      if (! newviewPartialGTRCAT(tr, p, i, ki)) return FALSE;

      }

    return TRUE;
  } /* initrav */




boolean readKeyValue (char *string, char *key, char *format, void *value)
  { /* readKeyValue */

    if (!(string = strstr(string, key)))  return FALSE;
    string += strlen(key);
    string = strchr(string, '=');
    if (! (string = strchr(string, '=')))  return FALSE;
    string++;
    return  sscanf(string, format, value);  /* 1 if read, otherwise 0 */
  } /* readKeyValue */

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

node * findAnyTip(nodeptr p)
  { /* findAnyTip */
    return  p->tip ? p : findAnyTip(p->next->back);
  } /* findAnyTip */


boolean testInsertParsimony (tree *tr, nodeptr p, nodeptr q)
  { /* testInsert */
    double  qz;
    nodeptr  r;
       
    r = q->back;             /* Save original connection */
    qz = q->z;
   
    if (! insert(tr, p, q, FALSE)) return FALSE;     
    if (evaluate(tr, p->next->next) == badEval) return FALSE;   
    
    if(tr->likelihood > tr->endLH)
      {
	/*printf("%f\n", tr->likelihood);*/
	tr->endLH = tr->likelihood;
      }

    saveBestTree(bt, tr);
    
    hookup(q, r, qz);
    p->next->next->back = p->next->back = (nodeptr) NULL;
    				            
    restoreZ(tr);           
    return TRUE;
  } /* testInsert */





void addTraverseRec(tree *tr, nodeptr p, nodeptr q,
                 int mintrav, int maxtrav)
{ /* addTraverse */

  
 
  if (--mintrav <= 0) 
    {              
      if (! testInsertParsimony(tr, p, q))  return;        
    }
  
  if ((! q->tip) && (--maxtrav > 0)) 
    {    
      addTraverseRec(tr, p, q->next->back,
		     mintrav, maxtrav);
      addTraverseRec(tr, p, q->next->next->back,
		     mintrav, maxtrav);    
  }

} 






int  rearrangeParsimony(tree *tr, nodeptr p, int mintrav, int maxtrav)
    /* rearranges the tree, globally or locally */
  { /* rearrange */
    double   p1z, p2z, q1z, q2z;
    nodeptr  p1, p2, q, q1, q2;
    int      mintrav2;
    int result = 0;
   
    if (maxtrav < 1 || mintrav > maxtrav)  return 0;
    

    if (! p->tip) {
     
      p1 = p->next->back;
      p2 = p->next->next->back;
      if (! p1->tip || ! p2->tip) 
	{
	  p1z = p1->z;
	  p2z = p2->z;
	  if (! removeNode(tr, p)) return badRear;
	  cacheZ(tr);
	  if (! p1->tip) 
	    {
	      addTraverseRec(tr, p, p1->next->back,
			     mintrav, maxtrav);         
	      addTraverseRec(tr, p, p1->next->next->back,
			     mintrav, maxtrav);          
	    }

	  if (! p2->tip) 
	    {
	      addTraverseRec(tr, p, p2->next->back,
			     mintrav, maxtrav);
	      addTraverseRec(tr, p, p2->next->next->back,
			     mintrav, maxtrav);          
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
          addTraverseRec(tr, q, q1->next->back,
                                  mintrav2 , maxtrav);
          addTraverseRec(tr, q, q1->next->next->back,
                                  mintrav2 , maxtrav);
         
          }

        if (! q2->tip) {
         addTraverseRec(tr, q, q2->next->back,
                                  mintrav2 , maxtrav);
          addTraverseRec(tr, q, q2->next->next->back,
                                  mintrav2 , maxtrav);
          
          }

        hookup(q->next,       q1, q1z);  /*  Restore original tree */
        hookup(q->next->next, q2, q2z);
	if (! (initrav(tr, tr->start)
	       && initrav(tr, tr->start->back))) return badRear;	  
        }
      }   /* if (! q->tip && maxtrav > 1) */

    return  1;
  } 


int optimizeTTinvocations = 1;
int optimizeRatesInvocations = 1;
int optimizeRateCategoryInvocations = 1;

void optimizeTT(tree *tr);

















void printTree(FILE *outf, tree *tr, int quoted)
{
  char *str = (char *)malloc(treeStringLength);
  char *outString;
  nodeptr startp;
  startp = tr->start;

  tr->start = tr->nodep[1];
  
  treeString(str, tr, tr->start->back, 1);
  
  tr->start = startp;

  outString = str;
  while(*outString++ != '(');
  fprintf(outf, "("); 
  if(!quoted)
  {
    while(*outString != '\0')
      {
	if(*outString != '\'')
	  fprintf(outf, "%c", *outString); 
	outString++;
      }
  }
      
  free(str);
  fflush(outf);
}

void printResult(tree *tr, int quoted)
{
  char *str = (char *)malloc(treeStringLength);
  char *outString;
  FILE *outf = fopen(resultFileName, "w");
  nodeptr startp;
  startp = tr->start;

  tr->start = tr->nodep[1];

  

  treeString(str, tr, tr->start->back, 1);  
  tr->start = startp; 
  outString = str;
  while(*outString++ != '(');
  fprintf(outf, "("); 
  if(!quoted)
  {
    while(*outString != '\0')
      {
	if(*outString != '\'')
	  fprintf(outf, "%c", *outString); 
	outString++;
      }
  }
      
  free(str);
  fclose(outf);  
}






void printLog(tree *tr)
{
  

  printf( "%f %f %d\n", gettime() - masterTime, tr->likelihood, checkPointCounter);
  fprintf(logFile,"%f %f %d\n", gettime() - masterTime, tr->likelihood, checkPointCounter); 
  fflush(logFile);
  if(checkpoints)
    {
      char thisCheckPointName[2048], str[16];
      FILE *cpf;
      strcpy(thisCheckPointName, checkpointFileName);
      sprintf(str, "%d", checkPointCounter);
      strcat(thisCheckPointName, str);
      checkPointCounter++;
      cpf = fopen(thisCheckPointName, "w");
      printTree(cpf, tr, 0);
      fflush(cpf);
      fclose(cpf);
    }
}


void treeOptimize(tree *tr, int _mintrav, int _maxtrav, analdef *adef)
{
  int i;
  int mintrav = _mintrav;
  int maxtrav = _maxtrav;
  int subsequent = 0;
  int initial = adef->initial;

  if (maxtrav > tr->ntips - 3)  
    maxtrav = tr->ntips - 3;
  
  tr->opt_level = 0;
  
  resetBestTree(bt);
  saveBestTree(bt, tr);

  checkTime(adef, tr);

  if(Multiple && !Thorough)
    {
      mintrav = 1;
      if(maxtrav < initial)
	maxtrav = 10;
    }

  /*printf("%d %d %d %d\n", mintrav, maxtrav, Multiple, Thorough);*/

  tr->startLH = tr->endLH = tr->likelihood;

  for(i = 1; i <= tr->mxtips + tr->mxtips - 2; i++)
    { 
    
      
      rearrangeParsimony(tr, tr->nodep[i], mintrav, maxtrav);
      if(Multiple && (tr->endLH > tr->startLH))                 	
	{	  
	  recallBestTree(bt, 1, tr);
	  /*printf("Tree improved %f\n", tr->likelihood);*/
	  tr->startLH = tr->endLH = tr->likelihood;
	  subsequent = 1;
	}	
    }
 
  
  Multiple = subsequent;
  return;     
}








/*===========  This is a problem if tr->start->back is a tip!  ===========*/
/*  All routines should be contrived so that tr->start->back is not a tip */

void initTreeString(char *treestr, tree *tr, int form)
{
   (void) sprintf(treestr, "[&&%s: version = '%s'",
                                 programName, programVersion);
   while (*treestr) treestr++;

   (void) sprintf(treestr, ", %s = %15.13g",
		  likelihood_key, unlikely);
   while (*treestr) treestr++;
   
   (void) sprintf(treestr, ", %s = %d", ntaxa_key, tr->mxtips);
   while (*treestr) treestr++;
   
   (void) sprintf(treestr,", %s = %d", opt_level_key, 0);
   while (*treestr) treestr++;
   
   (void) sprintf(treestr, ", %s = %d", smoothed_key, 0);
   while (*treestr) treestr++;
   
   (void) sprintf(treestr, "]%s", form == treeProlog ? ", " : " ");
   while (*treestr) treestr++;
}


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

    if (p == tr->start->back) 
      {
	(void) sprintf(treestr, ":0.0%s\n", (form != treeProlog) ? ";" : ").");
      }
    else {
      z = p->z;
      if (z < zmin) z = zmin;
      if(adef->model == M_GTR || adef->model == M_GTRCAT)
	x = -log(z) * tr->rdta->fracchangeav; 
      else
	x = -log(z) * tr->rdta->fracchange;
     
      (void) sprintf(treestr, ":%8.6f", x);  /* prolog needs the space */
      }

    while (*treestr) treestr++;     /* move pointer up to null termination */
    return  treestr;
  } /* treeString */







void treeOut (FILE *treefile, tree *tr, int form)
    /* write out file with representation of final tree */
  { /* treeOut */
    int    c;
    char  *cptr, *treestr;

    treestr = (char *) malloc((tr->ntips * (nmlngth+32)) + 256);
    if (! treestr) {
      printf( "treeOut: malloc failure\n");
      exit(1);
      }

    (void) treeString(treestr, tr, tr->start->back, form);
    cptr = treestr;
    while (c = *cptr++) putc(c, treefile);

    printf( "%s\n", treestr);
    printf("%s\n", treestr);

    free(treestr);
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
      /*
	else if (ch == '_') ch = ' ';*/  /* unquoted _ goes to space */

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


int treeFlushLen (FILE  *fp)
  { /* treeFlushLen */
    double  dummy;
    boolean res;
    int     ch;

    ch = treeGetCh(fp);
    
    if (ch == ':') 
      {
	ch = treeGetCh(fp);
	
	    ungetc(ch, fp);
	    if(!treeProcessLength(fp, & dummy)) return 0;
	    return 1;	  
      }
   
    

    if (ch != EOF) (void) ungetc(ch, fp);
    return 1;
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
    int      n, ch, fres;
    
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

  

 
    
    fres = treeFlushLen(fp);
    if(!fres) return FALSE;
    
    hookup(p, q, defaultz);
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
    hookup(q, r, defaultz);

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


static char buffer[] = "foobar";



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

int str_treeFinishCom (char **treestrp, char **strp)
                      /* treestrp -- tree string pointer */
                      /* strp -- comment string pointer */
  { /* str_treeFinishCom */
    int  ch;
    while ( (ch=*(*treestrp)++)!='\0' && ch!=']' ) {
      if (strp != NULL) *(*strp)++ = ch;    /* save character  */
      if (ch == '[') {                      /* nested comment; find its end */
        if ((ch=str_treeFinishCom(treestrp,strp)) == '\0')  break;
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
    while ((ch=*(*treestrp)++) != '\0') {
      if (whitechar(ch)) ;
      else if (ch == '[') {                  /* comment; find its end */
        if ((ch = str_treeFinishCom(treestrp,(char**)NULL)) == '\0')  break;
        }
      else  break;
      }

    return  ch;
  } /* str_treeGetCh */




boolean  str_treeGetLabel (char **treestrp, char *lblPtr, int maxlen)
  { /* str_treeGetLabel */
    int      ch;
    boolean  done, quoted, lblfound;


    if (--maxlen < 0)
      lblPtr = (char*)NULL;  /* reserves space for '\0' */
    else if(lblPtr == NULL)
      maxlen = 0;

    ch = *(*treestrp)++;
    done = treeLabelEnd(ch);

    lblfound = !done;
    quoted = (ch == '\'');
    if (quoted && ! done) {
      ch = *(*treestrp)++;
      done = (ch == '\0');
    }

    while(!done) {
      if (quoted) 
	{
	  if (ch == '\'') 
	    {
	      ch = *(*treestrp)++;
	      if (ch != '\'') break;
	    }
	}
      else 
	if (treeLabelEnd(ch))
	  break;
      /*else 
	  if (ch == '_')
	    ch = ' ';*/
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
    return  str_treeGetLabel(treestrp, (char*)NULL, (int)0);
  } /* str_treeFlushLabel */


int  str_treeFindTipName (char **treestrp, tree *tr)         /*DKB-orig*/
/*int  str_treeFindTipName (char **treestrp, tree *tr, int ch)*/   /*DKB-change*/
  { /* str_treeFindTipName */
    nodeptr  q;
    char    *nameptr, str[nmlngth+2];
    int      i, n;

    if (tr->prelabeled) {
      if (str_treeGetLabel(treestrp, str, nmlngth+2)) {
        n = treeFindTipByLabel(str, tr);
      }
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

    if(!str_treeGetCh(treestrp))  return FALSE;    /*  Skip comments */
    (*treestrp)--;

    if (sscanf(*treestrp, "%lf%n", dptr, &used) != 1) {
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
    double  x;

    if ((ch = str_treeGetCh(treestrp)) == ':')
    /*return str_treeProcessLength(treestrp, (double*)NULL);*/   /*DKB-orig*/
      return str_treeProcessLength(treestrp, &x);              /*DKB-change*/
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


boolean str_processTreeCom(tree *tr, char **treestrp)
  { /* str_processTreeCom */
    char  *com, *com_end;
    int  text_started, functor_read, com_open;

    com = *treestrp;

    /* Comment must begin with either "phylip_tree" or "pseudoNewick".
     * If it is neither, return FALSE. */
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
        return  FALSE;
      }
      else {
        com += functor_read;
      }
    }

    /* Find opening bracket of comment */
    com_open = 0;
    sscanf(com, " [%n", &com_open);
    com += com_open;

    /* Read comment. */
    if (com_open) {
      if (!(com_end = strchr(com, ']'))) {
        printf("Missing end of tree comment.\n");
        return  FALSE;
      }
      *com_end = 0;
      (void)readKeyValue(com,likelihood_key,"%lg",(void*)&(tr->likelihood));
      (void)readKeyValue(com,opt_level_key, "%d", (void*)&(tr->opt_level));
      (void)readKeyValue(com,smoothed_key,  "%d", (void*)&(tr->smoothed));
      *com_end = ']';
      com_end++;

      /* Remove trailing comma, and return addr of next char in treestp */
      if (functor_read) {
        text_started = 0;
        sscanf(com_end, " ,%n", & text_started);
        com_end += text_started;
      }
      *treestrp = com_end;
    }
    return (functor_read > 0);
  } /* str_processTreeCom */

boolean str_processTreeComMerge(int *ntaxa, char **treestrp)
  { /* str_processTreeCom */
    char  *com, *com_end;
    int  text_started, functor_read, com_open;

    com = *treestrp;

    /* Comment must begin with either "phylip_tree" or "pseudoNewick".
     * If it is neither, return FALSE. */
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
        return  FALSE;
      }
      else {
        com += functor_read;
      }
    }

    /* Find opening bracket of comment */
    com_open = 0;
    sscanf(com, " [%n", &com_open);
    com += com_open;

    /* Read comment. */
    if (com_open) {
      if (!(com_end = strchr(com, ']'))) {
        printf("Missing end of tree comment.\n");
        return  FALSE;
      }
      *com_end = 0;
      /*(void)readKeyValue(com,likelihood_key,"%lg",(void*)&(tr->likelihood));*/
      (void)readKeyValue(com,ntaxa_key, "%d", (void *)ntaxa);   
      /*      (void)readKeyValue(com,opt_level_key, "%d", (void*)&(tr->opt_level));
	      (void)readKeyValue(com,smoothed_key,  "%d", (void*)&(tr->smoothed));*/
      *com_end = ']';
      com_end++;

      /* Remove trailing comma, and return addr of next char in treestp */
      if (functor_read) {
        text_started = 0;
        sscanf(com_end, " ,%n", & text_started);
        com_end += text_started;
      }
      *treestrp = com_end;
    }
    return (functor_read > 0);
  } /* str_processTreeCom */


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
    /*if (! str_treeFlushLabel(treestrp))                      return FALSE;*//*DKB-orig*/
    }

    else {                           /*  A new tip */
    /*n = str_treeFindTipName(treestrp, tr, ch);*/ /*DKB-orig*/
      if(ch=='\'') (*treestrp)--;                  /*DKB-change*/
      n = str_treeFindTipName(treestrp, tr);       /*DKB-change*/
      if (n <= 0) return FALSE;
      q = tr->nodep[n];
      if (tr->start->number > n)  tr->start = q;
      (tr->ntips)++;
    }

    /*  Master and Slave always use lengths */

    if (! str_treeNeedCh(treestrp, ':', "in"))                 return FALSE;
    if (! str_treeProcessLength(treestrp, &branch))            return FALSE;
    if(adef->model == M_GTR || adef->model == M_GTRCAT)
      z = exp(-branch / tr->rdta->fracchangeav); 
    else
      z = exp(-branch / tr->rdta->fracchange);
    if (z > zmax)  z = zmax;
    hookup(p, q, z);

    return  TRUE;
  } /* str_addElementLen */




boolean str_treeReadLen (char *treestr, tree *tr)
    /* read string with representation of tree */
  { /* str_treeReadLen */
    nodeptr  p;
    int  i;
    boolean  is_fact, found;

    for(i=1; i<=(tr->mxtips); i++) tr->nodep[i]->back = (node*)NULL;
    tr->start       = tr->nodep[tr->mxtips];
    tr->ntips       = 0;
    tr->nextnode    = tr->mxtips + 1;
    tr->opt_level   = 0;
    tr->smoothed    = 1;/*(myprogtype==DNAML_MASTER); DANGER !!!!!!!!!*/
    tr->rooted      = FALSE;

    is_fact = str_processTreeCom(tr,&treestr);
    p = tr->nodep[(tr->nextnode)++];

    if(!str_treeNeedCh(&treestr, '(', "at start of"))       return FALSE;
    if(!str_addElementLen(&treestr, tr, p))                 return FALSE;
    if(!str_treeNeedCh(&treestr, ',', "in"))                return FALSE;
    if(!str_addElementLen(&treestr, tr, p->next))           return FALSE;
    if(!tr->rooted) {
      if(str_treeGetCh(&treestr) == ',') {        /*  An unrooted format */
        if(!str_addElementLen(&treestr,tr,p->next->next)) return FALSE;
      }
      else {                                       /*  A rooted format */
        p->next->next->back = (nodeptr) NULL;
        tr->rooted = TRUE;
        treestr--;
      }
    }
    if(!str_treeNeedCh(&treestr, ')', "in"))                 return FALSE;
  /*if(!str_treeFlushLabel(&treestr))                        return FALSE;*//*DKB-orig*/
    if(!str_treeFlushLen(&treestr))                          return FALSE;
    if(is_fact) {
      if(!str_treeNeedCh(& treestr, ')', "at end of"))       return FALSE;
      if(!str_treeNeedCh(& treestr, '.', "at end of"))       return FALSE;
    }
    else {
      if(!str_treeNeedCh(& treestr, ';', "at end of"))       return FALSE;
    }

    if(tr->rooted)  if (! uprootTree(tr, p->next->next))     return FALSE;
    tr->start = p->next->next->back;  /* This is start used by treeString */

    return  (initrav(tr,tr->start) && initrav(tr,tr->start->back));
  } /* str_treeReadLen */








boolean treeEvaluate (tree *tr, double smoothFactor)       /* Evaluate a user tree */
  { /* treeEvaluate */
    
   
    if (! smoothTree(tr, (int)((double)smoothings * smoothFactor))) 
      {
	return FALSE;      
      }
   
    if (evaluate(tr, tr->start) == badEval)  
      {
	return FALSE;
      }
     
    return TRUE;
  } /* treeEvaluate */



boolean evalFast (tree *tr)
  {
       
    if (! smoothTree(tr, 1)) 
      {
	return FALSE;      
      }
    if (evaluate(tr, tr->start) == badEval)  
      {
	return FALSE;
      }
     
    return TRUE;
  } /* treeEvaluate */

















double str_readTreeLikelihood (char *treestr)
  { /* str_readTreeLikelihood */
    double lk1;
    char    *com, *com_end;
    boolean  readKeyValue();

    if ((com = strchr(treestr, '[')) /*&& (com < strchr(treestr, '('))*/
                                     && (com_end = strchr(com, ']'))) 
      {
      com++;
      *com_end = 0;
      if (readKeyValue(com, likelihood_key, "%lg", (void *) &(lk1))) {
        *com_end = ']';
        return lk1;
        }
      }

    printf( "ERROR reading likelihood in receiveTree\n");
    printf("%s \n", treestr);
    return  badEval;
  } /* str_readTreeLikelihood */



















int randomInt(int n)
{
  return rand() %n;
}





void makePermutation(int *perm, int n)
{    
  int  i, j, k;
  int sum1 = 0, sum2 = 0;

  
  srand((unsigned int) time(NULL));
  
  for (i = 1; i <= n; i++)
    {
      perm[i] = i;
      sum1 += i;
    }

  for (i = 1; i <= n; i++) 
    {
      k        = randomInt(n + 1 - i);
      j        = perm[i];
      perm[i]     = perm[i + k];
      perm[i + k] = j; 
    }

}


void initAdef(analdef *adef)
{
  adef->model = M_HKY85;
  adef->timeLimit = 0;
  adef->t0 = 3.0;
  adef->repeats = 0;
  adef->max_rearrange = 21;  
  adef->stepwidth = 5;
  adef->initial = 10;
  adef->restart = FALSE;
  adef->mode = TREE_INFERENCE;
  adef->categories = 50;
  adef->boot    =           0;
  adef->interleaved =    TRUE;  
  adef->interleaved = 1;
  adef->maxSubProblemSize = 100;
  adef->useWeightFile = FALSE;
}


void get_args(int argc, char *argv[], boolean print_usage, analdef *adef, rawdata *rdta)
{ /* get_args */
  int        c;
  boolean    bad_opt=FALSE;
  int        i, time, temperature;
  char       buf[2048];
  char       model[2048] = "";
  char       modelChar;
  int nameSet = 0, alignmentSet = 0;

  run_id[0] = 0;
  workdir[0] = 0;
  seq_file[0] = 0;
  tree_file[0] = 0;
  model[0] = 0;
  weightFileName[0] = 0;


  

  while(!bad_opt && ((c=getopt(argc,argv,"a:b:c:d:f:g:i:k:l:m:r:t:w:s:n:e:"))!=-1) ) {
    switch(c) 
      {
      case 'a':
	strcpy(weightFileName,optarg);
	adef->useWeightFile = TRUE;
        break;
      case 'e':
	sscanf(optarg,"%d", &adef->maxSubProblemSize);	
	break;
      case 'b':
	sscanf(optarg,"%ld", &adef->boot);
	printf( "Bootstrap random number seed = %ld\n\n", adef->boot);
	break;
      case 'c':
	sscanf(optarg, "%d", &adef->categories);
	break;	      
      case 'd':
	sscanf(optarg, "%d", &temperature);
	adef->t0 = (double)temperature;	
	break;
      case 'g':
	sscanf(optarg, "%d", &adef->repeats);
	break;
      case 'l':
	sscanf(optarg, "%d", &time);
	adef->timeLimit = (double)time;
	break;
      case 'f': 
	sscanf(optarg, "%c", &modelChar);
	switch(modelChar)
	  {
	  case 'f': adef->mode = FAST_MODE;break;
	  case 's': adef->mode = SIMULATED_ANNEALING; break;
	  case 'e': adef->mode = TREE_EVALUATION; break;
	  case 'c': adef->mode = TREE_INFERENCE; break;	
	  case 'r': adef->mode = TREE_RECURSIVE; break;  
	  default: adef->mode = TREE_INFERENCE;
	  }
	break;      
      case 'i':
	sscanf(optarg, "%d", &adef->initial);
	break;     
      case 'n':
        strcpy(run_id,optarg);
	nameSet = 1;
        break;
      case 'w':
        strcpy(workdir,optarg);
        break;                 
      case 't':
	strcpy(tree_file, optarg);
	adef->restart = TRUE;
	break;
      case 'r':
	sscanf(optarg, "%d", &adef->stepwidth);	
	if(adef->stepwidth < 2) adef->stepwidth = 2;
	break;    
      case 'k':
	sscanf(optarg, "%d", &adef->max_rearrange);
	if(adef->max_rearrange < 5) adef->max_rearrange = 5;
	break;  
      case 's':
	strcpy(seq_file, optarg);
	alignmentSet = 1;
	break;
      case 'm':
	strcpy(model, optarg);
	if(strcmp(model, "HKY85\0") == 0)
	  {
	    adef->model = M_HKY85;
	    goto l_1;
	  }
	if(strcmp(model, "GTR\0") == 0)
	  {
	    adef->model = M_GTR;
	    goto l_1;
	  }
	if(strcmp(model, "HKY85CAT\0") == 0)
	  {
	    adef->model = M_HKY85CAT;
	    goto l_1;
	  }
	if(strcmp(model, "GTRCAT\0") == 0)
	  {
	    adef->model = M_GTRCAT;
	    goto l_1;
	  }
	
	printf("Model %s does not exist, please use either HKY85, HKY85CAT or GTR\n", model);
	exit(-1);
      l_1:
	break;      
      default:
        bad_opt = TRUE;
        break;
    }

    

  }
  
  if(adef->mode == TREE_EVALUATION && (!adef->restart))
    {
      printf("\n Error: please specify a treefile for the tree you want to evaluate with -t\n");
      exit(-1);
    }

  if(!nameSet)
    {
      printf("\n Error: please specify a name for this run with -n\n");
      exit(-1);
    }
    
  if(! alignmentSet)
    {
      printf("\n Error: please specify an alignment for this run with -s\n");
      exit(-1);
    }


#ifdef WIN32
  if(workdir[0]==0 || (workdir[1] != ':' && workdir[2] != '\\'))
    {
      getcwd(buf,sizeof(buf));
      if( buf[strlen(buf)-1] != '\\') strcat(buf,"\\");
      strcat(buf,workdir);
      if( buf[strlen(buf)-1] != '\\') strcat(buf,"\\");
      strcpy(workdir,buf);
    }
#else
  if(workdir[0]==0 || workdir[0] != '/') 
    {
      getcwd(buf,sizeof(buf));
      if( buf[strlen(buf)-1] != '/') strcat(buf,"/");
      strcat(buf,workdir);
      if( buf[strlen(buf)-1] != '/') strcat(buf,"/");
      strcpy(workdir,buf);
    }
#endif

  if(bad_opt) 
    {
      
      exit(-1);
    }
    return;
}










boolean str_treeReadLenSpecial (char *treestr, tree *tr)
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
    tr->smoothed    = FALSE;
    tr->rooted      = FALSE;


    is_fact = str_processTreeCom(tr,&treestr);
    p = tr->nodep[(tr->nextnode)++];

    if(!str_treeNeedCh(&treestr, '(', "at start of"))       return FALSE;
    if(!str_addElementLen(&treestr, tr, p))                 return FALSE;
    if(!str_treeNeedCh(&treestr, ',', "in"))                return FALSE;
    if(!str_addElementLen(&treestr, tr, p->next))           return FALSE;
    if (! tr->rooted) 
      { 
	if(str_treeGetCh(&treestr) == ',')
	  {
	    if (! str_addElementLen(&treestr, tr, p->next->next)) return FALSE;
	  }
	else
	  {
	    tr->rooted = TRUE;
	    if(*treestr != '\n') treestr--;
	  }
      }
    else
      {
	p->next->next->back = (nodeptr) NULL;
      }
      
    if(!str_treeNeedCh(&treestr, ')', "in"))                 return FALSE;
    str_treeFlushLabel(&treestr);           
    if(!str_treeFlushLen(&treestr))                          return FALSE;
    if(is_fact) {
      if(!str_treeNeedCh(& treestr, ')', "at end of"))       return FALSE;
      if(!str_treeNeedCh(& treestr, '.', "at end of"))       return FALSE;
    }
    else 
      {
	if(!str_treeNeedCh(& treestr, ';', "at end of"))       return FALSE;
      }

    if(tr->rooted)
      {
	p->next->next->back = (nodeptr) NULL;
	tr->start =  uprootTree(tr, p->next->next);
	if(!tr->start) return FALSE;
	}
    else
      {
	tr->start = p->next->next->back;
      }

    return  (initrav(tr,tr->start) && initrav(tr,tr->start->back));
  } /* str_treeReadLen */




void optimizeTT(tree *tr)
{
  int k;
 
  double currentTT = tr->t_value;
  double startLikelihood = tr->likelihood;
  double maxLikelihoodMinus = tr->likelihood;
  double maxLikelihoodPlus = tr->likelihood;
  double maxTTPlus = currentTT;
  double maxTTMinus = currentTT;
  double spacing = 0.5/optimizeTTinvocations;
  double tree_likelihood = startLikelihood;
  optimizeTTinvocations++;
      
  k = 1;
      
  while(tree_likelihood >= maxLikelihoodMinus && (currentTT - spacing * k) > 0) 
    {
      
      alterTT(tr, currentTT - spacing * k);     
      initrav(tr, tr->start);
      initrav(tr, tr->start->back);
      evaluate(tr, tr->start);
      
      tree_likelihood = tr->likelihood;
      if(tr->likelihood > maxLikelihoodMinus)
	{
	  maxLikelihoodMinus = tr->likelihood;
	  maxTTMinus = currentTT - spacing * k;
	}
      k++;
    }
  
  k = 1;
  tree_likelihood = startLikelihood;
  while(tree_likelihood >= maxLikelihoodPlus) 
    {      
      alterTT(tr, currentTT + spacing * k);     
      initrav(tr, tr->start);
      initrav(tr, tr->start->back);
      evaluate(tr, tr->start);      

      tree_likelihood = tr->likelihood;
      if(tr->likelihood > maxLikelihoodPlus)
	{
	  maxLikelihoodPlus = tr->likelihood;
	  maxTTPlus = currentTT + spacing * k;
	}	 
      k++;
    }
  
  if(maxLikelihoodPlus > startLikelihood || maxLikelihoodMinus > startLikelihood)
    {
      if(maxLikelihoodPlus > maxLikelihoodMinus)
	{
	  alterTT(tr, maxTTPlus);
	}
      else
	{
	  alterTT(tr, maxTTMinus);
	}     
      initrav(tr, tr->start);
      initrav(tr, tr->start->back);
      evaluate(tr, tr->start);
    }
  else
    { 
      alterTT(tr, currentTT);	
      initrav(tr, tr->start);
      initrav(tr, tr->start->back);      
      evaluate(tr, tr->start);
    }
}


void baseFrequenciesGTR(rawdata *rdta, cruncheddata *cdta)
{
  double  sum, suma, sumc, sumg, sumt, wj, fa, fc, fg, ft, freqa, freqc, freqg, freqt;
  int     i, j, k, code;
  yType  *yptr;

  freqa = 0.25;
  freqc = 0.25;
  freqg = 0.25;
  freqt = 0.25;
  for (k = 1; k <= 8; k++) 
    {
      suma = 0.0;
      sumc = 0.0;
      sumg = 0.0;
      sumt = 0.0;
      for (i = 0; i < rdta->numsp; i++) 
	{
	  yptr = rdta->y[i];
	  for (j = 0; j < cdta->endsite; j++) 
	    {
	      code = *yptr++;
	      fa = freqa * ( code       & 1);
	      fc = freqc * ((code >> 1) & 1);
	      fg = freqg * ((code >> 2) & 1);
	      ft = freqt * ((code >> 3) & 1);
	      wj = cdta->aliaswgt[j] / (fa + fc + fg + ft);
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

  rdta->freqa = freqa;
  rdta->freqc = freqc;
  rdta->freqg = freqg;
  rdta->freqt = freqt;

  
      
  return;
}

void initReversibleGTR(rawdata *rdta, cruncheddata *cdta)
{
  
  double r[4][4], a[4][4], *initialRates = rdta->initialRates, 
    f[4], e[4], d[4];
    int i, j, k, m;
    
    f[0] = rdta->freqa;
    f[1] = rdta->freqc;
    f[2] = rdta->freqg;
    f[3] = rdta->freqt;

    i = 0;
    for (j = 0; j < 2; j++)
      for (k = j+1; k< 4; k++)
	r[j][k] = initialRates[i++];
    r[2][3] = 1;
    for (j = 0; j < 4; j++) 
	{
	  r[j][j] = 0;
	  for (k = 0; k < j; k++)
	    r[j][k] = r[k][j];
	}
    
    
    rdta->fracchangeav = 0;
    for (j = 0; j<4; j++)
      for (k = 0; k<4; k++)
	rdta->fracchangeav += f[j] * r[j][k] * f[k];
   
    
    
    m = 0;
    
      for(i=0; i<4; i++) 
	a[i][i] = 0;
      
      for(i=0; i<4; i++) 
	{
	  for(j=i+1; j<4; j++) 
	    {
	      double factor = ((m>=5) ? 1 : initialRates[m++]);
	      a[i][j] = a[j][i] = factor * sqrt( f[i] * f[j]);
	      a[i][i] -= factor * f[j];
	      a[j][j] -= factor * f[i];
	    }
	}
      
 

      tred2((double *)a,4,4,d,e);       
      tqli(d, e, 4 , 4, (double *)a);
           
      for(i=0; i<4; i++) 
	{
	  for(j=0; j<4; j++) 
	    {
	      a[i][j] *= sqrt(f[j]);
	    }
	}    


     
      for (i=0; i<4; i++)
	{	  
	  if (d[i] > -1e-8) 
	    {
	      
	      if (i != 0) 
		{		    
		  double tmp = d[i], sum=0;
		  d[i] = d[0];
		  d[0] = tmp;
		  for (j=0; j < 4; j++) 
		    {
		      tmp = a[i][j];
		      a[i][j] = a[0][j];
		      sum += (a[0][j] = tmp);
		    }
		  for (j=0; j < 4; j++) 
		    a[0][j] /= sum;
		}
	      break;
	    }
	}
      for (i=0; i< 4; i++) 
	{
	  rdta->EIGN[0][i] = -d[i];
	  for (j=0; j<4; j++)
	    rdta->EIGV[0][i][j] = a[j][i];
	}
     
      rdta->EIGN[1][0] = rdta->EIGN[0][0];
      rdta->EIGN[1][1] = rdta->EIGN[0][1];
      rdta->EIGN[1][2] = rdta->EIGN[0][2];
      rdta->EIGN[1][3] = rdta->EIGN[0][3];

     
      rdta->matread = TRUE;
      

      for (i = 0; i<4; i++) 
      {
	rdta->invfreq[0][i] = 1 / rdta->EIGV[0][i][0];	
      }


      /*invfreq[4], EIGV[4][4], EIGN[4]*/

}








void alterRates(tree *tr, int k)
{
  int i, increasing;
  double granularity = 0.1/((double)optimizeRatesInvocations);
  double bestLikelihood = tr->likelihood, 
    maxLikelihoodMinus = tr->likelihood, maxLikelihoodPlus = tr->likelihood,
    treeLikelihood, originalRate, maxRateMinus, maxRatePlus;
 
  
  i = 1;
  originalRate = tr->rdta->initialRates[k];
  maxRateMinus = maxRatePlus = originalRate;
  treeLikelihood = bestLikelihood;


  while(treeLikelihood >= maxLikelihoodMinus && (originalRate - granularity * i) > 0)
    {
      tr->rdta->initialRates[k] = originalRate - granularity * i;
      initReversibleGTR(tr->rdta, tr->cdta);
      initrav(tr, tr->start);
      initrav(tr, tr->start->back);
      evaluate(tr, tr->start);      
      treeLikelihood = tr->likelihood;
      if(tr->likelihood > maxLikelihoodMinus)
	{
	  increasing = 1;
	  maxLikelihoodMinus = tr->likelihood;
	  maxRateMinus = originalRate - granularity * i;
	}      
      i++;
    }

  i = 1;
  treeLikelihood = bestLikelihood;
  tr->rdta->initialRates[k] = originalRate;
  while(treeLikelihood >= maxLikelihoodPlus)
    {
      tr->rdta->initialRates[k] = originalRate + i * granularity;
      initReversibleGTR(tr->rdta, tr->cdta);
      initrav(tr, tr->start);
      initrav(tr, tr->start->back);
      evaluate(tr, tr->start);      
      treeLikelihood = tr->likelihood;
      if(tr->likelihood > maxLikelihoodPlus)
	{
	  increasing = 1;
	  maxLikelihoodPlus = tr->likelihood;
	  maxRatePlus = originalRate + granularity * i;
	}      
      i++;
    }
  if(maxLikelihoodPlus > bestLikelihood || maxLikelihoodMinus > bestLikelihood)
    {
      if(maxLikelihoodPlus > maxLikelihoodMinus)
	{	  
	  tr->rdta->initialRates[k] = maxRatePlus;
	  initReversibleGTR(tr->rdta, tr->cdta);
	}
      else
	{
	 tr->rdta->initialRates[k] = maxRateMinus;
	 initReversibleGTR(tr->rdta, tr->cdta);	  
	}     
      
      initrav(tr, tr->start);
      initrav(tr, tr->start->back);
      evaluate(tr, tr->start);     
    }
  else
    {      
      tr->rdta->initialRates[k] = originalRate;
      initReversibleGTR(tr->rdta, tr->cdta);      
      initrav(tr, tr->start);
      initrav(tr, tr->start->back);
      evaluate(tr, tr->start);     
    }

  
}

void optimizeRates(tree *tr)
{
  int i;
  double initialLikelihood, improvedLikelihood;
 
  for(i = 0; i < 5; i++)
    {	      
      alterRates(tr, i);     
    }

  optimizeRatesInvocations++;
}



void printModelAndProgramInfo(tree *tr, analdef *adef, int argc, char *argv[])
{ 
  int i;
  
  infoFile = fopen(infoFileName, "w");

  if(adef->repeats == 0)
    adef->repeats = tr->mxtips;


  switch(adef->mode)
     {   
     case TREE_INFERENCE : 
       printf("RAxML\n\n"); 
       fprintf(infoFile, "RAxML\n\n");
       break;		
     case FAST_MODE : 
       printf("RAxML FAST MODE\n\n");  
       fprintf(infoFile, "RAxML FAST MODE\n\n");
       break;  
     case SIMULATED_ANNEALING : 
       printf("RAxML SIMULATED ANNEALING\n");
       printf("start temperature: %f repeats: %d\n\n",adef->t0, adef->repeats);
       fprintf(infoFile, "RAxML SIMULATED ANNEALING\n");
       fprintf(infoFile, "start temperature: %f repeats: %d\n\n",adef->t0, adef->repeats);
       break;    
     case TREE_EVALUATION : 
       printf("RAxML Model Optimization\n\n");
       fprintf(infoFile, "RAxML Model Optimization\n\n");
       break; 
     case  TREE_RECURSIVE:  
       printf("RAxML DIVIDE & CONQUER MODE\nMAXIMUM SUBPROBLEM SIZE: %d\n\n", adef->maxSubProblemSize);
       fprintf(infoFile, "RAxML DIVIDE & CONQUER MODE\nMAXIMUM SUBPROBLEM SIZE: %d\n\n", adef->maxSubProblemSize);       
       break;    
     }
    
  switch(adef->model)
    {
    case M_HKY85:
       printf( "HKY85 model of nucleotide substitution\n");  
       printf( "Transition/Transversion parameter will be estimated\n"); 
       fprintf(infoFile, "HKY85 model of nucleotide substitution\n");  
       fprintf(infoFile, "Transition/Transversion parameter will be estimated\n");
       break;      
    case M_HKY85CAT:
      printf( "HKY85 model of nucleotide substitution\n");  
      printf( "Transition/Transversion parameter will be estimated\n");  
      printf( "ML estimate of %d per site rate categories\n", adef->categories);
      fprintf(infoFile, "HKY85 model of nucleotide substitution\n");  
      fprintf(infoFile, "Transition/Transversion parameter will be estimated\n");  
      fprintf(infoFile, "ML estimate of %d per site rate categories\n", adef->categories);
      break;
    case M_GTR:
       printf( "GTR model of nucleotide substitution\n"); 
       printf( "All model parameters will be estimated by RAxML\n");
       fprintf(infoFile, "GTR model of nucleotide substitution\n"); 
       fprintf(infoFile, "All model parameters will be estimated by RAxML\n");
       break;
    case M_GTRCAT:
       printf( "GTR model of nucleotide substitution\n"); 
       printf( "All model parameters will be estimated by RAxML\n");
       printf( "ML estimate of %d per site rate categories\n", adef->categories);
       fprintf(infoFile, "GTR model of nucleotide substitution\n"); 
       fprintf(infoFile,"All model parameters will be estimated by RAxML\n");
       fprintf(infoFile, "ML estimate of %d per site rate categories\n", adef->categories);
       break;
    }

  printf("Empirical Base Frequencies:\n");

  printf("pi(A): %f ",   tr->rdta->freqa);
  printf("pi(C): %f ",   tr->rdta->freqc);
  printf("pi(G): %f ",   tr->rdta->freqg);
  printf("pi(T): %f \n",   tr->rdta->freqt);  

  printf("\n\n");

  fprintf(infoFile, "Empirical Base Frequencies:\n");

  fprintf(infoFile, "pi(A): %f ",   tr->rdta->freqa);
  fprintf(infoFile, "pi(C): %f ",   tr->rdta->freqc);
  fprintf(infoFile, "pi(G): %f ",   tr->rdta->freqg);
  fprintf(infoFile, "pi(T): %f \n",   tr->rdta->freqt);  

  fprintf(infoFile, "\n\n");

  fprintf(infoFile,"RAxML was called as follows:\n\n");
  for(i = 0; i < argc; i++)
    fprintf(infoFile,"%s ", argv[i]);
  fprintf(infoFile,"\n\n\n");

  fclose(infoFile);
}








void checkTime(analdef *adef, tree *tr)
{
  if(adef->timeLimit == 0) return;
  {
    double delta = gettime() - masterTime;
    if(delta > adef->timeLimit)
      {
	printf("time limit exceeded ... exiting with Lh %f at time %f\n", 
		tr->likelihood, delta);
	treeEvaluate(tr, 4);
	printResult(tr, 0);
	exit(1);
      }
  }
}


/* start of Divide and Conquer methods */


void treeOptimizeSubtree(tree *tr, int _mintrav, int _maxtrav, int initial, subNodes *sn)
{
  int i;
  int mintrav = _mintrav;
  int maxtrav = _maxtrav;
  int subsequent = 0;

  if (maxtrav > tr->ntips - 3)  
    maxtrav = tr->ntips - 3;
  
  tr->opt_level = 0;
  
  resetBestTree(bt);
  saveBestTree(bt, tr);

 

  /*fprintf(stderr," treeOptimizeSubtree() %d %d %d %d\n", mintrav, maxtrav, Multiple, Thorough);*/

  tr->startLH = tr->endLH = tr->likelihood;


  for(i = 0; i < sn->count; i++)
    {           
      rearrangeParsimony(tr, sn->nodeArray[i], mintrav, maxtrav);
           
      if(Multiple && (tr->endLH > tr->startLH))                 	
	{	  
	  recallBestTree(bt, 1, tr);
	  /*fprintf(stderr,"Tree improved %f\n", tr->likelihood);*/
	  tr->startLH = tr->endLH = tr->likelihood;
	  subsequent = 1;
	}	
    }
 
  
  Multiple = subsequent;
  return;     
}





boolean computeSubtree (tree *tr, analdef *adef, subNodes *sn) 
  { 
    int i, rearrangementsMax, rearrangementsMin, noImproveCount, impr;                
    double lh, previousLh, difference, epsilon;
    bestlist *bestT;


    bestT = (bestlist *) malloc(sizeof(bestlist));
    bestT->ninit = 0;
    initBestTree(bestT, 1, tr->mxtips, tr->cdta->endsite);
  
    difference = 10.0;
    epsilon = 0.001;
    
    Multiple = 0;    
        
    resetBestTree(bt);            
    lh = tr->likelihood;    

   
    saveBestTree(bestT, tr);  
   
    impr = 1;		  		  
    
    while(1)
      {		
	recallBestTree(bestT, 1, tr);
	treeEvaluate(tr, 2);
	lh = tr->likelihood;

     	 
	if(impr)
	{
	    rearrangementsMin = 1;
	    rearrangementsMax = 5;    
	}
	else
	{
	  
	  if(rearrangementsMax < 10)
	    {
	      rearrangementsMin = 6;
	      rearrangementsMax = 10;			
	    }
	  else
	    goto cleanup; 	   	    
	}

	

	treeOptimizeSubtree(tr, rearrangementsMin, rearrangementsMax, adef->initial, sn);
	
	impr = 0;			      	
	previousLh = lh;
	for(i = 1; i <= bt->nvalid; i++)
	  {	    		  	   
	    recallBestTree(bt, i, tr);
	    /*fprintf(stderr,"%d %f\n", i, tr->likelihood);*/
	    treeEvaluate(tr, 0.25);
	    /*fprintf(stderr,"%d %f\n\n", i, tr->likelihood);*/
	    difference = ((tr->likelihood > previousLh)? 
			  tr->likelihood - previousLh: 
			  previousLh - tr->likelihood); 	    
	    if(tr->likelihood > lh && difference > epsilon)
	      {
		impr = 1;	       
		lh = tr->likelihood;	       
		saveBestTree(bestT, tr);		
	      }	   	   
	  }	
      }
    
  cleanup: 
    freeBestTree(bestT);
  }


boolean computeSubtreeReconnect (tree *tr, analdef *adef, subNodes *sn, int areaSize, int algoStage) 
  { 
    int i, rearrangementsMax, rearrangementsMin, noImproveCount, impr;                
    double lh, previousLh, difference, epsilon;
    bestlist *bestT;


    bestT = (bestlist *) malloc(sizeof(bestlist));
    bestT->ninit = 0;
    initBestTree(bestT, 1, tr->mxtips, tr->cdta->endsite);
  
    difference = 10.0;
    epsilon = 0.001;
    
    Multiple = 1;
    
        
    resetBestTree(bt);            
    lh = tr->likelihood;    

 
    saveBestTree(bestT, tr);  
   
    impr = 1;		  		  
    rearrangementsMin = 1;
    rearrangementsMax = areaSize;

    while(1)
      {
	recallBestTree(bestT, 1, tr);
	treeEvaluate(tr, 2);
	lh = tr->likelihood;
	/*fprintf(stderr,"%f\n", lh);*/

     	 
	if(!impr)
	  {	    
	    goto cleanup; 	   
	  }
	
	treeOptimizeSubtree(tr, rearrangementsMin, rearrangementsMax, adef->initial, sn);
	
	impr = 0;			      	
	previousLh = lh;
		    		  	   
	if(algoStage == 1)
	  {
	    for(i = 1; i <= bt->nvalid; i++)
	      {	    		  	   
		recallBestTree(bt, i, tr);		
		treeEvaluate(tr, 0.25);
		/*fprintf(stderr,"%d %f\n\n", i, tr->likelihood);*/
		difference = ((tr->likelihood > previousLh)? 
			      tr->likelihood - previousLh: 
			      previousLh - tr->likelihood); 	    
		if(tr->likelihood > lh && difference > epsilon)
		  {
		    impr = 1;	       
		    lh = tr->likelihood;	       
		    saveBestTree(bestT, tr);		
		  }	   	   
	      }	
	  }
	else
	  {
	    recallBestTree(bt, 1, tr);
	    treeEvaluate(tr, 1);

	    difference = ((tr->likelihood > previousLh)? 
			  tr->likelihood - previousLh: 
			  previousLh - tr->likelihood); 

	    if(tr->likelihood > lh && difference > epsilon)
	      {
		impr = 1;	       
		lh = tr->likelihood;	       
		saveBestTree(bestT, tr);		
	      }	 

	  }


	


  	   
      }
    
  cleanup: 
    freeBestTree(bestT);
  }

boolean computeFastSubtree (tree *tr, analdef *adef, subNodes *sn) 
{ 
  int i, mintrav, maxtrav, which;
  double initialLH, epsilon, diff;
 

  epsilon = 0.01;
  Multiple = 1;
  mintrav = 1;
  maxtrav = 10;
  
  if (maxtrav > tr->ntips - 3)  maxtrav = tr->ntips - 3;
          
  tr->opt_level = 0; 
  resetBestTree(bt);
  saveBestTree(bt, tr);
  /*fprintf(stderr,"computeFastSubtree() %d %d %d %d\n", mintrav, maxtrav, Multiple, Thorough);*/
  do
    {   
      /*fprintf(stderr,"%f\n", tr->likelihood);*/
      initialLH = tr->likelihood;
      tr->startLH = tr->endLH = tr->likelihood;

      for(i = 0; i < sn->count; i++)
	{ 
	  
	  rearrangeParsimony(tr, sn->nodeArray[i], mintrav, maxtrav);
	  if(tr->endLH > tr->startLH)   
	    {
	      recallBestTree(bt, 1, tr);
	      tr->startLH = tr->endLH = tr->likelihood;		    
	    }
	 
	}
      recallBestTree(bt, 1, tr);      
      treeEvaluate(tr, 1);
      checkTime(adef, tr);
      saveBestTree(bt, tr);
      diff = (tr->likelihood > initialLH)? tr->likelihood - initialLH: -1.0;        
    }
  while(diff > epsilon);

  initialLH = unlikely;
   	    	
  recallBestTree(bt, 1, tr);			       
  treeEvaluate(tr, 2); 
}

boolean computeRapidSubtree (tree *tr, analdef *adef, subNodes *sn) 
{ 
  int i, mintrav, maxtrav, which;
  double initialLH, epsilon, diff;
 

  epsilon = 0.01;
  Multiple = 1;
  mintrav = 1;
  maxtrav = 5;
  
  if (maxtrav > tr->ntips - 3)  maxtrav = tr->ntips - 3;
          
  tr->opt_level = 0; 
  resetBestTree(bt);
  saveBestTree(bt, tr);

  fprintf(stderr,"computeRapidSubtree() %d %d %d %d\n", mintrav, maxtrav, Multiple, Thorough);
   
  fprintf(stderr,"%f\n", tr->likelihood);
  initialLH = tr->likelihood;
  tr->startLH = tr->endLH = tr->likelihood;

  for(i = 0; i < sn->count; i++)
    { 
	  
      rearrangeParsimony(tr, sn->nodeArray[i], mintrav, maxtrav);
      if(tr->endLH > tr->startLH)   
	{
	  recallBestTree(bt, 1, tr);
	  tr->startLH = tr->endLH = tr->likelihood;		    
	}	 
    }

  recallBestTree(bt, 1, tr);      
  treeEvaluate(tr, 2); 
  fprintf(stderr,"%f\n", tr->likelihood);
}



void getNodes(tree *tr, nodeptr p,subNodes *sn)
{
  nodeptr q;

  sn->nodeArray[sn->count] = p;
  sn->count++;

  if(p->tip)          
    return;

  q = p->next;
  while(q != p)
    {
      getNodes(tr, q->back, sn);
      q = q->next;
    }

  return;
}


int countTips(tree *tr, nodeptr p)
{
  nodeptr q;
  int acc = 0;

  if(p->tip)
    return 1;

  q = p->next;
  while(q != p)
    {
      acc += countTips(tr, q->back);
      q = q->next;
    }

  return acc;
}


int makeArea(nodeptr p, int max, subNodes *sn)
{
  nodeptr q;
  int acc = 0;

  if(max == 0)
    return 0;

  sn->nodeArray[sn->count] = p;
  sn->count++;

  if(p->tip)     
    return 1;  

  q = p->next;
  acc = 1;
  while(q != p)
    {	
      acc += makeArea(q->back, max - 1, sn);
      q = q->next;
    }

  return acc;
}


static int BTypeCompare(const void *p1, const void *p2)
{
 BType *rc1 = (BType *)p1;
 BType *rc2 = (BType *)p2;

 int i = rc1->difference;
 int j = rc2->difference;
  
  if (i > j)
    return (1);
  if (i < j)
    return (-1);
  return (0);
}



int bisectNode(tree *tr, subNodes *sn)
{
  int i, leftTips, rightTips, balance, difference, result, sum;
  double zMax;
  nodeptr p, q;
  
  balance = 2 * tr->mxtips;

  for (i = 0; i < sn->count; i++) 
    {
      p = sn->nodeArray[i];
     
      q = p->back;
     
      if((!p->tip) && (!q->tip))
	{
	  if(q->back != p)
	    {
	      fprintf(stderr,"Bad rear at %d\n", i);
	      exit(1);
	    }
	  
	  leftTips = countTips(tr, p);
	  rightTips = countTips(tr,q);     
	  difference = ABS(leftTips - rightTips);

	  if(balance > difference)
	    {
	      balance = difference; 
	      result = p->number;
	    }	
	}
      
    }
  
  return result;
}






double reconnect(tree *tr, nodeptr p, nodeptr q, nodeptr p1, nodeptr q1, subNodes *sn, int algoStage)
{
  nodeptr p2, q2;
  int areaSize;

  p2 = p1->back;
  if(p2->back != p1)
    {
      fprintf(stderr,"Bad rear\n");
      exit(1);
    }

  p->next->back = p1; 
  p1->back = p->next;

  p->next->z = p->next->back->z = defaultz;

  p->next->next->back = p2;
  p2->back = p->next->next;

  p->next->next->z = p->next->next->back->z = defaultz;
  
  q2 = q1->back;
  if(q2->back != q1)
    {
      fprintf(stderr,"Bad rear\n");
      exit(1);
    }
  
  q->next->back = q1; 
  q1->back = q->next;

  q->next->z = q->next->back->z = defaultz;

  q->next->next->back = q2;
  q2->back = q->next->next;

  q->next->next->z = q->next->next->back->z = defaultz;
  
  tr->start = p;  

  initrav(tr, p);  
  initrav(tr, q);
  evaluate(tr, p); 
   
  sn->count = 0;
 
  /*areaSize = makeArea(p, 3, sn) + makeArea(q, 3, sn);*/

  makeArea(p1, 4, sn);
  makeArea(p2, 4, sn);
  makeArea(q1, 4, sn);
  makeArea(q2, 4, sn);
  makeArea(p, 4, sn);
  makeArea(q, 4, sn);
  computeSubtreeReconnect(tr,adef, sn, 10, algoStage);
  /* 10 or areaSize*/
 
     
  return tr->likelihood;
}




void recursiveAnalysis(tree *tr, analdef *adef, subNodes *sn, int algoStage)
{
  nodeptr p, q;
  int leftTips, rightTips, subtreeSize, splitNumber;
  p = tr->start;
  q = p->back;

  leftTips = countTips(tr, p);
  rightTips = countTips(tr,q);

  subtreeSize = leftTips + rightTips;

  if(subtreeSize > adef->maxSubProblemSize)
    {
      nodeptr p1, p2, q1, q2;
      subNodes *sn2;
      bestlist *T1, *T2;
      int areaSize;

      T1 = (bestlist *) malloc(sizeof(bestlist));
      T2 = (bestlist *) malloc(sizeof(bestlist));
      T1->ninit = 0;
      T2->ninit = 0;
      initBestTree(T1, 1, tr->mxtips, tr->cdta->endsite);
      initBestTree(T2, 1, tr->mxtips, tr->cdta->endsite);
      sn2 = (subNodes *)malloc(sizeof(subNodes));

      sn2->count = 0;
      sn2->nodeArray = (node **)malloc(2 * tr->mxtips * sizeof(node *));

     
      splitNumber = bisectNode(tr, sn);

      p = tr->nodep[splitNumber];
     
      q = p->back; 
      
      if(p->tip || q->tip)
	{
	  fprintf(stderr,"recursiveAnalysis split at tip, doing subtree inference instead\n");
	  freeBestTree(T1);
	  freeBestTree(T2);
	  free(sn2->nodeArray);
	  free(sn2);  
	  goto subtreeInference;
	}

      if(q->back->number != p->number) 
	{
	  fprintf(stderr,"recursiveAnalysis bad rear\n");
	  exit(-1);
	}

      p1 = p->next->back;
      p2 = p->next->next->back;
      hookup(p1, p2, defaultz);
      initrav(tr, p1); 
      initrav(tr, p2);
      evaluate(tr, p1);
      tr->start = p1;
      evalFast(tr);    
      saveBestTree(T1, tr);
  
      q1 = q->next->back;
      q2 = q->next->next->back;
      hookup(q1, q2, defaultz);
      initrav(tr, q1);  
      initrav(tr, q2);
      evaluate(tr, q1); 
      tr->start = q1;
      evalFast(tr);      
      saveBestTree(T2, tr);

      recallBestTree(T1, 1, tr);
      sn2->count = 0;
      getNodes(tr, p1, sn2);
      getNodes(tr, p2, sn2);   
      recursiveAnalysis(tr, adef, sn2, algoStage);
      saveBestTree(T1, tr);
    
      recallBestTree(T2, 1, tr);
      sn2->count = 0;
      getNodes(tr, q1, sn2);
      getNodes(tr, q2, sn2);
      recursiveAnalysis(tr, adef, sn2, algoStage);
      saveBestTree(T2, tr);

      recallBestTreeRecursive(T1, 1, tr);
      recallBestTreeRecursive(T2, 1, tr);

      initrav(tr, p1);  
      initrav(tr, p1->back);
      evaluate(tr, p1); 
 
      initrav(tr, q1);  
      initrav(tr, q1->back);
      evaluate(tr, q1); 
  
      
      
      hookup(q, p, defaultz);  
      tr->start = p;
      
    
      sn2->count = 0;
      
     
      reconnect(tr, p, q, p1, q1, sn2, algoStage);


      freeBestTree(T1);
      freeBestTree(T2);
      free(sn2->nodeArray);
      free(sn2);   
    }
  else
    { 
    subtreeInference:
       computeSubtree(tr, adef, sn);
      /*computeFastSubtree(tr, adef, sn);*/

      /*switch(algoStage)
	{
	case 0: 	  
	  computeFastSubtree(tr, adef, sn);
	  break;
	case 1: 
	  computeSubtree(tr, adef, sn);
	  break;
	default: 
	  fprintf(stderr,"fatal error\n"); 
	  exit(1);
	  }*/

    }
}

boolean computeRecursive2 (tree *tr, analdef *adef) 
{
  int lc, i, algoStage;      
  subNodes *sn;
  double startLH, endLH;
 
  sn = (subNodes *)malloc(sizeof(subNodes)); 
  sn->nodeArray = (node **)malloc(2 * tr->mxtips * sizeof(node *));
 
  Thorough = 0;
  algoStage = 0;
  lc = 10;

  while(lc > 0)
    {
      optimizeModel(tr, adef, 1);	  
      treeEvaluate(tr, 2);    
      printLog(tr);
      printResult(tr, 0);
      startLH = tr->likelihood;
           
      sn->count = 0;
      getNodes(tr, tr->start, sn);
      getNodes(tr, tr->start->back, sn);
      recursiveAnalysis(tr, adef, sn, algoStage);
      
      endLH = tr->likelihood;
     
	  
     
      if(lc < 8)
	algoStage = 1;
      fprintf(stderr,"Algo stage of next it %d\n", algoStage);
      Thorough = 1;
      lc--;
    }

}


boolean computeRecursive (tree *tr, analdef *adef) 
{
  int lc, i, algoStage;      
  subNodes *sn;
  double startLH, endLH;
 
  sn = (subNodes *)malloc(sizeof(subNodes)); 
  sn->nodeArray = (node **)malloc(2 * tr->mxtips * sizeof(node *));
 
  Thorough = 0;
  algoStage = 0;
  lc = 10;

 
  optimizeModel(tr, adef, 1);	  
  treeEvaluate(tr, 2);    
  printLog(tr);
  printResult(tr, 0);
  startLH = tr->likelihood;           
  sn->count = 0;
  getNodes(tr, tr->start, sn);
  getNodes(tr, tr->start->back, sn);
  recursiveAnalysis(tr, adef, sn, algoStage);
     	            
  Thorough = 1;
  optimizeModel(tr, adef, 1);	  
  treeEvaluate(tr, 2);    
  printLog(tr);
  printResult(tr, 0);
  startLH = tr->likelihood;           
  sn->count = 0;
  getNodes(tr, tr->start, sn);
  getNodes(tr, tr->start->back, sn);
  recursiveAnalysis(tr, adef, sn, algoStage);

  exit(1);
  /* TODO */

  compute(tr, adef);
  

}






/* end of Divide and Conquer methods */



boolean compute (tree *tr, analdef *adef) 
  { 
    int i, rearrangementsMax, rearrangementsMin, noImproveCount, impr;                
    double lh, previousLh, difference, epsilon;
    bestlist *bestT;

    bestT = (bestlist *) malloc(sizeof(bestlist));
    bestT->ninit = 0;
    initBestTree(bestT, 1, tr->mxtips, tr->cdta->endsite);
  
    difference = 10.0;
    epsilon = 0.001;
    
    Multiple = 1;
    Thorough = 0;     
        
    resetBestTree(bt);            
    lh = tr->likelihood;    

 
    saveBestTree(bestT, tr);  
   
    impr = 1;		  		  
 
    while(1)
      {			
	recallBestTree(bestT, 1, tr);	
	optimizeModel(tr, adef, 1);
	treeEvaluate(tr, 2);

	lh = tr->likelihood;
	printLog(tr);
	checkTime(adef, tr);
     	 
	if(impr)
	  {	 	   
	    noImproveCount = 0;
	    printResult(tr, 0);	   
	    rearrangementsMin = 1;
	    rearrangementsMax = adef->stepwidth;
	  }			  			
	else
	  {	   
	    if(Thorough == 0)
	      { 		
		Thorough = 1;
		rearrangementsMax = adef->stepwidth;
		rearrangementsMin = 1; 
	      }
	    else
	      { 
		rearrangementsMax += adef->stepwidth;
		rearrangementsMin += adef->stepwidth; 
	      }	    	      
	    if(rearrangementsMax > adef->max_rearrange)	     	     	 
	      goto cleanup; 	   
	  }

	

	treeOptimize(tr, rearrangementsMin, rearrangementsMax, adef);
	
	impr = 0;			      	
	previousLh = lh;

	checkTime(adef, tr);
      
	for(i = 1; i <= bt->nvalid; i++)
	  {	    		  	   
	    recallBestTree(bt, i, tr);	    
	    treeEvaluate(tr, 0.25);	    
	    difference = ((tr->likelihood > previousLh)? 
			  tr->likelihood - previousLh: 
			  previousLh - tr->likelihood); 	    
	    if(tr->likelihood > lh && difference > epsilon)
	      {
		impr = 1;	       
		lh = tr->likelihood;	       
		saveBestTree(bestT, tr);		
	      }	   	   
	  }

      }
    
  cleanup: 
    recallBestTree(bestT, 1, tr);
    printLog(tr);
    printResult(tr, 0);	   
    freeBestTree(bestT);
  }










boolean computeFast (tree *tr, analdef *adef) 
{ 
  int i, mintrav, maxtrav, which;
  double initialLH, epsilon, diff;
 

  epsilon = 0.01;
  Thorough = 0;
  Multiple = 1;
  mintrav = 1;
  maxtrav = 8;
  
  if (maxtrav > tr->ntips - 3)  maxtrav = tr->ntips - 3;
          
  tr->opt_level = 0;
  optimizeModel(tr, adef, 1);
  resetBestTree(bt);
  saveBestTree(bt, tr);

  do
    {
      printLog(tr);
      initialLH = tr->likelihood;
      tr->startLH = tr->endLH = tr->likelihood;

      for(i = 1; i < (2 * tr->mxtips - 1); i++)
	{ 
	  
	  rearrangeParsimony(tr, tr->nodep[i], mintrav, maxtrav);
	  if(tr->endLH > tr->startLH)   
	    {
	      recallBestTree(bt, 1, tr);
	      tr->startLH = tr->endLH = tr->likelihood;		    
	    }
	 
	}
      recallBestTree(bt, 1, tr);      
      treeEvaluate(tr, 1);
      checkTime(adef, tr);
      saveBestTree(bt, tr);
      diff = (tr->likelihood > initialLH)? tr->likelihood - initialLH: -1.0;        
    }
  while(diff > epsilon);

  initialLH = unlikely;

  for(i = 1; i <= bt->nvalid; i++)
    {	    	    	
      recallBestTree(bt, i, tr);			    
      smoothTree(tr, smoothings/4);
      evaluate(tr, tr->start);		           
      if(tr->likelihood > initialLH)
	{
	  which = i;
	  initialLH = tr->likelihood;
	}           
    }

  recallBestTree(bt, which, tr);
  treeEvaluate(tr, 1);
  checkTime(adef, tr);
  optimizeModel(tr, adef, 1);
  printLog(tr);
  printResult(tr, 0);
}




double calcPartialLikelihood(tree *tr, int i, double ki)
{
  initravPartial(tr, tr->start, i, ki);
  initravPartial(tr, tr->start->back, i, ki);
  return evaluatePartial(tr, tr->start, i, ki);
}









static int doublecompare2(const void *p1, const void *p2)
{
  double i = *((double *)p1);
  double j = *((double *)p2);
  
  if (i > j)
    return (1);
  if (i < j)
    return (-1);
  return (0);
}

static int catCompare(const void *p1, const void *p2)
{
 rateCategorize *rc1 = (rateCategorize *)p1;
 rateCategorize *rc2 = (rateCategorize *)p2;

  double i = rc1->accumulatedSiteLikelihood;
  double j = rc2->accumulatedSiteLikelihood;
  
  if (i > j)
    return (1);
  if (i < j)
    return (-1);
  return (0);
}



void categorize(tree *tr, rateCategorize *rc)
{
  int i, k, found;
  double temp, diff, min;

  for (i = 0; i < tr->cdta->endsite; i++) 
      {
	temp = tr->cdta->patrat[i];
	found = 0;
	for(k = 0; k < tr->NumberOfCategories; k++)
	  {
	    if(temp == rc[k].rate || (fabs(temp - rc[k].rate) < 0.001))
	      {
		found = 1;
		tr->cdta->rateCategory[i] = k;				
		break;
	      }
	  }
	if(!found)
	  {
	    min = fabs(temp - rc[0].rate);
	    tr->cdta->rateCategory[i] = 0;

	    for(k = 1; k < tr->NumberOfCategories; k++)
	    {
	      diff = fabs(temp - rc[k].rate);
	      if(diff < min)
		{
		  min = diff;
		  tr->cdta->rateCategory[i] = k;
		}
	    }
	  }
      }

  for(k = 0; k < tr->NumberOfCategories; k++)
    tr->cdta->patrat[k] = rc[k].rate; 

}




void optimizeRateCategories(tree *tr, int categorized, int _maxCategories)
{
  int i, k;
  double initialRate, initialLikelihood, v, leftRate, rightRate, leftLH, rightLH, temp, wtemp;   
  double lower_spacing, upper_spacing;
  int maxCategories = _maxCategories;
  double initialLH = tr->likelihood;
  double *oldRat = (double *)malloc(sizeof(double) * tr->cdta->endsite);
  double *ratStored = (double *)malloc(sizeof(double) * tr->cdta->endsite);
  double *oldwr = (double *)malloc(sizeof(double) * tr->cdta->endsite);
  double *oldwr2 = (double *)malloc(sizeof(double) * tr->cdta->endsite);
  int *oldCategory = (int *)malloc(sizeof(int) * tr->cdta->endsite);
  int oldNumber; 
  double epsilon = 0.00001;
  
  if(optimizeRateCategoryInvocations == 1)
    {
      lower_spacing = 0.5 / ((double)optimizeRateCategoryInvocations);
      upper_spacing = 1.0 / ((double)optimizeRateCategoryInvocations);
    }
  else
    {
      lower_spacing = 0.05 / ((double)optimizeRateCategoryInvocations);
      upper_spacing = 0.1 / ((double)optimizeRateCategoryInvocations);
    }

  if(lower_spacing < 0.001)
    lower_spacing = 0.001;

  if(upper_spacing < 0.001)
    upper_spacing = 0.001;

  optimizeRateCategoryInvocations++;
  
  oldNumber = tr->NumberOfCategories;
  for(i = 0; i < tr->cdta->endsite; i++)
    {    
      oldCategory[i] = tr->cdta->rateCategory[i];
      ratStored[i] = tr->cdta->patratStored[i];    
      oldRat[i] = tr->cdta->patrat[i];
      oldwr[i] =  tr->cdta->wr[i];
      oldwr2[i] =  tr->cdta->wr2[i];
    }
  

  for(i = 0; i < tr->cdta->endsite; i++)
    {    
      tr->cdta->patrat[i] = tr->cdta->patratStored[i];     
      initialRate = tr->cdta->patrat[i];
      initialLikelihood = calcPartialLikelihood(tr, i, initialRate);
      leftLH = rightLH = initialLikelihood;
      leftRate = rightRate = initialRate;
  
      k = 1;
      while((initialRate - k * lower_spacing > 0.0001) && 
	    ((v = calcPartialLikelihood(tr, i, initialRate - k * lower_spacing)) > leftLH) && 
	    (fabs(leftLH - v) > epsilon))
	{	  
	  leftLH = v;
	  leftRate = initialRate - k * lower_spacing;
	  k++;	  
	}

   

      k = 1;
      while(((v = calcPartialLikelihood(tr, i, initialRate + k * upper_spacing)) > rightLH) &&
	    (fabs(rightLH - v) > epsilon))
	{
	  rightLH = v;
	  rightRate = initialRate + k * upper_spacing;	 
	  k++;
	}
      
     

      if(rightLH > initialLikelihood || leftLH > initialLikelihood)
	{
	  if(rightLH > leftLH)	    
	    tr->cdta->patrat[i] = rightRate;
	  else
	    tr->cdta->patrat[i] = leftRate;
	}
      tr->cdta->patratStored[i] = tr->cdta->patrat[i];     
    }
     
 
 
  

  {     
    rateCategorize *rc = (rateCategorize *)malloc(sizeof(rateCategorize) * tr->cdta->endsite);
    int where;
    int found = 0;
    for (i = 0; i < tr->cdta->endsite; i++)
      {
	rc[i].accumulatedSiteLikelihood = 0;
	rc[i].rate = 0;
      }
      
    where = 1;   
    rc[0].accumulatedSiteLikelihood = calcPartialLikelihood(tr, 0, tr->cdta->patrat[0]);
    rc[0].rate = tr->cdta->patrat[0];
    tr->cdta->rateCategory[0] = 0;
    
    for (i = 1; i < tr->cdta->endsite; i++) 
      {
	temp = tr->cdta->patrat[i];
	found = 0;
	for(k = 0; k < where; k++)
	  {
	    if(temp == rc[k].rate || (fabs(temp - rc[k].rate) < 0.001))
	      {
		found = 1;				
		rc[k].accumulatedSiteLikelihood += 
		  calcPartialLikelihood(tr, i, temp);
		break;
	      }
	  }
	if(!found)
	  {	    
	    rc[where].rate = temp;
	    rc[where].accumulatedSiteLikelihood += 
	      calcPartialLikelihood(tr, i, temp);
	    where++;
	  }
	}

    qsort(rc, where, sizeof(rateCategorize), catCompare);

    if(where < maxCategories)
      {
	tr->NumberOfCategories = where;
	categorize(tr, rc);
      }
    else
      {
	tr->NumberOfCategories = maxCategories;	
	categorize(tr, rc);
      }

    
      
    free(rc);
  
    for (i = 0; i < tr->cdta->endsite; i++) 
      {
	/*printf("%d %f\n", i, tr->cdta->patrat[tr->cdta->rateCategory[i]]);*/
	temp = tr->cdta->patrat[tr->cdta->rateCategory[i]];

	tr->cdta->wr[i]  = wtemp = temp * tr->cdta->aliaswgt[i];
	tr->cdta->wr2[i] = temp * wtemp;
      }
      
    initrav(tr, tr->start);
    initrav(tr, tr->start->back);
    evaluate(tr, tr->start);
   

    if(tr->likelihood < initialLH)
      {
	/*printf("No improvement->restoring %f < %f\n", tr->likelihood, initialLH);*/
	tr->NumberOfCategories = oldNumber;
	for (i = 0; i < tr->cdta->endsite; i++)
	    {
	      tr->cdta->patratStored[i] = ratStored[i]; 
	      tr->cdta->rateCategory[i] = oldCategory[i];
	      tr->cdta->patrat[i] = oldRat[i];	    
	      tr->cdta->wr[i]  = oldwr[i];
	      tr->cdta->wr2[i] = oldwr2[i];
	    } 
	 	  
	initrav(tr, tr->start);
	initrav(tr, tr->start->back);
        evaluate(tr, tr->start);

      }
    }
  free(oldCategory);
  free(oldRat);
  free(ratStored);
  free(oldwr);
  free(oldwr2); 

 
}
  


void columnAnalysis(tree *tr, int i)
{
  int j;
  int counters[16];
  int val;

  for(j = 0; j < 16; j++)
    counters[j] = 0;

  for(j = 1; j <= tr->mxtips; j++)
    {
      val = tr->nodep[j]->tip[i];     
      counters[val] = counters[val] + 1;
    }


  printf("%d ", counters[1]);
  printf("%d ", counters[2]);
  printf("%d ", counters[4]);
  printf("%d ", counters[8]);
  printf("%d ", counters[15]);
  printf("\n");
}


void optimizeRateCategories2(tree *tr, int categorized, int _maxCategories)
{
  int i, k;
  double initialRate, initialLikelihood, v, leftRate, rightRate, leftLH, rightLH, temp, wtemp;   
  double spacing = 0.5 / ((double)optimizeRateCategoryInvocations); 
  int maxCategories = _maxCategories;
  double initialLH = tr->likelihood;
  double *oldRat = (double *)malloc(sizeof(double) * tr->cdta->endsite);
  double *ratStored = (double *)malloc(sizeof(double) * tr->cdta->endsite);
  double *oldwr = (double *)malloc(sizeof(double) * tr->cdta->endsite);
  double *oldwr2 = (double *)malloc(sizeof(double) * tr->cdta->endsite);
  int *oldCategory = (int *)malloc(sizeof(int) * tr->cdta->endsite);
  int oldNumber; 
  double epsilon = 0.00001;
  printf("enter CATOPT\n");
  /*fprintf(stderr,"opt rate cats %d %d %d\n", categorized, _maxCategories, tr->NumberOfCategories);*/

  optimizeRateCategoryInvocations++;


  if(spacing < 0.001)
    spacing = 0.001;
  
  oldNumber = tr->NumberOfCategories;
  for(i = 0; i < tr->cdta->endsite; i++)
    {    
      oldCategory[i] = tr->cdta->rateCategory[i];
      ratStored[i] = tr->cdta->patratStored[i];    
      oldRat[i] = tr->cdta->patrat[i];
      oldwr[i] =  tr->cdta->wr[i];
      oldwr2[i] =  tr->cdta->wr2[i];
    }

  for(i = 0; i < tr->cdta->endsite; i++)
    {    
      tr->cdta->patrat[i] = tr->cdta->patratStored[i];     
      initialRate = tr->cdta->patrat[i];
      initialLikelihood = calcPartialLikelihood(tr, i, initialRate);
      
      leftLH = rightLH = initialLikelihood;
  
      k = 1;
      while((initialRate - k * spacing > 0) && 
	    ((v = calcPartialLikelihood(tr, i, initialRate - k * spacing)) > leftLH) && (fabs(leftLH - v) > epsilon))
	{
	  leftLH = v;
	  leftRate = initialRate - k * spacing;
	  /*printf("%d %f %f\n", i, leftLH, leftRate);*/
	  k++;
	}

      

      k = 1;
      while(((v = calcPartialLikelihood(tr, i, initialRate + k * spacing)) > rightLH) && (fabs(rightLH - v) > epsilon))
	{
	  if(i == 2657)
	    {
	      printf("%f %f %f\n", v, rightLH, initialRate + k * spacing);
	    }
	  rightLH = v;
	  rightRate = initialRate + k * spacing;
	  k++;
	}
      
      printf("%d r\n", i);
      if(rightLH > initialLikelihood || leftLH > initialLikelihood)
	{
	  if(rightLH > leftLH)
	    tr->cdta->patrat[i] = rightRate;
	  else
	    tr->cdta->patrat[i] = leftRate;
	}
      tr->cdta->patratStored[i] = tr->cdta->patrat[i];

     
    }
     

  /*for(i = 0; i < tr->cdta->endsite; i++)
    {     
      if(tr->cdta->patrat[i] > 4.0)
	{
	  printf("%f %f %d %f:", tr->cdta->wr[i], tr->cdta->wr2[i], i,  tr->cdta->patrat[i]);
	  columnAnalysis(tr, i);	
	}
	}*/

  printf("categorize\n");
  {     
    rateCategorize *rc = (rateCategorize *)malloc(sizeof(rateCategorize) * tr->cdta->endsite);
    int where;
    int found = 0;
    for (i = 0; i < tr->cdta->endsite; i++)
      {
	rc[i].accumulatedSiteLikelihood = 0;
	rc[i].rate = 0;
      }
      
    where = 1;   
    rc[0].accumulatedSiteLikelihood = calcPartialLikelihood(tr, 0, tr->cdta->patrat[0]);
    rc[0].rate = tr->cdta->patrat[0];
    tr->cdta->rateCategory[0] = 0;
    
    for (i = 1; i < tr->cdta->endsite; i++) 
      {
	temp = tr->cdta->patrat[i];
	found = 0;
	for(k = 0; k < where; k++)
	  {
	    if(temp == rc[k].rate || (fabs(temp - rc[k].rate) < 0.001))
	      {
		found = 1;				
		rc[k].accumulatedSiteLikelihood += 
		  calcPartialLikelihood(tr, i, temp);
		break;
	      }
	  }
	if(!found)
	  {	    
	    rc[where].rate = temp;
	    rc[where].accumulatedSiteLikelihood += 
	      calcPartialLikelihood(tr, i, temp);
	    where++;
	  }
	}

    qsort(rc, where, sizeof(rateCategorize), catCompare);
    printf("categorize invok\n");
    if(where < maxCategories)
      {
	tr->NumberOfCategories = where;	
	categorize(tr, rc);
      }
    else
      {
	tr->NumberOfCategories = maxCategories;		
	categorize(tr, rc);
      }
      
    free(rc);
    printf("after categorize\n");
    for (i = 0; i < tr->cdta->endsite; i++) 
      {
	temp = tr->cdta->patrat[tr->cdta->rateCategory[i]];
	tr->cdta->wr[i]  = wtemp = temp * tr->cdta->aliaswgt[i];
	tr->cdta->wr2[i] = temp * wtemp;
	/*	fprintf(stderr,"%d %f\n", i, temp);*/
      }
    printf("calc new values\n");
    initrav(tr, tr->start);
    initrav(tr, tr->start->back);
    evaluate(tr, tr->start);
    
    printf("compare\n");
    if(tr->likelihood < initialLH)
      {
	fprintf(stderr,"No improvement->restoring %f < %f\n", tr->likelihood, initialLH);
	tr->NumberOfCategories = oldNumber;
	for (i = 0; i < tr->cdta->endsite; i++)
	    {
	      tr->cdta->patratStored[i] = ratStored[i]; 
	      tr->cdta->rateCategory[i] = oldCategory[i];
	      tr->cdta->patrat[i] = oldRat[i];	    
	      tr->cdta->wr[i]  = oldwr[i];
	      tr->cdta->wr2[i] = oldwr2[i];
	    } 
	 	  
	initrav(tr, tr->start);
	initrav(tr, tr->start->back);
        evaluate(tr, tr->start);	
      }
    }
  free(oldCategory);
  free(oldRat);
  free(ratStored);
  free(oldwr);
  free(oldwr2);
  printf("exit CATOPT\n"); 
}
  




int optimizeModel(tree *tr, analdef *adef, int finalOptimization)
{
  double startLH = tr->likelihood;
  double initialLH;
  int oldInv;

  switch(adef->model)
    {
    case M_GTR:
      {
	oldInv = optimizeRatesInvocations;
	do
	  {	    
	    initialLH = tr->likelihood;
	    optimizeRates(tr);       
	  }
	while(tr->likelihood > initialLH && optimizeRatesInvocations < oldInv + 10);   
      }
      break;
    case M_GTRCAT:
       {	 
	 oldInv = optimizeRatesInvocations;
	 do
	   {	    
	     initialLH = tr->likelihood;
	     optimizeRates(tr);       
	   }
	 while(tr->likelihood > initialLH && optimizeRatesInvocations < oldInv + 10);  	



	 oldInv = optimizeRateCategoryInvocations;
	 optimizeRateCategories(tr, finalOptimization, adef->categories);	
       }
      break;
    case M_HKY85:
      {
	oldInv = optimizeTTinvocations;
	
	do
	  {
	    initialLH = tr->likelihood;
	    optimizeTT(tr);
	  }
	while(tr->likelihood > initialLH && optimizeTTinvocations < oldInv + 10);      
      }
      break;
    case M_HKY85CAT:
       {
	oldInv = optimizeTTinvocations;
	
	do
	  {
	    initialLH = tr->likelihood;
	    optimizeTT(tr);
	  }
	while(tr->likelihood > initialLH && optimizeTTinvocations < oldInv + 10); 

	oldInv = optimizeRateCategoryInvocations;
	optimizeRateCategories(tr, finalOptimization, adef->categories);
       }
    }

 

  if(optimizeRatesInvocations > 90)
    optimizeRatesInvocations = 90;
  if(optimizeTTinvocations > 90)
    optimizeTTinvocations = 90;
  if(optimizeRateCategoryInvocations > 90)
    optimizeRateCategoryInvocations = 90;
  if(startLH > tr->likelihood) return 0;
  else return 1;
}

void consensusTree(bestlist *consensusList, tree *tr)
{
  
  FILE *temporary;  
  char consensusFileName[1024], str[16], infileName[1024],
    consensusCommand[1024];
  int i;
  bestlist *bestT;

  strcpy(infileName,workdir);
  strcat(infileName,"RAxML_TreesForConsensus.");
  strcat(infileName,run_id);
  
  strcpy(consensusFileName,workdir);
  strcat(consensusFileName,"RAxML_consensus.");
  strcat(consensusFileName,run_id);
  strcat(consensusFileName,".");
  sprintf(str, "%d", consensusTreeCounter);
  strcat(consensusFileName, str);
  printf("Consensus Tree written to RAxML_consensus.%s.%s\n",
	  run_id, str);

  
  strcpy(consensusCommand,"./consense ");
  strcat(consensusCommand, infileName);
  strcat(consensusCommand," "); 
  strcat(consensusCommand, consensusFileName);

  temporary = fopen(infileName, "w");

  bestT = (bestlist *) malloc(sizeof(bestlist));
  bestT->ninit = 0;
  initBestTree(bestT, 1, tr->mxtips, tr->cdta->endsite);
  saveBestTree(bestT, tr);
  
  infoFile = fopen(infoFileName, "a");

  for(i = 1; i <= consensusList->nvalid; i++)
    {
      recallBestTree(consensusList, i, tr);
      if(i == 1)
	fprintf(infoFile, "CONSENSUS(%d): best LH %f ",consensusTreeCounter, tr->likelihood);
      if(i == consensusList->nvalid)
	fprintf(infoFile, " <-> worst LH %f out of %d trees\n",tr->likelihood, consensusList->nvalid);
      tr->start = tr->nodep[1];
      printTree(temporary, tr, 0);      
    }

  fclose(infoFile);

  fflush(temporary);
  fclose(temporary);

  system(consensusCommand);

  recallBestTree(bestT, 1, tr);
  freeBestTree(bestT);
  consensusTreeCounter++;
}





void simulatedAnnealing(tree *tr, analdef *adef)
{
  double lh1, lh2, best, p1, p2, deltaf, alpha, t0;
  int repeats;
  int node, mintrav, maxtrav, rn, c_imp, selectMove, counter, topologyMove;
  bestlist *consensusList, *bestT;
  int maxR, i, statt, stati;
  int *subtreeStats, *nodeList, nodeCount;
  

  srand((unsigned int) time(NULL));

  bestT = (bestlist *) malloc(sizeof(bestlist));
  bestT->ninit = 0;
  initBestTree(bestT, 1, tr->mxtips, tr->cdta->endsite);

  maxR = adef->max_rearrange; 

  subtreeStats = (int *)malloc( 2 * tr->mxtips * sizeof(int));
  nodeList = (int *)malloc( 10 * tr->mxtips * sizeof(int));

  for(i = 0; i < 2 * tr->mxtips; i++)
    subtreeStats[i] = 0;

  nodeCount = 0;
  for(i = 0; i < 2 * tr->mxtips - 2; i++)
    {
      nodeList[i] = i + 1;
      nodeCount++;
    }
  /*for(i = 0; i < tr->mxtips - 2; i++)
    {
      nodeList[nodeCount] = tr->mxtips + 1 + i;
      nodeCount++;
      }*/

  if(buildConsensus)
    {
      consensusList = (bestlist *) malloc(sizeof(bestlist));
      consensusList->ninit = 0;
      initBestTree(consensusList, 100, tr->mxtips, tr->cdta->endsite);
    }
  
  Multiple = 0; 
  Thorough = 0;
   
  optimizeModel(tr, adef, 1);
  treeEvaluate(tr, 4);

  alpha = 0.9;
  best = unlikely;
  t0 = adef->t0;
  
  if(adef->repeats == 0)
    adef->repeats = tr->mxtips;
  else
    repeats = adef->repeats;

  

  c_imp = 0;
  counter = 0;

 

  while(1)
    {      
      lh1 = tr->likelihood;
      
      if(lh1 > best)
	{
	  best = lh1;
	  printLog(tr);	 
	}
      checkTime(adef, tr);
      resetBestTree(bestT);
      saveBestTree(bestT, tr);
      resetBestTree(bt);
     
      /*node = 1 + randomInt(2 * tr->mxtips - 2);*/

      node = nodeList[randomInt(nodeCount)];
      
      selectMove = randomInt(100);
      
      /*printf("Node %d SelectMove %d\n", node, selectMove);*/

      if(selectMove < 95)
	{
	  topologyMove = 1;
	  mintrav = 1;	  
	  maxtrav = 2 + randomInt(maxR);  	  
	  rearrangeParsimony(tr, tr->nodep[node], mintrav, maxtrav);
	  recallBestTree(bt, 1, tr);	        	  	  	 
	  evalFast(tr);
	  c_imp++;
	}
      else	
	{
	  topologyMove = 0;
	  selectMove = randomInt(100);
	  
	  if(selectMove < 95)
	    {	      
	      nodeptr p;
	      nodeptr q;
	      double z0, z;
	      
	      p = tr->nodep[node];
	      
	      q = p->back;
	      z0 = q->z;		
	      z = makenewz(tr, p, q, z0, newzpercycle); 	  
	      p->z = q->z = z;
	      evaluate(tr, tr->start);	
	    }
	  else
	    {
	      optimizeModel(tr, adef, 1);
	      treeEvaluate(tr, 4);
	    }
	}
      
     
      lh2 = tr->likelihood;

      if(lh1 > lh2)
	{	  
	  deltaf = lh2 - lh1;
	  p1 = exp( deltaf / t0);
	  rn = randomInt(10000);
	  if(rn == 0) 
	    p2 = 0;
	  else 
	    p2 = ((double)rn)/10000.0;	  	  

	  if(buildConsensus && topologyMove && p1 < p2)
	    saveBestTree(consensusList, tr);	
	  if(p1 < p2)	  
	    recallBestTree(bestT, 1, tr);	  
	  else
	    {
	      /*printf("BACKWARD %f -> %f \n ", lh1, lh2);	     	     */
	    }
	}    
      else
	{
	  if(topologyMove)
	    {
	      subtreeStats[node] =  subtreeStats[node] + 1;
	      if(buildConsensus)
		saveBestTree(consensusList, tr);
	    }
	  /* TODO introduce penalties for missed moves ?*/
	  /* increase node selection probability while computation advances ? */
	}

      if(c_imp >= repeats)
	{
	  t0 = t0 * alpha;	  
	  c_imp = 0;
	}
     
      counter++;     

      if((counter % (2 * tr->mxtips)) == 0) 	
	{
	  if(buildConsensus) 
	    consensusTree(consensusList, tr);
	  /*
	    statt = 0;
	    stati = 0;
	    for(i = 0; i < 2 * tr->mxtips; i++)
	    {
	    if(subtreeStats[i] > 0)
	    {
	    printf("%d %d\n", i, subtreeStats[i]);
	    
	    if(i <= tr->mxtips)
	    statt++;
	    else
	    stati++;
	    }
	    }
	    printf("move-nodes tips %d inner %d\n", statt, stati);*/
	  counter = 0;
	}          
    }          
}







void treeEvaluation(tree *tr, analdef *adef)
{
  int i;
  
  for(i = 0; i < 10; i++)
    {
      optimizeModel(tr, adef, 1);      
      if(i < 9)
	treeEvaluate(tr, 1);
      else
	treeEvaluate(tr, 4);           
    }
  /*printf("optimized LH: %f\n", tr->likelihood);*/
  printLog(tr);

}



void initModel(tree *tr, rawdata *rdta, cruncheddata *cdta, analdef *adef)
{
  rdta->ttratio =         2.0;
  tr->t_value = 2.0;
  switch(adef->model)
    {
    case M_HKY85:
      newview = newviewHKY85;
      makenewz = makenewzHKY85;
      evaluate = evaluateHKY85;
      empiricalfreqs(adef, rdta, cdta);
      break;
    case M_HKY85CAT:
      newview = newviewHKY85CAT;
      makenewz = makenewzHKY85CAT;
      evaluate = evaluateHKY85CAT;
      evaluatePartial = evaluatePartialHKY85CAT;
      newviewPartial =  newviewPartialHKY85CAT;
      initravPartial = initravPartialHKY85CAT;     
      empiricalfreqs(adef, rdta, cdta);
      break;
    case M_GTR:
      newview = newviewGTR;
      makenewz = makenewzGTR;
      evaluate = evaluateGTR;      
      baseFrequenciesGTR(rdta, cdta);       
      rdta->initialRates[0] = 0.5;
      rdta->initialRates[1] =  0.5;
      rdta->initialRates[2] = 0.5;
      rdta->initialRates[3] = 0.5;
      rdta->initialRates[4] = 0.5;      
      initReversibleGTR(rdta, cdta);
      break;
    case M_GTRCAT:
      newview = newviewGTRCAT;
      makenewz = makenewzGTRCAT;
      evaluate = evaluateGTRCAT;
      evaluatePartial = evaluatePartialGTRCAT;
      newviewPartial =  newviewPartialGTRCAT;
      initravPartial = initravPartialGTRCAT;
      baseFrequenciesGTR(rdta, cdta);       
      rdta->initialRates[0] = 0.5;
      rdta->initialRates[1] =  0.5;
      rdta->initialRates[2] = 0.5;
      rdta->initialRates[3] = 0.5;
      rdta->initialRates[4] = 0.5;      
      initReversibleGTR(rdta, cdta);
      break;
    }
}

int filexists(char *filename)
{
  FILE *fp = fopen(filename,"r");
  int res;
  

  if(fp) 
    {
      res = 1;
      fclose(fp);
    }
  else 
    res = 0;
    
 
   
  return res;
} 

main (int argc, char *argv[]) 
  {   
    rawdata      *rdta;
    cruncheddata  *cdta;
    tree         *tr;   
    int *perm;    
    double t;
    char *parsimonyTree;

    masterTime = gettime();

    adef = (analdef *) malloc(sizeof(analdef));
    rdta = (rawdata *)malloc(sizeof(rawdata));
    cdta = (cruncheddata *)malloc(sizeof(cruncheddata));     
    tr = (tree *)malloc(sizeof(tree));

    initAdef(adef);
    get_args(argc,argv,(boolean)1, adef, rdta);
       	    	         
    INFILE = fopen(seq_file, "r");
    if (!INFILE)
      {
	printf( "Could not open sequence file: %s\n", seq_file);
	return 1;
      }
    getinput(adef, rdta, cdta, tr);   
    fclose(INFILE);
   
    if (! makeweights(adef, rdta, cdta))                             return 1; 
    if (! makevalues(rdta, cdta))                                    return 1;

    initModel(tr, rdta, cdta, adef);
      
    if (! linkdata2tree(rdta, cdta, tr))                             return 1;
    if (! linkxarray(3, 3, cdta->endsite, & freextip, & usedxtip))   return 1;
    if (! setupnodex(tr))                                            return 1; 
    
  





#ifndef OPEN_MP
   switch(adef->model)
     {
     case  M_HKY85:    
       if( ! setupEqualityVectorHKY85(tr)) return 1;
       break;
     case M_GTR:
        if( ! setupEqualityVectorGTR(tr)) return 1; 
	break;
     }   
#endif

   
    treeStringLength = (tr->mxtips * (nmlngth+32)) + 256 + (tr->mxtips * 2);   

    perm = (int *)malloc((2 * tr->mxtips + 1) * sizeof(int));
    bt = (bestlist *) malloc(sizeof(bestlist));
    bt->ninit = 0;
    initBestTree(bt, 20, tr->mxtips, tr->cdta->endsite);
    
    strcpy(permFileName, workdir);    
    strcpy(resultFileName, workdir);
    strcpy(logFileName, workdir);
    strcpy(checkpointFileName, workdir);
    strcpy(infoFileName, workdir);
    
    strcat(permFileName, "RAxML_parsimonyTree.");
    strcat(resultFileName, "RAxML_result.");
    strcat(logFileName, "RAxML_log.");
    strcat(checkpointFileName, "RAxML_checkpoint.");
    strcat(infoFileName, "RAxML_info.");

    strcat(infoFileName, run_id);
    strcat(permFileName, run_id);
    strcat(resultFileName, run_id);
    strcat(logFileName, run_id);
    
    strcat(checkpointFileName, run_id);
    strcat(checkpointFileName,".");

    if(filexists(infoFileName))
      {
	printf("RAxML output files with the run ID <%s> already exist \n", run_id);
	printf("in directory %s ...... exiting\n", workdir);
	exit(-1);
      }

    logFile = fopen(logFileName, "w");
        
    printModelAndProgramInfo(tr, adef, argc, argv);
  
    if (adef->restart) 
      {
	INFILE = fopen(tree_file, "r");	
	if (!INFILE)
	  {
	    printf( "Could not open input tree: %s\n", tree_file);
	    return FALSE;
	  }
	if (! treeReadLen(INFILE, tr))          return FALSE;
	treeEvaluate(tr, 4);	
	fclose(INFILE);
      }
    else
      {  		    		    	   
	makePermutation(perm, tr->mxtips);	    
	RAxML_parsimony(perm, treeStringLength, &parsimonyTree);	    
	str_treeReadLenSpecial(parsimonyTree ,tr);
	treeEvaluate(tr, 4);	
	free(parsimonyTree);	    	       
	permutationFile = fopen(permFileName,"w");
	printTree(permutationFile, tr, 0);
	fclose(permutationFile);
	RAxML_parsimony_cleanup();
      }   
     
   
         
    switch(adef->mode)
      {      
      case TREE_EVALUATION : treeEvaluation(tr, adef);break; 
      case TREE_INFERENCE : compute(tr, adef);break;
      case FAST_MODE : computeFast(tr, adef); break;
      case SIMULATED_ANNEALING : simulatedAnnealing(tr, adef);break;
      case TREE_RECURSIVE: computeRecursive(tr, adef);break;	
      }
    
    fclose(logFile);
    
    printf("\n\n");

    if(checkpoints)
      {	
	if(checkpoints > 1)
	  printf("intermediate checkpoints written to: %s0-%d\n", checkpointFileName, (checkPointCounter - 1));
	else
	  printf("checkpoint written to: %s0\n", checkpointFileName); 
      }
    
    if((!adef->restart) && (!adef->mode == TREE_EVALUATION))
      printf("Parsimony starting tree written to: %s\n", permFileName);   	          
    if(!adef->mode == TREE_EVALUATION)
      printf("Final tree written to: %s\n", resultFileName);  
    printf("Execution LogFile written to: %s\n", logFileName);   
    printf("Execution information file written to: %s\n", infoFileName);
  }
