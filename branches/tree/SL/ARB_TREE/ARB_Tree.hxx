// =============================================================== //
//                                                                 //
//   File      : ARB_Tree.hxx                                      //
//   Purpose   : C++ wrapper for (linked) GBT_TREEs                //
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

#define at_assert(cond) arb_assert(cond)

#if defined(DEBUG)
#if defined(DEVEL_RALF)
// #define CHECK_TREE_STRUCTURE
#endif // DEVEL_RALF
#endif // DEBUG

typedef void (*ARB_tree_node_del_cb)(GBDATA*, class ARB_tree*);

class ARB_tree;
class ARB_edge;
class AP_weights;
class AP_sequence;
class AliView;

class ARB_tree_root : virtual Noncopyable {
    AliView     *ali;
    ARB_tree    *rootNode;                          // root node of the tree
    ARB_tree    *nodeTemplate; // @@@ elim? use TreeNodeFactory here?
    AP_sequence *seqTemplate;

    // following variables are set, if the tree has been loaded from DB
    char   *tree_name;                              // name of the tree in DB
    GBDATA *gb_tree;                                // tree container in DB

    bool isLinkedToDB;
    bool addDeleteCallbacks;

protected:
    void set_gb_tree(GBDATA *gbTree) {
        at_assert(implicated(gb_tree, gb_tree == gbTree));
        at_assert(tree_name);
        gb_tree = gbTree;
    }

public:
    ARB_tree_root(AliView *aliView, const ARB_tree& nodeTempl, AP_sequence *seqTempl, bool add_delete_callbacks);

    virtual ~ARB_tree_root();

    virtual void change_root(ARB_tree *old, ARB_tree *newroot);

    const AliView *get_aliview() const { return ali; }

    const AP_filter *get_filter() const { return ali->get_filter(); }
    const AP_weights *get_weights() const { return ali->get_weights(); }

    GBDATA *get_gb_main() const { return ali->get_gb_main(); }

    GBDATA *get_gb_tree() const { return gb_tree; } // NULL if no tree loaded (or tree disappeared from DB)
    void tree_deleted_cb(GBDATA *gb_tree_del);      // internal

    const char *get_tree_name() const { return tree_name; } // NULL if no tree loaded (or tree disappeared from DB)

    ARB_tree *get_root_node() { return rootNode; }
    const ARB_tree *get_root_node() const { return rootNode; }

    const ARB_tree *get_nodeTemplate() const { return nodeTemplate; }

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
    // size_t with_sequence; // @@@ add when AP_sequence is member of ARB_tree

    ARB_tree_info();

    size_t nodes()    const { return innerNodes+leafs; }
    size_t linked()   const { return leafs-unlinked; }
    size_t unmarked() const { return linked()-marked; }
};


class ARB_tree : public GBT_TREE { // derived from Noncopyable
    friend void     ARB_tree_root::change_root(ARB_tree *old, ARB_tree *newroot);
    friend GB_ERROR ARB_tree_root::loadFromDB(const char *name);
    friend GB_ERROR ARB_tree_root::linkToDB(int *zombies, int *duplicates);
    friend void ARB_tree_root::unlinkFromDB();

    ARB_tree_root *tree_root;
    AP_sequence   *seq;                          /* NULL if tree is unlinked
                                                  * otherwise automatically valid for leafs with gb_node!
                                                  * may be set manually for inner nodes
                                                  */

    // ------------------
    //      functions

    GBT_LEN& length_ref() { return is_leftson() ? father->leftlen : father->rightlen; } // @@@ move to GBT_TREE
    const GBT_LEN& length_ref() const { return const_cast<ARB_tree*>(this)->length_ref(); }

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

    virtual void move_gbt_info(GBT_TREE *tree); // @@@ elim?

    void set_tree_root(ARB_tree_root *new_root);
    ARB_tree_root *get_tree_root() const { return tree_root; }

    AP_sequence *take_seq() {                       // afterwards not has no seq and caller is responsible for sequence
        AP_sequence *result = seq;

        seq = NULL;

        return result;
    }
    void replace_seq(AP_sequence *sequence);

public:
    ARB_tree(ARB_tree_root *root);
    virtual ~ARB_tree();                            // needed (see FAKE_VTAB_PTR)

