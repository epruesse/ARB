#include <stdio.h>
#include <stdlib.h>

#include <string.h>
/* #include <malloc.h> */

/*#include "arbdb.h"*/
#include "adlocal.h"

#ifndef __cplusplus
void ad_use(int dummy, ...)  {
    dummy = 0;
}
#endif

/********************************************************************************************
                    GB data manangement
********************************************************************************************/
void gb_touch_entry(GBDATA * gbd, GB_CHANGED val) {
    register GBCONTAINER *gbc;
    register GBCONTAINER *gbc_father;

    gbd->flags2.update_in_server = 0;
    if ( val > (GB_CHANGED)(int)GB_ARRAY_FLAGS(gbd).changed) {
        GB_ARRAY_FLAGS(gbd).changed = val;
        GB_ARRAY_FLAGS(gbd).ever_changed = 1;
    }
    gbc = GB_FATHER(gbd);

    if ((!gbc->index_of_touched_one_son) || gbc->index_of_touched_one_son == gbd->index+1) {
        gbc->index_of_touched_one_son = gbd->index+1;
    }else{
        gbc->index_of_touched_one_son = -1;
    }

    while ((gbc_father=GB_FATHER(gbc))!=NULL)
    {
        if ( (!gbc_father->index_of_touched_one_son) || gbc_father->index_of_touched_one_son == gbc->index+1 ) {
            gbc_father->index_of_touched_one_son = gbc->index+1;
        }else{
            gbc_father->index_of_touched_one_son = -1;
        }

        if (gbc->flags2.update_in_server) {
            gbc->flags2.update_in_server = 0;
        } else {
            if (GB_ARRAY_FLAGS(gbc).changed >= (unsigned int)gb_son_changed)
                return;
        }
        if (gb_son_changed > (int)GB_ARRAY_FLAGS(gbc).changed) {
            GB_ARRAY_FLAGS(gbc).changed = gb_son_changed;
            GB_ARRAY_FLAGS(gbc).ever_changed = 1;
        }
        gbc = gbc_father;
    }
}

void gb_touch_header(GBCONTAINER *gbc)
{
    gbc->flags2.header_changed = 1;
    gb_touch_entry((GBDATA*)gbc, gb_changed);
}


