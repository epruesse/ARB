// =============================================================== //
//                                                                 //
//   File      : adtree.cxx                                        //
//   Purpose   : tree functions                                    //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <arbdbt.h>
#include "gb_local.h"

#define GBT_PUT_DATA 1
#define GBT_GET_SIZE 0

// ----------------------
//      remove leafs


static GBT_TREE *fixDeletedSon(GBT_TREE *tree) {
    // fix tree after one son has been deleted
    // (Note: this function only works correct for trees with minimum element size!)
    GBT_TREE *delNode = tree;

    if (delNode->leftson) {
        gb_assert(!delNode->rightson);
        tree             = delNode->leftson;
        delNode->leftson = 0;
    }
    else {
        gb_assert(!delNode->leftson);
        gb_assert(delNode->rightson);

        tree              = delNode->rightson;
        delNode->rightson = 0;
    }

    // now tree is the new tree
    tree->father = delNode->father;

    if (delNode->remark_branch && !tree->remark_branch) { // rescue remarks if possible
        tree->remark_branch    = delNode->remark_branch;
        delNode->remark_branch = 0;
    }
    if (delNode->gb_node && !tree->gb_node) { // rescue group if possible
        tree->gb_node    = delNode->gb_node;
        delNode->gb_node = 0;
    }

    delNode->is_leaf = true; // don't try recursive delete

    if (delNode->father) { // not root
        GBT_delete_tree(delNode);
    }
    else { // root node
        if (delNode->tree_is_one_piece_of_memory) {
            // don't change root -> copy instead
            memcpy(delNode, tree, sizeof(GBT_TREE));
            tree = delNode;
        }
        else {
            GBT_delete_tree(delNode);
        }
    }
    return tree;
}


GBT_TREE *GBT_remove_leafs(GBT_TREE *tree, GBT_TREE_REMOVE_TYPE mode, GB_HASH *species_hash, int *removed, int *groups_removed) {
    // Given 'tree' can either
    // - be linked (in this case 'species_hash' shall be NULL)
    // - be unlinked (in this case 'species_hash' has to be provided)
    //
    // If 'removed' and/or 'groups_removed' is given, it's used to count the removed leafs/groups.

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
                if (mode & (GBT_REMOVE_MARKED|GBT_REMOVE_NOT_MARKED)) {
                    long flag = GB_read_flag(gb_node);
                    deleteSelf = (flag && (mode&GBT_REMOVE_MARKED)) || (!flag && (mode&GBT_REMOVE_NOT_MARKED));
                }
            }
            else { // zombie
                if (mode & GBT_REMOVE_DELETED) deleteSelf = true;
            }

            if (deleteSelf) {
                GBT_delete_tree(tree);
                if (removed) (*removed)++;
                tree = 0;
            }
        }
    }
    else {
        tree->leftson  = GBT_remove_leafs(tree->leftson, mode, species_hash, removed, groups_removed);
        tree->rightson = GBT_remove_leafs(tree->rightson, mode, species_hash, removed, groups_removed);

        if (tree->leftson) {
            if (!tree->rightson) { // right son deleted
                tree = fixDeletedSon(tree);
            }
            // otherwise no son deleted
        }
        else if (tree->rightson) { // left son deleted
            tree = fixDeletedSon(tree);
        }
        else {                  // everything deleted -> delete self
            if (tree->name && groups_removed) (*groups_removed)++;
            tree->is_leaf = true;
            GBT_delete_tree(tree);
            tree          = 0;
        }
    }

    return tree;
}

// -------------------
//      free tree


