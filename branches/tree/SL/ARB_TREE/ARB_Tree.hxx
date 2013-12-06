// =============================================================== //
//                                                                 //
//   File      : ARB_Tree.hxx                                      //
//   Purpose   : Tree types with sequence knowledge                //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2009   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef ARB_TREE_HXX
#define ARB_TREE_HXX

#ifndef ALIVIEW_HXX
#include <AliView.hxx>
#endif
#ifndef ARBDBT_H
#include <arbdbt.h>
#endif
#ifndef DOWNCAST_H
#include <downcast.h>
#endif
#ifndef ROOTEDTREE_H
#include <RootedTree.h>
#endif

#define at_assert(cond) arb_assert(cond)

#if defined(DEBUG)
#if defined(DEVEL_RALF)
// #define CHECK_TREE_STRUCTURE
#endif // DEVEL_RALF
#endif // DEBUG

typedef void (*ARB_tree_node_del_cb)(GBDATA*, class ARB_seqtree*);

class ARB_seqtree_root;
class ARB_seqtree;
class ARB_edge;
class AP_weights;
class AP_sequence;
class AliView;

struct RootedTreeNodeFactory { // acts similar to TreeNodeFactory for trees with root
    virtual ~RootedTreeNodeFactory() {}
    virtual ARB_seqtree *makeNode(ARB_seqtree_root *root) const = 0;
};

class ARB_seqtree_root : public TreeRoot, public TreeNodeFactory, virtual Noncopyable {
    AliView     *ali;
    ARB_seqtree *rootNode; // root node of the tree

    AP_sequence *seqTemplate;

    // following variables are set, if the tree has been loaded from DB
    char   *tree_name;                              // name of the tree in DB
    GBDATA *gb_tree;                                // tree container in DB

    bool isLinkedToDB;
    bool addDeleteCallbacks;

    RootedTreeNodeFactory *nodeMaker;

protected:
    void set_gb_tree(GBDATA *gbTree) {
        at_assert(implicated(gb_tree, gb_tree == gbTree));
        at_assert(tree_name);
        gb_tree = gbTree;
    }

public:
    ARB_seqtree_root(AliView *aliView, RootedTreeNodeFactory *nodeMaker_, AP_sequence *seqTempl, bool add_delete_callbacks);
    virtual ~ARB_seqtree_root();

    // TreeNodeFactory interface
    inline GBT_TREE *makeNode() const OVERRIDE;

    virtual void change_root(ARB_seqtree *old, ARB_seqtree *newroot);

    const AliView *get_aliview() const { return ali; }

    const AP_filter *get_filter() const { return ali->get_filter(); }
    const AP_weights *get_weights() const { return ali->get_weights(); }

    GBDATA *get_gb_main() const { return ali->get_gb_main(); }

    GBDATA *get_gb_tree() const { return gb_tree; } // NULL if no tree loaded (or tree disappeared from DB)
    void tree_deleted_cb(GBDATA *gb_tree_del);      // internal

    const char *get_tree_name() const { return tree_name; } // NULL if no tree loaded (or tree disappeared from DB)

    ARB_seqtree *get_root_node() { return rootNode; }
    const ARB_seqtree *get_root_node() const { return rootNode; }

    virtual GB_ERROR loadFromDB(const char *name) __ATTR__USERESULT;
    virtual GB_ERROR saveToDB() __ATTR__USERESULT;

    const AP_sequence *get_seqTemplate() const { return seqTemplate; }

    GB_ERROR linkToDB(int *zombies, int *duplicates) __ATTR__USERESULT;
    void unlinkFromDB(); // @@@ is (but should not be) unused
};


struct ARB_tree_info {
    size_t leafs;
    size_t innerNodes;
    size_t groups;
    size_t unlinked;                                // unlinked leafs (same as 'leafs' if tree is unlinked)
    size_t marked;                                  // leafs linked to marked species
    // size_t with_sequence; // @@@ add when AP_sequence is member of ARB_seqtree

    ARB_tree_info();

    size_t nodes()    const { return innerNodes+leafs; }
    size_t linked()   const { return leafs-unlinked; }
    size_t unmarked() const { return linked()-marked; }
};


