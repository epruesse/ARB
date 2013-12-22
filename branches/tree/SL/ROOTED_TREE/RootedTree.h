// ================================================================ //
//                                                                  //
//   File      : RootedTree.h                                       //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in December 2013   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef ROOTEDTREE_H
#define ROOTEDTREE_H

#ifndef ARBDBT_H
#include <arbdbt.h>
#endif
#ifndef _GLIBCXX_ALGORITHM
#include <algorithm>
#endif

#define rt_assert(cond) arb_assert(cond)

#if defined(DEBUG) || defined(UNIT_TESTS)
#define PROVIDE_TREE_STRUCTURE_TESTS
#endif
#if defined(DEVEL_RALF) && defined(PROVIDE_TREE_STRUCTURE_TESTS)
#define AUTO_CHECK_TREE_STRUCTURE // Note: dramatically slows down most tree operations
#endif

class TreeRoot;
class RootedTree;
class ARB_edge;

struct RootedTreeNodeFactory { // acts similar to TreeNodeFactory for trees with root
    virtual ~RootedTreeNodeFactory() {}
    virtual RootedTree *makeNode(TreeRoot *root) const = 0;
};

class TreeRoot : public TreeNodeFactory, virtual Noncopyable {
    RootedTree            *rootNode; // root node of the tree
    RootedTreeNodeFactory *nodeMaker;
    bool                   deleteWithNodes;

public:
    TreeRoot(RootedTreeNodeFactory *nodeMaker_, bool deleteWithNodes_)
        : rootNode(NULL),
          nodeMaker(nodeMaker_),
          deleteWithNodes(deleteWithNodes_)
    {
        /*! Create a TreeRoot for a RootedTree.
         * Purpose:
         * - act as TreeNodeFactory
         * - place to store the current rootNode
         * - place to store other tree related information by deriving from TreeRoot
         *
         * @param nodeMaker_ heap-copy of a RootedTreeNodeFactory, will be deleted when this is destructed
         * @param deleteWithNodes_ true -> delete TreeRoot when the rootNode gets destroyed (TreeRoot needs to be a heap-copy in that case)
         *
         * Ressource handling of the tree structure is quite difficult (and error-prone).
         * There are two common use-cases:
         * 1. TreeRoot is owned by some other object/scope
         *    - pass false for deleteWithNodes_
         *    - you may or may not destroy (parts of) the RootedTree manually
         * 2. TreeRoot is owned by the RootedTree
         *    - pass true for deleteWithNodes_
         *    - when the rootNode gets destroyed, the TreeRoot will be destroyed as well
         */
    }
    virtual ~TreeRoot();
    virtual void change_root(RootedTree *old, RootedTree *newroot);

    void delete_by_node() {
        if (deleteWithNodes) {
            rt_assert(!rootNode);
            delete this;
        }
    }

    // TreeNodeFactory interface
    inline GBT_TREE *makeNode() const OVERRIDE;

    RootedTree *get_root_node() { return rootNode; }
    const RootedTree *get_root_node() const { return rootNode; }

    ARB_edge find_innermost_edge();
};

enum TreeOrder { // contains bit values!
    BIG_BRANCHES_TO_TOP    = 0, // bit 0 = top or bottom
    BIG_BRANCHES_TO_BOTTOM = 1,
    BIG_BRANCHES_TO_EDGE   = 2, // bit 1 = center or edge
};

class RootedTree : public GBT_TREE { // derived from Noncopyable
    friend void TreeRoot::change_root(RootedTree *old, RootedTree *newroot);

    TreeRoot *tree_root;

    // ------------------
    //      functions

    GBT_LEN& length_ref() { return is_leftson() ? father->leftlen : father->rightlen; }
    const GBT_LEN& length_ref() const { return const_cast<RootedTree*>(this)->length_ref(); }

    void reorder_subtree(TreeOrder mode);

protected:
    void set_tree_root(TreeRoot *new_root);

    bool at_root() const {
        //! return true for root-node and its sons
        return !father || !father->father;
    }

public:
    RootedTree(TreeRoot *root)
        : tree_root(root)
    {}
    ~RootedTree() OVERRIDE {
        if (tree_root) {
            rt_assert(tree_root->get_root_node() == this); // you may only free the root-node or unlinked nodes (i.e. such with tree_root==NULL)

            TreeRoot *root = tree_root;
            root->TreeRoot::change_root(this, NULL);
            root->delete_by_node();
        }
    }

