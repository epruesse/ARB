// =============================================================== //
//                                                                 //
//   File      : ad_core.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "gb_ts.h"
#include "gb_storage.h"
#include "gb_index.h"
#include "gb_localdata.h"

// Copy all info + external data mem to an one step undo buffer
// (needed to abort transactions)

inline void _GB_CHECK_IN_UNDO_DELETE(GB_MAIN_TYPE *Main, GBDATA *& gbd) {
    if (Main->undo_type) gb_check_in_undo_delete(Main, gbd, 0);
    else gb_delete_entry(&gbd);
}
inline void _GB_CHECK_IN_UNDO_CREATE(GB_MAIN_TYPE *Main, GBDATA *gbd) {
    if (Main->undo_type) gb_check_in_undo_create(Main, gbd);
}
inline void _GB_CHECK_IN_UNDO_MODIFY(GB_MAIN_TYPE *Main, GBDATA *gbd) {
    if (Main->undo_type) gb_check_in_undo_modify(Main, gbd);
}


// ---------------------------
//      GB data management

void gb_touch_entry(GBDATA * gbd, GB_CHANGE val) {
    GBCONTAINER *gbc;
    GBCONTAINER *gbc_father;

    gbd->flags2.update_in_server = 0;
    if (val > (GB_CHANGE)(int)GB_ARRAY_FLAGS(gbd).changed) {
        GB_ARRAY_FLAGS(gbd).changed = val;
        GB_ARRAY_FLAGS(gbd).ever_changed = 1;
    }
    gbc = GB_FATHER(gbd);

    if ((!gbc->index_of_touched_one_son) || gbc->index_of_touched_one_son == gbd->index+1) {
        gbc->index_of_touched_one_son = gbd->index+1;
    }
    else {
        gbc->index_of_touched_one_son = -1;
    }

    while ((gbc_father=GB_FATHER(gbc))!=NULL)
    {
        if ((!gbc_father->index_of_touched_one_son) || gbc_father->index_of_touched_one_son == gbc->index+1) {
            gbc_father->index_of_touched_one_son = gbc->index+1;
        }
        else {
            gbc_father->index_of_touched_one_son = -1;
        }

        if (gbc->flags2.update_in_server) {
            gbc->flags2.update_in_server = 0;
        }
        else {
            if (GB_ARRAY_FLAGS(gbc).changed >= GB_SON_CHANGED)
                return;
        }
        if (GB_ARRAY_FLAGS(gbc).changed < GB_SON_CHANGED) {
            GB_ARRAY_FLAGS(gbc).changed = GB_SON_CHANGED;
            GB_ARRAY_FLAGS(gbc).ever_changed = 1;
        }
        gbc = gbc_father;
    }
}

void gb_touch_header(GBCONTAINER *gbc) {
    gbc->flags2.header_changed = 1;
    gb_touch_entry((GBDATA*)gbc, GB_NORMAL_CHANGE);
}


void gb_untouch_children(GBCONTAINER * gbc) {
    GBDATA         *gbd;
    int             index, start, end;
    GB_CHANGE       changed;
    gb_header_list *header = GB_DATA_LIST_HEADER(gbc->d);

    if (gbc->index_of_touched_one_son > 0) {
        start = (int)gbc->index_of_touched_one_son-1;
        end = start + 1;
    }
    else {
        if (!gbc->index_of_touched_one_son) {
            start = end = 0;
        }
        else {
            start = 0;
            end = gbc->d.nheader;
        }
    }

    for (index = start; index < end; index++) {
        if ((gbd = GB_HEADER_LIST_GBD(header[index]))!=NULL)
        {
            changed = (GB_CHANGE)header[index].flags.changed;
            if (changed != GB_UNCHANGED && changed < GB_DELETED) {
                header[index].flags.changed = GB_UNCHANGED;
                if (GB_TYPE(gbd) == GB_DB)
                {
                    gb_untouch_children((GBCONTAINER *) gbd);
                }
            }
            gbd->flags2.update_in_server = 0;
        }
    }
    gbc->index_of_touched_one_son = 0;
}

void gb_untouch_me(GBDATA * gbc) {
    GB_DATA_LIST_HEADER(GB_FATHER(gbc)->d)[gbc->index].flags.changed = GB_UNCHANGED;
    if (GB_TYPE(gbc) == GB_DB) {
        gbc->flags2.header_changed = 0;
        ((GBCONTAINER *)gbc)->index_of_touched_one_son = 0;
    }
}

