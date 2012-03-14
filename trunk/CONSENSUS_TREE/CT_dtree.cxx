// ============================================================= //
//                                                               //
//   File      : CT_dtree.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "CT_dtree.hxx"
#include "CT_hash.hxx"
#include "CT_ctree.hxx"

// destruct gbt-tree and build parts
// insert afterwards in Hashtable


PART *ConsensusTree::dtree(const GBT_TREE *tree, int weight, GBT_LEN len) {
    /* destruct GBT-Tree and build partitions. This is done recursive by concatenate
       all sons to build the father partition. All partitions are inserted in the
       hashtable */
    // caution: I use the fact that each inner node must have two sons.
    PART *ptree = part_new();
    if (tree->is_leaf) {
        part_setbit(ptree, get_species_index(tree->name));
    }
    else {
        // In any possible case left and rightson always exist ...
        PART *p1 = dtree(tree->leftson, weight, tree->leftlen);
        PART *p2 = dtree(tree->rightson, weight, tree->rightlen);

        arb_assert(are_brothers(p1, p2));

        part_copy(p1, ptree);
        part_or(p2, ptree);

        hash_insert(p1, weight);
        hash_insert(p2, weight);
    }
    part_setlen(ptree, len);
    return ptree;
}


void ConsensusTree::remember_subtrees(const GBT_TREE *tree, int weight) {
    /* it is necessary to destruct the left and the right side separately, because
       the root is only a virtual node and must be ignored. Moreover the left and
       rightson are the same partition. So I may only insert right son. */

    PART *p1 = dtree(tree->leftson, weight, 0.0);
    PART *p2 = dtree(tree->rightson, weight, 0.0);

    arb_assert(are_brothers(p1, p2));

    part_free(p1);

    part_setlen(p2, (tree->leftlen + tree->rightlen));
    hash_insert(p2, weight);
}

