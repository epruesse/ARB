/*!
 * \file bin_tree.h
 *
 * \date 02.02.2011
 * \author Chris Hodges
 * \author Tilo Eissler
 */

#ifndef PTPAN_BIN_TREE_H_
#define PTPAN_BIN_TREE_H_

#include "dlist.h"

struct BinTree {
    struct BinTree *bt_Child[2]; /* children */
    struct Node *bt_Leaf[2]; /* data leaf pointer */
    ULONG bt_Key; /* link for left/right */
};

// prototypes

struct BinTree
* BuildBinTreeRec(struct Node **nodearr, ULONG left, ULONG right);
struct BinTree * BuildBinTree(struct List *list);
void FreeBinTree(struct BinTree *root);
struct Node *FindBinTreeLowerKey(struct BinTree *root, ULLONG key);

#endif /* PTPAN_BIN_TREE_H_ */
