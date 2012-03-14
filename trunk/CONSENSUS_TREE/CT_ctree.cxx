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

#include <arbdbt.h>
#include <arb_strarray.h>

static int      Tree_count = 0;
static GB_HASH *Name_hash  = 0;

void ctree_init(int node_count, const CharPtrArray& names) {
    /* node_count  number of different leafs // @@@ unneeded (use names.size())
     * names       leafnames (=species names)
     *
     * Note: the explicit order of the branches in the generated tree
     *       depends on the order of the names.
     * 
     * The topology of the generated tree will be identical (regardless of the branch-order)
     */

    arb_assert(!Name_hash); // forgot to call ctree_cleanup?
    arb_assert(names.size() == node_count);

    Name_hash = GBS_create_hash(node_count, GB_MIND_CASE);

    for (int i=0; i< node_count; i++) {
        GBS_write_hash(Name_hash, names[i], i+1);
    }

    part_init(node_count);  // Amount of Bits used
    hash_init();
    rb_init(names);

    Tree_count = 0;
}

void ctree_cleanup() {
    if (Name_hash) GBS_free_hash(Name_hash);
    Name_hash  = 0;
    Tree_count = 0;
    
    rb_cleanup();
    hash_cleanup();
    part_cleanup();
}


// Insert a GBT-tree in the Hash-Table
// The GBT-tree is destructed afterwards!
void insert_ctree(GBT_TREE *tree, int weight)
{
    Tree_count += weight;
    remember_subtrees(tree, weight);
}


// Get new consensus-tree -> GBT-tree
/* This function is little bit tricky:
   the root-partition consist of 111....111 so it must have two sons
   that represent the same partition son1 == ~son2 to do this we must split
   the fist son-partition in two parts through logical calculation there
   could only be one son! */
GBT_TREE *get_ctree() {
    ntree_init();
    build_sorted_list();

    {
        PART *p = hash_getpart();
        while (p != NULL) {
            insert_ntree(p);
            p = hash_getpart();
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

int get_tree_count() {
    // not really tree count, but sum of weights of added trees
    return Tree_count;
}

int get_species_index(const char *name) {
    int idx = GBS_read_hash(Name_hash, name);
    arb_assert(idx>0); // given 'name' is unknown
    return idx-1;
}