void gb_set_update_in_server_flags(GBCONTAINER * gbc) {
    int             index;
    GBDATA         *gbd;

    for (index = 0; index < gbc->d.nheader; index++) {
        if ((gbd = GBCONTAINER_ELEM(gbc, index))!=NULL) {
            if (GB_TYPE(gbd) == GB_DB) {
                gb_set_update_in_server_flags((GBCONTAINER *) gbd);
            }
            gbd->flags2.update_in_server = 1;
        }
    }
}

void gb_create_header_array(GBCONTAINER *gbc, int size) {
    // creates or resizes an old array to children
    gb_header_list *nl, *ol;

    if (size <= gbc->d.headermemsize) return;
    if (!size) return;
    if (size > 10) size++;
    if (size > 30) size = size*3/2;
    nl = (gb_header_list *)gbm_get_mem(sizeof(gb_header_list)*size, GBM_HEADER_INDEX);

    if ((ol=GB_DATA_LIST_HEADER(gbc->d))!=NULL)
    {
        int     idx,
            maxidx = gbc->d.headermemsize; // ???: oder ->d.nheader

        for (idx=0; idx<maxidx; idx++)
        {
            GBDATA *gbd = GB_HEADER_LIST_GBD(ol[idx]);
            nl[idx].flags =  ol[idx].flags;

            if (gbd)
            {
                gb_assert(gbd->server_id==GBTUM_MAGIC_NUMBER || GB_read_clients(gbd)<0); // or I am a client
                SET_GB_HEADER_LIST_GBD(nl[idx], gbd);
            }
        }

        gbm_free_mem(ol, sizeof(gb_header_list)*gbc->d.headermemsize, GBM_HEADER_INDEX);
    }

    gbc->d.headermemsize = size;
    SET_GB_DATA_LIST_HEADER(gbc->d, nl);
}

void gb_link_entry(GBCONTAINER* father, GBDATA * gbd, long index_pos)
{
    /* if index_pos == -1 -> to end of data;
       else special index position; error when data already exists in index pos */

    SET_GB_FATHER(gbd, father);
    if (father == NULL) {   // 'main' entry in GB
        return;
    }

    if (GB_TYPE(father) != GB_DB) {
        GB_internal_errorf("to read a database into a non database keyword %s,"
                           "probably %%%% is missing\n", GB_read_key_pntr((GBDATA*)father));
        return;
    }
    if (index_pos < 0) {
        index_pos = father->d.nheader++;
    }
    else {
        if (index_pos >= father->d.nheader) {
            father->d.nheader = (int)index_pos+1;
        }
    }

    gb_create_header_array(father, (int)index_pos+1);

    if (GBCONTAINER_ELEM(father, index_pos)) {
        GB_internal_error("Index of Databaseentry used twice");
        index_pos = father->d.nheader++;
        gb_create_header_array(father, (int)index_pos+1);
    }

    /* the following code skips just-deleted index position, while searching for an unused
       index position. I'm unsure whether this works w/o problems (ralf 2004-Oct-08) */

    while (GB_DATA_LIST_HEADER(father->d)[index_pos].flags.changed >= GB_DELETED) {
#if defined(DEBUG)
        fprintf(stderr, "Warning: index_pos %li of father(%p) contains just-deleted entry -> using next index_pos..\n", index_pos, father);
#endif // DEBUG
        index_pos = father->d.nheader++;
        gb_create_header_array(father, (int)index_pos+1);
    }

    gbd->index = index_pos;
    SET_GBCONTAINER_ELEM(father, index_pos, gbd);
    father->d.size++;
}

void gb_unlink_entry(GBDATA * gbd)
{
    GBCONTAINER *father = GB_FATHER(gbd);

    if (father)
    {
        int             index_pos = (int)gbd->index;
        gb_header_list *hls       = &(GB_DATA_LIST_HEADER(father->d)[index_pos]);

        SET_GB_HEADER_LIST_GBD(*hls, NULL);
        hls->flags.key_quark    = 0;
        hls->flags.changed      = GB_DELETED;
        hls->flags.ever_changed = 1;
        father->d.size--;
        SET_GB_FATHER(gbd, NULL);
    }
}

void gb_create_extended(GBDATA *gbd) {
    if (!gbd->ext) {
        int index = GB_GBM_INDEX(gbd);
        gbd->ext = (gb_db_extended *)gbm_get_mem(sizeof(gb_db_extended), index);
    }
}

