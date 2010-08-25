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

int Tree_count;
GB_HASH *Name_hash;

/*  node_count  number of different leafs
    names       name of each leaf   */
void ctree_init(int node_count, char **names)
{
    int i;

    Name_hash = GBS_create_hash(node_count, GB_MIND_CASE);

    for (i=0; i< node_count; i++) {
        GBS_write_hash(Name_hash, names[i], (long) i);
    }

    part_init(node_count);  /* Amount of Bits used */
    hash_init();
    destree_init(Name_hash);
    rb_init(names);
    Tree_count = 0;

}


/* Insert a GBT-tree in the Hash-Table */
/* The GBT-tree is destructed afterwards! */
void insert_ctree(GBT_TREE *tree, int weight)
{
    Tree_count += weight;
    des_tree(tree, weight);
}


/* Get new consensus-tree -> GBT-tree */
/* This function is little bit tricky:
   the root-partition consist of 111....111 so it must have two sons
   that represent the same partition son1 == ~son2 to do this we must split
   the fist son-partition in two parts through logical calculation there
   could only be one son! */
GBT_TREE *get_ctree()
{
    PART *p;
    NT_NODE *n;

    hash_settreecount(Tree_count);
    ntree_init();
    build_sorted_list();
    p = hash_getpart();
    while (p != NULL) {
        insert_ntree(p);
        p = hash_getpart();
    }
    n = ntree_get();
    if (n->son_list->next == NULL) { /* if father has only one son */
        p  = part_new();
        n->son_list->node->part->len /= 2;
        part_copy(n->son_list->node->part, p);
        part_invert(p);
        insert_ntree(p);
        n = ntree_get();
    }
    else {  /* if father has tree sons */
            /* this case should happen nerver! */
        printf("Es gibt noch was zu tun !!!!!! \n");
    }

    return rb_gettree(n);
}
