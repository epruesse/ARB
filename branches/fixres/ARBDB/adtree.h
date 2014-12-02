// ================================================================ //
//                                                                  //
//   File      : adtree.h                                           //
//   Purpose   : tree base class                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2014   //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef ADTREE_H
#define ADTREE_H

#ifndef ARBDBT_H
#include "arbdbt.h"
#endif

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
    friend class RootedTree;

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

inline void destroy(ELIMtree *that) { ELIMtree::destroy(that); }

struct TreeNodeFactory { // @@@ move into RootedTree?
    virtual ~TreeNodeFactory() {}
    virtual GBT_TREE *makeNode() const             = 0;
    virtual void destroyNode(GBT_TREE *node) const = 0;
};

#ifndef ROOTEDTREE_H
#include "RootedTree.h"
#endif

#else
#error adtree.h included twice
#endif // ADTREE_H
