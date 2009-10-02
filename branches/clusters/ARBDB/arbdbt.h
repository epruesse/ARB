#ifndef ARBDBT_H
#define ARBDBT_H

#ifndef ARBDB_H
#include <arbdb.h>
#endif

#define GBT_SPECIES_INDEX_SIZE       10000
#define GBT_SAI_INDEX_SIZE           1000
#define GB_COMPRESSION_TAGS_SIZE_MAX 100

#define GB_GROUP_NAME_MAX 256

typedef float GBT_LEN;

#define GBT_TREE_ELEMENTS(type)                     \
    GB_BOOL         is_leaf;                        \
    GB_BOOL         tree_is_one_piece_of_memory;    \
    type            *father,*leftson,*rightson;     \
    GBT_LEN         leftlen,rightlen;               \
    GBDATA          *gb_node;                       \
    char            *name;                          \
    char            *remark_branch

// remark_branch normally contains some bootstrap value in format 'xx%'
// if you store other info there, please make sure that this info does not
// start with digits!!
// Otherwise the tree export routines will not work correctly!
// --------------------------------------------------------------------------------

#define CLEAR_GBT_TREE_ELEMENTS(tree_obj_ptr)           \
(tree_obj_ptr)->is_leaf = GB_FALSE;                     \
(tree_obj_ptr)->tree_is_one_piece_of_memory = GB_FALSE; \
(tree_obj_ptr)->father = 0;                             \
(tree_obj_ptr)->leftson = 0;                            \
(tree_obj_ptr)->rightson = 0;                           \
(tree_obj_ptr)->leftlen = 0;                            \
(tree_obj_ptr)->rightlen = 0;                           \
(tree_obj_ptr)->gb_node = 0;                            \
(tree_obj_ptr)->name = 0;                               \
(tree_obj_ptr)->remark_branch = 0

#define AWAR_ERROR_CONTAINER    "tmp/message/pending"

#ifdef FAKE_VTAB_PTR
/* if defined, FAKE_VTAB_PTR contains 'char' */
typedef FAKE_VTAB_PTR  virtualTable;
#define GBT_VTAB_AND_TREE_ELEMENTS(type)        \
    virtualTable      *dummy_virtual;           \
    GBT_TREE_ELEMENTS(type)
#else
#define GBT_VTAB_AND_TREE_ELEMENTS(type) GBT_TREE_ELEMENTS(type)
#endif

typedef struct gbt_tree_struct {
    GBT_VTAB_AND_TREE_ELEMENTS(struct gbt_tree_struct);
} GBT_TREE;

typedef enum { // bit flags
    GBT_REMOVE_MARKED     = 1,
    GBT_REMOVE_NOT_MARKED = 2,
    GBT_REMOVE_DELETED    = 4,

    // please keep AWT_REMOVE_TYPE in sync with GBT_TREE_REMOVE_TYPE
    // see ../AWT/awt_tree.hxx@sync_GBT_TREE_REMOVE_TYPE_AWT_REMOVE_TYPE

} GBT_TREE_REMOVE_TYPE;

#define GBT_TREE_AWAR_SRT       " = :\n=:*=tree_*1:tree_tree_*=tree_*1"
#define GBT_ALI_AWAR_SRT        " =:\n=:*=ali_*1:ali_ali_*=ali_*1"

#define P_(s) s

#if defined(__GNUG__) || defined(__cplusplus)
extern "C" {
#endif

    typedef GB_ERROR (*species_callback)(GBDATA *gb_species, int *clientdata);

#if defined(__GNUG__) || defined(__cplusplus)
}
#endif

# include <ad_t_prot.h>
# ifdef ADLOCAL_H
#  include <ad_t_lpro.h>
# endif

#undef P_

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
