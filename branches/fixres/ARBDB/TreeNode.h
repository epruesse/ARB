// ================================================================ //
//                                                                  //
//   File      : TreeNode.h                                         //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in December 2013   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef TREENODE_H
#define TREENODE_H

#ifndef ARBDBT_H
#include "arbdbt.h"
#endif
#ifndef _GLIBCXX_ALGORITHM
#include <algorithm>
#endif

#define rt_assert(cond) arb_assert(cond)

#if defined(DEBUG) || defined(UNIT_TESTS) // UT_DIFF
#define PROVIDE_TREE_STRUCTURE_TESTS
#endif
#if defined(DEVEL_RALF) && defined(PROVIDE_TREE_STRUCTURE_TESTS)
#define AUTO_CHECK_TREE_STRUCTURE // Note: dramatically slows down most tree operations
#endif

class TreeRoot;
class TreeNode;
class ARB_edge;

enum TreeOrder { // contains bit values!
    ORDER_BIG_DOWN      = 1, // bit 0 set -> big branches down
    ORDER_BIG_TO_EDGE   = 2, // bit 1 set -> big branches to edge
    ORDER_BIG_TO_CENTER = 4, // bit 2 set -> big branches to center
    ORDER_ALTERNATING   = 8, // bit 3 set -> alternate bit 0

    // user visible orders:
    BIG_BRANCHES_TO_TOP      = 0,
    BIG_BRANCHES_TO_BOTTOM   = ORDER_BIG_DOWN,
    BIG_BRANCHES_TO_EDGE     = ORDER_BIG_TO_EDGE,
    BIG_BRANCHES_TO_CENTER   = ORDER_BIG_TO_CENTER,
    BIG_BRANCHES_ALTERNATING = ORDER_BIG_TO_CENTER|ORDER_ALTERNATING,
};

struct TreeNodeFactory { // @@@ move into TreeNode?
    virtual ~TreeNodeFactory() {}
    virtual TreeNode *makeNode() const             = 0;
    virtual void destroyNode(TreeNode *node) const = 0;
};

struct RootedTreeNodeFactory { // acts similar to TreeNodeFactory for trees with root
    virtual ~RootedTreeNodeFactory() {}
    virtual TreeNode *makeNode(TreeRoot *root) const               = 0;
    virtual void destroyNode(TreeRoot *root, TreeNode *node) const = 0;
};

class TreeRoot : public TreeNodeFactory, virtual Noncopyable {
    TreeNode            *rootNode; // root node of the tree
    RootedTreeNodeFactory *nodeMaker;
    bool                   deleteWithNodes;

public:
    TreeRoot(RootedTreeNodeFactory *nodeMaker_, bool deleteWithNodes_)
        : rootNode(NULL),
          nodeMaker(nodeMaker_),
          deleteWithNodes(deleteWithNodes_)
    {
        /*! Create a TreeRoot for a TreeNode.
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
         *    - you may or may not destroy (parts of) the TreeNode manually
         * 2. TreeRoot is owned by the TreeNode
         *    - pass true for deleteWithNodes_
         *    - when the rootNode gets destroyed, the TreeRoot will be destroyed as well
         */
    }
    virtual ~TreeRoot();
    virtual void change_root(TreeNode *old, TreeNode *newroot);

    void delete_by_node() {
        if (deleteWithNodes) {
            rt_assert(!rootNode);
            delete this;
        }
    }

    // TreeNodeFactory interface
    inline TreeNode *makeNode() const OVERRIDE;
    inline void destroyNode(TreeNode *node) const OVERRIDE;

    TreeNode *get_root_node() { return rootNode; }
    const TreeNode *get_root_node() const { return rootNode; }

    ARB_edge find_innermost_edge();
};

#define DEFINE_SIMPLE_TREE_RELATIVES_ACCESSORS(TreeType)        \
    DEFINE_DOWNCAST_ACCESSORS(TreeType, get_father, father);    \
    DEFINE_DOWNCAST_ACCESSORS(TreeType, get_leftson, leftson);  \
    DEFINE_DOWNCAST_ACCESSORS(TreeType, get_rightson, rightson)

struct ELIMtree : virtual Noncopyable {
    bool      is_leaf;
    ELIMtree *father, *leftson, *rightson;
    GBT_LEN   leftlen, rightlen;
    GBDATA   *gb_node;
    char     *name;

private:
    friend class TreeNode;

