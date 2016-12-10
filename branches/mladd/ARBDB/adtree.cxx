// =============================================================== //
//                                                                 //
//   File      : adtree.cxx                                        //
//   Purpose   : tree functions                                    //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <arb_progress.h>
#include "gb_local.h"
#include <arb_strarray.h>
#include <set>
#include <limits.h>
#include <arb_global_defs.h>
#include <arb_strbuf.h>
#include <arb_diff.h>
#include <arb_defs.h>
#include <arb_match.h>
#include "TreeNode.h"

#define GBT_PUT_DATA 1
#define GBT_GET_SIZE 0

GBDATA *GBT_get_tree_data(GBDATA *gb_main) {
    return GBT_find_or_create(gb_main, "tree_data", 7);
}

// ----------------------
//      remove leafs

TreeNode *GBT_remove_leafs(TreeNode *tree, GBT_TreeRemoveType mode, const GB_HASH *species_hash, int *removed, int *groups_removed) { // @@@ add tests for GBT_remove_leafs()
    /*! Remove leafs from given 'tree'.
     * @param tree tree from which species will be removed
     * @param mode defines what to remove
     * @param species_hash hash translation from leaf-name to species-dbnode (not needed if tree is linked; see GBT_link_tree)
     * @param removed will be incremented for each removed leaf (if !NULL)
     * @param groups_removed will be incremented for each removed group (if !NULL)
     * @return new root node
     *
     * if 'species_hash' is not provided and tree is not linked,
     * the function will silently act strange:
     * - GBT_REMOVE_MARKED and GBT_REMOVE_UNMARKED will remove any leaf
     * - GBT_REMOVE_ZOMBIES and GBT_KEEP_MARKED will remove all leafs
     */

    if (tree->is_leaf) {
        if (tree->name) {
            bool    deleteSelf = false;
            GBDATA *gb_node;

            if (species_hash) {
                gb_node = (GBDATA*)GBS_read_hash(species_hash, tree->name);
                gb_assert(tree->gb_node == 0); // don't call linked tree with 'species_hash'!
            }
            else gb_node = tree->gb_node;

            if (gb_node) {
                if (mode & (GBT_REMOVE_MARKED|GBT_REMOVE_UNMARKED)) {
                    long flag = GB_read_flag(gb_node);
                    deleteSelf = (flag && (mode&GBT_REMOVE_MARKED)) || (!flag && (mode&GBT_REMOVE_UNMARKED));
                }
            }
            else { // zombie
                if (mode & GBT_REMOVE_ZOMBIES) deleteSelf = true;
            }

            if (deleteSelf) {
                gb_assert(!tree->is_root_node());

                TreeRoot *troot = tree->get_tree_root();
                tree->forget_origin();
                destroy(tree, troot);

                tree = NULL;
                if (removed) (*removed)++;
            }
        }
    }
    else {
        tree->leftson  = GBT_remove_leafs(tree->get_leftson(), mode, species_hash, removed, groups_removed);
        tree->rightson = GBT_remove_leafs(tree->get_rightson(), mode, species_hash, removed, groups_removed);

        if (tree->leftson) {
            if (!tree->rightson) { // right son deleted
                tree = tree->fixDeletedSon();
            }
            // otherwise no son deleted
        }
        else if (tree->rightson) { // left son deleted
            tree = tree->fixDeletedSon();
        }
        else {                  // everything deleted -> delete self
            if (tree->name && groups_removed) (*groups_removed)++;

            TreeRoot *troot  = tree->get_tree_root();
            if (!tree->is_root_node()) tree->forget_origin();
            tree->forget_relatives();
            destroy(tree, troot);

            tree = NULL;
        }
    }

    return tree;
}

// ---------------------
//      trees order

inline int get_tree_idx(GBDATA *gb_tree) {
    GBDATA *gb_order = GB_entry(gb_tree, "order");
    int     idx      = 0;
    if (gb_order) {
        idx = GB_read_int(gb_order);
        gb_assert(idx>0); // invalid index
    }
    return idx;
}

inline int get_max_tree_idx(GBDATA *gb_treedata) {
    int max_idx = 0;
    for (GBDATA *gb_tree = GB_child(gb_treedata); gb_tree; gb_tree = GB_nextChild(gb_tree)) {
        int idx = get_tree_idx(gb_tree);
        if (idx>max_idx) max_idx = idx;
    }
    return max_idx;
}

inline GBDATA *get_tree_with_idx(GBDATA *gb_treedata, int at_idx) {
    GBDATA *gb_found = NULL;
    for (GBDATA *gb_tree = GB_child(gb_treedata); gb_tree && !gb_found; gb_tree = GB_nextChild(gb_tree)) {
        int idx = get_tree_idx(gb_tree);
        if (idx == at_idx) {
            gb_found = gb_tree;
        }
    }
    return gb_found;
}

inline GBDATA *get_tree_infrontof_idx(GBDATA *gb_treedata, int infrontof_idx) {
    GBDATA *gb_infrontof = NULL;
    if (infrontof_idx) {
        int best_idx = 0;
        for (GBDATA *gb_tree = GB_child(gb_treedata); gb_tree; gb_tree = GB_nextChild(gb_tree)) {
            int idx = get_tree_idx(gb_tree);
            gb_assert(idx);
            if (idx>best_idx && idx<infrontof_idx) {
                best_idx     = idx;
                gb_infrontof = gb_tree;
            }
        }
    }
    return gb_infrontof;
}

inline GBDATA *get_tree_behind_idx(GBDATA *gb_treedata, int behind_idx) {
    GBDATA *gb_behind = NULL;
    if (behind_idx) {
        int best_idx = INT_MAX;
        for (GBDATA *gb_tree = GB_child(gb_treedata); gb_tree; gb_tree = GB_nextChild(gb_tree)) {
            int idx = get_tree_idx(gb_tree);
            gb_assert(idx);
            if (idx>behind_idx && idx<best_idx) {
                best_idx  = idx;
                gb_behind = gb_tree;
            }
        }
    }
    return gb_behind;
}

inline GB_ERROR set_tree_idx(GBDATA *gb_tree, int idx) {
    GB_ERROR  error    = NULL;
    GBDATA   *gb_order = GB_entry(gb_tree, "order");
    if (!gb_order) {
        gb_order = GB_create(gb_tree, "order", GB_INT);
        if (!gb_order) error = GB_await_error();
    }
    if (!error) error = GB_write_int(gb_order, idx);
    return error;
}

static GB_ERROR reserve_tree_idx(GBDATA *gb_treedata, int idx) {
    GB_ERROR  error   = NULL;
    GBDATA   *gb_tree = get_tree_with_idx(gb_treedata, idx);
    if (gb_tree) {
        error = reserve_tree_idx(gb_treedata, idx+1);
        if (!error) error = set_tree_idx(gb_tree, idx+1);
    }
    return error;
}

static void ensure_trees_have_order(GBDATA *gb_treedata) {
    GBDATA *gb_main = GB_get_father(gb_treedata);

    gb_assert(GB_get_root(gb_main)       == gb_main);
    gb_assert(GBT_get_tree_data(gb_main) == gb_treedata);

    GB_ERROR  error              = NULL;
    GBDATA   *gb_tree_order_flag = GB_search(gb_main, "/tmp/trees_have_order", GB_INT);

    if (!gb_tree_order_flag) error = GB_await_error();
    else {
        if (GB_read_int(gb_tree_order_flag) == 0) { // not checked yet
            int max_idx = get_max_tree_idx(gb_treedata);
            for (GBDATA *gb_tree = GB_child(gb_treedata); gb_tree && !error; gb_tree = GB_nextChild(gb_tree)) {
                if (!get_tree_idx(gb_tree)) {
                    error = set_tree_idx(gb_tree, ++max_idx);
                }
            }
            if (!error) error = GB_write_int(gb_tree_order_flag, 1);
        }
    }
    if (error) GBK_terminatef("failed to order trees (Reason: %s)", error);
}

static void tree_set_default_order(GBDATA *gb_tree) {
    // if 'gb_tree' has no order yet, move it to the bottom (as done previously)
    if (!get_tree_idx(gb_tree)) {
        set_tree_idx(gb_tree, get_max_tree_idx(GB_get_father(gb_tree))+1);
    }
}

// -----------------------------
//      tree write functions

GB_ERROR GBT_write_group_name(GBDATA *gb_group_name, const char *new_group_name) {
    GB_ERROR error = 0;
    size_t   len   = strlen(new_group_name);

    if (len >= GB_GROUP_NAME_MAX) {
        error = GBS_global_string("Group name '%s' too long (max %i characters)", new_group_name, GB_GROUP_NAME_MAX);
    }
    else {
        error = GB_write_string(gb_group_name, new_group_name);
    }
    return error;
}

