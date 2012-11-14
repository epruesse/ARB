/************************************************************************
 Doubly linked list stuff (implementation similar to AmigaOS lists)
 Written by Chris Hodges <hodges@in.tum.de>.
 Last change: 06.05.03
 ************************************************************************/

#ifndef DLIST_H
#define DLIST_H

#include "types.h"

/* Simple doubly linked list node */
struct Node
{
  struct  Node *ln_Succ;     /* Pointer to next (successor) */
  struct  Node *ln_Pred;     /* Pointer to previous (predecessor) */
  /*LONG          ln_Type;*/
  LLONG         ln_Pri;      /* priority or key */
};

/* List header, empty list must be initialized with NewList() */
struct List
{
  struct  Node *lh_Head;
  struct  Node *lh_Tail;
  struct  Node *lh_TailPred;
};

struct BinTree
{
  struct BinTree *bt_Child[2];   /* children */
  struct Node    *bt_Leaf[2];    /* data leaf pointer */
  ULONG           bt_Key;        /* link for left/right */
};

/* prototypes */

void NewList(struct List *lh);
void AddHead(struct List *lh, struct Node *nd);
void AddTail(struct List *lh, struct Node *nd);
void Remove(struct Node *nd);

#endif /* DLIST_H */

