// ============================================================= //
//                                                               //
//   File      : CT_ctree.hxx                                    //
//   Purpose   : interface of CONSENSUS_TREE library             //
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
#ifndef ROOTEDTREE_H
#include <RootedTree.h>
#endif

class  PART;
struct NT_NODE;
class  PartitionSize;
class  PartRegistry;
class  RB_INFO;

// -----------------------
//      SizeAwareTree

class SizeAwareTree : public RootedTree {
    // simple size-aware tree
    unsigned leaf_count;
public:
    SizeAwareTree(TreeRoot *root) : RootedTree(root) {}
    unsigned get_leaf_count() const OVERRIDE {
        return leaf_count;
    }
    void compute_tree() OVERRIDE {
        if (is_leaf) {
            leaf_count = 1;
        }
        else {
            get_leftson()->compute_tree();
            get_rightson()->compute_tree();
            leaf_count = get_leftson()->get_leaf_count()+get_rightson()->get_leaf_count();
        }
    }
};

struct SizeAwareNodeFactory : public RootedTreeNodeFactory {
    RootedTree *makeNode(TreeRoot *root) const OVERRIDE { return new SizeAwareTree(root); }
};

// -----------------------
//      ConsensusTree

class ConsensusTree : virtual Noncopyable {
    double               overall_weight;
    GB_HASH             *Name_hash;
    PartitionSize       *size;
    PART                *allSpecies;
    PartRegistry        *registry;
    const CharPtrArray&  names;
    bool                 added_partial_tree;

    PART *deconstruct_full_subtree(const GBT_TREE *tree, const GBT_LEN& len, const double& weight);
    PART *deconstruct_partial_subtree(const GBT_TREE *tree, const GBT_LEN& len, const double& weight, const PART *partialTree);

    void deconstruct_full_rootnode(const GBT_TREE *tree, const double& weight);
    void deconstruct_partial_rootnode(const GBT_TREE *tree, const double& weight, const PART *partialTree);

    void remember_subtrees(const GBT_TREE *tree, double weight);

    int get_species_index(const char *name) const {
        int idx = GBS_read_hash(Name_hash, name);
        arb_assert(idx>0); // given 'name' is unknown
        return idx-1;
    }

    const char *get_species_name(int idx) const {
        return names[idx];
    }

    struct RB_INFO *rbtree(const NT_NODE *tree, TreeRoot *root);
    SizeAwareTree  *rb_gettree(const NT_NODE *tree);

    void add_tree_to_PART(const GBT_TREE *tree, PART& part) const;
    PART *create_tree_PART(const GBT_TREE *tree, const double& weight) const;

public:
    ConsensusTree(const CharPtrArray& names_);
    ~ConsensusTree();

    void insert_tree_weighted(GBT_TREE *tree, double weight);

    SizeAwareTree *get_consensus_tree();
};

// ------------------------------
//      ConsensusTreeBuilder

class ConsensusTreeBuilder {
    // wrapper for ConsensusTree
    // - automatically collects species occurring in added trees (has to be done by caller of ConsensusTree)
    // - uses much more memory than using ConsensusTree directly, since it stores all added trees
    //
    // Not helpful for consensing thousands of trees like bootstrapping does

    typedef std::map<std::string, size_t> OccurCount;
    typedef std::vector<SizeAwareTree*>   Trees;
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

    void add(SizeAwareTree*& tree, double weight) {
        tree->get_tree_root()->find_innermost_edge().set_root();

        // (currently) reordering trees before deconstructing no longer
        // affects the resulting consense tree (as performed as unit tests).
        //
        // tree->reorder_tree(BIG_BRANCHES_TO_TOP);
        // tree->reorder_tree(BIG_BRANCHES_TO_BOTTOM);
        // tree->reorder_tree(BIG_BRANCHES_TO_EDGE);

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

    SizeAwareTree *get(size_t& different_species) {
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
