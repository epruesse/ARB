#ifndef ARBDBT_H
#define ARBDBT_H

#ifndef ARBDB_H
#include <arbdb.h>
#endif
#ifndef DOWNCAST_H
#include <downcast.h>
#endif

#define GBT_SPECIES_INDEX_SIZE       10000L
#define GBT_SAI_INDEX_SIZE           1000L

#define GB_GROUP_NAME_MAX 256

#define DEFAULT_BRANCH_LENGTH 0.1

#define GBT_TREE_ELEMENTS(type)                 \
    bool     is_leaf;                           \
    bool     tree_is_one_piece_of_memory;       \
    type    *father, *leftson, *rightson;       \
    GBT_LEN  leftlen, rightlen;                 \
    GBDATA  *gb_node;                           \
    char    *name;                              \
    char    *remark_branch

// remark_branch normally contains some bootstrap value in format 'xx%'
// if you store other info there, please make sure that this info does not
// start with digits!!
// Otherwise the tree export routines will not work correctly!
// --------------------------------------------------------------------------------

#define CLEAR_GBT_TREE_ELEMENTS(tree_obj_ptr)                   \
    (tree_obj_ptr)->is_leaf = false;                            \
    (tree_obj_ptr)->tree_is_one_piece_of_memory = false;        \
    (tree_obj_ptr)->father = 0;                                 \
    (tree_obj_ptr)->leftson = 0;                                \
    (tree_obj_ptr)->rightson = 0;                               \
    (tree_obj_ptr)->leftlen = 0;                                \
    (tree_obj_ptr)->rightlen = 0;                               \
    (tree_obj_ptr)->gb_node = 0;                                \
    (tree_obj_ptr)->name = 0;                                   \
    (tree_obj_ptr)->remark_branch = 0

#define ERROR_CONTAINER_PATH    "tmp/message/pending"

#define REMOTE_BASE              "tmp/remote/"
#define MACRO_TRIGGER_CONTAINER  REMOTE_BASE "trigger"
#define MACRO_TRIGGER_TERMINATED MACRO_TRIGGER_CONTAINER "/terminated"
#define MACRO_TRIGGER_RECORDING  MACRO_TRIGGER_CONTAINER "/recording"
#define MACRO_TRIGGER_ERROR      MACRO_TRIGGER_CONTAINER "/error"
#define MACRO_TRIGGER_TRACKED    MACRO_TRIGGER_CONTAINER "/tracked"

#ifdef FAKE_VTAB_PTR
// if defined, FAKE_VTAB_PTR contains 'char'
typedef FAKE_VTAB_PTR  virtualTable;
#define GBT_VTAB_AND_TREE_ELEMENTS(type)        \
    virtualTable      *dummy_virtual;           \
    GBT_TREE_ELEMENTS(type)
#else
#define GBT_VTAB_AND_TREE_ELEMENTS(type) GBT_TREE_ELEMENTS(type)
#endif

struct GBT_TREE {
    GBT_VTAB_AND_TREE_ELEMENTS(GBT_TREE);
};

enum GBT_TREE_REMOVE_TYPE {
    GBT_REMOVE_MARKED     = 1,
    GBT_REMOVE_NOT_MARKED = 2,
    GBT_REMOVE_DELETED    = 4,

    // please keep AWT_REMOVE_TYPE in sync with GBT_TREE_REMOVE_TYPE
    // see ../SL/AP_TREE/AP_Tree.hxx@sync_GBT_TREE_REMOVE_TYPE_AWT_REMOVE_TYPE
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
