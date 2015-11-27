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

PART *ConsensusTree::deconstruct_full_subtree(const TreeNode *tree, const GBT_LEN& len, const double& weight) {
    /*! deconstruct TreeNode and register found partitions.
     * @param len length of branch towards subtree 'tree'
     * @param weight summarized for indentical branches (from different trees)
     */

    arb_assert(tree->father);

    PART *ptree = NULL;
    if (tree->is_leaf) {
        ptree = create_tree_PART(tree, weight);
    }
    else {
        PART *p1 = deconstruct_full_subtree(tree->get_leftson(), tree->leftlen, weight);
        PART *p2 = deconstruct_full_subtree(tree->get_rightson(), tree->rightlen, weight);

        arb_assert(p1->disjunct_from(p2));

        ptree = p1->clone();
        ptree->add_members_from(p2);

        registry->put_part_from_complete_tree(p1);
        registry->put_part_from_complete_tree(p2);
    }
    ptree->set_len(len);
    inc_insert_progress();
    return ptree;
}

void ConsensusTree::deconstruct_full_rootnode(const TreeNode *tree, const double& weight) {
    /*! deconstruct TreeNode and register found partitions.
     * @param weight summarized for indentical branches (from different trees)
     */

    arb_assert(!tree->father);
    arb_assert(!tree->is_leaf);

    double root_length = (tree->leftlen + tree->rightlen);

    PART *p1 = deconstruct_full_subtree(tree->get_leftson(), root_length, weight);
    PART *p2 = deconstruct_full_subtree(tree->get_rightson(), root_length, weight);

    arb_assert(p1->disjunct_from(p2));

    // add only one of the partition p1 and p2 (they both represent the root-edge)
    registry->put_part_from_complete_tree(p1);
    delete p2;

    inc_insert_progress();
}

PART *ConsensusTree::deconstruct_partial_subtree(const TreeNode *tree, const GBT_LEN& len, const double& weight, const PART *partialTree) {
    /*! deconstruct partial TreeNode
     *
     * similar to deconstruct_full_subtree(),
     * but the set of missing species is added at each branch.
     */

    arb_assert(tree->father);

    PART *ptree = NULL;
    if (tree->is_leaf) {
        ptree = create_tree_PART(tree, weight);
    }
    else {
        PART *p1 = deconstruct_partial_subtree(tree->get_leftson(), tree->leftlen, weight, partialTree);
        PART *p2 = deconstruct_partial_subtree(tree->get_rightson(), tree->rightlen, weight, partialTree);

        arb_assert(p1->disjunct_from(p2));

        ptree = p1->clone();
        ptree->add_members_from(p2);

        registry->put_part_from_partial_tree(p1, partialTree);
        registry->put_part_from_partial_tree(p2, partialTree);
    }
    ptree->set_len(len);
    inc_insert_progress();
    return ptree;
}

void ConsensusTree::deconstruct_partial_rootnode(const TreeNode *tree, const double& weight, const PART *partialTree) {
    /*! deconstruct partial TreeNode
     *
     * similar to deconstruct_full_rootnode(),
     * but the set of missing species is added at each branch.
     */

    arb_assert(!tree->father);
    arb_assert(!tree->is_leaf);

    double root_length = (tree->leftlen + tree->rightlen);

    PART *p1 = deconstruct_partial_subtree(tree->get_leftson(), root_length, weight, partialTree);
    PART *p2 = deconstruct_partial_subtree(tree->get_rightson(), root_length, weight, partialTree);

    arb_assert(p1->disjunct_from(p2));

    p2->add_members_from(p1); // in p2 we collect the whole partialTree
    registry->put_part_from_partial_tree(p1, partialTree); // because we are at root edge, we only insert one partition

    arb_assert(p2->equals(partialTree));

    arb_assert(is_similar(p2->get_weight(), weight, 0.01));
    registry->put_artificial_part(p2);
    inc_insert_progress();
}

void ConsensusTree::add_tree_to_PART(const TreeNode *tree, PART& part) const {
    if (tree->is_leaf) {
        part.setbit(get_species_index(tree->name));
    }
    else {
        add_tree_to_PART(tree->get_leftson(), part);
        add_tree_to_PART(tree->get_rightson(), part);
    }
}

PART *ConsensusTree::create_tree_PART(const TreeNode *tree, const double& weight) const {
    PART *part = new PART(size, weight);
    if (part) add_tree_to_PART(tree, *part);
    return part;
}