GB_MAIN_TYPE *gb_make_gb_main_type(const char *path) {
    GB_MAIN_TYPE *Main;

    Main = (GB_MAIN_TYPE *)gbm_get_mem(sizeof(*Main), 0);
    if (path) Main->path   = strdup((char*)path);
    Main->key_2_index_hash = GBS_create_hash(ALLOWED_KEYS, GB_MIND_CASE);
    Main->compression_mask = -1;                    // allow all compressions
    gb_init_cache(Main);
    gb_init_undo_stack(Main);
    gb_init_ctype_table();
    gb_local->openedDBs++;
    return Main;
}

char *gb_destroy_main(GB_MAIN_TYPE *Main) {
    gb_assert(!Main->dummy_father);
    gb_assert(!Main->data);

    gb_destroy_cache(Main);
    gb_release_main_idx(Main);

    if (Main->command_hash) GBS_free_hash(Main->command_hash);

    gb_free_all_keys(Main);
    
    if (Main->key_2_index_hash) GBS_free_hash(Main->key_2_index_hash);
    freenull(Main->keys);

    gb_free_undo_stack(Main);

    for (int j = 0; j<ALLOWED_DATES; ++j) freenull(Main->dates[j]);

    free(Main->path);
    free(Main->qs.quick_save_disabled);

    gbm_free_mem(Main, sizeof(*Main), 0);

    gb_local->closedDBs++;
    if (gb_local->closedDBs == gb_local->openedDBs) {
        GB_exit_gb(); // free most memory allocated by ARBDB library
    }

    return 0;
}

GBDATA *gb_make_pre_defined_entry(GBCONTAINER *father, GBDATA *gbd, long index_pos, GBQUARK keyq) {
    // inserts an object into the dabase hierarchy
    GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(father);

    SET_GB_FATHER(gbd, father);
    if (Main->local_mode) {
        gbd->server_id = GBTUM_MAGIC_NUMBER;
    }
    if (Main->clock) {
        GB_CREATE_EXT(gbd);
        gbd->ext->creation_date = Main->clock;
    }

    gb_link_entry(father, gbd, index_pos);
    gb_write_index_key(father, gbd->index, keyq);

    return gbd;
}

void gb_rename_entry(GBCONTAINER *gbc, const char *new_key) {
    GBCONTAINER  *gb_father = GB_FATHER(gbc);
    GB_MAIN_TYPE *Main      = GBCONTAINER_MAIN(gb_father);
    GBQUARK       new_keyq;
    long          new_gbm_index;

    gb_unlink_entry((GBDATA*)gbc);

    new_keyq          = gb_key_2_quark(Main, new_key);
    new_gbm_index     = GB_QUARK_2_GBMINDEX(Main, new_keyq);
    GB_GBM_INDEX(gbc) = new_gbm_index;

    gb_link_entry(gb_father, (GBDATA*)gbc, -1);
    gb_write_key((GBDATA*)gbc, new_key);
}


GBDATA *gb_make_entry(GBCONTAINER * father, const char *key, long index_pos, GBQUARK keyq, GB_TYPES type) {
    // creates a terminal database object
    GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(father);

    if (!keyq) keyq = gb_key_2_quark(Main, key);

    long    gbm_index = GB_QUARK_2_GBMINDEX(Main, keyq);
    GBDATA *gbd       = (GBDATA *) gbm_get_mem(sizeof(GBDATA), gbm_index);

    GB_GBM_INDEX(gbd) = gbm_index;
    SET_GB_FATHER(gbd, father);

    switch (type) {
        case GB_STRING_SHRT:
            type = GB_STRING;
            // fall-through
        case GB_STRING:
            GB_SETSMDMALLOC(gbd, 6, 7, "<NONE>");
            break;
        case GB_LINK:
            GB_SETSMDMALLOC(gbd, 1, 2, ":");
            break;
        default:
            break;
    }
    gbd->flags.type = type;

    if (Main->local_mode) {
        gbd->server_id = GBTUM_MAGIC_NUMBER;
    }
    if (Main->clock) {
        GB_CREATE_EXT(gbd);
        gbd->ext->creation_date = Main->clock;
    }

    gb_link_entry(father, gbd, index_pos);
    if (key)    gb_write_key(gbd, key);
    else        gb_write_index_key(father, gbd->index, keyq);

    return gbd;
}

