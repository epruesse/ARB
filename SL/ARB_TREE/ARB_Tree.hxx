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
#ifndef DOWNCAST_H
#include <downcast.h>
#endif
#ifndef TREENODE_H
#include <TreeNode.h>
#endif
#ifndef AP_SEQUENCE_HXX
#include <AP_sequence.hxx>
#endif


#define at_assert(cond) arb_assert(cond)

typedef void (*ARB_tree_node_del_cb)(GBDATA*, class ARB_seqtree*);

class ARB_seqtree_root;
class ARB_seqtree;
class ARB_edge;
class AP_weights;
class AliView;

class ARB_seqtree_root : public TreeRoot { // derived from Noncopyable
    AliView     *ali;

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
    void set_gb_tree_and_name(GBDATA *gbTree, const char *name) {
        at_assert(!gb_tree);
        at_assert(!tree_name);
        gb_tree   = gbTree;
        tree_name = strdup(name);
    }

public:
    ARB_seqtree_root(AliView *aliView, AP_sequence *seqTempl, bool add_delete_callbacks);
    ~ARB_seqtree_root() OVERRIDE;

    DEFINE_TREE_ROOT_ACCESSORS(ARB_seqtree_root, ARB_seqtree);

    const AliView *get_aliview() const { return ali; }

    const AP_filter *get_filter() const { return ali->get_filter(); }
    const AP_weights *get_weights() const { return ali->get_weights(); }

    GBDATA *get_gb_main() const { return ali->get_gb_main(); }

    GBDATA *get_gb_tree() const { return gb_tree; } // NULL if no tree loaded (or tree disappeared from DB)
    void tree_deleted_cb(GBDATA *gb_tree_del);      // internal

    const char *get_tree_name() const { return tree_name; } // NULL if no tree loaded (or tree disappeared from DB)

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


class ARB_seqtree : public TreeNode { // derived from Noncopyable
    friend GB_ERROR ARB_seqtree_root::loadFromDB(const char *name);
    friend GB_ERROR ARB_seqtree_root::linkToDB(int *zombies, int *duplicates);
    friend void     ARB_seqtree_root::unlinkFromDB();

    AP_sequence   *seq;                          /* NULL if tree is unlinked
                                                  * otherwise automatically valid for leafs with gb_node!
                                                  * may be set manually for inner nodes
                                                  */

    // ------------------
    //      functions

    void unloadSequences();
    GB_ERROR preloadLeafSequences();

    GB_ERROR add_delete_cb_rec(ARB_tree_node_del_cb cb) __ATTR__USERESULT;
    void remove_delete_cb_rec(ARB_tree_node_del_cb cb);

protected:
    AP_sequence *take_seq() { // afterwards not has no seq and caller is responsible for sequence
        AP_sequence *result = seq;
        seq = NULL;
        return result;
    }
    void replace_seq(AP_sequence *sequence);

    ~ARB_seqtree() OVERRIDE;

public:
    ARB_seqtree(ARB_seqtree_root *root)
        : TreeNode(root),
          seq(NULL)
    {}

    DEFINE_TREE_ACCESSORS(ARB_seqtree_root, ARB_seqtree);

    void calcTreeInfo(ARB_tree_info& info);

    // order in dendogram:
    bool is_upper_son() const { return is_leftson(); }
    bool is_lower_son() const { return is_rightson(); }

    AP_sequence *get_seq() { return seq; }
    const AP_sequence *get_seq() const { return seq; }
    AP_sequence * set_seq(AP_sequence *sequence) {
        at_assert(!seq); // already set
#ifndef UNIT_TESTS // UT_DIFF
        // unit tests are allowed to leave sequence undefined // @@@ better solution?
        at_assert(sequence);
#endif
        seq = sequence;
        return seq;
    }

    bool hasSequence() const { return seq && seq->hasSequence(); }

    void mark_subtree();
    bool contains_marked_species();

};

struct ARB_tree_predicate {
    virtual ~ARB_tree_predicate() {}
    virtual bool selects(const ARB_seqtree& tree) const = 0;
};

// ------------------------
//      ARB_countedTree
//      tree that knows its size

class ARB_countedTree : public ARB_seqtree {
protected:
    ~ARB_countedTree() OVERRIDE {}
public:
    explicit ARB_countedTree(ARB_seqtree_root *tree_root_)
        : ARB_seqtree(tree_root_)
    {}
    DEFINE_TREE_ACCESSORS(ARB_seqtree_root, ARB_countedTree);

    // @@@ TODO:
    // - add debug code (checking init_tree() is called exactly once)
    // - init_tree() might be called automatically via announce_tree_constructed()

    virtual void init_tree()              = 0;      /* impl. shall initialize the tree
                                                     * (including some kind of leaf counter)
                                                     * needs to be called manually */

    void compute_tree() OVERRIDE {} // ARB_countedTree always is informed about its subtrees

    size_t relative_position_in(const ARB_countedTree *upgroup) const;
};



#else
#error ARB_Tree.hxx included twice
#endif // ARB_TREE_HXX
