// ============================================================= //
//                                                               //
//   File      : CT_ctree.cxx                                    //
//   Purpose   : consensus tree                                  //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "CT_ctree.hxx"
#include "CT_hash.hxx"
#include "CT_rbtree.hxx"
#include "CT_dtree.hxx"

#include <arb_strarray.h>

ConsensusTree::ConsensusTree(const class CharPtrArray& names)
    : Tree_count(0),
      Name_hash(NULL),
      size(NULL)
{
    // names = leafnames (=species names)

    int leaf_count = names.size();
    Name_hash = GBS_create_hash(leaf_count, GB_MIND_CASE);
    for (int i=0; i< leaf_count; i++) {
        GBS_write_hash(Name_hash, names[i], i+1);
    }

    size = new PartitionSize(leaf_count);

    registry = new PartRegistry();

    rb_init(names);
}

ConsensusTree::~ConsensusTree() {
    if (Name_hash) GBS_free_hash(Name_hash);
    Name_hash  = 0;
    Tree_count = 0;

    rb_cleanup();

    delete registry;
    delete size;
}

void ConsensusTree::insert(GBT_TREE *tree, int weight) {
    // Insert a GBT-tree in the Hash-Table
    // The GBT-tree is destructed afterwards!
    Tree_count += weight;
    remember_subtrees(tree, weight);
}

GBT_TREE *ConsensusTree::get_consensus_tree() {
    // Get new consensus-tree -> GBT-tree
    /* This function is little bit tricky:
       the root-partition consist of 111....111 so it must have two sons
       that represent the same partition son1 == ~son2 to do this we must split
       the fist son-partition in two parts through logical calculation there
       could only be one son! */

    ntree_init(size);
    registry->build_sorted_list();

    {
        PART *p = registry->get_part(Tree_count);
        while (p != NULL) {
            insert_ntree(p);
            p = registry->get_part(Tree_count);
        }
    }

    const NT_NODE *n = ntree_get();

    arb_assert(ntree_count_sons(n) == 2);

#if defined(NTREE_DEBUG_FUNCTIONS)
    arb_assert(is_well_formed(n));
#endif

    GBT_TREE *result_tree = rb_gettree(n);
    ntree_cleanup();

    return result_tree;
}

