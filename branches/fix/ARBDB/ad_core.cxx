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
#include "gb_index.h"
#include "gb_localdata.h"
#include "ad_hcb.h"

// Copy all info + external data mem to an one step undo buffer
// (needed to abort transactions)

inline void _GB_CHECK_IN_UNDO_DELETE(GB_MAIN_TYPE *Main, GBDATA *& gbd) {
    if (Main->undo_type) gb_check_in_undo_delete(Main, gbd);
    else gb_delete_entry(gbd);
}
inline void _GB_CHECK_IN_UNDO_CREATE(GB_MAIN_TYPE *Main, GBDATA *gbd) {
    if (Main->undo_type) gb_check_in_undo_create(Main, gbd);
}
inline void _GB_CHECK_IN_UNDO_MODIFY(GB_MAIN_TYPE *Main, GBDATA *gbd) {
    if (Main->undo_type) gb_check_in_undo_modify(Main, gbd);
}

// ---------------------------
//      trigger callbacks
//      (i.e. add them to pending callbacks)

void GB_MAIN_TYPE::callback_group::trigger(GBDATA *gbd, GB_CB_TYPE type, gb_callback_list *dataCBs) {
    if (hierarchy_cbs) {
        for (gb_hierarchy_callback_list::itertype cb = hierarchy_cbs->callbacks.begin(); cb != hierarchy_cbs->callbacks.end(); ++cb) {
            if ((cb->spec.get_type() & type) && cb->triggered_by(gbd)) {
                pending.add_unchecked(gb_triggered_callback(gbd, gbd->ext->old, cb->spec));
            }
        }
    }
    if (dataCBs) {
        for (gb_callback_list::itertype cb = dataCBs->callbacks.begin(); cb != dataCBs->callbacks.end(); ++cb) {
            if (cb->spec.get_type() & type) {
                pending.add_unchecked(gb_triggered_callback(gbd, gbd->ext->old, cb->spec));
            }
        }
    }
}

inline void GB_MAIN_TYPE::trigger_change_callbacks(GBDATA *gbd, GB_CB_TYPE type) {
    changeCBs.trigger(gbd, type, gbd->get_callbacks());
}

void GB_MAIN_TYPE::trigger_delete_callbacks(GBDATA *gbd) {
    gb_callback_list *cbl = gbd->get_callbacks();
    if (cbl || deleteCBs.hierarchy_cbs) {
        gb_assert(gbd->ext);

        gbd->ext->callback = NULL;
        if (!gbd->ext->old && gbd->type() != GB_DB) {
            gb_save_extern_data_in_ts(gbd->as_entry());
        }

        deleteCBs.trigger(gbd, GB_CB_DELETE, cbl);

        gb_assert(gbd->ext->callback == NULL);
        delete cbl;
    }
}
// ---------------------------
//      GB data management

void gb_touch_entry(GBDATA *gbd, GB_CHANGE val) {
    gbd->flags2.update_in_server = 0;
    GB_ARRAY_FLAGS(gbd).inc_change(val);

    GBCONTAINER *gbc = GB_FATHER(gbd);
    gbc->set_touched_idx(gbd->index);

    while (1) {
        GBCONTAINER *gbc_father = GB_FATHER(gbc);
        if (!gbc_father) break;

        gbc_father->set_touched_idx(gbc->index);

        if (gbc->flags2.update_in_server) {
            gbc->flags2.update_in_server = 0;
        }
        else {
            if (GB_ARRAY_FLAGS(gbc).changed >= GB_SON_CHANGED)
                return;
        }
        GB_ARRAY_FLAGS(gbc).inc_change(GB_SON_CHANGED);
        gbc = gbc_father;
    }
}

void gb_touch_header(GBCONTAINER *gbc) {
    gbc->flags2.header_changed = 1;
    gb_touch_entry(gbc, GB_NORMAL_CHANGE);
}


