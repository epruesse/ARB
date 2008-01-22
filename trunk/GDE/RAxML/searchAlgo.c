/*  RAxML-VI-HPC (version 2.2) a program for sequential and parallel estimation of phylogenetic trees 
 *  Copyright August 2006 by Alexandros Stamatakis
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
 *  Alexandros.Stamatakis@epfl.ch
 *
 *  When publishing work that is based on the results from RAxML-VI-HPC please cite:
 *
 *  Alexandros Stamatakis:"RAxML-VI-HPC: maximum likelihood-based phylogenetic analyses with thousands of taxa and mixed models". 
 *  Bioinformatics 2006; doi: 10.1093/bioinformatics/btl446
 */

#ifndef WIN32
#include <sys/times.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h> 
#endif

#include <math.h>
#include <time.h> 
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>



#include "axml.h"

extern int Thorough;
extern int numBranches;
extern infoList iList;
extern char inverseMeaningDNA[16];
extern char seq_file[1024];
extern int multiBranch;

boolean initrav (tree *tr, nodeptr p)
{ 
  nodeptr  q;
  
  if (!isTip(p->number, tr->rdta->numsp)) 
    {      
      q = p->next;
      
      do 
	{	   
	  if (! initrav(tr, q->back))  return FALSE;		   
	  q = q->next;	
	} 
      while (q != p);  
      
      newviewGeneric(tr, p);
    }
  
  return TRUE;
} 





boolean initravDIST (tree *tr, nodeptr p, int distance)
  {
    nodeptr  q;

    if (/*! p->tip*/ !isTip(p->number, tr->rdta->numsp) && distance > 0) 
      {
      
	q = p->next;
      
	do 
	  {
	    if (! initravDIST(tr, q->back, --distance))  return FALSE;	
	    q = q->next;	
	  } 
	while (q != p);
      
      
	newviewGeneric(tr, p);
      }

    return TRUE;
  } /* initrav */

void initravPartition (tree *tr, nodeptr p, int model)
{
  nodeptr  q;
  
  if (/*!p->tip*/ !isTip(p->number, tr->rdta->numsp)) 
    {      
      q = p->next;      
      do 
	{
	  initravPartition(tr, q->back, model);
	  q = q->next;	
	} 
      while (q != p);
    
      newviewPartitionGeneric(tr, p, model);       
    }
} 





boolean update(tree *tr, nodeptr p)
{       
  nodeptr  q;
  boolean smoothed;
  int i;
  double   z[NUM_BRANCHES], z0[NUM_BRANCHES];
  
  q = p->back;   

  for(i = 0; i < numBranches; i++)
    z0[i] = q->z[i];

  makenewzGeneric(tr, p, q, z0, newzpercycle, z);  
  
  smoothed = tr->smoothed;

  for(i = 0; i < numBranches; i++)
    {      
      if (ABS(z[i] - z0[i]) > deltaz)  
	smoothed = FALSE;
      p->z[i] = q->z[i] = z[i];      
    }
  
  tr->smoothed = smoothed;
  
  return TRUE;
}




boolean smooth (tree *tr, nodeptr p)
{
  nodeptr  q;
  
  if (! update(tr, p))               return FALSE; /*  Adjust branch */
  if (/*! p->tip*/ ! isTip(p->number, tr->rdta->numsp)) 
    {                                  /*  Adjust descendants */
      q = p->next;
      while (q != p) 
	{
	  if (! smooth(tr, q->back))   return FALSE;
	  q = q->next;
	}	
      
      newviewGeneric(tr, p);
    }
  
  return TRUE;
} 


boolean smoothTree (tree *tr, int maxtimes)
  { /* smoothTree */
    nodeptr  p, q;   

    /*printf("smoothTree %d \n", maxtimes);*/

    p = tr->start;

    while (--maxtimes >= 0) 
      {
	tr->smoothed = TRUE;
	if (! smooth(tr, p->back))       return FALSE;
	if (/*! p->tip*/ !isTip(p->number, tr->rdta->numsp)) 
	  {
	    q = p->next;
	    while (q != p) 
	      {
		if (! smooth(tr, q->back))   return FALSE;
		q = q->next;
	      }
	  }
	if (tr->smoothed)  break;
      }

    return TRUE;
  } /* smoothTree */



boolean localSmooth (tree *tr, nodeptr p, int maxtimes)
{ 
  nodeptr  q;
  
  if (/*p->tip*/ isTip(p->number, tr->rdta->numsp)) return FALSE;            /* Should be an error */
  
  while (--maxtimes >= 0) 
    {
      tr->smoothed = TRUE;
      q = p;
      do 
	{
	  if (! update(tr, q)) return FALSE;
	  q = q->next;
        } 
      while (q != p);
      if (tr->smoothed)  break;
    }

  tr->smoothed = FALSE;             /* Only smooth locally */
  return TRUE;
}






static void resetInfoList(void)
{
  int i;

  iList.valid = 0;

  for(i = 0; i < iList.n; i++)    
    {
      iList.list[i].node = (nodeptr)NULL;
      iList.list[i].likelihood = unlikely;
    }    
}

