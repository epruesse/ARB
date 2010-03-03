// ============================================================= //
//                                                               //
//   File      : CT_ctree.hxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef CT_CTREE_HXX
#define CT_CTREE_HXX

struct GBT_TREE;

void      ctree_init(int node_count, char **names);
void      insert_ctree(GBT_TREE *tree, int weight);
GBT_TREE *get_ctree();

#else
#error CT_ctree.hxx included twice
#endif // CT_CTREE_HXX
