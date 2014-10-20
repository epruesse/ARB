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


void ntree_init(const PartitionSize *size);
void ntree_cleanup();

int ntree_count_sons(const NT_NODE *tree);

void insert_ntree(PART*& part);
const NT_NODE *ntree_get();

#if defined(NTREE_DEBUG_FUNCTIONS)
void print_ntree(NT_NODE *tree, int indent);
bool is_well_formed(const NT_NODE *tree);
#endif


#else
#error CT_ntree.hxx included twice
#endif // CT_NTREE_HXX