void initInfoList(int n)
{
  int i;

  iList.n = n;
  iList.valid = 0;
  iList.list = (bestInfo *)malloc(sizeof(bestInfo) * n);

  for(i = 0; i < n; i++)
    {
      iList.list[i].node = (nodeptr)NULL;
      iList.list[i].likelihood = unlikely;
    }
}

void freeInfoList(void)
{ 
  free(iList.list);   
}


void insertInfoList(nodeptr node, double likelihood)
{
  int i;
  int min = 0;
  double min_l =  iList.list[0].likelihood;

  for(i = 1; i < iList.n; i++)
    {
      if(iList.list[i].likelihood < min_l)
	{
	  min = i;
	  min_l = iList.list[i].likelihood;
	}
    }

  if(likelihood > min_l)
    {
      iList.list[min].likelihood = likelihood;
      iList.list[min].node = node;
      iList.valid += 1;
    }

  if(iList.valid > iList.n)
    iList.valid = iList.n;
}


boolean smoothRegion (tree *tr, nodeptr p, int region)
{ 
  nodeptr  q;
  
  if (! update(tr, p))               return FALSE; /*  Adjust branch */

  if(region > 0)
    {
      if (/*! p->tip*/  !isTip(p->number, tr->rdta->numsp)) 
	{                                 
	  q = p->next;
	  while (q != p) 
	    {
	      if (! smoothRegion(tr, q->back, --region))   return FALSE;
	      q = q->next;
	    }	
	  
	    newviewGeneric(tr, p);
	}
    }
  
  return TRUE;
}

boolean regionalSmooth (tree *tr, nodeptr p, int maxtimes, int region)
  {
    nodeptr  q;

    if (/*p->tip*/ isTip(p->number, tr->rdta->numsp)) return FALSE;            /* Should be an error */

    while (--maxtimes >= 0) 
      {
	tr->smoothed = TRUE;
	q = p;
	do 
	  {
	    if (! smoothRegion(tr, q, region)) return FALSE;
	    q = q->next;
	  } 
	while (q != p);
	if (tr->smoothed)  break;
      }

    tr->smoothed = FALSE;             /* Only smooth locally */
    return TRUE;
  } /* localSmooth */





nodeptr  removeNodeBIG (tree *tr, nodeptr p)
{  
  double   zqr[NUM_BRANCHES], result[NUM_BRANCHES];
  nodeptr  q, r;
  int i;
        
  q = p->next->back;
  r = p->next->next->back;
  
  for(i = 0; i < numBranches; i++)
    zqr[i] = q->z[i] * r->z[i];        
   
  makenewzGeneric(tr, q, r, zqr, iterations, result);   

  for(i = 0; i < numBranches; i++)        
    tr->zqr[i] = result[i];

  hookup(q, r, result); 
      
  p->next->next->back = p->next->back = (node *) NULL;

  return  q; 
}

nodeptr  removeNodeRestoreBIG (tree *tr, nodeptr p)
{
  nodeptr  q, r;
        
  q = p->next->back;
  r = p->next->next->back;  

  newviewGeneric(tr, q);
  newviewGeneric(tr, r);
  
  hookup(q, r, tr->currentZQR);

  p->next->next->back = p->next->back = (node *) NULL;
     
  return  q;
}


boolean insertBIG (tree *tr, nodeptr p, nodeptr q)
{
  nodeptr  r, s;
  int i;
  
  r = q->back;
  s = p->back;
      
  for(i = 0; i < numBranches; i++)
    tr->lzi[i] = q->z[i];
  
  if(Thorough)
    { 
      double  zqr[NUM_BRANCHES], zqs[NUM_BRANCHES], zrs[NUM_BRANCHES], lzqr, lzqs, lzrs, lzsum, lzq, lzr, lzs, lzmax;      
      double defaultArray[NUM_BRANCHES];	
      double e1[NUM_BRANCHES], e2[NUM_BRANCHES], e3[NUM_BRANCHES];
      double *qz;
      
      qz = q->z;
      
      for(i = 0; i < numBranches; i++)
	defaultArray[i] = defaultz;
      
      makenewzGeneric(tr, q, r, qz, iterations, zqr);           
      makenewzGeneric(tr, q, s, defaultArray, iterations, zqs);                  
      makenewzGeneric(tr, r, s, defaultArray, iterations, zrs);
      
      
      for(i = 0; i < numBranches; i++)
	{
	  lzqr = (zqr[i] > zmin) ? log(zqr[i]) : log(zmin); 
	  lzqs = (zqs[i] > zmin) ? log(zqs[i]) : log(zmin);
	  lzrs = (zrs[i] > zmin) ? log(zrs[i]) : log(zmin);
	  lzsum = 0.5 * (lzqr + lzqs + lzrs);
	  
	  lzq = lzsum - lzrs;
	  lzr = lzsum - lzqs;
	  lzs = lzsum - lzqr;
	  lzmax = log(zmax);
	  
	  if      (lzq > lzmax) {lzq = lzmax; lzr = lzqr; lzs = lzqs;} 
	  else if (lzr > lzmax) {lzr = lzmax; lzq = lzqr; lzs = lzrs;}
	  else if (lzs > lzmax) {lzs = lzmax; lzq = lzqs; lzr = lzrs;}          
	  
	  e1[i] = exp(lzq);
	  e2[i] = exp(lzr);
	  e3[i] = exp(lzs);
	}
      hookup(p->next,       q, e1);
      hookup(p->next->next, r, e2);
      hookup(p,             s, e3);      		  
    }
  else
    {       
      double  z[NUM_BRANCHES]; 
      
      for(i = 0; i < numBranches; i++)
	{
	  z[i] = sqrt(q->z[i]);      
	  
	  if(z[i] < zmin) 
	    z[i] = zmin;
	  if(z[i] > zmax)
	    z[i] = zmax;
	}
      
      hookup(p->next,       q, z);
      hookup(p->next->next, r, z);	                         
    }
  
  newviewGeneric(tr, p);
  
  if(Thorough)
    {     
      localSmooth(tr, p, smoothings);   
      for(i = 0; i < numBranches; i++)
	{
	  tr->lzq[i] = p->next->z[i];
	  tr->lzr[i] = p->next->next->z[i];
	  tr->lzs[i] = p->z[i];            
	}
    }           
  
  return  TRUE;
}