void gb_untouch_children(GBCONTAINER *gbc) {
    GBDATA          *gbd;
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
                if (gbd->is_container()) {
                    gb_untouch_children(gbd->as_container());
                }
            }
            gbd->flags2.update_in_server = 0;
        }
    }
    gbc->index_of_touched_one_son = 0;
}

void gb_untouch_me(GBENTRY *gbe) {
    GB_DATA_LIST_HEADER(GB_FATHER(gbe)->d)[gbe->index].flags.changed = GB_UNCHANGED;
}
inline void gb_untouch_me(GBCONTAINER *gbc) {
    GB_DATA_LIST_HEADER(GB_FATHER(gbc)->d)[gbc->index].flags.changed = GB_UNCHANGED;

    gbc->flags2.header_changed    = 0;
    gbc->index_of_touched_one_son = 0;
}

void gb_untouch_children_and_me(GBCONTAINER *gbc) {
    gb_untouch_children(gbc);
    gb_untouch_me(gbc);
}

static void gb_set_update_in_server_flags(GBCONTAINER *gbc) {
    for (int index = 0; index < gbc->d.nheader; index++) {
        GBDATA *gbd = GBCONTAINER_ELEM(gbc, index);
        if (gbd) {
            if (gbd->is_container()) {
                gb_set_update_in_server_flags(gbd->as_container());
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
        int idx;
        int maxidx = gbc->d.headermemsize; // ???: oder ->d.nheader

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

static void gb_link_entry(GBCONTAINER* father, GBDATA *gbd, long index_pos) {
    /* if index_pos == -1 -> to end of data;
       else special index position; error when data already exists in index pos */

    SET_GB_FATHER(gbd, father);
    if (father == NULL) {   // 'main' entry in GB
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

static void gb_unlink_entry(GBDATA * gbd) {
    GBCONTAINER *father = GB_FATHER(gbd);

    if (father)
    {
        int             index_pos = (int)gbd->index;
        gb_header_list *hls       = &(GB_DATA_LIST_HEADER(father->d)[index_pos]);

        SET_GB_HEADER_LIST_GBD(*hls, NULL);
        hls->flags.key_quark    = 0;
        hls->flags.set_change(GB_DELETED);
        father->d.size--;
        SET_GB_FATHER(gbd, NULL);
    }
}

GB_MAIN_TYPE::GB_MAIN_TYPE(const char *db_path)
    : transaction_level(0),
      aborted_transaction(0),
      i_am_server(false),
      c_link(NULL),
      server_data(NULL),
      dummy_father(NULL),
      root_container(NULL),
      gb_key_data(NULL),
      path(nulldup(db_path)),
      opentype(gb_open_all),
      disabled_path(NULL),
      allow_corrupt_file_recovery(0),
      compression_mask(-1), // allow all compressions
      keycnt(0),
      sizeofkeys(0),
      first_free_key(0),
      keys(NULL),
      key_2_index_hash(GBS_create_hash(ALLOWED_KEYS, GB_MIND_CASE)),
      key_clock(0),
      mapped(false),
      last_updated(0),
      last_saved_time(0),
      last_saved_transaction(0),
      last_main_saved_transaction(0),
      requested_undo_type(GB_UNDO_NONE),
      undo_type(GB_UNDO_NONE),
      undo(NULL),
      security_level(0),
      old_security_level(0),
      pushed_security_level(0),
      clock(0),
      remote_hash(NULL),
      command_hash(NULL),
      resolve_link_hash(NULL),
      table_hash(NULL),
      close_callbacks(NULL),
      this_user(NULL)
{
    for (int i = 0; i<ALLOWED_DATES; ++i) dates[i] = NULL;
    for (int i = 0; i<GB_MAX_USERS;  ++i) users[i] = NULL;

    gb_init_undo_stack(this);
    gb_local->announce_db_open(this);
}

GB_MAIN_TYPE::~GB_MAIN_TYPE() {
    gb_assert(!dummy_father);
    gb_assert(!root_container);

    release_main_idx();

    if (command_hash)      GBS_free_hash(command_hash);
    if (table_hash)        GBS_free_hash(table_hash);
    if (resolve_link_hash) GBS_free_hash(resolve_link_hash);
    if (remote_hash)       GBS_free_numhash(remote_hash);

    free_all_keys();
    
    if (key_2_index_hash) GBS_free_hash(key_2_index_hash);
    freenull(keys);

    gb_free_undo_stack(this);

    for (int j = 0; j<ALLOWED_DATES; ++j) freenull(dates[j]);

    free(path);
    free(disabled_path);
    free(qs.quick_save_disabled);

    gb_local->announce_db_close(this);
}

GBDATA *gb_make_pre_defined_entry(GBCONTAINER *father, GBDATA *gbd, long index_pos, GBQUARK keyq) {
    // inserts an object into the dabase hierarchy
    GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(father);

    SET_GB_FATHER(gbd, father);
    if (Main->is_server()) {
        gbd->server_id = GBTUM_MAGIC_NUMBER;
    }
    if (Main->clock) {
        gbd->create_extended();
        gbd->touch_creation(Main->clock);
    }

    gb_link_entry(father, gbd, index_pos);
    gb_write_index_key(father, gbd->index, keyq);

    return gbd;
}

static void gb_write_key(GBDATA *gbd, const char *s) {
    GB_MAIN_TYPE *Main        = GB_MAIN(gbd);
    GBQUARK       new_index   = GBS_read_hash(Main->key_2_index_hash, s);
    if (!new_index) new_index = (int)gb_create_key(Main, s, true); // create new index
    gb_write_index_key(GB_FATHER(gbd), gbd->index, new_index);
}

GBENTRY *gb_make_entry(GBCONTAINER *father, const char *key, long index_pos, GBQUARK keyq, GB_TYPES type) {
    // creates a terminal database object
    GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(father);

    if (!keyq) keyq = gb_find_or_create_quark(Main, key);

    long     gbm_index = quark2gbmindex(Main, keyq);
    GBENTRY *gbe       = (GBENTRY*)gbm_get_mem(sizeof(GBENTRY), gbm_index);

    GB_GBM_INDEX(gbe) = gbm_index;
    SET_GB_FATHER(gbe, father);

    switch (type) {
        case GB_STRING_SHRT:
            type = GB_STRING;
            // fall-through
        case GB_STRING:
            gbe->insert_data("", 0, 1);
            break;
        case GB_LINK:
            gbe->insert_data(":", 1, 2);
            break;
        default:
            break;
    }
    gbe->flags.type = type;

    if (Main->is_server()) {
        gbe->server_id = GBTUM_MAGIC_NUMBER;
    }
    if (Main->clock) {
        gbe->create_extended();
        gbe->touch_creation(Main->clock);
    }

    gb_link_entry(father, gbe, index_pos);
    if (key)    gb_write_key(gbe, key);
    else        gb_write_index_key(father, gbe->index, keyq);

    return gbe;
}

GBCONTAINER *gb_make_pre_defined_container(GBCONTAINER *father, GBCONTAINER *gbc, long index_pos, GBQUARK keyq) {
    // inserts an object into the dabase hierarchy
    GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(father);

    SET_GB_FATHER(gbc, father);
    gbc->main_idx = father->main_idx;

    if (Main->is_server()) gbc->server_id = GBTUM_MAGIC_NUMBER;
    if (Main->clock) {
        gbc->create_extended();
        gbc->touch_creation(Main->clock);
    }
    gb_link_entry(father, gbc, index_pos);
    gb_write_index_key(father, gbc->index, keyq);

    return gbc;
}


GBCONTAINER *gb_make_container(GBCONTAINER * father, const char *key, long index_pos, GBQUARK keyq) {
    GBCONTAINER *gbc;

    if (father) {
        GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(father);

        if (!keyq) keyq   = gb_find_or_create_NULL_quark(Main, key);
        long gbm_index    = quark2gbmindex(Main, keyq);
        gbc               = (GBCONTAINER *)gbm_get_mem(sizeof(GBCONTAINER), gbm_index);
        GB_GBM_INDEX(gbc) = gbm_index;

        SET_GB_FATHER(gbc, father);
        gbc->flags.type = GB_DB;
        gbc->main_idx = father->main_idx;
        if (Main->is_server()) gbc->server_id = GBTUM_MAGIC_NUMBER;
        if (Main->clock) {
            gbc->create_extended();
            gbc->touch_creation(Main->clock);
        }
        gb_link_entry(father, gbc, index_pos);
        if (key)    gb_write_key(gbc, key);
        else        gb_write_index_key(father, gbc->index, keyq);
    }
    else { // main entry
        gbc = (GBCONTAINER *) gbm_get_mem(sizeof(GBCONTAINER), 0);
        gbc->flags.type = GB_DB;
    }

    return gbc;
}

void gb_pre_delete_entry(GBDATA *gbd) {
    // Reduce an entry to its absolute minimum and remove it from database
    GB_MAIN_TYPE *Main = GB_MAIN_NO_FATHER(gbd);
    GB_TYPES      type = gbd->type();

    Main->trigger_delete_callbacks(gbd);

    {
        GBCONTAINER *gb_father = GB_FATHER(gbd);
        if (gb_father) gb_write_index_key(gb_father, gbd->index, 0);
    }
    gb_unlink_entry(gbd);

    /* as soon as an entry is deleted, there is
     * no need to keep track of the database entry
     * within the server at the client side
     */
    if (Main->is_client() && gbd->server_id) {
        if (Main->remote_hash) GBS_write_numhash(Main->remote_hash, gbd->server_id, 0);
    }

    if (type >= GB_BITS && type < GB_DB) {
        gb_free_cache(Main, gbd->as_entry()); // cant use gb_uncache (since entry is already unlinked!)
    }
    GB_FREE_TRANSACTION_SAVE(gbd);
    gbd->destroy_extended();
}

void gb_delete_entry(GBCONTAINER*& gbc) {
    long gbm_index = GB_GBM_INDEX(gbc);

    gb_assert(gbc->type() == GB_DB);

    for (long index = 0; index < gbc->d.nheader; index++) {
        GBDATA *gbd = GBCONTAINER_ELEM(gbc, index);
        if (gbd) {
            gb_delete_entry(gbd);
            SET_GBCONTAINER_ELEM(gbc, index, NULL);
        }
    }

    gb_pre_delete_entry(gbc);

    // what is left now, is the core database entry!

    gb_destroy_indices(gbc);
    gb_header_list *hls;

    if ((hls=GB_DATA_LIST_HEADER(gbc->d)) != NULL) {
        gbm_free_mem(hls, sizeof(gb_header_list) * gbc->d.headermemsize, GBM_HEADER_INDEX);
    }
    gbm_free_mem(gbc, sizeof(GBCONTAINER), gbm_index);

    gbc = NULL; // avoid further usage
}

void gb_delete_entry(GBENTRY*& gbe) {
    long gbm_index = GB_GBM_INDEX(gbe);

    gb_pre_delete_entry(gbe);
    if (gbe->type() >= GB_BITS) gbe->free_data();
    gbm_free_mem(gbe, sizeof(GBENTRY), gbm_index);

    gbe = NULL; // avoid further usage
}

void gb_delete_entry(GBDATA*& gbd) {
    if (gbd->is_container()) {
        gb_delete_entry(reinterpret_cast<GBCONTAINER*&>(gbd));
    }
    else {
        gb_delete_entry(reinterpret_cast<GBENTRY*&>(gbd));
    }
}

static void gb_delete_main_entry(GBCONTAINER*& gb_main) {
    GBQUARK sys_quark = key2quark(GB_MAIN(gb_main), GB_SYSTEM_FOLDER);

    // Note: sys_quark may be 0 (happens when destroying client db which never established a connection).
    // In this case no system folder/quark has been created (and we do no longer try to create it)
    // Nothing will happen in pass 2 below.

    for (int pass = 1; pass <= 2; pass++) {
        for (int index = 0; index < gb_main->d.nheader; index++) {
            GBDATA *gbd = GBCONTAINER_ELEM(gb_main, index);
            if (gbd) {
                // delay deletion of system folder to pass 2:
                if (pass == 2 || GB_KEY_QUARK(gbd) != sys_quark) { 
                    gb_delete_entry(gbd);
                    SET_GBCONTAINER_ELEM(gb_main, index, NULL);
                }
            }
        }
    }
    gb_delete_entry(gb_main);
}

void gb_delete_dummy_father(GBCONTAINER*& gbc) {
    gb_assert(GB_FATHER(gbc) == NULL);

    GB_MAIN_TYPE *Main = GB_MAIN(gbc);
    for (int index = 0; index < gbc->d.nheader; index++) {
        GBDATA *gbd = GBCONTAINER_ELEM(gbc, index);
        if (gbd) {
            // dummy fathers should only have one element (which is the root_container)
            GBCONTAINER *gb_main = gbd->as_container();
            gb_assert(gb_main == Main->root_container);

            gb_delete_main_entry(gb_main);
            SET_GBCONTAINER_ELEM(gbc, index, NULL);
            Main->root_container = NULL;
        }
    }

    gb_delete_entry(gbc);
}

// ---------------------
//      Data Storage

static gb_transaction_save *gb_new_gb_transaction_save(GBENTRY *gbe) {
    // Note: does not increment the refcounter
    gb_transaction_save *ts = (gb_transaction_save *)gbm_get_mem(sizeof(gb_transaction_save), GBM_CB_INDEX);

    ts->flags  = gbe->flags;
    ts->flags2 = gbe->flags2;

    if (gbe->stored_external()) {
        ts->info.ex.data    = gbe->info.ex.get_data();
        ts->info.ex.memsize = gbe->info.ex.memsize;
        ts->info.ex.size    = gbe->info.ex.size;
    }
    else {
        memcpy(&(ts->info), &(gbe->info), sizeof(gbe->info));
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
        if (ts->stored_external()) {
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
    if (ts->stored_external()) {
        ts->info.ex.data = 0;
    }
    gb_del_ref_gb_transaction_save(ts);
}

static void gb_abortdata(GBENTRY *gbe) {
    gb_transaction_save *old;

    gbe->index_check_out();
    old = gbe->ext->old;
    gb_assert(old!=0);

    gbe->flags = old->flags;
    gbe->flags2 = old->flags2;

    if (old->stored_external())
    {
        gbe->info.ex.set_data(old->info.ex.data);
        gbe->info.ex.memsize = old->info.ex.memsize;
        gbe->info.ex.size = old->info.ex.size;
    }
    else
    {
        memcpy(&(gbe->info), &(old->info), sizeof(old->info));
    }
    gb_del_ref_and_extern_gb_transaction_save(old);
    gbe->ext->old = NULL;

    gbe->index_re_check_in();
}


void gb_save_extern_data_in_ts(GBENTRY *gbe) {
    /* Saves gbe->info into gbe->ext->old
     * Destroys gbe->info!
     * Don't call with GBCONTAINER
     */

    gbe->create_extended();
    gbe->index_check_out();
    if (gbe->ext->old || (GB_ARRAY_FLAGS(gbe).changed == GB_CREATED)) {
        gbe->free_data();
    }
    else {
        gbe->ext->old = gb_new_gb_transaction_save(gbe);
        gbe->info.ex.set_data(0);
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

    Main->keys[old_index].nref--;
    Main->keys[new_index].nref++;

    if (Main->is_server()) {
        GBDATA *gbd = GB_HEADER_LIST_GBD(hls[index]);

        if (gbd && gbd->is_indexable()) {
            GBENTRY        *gbe = gbd->as_entry();
            gb_index_files *ifs = 0;

            gbe->index_check_out();
            gbe->flags2.should_be_indexed = 0; // do not re-checkin
            
            GBCONTAINER *gfather = GB_FATHER(father);
            if (gfather) {
                for (ifs = GBCONTAINER_IFS(gfather); ifs; ifs = GB_INDEX_FILES_NEXT(ifs)) {
                    if (ifs->key == new_index) break;
                }
            }
            hls[index].flags.key_quark = new_index;
            if (ifs) gbe->index_check_in();

            return;
        }
    }

    hls[index].flags.key_quark = new_index;
}

void gb_create_key_array(GB_MAIN_TYPE *Main, int index) {
    if (index >= Main->sizeofkeys) {
        Main->sizeofkeys = index*3/2+1;
        if (Main->keys) {
            Main->keys = (gb_Key *)realloc(Main->keys, sizeof(gb_Key) * (size_t)Main->sizeofkeys);
            memset((char *)&(Main->keys[Main->keycnt]), 0, sizeof(gb_Key) * (size_t) (Main->sizeofkeys - Main->keycnt));
        }
        else {
            Main->sizeofkeys = 1000;
            if (index>=Main->sizeofkeys) Main->sizeofkeys = index+1;
            Main->keys = (gb_Key *)GB_calloc(sizeof(gb_Key), (size_t)Main->sizeofkeys);
        }
        for (int i = Main->keycnt; i < Main->sizeofkeys; i++) {
            Main->keys[i].compression_mask = -1;
        }
    }
    gb_assert(index<Main->sizeofkeys);
}

long gb_create_key(GB_MAIN_TYPE *Main, const char *key, bool create_gb_key) {
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
    if (Main->is_client()) {
        long test_index = gbcmc_key_alloc(Main->gb_main(), key);
        if (test_index != index) {
            if (test_index == 0) { // comm error
                GBK_terminatef("Allocating quark for '%s' failed (Reason: %s)", key, GB_await_error());
            }
            else {
                GBK_terminatef("Database corrupt (allocating quark '%s' in server failed)", key);
            }
        }
    }
    Main->keys[index].nref = 0;

    if (key) {
        if (!key[0]) GBK_terminate("Attempt to allocate empty key");

        Main->keys[index].key = strdup(key);
        GBS_write_hash(Main->key_2_index_hash, key, index);
        gb_assert(GBS_hash_elements(Main->key_2_index_hash) <= ALLOWED_KEYS);
        if (Main->gb_key_data && create_gb_key) {
            gb_load_single_key_data(Main->gb_main(), (GBQUARK)index);
            // Warning: starts a big recursion
            if (Main->is_client()) { // send new gb_key to server, needed for searching
                GBK_terminate_on_error(Main->send_update_to_server(Main->gb_main()));
            }
        }
    }


    Main->key_clock = Main->clock;
    return index;
}

void GB_MAIN_TYPE::free_all_keys() {
    if (keys) {
        for (long index = 1; index < keycnt; index++) {
            if (keys[index].key) {
                GBS_write_hash(key_2_index_hash, keys[index].key, 0);
                freenull(keys[index].key);
            }
            keys[index].nref = 0;
            keys[index].next_free_key = 0;
        }
        freenull(keys[0].key); // "main"
        first_free_key = 0;
        keycnt         = 1;
    }
}

#if defined(WARN_TODO)
#warning useless return value - always 0
#endif
char *gb_abort_entry(GBDATA *gbd) { // @@@ result is always 0
    GB_ARRAY_FLAGS(gbd).flags = gbd->flags.saved_flags;

    if (gbd->is_entry()) {
        GBENTRY *gbe = gbd->as_entry();
        if (gbe->get_oldData()) {
            if (gbe->type() >= GB_BITS) {
                gb_uncache(gbe);
                gbe->free_data();
            }
            gb_abortdata(gbe);
        }
    }
    return 0;
}

// ---------------------
//      Transactions

void gb_abort_transaction_local_rek(GBDATA*& gbd) {
    // delete created, undo changed
    GB_CHANGE change = (GB_CHANGE)GB_ARRAY_FLAGS(gbd).changed;

    switch (change) {
        case GB_UNCHANGED:
            break;

        case GB_CREATED:
            GB_PUT_SECURITY_DELETE(gbd, 0);
            gb_delete_entry(gbd);
            break;

        case GB_DELETED:
            GB_ARRAY_FLAGS(gbd).changed = GB_UNCHANGED;
            // fall-through
        default:
            if (gbd->is_container()) {
                GBCONTAINER    *gbc = gbd->as_container();
                gb_header_list *hls = GB_DATA_LIST_HEADER(gbc->d);

                for (int index = 0; index < gbc->d.nheader; index++) {
                    GBDATA *gb = GB_HEADER_LIST_GBD(hls[index]);
                    if (gb) gb_abort_transaction_local_rek(gb);
                }
            }
            gb_abort_entry(gbd);
            break;
    }
}

GB_ERROR gb_commit_transaction_local_rek(GBDATA*& gbd, long mode, int *pson_created) {
    // goes to header: __ATTR__USERESULT

    /* commit created / delete deleted
     *   mode   0   local     = server    or
     *              begin trans in client or
     *              commit_client_in_server
     *   mode   1   remote    = client
     *   mode   2   remote    = client (only send updated data)
     */

    GB_MAIN_TYPE *Main        = GB_MAIN(gbd);
    GB_CHANGE     change      = (GB_CHANGE)GB_ARRAY_FLAGS(gbd).changed;
    int           son_created = 0;
    GB_ERROR      error;

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
            gb_delete_entry(gbd);
            return 0;

        case GB_CREATED:
            if (mode) {
                if (!gbd->flags2.update_in_server) {
                    if (gbd->server_id) goto gb_changed_label;
                    // already created, do only a change
                    error = gbcmc_sendupdate_create(gbd);
                    if (gbd->is_container()) {
                        gb_set_update_in_server_flags(gbd->as_container());
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
                gbd->as_container()->header_update_date = Main->clock;
            }
            goto gb_commit_do_callbacks;

        case GB_NORMAL_CHANGE:
            if (mode) {
                if (!gbd->flags2.update_in_server) {
                  gb_changed_label:
                    int send_header = gbd->flags2.header_changed ? 1 : 0;
                    error           = gbcmc_sendupdate_update(gbd, send_header);
                    if (error) return error;
                    gbd->flags2.update_in_server = 1;
                }
            }
            else {
                _GB_CHECK_IN_UNDO_MODIFY(Main, gbd);
            }
            // fall-through

        default: // means GB_SON_CHANGED + GB_NORMAL_CHANGE

            if (gbd->is_container()) {
                GBCONTAINER    *gbc = gbd->as_container();
                gb_header_list *hls = GB_DATA_LIST_HEADER(gbc->d);

                int start, end;

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

                for (int index = start; index < end; index++) {
                    GBDATA *gb = GB_HEADER_LIST_GBD(hls[index]);
                    if (gb) {
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
                GB_CB_TYPE cbtype = son_created ? GB_CB_CHANGED_OR_SON_CREATED : GB_CB_CHANGED;
                gbd->create_extended();
                gbd->touch_update(Main->clock);
                if (gbd->flags2.header_changed) {
                    gbd->as_container()->header_update_date = Main->clock;
                }

                Main->trigger_change_callbacks(gbd, cbtype);

                GB_FREE_TRANSACTION_SAVE(gbd);
            }
    }

    return 0;
}