static GB_ERROR gbt_write_tree_nodes(GBDATA *gb_tree, TreeNode *node, long *startid) {
    // increments '*startid' for each inner node (not for leafs)

    GB_ERROR error = NULL;

    if (!node->is_leaf) {
        bool node_is_used = false;

        if (node->name && node->name[0]) {
            if (!node->gb_node) {
                node->gb_node = GB_create_container(gb_tree, "node");
                if (!node->gb_node) error = GB_await_error();
            }
            if (!error) {
                GBDATA *gb_name     = GB_search(node->gb_node, "group_name", GB_STRING);
                if (!gb_name) error = GB_await_error();
                else    error       = GBT_write_group_name(gb_name, node->name);

                node_is_used = true; // wrote groupname -> node is used
            }
        }

        if (node->gb_node && !error) {
            if (!node_is_used) {
                GBDATA *gb_nonid = GB_child(node->gb_node);
                while (gb_nonid && strcmp("id", GB_read_key_pntr(gb_nonid)) == 0) {
                    gb_nonid = GB_nextChild(gb_nonid);
                }
                if (gb_nonid) node_is_used = true; // found child that is not "id" -> node is used
            }

            if (node_is_used) { // set id for used nodes
                error = GBT_write_int(node->gb_node, "id", *startid);
                if (!error) GB_clear_user_flag(node->gb_node, GB_USERFLAG_GHOSTNODE); // mark node as "used"
            }
            else {          // delete unused nodes
                error = GB_delete(node->gb_node);
                if (!error) node->gb_node = 0;
            }
        }

        (*startid)++;
        if (!error) error = gbt_write_tree_nodes(gb_tree, node->get_leftson(), startid);
        if (!error) error = gbt_write_tree_nodes(gb_tree, node->get_rightson(), startid);
    }
    return error;
}

static char *gbt_write_tree_rek_new(const TreeNode *node, char *dest, long mode) {
    {
        const char *c1 = node->get_remark();
        if (c1) {
            if (mode == GBT_PUT_DATA) {
                int c;
                *(dest++) = 'R';
                while ((c = *(c1++))) {
                    if (c == 1) continue;
                    *(dest++) = c;
                }
                *(dest++) = 1;
            }
            else {
                dest += strlen(c1) + 2;
            }
        }
    }
    if (node->is_leaf) {
        if (mode == GBT_PUT_DATA) {
            *(dest++) = 'L';
            if (node->name) strcpy(dest, node->name);

            char *c1;
            while ((c1 = (char *)strchr(dest, 1))) {
                *c1 = 2;
            }
            dest      += strlen(dest);
            *(dest++)  = 1;
            
            return dest;
        }
        else {
            if (node->name) return dest+1+strlen(node->name)+1; // N name term
            return dest+1+1;
        }
    }
    else {
        char buffer[40];
        sprintf(buffer, "%g,%g;", node->leftlen, node->rightlen);
        if (mode == GBT_PUT_DATA) {
            *(dest++) = 'N';
            strcpy(dest, buffer);
            dest += strlen(buffer);
        }
        else {
            dest += strlen(buffer)+1;
        }
        dest = gbt_write_tree_rek_new(node->get_leftson(),  dest, mode);
        dest = gbt_write_tree_rek_new(node->get_rightson(), dest, mode);
        return dest;
    }
}

static GB_ERROR gbt_write_tree(GBDATA *gb_main, GBDATA *gb_tree, const char *tree_name, TreeNode *tree) {
    /*! writes a tree to the database.
     *
     * If tree is loaded by function GBT_read_tree(..) then 'tree_name' should be NULL
     * else 'gb_tree' should be set to NULL
     *
     * To copy a tree call GB_copy(dest,source);
     * or set recursively all tree->gb_node variables to zero (that unlinks the tree),
     */

    GB_ERROR error = 0;

    if (tree) {
        if (tree_name) {
            if (gb_tree) error = GBS_global_string("can't change name of existing tree (to '%s')", tree_name);
            else {
                error = GBT_check_tree_name(tree_name);
                if (!error) {
                    GBDATA *gb_tree_data = GBT_get_tree_data(gb_main);
                    gb_tree              = GB_search(gb_tree_data, tree_name, GB_CREATE_CONTAINER);

                    if (!gb_tree) error = GB_await_error();
                }
            }
        }
        else {
            if (!gb_tree) error = "No tree name given";
        }

        gb_assert(gb_tree || error);

        if (!error) {
            // mark all old style tree data for deletion
            GBDATA *gb_node;
            for (gb_node = GB_entry(gb_tree, "node"); gb_node; gb_node = GB_nextEntry(gb_node)) {
                GB_raise_user_flag(gb_node, GB_USERFLAG_GHOSTNODE); // mark as "possibly unused"
            }

            // build tree-string and save to DB
            {
                char *t_size = gbt_write_tree_rek_new(tree, 0, GBT_GET_SIZE); // calc size of tree-string
                char *ctree  = ARB_calloc<char>(size_t(t_size+1));            // allocate buffer for tree-string

                t_size = gbt_write_tree_rek_new(tree, ctree, GBT_PUT_DATA); // write into buffer
                *(t_size) = 0;

                bool was_allowed = GB_allow_compression(gb_main, false);
                error            = GBT_write_string(gb_tree, "tree", ctree);
                GB_allow_compression(gb_main, was_allowed);
                free(ctree);
            }
        }

        if (!error) {
            // save nodes to DB
            long size         = 0;
            error             = gbt_write_tree_nodes(gb_tree, tree, &size); // reports number of nodes in 'size'
            if (!error) error = GBT_write_int(gb_tree, "nnodes", size);

            if (!error) {
                if (!GB_entry(gb_tree, "keep_ghostnodes")) { // see ../PARSIMONY/PARS_main.cxx@keep_ghostnodes
                    GBDATA *gb_node;
                    GBDATA *gb_node_next;

                    for (gb_node = GB_entry(gb_tree, "node"); // delete all ghost nodes
                         gb_node && !error;
                         gb_node = gb_node_next)
                    {
                        GBDATA *gbd = GB_entry(gb_node, "id");
                        gb_node_next = GB_nextEntry(gb_node);
                        if (!gbd || GB_user_flag(gb_node, GB_USERFLAG_GHOSTNODE)) error = GB_delete(gb_node);
                    }
                }
            }
        }

        if (!error) tree_set_default_order(gb_tree);
    }

    return error;
}

GB_ERROR GBT_write_tree(GBDATA *gb_main, const char *tree_name, TreeNode *tree) {
    return gbt_write_tree(gb_main, NULL, tree_name, tree);
}
GB_ERROR GBT_overwrite_tree(GBDATA *gb_tree, TreeNode *tree) {
    return gbt_write_tree(GB_get_root(gb_tree), gb_tree, NULL, tree);
}

static GB_ERROR write_tree_remark(GBDATA *gb_tree, const char *remark) {
    return GBT_write_string(gb_tree, "remark", remark);
}
GB_ERROR GBT_write_tree_remark(GBDATA *gb_main, const char *tree_name, const char *remark) {
    return write_tree_remark(GBT_find_tree(gb_main, tree_name), remark);
}

GB_ERROR GBT_log_to_tree_remark(GBDATA *gb_tree, const char *log_entry) { // @@@ add bool-flag 'stamp' and forward it to GBS_log_action_to
    GB_ERROR    error      = NULL;
    const char *old_remark = GBT_read_char_pntr(gb_tree, "remark");
    if (!old_remark && GB_have_error()) {
        error = GB_await_error();
    }
    else {
        char *new_remark = GBS_log_action_to(old_remark, log_entry, true);
        error            = write_tree_remark(gb_tree, new_remark);
        free(new_remark);
    }
    return error;
}
GB_ERROR GBT_log_to_tree_remark(GBDATA *gb_main, const char *tree_name, const char *log_entry) {
    return GBT_log_to_tree_remark(GBT_find_tree(gb_main, tree_name), log_entry);
}

GB_ERROR GBT_write_tree_with_remark(GBDATA *gb_main, const char *tree_name, TreeNode *tree, const char *remark) {
    GB_ERROR error              = GBT_write_tree(gb_main, tree_name, tree);
    if (!error && remark) error = GBT_write_tree_remark(gb_main, tree_name, remark);
    return error;
}

// ----------------------------
//      tree read functions

