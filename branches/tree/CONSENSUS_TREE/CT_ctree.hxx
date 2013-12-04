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
#ifndef _GLIBCXX_MAP
#include <map>
#endif
#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif
#ifndef _GLIBCXX_STRING
#include <string>
#endif

class  PART;
struct NT_NODE;
class  PartitionSize;
class  PartRegistry;
class RB_INFO;

class ConsensusTree : virtual Noncopyable {
    double   overall_weight;
    GB_HASH *Name_hash;

    PartitionSize *size;
    PartRegistry  *registry;

    const CharPtrArray& names;

    PART *dtree(const GBT_TREE *tree, double weight, GBT_LEN len);
    void remember_subtrees(const GBT_TREE *tree, double weight);

    int get_species_index(const char *name) const {
        int idx = GBS_read_hash(Name_hash, name);
        arb_assert(idx>0); // given 'name' is unknown
        return idx-1;
    }

    const char *get_species_name(int idx) const {
        return names[idx];
    }

    struct RB_INFO *rbtree(const NT_NODE *tree, GBT_TREE *father);
    GBT_TREE       *rb_gettree(const NT_NODE *tree);


public:
    ConsensusTree(const CharPtrArray& names_);
    ~ConsensusTree();

    void insert_tree_weighted(GBT_TREE *tree, double weight);

    GBT_TREE *get_consensus_tree();

};


class ConsensusTreeBuilder {
    // wrapper for ConsensusTree
    // - automatically collects species occurring in added trees (has to be done by caller of ConsensusTree)
    // - uses much more memory than using ConsensusTree directly, since it stores all added trees
    // 
    // Not helpful for consensing thousands of trees like bootstrapping does

    typedef std::map<std::string, size_t> OccurCount;
    typedef std::vector<GBT_TREE*>        Trees;
    typedef std::vector<double>           Weights;

    OccurCount species_occurred;
    Trees      trees;
    Weights    weights;

public:
    ~ConsensusTreeBuilder() {
        for (size_t i = 0; i<trees.size(); ++i) {
            delete trees[i];
        }
    }

    void add(GBT_TREE*& tree, double weight) {
        trees.push_back(tree);
        weights.push_back(weight);

        size_t   name_count;
        GB_CSTR *names = GBT_get_names_of_species_in_tree(tree, &name_count);

        for (size_t n = 0; n<name_count; ++n) {
            const char *name = names[n];
            if (species_occurred.find(name) == species_occurred.end()) {
                species_occurred[name] = 1;
            }
            else {
                species_occurred[name]++;
            }
        }

        free(names);
        tree = NULL;
    }

    GBT_TREE *get(size_t& different_species) {
        ConstStrArray species_names;

        for (OccurCount::iterator s = species_occurred.begin(); s != species_occurred.end(); ++s) {
            species_names.put(s->first.c_str());
        }

        different_species = species_occurred.size();
        ConsensusTree ctree(species_names);
        for (size_t i = 0; i<trees.size(); ++i) {
            ctree.insert_tree_weighted(trees[i], weights[i]);
        }

#if defined(DEBUG) 
        // if class ConsensusTree does depend on any local data,
        // instanciating another instance will interfere:
        ConsensusTree influence(species_names);
#endif

        return ctree.get_consensus_tree();
    }
};


#else
#error CT_ctree.hxx included twice
#endif // CT_CTREE_HXX