void GBT_delete_tree(GBT_TREE *tree)
     /* frees a tree only in memory (not in the database)
        to delete the tree in Database
        just call GB_delete((GBDATA *)gb_tree);
     */
{
    free(tree->name);
    free(tree->remark_branch);

    if (!tree->is_leaf) {
        GBT_delete_tree(tree->leftson);
        GBT_delete_tree(tree->rightson);
    }
    if (!tree->tree_is_one_piece_of_memory || !tree->father) {
        free(tree);
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

static GB_ERROR gbt_write_tree_nodes(GBDATA *gb_tree, GBT_TREE *node, long *startid) {
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
                error             = GBT_write_int(node->gb_node, "id", *startid);
                if (!error) error = GB_write_usr_private(node->gb_node, 0);
            }
            else {          // delete unused nodes
                error = GB_delete(node->gb_node);
                if (!error) node->gb_node = 0;
            }
        }

        (*startid)++;
        if (!error) error = gbt_write_tree_nodes(gb_tree, node->leftson, startid);
        if (!error) error = gbt_write_tree_nodes(gb_tree, node->rightson, startid);
    }
    return error;
}

static char *gbt_write_tree_rek_new(const GBT_TREE *node, char *dest, long mode) {
    char buffer[40];        // just real numbers
    char    *c1;

    if ((c1 = node->remark_branch)) {
        int c;
        if (mode == GBT_PUT_DATA) {
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

    if (node->is_leaf) {
        if (mode == GBT_PUT_DATA) {
            *(dest++) = 'L';
            if (node->name) strcpy(dest, node->name);
            while ((c1 = (char *)strchr(dest, 1))) *c1 = 2;
            dest += strlen(dest);
            *(dest++) = 1;
            return dest;
        }
        else {
            if (node->name) return dest+1+strlen(node->name)+1; // N name term
            return dest+1+1;
        }
    }
    else {
        sprintf(buffer, "%g,%g;", node->leftlen, node->rightlen);
        if (mode == GBT_PUT_DATA) {
            *(dest++) = 'N';
            strcpy(dest, buffer);
            dest += strlen(buffer);
        }
        else {
            dest += strlen(buffer)+1;
        }
        dest = gbt_write_tree_rek_new(node->leftson,  dest, mode);
        dest = gbt_write_tree_rek_new(node->rightson, dest, mode);
        return dest;
    }
}

static GB_ERROR gbt_write_tree(GBDATA *gb_main, GBDATA *gb_tree, const char *tree_name, GBT_TREE *tree, int plain_only) {
    /*! writes a tree to the database.
     *
     * If tree is loaded by function GBT_read_tree(..) then 'tree_name' should be NULL
     * else 'gb_tree' should be set to NULL
     *
     * To copy a tree call GB_copy((GBDATA *)dest,(GBDATA *)source);
     * or set recursively all tree->gb_node variables to zero (that unlinks the tree),
     *
     * if 'plain_only' == 1 only the plain tree string is written
     */

    GB_ERROR error = 0;

    gb_assert(!plain_only || (tree_name == 0)); // if plain_only == 1 -> set tree_name to 0

    if (tree) {
        if (tree_name) {
            if (gb_tree) error = GBS_global_string("can't change name of existing tree (to '%s')", tree_name);
            else {
                error = GBT_check_tree_name(tree_name);
                if (!error) {
                    GBDATA *gb_tree_data = GB_search(gb_main, "tree_data", GB_CREATE_CONTAINER);
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
            if (!plain_only) {
                // mark all old style tree data for deletion
                GBDATA *gb_node;
                for (gb_node = GB_entry(gb_tree, "node"); gb_node; gb_node = GB_nextEntry(gb_node)) {
                    GB_write_usr_private(gb_node, 1);
                }
            }

            // build tree-string and save to DB
            {
                char *t_size = gbt_write_tree_rek_new(tree, 0, GBT_GET_SIZE); // calc size of tree-string
                char *ctree  = (char *)GB_calloc(sizeof(char), (size_t)(t_size+1)); // allocate buffer for tree-string

                t_size = gbt_write_tree_rek_new(tree, ctree, GBT_PUT_DATA); // write into buffer
                *(t_size) = 0;

                gb_set_compression_mask(gb_main, 0); // no more compressions
                error = GBT_write_string(gb_tree, "tree", ctree);
                gb_set_compression_mask(gb_main, -1); // again allow all types of compression
                free(ctree);
            }
        }

        if (!plain_only && !error) {
            // save nodes to DB
            long size         = 0;
            error             = gbt_write_tree_nodes(gb_tree, tree, &size); // reports number of nodes in 'size'
            if (!error) error = GBT_write_int(gb_tree, "nnodes", size);

            if (!error) {
                GBDATA *gb_node;
                GBDATA *gb_node_next;

                for (gb_node = GB_entry(gb_tree, "node"); // delete all ghost nodes
                     gb_node && !error;
                     gb_node = gb_node_next)
                {
                    GBDATA *gbd = GB_entry(gb_node, "id");
                    gb_node_next = GB_nextEntry(gb_node);
                    if (!gbd || GB_read_usr_private(gb_node)) error = GB_delete(gb_node);
                }
            }
        }
    }

    return error;
}

GB_ERROR GBT_write_tree(GBDATA *gb_main, GBDATA *gb_tree, const char *tree_name, GBT_TREE *tree) {
    return gbt_write_tree(gb_main, gb_tree, tree_name, tree, 0);
}
GB_ERROR GBT_write_plain_tree(GBDATA *gb_main, GBDATA *gb_tree, char *tree_name, GBT_TREE *tree) {
    return gbt_write_tree(gb_main, gb_tree, tree_name, tree, 1);
}

GB_ERROR GBT_write_tree_rem(GBDATA *gb_main, const char *tree_name, const char *remark) {
    return GBT_write_string(GBT_get_tree(gb_main, tree_name), "remark", remark);
}

// ----------------------------
//      tree read functions

GBT_TREE *gbt_read_tree_rek(char **data, long *startid, GBDATA **gb_tree_nodes, long structure_size, int size_of_tree, GB_ERROR *error)
{
    GBT_TREE    *node;
    GBDATA      *gb_group_name;
    char         c;
    char        *p1;
    static char *membase;

    gb_assert(error);
    if (*error) return NULL;

    if (structure_size>0) {
        node = (GBT_TREE *)GB_calloc(1, (size_t)structure_size);
    }
    else {
        if (!startid[0]) {
            membase = (char *)GB_calloc(size_of_tree+1, (size_t)(-2*structure_size)); // because of inner nodes
        }
        node = (GBT_TREE *)membase;
        node->tree_is_one_piece_of_memory = 1;
        membase -= structure_size;
    }

    c = *((*data)++);

    if (c=='R') {
        p1 = strchr(*data, 1);
        *(p1++) = 0;
        node->remark_branch = strdup(*data);
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
            gb_group_name = GB_entry(node->gb_node, "group_name");
            if (gb_group_name) {
                node->name = GB_read_string(gb_group_name);
            }
        }
        (*startid)++;
        node->leftson = gbt_read_tree_rek(data, startid, gb_tree_nodes, structure_size, size_of_tree, error);
        if (!node->leftson) {
            if (!node->tree_is_one_piece_of_memory) free((char *)node);
            return NULL;
        }
        node->rightson = gbt_read_tree_rek(data, startid, gb_tree_nodes, structure_size, size_of_tree, error);
        if (!node->rightson) {
            if (!node->tree_is_one_piece_of_memory) free((char *)node);
            return NULL;
        }
        node->leftson->father = node;
        node->rightson->father = node;
    }
    else if (c=='L') {
        node->is_leaf = true;
        p1            = (char *)strchr(*data, 1);

        gb_assert(p1);
        gb_assert(p1[0] == 1);

        *p1        = 0;
        node->name = strdup(*data);
        *data      = p1+1;
    }
    else {
        if (!c) {
            *error = "Unexpected end of tree definition.";
        }
        else {
            *error = GBS_global_string("Can't interpret tree definition (expected 'N' or 'L' - not '%c')", c);
        }
        return NULL;
    }
    return node;
}


static GBT_TREE *read_tree_and_size_internal(GBDATA *gb_tree, GBDATA *gb_ctree, int structure_size, int size, GB_ERROR *error) {
    GBDATA   **gb_tree_nodes;
    GBT_TREE  *node = 0;

    gb_tree_nodes = (GBDATA **)GB_calloc(sizeof(GBDATA *), (size_t)size);
    if (gb_tree) {
        GBDATA *gb_node;

        for (gb_node = GB_entry(gb_tree, "node"); gb_node && !*error; gb_node = GB_nextEntry(gb_node)) {
            long    i;
            GBDATA *gbd = GB_entry(gb_node, "id");
            if (!gbd) continue;

            i = GB_read_int(gbd);
            if (i<0 || i >= size) {
                *error = "An inner node of the tree is corrupt";
            }
            else {
                gb_tree_nodes[i] = gb_node;
            }
        }
    }
    if (!*error) {
        char *cptr[1];
        long  startid[1];
        char *fbuf;

        startid[0] = 0;
        fbuf       = cptr[0] = GB_read_string(gb_ctree);
        node       = gbt_read_tree_rek(cptr, startid, gb_tree_nodes, structure_size, (int)size, error);
        free (fbuf);
    }

    free((char *)gb_tree_nodes);

    return node;
}

GBT_TREE *GBT_read_tree_and_size(GBDATA *gb_main, const char *tree_name, long structure_size, int *tree_size) {
    /*! Loads a tree from DB into any user defined structure.
     *
     * Make sure that the first members of your structure look exactly like GBT_TREE!
     *
     * @param structure_size sizeof(yourStructure)
     *
     * If structure_size < 0 then the tree is allocated as just one big piece of memory,
     * which can be freed by free((char *)root_of_tree) + deleting names or
     * by GBT_delete_tree().
     *
     * @param tree_name is the name of the tree in the db
     *
     * @param tree_size if != NULL -> gets set to "size of tree"
     *
     * @return
     * - NULL if any error occurs (which is exported then)
     * - root of loaded tree
     */

    GB_ERROR error = 0;

    if (!tree_name) {
        error = "no treename given";
    }
    else {
        error = GBT_check_tree_name(tree_name);
        if (!error) {
            GBDATA *gb_tree = GBT_get_tree(gb_main, tree_name);

            if (!gb_tree) {
                error = GBS_global_string("Could not find tree '%s'", tree_name);
            }
            else {
                GBDATA *gb_nnodes = GB_entry(gb_tree, "nnodes");
                if (!gb_nnodes) {
                    error = GBS_global_string("Tree '%s' is empty", tree_name);
                }
                else {
                    long size = GB_read_int(gb_nnodes);
                    if (!size) {
                        error = GBS_global_string("Tree '%s' has zero size", tree_name);
                    }
                    else {
                        GBDATA *gb_ctree = GB_search(gb_tree, "tree", GB_FIND);
                        if (!gb_ctree) {
                            error = "Sorry - old tree format no longer supported";
                        }
                        else { // "new" style tree
                            GBT_TREE *tree = read_tree_and_size_internal(gb_tree, gb_ctree, structure_size, size, &error);
                            if (!error) {
                                gb_assert(tree);
                                if (tree_size) *tree_size = size; // return size of tree
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
    GB_export_errorf("Couldn't read tree '%s' (Reason: %s)", tree_name, error);
    return NULL;
}

GBT_TREE *GBT_read_tree(GBDATA *gb_main, const char *tree_name, long structure_size) {
    //! @see GBT_read_tree_and_size()
    return GBT_read_tree_and_size(gb_main, tree_name, structure_size, 0);
}

GBT_TREE *GBT_read_plain_tree(GBDATA *gb_main, GBDATA *gb_ctree, long structure_size, GB_ERROR *error) {
    GBT_TREE *t;

    gb_assert(error && !*error); // expect cleared error

    GB_push_transaction(gb_main);
    t = read_tree_and_size_internal(0, gb_ctree, structure_size, 0, error);
    GB_pop_transaction(gb_main);

    return t;
}

size_t GBT_count_leafs(const GBT_TREE *tree) {
    if (tree->is_leaf) {
        return 1;
    }
    return GBT_count_leafs(tree->leftson) + GBT_count_leafs(tree->rightson);
}

// -------------------------------------------
//      link the tree tips to the database

struct link_tree_data {
    GB_HASH *species_hash;
    GB_HASH *seen_species;                          // used to count duplicates
    int      nodes;                                 // if != 0 -> update status
    int      counter;
    int      zombies;                               // counts zombies
    int      duplicates;                            // counts duplicates
};

static GB_ERROR gbt_link_tree_to_hash_rek(GBT_TREE *tree, struct link_tree_data *ltd) {
    GB_ERROR error = 0;
    if (tree->is_leaf) {
        if (ltd->nodes) { // update status?
            ltd->counter++;
            GB_status(ltd->counter/(double)ltd->nodes);
        }

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
    }
    else {
        error = gbt_link_tree_to_hash_rek(tree->leftson, ltd);
        if (!error) error = gbt_link_tree_to_hash_rek(tree->rightson, ltd);
    }
    return error;
}

GB_ERROR GBT_link_tree_using_species_hash(GBT_TREE *tree, bool show_status, GB_HASH *species_hash, int *zombies, int *duplicates) {
    GB_ERROR              error;
    struct link_tree_data ltd;
    long                  leafs = 0;

    if (duplicates || show_status) {
        leafs = GBT_count_leafs(tree);
    }

    ltd.species_hash = species_hash;
    ltd.seen_species = leafs ? GBS_create_hash(2*leafs, GB_IGNORE_CASE) : 0;
    ltd.zombies      = 0;
    ltd.duplicates   = 0;
    ltd.counter      = 0;

    if (show_status) {
        GB_status2("Relinking tree to database");
        ltd.nodes = leafs;
    }
    else {
        ltd.nodes = 0;
    }

    error = gbt_link_tree_to_hash_rek(tree, &ltd);
    if (ltd.seen_species) GBS_free_hash(ltd.seen_species);

    if (zombies) *zombies = ltd.zombies;
    if (duplicates) *duplicates = ltd.duplicates;

    return error;
}

GB_ERROR GBT_link_tree(GBT_TREE *tree, GBDATA *gb_main, bool show_status, int *zombies, int *duplicates) {
    /*! Link a given tree to the database. That means that for all tips the member
     * 'gb_node' is set to the database container holding the species data.
     *
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

void GBT_unlink_tree(GBT_TREE *tree) {
    /*! Unlink tree from the database.
     * @see GBT_link_tree()
     */
    tree->gb_node = 0;
    if (!tree->is_leaf) {
        GBT_unlink_tree(tree->leftson);
        GBT_unlink_tree(tree->rightson);
    }
}


GBDATA *GBT_get_tree(GBDATA *gb_main, const char *tree_name) {
    /*! @return
     * - DB tree container associated with tree_name
     * - NULL if no such tree exists
     */
    GBDATA *gb_treedata = GB_search(gb_main, "tree_data", GB_CREATE_CONTAINER);
    return GB_entry(gb_treedata, tree_name);
}

long GBT_size_of_tree(GBDATA *gb_main, const char *tree_name) {
    GBDATA *gb_tree = GBT_get_tree(gb_main, tree_name);
    GBDATA *gb_nnodes;
    if (!gb_tree) return -1;
    gb_nnodes = GB_entry(gb_tree, "nnodes");
    if (!gb_nnodes) return -1;
    return GB_read_int(gb_nnodes);
}

char *GBT_find_largest_tree(GBDATA *gb_main) {
    char   *largest     = 0;
    long    maxnodes    = 0;
    GBDATA *gb_treedata = GB_search(gb_main, "tree_data", GB_CREATE_CONTAINER);
    GBDATA *gb_tree;

    for (gb_tree = GB_child(gb_treedata);
         gb_tree;
         gb_tree = GB_nextChild(gb_tree))
    {
        long *nnodes = GBT_read_int(gb_tree, "nnodes");
        if (nnodes && *nnodes>maxnodes) {
            freeset(largest, GB_read_key(gb_tree));
            maxnodes = *nnodes;
        }
    }
    return largest;
}

char *GBT_find_latest_tree(GBDATA *gb_main) {
    char **names = GBT_get_tree_names(gb_main);
    char *name = 0;
    char **pname;
    if (!names) return NULL;
    for (pname = names; *pname; pname++) name = *pname;
    if (name) name = strdup(name);
    GBT_free_names(names);
    return name;
}

const char *GBT_tree_info_string(GBDATA *gb_main, const char *tree_name, int maxTreeNameLen) {
    // maxTreeNameLen shall be the max len of the longest tree name (or -1 -> do not format)

    const char *result  = 0;
    GBDATA     *gb_tree = GBT_get_tree(gb_main, tree_name);

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

GB_ERROR GBT_check_tree_name(const char *tree_name)
{
    GB_ERROR error;
    if ((error = GB_check_key(tree_name))) return error;
    if (strncmp(tree_name, "tree_", 5)) {
        return GB_export_errorf("your treename '%s' does not begin with 'tree_'", tree_name);
    }
    return 0;
}

char **GBT_get_tree_names_and_count(GBDATA *Main, int *countPtr) {
    // returns an null terminated array of string pointers

    int      count       = 0;
    GBDATA  *gb_treedata = GB_entry(Main, "tree_data");
    char   **erg         = 0;

    if (gb_treedata) {
        GBDATA *gb_tree;
        count = 0;

        for (gb_tree = GB_child(gb_treedata);
             gb_tree;
             gb_tree = GB_nextChild(gb_tree))
        {
            count ++;
        }

        if (count) {
            erg   = (char **)GB_calloc(sizeof(char *), (size_t)count+1);
            count = 0;

            for (gb_tree = GB_child(gb_treedata);
                 gb_tree;
                 gb_tree = GB_nextChild(gb_tree))
            {
                erg[count] = GB_read_key(gb_tree);
                count ++;
            }
        }
    }

    *countPtr = count;
    return erg;
}

char **GBT_get_tree_names(GBDATA *Main) {
    int dummy;
    return GBT_get_tree_names_and_count(Main, &dummy);
}

char *GBT_get_name_of_next_tree(GBDATA *gb_main, const char *tree_name) {
    /* returns a heap-copy of the name of the next tree behind 'tree_name'.
     * If 'tree_name' is NULL or points to the last tree, returns the first tree.
     * If no tree exists, returns NULL.
     */
    GBDATA *gb_tree = 0;

    if (tree_name) {
        gb_tree = GBT_get_tree(gb_main, tree_name);
        gb_tree = GB_nextChild(gb_tree);
    }
    if (!gb_tree) {
        GBDATA *gb_treedata = GB_search(gb_main, "tree_data", GB_CREATE_CONTAINER);
        gb_tree = GB_child(gb_treedata);
    }

    return gb_tree ? GB_read_key(gb_tree) : NULL;
}

static GB_CSTR *fill_species_name_array(GB_CSTR *current, const GBT_TREE *tree) {
    if (tree->is_leaf) {
        current[0] = tree->name;
        return current+1;
    }
    current = fill_species_name_array(current, tree->leftson);
    current = fill_species_name_array(current, tree->rightson);
    return current;
}

GB_CSTR *GBT_get_names_of_species_in_tree(const GBT_TREE *tree) {
    /* creates an array of all species names in a tree,
     * The names are not allocated (so they may change as side effect of renaming species) */

    size_t size = GBT_count_leafs(tree);
    GB_CSTR *result = (GB_CSTR *)GB_calloc(sizeof(char *), size + 1);
#if defined(DEBUG)
    GB_CSTR *check =
#endif // DEBUG
        fill_species_name_array(result, tree);
    gb_assert(check - size == result);
    return result;
}

char *GBT_existing_tree(GBDATA *gb_main, const char *tree_name) {
    // search for an existing or an alternate tree
    GBDATA *gb_tree     = 0;
    GBDATA *gb_treedata = GB_entry(gb_main, "tree_data");

    if (gb_treedata) {
        gb_tree = GB_entry(gb_treedata, tree_name);
        if (!gb_tree) gb_tree = GB_child(gb_treedata); // take any tree
    }

    return gb_tree ? GB_read_key(gb_tree) : NULL;
}