    char *remark_branch; // remark_branch normally contains some bootstrap value in format 'xx%'
                         // if you store other info there, please make sure that this info does not start with digits!!
                         // Otherwise the tree export routines will not work correctly!

    GBT_LEN& length_ref() { return is_leftson() ? father->leftlen : father->rightlen; }
    const GBT_LEN& length_ref() const { return const_cast<ELIMtree*>(this)->length_ref(); }

protected:
    ELIMtree*& self_ref() {
        return is_leftson() ? father->leftson : father->rightson;
    }
    void unlink_from_father() {
        if (father) {
            self_ref() = NULL;
            father     = NULL;
        }
    }

    char *swap_remark(char *new_remark) {
        char *result  = remark_branch;
        remark_branch = new_remark;
        return result;
    }

    virtual ~ELIMtree() {
        delete leftson;  gb_assert(!leftson);
        delete rightson; gb_assert(!rightson);

        unlink_from_father();

        free(name);
        free(remark_branch);
    }
    friend class ELIMtree_NodeFactory;
    virtual void destroy()  {
        delete this;
    }

public:
    ELIMtree()
        : is_leaf(false),
          father(NULL), leftson(NULL), rightson(NULL),
          leftlen(0.0), rightlen(0.0),
          gb_node(NULL),
          name(NULL),
          remark_branch(NULL)
    {}
    static void destroy(ELIMtree *that)  { // replacement for destructor
        if (that) delete that;
    }

    DEFINE_SIMPLE_TREE_RELATIVES_ACCESSORS(ELIMtree);

    virtual void announce_tree_constructed() {
        // (has to be) called after tree has been constructed
        gb_assert(!father); // has to be called with root-node!
    }

    bool is_son_of(const ELIMtree *Father) const {
        return father == Father &&
            (father->leftson == this || father->rightson == this);
    }
    bool is_leftson() const {
        gb_assert(is_son_of(get_father())); // do only call with sons!
        return father->leftson == this;
    }
    bool is_rightson() const {
        gb_assert(is_son_of(get_father())); // do only call with sons!
        return father->rightson == this;
    }

    bool is_root_node() const { return !father; }

    bool is_inside(const ELIMtree *subtree) const {
        return this == subtree || (father && get_father()->is_inside(subtree));
    }
    bool is_anchestor_of(const ELIMtree *descendant) const {
        return !is_leaf && descendant != this && descendant->is_inside(this);
    }
    const ELIMtree *ancestor_common_with(const ELIMtree *other) const;
    ELIMtree *ancestor_common_with(ELIMtree *other) { return const_cast<ELIMtree*>(ancestor_common_with(other)); }

    GBT_LEN get_branchlength() const { return length_ref(); }
    void set_branchlength(GBT_LEN newlen) {
        gb_assert(!is_nan_or_inf(newlen));
        length_ref() = newlen;
    }

    GBT_LEN get_branchlength_unrooted() const {
        //! like get_branchlength, but root-edge is treated correctly
        if (father->is_root_node()) {
            return father->leftlen+father->rightlen;
        }
        return get_branchlength();
    }
    void set_branchlength_unrooted(GBT_LEN newlen) {
        //! like set_branchlength, but root-edge is treated correctly
        if (father->is_root_node()) {
            father->leftlen = father->rightlen = newlen/2;
        }
        else {
            set_branchlength(newlen);
        }
    }

    GBT_LEN sum_child_lengths() const;
    GBT_LEN root_distance() const {
        //! returns distance from node to root (including nodes own length)
        return father ? get_branchlength()+father->root_distance() : 0.0;
    }
    GBT_LEN intree_distance_to(const ELIMtree *other) const {
        const ELIMtree *ancestor = ancestor_common_with(other);
        return root_distance() + other->root_distance() - 2*ancestor->root_distance();
    }

    enum bs100_mode { BS_UNDECIDED, BS_REMOVE, BS_INSERT };
    bs100_mode toggle_bootstrap100(bs100_mode mode = BS_UNDECIDED); // toggle bootstrap '100%' <-> ''
    void       remove_bootstrap();                                  // remove bootstrap values from subtree

