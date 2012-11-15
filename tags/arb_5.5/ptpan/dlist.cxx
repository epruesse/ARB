/************************************************************************
 Doubly linked list stuff (implementation similar to AmigaOS lists)
 Written by Chris Hodges <hodges@in.tum.de>.
 Last change: 26.08.03
 ************************************************************************/

/* Implementation */

#include <stdio.h>
#include <stdlib.h>
#include "dlist.h"

/* /// "NewList()" */
/* Initialize empty list */
void NewList(struct List *lh)
{
  lh->lh_TailPred = (struct Node *) lh;
  lh->lh_Tail = NULL;
  lh->lh_Head = (struct Node *) &lh->lh_Tail;
}
/* \\\ */

/* /// "AddHead()" */
/* Add node to the bottom of the list */
void AddHead(struct List *lh, struct Node *nd)
{
  struct Node *oldhead = lh->lh_Head;
  lh->lh_Head = nd;
  nd->ln_Pred = (struct Node *) &lh->lh_Head;
  nd->ln_Succ = oldhead;
  oldhead->ln_Pred = nd;
}
/* \\\ */

/* /// "AddTail()" */
/* Add node at the front of the list */
void AddTail(struct List *lh, struct Node *nd)
{
  struct Node *oldtail = lh->lh_TailPred;
  lh->lh_TailPred = nd;
  nd->ln_Succ = (struct Node *) &lh->lh_Tail;
  nd->ln_Pred = oldtail;
  oldtail->ln_Succ = nd;
}
/* \\\ */

/* /// "Remove()" */
/* Remove node from whatever list it is in */
void Remove(struct Node *nd)
{
  nd->ln_Pred->ln_Succ = nd->ln_Succ;
  nd->ln_Succ->ln_Pred = nd->ln_Pred;
}
/* \\\ */

/* /// "NodePriCompare()" */
/* compare function for node sorting */
LONG NodePriCompare(const struct Node **node1, const struct Node **node2)
{
  if((*node1)->ln_Pri < (*node2)->ln_Pri)
  {
    return(-1);
  }    
  if((*node1)->ln_Pri > (*node2)->ln_Pri)
  {
    return(1);
  }    
  return(0);
}
/* \\\ */

/* /// "SortList()" */
BOOL SortList(struct List *lh)
{
  struct Node *ln;
  struct Node **tmparr;
  struct Node **arrptr;
  BOOL sorted = TRUE;
  ULONG numelem = 0;

  /* count elements and check, if they are sorted already */
  ln = lh->lh_Head;
  while(ln->ln_Succ)
  {
    numelem++;
    if(ln->ln_Succ->ln_Succ)
    {
      if(ln->ln_Succ->ln_Pri < ln->ln_Pri)
      {
       sorted = FALSE;
      }
    }
    ln = ln->ln_Succ;
  }
  if(sorted || (numelem < 2))
  {
    return(TRUE); /* was already sorted */
  }

  /* allocate a temporary array */
  tmparr = (struct Node **) calloc(numelem, sizeof(struct Node *));
  if(!tmparr)
  {
    return(FALSE); /* out of memory */
  }

  /* enter all elements */
  arrptr = tmparr;
  ln = lh->lh_Head;
  while(ln->ln_Succ)
  {
    *arrptr++ = ln;
    ln = ln->ln_Succ;
  }
  /* sort elements */
  //printf("Sorting...\n");
  qsort(tmparr, numelem, sizeof(struct Node *),
        (int (*)(const void *, const void *)) NodePriCompare);
  /* rebuild list */
  NewList(lh);
  arrptr = tmparr;
  do
  {
    AddTail(lh, *arrptr++);
  } while(--numelem);
  free(tmparr);
  return(TRUE);
}
/* \\\ */

/* /// "BuildBinTreeRec()" */
struct BinTree * BuildBinTreeRec(struct Node **nodearr, ULONG left, ULONG right)
{
  struct BinTree *bt;
  ULONG mid;
  if(left >= right)
  {
    return(NULL);
  }
  mid = (left + right) >> 1;
  bt = (struct BinTree *) calloc(sizeof(struct BinTree), 1);
  if(!bt)
  {
    return(NULL); /* out of memory */
  }

  /* fill children and leaves */
  bt->bt_Key = nodearr[mid+1]->ln_Pri;
  bt->bt_Child[0] = BuildBinTreeRec(nodearr, left, mid);
  if(!bt->bt_Child[0])
  {
    bt->bt_Leaf[0] = nodearr[left];
  }
  bt->bt_Child[1] = BuildBinTreeRec(nodearr, mid+1, right);
  if(!bt->bt_Child[1])
  {
    bt->bt_Leaf[1] = nodearr[mid+1];
  }
  return(bt);
}
/* \\\ */

/* /// "BuildBinTree()" */
struct BinTree * BuildBinTree(struct List *list)
{
  struct BinTree *bt;
  struct Node *ln;
  struct Node **tmparr;
  struct Node **arrptr;
  BOOL sorted = TRUE;
  ULONG numelem = 0;

  /* count elements */
  ln = list->lh_Head;
  while(ln->ln_Succ)
  {
    numelem++;
    ln = ln->ln_Succ;
  }
  /* allocate a temporary array */
  tmparr = (struct Node **) calloc(numelem, sizeof(struct Node *));
  if(!tmparr)
  {
    return(NULL); /* out of memory */
  }

  /* enter all elements and check, if they are sorted already */
  arrptr = tmparr;
  ln = list->lh_Head;
  while(ln->ln_Succ)
  {
    *arrptr++ = ln;
    if(ln->ln_Succ->ln_Succ)
    {
      if(ln->ln_Succ->ln_Pri < ln->ln_Pri)
      {
        sorted = FALSE;
      }
    }
    ln = ln->ln_Succ;
  }
  if(!sorted) /* only sort the array, if it wasn't sorted before */
  {
    /* enter elements */
    //printf("Sorting...\n");
    qsort(tmparr, numelem, sizeof(struct Node *),
        (int (*)(const void *, const void *)) NodePriCompare);
  }
  /* build up the tree */
  bt = BuildBinTreeRec(tmparr, 0, numelem-1);
  free(tmparr);
  return(bt);
}
/* \\\ */

/* /// "FreeBinTree()" */
void FreeBinTree(struct BinTree *root)
{
  if(!root)
  {
    return;
  }
  FreeBinTree(root->bt_Child[0]);
  FreeBinTree(root->bt_Child[1]);
  free(root);
}
/* \\\ */

/* /// "FindBinTreeLowerKey()" */
struct Node *FindBinTreeLowerKey(struct BinTree *root, LLONG key)
{
  if(root->bt_Key > key)
  {
    if(root->bt_Leaf[0])
    {
      return(root->bt_Leaf[0]);
    }
    if(root->bt_Child[0])
    {
      return(FindBinTreeLowerKey(root->bt_Child[0], key));
    }
  }
  if(root->bt_Leaf[1])
  {
    return(root->bt_Leaf[1]);
  }
  if(root->bt_Child[1])
  {
    return(FindBinTreeLowerKey(root->bt_Child[1], key));
  }
  printf("Huh! Key %lld not found!\n", key);
  return(NULL);
}
