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
    /*  node_count  number of different leafs
        names       name of each leaf   */

    arb_assert(!Name_hash); // forgot to call ctree_cleanup?
    Name_hash = GBS_create_hash(node_count, GB_MIND_CASE);

    for (int i=0; i< node_count; i++) {
        GBS_write_hash(Name_hash, names[i], (long) i);
    }

    part_init(node_count);  // Amount of Bits used
    hash_init();
    rb_init(names);

    Tree_count = 0;
}

void ctree_cleanup() {
    GBS_free_hash(Name_hash);
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
    des_tree(tree, weight);
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
    
    NT_NODE *n = ntree_get();
    if (n->son_list->next == NULL) { // if father has only one son
        PART *p  = part_new();
        n->son_list->node->part->len /= 2;
        part_copy(n->son_list->node->part, p);
        part_invert(p);
        insert_ntree(p);
        n = ntree_get();
    }
    else {  // if father has tree sons
        arb_assert(0); // this case should never happen!
    }

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
    return idx;
}