void
gb_untouch_children(GBCONTAINER * gbc)
{
    register GBDATA *gbd;
    register int    index, start, end;
    register GB_CHANGED changed;
    struct gb_header_list_struct *header = GB_DATA_LIST_HEADER(gbc->d);

    if (gbc->index_of_touched_one_son > 0) {
        start = (int)gbc->index_of_touched_one_son-1;
        end = start + 1;
    } else {
        if (!gbc->index_of_touched_one_son){
            start = end = 0;
        }else{
            start = 0;
            end = gbc->d.nheader;
        }
    }

    for (index = start; index < end; index++)
    {
        if ((gbd = GB_HEADER_LIST_GBD(header[index]))!=NULL)
        {
            if (    (changed = (GB_CHANGED)header[index].flags.changed) &&
                    (changed < gb_deleted)  )
            {
                header[index].flags.changed = gb_not_changed;
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

void gb_untouch_me(GBDATA * gbc)
{
    GB_DATA_LIST_HEADER(GB_FATHER(gbc)->d)[gbc->index].flags.changed = gb_not_changed;
    if (GB_TYPE(gbc) == GB_DB){
        gbc->flags2.header_changed = 0;
        ((GBCONTAINER *)gbc)->index_of_touched_one_son = 0;
    }
}

void gb_set_update_in_server_flags(GBCONTAINER * gbc)
{
    int             index;
    GBDATA         *gbd;

    for (index = 0; index < gbc->d.nheader; index++) {
        if ((gbd = GBCONTAINER_ELEM(gbc,index))!=NULL) {
            if (GB_TYPE(gbd) == GB_DB) {
                gb_set_update_in_server_flags((GBCONTAINER *) gbd);
            }
            gbd->flags2.update_in_server = 1;
        }
    }
}

void gb_create_header_array(GBCONTAINER *gbc, int size){
    /* creates or resizes an old array to children */
    struct gb_header_list_struct *nl, *ol;

    if (size <= gbc->d.headermemsize) return;
    if (!size) return;
    if (size > 10) size++;
    if (size > 30) size = size*3/2;
    nl = (struct gb_header_list_struct *)
        gbm_get_mem(sizeof(struct gb_header_list_struct)*size,GBM_HEADER_INDEX);

    if ((ol=GB_DATA_LIST_HEADER(gbc->d))!=NULL)
    {
        int     idx,
            maxidx = gbc->d.headermemsize; /* ???: oder ->d.nheader */

        for (idx=0; idx<maxidx; idx++)
        {
            GBDATA *gbd = GB_HEADER_LIST_GBD(ol[idx]);
            nl[idx].flags =  ol[idx].flags;

            if (gbd)
            {
                ad_assert(gbd->server_id==GBTUM_MAGIC_NUMBER || GB_read_clients(gbd)<0); /* or I am a client */
                SET_GB_HEADER_LIST_GBD(nl[idx],gbd);
            }
        }

        gbm_free_mem((char *)ol,
                     sizeof(struct gb_header_list_struct)*gbc->d.headermemsize,
                     GBM_HEADER_INDEX );
    }

    gbc->d.headermemsize = size;
    SET_GB_DATA_LIST_HEADER(gbc->d,nl);
}

void gb_link_entry(GBCONTAINER* father, GBDATA * gbd, long index_pos)
{
    /* if index_pos == -1 -> to end of data;
       else special index position; error when data already exists in index pos */

    SET_GB_FATHER(gbd,father);
    if (father == NULL) {   /* 'main' entry in GB */
        return;
    }

    if (GB_TYPE(father) != GB_DB) {
        GB_internal_error(  "to read a database into a non database keyword %s,"
                            "probably %%%% is missing\n", GB_read_key_pntr((GBDATA*)father));
        return;
    }
    if ( index_pos < 0) {
        index_pos = father->d.nheader++;
    }else{
        if ( index_pos >= father->d.nheader){
            father->d.nheader = (int)index_pos+1;
        }
    }

    gb_create_header_array(father, (int)index_pos+1);

    if ( GBCONTAINER_ELEM(father,index_pos) ) {
        GB_internal_error("Index of Databaseentry used twice");
        index_pos = father->d.nheader++;
        gb_create_header_array(father, (int)index_pos+1);
    }
    gbd->index = index_pos;
    SET_GBCONTAINER_ELEM(father,index_pos,gbd);
    father->d.size++;
}

void gb_unlink_entry(GBDATA * gbd)
{
    GBCONTAINER *father = GB_FATHER(gbd);

    if (father)
    {
        int index_pos = (int)gbd->index;
        struct gb_header_list_struct *hls = &(GB_DATA_LIST_HEADER(father->d)[index_pos]);

        SET_GB_HEADER_LIST_GBD(*hls,NULL);
        hls->flags.key_quark = 0;
        hls->flags.changed = gb_deleted;
        hls->flags.ever_changed = 1;
        father->d.size--;
        SET_GB_FATHER(gbd,NULL);
    }
}

void gb_create_extended(GBDATA *gbd){
    int index;
    if (gbd->ext) return;
    index = GB_GBM_INDEX(gbd);
    gbd->ext = (struct gb_db_extended *)gbm_get_mem(
                                                    sizeof(struct gb_db_extended),
                                                    index);
}

struct gb_main_type *gb_make_gb_main_type(const char *path)
{
    struct gb_main_type *Main;

    Main = (struct gb_main_type *)gbm_get_mem(sizeof(struct gb_main_type),0);
    if (path) Main->path = GB_STRDUP((char*)path);
    Main->key_2_index_hash = GBS_create_hash(20000,0);
    Main->compression_mask = -1;        /* allow all compressions */
    gb_init_cache(Main);
    gb_init_undo_stack(Main);
    gb_init_ctype_table();
    return Main;
}

char *gb_destroy_main(struct gb_main_type *Main)
{
    if (Main->path) free(Main->path);
    gb_free_undo_stack(Main);
    gbm_free_mem((char *)Main,sizeof(struct gb_main_type),0);

    return 0;
}

/* inserts an object into the dabase hierarchy */
GBDATA   *gb_make_pre_defined_entry(    GBCONTAINER * father, GBDATA *gbd,
                                        long index_pos, GBQUARK keyq)
{
    GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(father);

    SET_GB_FATHER(gbd,father);
    if (Main->local_mode) {
        gbd->server_id = GBTUM_MAGIC_NUMBER;
    }
    if (Main->clock) {
        GB_CREATE_EXT(gbd);
        gbd->ext->creation_date = Main->clock;
    }

    gb_link_entry(father, gbd, index_pos);
    gb_write_index_key(father,gbd->index,keyq);

    return gbd;
}

void
gb_rename_entry(GBCONTAINER *gbc, const char *new_key) {
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


/* creates a terminal database object */
GBDATA         *
gb_make_entry(GBCONTAINER * father, const char *key, long index_pos, GBQUARK keyq, GB_TYPES type)
{
    GBDATA         *gbd;
    long            gbm_index;
    static char *buffer = 0;
    register char *p;
    GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(father);

    if (!keyq) keyq = gb_key_2_quark(Main,key);
    gbm_index = GB_QUARK_2_GBMINDEX(Main, keyq);
    gbd = (GBDATA *) gbm_get_mem(sizeof(GBDATA), gbm_index);
    GB_GBM_INDEX(gbd) = gbm_index;
    SET_GB_FATHER(gbd,father);

    switch(type)
    {
        case GB_STRING_SHRT:    type = GB_STRING;
        case GB_STRING:
            if (!buffer) buffer = GB_STRDUP("1234");
            p = buffer;
            while ( !(++(*p) )) { (*p)++;p++; if (!(*p)) break; }
            GB_SETSMDMALLOC(gbd,5,5,buffer);
            break;
        case GB_LINK:
            buffer[0] = ':';
            buffer[1] = 0;
            GB_SETSMDMALLOC(gbd,0,0,buffer);
            break;
        default:        break;
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
    else        gb_write_index_key(father,gbd->index,keyq);

    return gbd;
}

/* inserts an object into the dabase hierarchy */
GBCONTAINER   *gb_make_pre_defined_container(   GBCONTAINER * father, GBCONTAINER *gbd,
                                                long index_pos, GBQUARK keyq)
{
    GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(father);

    SET_GB_FATHER(gbd,father);
    gbd->main_idx = father->main_idx;

    if (Main->local_mode) gbd->server_id = GBTUM_MAGIC_NUMBER;
    if (Main->clock)
    {
        GB_CREATE_EXT((GBDATA *) gbd);
        gbd->ext->creation_date = Main->clock;
    }
    gb_link_entry(father, (GBDATA *) gbd, index_pos);
    gb_write_index_key(father,gbd->index,keyq);

    return gbd;
}


GBCONTAINER *gb_make_container(GBCONTAINER * father, const char *key, long index_pos, GBQUARK keyq)
{
    GBCONTAINER    *gbd;
    long            gbm_index;

    if (father)
    {
        GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(father);

        if (!keyq) keyq = gb_key_2_quark(Main,key);
        gbm_index = GB_QUARK_2_GBMINDEX(Main, keyq);
        gbd = (GBCONTAINER *) gbm_get_mem(sizeof(GBCONTAINER), gbm_index);
        GB_GBM_INDEX(gbd) = gbm_index;
        SET_GB_FATHER(gbd,father);
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
        else        gb_write_index_key(father,gbd->index,keyq);

        return gbd;
    }
    else    /* main entry */
    {
        gbd = (GBCONTAINER *) gbm_get_mem(sizeof(GBCONTAINER), 0);
        gbd->flags.type = GB_DB;
    }

    return gbd;
}

/** Reduce an entry to its absolute minimum and remove it from database */
void gb_pre_delete_entry(GBDATA *gbd){
    GB_MAIN_TYPE   *Main = GB_MAIN(gbd);
    long    type = GB_TYPE(gbd);

    struct gb_callback *cb, *cb2;
    long            gbm_index;
    gbm_index = GB_GBM_INDEX(gbd);
    for (cb = GB_GET_EXT_CALLBACKS(gbd); cb; cb = cb2) {
        gbd->ext->callback = 0;
        cb2 = cb->next;
        if (!gbd->ext->old && type != GB_DB){
            gb_save_extern_data_in_ts(gbd);
        }
        if (cb->type & GB_CB_DELETE) {
            gb_add_delete_callback_list(gbd, gbd->ext->old, cb->func, cb->clientdata);
        }
        gbm_free_mem((char *) cb, sizeof(struct gb_callback), gbm_index);
    }
    gb_write_key(gbd, 0);
    gb_unlink_entry(gbd);

    /* as soon as an entry is deleted, there is
       no need to keep track of the database entry
       within the server at the client side */
    if (!Main->local_mode && gbd->server_id) {
        GBS_write_hashi(Main->remote_hash, gbd->server_id, 0);
    }

    if (type>= GB_BITS && type < GB_DB) {
        gb_free_cache(Main,gbd);
    }
    GB_FREE_TRANSACTION_SAVE(gbd);
    _GB_DELETE_EXT(gbd, gbm_index);
}


void
gb_delete_entry(GBDATA * gbd)
{
    long            gbm_index;
    long    type = GB_TYPE(gbd);

    gbm_index = GB_GBM_INDEX(gbd);

    if (type == GB_DB) {
        int             index;
        GBDATA         *gbd2;
        GBCONTAINER    *gbc = ((GBCONTAINER *) gbd);
        for (index = 0; index < gbc->d.nheader; index++) {
            if ((gbd2 = GBCONTAINER_ELEM(gbc,index))!=NULL) {
                gb_delete_entry(gbd2);
            }
        };
    }
    gb_pre_delete_entry(gbd);

    /* Now what is left is the core database entry !!! */

    if (type == GB_DB) {
        GBCONTAINER    *gbc = ((GBCONTAINER *) gbd);
        struct gb_header_list_struct *hls;

        if ((hls=GB_DATA_LIST_HEADER(gbc->d))!=NULL){
            gbm_free_mem(   (char *)hls,
                            sizeof(struct gb_header_list_struct) *
                            gbc->d.headermemsize,GBM_HEADER_INDEX );
        }
        gbm_free_mem((char *) gbd, sizeof(GBCONTAINER), gbm_index);
    } else {
        if (type >= GB_BITS){
            GB_FREEDATA(gbd);
        }
        gbm_free_mem((char *) gbd, sizeof(GBDATA), gbm_index);
    }
}

/********************************************************************************************
                    Data Storage
********************************************************************************************/
/******************** Does not increment the refcounter ********************/
struct gb_transaction_save *gb_new_gb_transaction_save(GBDATA *gbd){
    struct gb_transaction_save *ts;

    ts = (struct gb_transaction_save *)gbm_get_mem(sizeof(struct gb_transaction_save),GBM_CB_INDEX);

    ts->flags = gbd->flags;
    ts->flags2 = gbd->flags2;

    if (gbd->flags2.extern_data)
    {
        ts->info.ex.data = GB_EXTERN_DATA_DATA(gbd->info.ex);
        ts->info.ex.memsize = gbd->info.ex.memsize;
        ts->info.ex.size = gbd->info.ex.size;
    }
    else
    {
        GB_MEMCPY(&(ts->info), &(gbd->info), sizeof(gbd->info));
    }

    ts->refcount = 1;

    return ts;
}

void gb_add_ref_gb_transaction_save(struct gb_transaction_save *ts){
    if (!ts) return;
    ts->refcount ++;
}

void gb_del_ref_gb_transaction_save(struct gb_transaction_save *ts){
    if (!ts) return;
    ts->refcount --;
    if (ts->refcount <=0) {     /* no more references !!!! */
        if (ts->flags2.extern_data) {
            if (ts->info.ex.data) {
                gbm_free_mem(ts->info.ex.data,
                             ts->info.ex.memsize,
                             ts->flags2.gbm_index);
            }
        }
        gbm_free_mem((char*)ts,
                     sizeof(struct gb_transaction_save),
                     GBM_CB_INDEX);
    }
}

/* remove reference to undo entry and set extern pointer to zero */
void gb_del_ref_and_extern_gb_transaction_save(struct gb_transaction_save *ts){
    if (ts->flags2.extern_data) {
        ts->info.ex.data = 0;
    }
    gb_del_ref_gb_transaction_save(ts);
}

void gb_abortdata(GBDATA *gbd)
{
    struct gb_transaction_save *old;

    GB_INDEX_CHECK_OUT(gbd);
    old = gbd->ext->old;
    ad_assert(old!=0);

    gbd->flags = old->flags;
    gbd->flags2 = old->flags2;

    if (old->flags2.extern_data)
    {
        SET_GB_EXTERN_DATA_DATA(gbd->info.ex,old->info.ex.data);
        gbd->info.ex.memsize = old->info.ex.memsize;
        gbd->info.ex.size = old->info.ex.size;
    }
    else
    {
        GB_MEMCPY(&(gbd->info), &(old->info),sizeof(old->info));
    }
    gb_del_ref_and_extern_gb_transaction_save(old);
    gbd->ext->old = NULL;

    GB_INDEX_CHECK_IN(gbd);
}


void gb_save_extern_data_in_ts(GBDATA *gbd){
    /* Saves gbd->info into gbd->ext->old
     *  destroys gbd->info !!!!
     *  dont call with GBCONTAINER */
    GB_CREATE_EXT(gbd);
    GB_INDEX_CHECK_OUT(gbd);
    if (gbd->ext->old || (GB_ARRAY_FLAGS(gbd).changed == gb_created)){
        GB_FREEDATA(gbd);
    }else{
        gbd->ext->old = gb_new_gb_transaction_save(gbd);
        SET_GB_EXTERN_DATA_DATA(gbd->info.ex,0);
    }
}


/********************************************************************************************
                    Key Management
********************************************************************************************/

/********** set the key quark of an database field
        check for indexing data field ***********/

char *gb_write_index_key(GBCONTAINER *father, long index, GBQUARK new_index)
{
    GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(father);
    struct gb_header_list_struct *hls = GB_DATA_LIST_HEADER(father->d);
    GBQUARK old_index = hls[index].flags.key_quark;

    GBCONTAINER *gfather;
    Main->keys[old_index].nref--;
    Main->keys[new_index].nref++;

    if (Main->local_mode)
    {
        GBDATA *gbd = GB_HEADER_LIST_GBD(hls[index]);

        if (gbd && (GB_TYPE(gbd) == GB_STRING || GB_TYPE(gbd) == GB_LINK))
        {
            struct gb_index_files_struct *ifs = 0;

            GB_INDEX_CHECK_OUT(gbd);
            gbd->flags2.tisa_index = 0;
            if ( (gfather = GB_FATHER(father)))
            {
                for (   ifs = GBCONTAINER_IFS(gfather); ifs;
                        ifs = GB_INDEX_FILES_NEXT(ifs))
                {
                    if (ifs->key == new_index) break;
                }
            }
            hls[index].flags.key_quark = new_index;
            if (ifs) gb_index_check_in(gbd);

            return 0;
        }
    }

    hls[index].flags.key_quark = new_index;
    return 0;
}

char *gb_write_key(GBDATA *gbd,const char *s)
{
    GBQUARK new_index = 0;
    register GB_MAIN_TYPE *Main = GB_MAIN(gbd);

    if (s){
        new_index = (int)GBS_read_hash(Main->key_2_index_hash,s);
        if (!new_index) {   /* create new index */
            new_index = (int)gb_create_key(Main,s,GB_TRUE);
        }
    }
    return gb_write_index_key(GB_FATHER(gbd), gbd->index, new_index);
}

void gb_create_key_array(GB_MAIN_TYPE *Main, int index){
    if (index >= Main->sizeofkeys) {
        Main->sizeofkeys = index*3/2+1;
        if (Main->keys) {
            int i;
            Main->keys = (struct gb_key_struct *)
                realloc((MALLOC_T)Main->keys,
                        sizeof(struct gb_key_struct) * (size_t)Main->sizeofkeys);
            memset( (char *)&(Main->keys[Main->keycnt]),
                    0,
                    sizeof(struct gb_key_struct) * (size_t)
                    (Main->sizeofkeys - Main->keycnt));
            for (i= Main->keycnt; i < Main->sizeofkeys; i++){
                Main->keys[i].compression_mask = -1;
            }
        }else{
            Main->sizeofkeys = 1000;
            Main->keys = (struct gb_key_struct *)
                GB_calloc(sizeof(struct gb_key_struct) ,(size_t)Main->sizeofkeys);
        }
    }
}

long gb_create_key(GB_MAIN_TYPE *Main, const char *s, GB_BOOL create_gb_key) {
    long index;
    if ( Main->first_free_key ) {
        index = Main->first_free_key;
        Main->first_free_key = Main->keys[index].next_free_key;
        Main->keys[index].next_free_key = 0;
    }else{
        index = Main->keycnt++;
        gb_create_key_array(Main,(int)index+1);
    }
    if (!Main->local_mode) {
        long test_index = gbcmc_key_alloc((GBDATA *)Main->data,s);
        if (test_index != index) {
            GB_internal_error("Database corrupt, Allocating Quark '%s' in Server failed",s);
            GB_CORE;
        }
    }
    Main->keys[index].nref = 0;

    if (s){
        Main->keys[index].key = GB_STRDUP(s);
        GBS_write_hash(Main->key_2_index_hash,s,index);
        if (Main->gb_key_data && create_gb_key){
            gb_load_single_key_data((GBDATA *)Main->data,(GBQUARK)index);
            /* Warning: starts a big recursion */
            if (!Main->local_mode){ /* send new gb_key to server, needed for searching */
                GB_update_server((GBDATA *)Main->data);
            }
        }
    }


    Main->key_clock = Main->clock;
    return index;
}

void gb_free_all_keys(GB_MAIN_TYPE *Main) {
    long index;
    if (!Main->keys) return;
    for (index = 1; index < Main->keycnt; index++) {
        if (Main->keys[index].key){
            GBS_write_hash(Main->key_2_index_hash,Main->keys[index].key,0);
            free(Main->keys[index].key);
        }
        Main->keys[index].key = 0;
        Main->keys[index].nref = 0;
        Main->keys[index].next_free_key = 0;
    }
    Main->first_free_key = 0;
    Main->keycnt = 1;
}

char *gb_abort_entry(GBDATA *gbd){
    int type = GB_TYPE(gbd);
    GB_ARRAY_FLAGS(gbd).flags = gbd->flags.saved_flags;
    if (type == GB_DB){
        return 0;
    }else{
        if (GB_GET_EXT_OLD_DATA(gbd)) {
            if ( (type >= GB_BITS) ) {
                gb_free_cache(GB_MAIN(gbd),gbd);
                GB_FREEDATA(gbd);
            }
            gb_abortdata(gbd);
        }
    }
    return 0;
}

/********************************************************************************************
                    Transactions
********************************************************************************************/
int
gb_abort_transaction_local_rek(GBDATA *gbd, long mode)
     /*     delete created
        undo    changed
    */
{
    GBDATA *gb;
    enum gb_key_types type;
    GB_CHANGED change = (GB_CHANGED)GB_ARRAY_FLAGS(gbd).changed;

    switch (change) {
        case gb_not_changed:    return 0;
        case gb_created:    GB_PUT_SECURITY_DELETE(gbd,0);
            gb_delete_entry(gbd);
            return 1;
        case gb_deleted:    GB_ARRAY_FLAGS(gbd).changed = gb_not_changed;
        default:
            type = (GB_TYPES)GB_TYPE(gbd);
            if (type == GB_DB)
            {
                register int index;
                GBCONTAINER *gbc = (GBCONTAINER *)gbd;
                struct gb_header_list_struct *hls = GB_DATA_LIST_HEADER(gbc->d);

                for (index = 0; index < gbc->d.nheader; index++)
                {
                    if ((gb = GB_HEADER_LIST_GBD(hls[index]))!=NULL)
                    {
                        gb_abort_transaction_local_rek(gb,mode);
                    }
                }
            }
            gb_abort_entry(gbd);
    }
    return 0;
}

GB_ERROR gb_commit_transaction_local_rek(GBDATA * gbd, long mode,int *pson_created)
     /*
 *  commit created
 *  delete  deleted
 *   mode   0   local = server  or begin trans in client or commit_client_in_server
 *   mode   1   remote = client
 *   mode   2   remote = client send only updated data
 */
{
    GBDATA         *gb;
    GB_MAIN_TYPE    *Main = GB_MAIN(gbd);
    GB_TYPES    type;
    GB_ERROR error;
    struct gb_callback *cb;
    GB_CHANGED      change = (GB_CHANGED)GB_ARRAY_FLAGS(gbd).changed;
    int send_header;
    int son_created = 0;

    type = (GB_TYPES)GB_TYPE(gbd);
    switch (change) {
        case gb_not_changed:
            return 0;
        case gb_deleted:
            GB_PUT_SECURITY_DELETE(gbd, 0);
            if (mode) {
                if (!gbd->flags2.update_in_server) {
                    error = gbcmc_sendupdate_delete(gbd);
                    if (error)
                        return error;
                    gbd->flags2.update_in_server = 1;
                }
                if (mode == 2) return 0;
            } else {
                gbcms_add_to_delete_list(gbd);
                _GB_CHECK_IN_UNDO_DELETE(Main,gbd);
                return 0;
            }
            gb_delete_entry(gbd);
            return 0;
        case gb_created:
            if (mode) {
                if (!gbd->flags2.update_in_server) {
                    if (gbd->server_id) goto gb_changed_label;
                    /* already created, do only a change */
                    error = gbcmc_sendupdate_create(gbd);
                    if (type == GB_DB) {
                        gb_set_update_in_server_flags(((GBCONTAINER *)gbd));
                        /* set all childrens update_in_server flags */
                    }
                    gbd->flags2.update_in_server = 1;
                    if (error)  return error;
                }
                if (mode == 2) return 0;
            }else{
                _GB_CHECK_IN_UNDO_CREATE(Main,gbd);
            }
            if (pson_created) {
                *pson_created  = 1;
            }

            if (gbd->flags2.header_changed == 1) {
                ((GBCONTAINER*)gbd)->header_update_date = Main->clock;
            }
            goto gb_commit_do_callbacks;

        case gb_changed:
            if (mode) {
                if (!gbd->flags2.update_in_server) {
                gb_changed_label:;
                    send_header = 0;
                    if (gbd->flags2.header_changed) send_header = 1;
                    error = gbcmc_sendupdate_update(gbd,send_header);
                    if (error) return error;
                    gbd->flags2.update_in_server = 1;
                }
            }else{
                _GB_CHECK_IN_UNDO_MODIFY(Main,gbd);
            }
        default:        /* means gb_son_changed + changed */

            if (type == GB_DB)
            {
                GBCONTAINER *gbc = (GBCONTAINER *)gbd;
                int index, start, end;
                struct gb_header_list_struct *hls = GB_DATA_LIST_HEADER(gbc->d);

                if (gbc->index_of_touched_one_son>0) {
                    start = (int)gbc->index_of_touched_one_son-1;
                    end = start+1;
                }else{  if (!gbc->index_of_touched_one_son){ start = end = 0;
                }else{  start = 0; end = gbc->d.nheader; }
                }

                for (index = start; index < end; index++)
                {
                    if ((gb = GB_HEADER_LIST_GBD(hls[index]))!=NULL)
                    {
                        if (!hls[index].flags.changed) continue;
                        error = gb_commit_transaction_local_rek(gb,mode,&son_created);
                        if (error) return error;
                    }
                }

                if (mode) gbd->flags2.update_in_server = 1;
            }
    gb_commit_do_callbacks:
            if (mode == 2) {    /* update server; no callbacks */
                gbd->flags2.update_in_server = 1;
            }else{
                GB_CB_TYPE gbtype = GB_CB_CHANGED;
                if (son_created) {
                    gbtype = (GB_CB_TYPE)(GB_CB_SON_CREATED | GB_CB_CHANGED);
                }
                GB_CREATE_EXT(gbd);
                gbd->ext->update_date = Main->clock;
                if (gbd->flags2.header_changed ) {
                    ((GBCONTAINER*)gbd)->header_update_date = Main->clock;
                }

                for (cb = GB_GET_EXT_CALLBACKS(gbd); cb; cb = cb->next) {
                    if (cb->type & (GB_CB_CHANGED|GB_CB_SON_CREATED)) {
                        gb_add_changed_callback_list(gbd,gbd->ext->old,gbtype,cb->func,cb->clientdata);
                    }
                }

                GB_FREE_TRANSACTION_SAVE(gbd);
            }
    }/*switch*/
    return 0;
}