boolean insertRestoreBIG (tree *tr, nodeptr p, nodeptr q)
{
  nodeptr  r, s;
  
  r = q->back;
  s = p->back;

  if(Thorough)
    {                        
      hookup(p->next,       q, tr->currentLZQ);
      hookup(p->next->next, r, tr->currentLZR);
      hookup(p,             s, tr->currentLZS);      		  
    }
  else
    {       
      double  z[NUM_BRANCHES];
      int i;
      
      for(i = 0; i < numBranches; i++)
	{
	  double zz;
	  zz = sqrt(q->z[i]);     
	  if(zz < zmin) 
	    zz = zmin;
	  if(zz > zmax)
	    zz = zmax;
  	  z[i] = zz;
	}

      hookup(p->next,       q, z);
      hookup(p->next->next, r, z);
    }   
    
  newviewGeneric(tr, p);
       
  return  TRUE;
}


void restoreTopologyOnly(tree *tr, bestlist *bt)
{ 
  nodeptr p = tr->removeNode;
  nodeptr q = tr->insertNode;
  double qz[NUM_BRANCHES], pz[NUM_BRANCHES], p1z[NUM_BRANCHES], p2z[NUM_BRANCHES];
  nodeptr p1, p2, r, s;
  double currentLH = tr->likelihood;
  int i;
      
  p1 = p->next->back;
  p2 = p->next->next->back;
  
  for(i = 0; i < numBranches; i++)
    {
      p1z[i] = p1->z[i];
      p2z[i] = p2->z[i];
    }
  
  hookup(p1, p2, tr->currentZQR);
  
  p->next->next->back = p->next->back = (node *) NULL;             
  for(i = 0; i < numBranches; i++)
    {
      qz[i] = q->z[i];
      pz[i] = p->z[i];           
    }
  
  r = q->back;
  s = p->back;
  
  if(Thorough)
    {                        
      hookup(p->next,       q, tr->currentLZQ);
      hookup(p->next->next, r, tr->currentLZR);
      hookup(p,             s, tr->currentLZS);      		  
    }
  else
    { 	
      double  z[NUM_BRANCHES];	
      for(i = 0; i < numBranches; i++)
	{
	  z[i] = sqrt(q->z[i]);      
	  if(z[i] < zmin)
	    z[i] = zmin;
	  if(z[i] > zmax)
	    z[i] = zmax;
	}
      hookup(p->next,       q, z);
      hookup(p->next->next, r, z);
    }     
  
  tr->likelihood = tr->bestOfNode;
  
  saveBestTree(bt, tr);
  
  tr->likelihood = currentLH;
  
  hookup(q, r, qz);
  
  p->next->next->back = p->next->back = (nodeptr) NULL;
  
  if(Thorough)    
    hookup(p, s, pz);          
      
  hookup(p->next,       p1, p1z); 
  hookup(p->next->next, p2, p2z);      
}


