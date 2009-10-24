// =============================================================== //
//                                                                 //
//   File      : ARB_Tree.hxx                                      //
//   Purpose   : Minimal interface between GBT_TREE and C++        //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2009   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef ARB_TREE_HXX
#define ARB_TREE_HXX

#ifndef ARBDBT_H
#include <arbdbt.h>
#endif

#define at_assert(cond) arb_assert(cond)

#if defined(DEBUG)
#if defined(DEVEL_RALF)
#define CHECK_TREE_STRUCTURE
#endif // DEVEL_RALF
#endif // DEBUG

typedef void (*ARB_tree_node_del_cb)(GBDATA *, int *cl_ARB_tree, GB_CB_TYPE);

class ARB_tree;
class AP_filter;
class AP_weights;

class ARB_tree_root : public Noncopyable {
    GBDATA *gb_main;

    AP_filter  *filter;
    AP_weights *weights;

    ARB_tree *rootNode;                             // root node of the tree
    ARB_tree *nodeTemplate;

    // following variables are set, if the tree has been loaded from DB
    char   *tree_name;                              // name of the tree in DB
    GBDATA *gb_tree;                                // tree container in DB

    bool isLinkedToDB;
    bool addDeleteCallbacks;

public:
    ARB_tree_root(GBDATA *gb_main_, const ARB_tree& nodeTempl, bool add_delete_callbacks);

    virtual ~ARB_tree_root();

    virtual void change_root(ARB_tree *old, ARB_tree *newroot);

    void set_filter(const AP_filter *filter_);
    void set_weights(const AP_weights *weights_);

    const AP_filter *get_filter() const { return filter; }
    const AP_weights *get_weights() const { return weights; }

    GBDATA *get_gb_main() const { return gb_main; }
    
    GBDATA *get_gb_tree() const { return gb_tree; } // NULL if no tree loaded (or tree disappeared from DB)
    void tree_deleted_cb(GBDATA *gb_tree_del); // internal

    const char *get_tree_name() const { return tree_name; } // NULL if no tree loaded (or tree disappeared from DB)

    ARB_tree *get_root_node() { return rootNode; }

    virtual GB_ERROR loadFromDB(const char *name) __ATTR__USERESULT;
    virtual GB_ERROR saveToDB() __ATTR__USERESULT;

    virtual GB_ERROR linkToDB(int *zombies, int *duplicates) __ATTR__USERESULT;
    virtual void unlinkFromDB();
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
    size_t linked()   const { return leafs-unlinked;   }
    size_t unmarked() const { return linked()-marked;  }
};

class ARB_tree {
public:
    GBT_TREE_ELEMENTS(ARB_tree);                    // these MUST be the first data members! (see FAKE_VTAB_PTR for more info)
private:
    friend void     ARB_tree_root::change_root(ARB_tree *old, ARB_tree *newroot);
    friend GB_ERROR ARB_tree_root::loadFromDB(const char *name);
    friend GB_ERROR linkToDB(int *zombies, int *duplicates);
    friend void unlinkFromDB();

    ARB_tree_root *tree_root;

    // ------------------
    //      functions
    
private:
    void unlink_son(ARB_tree *son) {
        if (son->is_leftson(this)) leftson = NULL;
        else rightson                      = NULL;
    }

protected:
    void unlink_from_father() {
        if (father) {
            father->unlink_son(this);
            father = NULL;
        }
    }

    virtual void move_gbt_info(GBT_TREE *tree);

    void set_tree_root(ARB_tree_root *new_root);
    ARB_tree_root *get_tree_root() const { return tree_root; }

public:
    ARB_tree(ARB_tree_root *root);
    virtual ~ARB_tree();                            // needed (see FAKE_VTAB_PTR)

    virtual ARB_tree *dup() const = 0;              // create new ARB_tree element from prototype

    void calcTreeInfo(ARB_tree_info& info);

    bool is_inside(const ARB_tree *subtree) const;
    bool is_anchestor_of(const ARB_tree *descendant) const {
        return !is_leaf && descendant != this && descendant->is_inside(this);
    }

    bool is_son(const ARB_tree *of_father) const {
        return father == of_father &&
            (father->leftson == this || father->rightson == this);
    }
    bool is_leftson(const ARB_tree *of_father) const {
        at_assert(is_son(of_father)); // do only call is_leftson() with sons!
        return of_father->leftson == this;
    }

    bool is_root() const { return father == NULL; }
    ARB_tree *get_root() {
        ARB_tree *up              = this;
        while (!up->is_root()) up = up->father;
        return up;
    }

    ARB_tree *get_brother() {
        at_assert(father);
        return is_leftson(father) ? father->rightson : father->leftson;
    }
    const ARB_tree *get_brother() const {
        return const_cast<const ARB_tree*>(const_cast<ARB_tree*>(this)->get_brother());
    }

    GBT_LEN get_branchlength() const {
        at_assert(father); // no father -> no branchlen
        return is_leftson(father) ? father->leftlen : father->rightlen;
    }
    void set_branchlength(GBT_LEN newlen) {
        if (is_leftson(father)) father->leftlen = newlen;
        else father->rightlen                   = newlen;
    }

    GBT_TREE *get_gbt_tree() { return (GBT_TREE*)this; }

#if defined(CHECK_TREE_STRUCTURE)
    void assert_valid() const;
#endif // CHECK_TREE_STRUCTURE

    void mark_subtree();
    bool contains_marked_species();

    // internal
    GB_ERROR add_delete_cb_rec(ARB_tree_node_del_cb cb) __ATTR__USERESULT;
    void remove_delete_cb_rec(ARB_tree_node_del_cb cb);
};

#if defined(CHECK_TREE_STRUCTURE)
#define ASSERT_VALID_TREE(tree) (tree)->assert_valid()
#else
#define ASSERT_VALID_TREE(tree)
#endif // CHECK_TREE_STRUCTURE

#else
#error ARB_Tree.hxx included twice
#endif // ARB_TREE_HXX