    void reset_branchlengths();                     // reset branchlengths of subtree to tree_defaults::LENGTH
    void scale_branchlengths(double factor);

    void bootstrap2branchlen();                     // copy bootstraps to branchlengths
    void branchlen2bootstrap();                     // copy branchlengths to bootstraps

    GBT_RemarkType parse_bootstrap(double& bootstrap) const {
        /*! analyse 'remark_branch' and return GBT_RemarkType.
         * If result is REMARK_BOOTSTRAP, 'bootstrap' contains the bootstrap value
         */
        if (!remark_branch) return REMARK_NONE;

        const char *end = 0;
        bootstrap       = strtod(remark_branch, (char**)&end);

        bool is_bootstrap = end[0] == '%' && end[1] == 0;
        return is_bootstrap ? REMARK_BOOTSTRAP : REMARK_OTHER;
    }
    const char *get_remark() const { return remark_branch; }
    void use_as_remark(char *newRemark) { freeset(remark_branch, newRemark); }
    void set_remark(const char *newRemark) { freedup(remark_branch, newRemark); }
    void set_bootstrap(double bootstrap) { use_as_remark(GBS_global_string_copy("%i%%", int(bootstrap+0.5))); } // @@@ protect against "100%"?
    void remove_remark() { use_as_remark(NULL); }
};

class TreeNode : public ELIMtree { // derived from Noncopyable
    friend void TreeRoot::change_root(TreeNode *old, TreeNode *newroot);

    TreeRoot *tree_root;

    // ------------------
    //      functions

    void reorder_subtree(TreeOrder mode);

protected:
    void set_tree_root(TreeRoot *new_root);

    bool at_root() const {
        //! return true for root-node and its sons
        return !father || !father->father;
    }
    ~TreeNode() OVERRIDE {
        if (tree_root) {
            rt_assert(tree_root->get_root_node() == this); // you may only free the root-node or unlinked nodes (i.e. such with tree_root==NULL)

            TreeRoot *root = tree_root;
            root->TreeRoot::change_root(this, NULL);
            root->delete_by_node();
        }
    }
    void destroy() OVERRIDE {
        rt_assert(this);
        TreeRoot *myRoot = get_tree_root();
        rt_assert(myRoot); // if this fails, you need to use destroy(TreeRoot*), i.e. destroy(TreeNode*, TreeRoot*)
        myRoot->destroyNode(this);
    }
    void destroy(TreeRoot *viaRoot) {
        rt_assert(this);
#if defined(ASSERTION_USED)
        TreeRoot *myRoot = get_tree_root();
        rt_assert(!myRoot || myRoot == viaRoot);
#endif
        viaRoot->destroyNode(this);
    }

public:
    TreeNode(TreeRoot *root)
        : tree_root(root)
    {}
    static void destroy(TreeNode *that, TreeRoot *root) {
        if (that) that->destroy(root);
    }

    TreeNode *fixDeletedSon(); // @@@ review (design)

    void announce_tree_constructed() OVERRIDE {
        ELIMtree::announce_tree_constructed();
        get_tree_root()->change_root(NULL, this);
    }

    virtual unsigned get_leaf_count() const = 0;
    virtual void compute_tree()             = 0;

    DEFINE_SIMPLE_TREE_RELATIVES_ACCESSORS(TreeNode);

    void forget_origin() { set_tree_root(NULL); }

    TreeRoot *get_tree_root() const { return tree_root; }

    const TreeNode *get_root_node() const {
        if (!tree_root) return NULL; // nodes removed from tree have no root-node

        const TreeNode *root = tree_root->get_root_node();
        rt_assert(is_inside(root)); // this is not in tree - behavior of get_root_node() changed!
        return root;
    }
    TreeNode *get_root_node() { return const_cast<TreeNode*>(const_cast<const TreeNode*>(this)->get_root_node()); }

    bool is_root_node() const { return get_root_node() == this; }
    virtual void set_root();

    TreeNode *get_brother() {
        rt_assert(!is_root_node()); // root node has no brother
        rt_assert(father);          // this is a removed node (not root, but no father)
        return is_leftson() ? get_father()->get_rightson() : get_father()->get_leftson();
    }
    const TreeNode *get_brother() const {
        return const_cast<const TreeNode*>(const_cast<TreeNode*>(this)->get_brother());
    }