boolean testInsertBIG (tree *tr, nodeptr p, nodeptr q)
{
  double  qz[NUM_BRANCHES], pz[NUM_BRANCHES];
  nodeptr  r;
  boolean doIt = TRUE;
  double startLH = tr->endLH;
  int i;
  
  r = q->back; 
  for(i = 0; i < numBranches; i++)
    {
      qz[i] = q->z[i];
      pz[i] = p->z[i];
    }
  
  if(tr->grouped)
    {
      int rNumber, qNumber, pNumber;
      
      doIt = FALSE;
      
      rNumber = tr->constraintVector[r->number];
      qNumber = tr->constraintVector[q->number];
      pNumber = tr->constraintVector[p->number];
      
      if(pNumber == -9)
	pNumber = checker(tr, p->back);
      if(pNumber == -9)
	doIt = TRUE;
      else
	{
	  if(qNumber == -9)
	    qNumber = checker(tr, q);
	  
	  if(rNumber == -9)
	    rNumber = checker(tr, r);
	  
	  if(pNumber == rNumber || pNumber == qNumber)
	    doIt = TRUE;       
	}
    }
  
  if(doIt)
    {     
      if (! insertBIG(tr, p, q))       return FALSE;         
      
      evaluateGeneric(tr, p->next->next);            
      		   
      if(tr->likelihood > tr->bestOfNode)
	{
	  tr->bestOfNode = tr->likelihood;
	  tr->insertNode = q;
	  tr->removeNode = p;   
	  for(i = 0; i < numBranches; i++)
	    {
	      tr->currentZQR[i] = tr->zqr[i];           
	      tr->currentLZR[i] = tr->lzr[i];
	      tr->currentLZQ[i] = tr->lzq[i];
	      tr->currentLZS[i] = tr->lzs[i];      
	    }
	}
      
      if(tr->likelihood > tr->endLH)
	{			  
	  tr->insertNode = q;
	  tr->removeNode = p;   
	  for(i = 0; i < numBranches; i++)
	    tr->currentZQR[i] = tr->zqr[i];      
	  tr->endLH = tr->likelihood;                      
	}        
      
      hookup(q, r, qz);
      
      p->next->next->back = p->next->back = (nodeptr) NULL;
      
      if(Thorough)
	{
	  nodeptr s = p->back;
	  hookup(p, s, pz);      
	} 
      
      if((tr->doCutoff) && (tr->likelihood < startLH))
	{
	  tr->lhAVG += (startLH - tr->likelihood);
	  tr->lhDEC++;
	  if((startLH - tr->likelihood) >= tr->lhCutoff)
	    return FALSE;	    
	  else
	    return TRUE;
	}
      else
	return TRUE;
    }
  else
    return TRUE;  
}



 
void addTraverseBIG(tree *tr, nodeptr p, nodeptr q, int mintrav, int maxtrav)
{  
  if (--mintrav <= 0) 
    {              
      if (! testInsertBIG(tr, p, q))  return;        
    }
  
  if ((/*! q->tip*/ !isTip(q->number, tr->rdta->numsp)) && (--maxtrav > 0)) 
    {    
      addTraverseBIG(tr, p, q->next->back, mintrav, maxtrav);
      addTraverseBIG(tr, p, q->next->next->back, mintrav, maxtrav);    
    }
} 





int rearrangeBIG(tree *tr, nodeptr p, int mintrav, int maxtrav)   
{  
  double   p1z[NUM_BRANCHES], p2z[NUM_BRANCHES], q1z[NUM_BRANCHES], q2z[NUM_BRANCHES];
  nodeptr  p1, p2, q, q1, q2;
  int      mintrav2, i;  
  boolean doP = TRUE, doQ = TRUE;
  
  if (maxtrav < 1 || mintrav > maxtrav)  return 0;
  q = p->back;
  
  if(tr->constrained)
    {
      if(! tipHomogeneityChecker(tr, p->back, 0))
	doP = FALSE;
      
      if(! tipHomogeneityChecker(tr, q->back, 0))
	doQ = FALSE;
      
      if(doQ == FALSE && doP == FALSE)
	return 0;
    }
  
  if (/*!p->tip*/ !isTip(p->number, tr->rdta->numsp) && doP) 
    {     
      p1 = p->next->back;
      p2 = p->next->next->back;
      
      /*if (! p1->tip || ! p2->tip) */
      if(!isTip(p1->number, tr->rdta->numsp) || !isTip(p2->number, tr->rdta->numsp))
	{
	  for(i = 0; i < numBranches; i++)
	    {
	      p1z[i] = p1->z[i];
	      p2z[i] = p2->z[i];	   	   
	    }
	  
	  if (! removeNodeBIG(tr, p)) return badRear;
	  
	  if (/*! p1->tip*/ !isTip(p1->number, tr->rdta->numsp)) 
	    {
	      addTraverseBIG(tr, p, p1->next->back,
			     mintrav, maxtrav);         
	      addTraverseBIG(tr, p, p1->next->next->back,
			     mintrav, maxtrav);          
	    }
	  
	  if (/*! p2->tip*/ !isTip(p2->number, tr->rdta->numsp)) 
	    {
	      addTraverseBIG(tr, p, p2->next->back,
			     mintrav, maxtrav);
	      addTraverseBIG(tr, p, p2->next->next->back,
			     mintrav, maxtrav);          
	    }
	  	  
	  hookup(p->next,       p1, p1z); 
	  hookup(p->next->next, p2, p2z);	   	    	    
	  initravDIST(tr, p, 1);	   	    
	}
    }  
  
  if (/*! q->tip*/ !isTip(q->number, tr->rdta->numsp) && maxtrav > 0 && doQ) 
    {
      q1 = q->next->back;
      q2 = q->next->next->back;
      
      /*if (((!q1->tip) && (!q1->next->back->tip || !q1->next->next->back->tip)) ||
	((!q2->tip) && (!q2->next->back->tip || !q2->next->next->back->tip))) */
      if (
	  (
	   ! isTip(q1->number, tr->rdta->numsp) && 
	   (! isTip(q1->next->back->number, tr->rdta->numsp) || ! isTip(q1->next->next->back->number, tr->rdta->numsp))
	   )
	  ||
	  (
	   ! isTip(q2->number, tr->rdta->numsp) && 
	   (! isTip(q2->next->back->number, tr->rdta->numsp) || ! isTip(q2->next->next->back->number, tr->rdta->numsp))
	   )
	  )
	{
	  
	  for(i = 0; i < numBranches; i++)
	    {
	      q1z[i] = q1->z[i];
	      q2z[i] = q2->z[i];
	    }
	  
	  if (! removeNodeBIG(tr, q)) return badRear;
	  
	  mintrav2 = mintrav > 2 ? mintrav : 2;
	  
	  if (/*! q1->tip*/ !isTip(q1->number, tr->rdta->numsp)) 
	    {
	      addTraverseBIG(tr, q, q1->next->back,
			     mintrav2 , maxtrav);
	      addTraverseBIG(tr, q, q1->next->next->back,
			     mintrav2 , maxtrav);         
	    }
	  
	  if (/*! q2->tip*/ ! isTip(q2->number, tr->rdta->numsp)) 
	    {
	      addTraverseBIG(tr, q, q2->next->back,
			     mintrav2 , maxtrav);
	      addTraverseBIG(tr, q, q2->next->next->back,
			     mintrav2 , maxtrav);          
	    }	   
	  
	  hookup(q->next,       q1, q1z); 
	  hookup(q->next->next, q2, q2z);
	  
	  initravDIST(tr, q, 1); 	   
	}
    } 
  
  return  1;
} 