GBCONTAINER *gb_make_pre_defined_container(GBCONTAINER *father, GBCONTAINER *gbd, long index_pos, GBQUARK keyq) {
    // inserts an object into the dabase hierarchy
    GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(father);

    SET_GB_FATHER(gbd, father);
    gbd->main_idx = father->main_idx;

    if (Main->local_mode) gbd->server_id = GBTUM_MAGIC_NUMBER;
    if (Main->clock)
    {
        GB_CREATE_EXT((GBDATA *) gbd);
        gbd->ext->creation_date = Main->clock;
    }
    gb_link_entry(father, (GBDATA *) gbd, index_pos);
    gb_write_index_key(father, gbd->index, keyq);

    return gbd;
}


GBCONTAINER *gb_make_container(GBCONTAINER * father, const char *key, long index_pos, GBQUARK keyq)
{
    GBCONTAINER    *gbd;
    long            gbm_index;

    if (father)
    {
        GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(father);

        if (!keyq) keyq = gb_key_2_quark(Main, key);
        gbm_index = GB_QUARK_2_GBMINDEX(Main, keyq);
        gbd = (GBCONTAINER *) gbm_get_mem(sizeof(GBCONTAINER), gbm_index);
        GB_GBM_INDEX(gbd) = gbm_index;
        SET_GB_FATHER(gbd, father);
        gbd->flags.type = GB_DB;
        gbd->main_idx = father->main_idx;
        if (Main->local_mode) gbd->server_id = GBTUM_MAGIC_NUMBER;
        if (Main->clock)
        {
            GB_CREATE_EXT((GBDATA *) gbd);
            gbd->ext->creation_date = Main->clock;
        }
        gb_link_entry(father, (GBDATA *) gbd, index_pos);
        if (key)    gb_write_key((GBDATA *)gbd, key);
        else        gb_write_index_key(father, gbd->index, keyq);

        return gbd;
    }
    else    // main entry
    {
        gbd = (GBCONTAINER *) gbm_get_mem(sizeof(GBCONTAINER), 0);
        gbd->flags.type = GB_DB;
    }

    return gbd;
}

void gb_pre_delete_entry(GBDATA *gbd) {
    // Reduce an entry to its absolute minimum and remove it from database
    GB_MAIN_TYPE *Main      = GB_MAIN_NO_FATHER(gbd);
    long          type      = GB_TYPE(gbd);
    long          gbm_index = GB_GBM_INDEX(gbd);

    gb_callback *cb_next;
    for (gb_callback *cb = GB_GET_EXT_CALLBACKS(gbd); cb; cb = cb_next) {
        gbd->ext->callback = 0;
        cb_next = cb->next;
        if (!gbd->ext->old && type != GB_DB) {
            gb_save_extern_data_in_ts(gbd);
        }
        if (cb->type & GB_CB_DELETE) {
            gb_add_delete_callback_list(gbd, gbd->ext->old, cb->func, cb->clientdata);
        }
        gbm_free_mem(cb, sizeof(gb_callback), gbm_index);
    }

    if (GB_FATHER(gbd)) {
        gb_write_key(gbd, 0);
    }
    gb_unlink_entry(gbd);

    /* as soon as an entry is deleted, there is
     * no need to keep track of the database entry
     * within the server at the client side
     */
    if (!Main->local_mode && gbd->server_id) {
        GBS_write_numhash(Main->remote_hash, gbd->server_id, 0);
    }

    if (type >= GB_BITS && type < GB_DB) {
        gb_free_cache(Main, gbd);
    }
    GB_FREE_TRANSACTION_SAVE(gbd);
    GB_DELETE_EXT(gbd, gbm_index);
}

void gb_delete_entry(GBDATA **gbd_ptr) {
    GBDATA *gbd       = *gbd_ptr;
    long    type      = GB_TYPE(gbd);

    if (type == GB_DB) {
        gb_delete_entry((GBCONTAINER**)gbd_ptr);
    }
    else {
        long gbm_index = GB_GBM_INDEX(gbd);

        gb_pre_delete_entry(gbd);
        if (type >= GB_BITS) GB_FREEDATA(gbd);
        gbm_free_mem(gbd, sizeof(GBDATA), gbm_index);
        
        *gbd_ptr = 0;                               // avoid further usage
    }
}

