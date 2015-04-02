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

enum GBT_RemarkType { REMARK_NONE, REMARK_BOOTSTRAP, REMARK_OTHER };

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