static TreeNode *gbt_read_tree_rek(char **data, long *startid, GBDATA **gb_tree_nodes, const TreeRoot *troot, int size_of_tree, GB_ERROR& error) {
    TreeNode *node = NULL;
    if (!error) {
        node = troot->makeNode();

        char  c = *((*data)++);
        char *p1;

        if (c=='R') {
            p1 = strchr(*data, 1);
            *(p1++) = 0;
            node->set_remark(*data);
            c = *(p1++);
            *data = p1;
        }


        if (c=='N') {
            p1 = (char *)strchr(*data, ',');
            *(p1++) = 0;
            node->leftlen = GB_atof(*data);
            *data = p1;
            p1 = (char *)strchr(*data, ';');
            *(p1++) = 0;
            node->rightlen = GB_atof(*data);
            *data = p1;
            if ((*startid < size_of_tree) && (node->gb_node = gb_tree_nodes[*startid])) {
                GBDATA *gb_group_name = GB_entry(node->gb_node, "group_name");
                if (gb_group_name) {
                    node->name = GB_read_string(gb_group_name);
                    if (!node->name || !node->name[0]) {
                        char   *auto_rename = ARB_strdup("<missing groupname>");
                        GBDATA *gb_main     = GB_get_root(gb_group_name);

                        const char *warn;
                        if (!node->name) {
                            warn = GBS_global_string("Unreadable 'group_name' detected (Reason: %s)", GB_await_error());
                        }
                        else {
                            warn = "Empty groupname detected";
                        }
                        warn = GBS_global_string("%s\nGroup has been named '%s'", warn, auto_rename);
                        GBT_message(gb_main, warn);

                        GB_ERROR rename_error = GB_write_string(gb_group_name, auto_rename);
                        if (rename_error) {
                            GBT_message(gb_main, GBS_global_string("Failed to name group (Reason: %s)", rename_error));
                        }
                        node->name = auto_rename;
                    }
                }
            }
            (*startid)++;
            node->leftson = gbt_read_tree_rek(data, startid, gb_tree_nodes, troot, size_of_tree, error);
            if (!node->leftson) freenull(node);
            else {
                node->rightson = gbt_read_tree_rek(data, startid, gb_tree_nodes, troot, size_of_tree, error);
                if (!node->rightson) {
                    freenull(node->leftson);
                    freenull(node);
                }
                else {
                    node->leftson->father  = node;
                    node->rightson->father = node;
                }
            }
        }
        else if (c=='L') {
            node->is_leaf = true;
            p1            = (char *)strchr(*data, 1);

            gb_assert(p1);
            gb_assert(p1[0] == 1);

            *p1        = 0;
            node->name = ARB_strdup(*data);
            *data      = p1+1;
        }
        else {
            if (!c) {
                error = "Unexpected end of tree definition.";
            }
            else {
                error = GBS_global_string("Can't interpret tree definition (expected 'N' or 'L' - not '%c')", c);
            }
            freenull(node);
        }
    }
    gb_assert(contradicted(node, error));
    return node;
}


static TreeNode *read_tree_and_size_internal(GBDATA *gb_tree, GBDATA *gb_ctree, const TreeRoot *troot, int node_count, GB_ERROR& error) {
    GBDATA   **gb_tree_nodes;
    TreeNode  *node = 0;

    ARB_calloc(gb_tree_nodes, node_count);
    if (gb_tree) {
        GBDATA *gb_node;

        for (gb_node = GB_entry(gb_tree, "node"); gb_node && !error; gb_node = GB_nextEntry(gb_node)) {
            long    i;
            GBDATA *gbd = GB_entry(gb_node, "id");
            if (!gbd) continue;

            i = GB_read_int(gbd);
            if (i<0 || i >= node_count) {
                error = "An inner node of the tree is corrupt";
            }
            else {
                gb_tree_nodes[i] = gb_node;
            }
        }
    }
    if (!error) {
        char * const treeString = GB_read_string(gb_ctree);
        if (!treeString) {
            error = GB_await_error();
        }
        else {
            char *ts = treeString;
            long  id = 0;
            node     = gbt_read_tree_rek(&ts, &id, gb_tree_nodes, troot, node_count, error);
        }
        free(treeString);
    }

    free(gb_tree_nodes);

    gb_assert(contradicted(node, error));
    return node;
}

TreeNode *GBT_read_tree_and_size(GBDATA *gb_main, const char *tree_name, TreeRoot *troot, int *tree_size) {
    /*! Loads a tree from DB into any user defined structure.
     *
     * @param gb_main DB root node
     * @param tree_name is the name of the tree in the db
     * @param nodeFactory makes the tree-node instances
     * @param tree_size if != NULL -> gets set to "size of tree" (aka number of leafs minus 1)
     *
     * @return
     * - NULL if any error occurs (which is exported then)
     * - root of loaded tree (dynamic type depends on 'nodeFactory')
     */

    GB_ERROR error = 0;

    if (!tree_name) {
        error = "no treename given";
    }
    else {
        error = GBT_check_tree_name(tree_name);
        if (!error) {
            GBDATA *gb_tree = GBT_find_tree(gb_main, tree_name);

            if (!gb_tree) {
                error = "tree not found";
            }
            else {
                GBDATA *gb_nnodes = GB_entry(gb_tree, "nnodes");
                if (!gb_nnodes) {
                    error = "tree is empty";
                }
                else {
                    long size = GB_read_int(gb_nnodes);
                    if (!size) {
                        error = "has no nodes";
                    }
                    else {
                        GBDATA *gb_ctree = GB_search(gb_tree, "tree", GB_FIND);
                        if (!gb_ctree) {
                            error = "old unsupported tree format";
                        }
                        else { // "new" style tree
                            TreeNode *tree = read_tree_and_size_internal(gb_tree, gb_ctree, troot, size, error);
                            if (!error) {
                                gb_assert(tree);
                                if (tree_size) *tree_size = size; // return size of tree (=leafs-1)
                                tree->announce_tree_constructed();
                                return tree;
                            }

                            gb_assert(!tree);
                        }
                    }
                }
            }
        }
    }

    gb_assert(error);
    GB_export_errorf("Failed to read tree '%s' (Reason: %s)", tree_name, error);
    troot->delete_by_node();
    return NULL;
}

TreeNode *GBT_read_tree(GBDATA *gb_main, const char *tree_name, TreeRoot *troot) {
    //! @see GBT_read_tree_and_size()
    return GBT_read_tree_and_size(gb_main, tree_name, troot, 0);
}

size_t GBT_count_leafs(const TreeNode *tree) {
    if (tree->is_leaf) {
        return 1;
    }
    return GBT_count_leafs(tree->get_leftson()) + GBT_count_leafs(tree->get_rightson());
}

static GB_ERROR gbt_invalid_because(const TreeNode *tree, const char *reason) {
    return GBS_global_string("((TreeNode*)0x%p) %s", tree, reason);
}

inline bool has_son(const TreeNode *father, const TreeNode *son) {
    return !father->is_leaf && (father->leftson == son || father->rightson == son);
}

static GB_ERROR gbt_is_invalid(bool is_root, const TreeNode *tree) {
    if (tree->father) {
        if (!has_son(tree->get_father(), tree)) return gbt_invalid_because(tree, "is not son of its father");
    }
    else {
        if (!is_root) return gbt_invalid_because(tree, "has no father (but isn't root)");
    }

    GB_ERROR error = NULL;
    if (tree->is_leaf) {
        if (tree->leftson) return gbt_invalid_because(tree, "is leaf, but has leftson");
        if (tree->rightson) return gbt_invalid_because(tree, "is leaf, but has rightson");
    }
    else {
        if (!tree->leftson) return gbt_invalid_because(tree, "is inner node, but has no leftson");
        if (!tree->rightson) return gbt_invalid_because(tree, "is inner node, but has no rightson");

        error             = gbt_is_invalid(false, tree->get_leftson());
        if (!error) error = gbt_is_invalid(false, tree->get_rightson());
    }
    return error;
}

GB_ERROR GBT_is_invalid(const TreeNode *tree) {
    if (tree->father) return gbt_invalid_because(tree, "is expected to be the root-node, but has father");
    if (tree->is_leaf) return gbt_invalid_because(tree, "is expected to be the root-node, but is a leaf (tree too small)");
    return gbt_is_invalid(true, tree);
}

// -------------------------------------------
//      link the tree tips to the database

struct link_tree_data {
    GB_HASH      *species_hash;
    GB_HASH      *seen_species;                     // used to count duplicates
    arb_progress *progress;
    int           zombies;                          // counts zombies
    int           duplicates;                       // counts duplicates
};

static GB_ERROR gbt_link_tree_to_hash_rek(TreeNode *tree, link_tree_data *ltd) {
    GB_ERROR error = 0;
    if (tree->is_leaf) {
        tree->gb_node = 0;
        if (tree->name) {
            GBDATA *gbd = (GBDATA*)GBS_read_hash(ltd->species_hash, tree->name);
            if (gbd) tree->gb_node = gbd;
            else ltd->zombies++;

            if (ltd->seen_species) {
                if (GBS_read_hash(ltd->seen_species, tree->name)) ltd->duplicates++;
                else GBS_write_hash(ltd->seen_species, tree->name, 1);
            }
        }

        if (ltd->progress) ++(*ltd->progress);
    }
    else {
        error             = gbt_link_tree_to_hash_rek(tree->get_leftson(), ltd);
        if (!error) error = gbt_link_tree_to_hash_rek(tree->get_rightson(), ltd);
    }
    return error;
}