double treeOptimizeRapid(tree *tr, int mintrav, int maxtrav, analdef *adef, bestlist *bt)
{
  int i, index,
    *perm = (int*)NULL;   

  nodeRectifier(tr);

  if (maxtrav > tr->ntips - 3)  
    maxtrav = tr->ntips - 3;  
    
  resetInfoList();
  
  resetBestTree(bt);
 
  tr->startLH = tr->endLH = tr->likelihood;
 
  if(tr->doCutoff)
    {
      if(tr->bigCutoff)
	{	  
	  if(tr->itCount == 0)    
	    tr->lhCutoff = 0.5 * (tr->likelihood / -1000.0);    
	  else    		 
	    tr->lhCutoff = 0.5 * ((tr->lhAVG) / ((double)(tr->lhDEC))); 	  
	}
      else
	{
	  if(tr->itCount == 0)    
	    tr->lhCutoff = tr->likelihood / -1000.0;    
	  else    		 
	    tr->lhCutoff = (tr->lhAVG) / ((double)(tr->lhDEC));   
	}    

      tr->itCount = tr->itCount + 1;
      tr->lhAVG = 0;
      tr->lhDEC = 0;
    }
  
  if(adef->permuteTreeoptimize)
    {
      int n = tr->mxtips + tr->mxtips - 2;   
      perm = (int *)malloc(sizeof(int) * (n + 1));
      makePermutation(perm, n, adef);
    }

  for(i = 1; i <= tr->mxtips + tr->mxtips - 2; i++)
    {           
      tr->bestOfNode = unlikely;          

      if(adef->permuteTreeoptimize)
	index = perm[i];
      else
	index = i;
      
      if(rearrangeBIG(tr, tr->nodep[index], mintrav, maxtrav))
	{    
	  if(Thorough)
	    {
	      if(tr->endLH > tr->startLH)                 	
		{			   	     
		  restoreTreeFast(tr);	 	 
		  tr->startLH = tr->endLH = tr->likelihood;	 
		  saveBestTree(bt, tr);
		}
	      else
		{ 		  
		  if(tr->bestOfNode != unlikely)		    	     
		    restoreTopologyOnly(tr, bt);		    
		}	   
	    }
	  else
	    {
	      insertInfoList(tr->nodep[index], tr->bestOfNode);	    
	      if(tr->endLH > tr->startLH)                 	
		{		      
		  restoreTreeFast(tr);	  	      
		  tr->startLH = tr->endLH = tr->likelihood;	  	 	  	  	  	  	  	  
		}	    	  
	    }
	}     
    }
   


  if(!Thorough)
    {           
      Thorough = 1;  
      
      for(i = 0; i < iList.valid; i++)
	{      
	 
	  tr->bestOfNode = unlikely;
	  
	  if(rearrangeBIG(tr, iList.list[i].node, mintrav, maxtrav))
	    {	  
	      if(tr->endLH > tr->startLH)                 	
		{	 	     
		  restoreTreeFast(tr);	 	 
		  tr->startLH = tr->endLH = tr->likelihood;	 
		  saveBestTree(bt, tr);
		}
	      else
		{ 
	      
		  if(tr->bestOfNode != unlikely)
		    {	     
		      restoreTopologyOnly(tr, bt);
		    }	
		}      
	    }
	}       
          
      Thorough = 0;
    }

  if(adef->permuteTreeoptimize)
    free(perm);

  return tr->startLH;     
}