    void announce_tree_constructed() OVERRIDE {
        GBT_TREE::announce_tree_constructed();
        get_tree_root()->change_root(NULL, this);
    }

    virtual unsigned get_leaf_count() const = 0;
    virtual void compute_tree()             = 0;

    DEFINE_SIMPLE_TREE_RELATIVES_ACCESSORS(RootedTree);

    void forget_origin() { set_tree_root(NULL); }

    bool is_inside(const RootedTree *subtree) const;
    bool is_anchestor_of(const RootedTree *descendant) const {
        return !is_leaf && descendant != this && descendant->is_inside(this);
    }

    TreeRoot *get_tree_root() const { return tree_root; }

    const RootedTree *get_root_node() const {
        if (!tree_root) return NULL; // nodes removed from tree have no root-node

        const RootedTree *root = tree_root->get_root_node();
        rt_assert(is_inside(root)); // this is not in tree - behavior of get_root_node() changed!
        return root;
    }
    RootedTree *get_root_node() { return const_cast<RootedTree*>(const_cast<const RootedTree*>(this)->get_root_node()); }

    bool is_root_node() const { return get_root_node() == this; }
    virtual void set_root();

    RootedTree *get_brother() {
        rt_assert(!is_root_node()); // root node has no brother
        rt_assert(father);          // this is a removed node (not root, but no father)
        return is_leftson() ? get_father()->get_rightson() : get_father()->get_leftson();
    }
    const RootedTree *get_brother() const {
        return const_cast<const RootedTree*>(const_cast<RootedTree*>(this)->get_brother());
    }

    GBT_LEN get_branchlength() const { return length_ref(); }
    void set_branchlength(GBT_LEN newlen) { length_ref() = newlen; }

    const char *group_name() const { return gb_node && name ? name : NULL; }

    virtual void swap_sons() {
        rt_assert(!is_leaf); // @@@ if never fails -> remove condition below 
        if (!is_leaf) {
            std::swap(leftson, rightson);
            std::swap(leftlen, rightlen);
        }
    }
    void rotate_subtree(); // flip whole subtree ( = recursive swap_sons())
    void reorder_tree(TreeOrder mode);

    RootedTree *findLeafNamed(const char *wantedName);

#if defined(PROVIDE_TREE_STRUCTURE_TESTS)
    void assert_valid() const;
#endif // PROVIDE_TREE_STRUCTURE_TESTS
};

inline GBT_TREE *TreeRoot::makeNode() const {
    return nodeMaker->makeNode(const_cast<TreeRoot*>(this));
}

// ---------------------------------------------------------------------------------------
//      macros to overwrite accessors in classes derived from TreeRoot or RootedTree:

#define DEFINE_TREE_ROOT_ACCESSORS(RootType, TreeType)                                  \
    DEFINE_DOWNCAST_ACCESSORS(TreeType, get_root_node, TreeRoot::get_root_node())

#define DEFINE_TREE_RELATIVES_ACCESSORS(TreeType)                                       \
    DEFINE_SIMPLE_TREE_RELATIVES_ACCESSORS(TreeType);                                   \
    DEFINE_DOWNCAST_ACCESSORS(TreeType, get_brother, RootedTree::get_brother());        \
    DEFINE_DOWNCAST_ACCESSORS(TreeType, get_root_node, RootedTree::get_root_node());    \
    TreeType *findLeafNamed(const char *wantedName) { return DOWNCAST(TreeType*, RootedTree::findLeafNamed(wantedName)); }

#define DEFINE_TREE_ACCESSORS(RootType, TreeType)                                       \
    DEFINE_DOWNCAST_ACCESSORS(RootType, get_tree_root, RootedTree::get_tree_root());    \
    DEFINE_TREE_RELATIVES_ACCESSORS(TreeType)


// -------------------------
//      structure tests

#if defined(PROVIDE_TREE_STRUCTURE_TESTS)
template <typename TREE>
inline void assert_tree_has_valid_structure(const TREE *tree, bool IF_ASSERTION_USED(acceptNULL)) {
    rt_assert(acceptNULL || tree);
    if (tree) tree->assert_valid();
}
#endif