static GB_ERROR GBT_link_tree_using_species_hash(TreeNode *tree, bool show_status, GB_HASH *species_hash, int *zombies, int *duplicates) {
    GB_ERROR       error;
    link_tree_data ltd;
    long           leafs = 0;

    if (duplicates || show_status) {
        leafs = GBT_count_leafs(tree);
    }

    ltd.species_hash = species_hash;
    ltd.seen_species = leafs ? GBS_create_hash(leafs, GB_IGNORE_CASE) : 0;
    ltd.zombies      = 0;
    ltd.duplicates   = 0;

    if (show_status) {
        ltd.progress = new arb_progress("Relinking tree to database", leafs);
    }
    else {
        ltd.progress = NULL;
    }

    error = gbt_link_tree_to_hash_rek(tree, &ltd);
    if (ltd.seen_species) GBS_free_hash(ltd.seen_species);

    if (zombies) *zombies       = ltd.zombies;
    if (duplicates) *duplicates = ltd.duplicates;

    delete ltd.progress;

    return error;
}

GB_ERROR GBT_link_tree(TreeNode *tree, GBDATA *gb_main, bool show_status, int *zombies, int *duplicates) {
    /*! Link a given tree to the database. That means that for all tips the member
     * 'gb_node' is set to the database container holding the species data.
     *
     * @param tree which will be linked to DB
     * @param gb_main DB root node
     * @param show_status show a progress indicator?
     * @param zombies if != NULL -> set to number of zombies (aka non-existing species) in tree
     * @param duplicates if != NULL -> set to number of duplicated species in tree
     *
     * @return error on failure
     *
     * @see GBT_unlink_tree()
     */

    GB_HASH  *species_hash = GBT_create_species_hash(gb_main);
    GB_ERROR  error        = GBT_link_tree_using_species_hash(tree, show_status, species_hash, zombies, duplicates);

    GBS_free_hash(species_hash);

    return error;
}

void TreeNode::unlink_from_DB() {
    /*! Unlink tree from the database.
     * @see GBT_link_tree()
     */
    gb_node = 0;
    if (!is_leaf) {
        get_leftson()->unlink_from_DB();
        get_rightson()->unlink_from_DB();
    }
}
void GBT_unlink_tree(TreeNode *tree) {
    tree->unlink_from_DB();
}

// ----------------------
//      search trees

GBDATA *GBT_find_tree(GBDATA *gb_main, const char *tree_name) {
    /*! @return
     * - DB tree container associated with tree_name
     * - NULL if no such tree exists
     */
    return GB_entry(GBT_get_tree_data(gb_main), tree_name);
}

inline bool is_tree(GBDATA *gb_tree) {
    if (!gb_tree) return false;
    GBDATA *gb_tree_data = GB_get_father(gb_tree);
    return gb_tree_data && GB_has_key(gb_tree_data, "tree_data");
}

inline GBDATA *get_first_tree(GBDATA *gb_main) {
    return GB_child(GBT_get_tree_data(gb_main));
}

inline GBDATA *get_next_tree(GBDATA *gb_tree) {
    if (!gb_tree) return NULL;
    gb_assert(is_tree(gb_tree));
    return GB_nextChild(gb_tree);
}

GBDATA *GBT_find_largest_tree(GBDATA *gb_main) {
    long    maxnodes   = 0;
    GBDATA *gb_largest = NULL;

    for (GBDATA *gb_tree = get_first_tree(gb_main); gb_tree; gb_tree = get_next_tree(gb_tree)) {
        long *nnodes = GBT_read_int(gb_tree, "nnodes");
        if (nnodes && *nnodes>maxnodes) {
            gb_largest = gb_tree;
            maxnodes   = *nnodes;
        }
    }
    return gb_largest;
}

GBDATA *GBT_tree_infrontof(GBDATA *gb_tree) {
    GBDATA *gb_treedata = GB_get_father(gb_tree);
    ensure_trees_have_order(gb_treedata);
    return get_tree_infrontof_idx(gb_treedata, get_tree_idx(gb_tree));
}
GBDATA *GBT_tree_behind(GBDATA *gb_tree) {
    GBDATA *gb_treedata = GB_get_father(gb_tree);
    ensure_trees_have_order(gb_treedata);
    return get_tree_behind_idx(gb_treedata, get_tree_idx(gb_tree));
}

GBDATA *GBT_find_top_tree(GBDATA *gb_main) {
    GBDATA *gb_treedata = GBT_get_tree_data(gb_main);
    ensure_trees_have_order(gb_treedata);

    GBDATA *gb_top = get_tree_with_idx(gb_treedata, 1);
    if (!gb_top) gb_top = get_tree_behind_idx(gb_treedata, 1);
    return gb_top;
}
GBDATA *GBT_find_bottom_tree(GBDATA *gb_main) {
    GBDATA *gb_treedata = GBT_get_tree_data(gb_main);
    ensure_trees_have_order(gb_treedata);
    return get_tree_infrontof_idx(gb_treedata, INT_MAX);
}

const char *GBT_existing_tree(GBDATA *gb_main, const char *tree_name) {
    // search for a specify existing tree (and fallback to any existing)
    GBDATA *gb_tree       = GBT_find_tree(gb_main, tree_name);
    if (!gb_tree) gb_tree = get_first_tree(gb_main);
    return GBT_get_tree_name(gb_tree);
}

GBDATA *GBT_find_next_tree(GBDATA *gb_tree) {
    GBDATA *gb_other = NULL;
    if (gb_tree) {
        gb_other = GBT_tree_behind(gb_tree);
        if (!gb_other) {
            gb_other = GBT_find_top_tree(GB_get_root(gb_tree));
            if (gb_other == gb_tree) gb_other = NULL;
        }
    }
    gb_assert(gb_other != gb_tree);
    return gb_other;
}

// --------------------
//      tree names

const char *GBT_get_tree_name(GBDATA *gb_tree) {
    if (!gb_tree) return NULL;
    gb_assert(is_tree(gb_tree));
    return GB_read_key_pntr(gb_tree);
}

GB_ERROR GBT_check_tree_name(const char *tree_name) {
    GB_ERROR error = GB_check_key(tree_name);
    if (!error) {
        if (strncmp(tree_name, "tree_", 5) != 0) {
            error = "has to start with 'tree_'";
        }
    }
    if (error) {
        error = GBS_global_string("not a valid treename '%s' (Reason: %s)", tree_name, error);
    }
    return error;
}

const char *GBT_name_of_largest_tree(GBDATA *gb_main) {
    return GBT_get_tree_name(GBT_find_largest_tree(gb_main));
}

const char *GBT_name_of_bottom_tree(GBDATA *gb_main) {
    return GBT_get_tree_name(GBT_find_bottom_tree(gb_main));
}

// -------------------
//      tree info

const char *GBT_tree_info_string(GBDATA *gb_main, const char *tree_name, int maxTreeNameLen) {
    // maxTreeNameLen shall be the max len of the longest tree name (or -1 -> do not format)

    const char *result  = 0;
    GBDATA     *gb_tree = GBT_find_tree(gb_main, tree_name);

    if (!gb_tree) {
        GB_export_errorf("tree '%s' not found", tree_name);
    }
    else {
        GBDATA *gb_nnodes = GB_entry(gb_tree, "nnodes");
        if (!gb_nnodes) {
            GB_export_errorf("nnodes not found in tree '%s'", tree_name);
        }
        else {
            const char *sizeInfo = GBS_global_string("(%li:%i)", GB_read_int(gb_nnodes)+1, GB_read_security_write(gb_tree));
            GBDATA     *gb_rem   = GB_entry(gb_tree, "remark");
            int         len;

            if (maxTreeNameLen == -1) {
                result = GBS_global_string("%s %11s", tree_name, sizeInfo);
                len    = strlen(tree_name);
            }
            else {
                result = GBS_global_string("%-*s %11s", maxTreeNameLen, tree_name, sizeInfo);
                len    = maxTreeNameLen;
            }
            if (gb_rem) {
                const char *remark    = GB_read_char_pntr(gb_rem);
                const int   remarkLen = 800;
                char       *res2      = GB_give_other_buffer(remark, len+1+11+2+remarkLen+1);

                strcpy(res2, result);
                strcat(res2, "  ");
                strncat(res2, remark, remarkLen);

                result = res2;
            }
        }
    }
    return result;
}