boolean testInsertRestoreBIG (tree *tr, nodeptr p, nodeptr q)
{    
  if(Thorough)
    {
      if (! insertBIG(tr, p, q))       return FALSE;    
      
      evaluateGeneric(tr, p->next->next);          


      /*	
	if (! insertRestoreBIG(tr, p, q))       return FALSE;
	
	{
	nodeptr x, y;
	
	x = p->next->next;
	y = p->back;
	while ((! x->x) || (! y->x)) 
	{
	if(! (x->x))
	newviewGeneric(tr, x);
	if (! (y->x)) 
	newviewGeneric(tr, y);
	}
	}
	
	tr->likelihood = tr->endLH;
      */

    }
  else
    {
      if (! insertRestoreBIG(tr, p, q))       return FALSE;
      
      {
	nodeptr x, y;
	x = p->next->next;
	y = p->back;
			
	/*if(!x->tip && y->tip)*/
	if(! isTip(x->number, tr->rdta->numsp) && isTip(y->number, tr->rdta->numsp))
	  {
	    while ((! x->x)) 
	      {
		if (! (x->x))
		  newviewGeneric(tr, x);		     
	      }
	  }
	/*if(x->tip && !y->tip)*/
	if(isTip(x->number, tr->rdta->numsp) && !isTip(y->number, tr->rdta->numsp))
	  {
	    while ((! y->x)) 
	      {		  
		if (! (y->x))
		  newviewGeneric(tr, y);
	      }
	  }
	/*if(!x->tip && !y->tip)*/
	if(!isTip(x->number, tr->rdta->numsp) && !isTip(y->number, tr->rdta->numsp))
	  {
	    while ((! x->x) || (! y->x)) 
	      {
		if (! (x->x))
		  newviewGeneric(tr, x);
		if (! (y->x))
		  newviewGeneric(tr, y);
	      }
	  }				      	
	
      }
	
      tr->likelihood = tr->endLH;
    }
     
  return TRUE;
} 

void restoreTreeFast(tree *tr)
{
  removeNodeRestoreBIG(tr, tr->removeNode);    
  testInsertRestoreBIG(tr, tr->removeNode, tr->insertNode);
}


int determineRearrangementSetting(tree *tr,  analdef *adef, bestlist *bestT, bestlist *bt)
{
  int i, mintrav, maxtrav, bestTrav, impr, index, MaxFast,
    *perm = (int*)NULL;
  double startLH; 
  boolean cutoff;  

  MaxFast = 26;

  startLH = tr->likelihood;

  cutoff = tr->doCutoff;
  tr->doCutoff = FALSE;
 
    
  mintrav = 1;
  maxtrav = 5;

  bestTrav = maxtrav = 5;

  impr = 1;

  resetBestTree(bt);

  if(adef->permuteTreeoptimize)
    {
      int n = tr->mxtips + tr->mxtips - 2;   
      perm = (int *)malloc(sizeof(int) * (n + 1));
      makePermutation(perm, n, adef);
    }
  

  while(impr && maxtrav < MaxFast)
    {	
      recallBestTree(bestT, 1, tr);     
      
      /* TODO, why are nodes not rectified here ? */
      
      if (maxtrav > tr->ntips - 3)  
	maxtrav = tr->ntips - 3;    
 
      tr->startLH = tr->endLH = tr->likelihood;
          
      for(i = 1; i <= tr->mxtips + tr->mxtips - 2; i++)
	{                

	  if(adef->permuteTreeoptimize)
	    index = perm[i];
	  else
	    index = i;	 	 

	  tr->bestOfNode = unlikely;
	  if(rearrangeBIG(tr, tr->nodep[index], mintrav, maxtrav))
	    {	     
	      if(tr->endLH > tr->startLH)                 	
		{		 	 	      
		  restoreTreeFast(tr);	        	  	 	  	      
		  tr->startLH = tr->endLH = tr->likelihood;	  	 	  	  	  	  	  	  	      
		}	         	       	
	    }
	}
      
      treeEvaluate(tr, 0.25);
      saveBestTree(bt, tr);                                    

      /*printf("DETERMINE_BEST: %d %f\n", maxtrav, tr->likelihood);*/

      if(tr->likelihood > startLH)
	{	 
	  startLH = tr->likelihood; 	  	  	  
	  printLog(tr, adef, FALSE);	  
	  bestTrav = maxtrav;	 
	  impr = 1;
	}
      else
	{
	  impr = 0;
	}
      maxtrav += 5;
      
      if(tr->doCutoff)
	{
	  tr->lhCutoff = (tr->lhAVG) / ((double)(tr->lhDEC));       
  
	  tr->itCount =  tr->itCount + 1;
	  tr->lhAVG = 0;
	  tr->lhDEC = 0;
	}
    }

  recallBestTree(bt, 1, tr);   
  tr->doCutoff = cutoff;

  if(adef->permuteTreeoptimize)
    free(perm);

  
  return bestTrav;     
}


