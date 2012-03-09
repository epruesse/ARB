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

struct NSONS;

struct NT_NODE {
    PART  *part;
    NSONS *son_list;
};


struct NSONS {
    NT_NODE *node;
    NSONS   *prev, *next;
};


void ntree_init();
void ntree_cleanup();

void insert_ntree(PART *part);
NT_NODE *ntree_get();

#else
#error CT_ntree.hxx included twice
#endif // CT_NTREE_HXX