long GBT_size_of_tree(GBDATA *gb_main, const char *tree_name) {
    // return the number of inner nodes in binary tree (or -1 if unknown)
    // Note:
    //        leafs                        = size + 1
    //        inner nodes in unrooted tree = size - 1

    long    nnodes  = -1;
    GBDATA *gb_tree = GBT_find_tree(gb_main, tree_name);
    if (gb_tree) {
        GBDATA *gb_nnodes = GB_entry(gb_tree, "nnodes");
        if (gb_nnodes) {
            nnodes = GB_read_int(gb_nnodes);
        }
    }
    return nnodes;
}


struct indexed_name {
    int         idx;
    const char *name;

    bool operator<(const indexed_name& other) const { return idx < other.idx; }
};

void GBT_get_tree_names(ConstStrArray& names, GBDATA *gb_main, bool sorted) {
    // stores tree names in 'names'

    GBDATA *gb_treedata = GBT_get_tree_data(gb_main);
    ensure_trees_have_order(gb_treedata);

    long tree_count = GB_number_of_subentries(gb_treedata);

    names.reserve(tree_count);
    typedef std::set<indexed_name> ordered_trees;
    ordered_trees trees;

    {
        int t     = 0;
        int count = 0;
        for (GBDATA *gb_tree = GB_child(gb_treedata); gb_tree; gb_tree = GB_nextChild(gb_tree), ++t) {
            indexed_name iname;
            iname.name = GB_read_key_pntr(gb_tree);
            iname.idx  = sorted ? get_tree_idx(gb_tree) : ++count;

            trees.insert(iname);
        }
    }

    if (tree_count != (long)trees.size()) { // there are duplicated "order" entries
        gb_assert(sorted); // should not happen in unsorted mode

        typedef std::set<int> ints;

        ints    used_indices;
        GBDATA *gb_first_tree = GB_child(gb_treedata);
        GBDATA *gb_tree       = gb_first_tree;

        while (gb_tree) {
            int idx = get_tree_idx(gb_tree);
            if (used_indices.find(idx) != used_indices.end()) { // duplicate order
                GB_ERROR error    = reserve_tree_idx(gb_treedata, idx+1);
                if (!error) error = set_tree_idx(gb_tree, idx+1);
                if (error) GBK_terminatef("failed to fix tree-order (Reason: %s)", error);

                // now restart
                used_indices.clear();
                gb_tree = gb_first_tree;
            }
            else {
                used_indices.insert(idx);
                gb_tree = GB_nextChild(gb_tree);
            }
        }
        GBT_get_tree_names(names, gb_main, sorted);
        return;
    }

    for (ordered_trees::const_iterator t = trees.begin(); t != trees.end(); ++t) {
        names.put(t->name);
    }
}

NOT4PERL GB_ERROR GBT_move_tree(GBDATA *gb_moved_tree, GBT_ORDER_MODE mode, GBDATA *gb_target_tree) {
    // moves 'gb_moved_tree' next to 'gb_target_tree' (only changes tree-order) 
    gb_assert(gb_moved_tree && gb_target_tree);

    GBDATA *gb_treedata = GB_get_father(gb_moved_tree);
    ensure_trees_have_order(gb_treedata);

    int target_idx = get_tree_idx(gb_target_tree);
    gb_assert(target_idx);

    if (mode == GBT_BEHIND) target_idx++;

    GB_ERROR error    = reserve_tree_idx(gb_treedata, target_idx);
    if (!error) error = set_tree_idx(gb_moved_tree, target_idx);

    return error;
}

static GBDATA *get_source_and_check_target_tree(GBDATA *gb_main, const char *source_tree, const char *dest_tree, GB_ERROR& error) {
    GBDATA *gb_source_tree = NULL;

    error             = GBT_check_tree_name(source_tree);
    if (!error) error = GBT_check_tree_name(dest_tree);

    if (error && strcmp(source_tree, NO_TREE_SELECTED) == 0) {
        error = "No tree selected";
    }

    if (!error && strcmp(source_tree, dest_tree) == 0) error = "source- and dest-tree are the same";

    if (!error) {
        gb_source_tree = GBT_find_tree(gb_main, source_tree);
        if (!gb_source_tree) error = GBS_global_string("tree '%s' not found", source_tree);
        else {
            GBDATA *gb_dest_tree = GBT_find_tree(gb_main, dest_tree);
            if (gb_dest_tree) {
                error = GBS_global_string("tree '%s' already exists", dest_tree);
                gb_source_tree = NULL;
            }
        }
    }

    gb_assert(contradicted(error, gb_source_tree));
    return gb_source_tree;
}

static GBDATA *copy_tree_container(GBDATA *gb_source_tree, const char *newName, GB_ERROR& error) {
    GBDATA *gb_treedata  = GB_get_father(gb_source_tree);
    GBDATA *gb_dest_tree = GB_create_container(gb_treedata, newName);

    if (!gb_dest_tree) error = GB_await_error();
    else error               = GB_copy(gb_dest_tree, gb_source_tree);

    gb_assert(contradicted(error, gb_dest_tree));
    return gb_dest_tree;
}

GB_ERROR GBT_copy_tree(GBDATA *gb_main, const char *source_name, const char *dest_name) {
    GB_ERROR  error;
    GBDATA   *gb_source_tree = get_source_and_check_target_tree(gb_main, source_name, dest_name, error);

    if (gb_source_tree) {
        GBDATA *gb_dest_tree = copy_tree_container(gb_source_tree, dest_name, error);
        if (gb_dest_tree) {
            int source_idx = get_tree_idx(gb_source_tree);
            int dest_idx   = source_idx+1;

            error             = reserve_tree_idx(GB_get_father(gb_dest_tree), dest_idx);
            if (!error) error = set_tree_idx(gb_dest_tree, dest_idx);
        }
    }

    return error;
}

GB_ERROR GBT_rename_tree(GBDATA *gb_main, const char *source_name, const char *dest_name) {
    GB_ERROR  error;
    GBDATA   *gb_source_tree = get_source_and_check_target_tree(gb_main, source_name, dest_name, error);

    if (gb_source_tree) {
        GBDATA *gb_dest_tree = copy_tree_container(gb_source_tree, dest_name, error);
        if (gb_dest_tree) error = GB_delete(gb_source_tree);
    }

    return error;
}

static GB_CSTR *fill_species_name_array(GB_CSTR *current, const TreeNode *tree) {
    if (tree->is_leaf) {
        current[0] = tree->name;
        return current+1;
    }
    current = fill_species_name_array(current, tree->get_leftson());
    current = fill_species_name_array(current, tree->get_rightson());
    return current;
}

GB_CSTR *GBT_get_names_of_species_in_tree(const TreeNode *tree, size_t *count) {
    /* creates an array of all species names in a tree,
     * The names are not allocated (so they may change as side effect of renaming species) */

    size_t   size   = GBT_count_leafs(tree);
    GB_CSTR *result = ARB_calloc<GB_CSTR>(size + 1);
    
    IF_ASSERTION_USED(GB_CSTR *check =) fill_species_name_array(result, tree);
    gb_assert(check - size == result);

    if (count) *count = size;

    return result;
}

static void tree2newick(const TreeNode *tree, GBS_strstruct& out, NewickFormat format, int indent) {
    gb_assert(tree);
    if ((format&nWRAP) && indent>0) { out.put('\n'); out.nput(' ', indent); }
    if (tree->is_leaf) {
        out.cat(tree->name);
    }
    else {
        out.put('(');
        tree2newick(tree->get_leftson(), out, format, indent+1);
        out.put(',');
        tree2newick(tree->get_rightson(), out, format, indent+1);
        if ((format&nWRAP) && indent>0) { out.put('\n'); out.nput(' ', indent); }
        out.put(')');

        if (format & (nGROUP|nREMARK)) {
            const char *remark = format&nREMARK ? tree->get_remark() : NULL;
            const char *group  = format&nGROUP ? tree->name : NULL;

            if (remark || group) {
                out.put('\'');
                if (remark) {
                    out.cat(remark);
                    if (group) out.put(':');
                }
                if (group) out.cat(group);
                out.put('\'');
            }
        }
    }

    if (format&nLENGTH && !tree->is_root_node()) {
        out.put(':');
        out.nprintf(10, "%5.3f", tree->get_branchlength());
    }
}

char *GBT_tree_2_newick(const TreeNode *tree, NewickFormat format, bool compact) {
    GBS_strstruct out(1000);
    if (tree) tree2newick(tree, out, format, 0);
    out.put(';');

    char *result = out.release();
    if (compact && (format&nWRAP)) {
        GB_ERROR error = NULL;

        char *compact1 = GBS_regreplace(result, "/[\n ]*[)]/)/", &error);
        if (compact1) {
            char *compact2 = GBS_regreplace(compact1, "/[(][\n ]*/(/", &error);
            if (compact2) freeset(result, compact2);
            free(compact1);
        }
        if (error) {
            fprintf(stderr, "Error in GBT_tree_2_newick: %s\n", error);
            gb_assert(!error); // should be impossible; falls back to 'result' if happens
        }
    }
    return result;
}


// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>

static const char *getTreeOrder(GBDATA *gb_main) {
    ConstStrArray names;
    GBT_get_tree_names(names, gb_main, true);

    char *joined         = GBT_join_strings(names, '|');
    char *size_and_names = GBS_global_string_copy("%zu:%s", names.size(), joined);
    free(joined);

    RETURN_LOCAL_ALLOC(size_and_names);
}

void TEST_tree_names() {
    TEST_EXPECT_ERROR_CONTAINS(GBT_check_tree_name(""),               "not a valid treename");
    TEST_EXPECT_ERROR_CONTAINS(GBT_check_tree_name("not_a_treename"), "not a valid treename");
    TEST_EXPECT_ERROR_CONTAINS(GBT_check_tree_name("tree_bad.dot"),   "not a valid treename");

    TEST_EXPECT_NO_ERROR(GBT_check_tree_name("tree_")); // ugly but ok
    TEST_EXPECT_NO_ERROR(GBT_check_tree_name("tree_ok"));
}

void TEST_tree_contraints() {
    // test minima
    const int MIN_LEAFS = 2;

    TEST_EXPECT_EQUAL(leafs_2_nodes(MIN_LEAFS, ROOTED), 3);
    TEST_EXPECT_EQUAL(leafs_2_nodes(MIN_LEAFS, UNROOTED), 2);
    TEST_EXPECT_EQUAL(leafs_2_edges(MIN_LEAFS, ROOTED), 2);
    TEST_EXPECT_EQUAL(leafs_2_edges(MIN_LEAFS, UNROOTED), 1);

    TEST_EXPECT_EQUAL(MIN_LEAFS, nodes_2_leafs(3, ROOTED));   // test minimum (3 nodes rooted)
    TEST_EXPECT_EQUAL(MIN_LEAFS, nodes_2_leafs(2, UNROOTED)); // test minimum (2 nodes unrooted)

    TEST_EXPECT_EQUAL(MIN_LEAFS, edges_2_leafs(2, ROOTED));   // test minimum (2 edges rooted)
    TEST_EXPECT_EQUAL(MIN_LEAFS, edges_2_leafs(1, UNROOTED)); // test minimum (1 edge unrooted)

    // test inverse functions:
    for (int i = 3; i<=7; ++i) {
        // test "leaf->XXX" and back conversions (any number of leafs is possible)
        TEST_EXPECT_EQUAL(i, nodes_2_leafs(leafs_2_nodes(i, ROOTED), ROOTED));
        TEST_EXPECT_EQUAL(i, nodes_2_leafs(leafs_2_nodes(i, UNROOTED), UNROOTED));

        TEST_EXPECT_EQUAL(i, edges_2_leafs(leafs_2_edges(i, ROOTED), ROOTED));
        TEST_EXPECT_EQUAL(i, edges_2_leafs(leafs_2_edges(i, UNROOTED), UNROOTED));

        bool odd = i%2;
        if (odd) {
            TEST_EXPECT_EQUAL(i, leafs_2_nodes(nodes_2_leafs(i, ROOTED), ROOTED)); // rooted trees only contain odd numbers of nodes
            TEST_EXPECT_EQUAL(i, leafs_2_edges(edges_2_leafs(i, UNROOTED), UNROOTED)); // unrooted trees only contain odd numbers of edges
        }
        else { // even
            TEST_EXPECT_EQUAL(i, leafs_2_nodes(nodes_2_leafs(i, UNROOTED), UNROOTED)); // unrooted trees only contain even numbers of nodes
            TEST_EXPECT_EQUAL(i, leafs_2_edges(edges_2_leafs(i, ROOTED), ROOTED)); // rooted trees only contain even numbers of edges
        }

        // test adding a leaf adds two nodes:
        int added = i+1;
        TEST_EXPECT_EQUAL(leafs_2_nodes(added, ROOTED)-leafs_2_nodes(i, ROOTED), 2);
        TEST_EXPECT_EQUAL(leafs_2_nodes(added, UNROOTED)-leafs_2_nodes(i, UNROOTED), 2);
    }


}