    bool is_named_group() const {
        rt_assert(!is_leaf); // checking whether a leaf is a group
        return gb_node && name;
    }
    const char *get_group_name() const {
        return is_named_group() ? name : NULL;
    }

    virtual void swap_sons() {
        rt_assert(!is_leaf); // @@@ if never fails -> remove condition below 
        if (!is_leaf) {
            std::swap(leftson, rightson);
            std::swap(leftlen, rightlen);
        }
    }
    void rotate_subtree(); // flip whole subtree ( = recursive swap_sons())
    void reorder_tree(TreeOrder mode);

    TreeNode *findLeafNamed(const char *wantedName);

    GBT_LEN reset_length_and_bootstrap() {
        //! remove remark + zero but return branchlen
        remove_remark();
        GBT_LEN len = get_branchlength_unrooted();
        set_branchlength_unrooted(0.0);
        return len;
    }

    struct multifurc_limits {
        double bootstrap;
        double branchlength;
        bool   applyAtLeafs;
        multifurc_limits(double bootstrap_, double branchlength_, bool applyAtLeafs_)
            : bootstrap(bootstrap_),
              branchlength(branchlength_),
              applyAtLeafs(applyAtLeafs_)
        {}
    };
    class LengthCollector;

    void multifurcate();
    void set_branchlength_preserving(GBT_LEN new_len);

    void multifurcate_whole_tree(const multifurc_limits& below);
private:
    void eliminate_and_collect(const multifurc_limits& below, LengthCollector& collect);
public:

#if defined(PROVIDE_TREE_STRUCTURE_TESTS)
    void assert_valid() const;
#endif // PROVIDE_TREE_STRUCTURE_TESTS
};

inline void destroy(ELIMtree *that) {
    ELIMtree::destroy(that);
}
inline void destroy(TreeNode *that, TreeRoot *root) {
    TreeNode::destroy(that, root);
}

inline TreeNode *TreeRoot::makeNode() const {
    return nodeMaker->makeNode(const_cast<TreeRoot*>(this));
}
inline void TreeRoot::destroyNode(TreeNode *node) const {
    nodeMaker->destroyNode(const_cast<TreeRoot*>(this), DOWNCAST(TreeNode*, node));
}

// ---------------------------------------------------------------------------------------
//      macros to overwrite accessors in classes derived from TreeRoot or TreeNode:

#define DEFINE_TREE_ROOT_ACCESSORS(RootType, TreeType)                                  \
    DEFINE_DOWNCAST_ACCESSORS(TreeType, get_root_node, TreeRoot::get_root_node())

#define DEFINE_TREE_RELATIVES_ACCESSORS(TreeType)                                       \
    DEFINE_SIMPLE_TREE_RELATIVES_ACCESSORS(TreeType);                                   \
    DEFINE_DOWNCAST_ACCESSORS(TreeType, get_brother, TreeNode::get_brother());        \
    DEFINE_DOWNCAST_ACCESSORS(TreeType, get_root_node, TreeNode::get_root_node());    \
    TreeType *findLeafNamed(const char *wantedName) { return DOWNCAST(TreeType*, TreeNode::findLeafNamed(wantedName)); }

#define DEFINE_TREE_ACCESSORS(RootType, TreeType)                                       \
    DEFINE_DOWNCAST_ACCESSORS(RootType, get_tree_root, TreeNode::get_tree_root());    \
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

// --------------------
//      SimpleTree

struct SimpleRoot : public TreeRoot {
    inline SimpleRoot();
};

class SimpleTree : public TreeNode {
protected:
    ~SimpleTree() OVERRIDE {}
    friend class SimpleTree_NodeFactory;
public:
    SimpleTree(SimpleRoot *sroot) : TreeNode(sroot) {
    }

    // TreeNode interface
    unsigned get_leaf_count() const OVERRIDE {
        rt_assert(0); // @@@ impl?
        return 0;
    }
    void compute_tree() OVERRIDE {}
};

class SimpleTree_NodeFactory : public RootedTreeNodeFactory {
    TreeNode *makeNode(TreeRoot *root) const OVERRIDE { return new SimpleTree(DOWNCAST(SimpleRoot*, root)); }
    void destroyNode(TreeRoot*, TreeNode *node) const OVERRIDE { delete DOWNCAST(SimpleTree*,node); }
};