void gb_delete_entry(GBCONTAINER **gbc_ptr) {
    GBCONTAINER *gbc       = *gbc_ptr;
    long         gbm_index = GB_GBM_INDEX(gbc);

    gb_assert(GB_TYPE(gbc) == GB_DB);

    for (long index = 0; index < gbc->d.nheader; index++) {
        GBDATA *gbd = GBCONTAINER_ELEM(gbc, index);
        if (gbd) {
            gb_delete_entry(&gbd);
            SET_GBCONTAINER_ELEM(gbc, index, NULL);
        }
    }

    gb_pre_delete_entry((GBDATA*)gbc);

    // what is left now, is the core database entry!

    gb_destroy_indices(gbc);
    gb_header_list *hls;
    
    if ((hls=GB_DATA_LIST_HEADER(gbc->d)) != NULL) {
        gbm_free_mem(hls, sizeof(gb_header_list) * gbc->d.headermemsize, GBM_HEADER_INDEX);
    }
    gbm_free_mem(gbc, sizeof(GBCONTAINER), gbm_index);

    *gbc_ptr = 0;                                   // avoid further usage
}

static void gb_delete_main_entry(GBCONTAINER **gb_main_ptr) {
    GBCONTAINER *gb_main  = *gb_main_ptr;
    
    gb_assert(GB_TYPE(gb_main) == GB_DB);

    GBQUARK sys_quark = GB_key_2_quark((GBDATA*)gb_main, GB_SYSTEM_FOLDER);

    for (int pass = 1; pass <= 2; pass++) {
        for (int index = 0; index < gb_main->d.nheader; index++) {
            GBDATA *gbd = GBCONTAINER_ELEM(gb_main, index);
            if (gbd) {
                // delay deletion of system folder to pass 2:
                if (pass == 2 || GB_KEY_QUARK(gbd) != sys_quark) { 
                    gb_delete_entry(&gbd);
                    SET_GBCONTAINER_ELEM(gb_main, index, NULL);
                }
            }
        }
    }
    gb_delete_entry(gb_main_ptr);
}

void gb_delete_dummy_father(GBCONTAINER **dummy_father) {
    GBCONTAINER  *gbc  = *dummy_father;
    GB_MAIN_TYPE *Main = GB_MAIN(gbc);

    gb_assert(GB_TYPE(gbc)   == GB_DB);
    gb_assert(GB_FATHER(gbc) == NULL);

    for (int index = 0; index < gbc->d.nheader; index++) {
        GBCONTAINER *gb_main = (GBCONTAINER*)GBCONTAINER_ELEM(gbc, index);
        if (gb_main) {
            gb_assert(GB_TYPE(gb_main) == GB_DB);
            gb_assert(gb_main == Main->data);
            gb_delete_main_entry((GBCONTAINER**)&gb_main);

            SET_GBCONTAINER_ELEM(gbc, index, NULL);
            Main->data = NULL;
        }
    }

    gb_delete_entry(dummy_father);
}

// ---------------------
//      Data Storage

gb_transaction_save *gb_new_gb_transaction_save(GBDATA *gbd) {
    // Note: does not increment the refcounter
    gb_transaction_save *ts = (gb_transaction_save *)gbm_get_mem(sizeof(gb_transaction_save), GBM_CB_INDEX);

    ts->flags  = gbd->flags;
    ts->flags2 = gbd->flags2;

    if (gbd->flags2.extern_data) {
        ts->info.ex.data    = GB_EXTERN_DATA_DATA(gbd->info.ex);
        ts->info.ex.memsize = gbd->info.ex.memsize;
        ts->info.ex.size    = gbd->info.ex.size;
    }
    else {
        memcpy(&(ts->info), &(gbd->info), sizeof(gbd->info));
    }

    ts->refcount = 1;

    return ts;
}

void gb_add_ref_gb_transaction_save(gb_transaction_save *ts) {
    if (!ts) return;
    ts->refcount ++;
}

void gb_del_ref_gb_transaction_save(gb_transaction_save *ts) {
    if (!ts) return;
    ts->refcount --;
    if (ts->refcount <= 0) {    // no more references !!!!
        if (ts->flags2.extern_data) {
            if (ts->info.ex.data) {
                gbm_free_mem(ts->info.ex.data,
                             ts->info.ex.memsize,
                             ts->flags2.gbm_index);
            }
        }
        gbm_free_mem(ts, sizeof(gb_transaction_save), GBM_CB_INDEX);
    }
}