void TEST_copy_rename_delete_tree_order() {
    GB_shell  shell;
    GBDATA   *gb_main = GB_open("TEST_trees.arb", "r");

    {
        GB_transaction ta(gb_main);

        {
            TEST_EXPECT_NULL(GBT_get_tree_name(NULL));
            
            TEST_EXPECT_EQUAL(GBT_name_of_largest_tree(gb_main), "tree_removal");

            TEST_EXPECT_EQUAL(GBT_get_tree_name(GBT_find_top_tree(gb_main)), "tree_test");
            TEST_EXPECT_EQUAL(GBT_name_of_bottom_tree(gb_main), "tree_removal");

            long inner_nodes = GBT_size_of_tree(gb_main, "tree_nj_bs");
            TEST_EXPECT_EQUAL(inner_nodes, 5);
            TEST_EXPECT_EQUAL(GBT_tree_info_string(gb_main, "tree_nj_bs", -1), "tree_nj_bs       (6:0)  PRG=dnadist CORR=none FILTER=none PKG=ARB");
            TEST_EXPECT_EQUAL(GBT_tree_info_string(gb_main, "tree_nj_bs", 20), "tree_nj_bs                 (6:0)  PRG=dnadist CORR=none FILTER=none PKG=ARB");

            {
                TreeNode *tree = GBT_read_tree(gb_main, "tree_nj_bs", new SimpleRoot);

                TEST_REJECT_NULL(tree);

                size_t leaf_count = GBT_count_leafs(tree);

                size_t   species_count;
                GB_CSTR *species = GBT_get_names_of_species_in_tree(tree, &species_count);

                StrArray species2;
                for (int i = 0; species[i]; ++i) species2.put(ARB_strdup(species[i]));

                TEST_EXPECT_EQUAL(species_count, leaf_count);
                TEST_EXPECT_EQUAL(long(species_count), inner_nodes+1);

                {
                    char *joined = GBT_join_strings(species2, '*');
                    TEST_EXPECT_EQUAL(joined, "CloButyr*CloButy2*CorGluta*CorAquat*CurCitre*CytAquat");
                    free(joined);
                }

                free(species);

                TEST_EXPECT_NEWICK(nSIMPLE, tree, "(CloButyr,(CloButy2,((CorGluta,(CorAquat,CurCitre)),CytAquat)));");
                TEST_EXPECT_NEWICK(nSIMPLE, NULL, ";");

                destroy(tree);
            }

            TEST_EXPECT_EQUAL(GBT_existing_tree(gb_main, "tree_nj_bs"), "tree_nj_bs");
            TEST_EXPECT_EQUAL(GBT_existing_tree(gb_main, "tree_nosuch"), "tree_test");
        }

        // changing tree order
        {
            TEST_EXPECT_EQUAL(getTreeOrder(gb_main), "5:tree_test|tree_tree2|tree_nj|tree_nj_bs|tree_removal");

            GBDATA *gb_test    = GBT_find_tree(gb_main, "tree_test");
            GBDATA *gb_tree2   = GBT_find_tree(gb_main, "tree_tree2");
            GBDATA *gb_nj      = GBT_find_tree(gb_main, "tree_nj");
            GBDATA *gb_nj_bs   = GBT_find_tree(gb_main, "tree_nj_bs");
            GBDATA *gb_removal = GBT_find_tree(gb_main, "tree_removal");

            TEST_EXPECT_NO_ERROR(GBT_move_tree(gb_test, GBT_BEHIND, gb_removal)); // move to bottom
            TEST_EXPECT_EQUAL(getTreeOrder(gb_main), "5:tree_tree2|tree_nj|tree_nj_bs|tree_removal|tree_test");

            TEST_EXPECT_EQUAL(GBT_tree_behind(gb_tree2), gb_nj);
            TEST_EXPECT_EQUAL(GBT_tree_behind(gb_nj), gb_nj_bs);
            TEST_EXPECT_EQUAL(GBT_tree_behind(gb_nj_bs), gb_removal);
            TEST_EXPECT_EQUAL(GBT_tree_behind(gb_removal), gb_test);
            TEST_EXPECT_NULL(GBT_tree_behind(gb_test));

            TEST_EXPECT_NULL(GBT_tree_infrontof(gb_tree2));
            TEST_EXPECT_EQUAL(GBT_tree_infrontof(gb_nj), gb_tree2);
            TEST_EXPECT_EQUAL(GBT_tree_infrontof(gb_nj_bs), gb_nj);
            TEST_EXPECT_EQUAL(GBT_tree_infrontof(gb_removal), gb_nj_bs);
            TEST_EXPECT_EQUAL(GBT_tree_infrontof(gb_test), gb_removal);

            TEST_EXPECT_NO_ERROR(GBT_move_tree(gb_test, GBT_INFRONTOF, gb_tree2)); // move back to top
            TEST_EXPECT_EQUAL(getTreeOrder(gb_main), "5:tree_test|tree_tree2|tree_nj|tree_nj_bs|tree_removal");

            TEST_EXPECT_NO_ERROR(GBT_move_tree(gb_test, GBT_BEHIND, gb_tree2)); // move from top
            TEST_EXPECT_EQUAL(getTreeOrder(gb_main), "5:tree_tree2|tree_test|tree_nj|tree_nj_bs|tree_removal");

            TEST_EXPECT_NO_ERROR(GBT_move_tree(gb_removal, GBT_INFRONTOF, gb_nj)); // move from end
            TEST_EXPECT_EQUAL(getTreeOrder(gb_main), "5:tree_tree2|tree_test|tree_removal|tree_nj|tree_nj_bs");

            TEST_EXPECT_NO_ERROR(GBT_move_tree(gb_nj_bs, GBT_INFRONTOF, gb_nj_bs)); // noop
            TEST_EXPECT_EQUAL(getTreeOrder(gb_main), "5:tree_tree2|tree_test|tree_removal|tree_nj|tree_nj_bs");

            TEST_EXPECT_EQUAL(GBT_get_tree_name(GBT_find_top_tree(gb_main)), "tree_tree2");

            TEST_EXPECT_EQUAL(GBT_get_tree_name(GBT_find_next_tree(gb_removal)), "tree_nj");
            TEST_EXPECT_EQUAL(GBT_get_tree_name(GBT_find_next_tree(gb_nj_bs)), "tree_tree2"); // last -> first
        }

        // check tree order is maintained by copy, rename and delete

        {
            // copy
            TEST_EXPECT_ERROR_CONTAINS(GBT_copy_tree(gb_main, "tree_nosuch", "tree_whatever"), "tree 'tree_nosuch' not found");
            TEST_EXPECT_ERROR_CONTAINS(GBT_copy_tree(gb_main, "tree_test",   "tree_test"),     "source- and dest-tree are the same");
            TEST_EXPECT_ERROR_CONTAINS(GBT_copy_tree(gb_main, "tree_tree2",  "tree_test"),     "tree 'tree_test' already exists");

            TEST_EXPECT_NO_ERROR(GBT_copy_tree(gb_main, "tree_test", "tree_test_copy"));
            TEST_REJECT_NULL(GBT_find_tree(gb_main, "tree_test_copy"));
            TEST_EXPECT_EQUAL(getTreeOrder(gb_main), "6:tree_tree2|tree_test|tree_test_copy|tree_removal|tree_nj|tree_nj_bs");

            // rename
            TEST_EXPECT_NO_ERROR(GBT_rename_tree(gb_main, "tree_nj", "tree_renamed_nj"));
            TEST_REJECT_NULL(GBT_find_tree(gb_main, "tree_renamed_nj"));
            TEST_EXPECT_EQUAL(getTreeOrder(gb_main), "6:tree_tree2|tree_test|tree_test_copy|tree_removal|tree_renamed_nj|tree_nj_bs");

            TEST_EXPECT_NO_ERROR(GBT_rename_tree(gb_main, "tree_tree2", "tree_renamed_tree2"));
            TEST_EXPECT_EQUAL(getTreeOrder(gb_main), "6:tree_renamed_tree2|tree_test|tree_test_copy|tree_removal|tree_renamed_nj|tree_nj_bs");

            TEST_EXPECT_NO_ERROR(GBT_rename_tree(gb_main, "tree_test_copy", "tree_renamed_test_copy"));
            TEST_EXPECT_EQUAL(getTreeOrder(gb_main), "6:tree_renamed_tree2|tree_test|tree_renamed_test_copy|tree_removal|tree_renamed_nj|tree_nj_bs");

            // delete

            GBDATA *gb_nj_bs             = GBT_find_tree(gb_main, "tree_nj_bs");
            GBDATA *gb_renamed_nj        = GBT_find_tree(gb_main, "tree_renamed_nj");
            GBDATA *gb_renamed_test_copy = GBT_find_tree(gb_main, "tree_renamed_test_copy");
            GBDATA *gb_renamed_tree2     = GBT_find_tree(gb_main, "tree_renamed_tree2");
            GBDATA *gb_test              = GBT_find_tree(gb_main, "tree_test");
            GBDATA *gb_removal           = GBT_find_tree(gb_main, "tree_removal");

            TEST_EXPECT_NO_ERROR(GB_delete(gb_renamed_tree2));
            TEST_EXPECT_NO_ERROR(GB_delete(gb_renamed_test_copy));
            TEST_EXPECT_NO_ERROR(GB_delete(gb_renamed_nj));
            TEST_EXPECT_NO_ERROR(GB_delete(gb_removal));

            // .. two trees left

            TEST_EXPECT_EQUAL(getTreeOrder(gb_main), "2:tree_test|tree_nj_bs");

            TEST_EXPECT_EQUAL(GBT_find_largest_tree(gb_main), gb_test);
            TEST_EXPECT_EQUAL(GBT_find_top_tree(gb_main), gb_test);
            TEST_EXPECT_EQUAL(GBT_find_bottom_tree(gb_main), gb_nj_bs);
            
            TEST_EXPECT_EQUAL(GBT_find_next_tree(gb_test), gb_nj_bs);
            TEST_EXPECT_EQUAL(GBT_find_next_tree(gb_test), gb_nj_bs);
            TEST_EXPECT_EQUAL(GBT_find_next_tree(gb_nj_bs), gb_test);

            TEST_EXPECT_NULL (GBT_tree_infrontof(gb_test));
            TEST_EXPECT_EQUAL(GBT_tree_behind   (gb_test), gb_nj_bs);
            
            TEST_EXPECT_EQUAL(GBT_tree_infrontof(gb_nj_bs), gb_test);
            TEST_EXPECT_NULL (GBT_tree_behind   (gb_nj_bs));

            TEST_EXPECT_NO_ERROR(GBT_move_tree(gb_test, GBT_BEHIND, gb_nj_bs)); // move to bottom
            TEST_EXPECT_EQUAL(getTreeOrder(gb_main), "2:tree_nj_bs|tree_test");
            TEST_EXPECT_NO_ERROR(GBT_move_tree(gb_test, GBT_INFRONTOF, gb_nj_bs)); // move to top
            TEST_EXPECT_EQUAL(getTreeOrder(gb_main), "2:tree_test|tree_nj_bs");
            
            TEST_EXPECT_NO_ERROR(GB_delete(gb_nj_bs));

            // .. one tree left

            TEST_EXPECT_EQUAL(getTreeOrder(gb_main), "1:tree_test");

            TEST_EXPECT_EQUAL(GBT_find_largest_tree(gb_main), gb_test);
            TEST_EXPECT_EQUAL(GBT_find_top_tree(gb_main), gb_test);
            TEST_EXPECT_EQUAL(GBT_find_bottom_tree(gb_main), gb_test);
            
            TEST_EXPECT_NULL(GBT_find_next_tree(gb_test)); // no other tree left
            TEST_EXPECT_NULL(GBT_tree_behind(gb_test));
            TEST_EXPECT_NULL(GBT_tree_infrontof(gb_test));

            TEST_EXPECT_NO_ERROR(GB_delete(gb_test));

            // .. no tree left
            
            TEST_EXPECT_EQUAL(getTreeOrder(gb_main), "0:");

            TEST_EXPECT_NULL(GBT_find_tree(gb_main, "tree_test"));
            TEST_EXPECT_NULL(GBT_existing_tree(gb_main, "tree_whatever"));
            TEST_EXPECT_NULL(GBT_find_largest_tree(gb_main));
        }
    }

    GB_close(gb_main);
}
TEST_PUBLISH(TEST_copy_rename_delete_tree_order);

