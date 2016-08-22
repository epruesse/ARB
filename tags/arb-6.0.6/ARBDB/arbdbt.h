#ifndef ARBDBT_H
#define ARBDBT_H

#ifndef ARBDB_H
#include <arbdb.h>
#endif
#ifndef DOWNCAST_H
#include <downcast.h>
#endif

#define gb_assert(cond) arb_assert(cond)

#define GBT_SPECIES_INDEX_SIZE       10000L
#define GBT_SAI_INDEX_SIZE           1000L

#define GB_GROUP_NAME_MAX 256

#define DEFAULT_BRANCH_LENGTH 0.1

#define ERROR_CONTAINER_PATH    "tmp/message/pending"

#define REMOTE_BASE              "tmp/remote/"
#define MACRO_TRIGGER_CONTAINER  REMOTE_BASE "trigger"
#define MACRO_TRIGGER_TERMINATED MACRO_TRIGGER_CONTAINER "/terminated"
#define MACRO_TRIGGER_RECORDING  MACRO_TRIGGER_CONTAINER "/recording"
#define MACRO_TRIGGER_ERROR      MACRO_TRIGGER_CONTAINER "/error"
#define MACRO_TRIGGER_TRACKED    MACRO_TRIGGER_CONTAINER "/tracked"

#define DEFINE_SIMPLE_TREE_RELATIVES_ACCESSORS(TreeType)        \
    DEFINE_DOWNCAST_ACCESSORS(TreeType, get_father, father);    \
    DEFINE_DOWNCAST_ACCESSORS(TreeType, get_leftson, leftson);  \
    DEFINE_DOWNCAST_ACCESSORS(TreeType, get_rightson, rightson)

enum GBT_RemarkType { REMARK_NONE, REMARK_BOOTSTRAP, REMARK_OTHER };

struct GBT_TREE : virtual Noncopyable {
    bool      is_leaf;
    GBT_TREE *father, *leftson, *rightson;
    GBT_LEN   leftlen, rightlen;
    GBDATA   *gb_node;
    char     *name;

private:
    char *remark_branch; // remark_branch normally contains some bootstrap value in format 'xx%'
                         // if you store other info there, please make sure that this info does not start with digits!!
                         // Otherwise the tree export routines will not work correctly!

    GBT_LEN& length_ref() { return is_leftson() ? father->leftlen : father->rightlen; }
    const GBT_LEN& length_ref() const { return const_cast<GBT_TREE*>(this)->length_ref(); }

protected:
    GBT_TREE*& self_ref() {
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

public:
    GBT_TREE()
        : is_leaf(false),
          father(NULL), leftson(NULL), rightson(NULL),
          leftlen(0.0), rightlen(0.0),
          gb_node(NULL),
          name(NULL),
          remark_branch(NULL)
    {}
    virtual ~GBT_TREE() {
        delete leftson;  gb_assert(!leftson);
        delete rightson; gb_assert(!rightson);

        unlink_from_father();

        free(name);
        free(remark_branch);
    }

    DEFINE_SIMPLE_TREE_RELATIVES_ACCESSORS(GBT_TREE);

    virtual void announce_tree_constructed() {
        // (has to be) called after tree has been constructed
        gb_assert(!father); // has to be called with root-node!
    }

    bool is_son_of(const GBT_TREE *Father) const {
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

    bool is_inside(const GBT_TREE *subtree) const {
        return this == subtree || (father && get_father()->is_inside(subtree));
    }
    bool is_anchestor_of(const GBT_TREE *descendant) const {
        return !is_leaf && descendant != this && descendant->is_inside(this);
    }
    const GBT_TREE *ancestor_common_with(const GBT_TREE *other) const;
    GBT_TREE *ancestor_common_with(GBT_TREE *other) { return const_cast<GBT_TREE*>(ancestor_common_with(other)); }

    GBT_TREE *fixDeletedSon();

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
    GBT_LEN intree_distance_to(const GBT_TREE *other) const {
        const GBT_TREE *ancestor = ancestor_common_with(other);
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

struct TreeNodeFactory {
    virtual ~TreeNodeFactory() {}
    virtual GBT_TREE *makeNode() const = 0;
};

struct GBT_TREE_NodeFactory : public TreeNodeFactory {
    GBT_TREE *makeNode() const OVERRIDE { return new GBT_TREE; }
};

enum GBT_TreeRemoveType {
    GBT_REMOVE_MARKED   = 1,
    GBT_REMOVE_UNMARKED = 2,
    GBT_REMOVE_ZOMBIES  = 4,

    // please keep AWT_RemoveType in sync with GBT_TreeRemoveType
    // see ../SL/AP_TREE/AP_Tree.hxx@sync_GBT_TreeRemoveType__AWT_RemoveType

    // combined defines:
    GBT_KEEP_MARKED = GBT_REMOVE_UNMARKED|GBT_REMOVE_ZOMBIES,
};

enum GBT_ORDER_MODE {
    GBT_BEHIND, 
    GBT_INFRONTOF, 
};

enum TreeModel { ROOTED = 0, UNROOTED = 1 };

inline CONSTEXPR_RETURN int nodes_2_edges(int nodes) { return nodes-1; }
inline CONSTEXPR_RETURN int edges_2_nodes(int nodes) { return nodes+1; }

inline CONSTEXPR_RETURN int leafs_2_nodes(int leafs, TreeModel model) {
    //! calculate the number of nodes (leaf- plus inner-nodes) in a tree with 'leafs' leafs
    return 2*leafs-1-int(model);
}
inline CONSTEXPR_RETURN int nodes_2_leafs(int nodes, TreeModel model) {
    //! calculate the number of leafs in a tree with 'nodes' nodes
    return (nodes+1+int(model))/2;
}
inline CONSTEXPR_RETURN int leafs_2_edges(int leafs, TreeModel model) {
    //! calculate the number of edges in a tree with 'leafs' leafs
    return nodes_2_edges(leafs_2_nodes(leafs, model));
}
inline CONSTEXPR_RETURN int edges_2_leafs(int edges, TreeModel model) {
    //! calculate the number of leafs in a tree with 'edges' edges
    return nodes_2_leafs(edges_2_nodes(edges), model);
}

#define GBT_TREE_AWAR_SRT       " = :\n=:*=tree_*1:tree_tree_*=tree_*1"
#define GBT_ALI_AWAR_SRT        " =:\n=:*=ali_*1:ali_ali_*=ali_*1"

typedef GB_ERROR (*species_callback)(GBDATA *gb_species, int *clientdata);

#include <ad_t_prot.h>

#define CHANGE_KEY_PATH             "presets/key_data"
#define CHANGE_KEY_PATH_GENES       "presets/gene_key_data"
#define CHANGE_KEY_PATH_EXPERIMENTS "presets/experiment_key_data"

#define CHANGEKEY        "key"
#define CHANGEKEY_NAME   "key_name"
#define CHANGEKEY_TYPE   "key_type"
#define CHANGEKEY_HIDDEN "key_hidden"


#else
#error arbdbt.h included twice
#endif


