// ============================================================= //
//                                                               //
//   File      : CT_ntree.hxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef CT_NTREE_HXX
#define CT_NTREE_HXX

#ifndef CT_PART_HXX
#include "CT_part.hxx"
#endif

struct nsons;

typedef struct nt_node {
    PART         *part;
    struct nsons *son_list;
} NT_NODE;


typedef struct nsons {
    NT_NODE *node;
    nsons   *prev, *next;
} NSONS;



NT_NODE *new_ntnode(PART *p);
void     del_tree(NT_NODE *tree);
void     ntree_init();
int      ntree_cont(int len);
void     insert_ntree(PART *part);
NT_NODE *ntree_get();
void     print_ntindex(NT_NODE *tree);
void     print_ntree(NT_NODE *tree);

#else
#error CT_ntree.hxx included twice
#endif // CT_NTREE_HXX
