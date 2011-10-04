/*!
 * \file dlist.cxx
 *
 * Doubly linked list stuff (implementation similar to AmigaOS lists)
 *
 * \author Chris Hodges <hodges@in.tum.de>
 */

#include <stdio.h>
#include <stdlib.h>

#include <algorithm>

#include "dlist.h"

/* /// "NewList()" */
/* Initialize empty list */
void NewList(struct List *lh) {
    lh->lh_TailPred = (struct Node *) lh;
    lh->lh_Tail = NULL;
    lh->lh_Head = (struct Node *) &lh->lh_Tail;
}
/* \\\ */

/* /// "AddHead()" */
/* Add node to the bottom of the list */
void AddHead(struct List *lh, struct Node *nd) {
    struct Node *oldhead = lh->lh_Head;
    lh->lh_Head = nd;
    nd->ln_Pred = (struct Node *) &lh->lh_Head;
    nd->ln_Succ = oldhead;
    oldhead->ln_Pred = nd;
}
/* \\\ */

/* /// "AddTail()" */
/* Add node at the front of the list */
void AddTail(struct List *lh, struct Node *nd) {
    struct Node *oldtail = lh->lh_TailPred;
    lh->lh_TailPred = nd;
    nd->ln_Succ = (struct Node *) &lh->lh_Tail;
    nd->ln_Pred = oldtail;
    oldtail->ln_Succ = nd;
}
/* \\\ */

/* /// "Remove()" */
/* Remove node from whatever list it is in */
void Remove(struct Node *nd) {
    if (nd->ln_Pred)
        nd->ln_Pred->ln_Succ = nd->ln_Succ;
    if (nd->ln_Succ)
        nd->ln_Succ->ln_Pred = nd->ln_Pred;
}
/* \\\ */

/* /// "NodePriCompare()" */
/* compare function for node sorting */
bool NodePriCompare(struct Node *node1, struct Node *node2) {
    return node1->ln_Pri < node2->ln_Pri;
}
/* \\\ */

/* /// "NodePriCompareReverse()" */
/* compare function for node sorting */
bool NodePriCompareReverse(struct Node *node1, struct Node *node2) {
    return node1->ln_Pri > node2->ln_Pri;
}
/* \\\ */

/* /// "SortList()" */
BOOL SortList(struct List *lh, bool reverse) {
    BOOL sorted = TRUE;
    ULONG numelem = 0;

    /* count elements and check, if they are sorted already */
    struct Node *ln = lh->lh_Head;
    while (ln->ln_Succ) {
        numelem++;
        if (ln->ln_Succ->ln_Succ) {
            if (ln->ln_Succ->ln_Pri < ln->ln_Pri) {
                sorted = FALSE;
            }
        }
        ln = ln->ln_Succ;
    }
    if (sorted || (numelem < 2)) {
        return (TRUE); /* was already sorted */
    }

    /* allocate a temporary array */
    struct Node **tmparr = (struct Node **) calloc(numelem,
            sizeof(struct Node *));
    if (!tmparr) {
        return (FALSE); /* out of memory */
    }

    /* enter all elements */
    struct Node **arrptr = tmparr;
    ln = lh->lh_Head;
    while (ln->ln_Succ) {
        *arrptr++ = ln;
        ln = ln->ln_Succ;
    }
    /* sort elements */
    //printf("Sorting...\n");
    if (reverse) {
        std::sort(tmparr, tmparr + numelem, NodePriCompareReverse);
    } else {
        std::sort(tmparr, tmparr + numelem, NodePriCompare);
    }
    /* rebuild list */
    NewList(lh);
    arrptr = tmparr;
    do {
        AddTail(lh, *arrptr++);
    } while (--numelem);
    free(tmparr);
    return (TRUE);
}
/* \\\ */