void gb_del_ref_and_extern_gb_transaction_save(gb_transaction_save *ts) {
    // remove reference to undo entry and set extern pointer to zero
    if (ts->flags2.extern_data) {
        ts->info.ex.data = 0;
    }
    gb_del_ref_gb_transaction_save(ts);
}

void gb_abortdata(GBDATA *gbd) {
    gb_transaction_save *old;

    GB_INDEX_CHECK_OUT(gbd);
    old = gbd->ext->old;
    gb_assert(old!=0);

    gbd->flags = old->flags;
    gbd->flags2 = old->flags2;

    if (old->flags2.extern_data)
    {
        SET_GB_EXTERN_DATA_DATA(gbd->info.ex, old->info.ex.data);
        gbd->info.ex.memsize = old->info.ex.memsize;
        gbd->info.ex.size = old->info.ex.size;
    }
    else
    {
        memcpy(&(gbd->info), &(old->info), sizeof(old->info));
    }
    gb_del_ref_and_extern_gb_transaction_save(old);
    gbd->ext->old = NULL;

    GB_INDEX_CHECK_IN(gbd);
}


void gb_save_extern_data_in_ts(GBDATA *gbd) {
    /* Saves gbd->info into gbd->ext->old
     * Destroys gbd->info!
     * Don't call with GBCONTAINER
     */

    GB_CREATE_EXT(gbd);
    GB_INDEX_CHECK_OUT(gbd);
    if (gbd->ext->old || (GB_ARRAY_FLAGS(gbd).changed == GB_CREATED)) {
        GB_FREEDATA(gbd);
    }
    else {
        gbd->ext->old = gb_new_gb_transaction_save(gbd);
        SET_GB_EXTERN_DATA_DATA(gbd->info.ex, 0);
    }
}


// -----------------------
//      Key Management

void gb_write_index_key(GBCONTAINER *father, long index, GBQUARK new_index) {
    // Set the key quark of an database field.
    // Check for indexing data field.

    GB_MAIN_TYPE   *Main      = GBCONTAINER_MAIN(father);
    gb_header_list *hls       = GB_DATA_LIST_HEADER(father->d);
    GBQUARK         old_index = hls[index].flags.key_quark;

    GBCONTAINER *gfather;
    Main->keys[old_index].nref--;
    Main->keys[new_index].nref++;

    if (Main->local_mode) {
        GBDATA *gbd = GB_HEADER_LIST_GBD(hls[index]);

        if (gbd && (GB_TYPE(gbd) == GB_STRING || GB_TYPE(gbd) == GB_LINK)) {
            gb_index_files *ifs = 0;

            GB_INDEX_CHECK_OUT(gbd);
            gbd->flags2.tisa_index = 0;
            if ((gfather = GB_FATHER(father))) {
                for (ifs = GBCONTAINER_IFS(gfather); ifs;
                        ifs = GB_INDEX_FILES_NEXT(ifs))
                {
                    if (ifs->key == new_index) break;
                }
            }
            hls[index].flags.key_quark = new_index;
            if (ifs) gb_index_check_in(gbd);

            return;
        }
    }

    hls[index].flags.key_quark = new_index;
}

void gb_write_key(GBDATA *gbd, const char *s) {
    GBQUARK new_index = 0;

    if (s) {
        GB_MAIN_TYPE *Main = GB_MAIN(gbd);
        new_index          = (int)GBS_read_hash(Main->key_2_index_hash, s);

        if (!new_index) {                           // create new index
            new_index = (int)gb_create_key(Main, s, true);
        }
    }
    gb_write_index_key(GB_FATHER(gbd), gbd->index, new_index);
}

void gb_create_key_array(GB_MAIN_TYPE *Main, int index) {
    if (index >= Main->sizeofkeys) {
        Main->sizeofkeys = index*3/2+1;
        if (Main->keys) {
            int i;
            Main->keys = (gb_Key *)realloc(Main->keys, sizeof(gb_Key) * (size_t)Main->sizeofkeys);
            memset((char *)&(Main->keys[Main->keycnt]), 0, sizeof(gb_Key) * (size_t) (Main->sizeofkeys - Main->keycnt));
            for (i = Main->keycnt; i < Main->sizeofkeys; i++) {
                Main->keys[i].compression_mask = -1;
            }
        }
        else {
            Main->sizeofkeys = 1000;
            Main->keys = (gb_Key *)GB_calloc(sizeof(gb_Key), (size_t)Main->sizeofkeys);
        }
    }
}

