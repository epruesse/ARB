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

#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef ARBDBT_H
#include <arbdbt.h>
#endif
#ifndef ARB_STRARRAY_H
#include <arb_strarray.h>
#endif

class PART;
class NT_NODE;
class PartitionSize;
class PartRegistry;

class ConsensusTree : virtual Noncopyable {
    int      overall_weight; 
    GB_HASH *Name_hash;

    PartitionSize *size;
    PartRegistry  *registry;

    const CharPtrArray& names;

    PART *dtree(const GBT_TREE *tree, int weight, GBT_LEN len);
    void remember_subtrees(const GBT_TREE *tree, int weight);

    int get_species_index(const char *name) const {
        int idx = GBS_read_hash(Name_hash, name);
        arb_assert(idx>0); // given 'name' is unknown
        return idx-1;
    }

    const char *get_species_name(int idx) const {
        return names[idx];
    }

    class RB_INFO *rbtree(const NT_NODE *tree, GBT_TREE *father);
    GBT_TREE      *rb_gettree(const NT_NODE *tree);


public:
    ConsensusTree(const CharPtrArray& names_);
    ~ConsensusTree();

    void insert(GBT_TREE *tree, int weight);

    GBT_TREE *get_consensus_tree();

};

#else
#error CT_ctree.hxx included twice
#endif // CT_CTREE_HXX