#ifdef _MULTI_GENE
static void analyzeMultiGene(tree *tr)
{
  int model, i, j;
  boolean complete;

  assert(tr->NumberOfModels > 1 && multiBranch);
  for(i = 1; i <= tr->mxtips; i++)
    {
      char *tip = tr->yVector[i];
      for(model = 0; model < tr->NumberOfModels; model++)
	{
	  int lower = tr->partitionData[model].lower;
	  int upper = tr->partitionData[model].upper;
	  char missing = 1;

	  for(j = lower; j < upper && missing; j++)
	    {
	      if(tip[j] != 15)		
		missing = 0;		
	    }

	  tr->tipMissing[i][model] = missing;

	  if(missing)
	    {
	      printf("Seq %d part %d completely missing\n", i, model);	      
	    }	      
	}
    }

  /* now let's see if we can find at least one complete sequence */ 

  for(i = 1; i <= tr->mxtips; i++)
    {
      complete = TRUE;

      for(model = 0; model < tr->NumberOfModels && complete; model++)	
	{
	  if(tr->tipMissing[i][model])
	    complete = FALSE;	
	}

      if(complete)
	{
	  int j;
	  tr->start = tr->nodep[i];
	  printf("%d: ", i);
	  for(j = 0; j < tr->NumberOfModels; j++)
	    {
	      printf("%d ", tr->tipMissing[i][j]);
	      tr->startVector[j] = tr->nodep[i];
	    }
	  printf("\n");
	  break;
	}
      else
	{
	  printf("Tip %d missing part %d\n", i, model);
	}
    }

  if(complete)
    return;
  else
    {
      printf("TODO\n");
      assert(0);
    }
}

static void reduceTreeModelREC(nodeptr p, int model)
{
  nodeptr q = p->next;
  nodeptr r = p->next->next;

  assert(p = p->next->next->next);

  int left  = containsModel(q->back, model);
  int right = containsModel(r->back, model);
 
  if(left && right)
    {
      q->backs[model]       = q->back;
      q->back->backs[model] = q;
      reduceTreeModelREC(q->back, model);

      r->backs[model]       = r->back;
      r->back->backs[model] = r;
      reduceTreeModelREC(r->back, model);
    }
  else
    
  

}