class ARB_seqtree : public RootedTree { // derived from Noncopyable
    friend void     ARB_seqtree_root::change_root(ARB_seqtree *old, ARB_seqtree *newroot);
    friend GB_ERROR ARB_seqtree_root::loadFromDB(const char *name);
    friend GB_ERROR ARB_seqtree_root::linkToDB(int *zombies, int *duplicates);
    friend void ARB_seqtree_root::unlinkFromDB();

    ARB_seqtree_root *tree_root;
    AP_sequence   *seq;                          /* NULL if tree is unlinked
                                                  * otherwise automatically valid for leafs with gb_node!
                                                  * may be set manually for inner nodes
                                                  */

    // ------------------
    //      functions

    GBT_LEN& length_ref() { return is_leftson() ? father->leftlen : father->rightlen; } // @@@ move to GBT_TREE
    const GBT_LEN& length_ref() const { return const_cast<ARB_seqtree*>(this)->length_ref(); }

    void unloadSequences();
    void preloadLeafSequences();

    GB_ERROR add_delete_cb_rec(ARB_tree_node_del_cb cb) __ATTR__USERESULT;
    void remove_delete_cb_rec(ARB_tree_node_del_cb cb);

protected:
    void unlink_from_father() { // @@@ move to GBT_TREE
        if (father) {
            self_ref() = NULL;
            father     = NULL;
        }
    }

    void set_tree_root(ARB_seqtree_root *new_root);
    ARB_seqtree_root *get_tree_root() const { return tree_root; }

    AP_sequence *take_seq() {                       // afterwards not has no seq and caller is responsible for sequence
        AP_sequence *result = seq;
        seq = NULL;
        return result;
    }
    void replace_seq(AP_sequence *sequence);

public:
    ARB_seqtree(ARB_seqtree_root *root)
        : tree_root(root),
          seq(NULL)
    {}
    ~ARB_seqtree() OVERRIDE;

    DEFINE_SIMPLE_TREE_RELATIVES_ACCESSORS(ARB_seqtree);

    void calcTreeInfo(ARB_tree_info& info);

    bool is_inside(const ARB_seqtree *subtree) const;
    bool is_anchestor_of(const ARB_seqtree *descendant) const {
        return !is_leaf && descendant != this && descendant->is_inside(this);
    }

    // order in dendogram:
    bool is_upper_son() const { return is_leftson(); }
    bool is_lower_son() const { return is_rightson(); }

    const ARB_seqtree *get_root_node() const {
        const ARB_seqtree *root = get_tree_root()->get_root_node();
        at_assert(is_inside(root)); // this is not in tree - behavior of get_root_node() changed!
        return root;
    }
    ARB_seqtree *get_root_node() { return const_cast<ARB_seqtree*>(const_cast<const ARB_seqtree*>(this)->get_root_node()); }

    bool is_root_node() const { return get_root_node() == this; }

    ARB_seqtree *get_brother() {
        at_assert(!is_root_node()); // root node has no brother
        return is_leftson() ? get_father()->get_rightson() : get_father()->get_leftson();
    }
    const ARB_seqtree *get_brother() const {
        return const_cast<const ARB_seqtree*>(const_cast<ARB_seqtree*>(this)->get_brother());
    }

    GBT_LEN get_branchlength() const { return length_ref(); }
    void set_branchlength(GBT_LEN newlen) { length_ref() = newlen; }

    AP_sequence *get_seq() { return seq; }
    const AP_sequence *get_seq() const { return seq; }
    AP_sequence * set_seq(AP_sequence *sequence) {
        at_assert(!seq);                            // already set
        at_assert(sequence);
        seq = sequence;
        return seq;
    }

    const char *group_name() const { return gb_node && name ? name : NULL; }

#if defined(CHECK_TREE_STRUCTURE)
    void assert_valid() const;
#endif // CHECK_TREE_STRUCTURE

    void mark_subtree();
    bool contains_marked_species();

};

inline GBT_TREE *ARB_seqtree_root::makeNode() const {
    return nodeMaker->makeNode(const_cast<ARB_seqtree_root*>(this));
}

struct ARB_tree_predicate {
    virtual ~ARB_tree_predicate() {}
    virtual bool selects(const ARB_seqtree& tree) const = 0;
};

// macros to overwrite accessors in classes derived from ARB_seqtree_root or ARB_seqtree:
#define DEFINE_TREE_ROOT_ACCESSORS(RootType, TreeType)                                          \
    DEFINE_DOWNCAST_ACCESSORS(TreeType, get_root_node, ARB_seqtree_root::get_root_node())

