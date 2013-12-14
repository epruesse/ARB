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

struct GBT_TREE : virtual Noncopyable {
    bool      is_leaf;
    GBT_TREE *father, *leftson, *rightson;
    GBT_LEN   leftlen, rightlen;
    GBDATA   *gb_node;
    char     *name;

    char *remark_branch; // remark_branch normally contains some bootstrap value in format 'xx%'
                         // if you store other info there, please make sure that this info does not start with digits!!
                         // Otherwise the tree export routines will not work correctly!

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
