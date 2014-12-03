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
#ifndef TREENODE_H
#include <TreeNode.h>
#endif
#ifndef ARB_PROGRESS_H
#include <arb_progress.h>
#endif

class  PART;
struct NT_NODE;
class  PartitionSize;
class  PartRegistry;
class  RB_INFO;

// -----------------------
//      SizeAwareTree

struct SizeAwareRoot : public TreeRoot {
    inline SizeAwareRoot();
    inline TreeNode *makeNode() const OVERRIDE;
    inline void destroyNode(TreeNode *node) const OVERRIDE;
};

class SizeAwareTree : public TreeNode {
    // simple size-aware tree
    unsigned leaf_count;
protected:
    ~SizeAwareTree() OVERRIDE {}
    friend class SizeAwareRoot; // allowed to call dtor
public:
    SizeAwareTree(TreeRoot *root) : TreeNode(root) {}
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

SizeAwareRoot::SizeAwareRoot() : TreeRoot(true) {}
inline TreeNode *SizeAwareRoot::makeNode() const { return new SizeAwareTree(const_cast<SizeAwareRoot*>(this)); }
inline void SizeAwareRoot::destroyNode(TreeNode *node) const { delete DOWNCAST(SizeAwareTree*,node); }

// -----------------------
//      ConsensusTree

class ConsensusTree : virtual Noncopyable {
    double               overall_weight;
    GB_HASH             *Name_hash;
    PartitionSize       *size;
    PART                *allSpecies;
    PartRegistry        *registry;
    const CharPtrArray&  names;

    arb_progress *insertProgress;

    PART *deconstruct_full_subtree(const TreeNode *tree, const GBT_LEN& len, const double& weight);
    PART *deconstruct_partial_subtree(const TreeNode *tree, const GBT_LEN& len, const double& weight, const PART *partialTree);

    void deconstruct_full_rootnode(const TreeNode *tree, const double& weight);
    void deconstruct_partial_rootnode(const TreeNode *tree, const double& weight, const PART *partialTree);

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

    void add_tree_to_PART(const TreeNode *tree, PART& part) const;
    PART *create_tree_PART(const TreeNode *tree, const double& weight) const;

    void inc_insert_progress() { if (insertProgress) ++(*insertProgress); }

public:
    ConsensusTree(const CharPtrArray& names_);
    ~ConsensusTree();

    __ATTR__USERESULT GB_ERROR insert_tree_weighted(const TreeNode *tree, int leafs, double weight, bool provideProgress);

    SizeAwareTree *get_consensus_tree(GB_ERROR& error);
};

// ------------------------------
//      ConsensusTreeBuilder

class TreeInfo {
    std::string Name;
    size_t      specCount;

public:
    explicit TreeInfo(const char *Name_, size_t specCount_)
        : Name(Name_),
          specCount(specCount_)
    {
    }

    const char *name() const { return Name.c_str(); }
    size_t species_count() const { return specCount; }
};

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

    std::vector<TreeInfo> tree_info;

public:
    ~ConsensusTreeBuilder() {
        for (size_t i = 0; i<trees.size(); ++i) {
            destroy(trees[i]);
        }
    }

    void add(SizeAwareTree*& tree, const char *treename, double weight) {
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

        tree_info.push_back(TreeInfo(treename, name_count));

        free(names);
        tree = NULL;
    }

    SizeAwareTree *get(size_t& different_species, GB_ERROR& error) {
        arb_assert(!error);

        ConstStrArray species_names;
        for (OccurCount::iterator s = species_occurred.begin(); s != species_occurred.end(); ++s) {
            species_names.put(s->first.c_str());
        }

        different_species = species_occurred.size();
        ConsensusTree ctree(species_names);
        {
            arb_progress deconstruct("deconstructing", trees.size());

            for (size_t i = 0; i<trees.size() && !error; ++i) {
                error = ctree.insert_tree_weighted(trees[i], tree_info[i].species_count(), weights[i], true);
                ++deconstruct;
                if (error) {
                    error = GBS_global_string("Failed to deconstruct '%s' (Reason: %s)", tree_info[i].name(), error);
                }
                else {
                    error = deconstruct.error_if_aborted();
                }
            }
            if (error) deconstruct.done();
        }

        if (error) return NULL;

#if defined(DEBUG)
        // if class ConsensusTree does depend on any local data,
        // instanciating another instance will interfere:
        ConsensusTree influence(species_names);
#endif

        arb_progress reconstruct("reconstructing");
        return ctree.get_consensus_tree(error);
    }

    char *get_remark() const;
    size_t species_count() const { return species_occurred.size(); }
};


#else
#error CT_ctree.hxx included twice
#endif // CT_CTREE_HXX
