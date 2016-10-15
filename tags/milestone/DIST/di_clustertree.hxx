// =============================================================== //
//                                                                 //
//   File      : di_clustertree.hxx                                //
//   Purpose   : Tree structure used for cluster detection         //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2009   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef DI_CLUSTERTREE_HXX
#define DI_CLUSTERTREE_HXX

#ifndef ARB_TREE_HXX
#include <ARB_Tree.hxx>
#endif
#ifndef AP_SEQUENCE_HXX
#include <AP_sequence.hxx>
#endif
#ifndef _GLIBCXX_MAP
#include <map>
#endif


#define cl_assert(cond) arb_assert(cond)

class ClusterTree;
class arb_progress;

enum ClusterState {
    CS_UNKNOWN       = 0,                           // initial state
    CS_TOO_SMALL     = 1,                           // cluster is too small
    CS_MAYBE_CLUSTER = 2,                           // need to test whether this is a cluster
    CS_NO_CLUSTER    = 4,                           // not a cluster (only worstKnownDistance is known)
    CS_IS_CLUSTER    = 8,                           // subtree is cluster (all sequence distances are known)
    CS_SUB_CLUSTER   = 16,                          // like CS_IS_CLUSTER, but father is cluster as well
};

// ------------------------
//      ClusterTreeRoot

class ClusterTreeRoot : public ARB_seqtree_root {
    AP_FLOAT maxDistance;                           // max. allowed distance inside cluster
    unsigned minClusterSize;                        // min. size of cluster (number of leafs)

public:
    ClusterTreeRoot(AliView *aliview, AP_sequence *seqTemplate_, AP_FLOAT maxDistance_, size_t minClusterSize_);
    ~ClusterTreeRoot() OVERRIDE { predelete(); }

    inline TreeNode *makeNode() const             OVERRIDE;
    inline void destroyNode(TreeNode *node) const OVERRIDE;

    DEFINE_DOWNCAST_ACCESSORS(ClusterTree, get_root_node, ARB_seqtree_root::get_root_node());

    GB_ERROR find_clusters();
    unsigned get_minClusterSize() const { return minClusterSize; }
    AP_FLOAT get_maxDistance() const { return maxDistance; }
};

// ------------------
//      LeafPairs

class TwoLeafs {
    ClusterTree *ct1, *ct2; // ct1<ct2!

public:
    TwoLeafs(ClusterTree *c1, ClusterTree *c2)
        : ct1(c1<c2 ? c1 : c2)
        , ct2(c1<c2 ? c2 : c1)
    {}

    const ClusterTree *first() const { return ct1; }
    const ClusterTree *second() const { return ct2; }

    bool operator<(const TwoLeafs& other) const {
        return ct1 == other.ct1 ? ct2<other.ct2 : ct1<other.ct1;
    }
};

class LeafRelation {
    const TwoLeafs *pair;
    const AP_FLOAT  value;
public:
    LeafRelation(const TwoLeafs& pair_, AP_FLOAT value_)
        : pair(&pair_)
        , value(value_)
    {}

    bool operator<(const LeafRelation& other) const {
        if (value == other.value) return *pair < *other.pair;
        return value < other.value;
    }

    const TwoLeafs& get_pair() const  { return *pair; }
};

typedef std::map<ClusterTree*, AP_FLOAT> NodeValues;
typedef std::map<TwoLeafs, AP_FLOAT>     LeafRelations;
typedef LeafRelations::const_iterator    LeafRelationCIter;

// --------------------
//      ClusterTree


#if defined(DEBUG)
#define TRACE_DIST_CALC
#endif // DEBUG

class ClusterTree : public ARB_countedTree { // derived from a Noncopyable
    ClusterState state;

    unsigned leaf_count;                            // number of leafs in subtree
    unsigned clus_count;                            // number of clusters at and in subtree
    unsigned depth;                                 // depth of node ( 1 == root )
    AP_FLOAT min_bases;                             // min. bases used for comparing two members

    NodeValues    *branchDepths;                    // leaf-depths (distance from this each leaf)
    LeafRelations *branchDists;                     // distance (branch) between two leafs
    LeafRelations *sequenceDists;                   // real distance between sequences of two leafs

    TwoLeafs *worstKnownDistance;

    void calc_branch_depths();
    void calc_branch_dists();

#if defined(TRACE_DIST_CALC)
    unsigned calculatedDistances;
#endif // TRACE_DIST_CALC

    unsigned get_depth() const { return depth; }
    bool knows_seqDists() const { return state & (CS_IS_CLUSTER|CS_SUB_CLUSTER); }
    unsigned possible_relations() const { return (leaf_count*(leaf_count-1)) / 2; }
    unsigned known_seqDists() const { return knows_seqDists() ? possible_relations() : 0; }

    const NodeValues *get_branch_depths() {
        if (!branchDepths) calc_branch_depths();
        return branchDepths;
    }

    const LeafRelations *get_branch_dists() {
        if (!branchDists) calc_branch_dists();
        return branchDists;
    }

    AP_FLOAT get_seqDist(const TwoLeafs& pair);
    const AP_FLOAT *has_seqDist(const TwoLeafs& pair) const;
    const ClusterTree *commonFatherWith(const ClusterTree *other) const;

    void oblivion(bool forgetDistances); // forget unneeded data

protected:
    ~ClusterTree() OVERRIDE {
        delete worstKnownDistance;
        delete sequenceDists;
        delete branchDists;
        delete branchDepths;
    }
    friend class ClusterTreeRoot;
public:
    explicit ClusterTree(ClusterTreeRoot *tree_root_)
        : ARB_countedTree(tree_root_)
        , state(CS_UNKNOWN)
        , leaf_count(0)
        , clus_count(0)
        , depth(0)
        , min_bases(-1.0)
        , branchDepths(NULL)
        , branchDists(NULL)
        , sequenceDists(NULL)
        , worstKnownDistance(NULL)
    {}

    DEFINE_TREE_ACCESSORS(ClusterTreeRoot, ClusterTree);

    unsigned get_cluster_count() const { return clus_count; }
    unsigned get_leaf_count() const OVERRIDE { return leaf_count; }

#if defined(TRACE_DIST_CALC)
    unsigned get_calculated_distances() const { return calculatedDistances; }
#endif // TRACE_DIST_CALC

    ClusterState get_state() const { return state; }

    void init_tree() OVERRIDE;
    void detect_clusters(arb_progress& progress);

    const LeafRelations *get_sequence_dists() const { return sequenceDists; }

    AP_FLOAT get_min_bases() const { return min_bases; }
};

inline TreeNode *ClusterTreeRoot::makeNode() const { return new ClusterTree(const_cast<ClusterTreeRoot*>(this)); }
inline void ClusterTreeRoot::destroyNode(TreeNode *node) const { delete DOWNCAST(ClusterTree*, node); }

class UseAnyTree : public ARB_tree_predicate {
    bool selects(const ARB_seqtree&) const OVERRIDE { return true; }
};

#else
#error di_clustertree.hxx included twice
#endif // DI_CLUSTERTREE_HXX