#if defined(AUTO_CHECK_TREE_STRUCTURE)
#define ASSERT_VALID_TREE(tree)         assert_tree_has_valid_structure(tree, false)
#define ASSERT_VALID_TREE_OR_NULL(tree) assert_tree_has_valid_structure(tree, true)
#else
#define ASSERT_VALID_TREE(tree)
#define ASSERT_VALID_TREE_OR_NULL(tree)
#endif // AUTO_CHECK_TREE_STRUCTURE

#if defined(PROVIDE_TREE_STRUCTURE_TESTS) && defined(UNIT_TESTS)
#define TEST_ASSERT_VALID_TREE(tree)         assert_tree_has_valid_structure(tree, false)
#define TEST_ASSERT_VALID_TREE_OR_NULL(tree) assert_tree_has_valid_structure(tree, true)
#else
#define TEST_ASSERT_VALID_TREE(tree)
#define TEST_ASSERT_VALID_TREE_OR_NULL(tree)
#endif

// ----------------------
//      ARB_edge_type

enum ARB_edge_type {
    EDGE_TO_ROOT, // edge points towards the root node
    EDGE_TO_LEAF, // edge points away from the root node
    ROOT_EDGE, // edge between sons of root node
};

class ARB_edge {
    // ARB_edge is a directional edge between two non-root-nodes of the same tree

    RootedTree    *from, *to;
    ARB_edge_type  type;

    ARB_edge_type detectType() const {
        rt_assert(to != from);
        rt_assert(!from->is_root_node());                // edges cannot be at root - use edge between sons of root!
        rt_assert(!to->is_root_node());

        if (from->father == to) return EDGE_TO_ROOT;
        if (to->father == from) return EDGE_TO_LEAF;

        rt_assert(from->get_brother() == to);       // no edge exists between 'from' and 'to'
        rt_assert(to->get_father()->is_root_node());
        return ROOT_EDGE;
    }

public:
    ARB_edge(RootedTree *From, RootedTree *To)
        : from(From)
        , to(To)
        , type(detectType())
    {}
    ARB_edge(RootedTree *From, RootedTree *To, ARB_edge_type Type)
        : from(From)
        , to(To)
        , type(Type)
    {
        rt_assert(type == detectType());
    }
    ARB_edge(const ARB_edge& otherEdge)
        : from(otherEdge.from)
        , to(otherEdge.to)
        , type(otherEdge.type)
    {
        rt_assert(type == detectType());
    }

    ARB_edge_type get_type() const { return type; }
    RootedTree *source() const { return from; }
    RootedTree *dest() const { return to; }

    RootedTree *son() const  { return type == EDGE_TO_ROOT ? from : to; }
    RootedTree *other() const  { return type == EDGE_TO_ROOT ? to : from; }

    GBT_LEN length() const {
        if (type == ROOT_EDGE) return from->get_branchlength() + to->get_branchlength();
        return other()->get_branchlength();
    }

    ARB_edge inverse() const {
        return ARB_edge(to, from, ARB_edge_type(type == ROOT_EDGE ? ROOT_EDGE : (EDGE_TO_LEAF+EDGE_TO_ROOT)-type));
    }

    // iterator functions: endlessly iterate over all edges of tree
    ARB_edge next() const { // descends rightson first
        if (type == EDGE_TO_ROOT) {
            rt_assert(from->is_son_of(to));
            if (from->is_rightson()) return ARB_edge(to, to->get_leftson(), EDGE_TO_LEAF);
            RootedTree *father = to->get_father();
            if (father->is_root_node()) return ARB_edge(to, to->get_brother(), ROOT_EDGE);
            return ARB_edge(to, father, EDGE_TO_ROOT);
        }
        if (to->is_leaf) return inverse();
        return ARB_edge(to, to->get_rightson(), EDGE_TO_LEAF);
    }

    bool operator == (const ARB_edge& otherEdge) const {
        return from == otherEdge.from && to == otherEdge.to;
    }

    void set_root() { son()->set_root(); }
};

#else
#error RootedTree.h included twice
#endif // ROOTEDTREE_H