void TEST_tree_remove_leafs() {
    GB_shell  shell;
    GBDATA   *gb_main = GB_open("TEST_trees.arb", "r");

    {
        GBT_TreeRemoveType tested_modes[] = {
            GBT_REMOVE_MARKED,
            GBT_REMOVE_UNMARKED,
            GBT_REMOVE_ZOMBIES,
            GBT_KEEP_MARKED,
        };

        const char *org_topo          = "((CloInnoc:0.371,(CloTyrob:0.009,(CloTyro2:0.017,(CloTyro3:1.046,CloTyro4:0.061):0.026):0.017):0.274):0.029,(CloBifer:0.388,((CloCarni:0.120,CurCitre:0.058):1.000,((CloPaste:0.179,(Zombie1:0.120,(CloButy2:0.009,CloButyr:0.000):0.564):0.010):0.131,(CytAquat:0.711,(CelBiazo:0.059,(CorGluta:0.522,(CorAquat:0.084,Zombie2:0.058):0.103):0.054):0.207):0.162):0.124):0.124):0.029);";
        const char *rem_marked_topo   = "((CloInnoc:0.371,(CloTyrob:0.009,(CloTyro2:0.017,(CloTyro3:1.046,CloTyro4:0.061):0.026):0.017):0.274):0.029,(CloBifer:0.388,(CloCarni:1.000,((CloPaste:0.179,Zombie1:0.010):0.131,(CelBiazo:0.059,Zombie2:0.054):0.162):0.124):0.124):0.029);";
        const char *rem_unmarked_topo = "(CurCitre:1.000,((Zombie1:0.120,(CloButy2:0.009,CloButyr:0.000):0.564):0.131,(CytAquat:0.711,(CorGluta:0.522,(CorAquat:0.084,Zombie2:0.058):0.103):0.207):0.162):0.124);";
        const char *rem_zombies_topo  = "((CloInnoc:0.371,(CloTyrob:0.009,(CloTyro2:0.017,(CloTyro3:1.046,CloTyro4:0.061):0.026):0.017):0.274):0.029,(CloBifer:0.388,((CloCarni:0.120,CurCitre:0.058):1.000,((CloPaste:0.179,(CloButy2:0.009,CloButyr:0.000):0.010):0.131,(CytAquat:0.711,(CelBiazo:0.059,(CorGluta:0.522,CorAquat:0.103):0.054):0.207):0.162):0.124):0.124):0.029);";
        const char *kept_marked_topo  = "(CurCitre:1.000,((CloButy2:0.009,CloButyr:0.000):0.131,(CytAquat:0.711,(CorGluta:0.522,CorAquat:0.103):0.207):0.162):0.124);";

        const char *kept_zombies_topo        = "(Zombie1:0.131,Zombie2:0.162);";
        const char *kept_zombies_broken_topo = "Zombie2;";

        const char *empty_topo = ";";

        GB_transaction ta(gb_main);
        for (unsigned mode = 0; mode<ARRAY_ELEMS(tested_modes); ++mode) {
            GBT_TreeRemoveType what = tested_modes[mode];

            for (int linked = 0; linked<=1; ++linked) {
                TEST_ANNOTATE(GBS_global_string("mode=%u linked=%i", mode, linked));

                TreeNode *tree = GBT_read_tree(gb_main, "tree_removal", new SimpleRoot);
                gb_assert(tree);
                bool once = mode == 0 && linked == 0;

                if (linked) {
                    int zombies    = 0;
                    int duplicates = 0;

                    TEST_EXPECT_NO_ERROR(GBT_link_tree(tree, gb_main, false, &zombies, &duplicates));

                    TEST_EXPECT_EQUAL(zombies, 2);
                    TEST_EXPECT_EQUAL(duplicates, 0);
                }

                if (once) TEST_EXPECT_NEWICK(nLENGTH, tree, org_topo);

                int removedCount       = 0;
                int groupsRemovedCount = 0;

                tree = GBT_remove_leafs(tree, what, NULL, &removedCount, &groupsRemovedCount);

                if (linked) {
                    GBT_TreeRemoveType what_next = what;

                    switch (what) {
                        case GBT_REMOVE_MARKED:
                            TEST_EXPECT_EQUAL(removedCount, 6);
                            TEST_EXPECT_EQUAL(groupsRemovedCount, 0);
                            TEST_EXPECT_NEWICK(nLENGTH, tree, rem_marked_topo);
                            what_next = GBT_REMOVE_UNMARKED;
                            break;
                        case GBT_REMOVE_UNMARKED:
                            TEST_EXPECT_EQUAL(removedCount, 9);
                            TEST_EXPECT_EQUAL(groupsRemovedCount, 1);
                            TEST_EXPECT_NEWICK(nLENGTH, tree, rem_unmarked_topo);
                            what_next = GBT_REMOVE_MARKED;
                            break;
                        case GBT_REMOVE_ZOMBIES:
                            TEST_EXPECT_EQUAL(removedCount, 2);
                            TEST_EXPECT_EQUAL(groupsRemovedCount, 0);
                            TEST_EXPECT_NEWICK(nLENGTH, tree, rem_zombies_topo);
                            break;
                        case GBT_KEEP_MARKED:
                            TEST_EXPECT_EQUAL(removedCount, 11);
                            TEST_EXPECT_EQUAL(groupsRemovedCount, 1);
                            TEST_EXPECT_NEWICK(nLENGTH, tree, kept_marked_topo);
                            {
                                // just a test for nWRAP NewickFormat (may be removed later)
                                const char *kept_marked_topo_wrapped =
                                    "(\n"
                                    " CurCitre:1.000,\n"
                                    " (\n"
                                    "  (\n"
                                    "   CloButy2:0.009,\n"
                                    "   CloButyr:0.000\n"
                                    "  ):0.131,\n"
                                    "  (\n"
                                    "   CytAquat:0.711,\n"
                                    "   (\n"
                                    "    CorGluta:0.522,\n"
                                    "    CorAquat:0.103\n"
                                    "   ):0.207\n"
                                    "  ):0.162\n"
                                    " ):0.124);";
                                TEST_EXPECT_NEWICK(NewickFormat(nLENGTH|nWRAP), tree, kept_marked_topo_wrapped);

                                const char *expected_compacted =
                                    "(CurCitre:1.000,\n"
                                    " ((CloButy2:0.009,\n"
                                    "   CloButyr:0.000):0.131,\n"
                                    "  (CytAquat:0.711,\n"
                                    "   (CorGluta:0.522,\n"
                                    "    CorAquat:0.103):0.207):0.162):0.124);";
                                char *compacted = GBT_tree_2_newick(tree, NewickFormat(nLENGTH|nWRAP), true);
                                TEST_EXPECT_EQUAL(compacted, expected_compacted);
                                free(compacted);
                            }
                            what_next = GBT_REMOVE_MARKED;
                            break;
                    }

                    if (what_next != what) {
                        gb_assert(tree);
                        tree = GBT_remove_leafs(tree, what_next, NULL, &removedCount, &groupsRemovedCount);

                        switch (what) {
                            case GBT_REMOVE_MARKED: // + GBT_REMOVE_UNMARKED
                                TEST_EXPECT_EQUAL(removedCount, 16);
                                TEST_EXPECT_EQUAL(groupsRemovedCount, 1);
                                TEST_EXPECT_NEWICK__BROKEN(nLENGTH, tree, kept_zombies_topo);
                                TEST_EXPECT_NEWICK(nLENGTH, tree, kept_zombies_broken_topo); // @@@ invalid topology (single leaf)
                                break;
                            case GBT_REMOVE_UNMARKED: // + GBT_REMOVE_MARKED
                                TEST_EXPECT_EQUAL(removedCount, 15);
                                TEST_EXPECT_EQUAL(groupsRemovedCount, 1);
                                TEST_EXPECT_NEWICK(nLENGTH, tree, kept_zombies_topo);
                                break;
                            case GBT_KEEP_MARKED: // + GBT_REMOVE_MARKED
                                TEST_EXPECT_EQUAL(removedCount, 17);
                                TEST_EXPECT_EQUAL__BROKEN(groupsRemovedCount, 2, 1); // @@@ expect that all groups have been removed! 
                                TEST_EXPECT_EQUAL(groupsRemovedCount, 1);
                                TEST_EXPECT_NEWICK(nLENGTH, tree, empty_topo);
                                break;
                            default:
                                TEST_REJECT(true);
                                break;
                        }
                    }
                }
                else {
                    switch (what) {
                        case GBT_REMOVE_MARKED:
                        case GBT_REMOVE_UNMARKED:
                            TEST_EXPECT_EQUAL(removedCount, 0);
                            TEST_EXPECT_EQUAL(groupsRemovedCount, 0);
                            TEST_EXPECT_NEWICK(nLENGTH, tree, org_topo);
                            break;
                        case GBT_REMOVE_ZOMBIES:
                        case GBT_KEEP_MARKED:
                            TEST_EXPECT_EQUAL(removedCount, 17);
                            TEST_EXPECT_EQUAL(groupsRemovedCount, 2);
                            TEST_EXPECT_NEWICK(nLENGTH, tree, empty_topo);
                            break;
                    }
                }

                if (tree) {
                    gb_assert(tree->is_root_node());
                    destroy(tree);
                }
            }
        }
    }

    GB_close(gb_main);
}
TEST_PUBLISH(TEST_tree_remove_leafs);


#endif // UNIT_TESTS
