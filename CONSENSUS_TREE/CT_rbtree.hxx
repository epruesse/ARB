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

struct RB_INFO {
    GBT_LEN   len;
    GBT_TREE *node;
    int       percent;
};


void rb_init(const class CharPtrArray& names);
void rb_cleanup();

GBT_TREE *rb_gettree(NT_NODE *tree);

#else
#error CT_rbtree.hxx included twice
#endif // CT_RBTREE_HXX