SimpleRoot::SimpleRoot() : TreeRoot(new SimpleTree_NodeFactory, true) {}

// ----------------------
//      ARB_edge_type

enum ARB_edge_type {
    EDGE_TO_ROOT, // edge points towards the root node
    EDGE_TO_LEAF, // edge points away from the root node
    ROOT_EDGE, // edge between sons of root node
};

class ARB_edge {
    // ARB_edge is a directional edge between two non-root-nodes of the same tree

    TreeNode    *from, *to;
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

    GBT_LEN adjacent_distance() const;
    GBT_LEN length_or_adjacent_distance() const {
        {
            GBT_LEN len = length();
            if (len>0.0) return len;
        }
        return adjacent_distance();
    }

    void virtually_add_or_distribute_length_forward(GBT_LEN len, TreeNode::LengthCollector& collect) const;
    void virtually_distribute_length_forward(GBT_LEN len, TreeNode::LengthCollector& collect) const;
public:
    void virtually_distribute_length(GBT_LEN len, TreeNode::LengthCollector& collect) const; // @@@ hm public :(
private:

#if defined(UNIT_TESTS) // UT_DIFF
    friend void TEST_edges();
#endif

public:
    ARB_edge(TreeNode *From, TreeNode *To)
        : from(From)
        , to(To)
        , type(detectType())
    {}
    ARB_edge(TreeNode *From, TreeNode *To, ARB_edge_type Type)
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
    TreeNode *source() const { return from; }
    TreeNode *dest() const { return to; }

    TreeNode *son() const  { return type == EDGE_TO_ROOT ? from : to; }
    TreeNode *other() const  { return type == EDGE_TO_ROOT ? to : from; }

    GBT_LEN length() const {
        if (type == ROOT_EDGE) return from->get_branchlength() + to->get_branchlength();
        return son()->get_branchlength();
    }
    void set_length(GBT_LEN len)  {
        if (type == ROOT_EDGE) {
            from->set_branchlength(len/2);
            to->set_branchlength(len/2);
        }
        else {
            son()->set_branchlength(len);
        }
    }
    GBT_LEN eliminate() {
        //! eliminates edge (zeroes length and bootstrap). returns eliminated length.
        if (type == ROOT_EDGE) {
            return source()->reset_length_and_bootstrap() + dest()->reset_length_and_bootstrap();
        }
        return son()->reset_length_and_bootstrap();
    }

    ARB_edge inverse() const {
        return ARB_edge(to, from, ARB_edge_type(type == ROOT_EDGE ? ROOT_EDGE : (EDGE_TO_LEAF+EDGE_TO_ROOT)-type));
    }

    // iterator functions: endlessly iterate over all edges of tree
    ARB_edge next() const { // descends rightson first
        if (type == EDGE_TO_ROOT) {
            rt_assert(from->is_son_of(to));
            if (from->is_rightson()) return ARB_edge(to, to->get_leftson(), EDGE_TO_LEAF);
            TreeNode *father = to->get_father();
            if (father->is_root_node()) return ARB_edge(to, to->get_brother(), ROOT_EDGE);
            return ARB_edge(to, father, EDGE_TO_ROOT);
        }
        if (at_leaf()) return inverse();
        return ARB_edge(to, to->get_rightson(), EDGE_TO_LEAF);
    }
    ARB_edge otherNext() const { // descends leftson first (slow)
        if (at_leaf()) return inverse();
        return next().inverse().next();
    }

    bool operator == (const ARB_edge& otherEdge) const {
        return from == otherEdge.from && to == otherEdge.to;
    }

    bool at_leaf() const {
        //! true if edge is leaf edge and points towards the leaf
        return dest()->is_leaf;
    }

    void set_root() { son()->set_root(); }

    void multifurcate();
};

inline ARB_edge parentEdge(TreeNode *son) {
    /*! returns edge to father (or to brother for sons of root).
     * Cannot be called with root-node (but can be called with each end of any ARB_edge)
     */
    TreeNode *father = son->get_father();
    rt_assert(father);

    if (father->is_root_node()) return ARB_edge(son, son->get_brother(), ROOT_EDGE);
    return ARB_edge(son, father, EDGE_TO_ROOT);
}

#else
#error TreeNode.h included twice
#endif // TREENODE_H
