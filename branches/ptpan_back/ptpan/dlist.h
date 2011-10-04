/************************************************************************
 Doubly linked list stuff (implementation similar to AmigaOS lists)
 Written by Chris Hodges <hodges@in.tum.de>.
 Last change: 06.05.03
 ************************************************************************/

#ifndef PTPAN_DLIST_H
#define PTPAN_DLIST_H

#include "types.h"

// Simple doubly linked list node
struct Node {
 struct Node *ln_Succ; /* Pointer to next (successor) */
 struct Node *ln_Pred; /* Pointer to previous (predecessor) */
 LLONG ln_Pri; /* priority or key */
};

// List header, empty list must be initialized with NewList()
struct List {
 struct Node *lh_Head;
 struct Node *lh_Tail;
 struct Node *lh_TailPred;
};

// prototypes

void Remove(struct Node *nd);

void NewList(struct List *lh);
void AddHead(struct List *lh, struct Node *nd);
void AddTail(struct List *lh, struct Node *nd);

bool NodePriCompare(struct Node *node1, struct Node *node2);
bool NodePriCompareReverse(struct Node *node1, struct Node *node2);
BOOL SortList(struct List *lh, bool reverse = false);

#endif /* PTPAN_DLIST_H */

