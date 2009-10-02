/* destruct gbt-tree and build parts */
/* insert afterwards in Hashtable */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>

#include "CT_part.hxx"
#include "CT_hash.hxx"

void destree_init(GB_HASH *hash)
{
    Name_hash = hash;
}


/* destruct GBT-Tree and build partitions. This is done recursive by concatenate
   all sons to build the father partition. All partitions are inserted in the
   hashtable */
/* caution: I use the fact that each inner node must have two sons. */
PART *dtree(GBT_TREE *tree, int weight, GBT_LEN len)
{
    PART *p1, *p2, *ph;
    int idx;

    if(tree->is_leaf) {
        ph = part_new();
        idx = GBS_read_hash(Name_hash, tree->name);
        part_setbit(ph, idx);
        part_setlen(ph, len);
        return ph;
    }

    /* In any possible case left and rightson always exist ... */
    p1 = dtree(tree->leftson, weight, tree->leftlen);
    ph = part_new();
    part_copy(p1, ph);
    hash_insert(ph, weight);

    p2 = dtree(tree->rightson, weight, tree->rightlen);
    part_or(p2, p1);
    hash_insert(p2, weight);
    part_setlen(p1, len);
    return p1;
}


/* it is necessary to destruct the left and the right side separately, because
   the root is only a virtual node and must be ignored. Moreover the left and
   rightson are the same partition. So I may only insert right son.            */
void des_tree(GBT_TREE *tree, int weight)
{
    PART *p;

    p = dtree(tree->leftson, weight, 0.0);
    part_free(p);
    p = dtree(tree->rightson, weight, 0.0);
    part_setlen(p, (tree->leftlen + tree->rightlen));
    hash_insert(p, weight);
}
