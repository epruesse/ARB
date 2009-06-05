/* ============================================================ */
/*                                                              */
/*   File      : adtree.c                                       */
/*   Purpose   : tree functions                                 */
/*                                                              */
/*   Institute of Microbiology (Technical University Munich)    */
/*   www.arb-home.de                                            */
/*                                                              */
/* ============================================================ */

#include <stdlib.h>
#include <string.h>

#include <adlocal.h>
#include <arbdbt.h>

#define GBT_PUT_DATA 1
#define GBT_GET_SIZE 0

#define DEFAULT_LENGTH        0.1                   /* default length of tree-edge w/o given length */
#define DEFAULT_LENGTH_MARKER -1000.0               /* tree-edges w/o length are marked with this value during read and corrected in GBT_scale_tree */

/* ---------------------- */
/*      remove leafs      */


static GBT_TREE *fixDeletedSon(GBT_TREE *tree) {
    // fix tree after one son has been deleted
    // (Note: this function only works correct for trees with minimum element size!)
    GBT_TREE *delNode = tree;

    if (delNode->leftson) {
        ad_assert(!delNode->rightson);
        tree             = delNode->leftson;
        delNode->leftson = 0;
    }
    else {
        ad_assert(!delNode->leftson);
        ad_assert(delNode->rightson);
        
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

    delNode->is_leaf = GB_TRUE; // don't try recursive delete

    if (delNode->father) { // not root
        GBT_delete_tree(delNode);
    }
    else { // root node
        if (delNode->tree_is_one_piece_of_memory) {
            // dont change root -> copy instead
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
            GB_BOOL  deleteSelf = GB_FALSE;
            GBDATA  *gb_node;

            if (species_hash) {
                gb_node = (GBDATA*)GBS_read_hash(species_hash, tree->name);
                ad_assert(tree->gb_node == 0); // dont call linked tree with 'species_hash'!
            }
            else gb_node = tree->gb_node;

            if (gb_node) {
                if (mode & (GBT_REMOVE_MARKED|GBT_REMOVE_NOT_MARKED)) {
                    long flag = GB_read_flag(gb_node);
                    deleteSelf = (flag && (mode&GBT_REMOVE_MARKED)) || (!flag && (mode&GBT_REMOVE_NOT_MARKED));
                }
            }
            else { // zombie
                if (mode & GBT_REMOVE_DELETED) deleteSelf = GB_TRUE;
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
            tree->is_leaf = GB_TRUE;
            GBT_delete_tree(tree);
            tree          = 0;
        }
    }

    return tree;
}

/* ------------------- */
/*      free tree      */


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


/********************************************************************************************
                    some tree write functions
********************************************************************************************/


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
        GB_BOOL node_is_used = GB_FALSE;

        if (node->name && node->name[0]) {
            if (!node->gb_node) {
                node->gb_node = GB_create_container(gb_tree, "node");
                if (!node->gb_node) error = GB_await_error();
            }
            if (!error) {
                GBDATA *gb_name     = GB_search(node->gb_node,"group_name",GB_STRING);
                if (!gb_name) error = GB_await_error();
                else    error       = GBT_write_group_name(gb_name, node->name);

                node_is_used = GB_TRUE; // wrote groupname -> node is used
            }
        }

        if (node->gb_node && !error) {
            if (!node_is_used) {
                GBDATA *gb_nonid = GB_child(node->gb_node);
                while (gb_nonid && strcmp("id", GB_read_key_pntr(gb_nonid)) == 0) {
                    gb_nonid = GB_nextChild(gb_nonid);
                }
                if (gb_nonid) node_is_used = GB_TRUE; // found child that is not "id" -> node is used
            }

            if (node_is_used) { // set id for used nodes
                error             = GBT_write_int(node->gb_node, "id", *startid);
                if (!error) error = GB_write_usr_private(node->gb_node,0);
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

#if 0
static long gbt_write_tree_nodes_old(GBDATA *gb_tree, GBT_TREE *node, long startid) {
    long me;
    GB_ERROR error;
    const char *key;
    GBDATA *gb_id,*gb_name,*gb_any;
    if (node->is_leaf) return 0;
    me = startid;
    if (node->name && (strlen(node->name)) ) {
        if (!node->gb_node) {
            node->gb_node = GB_create_container(gb_tree,"node");
        }
        gb_name = GB_search(node->gb_node,"group_name",GB_STRING);
        error = GBT_write_group_name(gb_name, node->name);
        if (error) return -1;
    }
    if (node->gb_node) {         /* delete not used nodes else write id */
        gb_any = GB_child(node->gb_node);
        if (gb_any) {
            key = GB_read_key_pntr(gb_any);
            if (strcmp(key,"id") == 0) {
                gb_any = GB_nextChild(gb_any);
            }
        }

        if (gb_any){
            gb_id = GB_search(node->gb_node,"id",GB_INT);
#if defined(DEBUG) && defined(DEVEL_RALF)
            {
                int old = GB_read_int(gb_id);
                if (old != me) {
                    printf("id changed in gbt_write_tree_nodes(): old=%i new=%li (tree-node=%p; gb_node=%p)\n",
                           old, me, node, node->gb_node);
                }
            }
#endif /* DEBUG */
            error = GB_write_int(gb_id,me);
            GB_write_usr_private(node->gb_node,0);
            if (error) return -1;
        }
        else {
#if defined(DEBUG) && defined(DEVEL_RALF)
            {
                GBDATA *gb_id2 = GB_entry(node->gb_node, "id");
                int     id     = 0;
                if (gb_id2) id = GB_read_int(gb_id2);

                printf("deleting node w/o info: tree-node=%p; gb_node=%p prev.id=%i\n",
                       node, node->gb_node, id);
            }
#endif /* DEBUG */
            GB_delete(node->gb_node);
            node->gb_node = 0;
        }
    }
    startid++;
    if (!node->leftson->is_leaf) {
        startid = gbt_write_tree_nodes(gb_tree,node->leftson,startid);
        if (startid<0) return startid;
    }

    if (!node->rightson->is_leaf) {
        startid = gbt_write_tree_nodes(gb_tree,node->rightson,startid);
        if (startid<0) return startid;
    }
    return startid;
}
#endif

static char *gbt_write_tree_rek_new(GBT_TREE *node, char *dest, long mode) {
    char buffer[40];        /* just real numbers */
    char    *c1;

    if ( (c1 = node->remark_branch) ) {
        int c;
        if (mode == GBT_PUT_DATA) {
            *(dest++) = 'R';
            while ( (c= *(c1++))  ) {
                if (c == 1) continue;
                *(dest++) = c;
            }
            *(dest++) = 1;
        }else{
            dest += strlen(c1) + 2;
        }
    }

    if (node->is_leaf){
        if (mode == GBT_PUT_DATA) {
            *(dest++) = 'L';
            if (node->name) strcpy(dest,node->name);
            while ( (c1= (char *)strchr(dest,1)) ) *c1 = 2;
            dest += strlen(dest);
            *(dest++) = 1;
            return dest;
        }else{
            if (node->name) return dest+1+strlen(node->name)+1; /* N name term */
            return dest+1+1;
        }
    }else{
        sprintf(buffer,"%g,%g;",node->leftlen,node->rightlen);
        if (mode == GBT_PUT_DATA) {
            *(dest++) = 'N';
            strcpy(dest,buffer);
            dest += strlen(buffer);
        }else{
            dest += strlen(buffer)+1;
        }
        dest = gbt_write_tree_rek_new(node->leftson,  dest, mode);
        dest = gbt_write_tree_rek_new(node->rightson, dest, mode);
        return dest;
    }
}

static GB_ERROR gbt_write_tree(GBDATA *gb_main, GBDATA *gb_tree, const char *tree_name, GBT_TREE *tree, int plain_only) {
    /* writes a tree to the database.

    If tree is loaded by function GBT_read_tree(..) then 'tree_name' should be zero !!!!!!
    else 'gb_tree' should be set to zero.

    to copy a tree call GB_copy((GBDATA *)dest,(GBDATA *)source);
    or set recursively all tree->gb_node variables to zero (that unlinks the tree),

    if 'plain_only' == 1 only the plain tree string is written

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

        ad_assert(gb_tree || error);

        if (!error) {
            if (!plain_only) {
                // mark all old style tree data for deletion
                GBDATA *gb_node;
                for (gb_node = GB_entry(gb_tree,"node"); gb_node; gb_node = GB_nextEntry(gb_node)) {
                    GB_write_usr_private(gb_node,1);
                }
            }

            // build tree-string and save to DB
            {
                char *t_size = gbt_write_tree_rek_new(tree, 0, GBT_GET_SIZE); // calc size of tree-string
                char *ctree  = (char *)GB_calloc(sizeof(char),(size_t)(t_size+1)); // allocate buffer for tree-string

                t_size = gbt_write_tree_rek_new(tree, ctree, GBT_PUT_DATA); // write into buffer
                *(t_size) = 0;

                error = GB_set_compression(gb_main,0); // no more compressions 
                if (!error) {
                    error        = GBT_write_string(gb_tree, "tree", ctree);
                    GB_ERROR err = GB_set_compression(gb_main,-1); // again allow all types of compression 
                    if (!error) error = err;
                }
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

                for (gb_node = GB_entry(gb_tree,"node"); // delete all ghost nodes
                     gb_node && !error;
                     gb_node = gb_node_next)
                {
                    GBDATA *gbd = GB_entry(gb_node,"id");
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

GB_ERROR GBT_write_tree_rem(GBDATA *gb_main,const char *tree_name, const char *remark) {
    return GBT_write_string(GBT_get_tree(gb_main, tree_name), "remark", remark);
}

/********************************************************************************************
                    some tree read functions
********************************************************************************************/

GBT_TREE *gbt_read_tree_rek(char **data, long *startid, GBDATA **gb_tree_nodes, long structure_size, int size_of_tree, GB_ERROR *error)
{
    GBT_TREE    *node;
    GBDATA      *gb_group_name;
    char         c;
    char        *p1;
    static char *membase;

    gb_assert(error);
    if (*error) return NULL;

    if (structure_size>0){
        node = (GBT_TREE *)GB_calloc(1,(size_t)structure_size);
    }
    else {
        if (!startid[0]){
            membase =(char *)GB_calloc(size_of_tree+1,(size_t)(-2*structure_size)); /* because of inner nodes */
        }
        node = (GBT_TREE *)membase;
        node->tree_is_one_piece_of_memory = 1;
        membase -= structure_size;
    }

    c = *((*data)++);

    if (c=='R') {
        p1 = strchr(*data,1);
        *(p1++) = 0;
        node->remark_branch = strdup(*data);
        c = *(p1++);
        *data = p1;
    }


    if (c=='N') {
        p1 = (char *)strchr(*data,',');
        *(p1++) = 0;
        node->leftlen = GB_atof(*data);
        *data = p1;
        p1 = (char *)strchr(*data,';');
        *(p1++) = 0;
        node->rightlen = GB_atof(*data);
        *data = p1;
        if ((*startid < size_of_tree) && (node->gb_node = gb_tree_nodes[*startid])){
            gb_group_name = GB_entry(node->gb_node,"group_name");
            if (gb_group_name) {
                node->name = GB_read_string(gb_group_name);
            }
        }
        (*startid)++;
        node->leftson = gbt_read_tree_rek(data,startid,gb_tree_nodes,structure_size,size_of_tree, error);
        if (!node->leftson) {
            if (!node->tree_is_one_piece_of_memory) free((char *)node);
            return NULL;
        }
        node->rightson = gbt_read_tree_rek(data,startid,gb_tree_nodes,structure_size,size_of_tree, error);
        if (!node->rightson) {
            if (!node->tree_is_one_piece_of_memory) free((char *)node);
            return NULL;
        }
        node->leftson->father = node;
        node->rightson->father = node;
    }
    else if (c=='L') {
        node->is_leaf = GB_TRUE;
        p1            = (char *)strchr(*data,1);

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
        /* GB_internal_error("Error reading tree 362436"); */
        return NULL;
    }
    return node;
}


/** Loads a tree from the database into any user defined structure.
    make sure that the first eight members members of your
    structure looks exectly like GBT_TREE, You should send the size
    of your structure ( minimum sizeof GBT_TREE) to this
    function.

    If size < 0 then the tree is allocated as just one big piece of memery,
    which can be freed by free((char *)root_of_tree) + deleting names or
    by GBT_delete_tree.

    tree_name is the name of the tree in the db
    return NULL if any error occur
*/

static GBT_TREE *read_tree_and_size_internal(GBDATA *gb_tree, GBDATA *gb_ctree, int structure_size, int size, GB_ERROR *error) {
    GBDATA   **gb_tree_nodes;
    GBT_TREE  *node = 0;

    gb_tree_nodes = (GBDATA **)GB_calloc(sizeof(GBDATA *),(size_t)size);
    if (gb_tree) {
        GBDATA *gb_node;

        for (gb_node = GB_entry(gb_tree,"node"); gb_node && !*error; gb_node = GB_nextEntry(gb_node)) {
            long    i;
            GBDATA *gbd = GB_entry(gb_node,"id");
            if (!gbd) continue;

            /*{ GB_export_error("ERROR while reading tree '%s' 4634",tree_name);return NULL;}*/
            i = GB_read_int(gbd);
            if ( i<0 || i>= size ) {
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
        node       = gbt_read_tree_rek(cptr, startid, gb_tree_nodes, structure_size,(int)size, error);
        free (fbuf);
    }

    free((char *)gb_tree_nodes);

    return node;
}

GBT_TREE *GBT_read_tree_and_size(GBDATA *gb_main,const char *tree_name, long structure_size, int *tree_size) {
    /* Read a tree from DB.
     * In case of failure, return NULL (and export error) 
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
                        else { /* "new" style tree */
                            GBT_TREE *tree = read_tree_and_size_internal(gb_tree, gb_ctree, structure_size, size, &error);
                            if (!error) {
                                gb_assert(tree);
                                if (tree_size) *tree_size = size; /* return size of tree */
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
    GB_export_error("Couldn't read tree '%s' (Reason: %s)", tree_name, error);
    return NULL;
}

GBT_TREE *GBT_read_tree(GBDATA *gb_main,const char *tree_name, long structure_size) {
    return GBT_read_tree_and_size(gb_main, tree_name, structure_size, 0);
}

GBT_TREE *GBT_read_plain_tree(GBDATA *gb_main, GBDATA *gb_ctree, long structure_size, GB_ERROR *error) {
    GBT_TREE *t;

    gb_assert(error && !*error); /* expect cleared error*/

    GB_push_transaction(gb_main);
    t = read_tree_and_size_internal(0, gb_ctree, structure_size, 0, error);
    GB_pop_transaction(gb_main);

    return t;
}

/********************************************************************************************
                    link the tree tips to the database
********************************************************************************************/
long GBT_count_nodes(GBT_TREE *tree){
    if ( tree->is_leaf ) {
        return 1;
    }
    return GBT_count_nodes(tree->leftson) + GBT_count_nodes(tree->rightson);
}

struct link_tree_data {
    GB_HASH *species_hash;
    GB_HASH *seen_species;      /* used to count duplicates */
    int      nodes; /* if != 0 -> update status */;
    int      counter;
    int      zombies;           /* counts zombies */
    int      duplicates;        /* counts duplicates */
};

static GB_ERROR gbt_link_tree_to_hash_rek(GBT_TREE *tree, struct link_tree_data *ltd) {
    GB_ERROR error = 0;
    if (tree->is_leaf) {
        if (ltd->nodes) { /* update status? */
            GB_status(ltd->counter/(double)ltd->nodes);
            ltd->counter++;
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

GB_ERROR GBT_link_tree_using_species_hash(GBT_TREE *tree, GB_BOOL show_status, GB_HASH *species_hash, int *zombies, int *duplicates) {
    GB_ERROR              error;
    struct link_tree_data ltd;
    long                  leafs = 0;

    if (duplicates || show_status) {
        leafs = GBT_count_nodes(tree);
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

/** Link a given tree to the database. That means that for all tips the member
    gb_node is set to the database container holding the species data.
    returns the number of zombies and duplicates in 'zombies' and 'duplicates'
*/
GB_ERROR GBT_link_tree(GBT_TREE *tree,GBDATA *gb_main,GB_BOOL show_status, int *zombies, int *duplicates)
{
    GB_HASH  *species_hash = GBT_create_species_hash(gb_main);
    GB_ERROR  error        = GBT_link_tree_using_species_hash(tree, show_status, species_hash, zombies, duplicates);

    GBS_free_hash(species_hash);

    return error;
}

/** Unlink a given tree from the database.
 */
void GBT_unlink_tree(GBT_TREE *tree)
{
    tree->gb_node = 0;
    if (!tree->is_leaf) {
        GBT_unlink_tree(tree->leftson);
        GBT_unlink_tree(tree->rightson);
    }
}


/********************************************************************************************
                    load a tree from file system
********************************************************************************************/


/* -------------------- */
/*      TreeReader      */
/* -------------------- */

enum tr_lfmode { LF_UNKNOWN, LF_N, LF_R, LF_NR, LF_RN, };

typedef struct {
    char                 *tree_file_name;
    FILE                 *in;
    int                   last_character; // may be EOF
    int                   line_cnt;
    struct GBS_strstruct *tree_comment;
    double                max_found_branchlen;
    double                max_found_bootstrap;
    GB_ERROR              error;
    enum tr_lfmode        lfmode;
} TreeReader;

static char gbt_getc(TreeReader *reader) {
    // reads character from stream (convert)
    // when linefeeds are completely broken, 
    char c   = getc(reader->in);
    int  inc = 0;

    if (c == '\n') {
        switch (reader->lfmode) {
            case LF_UNKNOWN: reader->lfmode = LF_N; inc = 1; break;
            case LF_N:       inc = 1; break;
            case LF_R:       reader->lfmode = LF_RN; c = gbt_getc(reader); break;
            case LF_NR:      c = gbt_getc(reader); break;
            case LF_RN:      inc = 1; break;
        }
    }
    else if (c == '\r') {
        switch (reader->lfmode) {
            case LF_UNKNOWN: reader->lfmode = LF_R; inc = 1; break;
            case LF_R:       inc = 1; break;
            case LF_N:       reader->lfmode = LF_NR; c = gbt_getc(reader); break; 
            case LF_RN:      c = gbt_getc(reader); break;
            case LF_NR:      inc = 1; break;
        }
        if (c == '\r') c = '\n';     // never report '\r'
    }
    if (inc) reader->line_cnt++;

    return c;
}

static char gbt_read_char(TreeReader *reader) {
    GB_BOOL done = GB_FALSE;
    int     c    = ' ';

    while (!done) {
        c = gbt_getc(reader);
        if (c == ' ' || c == '\t' || c == '\n') ; // skip
        else if (c == '[') {    // collect tree comment(s)
            if (reader->tree_comment != 0) {
                // not first comment -> do new line
                GBS_chrcat(reader->tree_comment, '\n');
            }
            c = getc(reader->in);
            while (c != ']' && c != EOF) {
                GBS_chrcat(reader->tree_comment, c);
                c = gbt_getc(reader);
            }
        }
        else done = GB_TRUE;
    }

    reader->last_character = c;
    return c;
}

static char gbt_get_char(TreeReader *reader) {
    int c = gbt_getc(reader);
    reader->last_character = c;
    return c;
}

static TreeReader *newTreeReader(FILE *input, const char *file_name) {
    TreeReader *reader = GB_calloc(1, sizeof(*reader));

    reader->tree_file_name      = strdup(file_name);
    reader->in                  = input;
    reader->tree_comment        = GBS_stropen(2048);
    reader->max_found_branchlen = -1;
    reader->max_found_bootstrap = -1;
    reader->line_cnt            = 1;
    reader->lfmode              = LF_UNKNOWN;

    gbt_read_char(reader);

    return reader;
}

static void freeTreeReader(TreeReader *reader) {
    free(reader->tree_file_name);
    if (reader->tree_comment) GBS_strforget(reader->tree_comment);
    free(reader);
}

static void setReaderError(TreeReader *reader, const char *message) {
    
    reader->error = GBS_global_string("Error reading %s:%i: %s",
                                      reader->tree_file_name,
                                      reader->line_cnt,
                                      message);
}

static char *getTreeComment(TreeReader *reader) {
    /* can only be called once. Deletes the comment from TreeReader! */
    char *comment = 0;
    if (reader->tree_comment) {
        comment = GBS_strclose(reader->tree_comment);
        reader->tree_comment = 0;
    }
    return comment;
}


/* ------------------------------------------------------------
 * The following functions assume that the "current" character
 * has already been read into 'TreeReader->last_character'
 */

static void gbt_eat_white(TreeReader *reader) {
    int c = reader->last_character;
    while ((c == ' ') || (c == '\n') || (c == '\r') || (c == '\t')){
        c = gbt_get_char(reader);
    }
}

static double gbt_read_number(TreeReader *reader) {
    char    strng[256];
    char   *s = strng;
    int     c = reader->last_character;
    double  fl;

    while (((c<='9') && (c>='0')) || (c=='.') || (c=='-') || (c=='e') || (c=='E')) {
        *(s++) = c;
        c = gbt_get_char(reader);
    }
    *s = 0;
    fl = GB_atof(strng);

    gbt_eat_white(reader);

    return fl;
}

static char *gbt_read_quoted_string(TreeReader *reader){
    /* Read in a quoted or unquoted string.
     * in quoted strings double quotes ('') are replaced by (')
     */

    char  buffer[1024];
    char *s = buffer;
    int   c = reader->last_character;

    if (c == '\'') {
        c = gbt_get_char(reader);
        while ( (c!= EOF) && (c!='\'') ) {
        gbt_lt_double_quot:
            *(s++) = c;
            if ((s-buffer) > 1000) {
                *s = 0;
                setReaderError(reader, GBS_global_string("Name '%s' longer than 1000 bytes", buffer));
                return NULL;
            }
            c = gbt_get_char(reader);
        }
        if (c == '\'') {
            c = gbt_read_char(reader);
            if (c == '\'') goto gbt_lt_double_quot;
        }
    }else{
        while ( c== '_') c = gbt_read_char(reader);
        while ( c== ' ') c = gbt_read_char(reader);
        while ( (c != ':') && (c!= EOF) && (c!=',') &&
                (c!=';') && (c!= ')') )
        {
            *(s++) = c;
            if ((s-buffer) > 1000) {
                *s = 0;
                setReaderError(reader, GBS_global_string("Name '%s' longer than 1000 bytes", buffer));
                return NULL;
            }
            c = gbt_read_char(reader);
        }
    }
    *s = 0;
    return strdup(buffer);
}

static void setBranchName(TreeReader *reader, GBT_TREE *node, char *name) {
    /* detect bootstrap values */
    /* name has to be stored in node or must be free'ed */
    
    char   *end       = 0;
    double  bootstrap = strtod(name, &end);

    if (end == name) {          // no digits -> no bootstrap
        node->name = name;
    }
    else {
        bootstrap = bootstrap*100.0 + 0.5; // needed if bootstrap values are between 0.0 and 1.0 */
        // downscaling in done later!

        if (bootstrap>reader->max_found_bootstrap) {
            reader->max_found_bootstrap = bootstrap;
        }

        ad_assert(node->remark_branch == 0);
        node->remark_branch  = GBS_global_string_copy("%i%%", (int)bootstrap);

        if (end[0] != 0) {      // sth behind bootstrap value
            if (end[0] == ':') ++end; // ARB format for nodes with bootstraps AND node name is 'bootstrap:nodename'
            node->name = strdup(end);
        }
        free(name);
    }
}

static GB_BOOL gbt_readNameAndLength(TreeReader *reader, GBT_TREE *node, GBT_LEN *len) {
    /* reads the branch-length and -name
       '*len' should normally be initialized with DEFAULT_LENGTH_MARKER 
     * returns the branch-length in 'len' and sets the branch-name of 'node'
     * returns GB_TRUE if successful, otherwise reader->error gets set
     */

    GB_BOOL done = GB_FALSE;
    while (!done && !reader->error) {
        switch (reader->last_character) {
            case ';':
            case ',':
            case ')':
                done = GB_TRUE;
                break;
            case ':':
                gbt_read_char(reader);      /* drop ':' */
                *len = gbt_read_number(reader);
                if (*len>reader->max_found_branchlen) {
                    reader->max_found_branchlen = *len;
                }
                break;
            default: {
                char *branchName = gbt_read_quoted_string(reader);
                if (branchName) {
                    setBranchName(reader, node, branchName);
                }
                else {
                    setReaderError(reader, "Expected branch-name or one of ':;,)'");
                }
                break;
            }
        }
    }

    return !reader->error;
}

static GBT_TREE *gbt_linkedTreeNode(GBT_TREE *left, GBT_LEN leftlen, GBT_TREE *right, GBT_LEN rightlen, int structuresize) {
    GBT_TREE *node = (GBT_TREE*)GB_calloc(1, structuresize);
                                
    node->leftson  = left;
    node->leftlen  = leftlen;
    node->rightson = right;
    node->rightlen = rightlen;

    left->father  = node;
    right->father = node;

    return node;
}

static GBT_TREE *gbt_load_tree_rek(TreeReader *reader, int structuresize, GBT_LEN *nodeLen) {
    GBT_TREE *node = 0;

    if (reader->last_character == '(') {
        gbt_read_char(reader);  // drop the '('

        GBT_LEN   leftLen = DEFAULT_LENGTH_MARKER;
        GBT_TREE *left    = gbt_load_tree_rek(reader, structuresize, &leftLen);

        ad_assert(left || reader->error);

        if (left) {
            if (gbt_readNameAndLength(reader, left, &leftLen)) {
                switch (reader->last_character) {
                    case ')':               /* single node */
                        *nodeLen = leftLen;
                        node     = left;
                        left     = 0;
                        break;

                    case ',': {
                        GBT_LEN   rightLen = DEFAULT_LENGTH_MARKER;
                        GBT_TREE *right    = 0;

                        while (reader->last_character == ',' && !reader->error) {
                            if (right) { /* multi-branch */
                                GBT_TREE *pair = gbt_linkedTreeNode(left, leftLen, right, rightLen, structuresize);
                                
                                left  = pair; leftLen = 0;
                                right = 0; rightLen = DEFAULT_LENGTH_MARKER;
                            }

                            gbt_read_char(reader); /* drop ',' */
                            right = gbt_load_tree_rek(reader, structuresize, &rightLen);
                            if (right) gbt_readNameAndLength(reader, right, &rightLen);
                        }

                        if (reader->last_character == ')') {
                            node     = gbt_linkedTreeNode(left, leftLen, right, rightLen, structuresize);
                            *nodeLen = DEFAULT_LENGTH_MARKER;

                            left = 0;
                            right  = 0;

                            gbt_read_char(reader); /* drop ')' */
                        }
                        else {
                            setReaderError(reader, "Expected one of ',)'");
                        }
                        
                        free(right);

                        break;
                    }

                    default:
                        setReaderError(reader, "Expected one of ',)'");
                        break;
                }
            }
            else {
                ad_assert(reader->error);
            }

            free(left);
        }
    }
    else {                      /* single node */
        gbt_eat_white(reader);
        char *name = gbt_read_quoted_string(reader);
        if (name) {
            node          = (GBT_TREE*)GB_calloc(1, structuresize);
            node->is_leaf = GB_TRUE;
            node->name    = name;
        }
        else {
            setReaderError(reader, "Expected quoted string");
        }
    }

    ad_assert(node || reader->error);
    return node;
}



void GBT_scale_tree(GBT_TREE *tree, double length_scale, double bootstrap_scale) {
    if (tree->leftson) {
        if (tree->leftlen <= DEFAULT_LENGTH_MARKER) tree->leftlen  = DEFAULT_LENGTH;
        else                                        tree->leftlen *= length_scale;
        
        GBT_scale_tree(tree->leftson, length_scale, bootstrap_scale);
    }
    if (tree->rightson) {
        if (tree->rightlen <= DEFAULT_LENGTH_MARKER) tree->rightlen  = DEFAULT_LENGTH; 
        else                                         tree->rightlen *= length_scale;
        
        GBT_scale_tree(tree->rightson, length_scale, bootstrap_scale);
    }

    if (tree->remark_branch) {
        const char *end          = 0;
        double      bootstrap    = strtod(tree->remark_branch, (char**)&end);
        GB_BOOL     is_bootstrap = end[0] == '%' && end[1] == 0;

        freeset(tree->remark_branch, 0);

        if (is_bootstrap) {
            bootstrap = bootstrap*bootstrap_scale+0.5;
            tree->remark_branch  = GBS_global_string_copy("%i%%", (int)bootstrap);
        }
    }
}

char *GBT_newick_comment(const char *comment, GB_BOOL escape) {
    /* escape or unescape a newick tree comment
     * (']' is not allowed there)
     */

    char *result = 0;
    if (escape) {
        result = GBS_string_eval(comment, "\\\\=\\\\\\\\:[=\\\\{:]=\\\\}", NULL); // \\\\ has to be first!
    }
    else {
        result = GBS_string_eval(comment, "\\\\}=]:\\\\{=[:\\\\\\\\=\\\\", NULL); // \\\\ has to be last!
    }
    return result;
}

GBT_TREE *GBT_load_tree(const char *path, int structuresize, char **commentPtr, int allow_length_scaling, char **warningPtr) {
    /* Load a newick compatible tree from file 'path',
       structure size should be >0, see GBT_read_tree for more information
       if commentPtr != NULL -> set it to a malloc copy of all concatenated comments found in tree file
       if warningPtr != NULL -> set it to a malloc copy auto-scale-warning (if autoscaling happens)
    */

    GBT_TREE *tree  = 0;
    FILE     *input = fopen(path, "rt");
    GB_ERROR  error = 0;

    if (!input) {
        error = GBS_global_string("No such file: %s", path);
    }
    else {
        const char *name_only = strrchr(path, '/');
        if (name_only) ++name_only;
        else        name_only = path;

        TreeReader *reader      = newTreeReader(input, name_only);
        GBT_LEN     rootNodeLen = DEFAULT_LENGTH_MARKER; /* root node has no length. only used as input to gbt_load_tree_rek*/
        tree                    = gbt_load_tree_rek(reader, structuresize, &rootNodeLen);
        fclose(input);

        if (tree) {
            double bootstrap_scale = 1.0;
            double branchlen_scale = 1.0;

            if (reader->max_found_bootstrap >= 101.0) { // bootstrap values were given in percent
                bootstrap_scale = 0.01;
                if (warningPtr) {
                    *warningPtr = GBS_global_string_copy("Auto-scaling bootstrap values by factor %.2f (max. found bootstrap was %5.2f)",
                                                         bootstrap_scale, reader->max_found_bootstrap);
                }
            }
            if (reader->max_found_branchlen >= 1.01) { // assume branchlengths have range [0;100]
                if (allow_length_scaling) {
                    branchlen_scale = 0.01;
                    if (warningPtr) {
                        char *w = GBS_global_string_copy("Auto-scaling branchlengths by factor %.2f (max. found branchlength = %5.2f)",
                                                         branchlen_scale, reader->max_found_branchlen);
                        if (*warningPtr) {
                            char *w2 = GBS_global_string_copy("%s\n%s", *warningPtr, w);

                            free(*warningPtr);
                            free(w);
                            *warningPtr = w2;
                        }
                        else {
                            *warningPtr = w;
                        }
                    }
                }
            }

            GBT_scale_tree(tree, branchlen_scale, bootstrap_scale); // scale bootstraps and branchlengths

            if (commentPtr) {
                char *comment = getTreeComment(reader);

                ad_assert(*commentPtr == 0);
                if (comment && comment[0]) {
                    char *unescaped = GBT_newick_comment(comment, GB_FALSE);
                    *commentPtr     = unescaped;
                }
                free(comment);
            }
        }

        freeTreeReader(reader);
    }

    ad_assert(tree||error);
    if (error) {
        GB_export_error("Import tree: %s", error);
        ad_assert(!tree);
    }

    return tree;
}


GBDATA *GBT_get_tree(GBDATA *gb_main, const char *tree_name) {
    /* returns the datapntr to the database structure, which is the container for the tree */
    GBDATA *gb_treedata = GB_search(gb_main,"tree_data",GB_CREATE_CONTAINER);
    return GB_entry(gb_treedata, tree_name);
}

long GBT_size_of_tree(GBDATA *gb_main, const char *tree_name) {
    GBDATA *gb_tree = GBT_get_tree(gb_main,tree_name);
    GBDATA *gb_nnodes;
    if (!gb_tree) return -1;
    gb_nnodes = GB_entry(gb_tree,"nnodes");
    if (!gb_nnodes) return -1;
    return GB_read_int(gb_nnodes);
}

char *GBT_find_largest_tree(GBDATA *gb_main){
    char   *largest     = 0;
    long    maxnodes    = 0;
    GBDATA *gb_treedata = GB_search(gb_main,"tree_data",GB_CREATE_CONTAINER);
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

char *GBT_find_latest_tree(GBDATA *gb_main){
    char **names = GBT_get_tree_names(gb_main);
    char *name = 0;
    char **pname;
    if (!names) return NULL;
    for (pname = names;*pname;pname++) name = *pname;
    if (name) name = strdup(name);
    GBT_free_names(names);
    return name;
}

const char *GBT_tree_info_string(GBDATA *gb_main, const char *tree_name, int maxTreeNameLen) {
    /* maxTreeNameLen shall be the max len of the longest tree name (or -1 -> do not format) */

    const char *result  = 0;
    GBDATA     *gb_tree = GBT_get_tree(gb_main,tree_name);
    
    if (!gb_tree) {
        GB_export_error("tree '%s' not found",tree_name);
    }
    else {
        GBDATA *gb_nnodes = GB_entry(gb_tree,"nnodes");
        if (!gb_nnodes) {
            GB_export_error("nnodes not found in tree '%s'",tree_name);
        }
        else {
            const char *sizeInfo = GBS_global_string("(%li:%i)", GB_read_int(gb_nnodes)+1, GB_read_security_write(gb_tree));
            GBDATA     *gb_rem   = GB_entry(gb_tree,"remark");
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
    if ( (error = GB_check_key(tree_name)) ) return error;
    if (strncmp(tree_name,"tree_",5)){
        return GB_export_error("your treename '%s' does not begin with 'tree_'",tree_name);
    }
    return 0;
}

char **GBT_get_tree_names_and_count(GBDATA *Main, int *countPtr){
    /* returns an null terminated array of string pointers */

    int      count       = 0;
    GBDATA  *gb_treedata = GB_entry(Main,"tree_data");
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
            erg   = (char **)GB_calloc(sizeof(char *),(size_t)count+1);
            count = 0;

            for (gb_tree = GB_child(gb_treedata);
                 gb_tree;
                 gb_tree = GB_nextChild(gb_tree) )
            {
                erg[count] = GB_read_key(gb_tree);
                count ++;
            }
        }
    }

    *countPtr = count;
    return erg;
}

char **GBT_get_tree_names(GBDATA *Main){
    int dummy;
    return GBT_get_tree_names_and_count(Main, &dummy);
}

char *GBT_get_next_tree_name(GBDATA *gb_main, const char *tree_name) {
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
        GBDATA *gb_treedata = GB_search(gb_main,"tree_data",GB_CREATE_CONTAINER);
        gb_tree = GB_child(gb_treedata);
    }

    return gb_tree ? GB_read_key(gb_tree) : NULL;
}

int gbt_sum_leafs(GBT_TREE *tree){
    if (tree->is_leaf){
        return 1;
    }
    return gbt_sum_leafs(tree->leftson) + gbt_sum_leafs(tree->rightson);
}

GB_CSTR *gbt_fill_species_names(GB_CSTR *des,GBT_TREE *tree) {
    if (tree->is_leaf){
        des[0] = tree->name;
        return des+1;
    }
    des = gbt_fill_species_names(des,tree->leftson);
    des = gbt_fill_species_names(des,tree->rightson);
    return des;
}

GB_CSTR *GBT_get_species_names_of_tree(GBT_TREE *tree){
    /* creates an array of all species names in a tree,
     * The names are not allocated (so they may change as side effect of renaming species) */

    int size = gbt_sum_leafs(tree);
    GB_CSTR *result = (GB_CSTR *)GB_calloc(sizeof(char *),size +1);
#if defined(DEBUG)
    GB_CSTR *check =
#endif // DEBUG
        gbt_fill_species_names(result,tree);
    ad_assert(check - size == result);
    return result;
}

char *GBT_existing_tree(GBDATA *gb_main, const char *tree_name) {
    /* search for an existing or an alternate tree */
    GBDATA *gb_tree     = 0;
    GBDATA *gb_treedata = GB_entry(gb_main,"tree_data");

    if (gb_treedata) {
        gb_tree = GB_entry(gb_treedata, tree_name);
        if (!gb_tree) gb_tree = GB_child(gb_treedata); // take any tree
    }

    return gb_tree ? GB_read_key(gb_tree) : NULL;
}

void gbt_export_tree_node_print_remove(char *str){
    int i,len;
    len = strlen (str);
    for(i=0;i<len;i++) {
        if (str[i] =='\'') str[i] ='.';
        if (str[i] =='\"') str[i] ='.';
    }
}

void gbt_export_tree_rek(GBT_TREE *tree,FILE *out){
    if (tree->is_leaf) {
        gbt_export_tree_node_print_remove(tree->name);
        fprintf(out," '%s' ",tree->name);
    }else{
        fprintf(out, "(");
        gbt_export_tree_rek(tree->leftson,out);
        fprintf(out, ":%.5f,", tree->leftlen);
        gbt_export_tree_rek(tree->rightson,out);
        fprintf(out, ":%.5f", tree->rightlen);
        fprintf(out, ")");
        if (tree->name){
            fprintf(out, "'%s'",tree->name);
        }
    }
}


GB_ERROR GBT_export_tree(GBDATA *gb_main,FILE *out,GBT_TREE *tree, GB_BOOL triple_root){
    GBUSE(gb_main);
    if(triple_root){
        GBT_TREE *one,*two,*three;
        if (tree->is_leaf){
            return GB_export_error("Tree is two small, minimum 3 nodes");
        }
        if (tree->leftson->is_leaf && tree->rightson->is_leaf){
            return GB_export_error("Tree is two small, minimum 3 nodes");
        }
        if (tree->leftson->is_leaf){
            one = tree->leftson;
            two = tree->rightson->leftson;
            three = tree->rightson->rightson;
        }else{
            one = tree->leftson->leftson;
            two = tree->leftson->rightson;
            three = tree->rightson;
        }
        fprintf(out, "(");
        gbt_export_tree_rek(one,out);
        fprintf(out, ":%.5f,", 1.0);
        gbt_export_tree_rek(two,out);
        fprintf(out, ":%.5f,", 1.0);
        gbt_export_tree_rek(three,out);
        fprintf(out, ":%.5f)", 1.0);
    }else{
        gbt_export_tree_rek(tree,out);
    }
    return 0;
}