static void reduceTreeModel(tree *tr, int model)
{
  assert(isTip(tr->startVector[model]->number, tr->mxtips);

  reduceTreeModelREC(tr->startVector[model]->back, model);
}

static void printTreeModel(tree *tr, int model)
{
  printf("Tree %d\n", model);
}

static void treeReduction(tree *tr)
{
  int model;

  for(model = 0; model < tr->NumberOfModels; model++)
    {
      reduceTreeModel(tr, model);
      printTreeModel(tr, model);
    }

}

#endif


void computeBIGRAPID (tree *tr, analdef *adef) 
{ 
  int i,  impr, bestTrav,
    rearrangementsMax = 0, 
    rearrangementsMin = 0;
   
  double lh, previousLh, difference, epsilon;              
  bestlist *bestT, *bt;  
  
  bestT = (bestlist *) malloc(sizeof(bestlist));
  bestT->ninit = 0;
  initBestTree(bestT, 1, tr->mxtips);
      
  bt = (bestlist *) malloc(sizeof(bestlist));      
  bt->ninit = 0;
  initBestTree(bt, 20, tr->mxtips); 

  initInfoList(50);
 
  difference = 10.0;
  epsilon = 0.01;    
    
  Thorough = 0; 

 

  optimizeModel(tr, adef);     
  
  treeEvaluate(tr, 2);   
 
  printLog(tr, adef, FALSE);  

  saveBestTree(bestT, tr);
  
#ifdef _MULTI_GENE
  analyzeMultiGene(tr);
  exit(1);
#endif

  if(!adef->initialSet)   
    bestTrav = adef->bestTrav = determineRearrangementSetting(tr, adef, bestT, bt);                   
  else
    bestTrav = adef->bestTrav = adef->initial;



  saveBestTree(bestT, tr); 
  impr = 1;
  if(tr->doCutoff)
    tr->itCount = 0;
 
  while(impr)
    {              
      recallBestTree(bestT, 1, tr);     
      optimizeModel(tr, adef);            
      treeEvaluate(tr, 2);	 	      
      saveBestTree(bestT, tr);           
      printLog(tr, adef, FALSE);            
      printResult(tr, adef, FALSE);    
      lh = previousLh = tr->likelihood;
   
     
      treeOptimizeRapid(tr, 1, bestTrav, adef, bt);   
      
      impr = 0;
	  
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

  Thorough = 1;
  impr = 1;
  
  while(1)
    {		
      recallBestTree(bestT, 1, tr);    
      if(impr)
	{	    
	  printResult(tr, adef, FALSE);
	  rearrangementsMin = 1;
	  rearrangementsMax = adef->stepwidth;	    
	}			  			
      else
	{		       	   
	  rearrangementsMax += adef->stepwidth;
	  rearrangementsMin += adef->stepwidth; 	        	      
	  if(rearrangementsMax > adef->max_rearrange)	     	     	 
	    goto cleanup; 	   
	}
      
      optimizeModel(tr, adef);       
      treeEvaluate(tr, 2.0);	      
      previousLh = lh = tr->likelihood;	      
      saveBestTree(bestT, tr);     
      printLog(tr, adef, FALSE);

      treeOptimizeRapid(tr, rearrangementsMin, rearrangementsMax, adef, bt);
	
      impr = 0;			      	
		
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
  freeBestTree(bestT);
  free(bestT);
  freeBestTree(bt);
  free(bt);
  freeInfoList();
  printLog(tr, adef, FALSE);
  printResult(tr, adef, FALSE);
}


void computeBIGRAPIDMULTIBOOT (tree *tr, analdef *adef) 
{ 
  int i,  impr, bestTrav,
    rearrangementsMax = 0, 
    rearrangementsMin = 0;
   
  double lh, previousLh, difference, epsilon;              
  bestlist *bestT, *bt;    
  
  bestT = (bestlist *) malloc(sizeof(bestlist));
  bestT->ninit = 0;
  initBestTree(bestT, 1, tr->mxtips);
      
  bt = (bestlist *) malloc(sizeof(bestlist));      
  bt->ninit = 0;
  initBestTree(bt, 20, tr->mxtips); 

  initInfoList(50);
 
  difference = 10.0;
  epsilon = 0.01;    
    
  Thorough = 0; 
 
  treeEvaluate(tr, 2);   
   
  printLog(tr, adef, FALSE);  

  saveBestTree(bestT, tr);

  if(!adef->initialSet)   
    bestTrav = adef->bestTrav = determineRearrangementSetting(tr, adef, bestT, bt);                   
  else
    bestTrav = adef->bestTrav = adef->initial;

  saveBestTree(bestT, tr); 
  impr = 1;
  if(tr->doCutoff)
    tr->itCount = 0;

  while(impr)
    {              
      recallBestTree(bestT, 1, tr);      
      treeEvaluate(tr, 2);	
      /*printf("%f\n", tr->likelihood);*/
      saveBestTree(bestT, tr);     
      printLog(tr, adef, FALSE);     
      printResult(tr, adef, FALSE);    
      lh = previousLh = tr->likelihood;
         
      treeOptimizeRapid(tr, 1, bestTrav, adef, bt);   
      
      impr = 0;
	  
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

  Thorough = 1;
  impr = 1;
  
  while(1)
    {		
      recallBestTree(bestT, 1, tr);    
      if(impr)
	{	    
	  printResult(tr, adef, FALSE);
	  rearrangementsMin = 1;
	  rearrangementsMax = adef->stepwidth;	    
	}			  			
      else
	{		       	   
	  rearrangementsMax += adef->stepwidth;
	  rearrangementsMin += adef->stepwidth; 	        	      
	  if(rearrangementsMax > adef->max_rearrange)	     	     	 
	    goto cleanup; 	   
	}
               
      treeEvaluate(tr, 2.0);
      /*printf("%f\n", tr->likelihood);*/
      previousLh = lh = tr->likelihood;	      
      saveBestTree(bestT, tr);     
      printLog(tr, adef, FALSE);

      treeOptimizeRapid(tr, rearrangementsMin, rearrangementsMax, adef, bt);
	
      impr = 0;			      	
		
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
  freeBestTree(bestT);
  free(bestT);
  freeBestTree(bt);
  free(bt);
  freeInfoList();
  printLog(tr, adef, FALSE);
  printResult(tr, adef, FALSE);
}


boolean treeEvaluate (tree *tr, double smoothFactor)       /* Evaluate a user tree */
  { /* treeEvaluate */
    
    /*double inLH = tr->likelihood;*/

    if (! smoothTree(tr, (int)((double)smoothings * smoothFactor))) 
      {
	return FALSE;      
      }
      
    evaluateGeneric(tr, tr->start);   
   

    /*    if(inLH > tr->likelihood)
      {
	printf("FATAL error in treeEvaluate %.20f <-> %.20f factor %d\n", inLH, tr->likelihood, (int)((double)smoothings * smoothFactor));
	}*/

    return TRUE;
  } /* treeEvaluate */


/************* per partition branch length optimization ****************************/

static boolean updatePartition(tree *tr, nodeptr p, int model)
{
  nodeptr  q;
  double   z0, z;
	
  q = p->back;
  z0 = q->z[0];
        
  z = makenewzPartitionGeneric(tr, p, q, z0, newzpercycle, model);
    
  p->z[0] = q->z[0] = z;
  if (ABS(z - z0) > deltaz)  tr->smoothed = FALSE;
        
  return TRUE; 
}

static boolean smoothPartition(tree *tr, nodeptr p, int model)
{
  nodeptr  q;
  
  if(! updatePartition(tr, p, model))               
    return FALSE; 

  if(/*! p->tip*/ !isTip(p->number, tr->rdta->numsp)) 
    {                    
      q = p->next;
      while (q != p) 
	{
	  if (! smoothPartition(tr, q->back, model))   
	    return FALSE;
	  q = q->next;
	}	

      newviewPartitionGeneric(tr, p, model);
    }

  return TRUE;
} 


static boolean smoothTreePartition(tree *tr, int maxtimes, int model)
{
  nodeptr  p, q;    

  p = tr->start;

  while(--maxtimes >= 0) 
    {
      tr->smoothed = TRUE;

      if(! smoothPartition(tr, p->back, model))      
	return FALSE;

      if(/*! p->tip*/ !isTip(p->number, tr->rdta->numsp)) 
	  {
	    q = p->next;
	    while (q != p) 
	      {
		if(! smoothPartition(tr, q->back, model))   
		  return FALSE;
		q = q->next;
	      }
	  }
      if (tr->smoothed)  break;
    }

  return TRUE;
}


boolean treeEvaluatePartition(tree *tr, double smoothFactor, int model)
{    
  if(! smoothTreePartition(tr, (int)((double)smoothings * smoothFactor), model))     
    return FALSE;          
      
  evaluatePartitionGeneric(tr, tr->start, model);    
   
  return TRUE;
}