    DEFINE_SIMPLE_TREE_RELATIVES_ACCESSORS(ARB_tree);

    virtual ARB_tree *dup() const = 0;              // create new ARB_tree element from prototype

    void calcTreeInfo(ARB_tree_info& info);

    bool is_inside(const ARB_tree *subtree) const;
    bool is_anchestor_of(const ARB_tree *descendant) const {
        return !is_leaf && descendant != this && descendant->is_inside(this);
    }

    // order in dendogram:
    bool is_upper_son() const { return is_leftson(); }
    bool is_lower_son() const { return is_rightson(); }

    const ARB_tree *get_root_node() const {
        const ARB_tree *root = get_tree_root()->get_root_node();
        at_assert(is_inside(root)); // this is not in tree - behavior of get_root_node() changed!
        return root;
    }
    ARB_tree *get_root_node() { return const_cast<ARB_tree*>(const_cast<const ARB_tree*>(this)->get_root_node()); }

    bool is_root_node() const { return get_root_node() == this; }

    ARB_tree *get_brother() {
        at_assert(!is_root_node()); // root node has no brother
        return is_leftson() ? get_father()->get_rightson() : get_father()->get_leftson();
    }
    const ARB_tree *get_brother() const {
        return const_cast<const ARB_tree*>(const_cast<ARB_tree*>(this)->get_brother());
    }

    GBT_LEN get_branchlength() const { return length_ref(); }
    void set_branchlength(GBT_LEN newlen) { length_ref() = newlen; }

    const GBT_TREE *get_gbt_tree() const { return (const GBT_TREE*)this; } // @@@ elim
    GBT_TREE *get_gbt_tree() { return (GBT_TREE*)this; } // @@@ elim

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

struct ARB_tree_predicate {
    virtual ~ARB_tree_predicate() {}
    virtual bool selects(const ARB_tree& tree) const = 0;
};

// macros to overwrite accessors in classes derived from ARB_tree_root or ARB_tree:
#define DEFINE_TREE_ROOT_ACCESSORS(RootType, TreeType)                                  \
    DEFINE_DOWNCAST_ACCESSORS(TreeType, get_root_node, ARB_tree_root::get_root_node())

#define DEFINE_TREE_RELATIVES_ACCESSORS(TreeType)                                       \
    DEFINE_SIMPLE_TREE_RELATIVES_ACCESSORS(TreeType);                                   \
    DEFINE_DOWNCAST_ACCESSORS(TreeType, get_brother, ARB_tree::get_brother());          \
    DEFINE_DOWNCAST_ACCESSORS(TreeType, get_root_node, ARB_tree::get_root_node())

#define DEFINE_TREE_ACCESSORS(RootType, TreeType)                                       \
    DEFINE_DOWNCAST_ACCESSORS(RootType, get_tree_root, ARB_tree::get_tree_root());      \
    DEFINE_TREE_RELATIVES_ACCESSORS(TreeType)

#if defined(CHECK_TREE_STRUCTURE)
#define ASSERT_VALID_TREE(tree) (tree)->assert_valid()
#else
#define ASSERT_VALID_TREE(tree)
#endif // CHECK_TREE_STRUCTURE


// ------------------------
//      ARB_countedTree
//      tree who knows its size

struct ARB_countedTree : public ARB_tree {
    ARB_countedTree(ARB_tree_root *tree_root_)
        : ARB_tree(tree_root_)
    {}
    virtual ~ARB_countedTree() OVERRIDE {}
    DEFINE_TREE_ACCESSORS(ARB_tree_root, ARB_countedTree);

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

    ARB_tree      *from, *to;
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
    ARB_edge(ARB_tree *From, ARB_tree *To)
        : from(From)
        , to(To)
        , type(detectType())
    {}
    ARB_edge(ARB_tree *From, ARB_tree *To, ARB_edge_type Type)
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
    ARB_tree *source() const { return from; }
    ARB_tree *dest() const { return to; }

    ARB_tree *son() const  { return type == EDGE_TO_ROOT ? from : to; }
    ARB_tree *other() const  { return type == EDGE_TO_ROOT ? to : from; }

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
            ARB_tree *father = to->get_father();
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
