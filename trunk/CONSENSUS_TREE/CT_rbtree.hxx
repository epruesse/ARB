// ============================================================= //
//                                                               //
//   File      : CT_rbtree.hxx                                   //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef CT_RBTREE_HXX
#define CT_RBTREE_HXX

#ifndef CT_NTREE_HXX
#include "CT_ntree.hxx"
#endif

typedef struct {
    GBT_LEN len;
    GBT_TREE *node;
    int percent;
} RB_INFO;


void rb_init(char **names);

GBT_TREE *rb_gettree(NT_NODE *tree);

#else
#error CT_rbtree.hxx included twice
#endif // CT_RBTREE_HXX
