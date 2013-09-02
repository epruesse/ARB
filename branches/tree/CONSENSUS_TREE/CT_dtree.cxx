// ============================================================= //
//                                                               //
//   File      : CT_dtree.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "CT_hash.hxx"
#include "CT_ctree.hxx"

PART *ConsensusTree::dtree(const GBT_TREE *tree, double weight, GBT_LEN len) {
    /* destruct GBT-Tree and build partitions. This is done recursive by concatenate
       all sons to build the father partition. All partitions are inserted in the
       hashtable */

    PART *ptree = NULL;
    if (tree->is_leaf) {
        ptree = new PART(size, weight);
        ptree->setbit(get_species_index(tree->name));
    }
    else {
        // In any possible case left and rightson always exist ...
        PART *p1 = dtree(tree->leftson, weight, tree->leftlen);
        PART *p2 = dtree(tree->rightson, weight, tree->rightlen);

        arb_assert(p1->disjunct_from(p2));

        ptree = p1->clone();
        ptree->add_members_from(p2);

        registry->put_part(p1);
        registry->put_part(p2);
    }
    ptree->set_len(len);
    return ptree;
}


void ConsensusTree::remember_subtrees(const GBT_TREE *tree, double weight) {
    /* it is necessary to destruct the left and the right side separately, because
     * the root is only a virtual node and must be ignored. Moreover the left and
     * rightson are the same partition. So only the right son is inserted here.
     */

    PART *p1 = dtree(tree->leftson, weight, 0.0);
    PART *p2 = dtree(tree->rightson, weight, 0.0);

    arb_assert(p1->disjunct_from(p2));

    delete p1;

    p2->set_len(tree->leftlen + tree->rightlen);
    registry->put_part(p2);
}