long gb_create_key(GB_MAIN_TYPE *Main, const char *s, bool create_gb_key) {
    long index;
    if (Main->first_free_key) {
        index = Main->first_free_key;
        Main->first_free_key = Main->keys[index].next_free_key;
        Main->keys[index].next_free_key = 0;
    }
    else {
        index = Main->keycnt++;
        gb_create_key_array(Main, (int)index+1);
    }
    if (!Main->local_mode) {
        long test_index = gbcmc_key_alloc((GBDATA *)Main->data, s);
        if (test_index != index) {
            GBK_terminatef("Database corrupt (allocating quark '%s' in server failed)", s);
        }
    }
    Main->keys[index].nref = 0;

    if (s) {
        Main->keys[index].key = strdup(s);
        GBS_write_hash(Main->key_2_index_hash, s, index);
        gb_assert(GBS_hash_count_elems(Main->key_2_index_hash) <= ALLOWED_KEYS);
        if (Main->gb_key_data && create_gb_key) {
            gb_load_single_key_data((GBDATA *)Main->data, (GBQUARK)index);
            // Warning: starts a big recursion
            if (!Main->local_mode) { // send new gb_key to server, needed for searching
                GB_update_server((GBDATA *)Main->data);
            }
        }
    }


    Main->key_clock = Main->clock;
    return index;
}

void gb_free_all_keys(GB_MAIN_TYPE *Main) {
    if (Main->keys) {
        for (long index = 1; index < Main->keycnt; index++) {
            if (Main->keys[index].key) {
                GBS_write_hash(Main->key_2_index_hash, Main->keys[index].key, 0);
                freenull(Main->keys[index].key);
            }
            Main->keys[index].nref = 0;
            Main->keys[index].next_free_key = 0;
        }
        freenull(Main->keys[0].key); // "main"
        Main->first_free_key = 0;
        Main->keycnt         = 1;
    }
}

#if defined(DEVEL_RALF)
#warning useless return value - always 0
#endif // DEVEL_RALF
char *gb_abort_entry(GBDATA *gbd) {
    int type = GB_TYPE(gbd);

    GB_ARRAY_FLAGS(gbd).flags = gbd->flags.saved_flags;

    if (type != GB_DB) {
        if (GB_GET_EXT_OLD_DATA(gbd)) {
            if (type >= GB_BITS) {
                gb_free_cache(GB_MAIN(gbd), gbd);
                GB_FREEDATA(gbd);
            }
            gb_abortdata(gbd);
        }
    }
    return 0;
}

// ---------------------
//      Transactions

#if defined(DEVEL_RALF)
#warning change param for gb_abort_transaction_local_rek to GBDATA **
#warning remove param 'mode' (unused!)
#endif // DEVEL_RALF

int gb_abort_transaction_local_rek(GBDATA *gbd, long mode) {
    // delete created, undo changed
    GBDATA    *gb;
    GB_TYPES   type;
    GB_CHANGE  change = (GB_CHANGE)GB_ARRAY_FLAGS(gbd).changed;

    switch (change) {
        case GB_UNCHANGED:
            return 0;

        case GB_CREATED:
            GB_PUT_SECURITY_DELETE(gbd, 0);
            gb_delete_entry(&gbd);
            return 1;

        case GB_DELETED:
            GB_ARRAY_FLAGS(gbd).changed = GB_UNCHANGED;
            // fall-through
        default:
            type = (GB_TYPES)GB_TYPE(gbd);
            if (type == GB_DB)
            {
                int             index;
                GBCONTAINER    *gbc = (GBCONTAINER *)gbd;
                gb_header_list *hls = GB_DATA_LIST_HEADER(gbc->d);

                for (index = 0; index < gbc->d.nheader; index++)
                {
                    if ((gb = GB_HEADER_LIST_GBD(hls[index]))!=NULL)
                    {
                        gb_abort_transaction_local_rek(gb, mode);
                    }
                }
            }
            gb_abort_entry(gbd);
    }
    return 0;
}

