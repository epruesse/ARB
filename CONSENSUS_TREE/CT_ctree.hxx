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

class CharPtrArray;
class PART;

class ConsensusTree : virtual Noncopyable {
    int      Tree_count; // not really tree count, but sum of weights of added trees
    GB_HASH *Name_hash;


    PART *dtree(const GBT_TREE *tree, int weight, GBT_LEN len);
    void remember_subtrees(const GBT_TREE *tree, int weight);

    int get_species_index(const char *name) const {
        int idx = GBS_read_hash(Name_hash, name);
        arb_assert(idx>0); // given 'name' is unknown
        return idx-1;
    }

public:
    ConsensusTree(const class CharPtrArray& names);
    ~ConsensusTree();

    void insert(GBT_TREE *tree, int weight);

    GBT_TREE *get_consensus_tree();
};

#else
#error CT_ctree.hxx included twice
#endif // CT_CTREE_HXX