#define DEFINE_TREE_RELATIVES_ACCESSORS(TreeType)                                       \
    DEFINE_SIMPLE_TREE_RELATIVES_ACCESSORS(TreeType);                                   \
    DEFINE_DOWNCAST_ACCESSORS(TreeType, get_brother, ARB_seqtree::get_brother());       \
    DEFINE_DOWNCAST_ACCESSORS(TreeType, get_root_node, ARB_seqtree::get_root_node())

#define DEFINE_TREE_ACCESSORS(RootType, TreeType)                                       \
    DEFINE_DOWNCAST_ACCESSORS(RootType, get_tree_root, ARB_seqtree::get_tree_root());   \
    DEFINE_TREE_RELATIVES_ACCESSORS(TreeType)

#if defined(CHECK_TREE_STRUCTURE)
#define ASSERT_VALID_TREE(tree) (tree)->assert_valid()
#else
#define ASSERT_VALID_TREE(tree)
#endif // CHECK_TREE_STRUCTURE


// ------------------------
//      ARB_countedTree
//      tree who knows its size

struct ARB_countedTree : public ARB_seqtree {
    explicit ARB_countedTree(ARB_seqtree_root *tree_root_)
        : ARB_seqtree(tree_root_)
    {}
    ~ARB_countedTree() OVERRIDE {}
    DEFINE_TREE_ACCESSORS(ARB_seqtree_root, ARB_countedTree);

    virtual size_t get_leaf_count() const = 0;
    virtual void init_tree()              = 0;      /* impl. shall initialize the tree
                                                     * (including some kind of leaf counter)
                                                     * needs to be called manually */

    size_t relative_position_in(const ARB_countedTree *upgroup) const;
};


// ----------------------
//      ARB_edge_type

enum ARB_edge_type {
    EDGE_TO_ROOT, // edge points towards the root node
    EDGE_TO_LEAF, // edge points away from the root node
    ROOT_EDGE, // edge between sons of root node
};

class ARB_edge {
    // ARB_edge is a directional edge between two non-root-nodes of the same tree

    ARB_seqtree      *from, *to;
    ARB_edge_type  type;

    ARB_edge_type detectType() const {
        at_assert(to != from);
        at_assert(!from->is_root_node());                // edges cannot be at root - use edge between sons of root!
        at_assert(!to->is_root_node());

        if (from->father == to) return EDGE_TO_ROOT;
        if (to->father == from) return EDGE_TO_LEAF;

        at_assert(from->get_brother() == to);       // no edge exists between 'from' and 'to'
        at_assert(to->get_father()->is_root_node());
        return ROOT_EDGE;
    }

public:
    ARB_edge(ARB_seqtree *From, ARB_seqtree *To)
        : from(From)
        , to(To)
        , type(detectType())
    {}
    ARB_edge(ARB_seqtree *From, ARB_seqtree *To, ARB_edge_type Type)
        : from(From)
        , to(To)
        , type(Type)
    {
        at_assert(type == detectType());
    }
    ARB_edge(const ARB_edge& otherEdge)
        : from(otherEdge.from)
        , to(otherEdge.to)
        , type(otherEdge.type)
    {}

    ARB_edge_type get_type() const { return type; }
    ARB_seqtree *source() const { return from; }
    ARB_seqtree *dest() const { return to; }

    ARB_seqtree *son() const  { return type == EDGE_TO_ROOT ? from : to; }
    ARB_seqtree *other() const  { return type == EDGE_TO_ROOT ? to : from; }

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
            at_assert(from->is_son_of(to));
            if (from->is_rightson()) return ARB_edge(to, to->get_leftson(), EDGE_TO_LEAF);
            ARB_seqtree *father = to->get_father();
            if (father->is_root_node()) return ARB_edge(to, to->get_brother(), ROOT_EDGE);
            return ARB_edge(to, father, EDGE_TO_ROOT);
        }
        if (to->is_leaf) return inverse();
        return ARB_edge(to, to->get_rightson(), EDGE_TO_LEAF);
    }

    bool operator == (const ARB_edge& otherEdge) const {
        return from == otherEdge.from && to == otherEdge.to;
    }
};



#else
#error ARB_Tree.hxx included twice
#endif // ARB_TREE_HXX