GB_ERROR gb_commit_transaction_local_rek(GBDATA * gbd, long mode, int *pson_created) {
    /* commit created / delete deleted
     *   mode   0   local     = server    or
     *              begin trans in client or
     *              commit_client_in_server
     *   mode   1   remote    = client
     *   mode   2   remote    = client (only send updated data)
     */
    GBDATA       *gb;
    GB_MAIN_TYPE *Main        = GB_MAIN(gbd);
    GB_TYPES      type;
    GB_ERROR      error;
    gb_callback  *cb;
    GB_CHANGE     change      = (GB_CHANGE)GB_ARRAY_FLAGS(gbd).changed;
    int           send_header;
    int           son_created = 0;

    type = (GB_TYPES)GB_TYPE(gbd);
    switch (change) {
        case GB_UNCHANGED:
            return 0;
        case GB_DELETED:
            GB_PUT_SECURITY_DELETE(gbd, 0);
            if (mode) {
                if (!gbd->flags2.update_in_server) {
                    error = gbcmc_sendupdate_delete(gbd);
                    if (error)
                        return error;
                    gbd->flags2.update_in_server = 1;
                }
                if (mode == 2) return 0;
            }
            else {
                gbcms_add_to_delete_list(gbd);
                _GB_CHECK_IN_UNDO_DELETE(Main, gbd);
                return 0;
            }
            gb_delete_entry(&gbd);
            return 0;
        case GB_CREATED:
            if (mode) {
                if (!gbd->flags2.update_in_server) {
                    if (gbd->server_id) goto gb_changed_label;
                    // already created, do only a change
                    error = gbcmc_sendupdate_create(gbd);
                    if (type == GB_DB) {
                        gb_set_update_in_server_flags(((GBCONTAINER *)gbd));
                        // set all children update_in_server flags
                    }
                    gbd->flags2.update_in_server = 1;
                    if (error)  return error;
                }
                if (mode == 2) return 0;
            }
            else {
                _GB_CHECK_IN_UNDO_CREATE(Main, gbd);
            }
            if (pson_created) {
                *pson_created  = 1;
            }

            if (gbd->flags2.header_changed == 1) {
                ((GBCONTAINER*)gbd)->header_update_date = Main->clock;
            }
            goto gb_commit_do_callbacks;

        case GB_NORMAL_CHANGE:
            if (mode) {
                if (!gbd->flags2.update_in_server) {
                gb_changed_label :;
                    send_header = 0;
                    if (gbd->flags2.header_changed) send_header = 1;
                    error = gbcmc_sendupdate_update(gbd, send_header);
                    if (error) return error;
                    gbd->flags2.update_in_server = 1;
                }
            }
            else {
                _GB_CHECK_IN_UNDO_MODIFY(Main, gbd);
            }
            // fall-through

        default:                                    // means GB_SON_CHANGED + GB_NORMAL_CHANGE

            if (type == GB_DB)
            {
                GBCONTAINER    *gbc = (GBCONTAINER *)gbd;
                int             index, start, end;
                gb_header_list *hls = GB_DATA_LIST_HEADER(gbc->d);

                if (gbc->index_of_touched_one_son>0) {
                    start = (int)gbc->index_of_touched_one_son-1;
                    end = start+1;
                }
                else {
                    if (!gbc->index_of_touched_one_son) {
                        start = end = 0;
                    }
                    else {
                        start = 0;
                        end = gbc->d.nheader;
                    }
                }

                for (index = start; index < end; index++) {
                    if ((gb = GB_HEADER_LIST_GBD(hls[index]))!=NULL) {
                        if (!hls[index].flags.changed) continue;
                        error = gb_commit_transaction_local_rek(gb, mode, &son_created);
                        if (error) return error;
                    }
                }

                if (mode) gbd->flags2.update_in_server = 1;
            }
    gb_commit_do_callbacks :
            if (mode == 2) {    // update server; no callbacks
                gbd->flags2.update_in_server = 1;
            }
            else {
                GB_CB_TYPE gbtype = GB_CB_CHANGED;
                if (son_created) {
                    gbtype = (GB_CB_TYPE)(GB_CB_SON_CREATED | GB_CB_CHANGED);
                }
                GB_CREATE_EXT(gbd);
                gbd->ext->update_date = Main->clock;
                if (gbd->flags2.header_changed) {
                    ((GBCONTAINER*)gbd)->header_update_date = Main->clock;
                }

                for (cb = GB_GET_EXT_CALLBACKS(gbd); cb; cb = cb->next) {
                    if (cb->type & (GB_CB_CHANGED|GB_CB_SON_CREATED)) {
                        gb_add_changed_callback_list(gbd, gbd->ext->old, gbtype, cb->func, cb->clientdata);
                    }
                }

                GB_FREE_TRANSACTION_SAVE(gbd);
            }
    }

    return 0;
}
