/*!
 * \file bin_tree.cxx
 *
 * \date 02.02.2011
 * \author Chris Hodges
 * \author Tilo Eissler
 */

#include <stdio.h>
#include <stdlib.h>

#include <algorithm>

#include "bin_tree.h"

/* /// "BuildBinTreeRec()" */
struct BinTree * BuildBinTreeRec(struct Node **nodearr, ULONG left, ULONG right) {
 if (left >= right) {
  return (NULL);
 }
 ULONG mid = (left + right) >> 1;
 struct BinTree *bt = (struct BinTree *) calloc(sizeof(struct BinTree), 1);
 if (!bt) {
  return (NULL); /* out of memory */
 }

 /* fill children and leaves */
 bt->bt_Key = nodearr[mid + 1]->ln_Pri;
 bt->bt_Child[0] = BuildBinTreeRec(nodearr, left, mid);
 if (!bt->bt_Child[0]) {
  bt->bt_Leaf[0] = nodearr[left];
 }
 bt->bt_Child[1] = BuildBinTreeRec(nodearr, mid + 1, right);
 if (!bt->bt_Child[1]) {
  bt->bt_Leaf[1] = nodearr[mid + 1];
 }
 return (bt);
}
/* \\\ */

/* /// "BuildBinTree()" */
struct BinTree * BuildBinTree(struct List *list) {
 ULONG numelem = 0;

 /* count elements */
 struct Node *ln = list->lh_Head;
 while (ln->ln_Succ) {
  numelem++;
  ln = ln->ln_Succ;
 }
 /* allocate a temporary array */
 struct Node **tmparr = (struct Node **) calloc(numelem,
   sizeof(struct Node *));
 if (!tmparr) {
  return (NULL); /* out of memory */
 }

 /* enter all elements and check, if they are sorted already */
 struct Node **arrptr = tmparr;
 BOOL sorted = TRUE;
 ln = list->lh_Head;
 while (ln->ln_Succ) {
  *arrptr++ = ln;
  if (ln->ln_Succ->ln_Succ) {
   if (ln->ln_Succ->ln_Pri < ln->ln_Pri) {
    sorted = FALSE;
   }
  }
  ln = ln->ln_Succ;
 }
 if (!sorted) {
  std::sort(tmparr, tmparr + numelem, NodePriCompare);
 }
 struct BinTree *bt;
 if (numelem > 1) {
  /* build up the tree */
  bt = BuildBinTreeRec(tmparr, 0, numelem - 1);
 } else {
  bt = (struct BinTree *) calloc(sizeof(struct BinTree), 1);
  bt->bt_Key = list->lh_Head->ln_Pri + 1;
  bt->bt_Leaf[0] = tmparr[0];
  bt->bt_Leaf[1] = tmparr[0];
  bt->bt_Child[0] = NULL;
  bt->bt_Child[1] = NULL;
 }
 free(tmparr);
 return (bt);
}
/* \\\ */

/* /// "FreeBinTree()" */
void FreeBinTree(struct BinTree *root) {
 if (!root) {
  return;
 }
 FreeBinTree(root->bt_Child[0]);
 FreeBinTree(root->bt_Child[1]);
 free(root);
}
/* \\\ */

/* /// "FindBinTreeLowerKey()" */
struct Node *FindBinTreeLowerKey(struct BinTree *root, ULLONG key) {
 if (root->bt_Key > key) {
  if (root->bt_Leaf[0]) {
   return (root->bt_Leaf[0]);
  }
  if (root->bt_Child[0]) {
   return (FindBinTreeLowerKey(root->bt_Child[0], key));
  }
 }
 if (root->bt_Leaf[1]) {
  return (root->bt_Leaf[1]);
 }
 if (root->bt_Child[1]) {
  return (FindBinTreeLowerKey(root->bt_Child[1], key));
 }
 printf("Huh! Key %lld not found!\n", key);
 return (NULL);
}
/* \\\ */
