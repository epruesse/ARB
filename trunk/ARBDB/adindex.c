#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "adlocal.h"
/*#include "arbdb.h"*/
#include "adlundo.h"

#define GB_INDEX_FIND(gbf,ifs,quark)                                        \
    for (ifs = GBCONTAINER_IFS(gbf); ifs; ifs = GB_INDEX_FILES_NEXT(ifs)) { \
        if (ifs->key == quark) break;                                       \
    }

#define GB_CALC_HASH_INDEX(string,index) {                                      \
        register const char *_ptr = string;                                     \
        register int _i;                                                        \
        index = 0xffffffffL; while ((_i = *(_ptr++))){                          \
            index = crctab[((int) index ^ toupper(_i)) & 0xff] ^ (index >> 8);  \
        }                                                                       \
    }

/* write field in index table */
char *gb_index_check_in(GBDATA *gbd)
{
    register struct gb_index_files_struct *ifs;
    register GBQUARK quark;
    register unsigned long index;
    char *data;
    GBCONTAINER *gfather;

    gfather = GB_GRANDPA(gbd);
    if (!gfather)   return 0;

    quark = GB_KEY_QUARK(gbd);
    GB_INDEX_FIND(gfather,ifs,quark);
    if (!ifs) return 0;     /* This key is not indexed */

    if (GB_TYPE(gbd) != GB_STRING && GB_TYPE(gbd) != GB_LINK) return 0;

    if (gbd->flags2.is_indexed)
    {
        GB_internal_error("Double checked in");
        return 0;
    }

    data = GB_read_char_pntr(gbd);
    GB_CALC_HASH_INDEX(data,index);
    index = index % ifs->hash_table_size;
    ifs->nr_of_elements++;
    {
        struct gb_if_entries *ifes;
        GB_REL_IFES *entries = GB_INDEX_FILES_ENTRIES(ifs);

        ifes = (struct gb_if_entries *)gbm_get_mem(sizeof(struct gb_if_entries),
                                                   GB_GBM_INDEX(gbd));

        SET_GB_IF_ENTRIES_NEXT(ifes,GB_ENTRIES_ENTRY(entries,index));
        SET_GB_IF_ENTRIES_GBD(ifes,gbd);
        SET_GB_ENTRIES_ENTRY(entries,index,ifes);
    }
    gbd->flags2.tisa_index = 1;
    gbd->flags2.is_indexed = 1;
    return 0;
}

/* remove entry from index table */
char *gb_index_check_out(GBDATA *gbd)
{
    register struct gb_index_files_struct *ifs;
    register GBQUARK quark;
    GBCONTAINER *gfather;
    char *data;
    register unsigned long index;
    register struct gb_if_entries *ifes;
    struct gb_if_entries *ifes2;
    GB_REL_IFES *entries;

    if (!gbd->flags2.is_indexed) return 0;
    gbd->flags2.is_indexed = 0;

    gfather = GB_GRANDPA(gbd);
    quark = GB_KEY_QUARK(gbd);
    GB_INDEX_FIND(gfather,ifs,quark);
    if (!ifs) {
        GB_internal_error("gb_index_check_out ifs not found");
        return 0;       /* This key is not indexed */
    }
    data = GB_read_char_pntr(gbd);
    GB_CALC_HASH_INDEX(data,index);
    index = index % ifs->hash_table_size;
    ifes2 = 0;
    entries = GB_INDEX_FILES_ENTRIES(ifs);

    for (   ifes = GB_ENTRIES_ENTRY(entries,index);
            ifes;
            ifes = GB_IF_ENTRIES_NEXT(ifes))
    {
        if (gbd == GB_IF_ENTRIES_GBD(ifes)) {       /* entry found */
            if (ifes2) {
                SET_GB_IF_ENTRIES_NEXT(ifes2, GB_IF_ENTRIES_NEXT(ifes));
            }else{
                SET_GB_ENTRIES_ENTRY(entries,index,GB_IF_ENTRIES_NEXT(ifes));
            }
            ifs->nr_of_elements--;
            gbm_free_mem((char *)ifes,
                         sizeof(struct gb_if_entries),
                         GB_GBM_INDEX(gbd));
            return 0;
        }
        ifes2 = ifes;
    }
    GB_internal_error("gb_index_check_out item not found in index list");
    return 0;
}

/* Create an index for a big database, in this version hash tables are used,
   Collisions are avoided by using linked lists */
GB_ERROR GB_create_index(GBDATA *gbd, const char *key, long estimated_size)
{
    GBQUARK key_quark;
    register struct gb_index_files_struct *ifs;
    GBCONTAINER *gbc;
    GBDATA *gbf;
    register GBDATA *gb2;

    if (GB_TYPE(gbd) != GB_DB)
        return GB_export_error("GB_create_index used on non CONTAINER Type");
    gbc = (GBCONTAINER *)gbd;
    if (GB_read_clients(gbd) <0) return GB_export_error("No index tables in clients allowed");

    key_quark = GB_key_2_quark(gbd,key);
    GB_INDEX_FIND(gbc,ifs,key_quark);
    if (ifs) return 0;
    ifs = (struct gb_index_files_struct *)GB_calloc(sizeof(struct gb_index_files_struct),1);
    SET_GB_INDEX_FILES_NEXT(ifs,GBCONTAINER_IFS(gbc));
    SET_GBCONTAINER_IFS(gbc,ifs);
    ifs->key= key_quark;
    ifs->hash_table_size = estimated_size;
    ifs->nr_of_elements = 0;
    SET_GB_INDEX_FILES_ENTRIES(ifs, (struct gb_if_entries **)GB_calloc(sizeof(void *),(int)ifs->hash_table_size));

    for (   gbf = GB_find_sub_by_quark(gbd,-1,0,0);
            gbf;
            gbf = GB_find_sub_by_quark(gbd,-1,0,gbf))
    {
        if (GB_TYPE(gbf) != GB_DB) continue;

        for (   gb2 = GB_find_sub_by_quark(gbf,key_quark,0,0);
                gb2;
                gb2 = GB_find_sub_by_quark(gbf,key_quark,0,gb2))
        {
            if (GB_TYPE(gb2) != GB_STRING && GB_TYPE(gb2) != GB_LINK) continue;
            gb_index_check_in(gb2);
        }
    }

    return 0;
}

/* find an entry in an hash table */
GBDATA *gb_index_find(GBCONTAINER *gbf, struct gb_index_files_struct *ifs, GBQUARK quark, const char *val, int after_index){
    register unsigned long index;
    register char *data;
    register struct gb_if_entries *ifes;
    GBDATA *result = 0;
    long    min_index;

    if (!ifs) {
        GB_INDEX_FIND(gbf,ifs,quark);
        if (!ifs) {
            GB_internal_error("gb_index_find called, but no index table found");
            return 0;
        }
    }

    GB_CALC_HASH_INDEX(val,index);
    index = index % ifs->hash_table_size;
    min_index = gbf->d.nheader;

    for (   ifes = GB_ENTRIES_ENTRY(GB_INDEX_FILES_ENTRIES(ifs),index);
            ifes;
            ifes = GB_IF_ENTRIES_NEXT(ifes))
    {
        GBDATA *igbd = GB_IF_ENTRIES_GBD(ifes);
        GBCONTAINER *ifather = GB_FATHER(igbd);

        if ( ifather->index < after_index) continue;
        if ( ifather->index >= min_index) continue;
        data = GB_read_char_pntr(igbd);
        if (!GBS_string_cmp(data,val,1) ) {     /* entry found */
            result    = igbd;
            min_index = ifather->index;
        }
    }
    return result;
}


/*****************************************************************************************
        UNDO    functions
******************************************************************************************/
/* How they work:
   There are three undo stacks:
   GB_UNDO_NONE    no undo
   GB_UNDO_UNDO    normal undo stack
   GB_UNDO_REDO    redo stack
*/

/*****************************************************************************************
        UNDO    internal functions
******************************************************************************************/

char *gb_set_undo_type(GBDATA *gb_main, GB_UNDO_TYPE type){
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    Main->undo_type = type;
    return 0;
}

/** mallocs the main structures to control undo/redo */

void g_b_add_size_to_undo_entry(struct g_b_undo_entry_struct *ue, long size){
    ue->sizeof_this += size;            /* undo entry */
    ue->father->sizeof_this += size;        /* one undo */
    ue->father->father->sizeof_this += size;    /* all undos */
}

struct g_b_undo_entry_struct *new_g_b_undo_entry_struct(struct g_b_undo_struct *u){
    struct g_b_undo_entry_struct *ue = (struct g_b_undo_entry_struct *)gbm_get_mem(
                                                                                   sizeof(struct g_b_undo_entry_struct), GBM_UNDO);
    ue->next = u->entries;
    ue->father = u;
    u->entries = ue;
    g_b_add_size_to_undo_entry(ue,sizeof(struct g_b_undo_entry_struct));
    return ue;
}



void gb_init_undo_stack(struct gb_main_type *Main){
    Main->undo = (struct g_b_undo_mgr_struct *)GB_calloc(sizeof(struct g_b_undo_mgr_struct),1);
    Main->undo->max_size_of_all_undos = GB_MAX_UNDO_SIZE;
    Main->undo->u = (struct g_b_undo_header_struct *) GB_calloc(sizeof(struct g_b_undo_header_struct),1);
    Main->undo->r = (struct g_b_undo_header_struct *) GB_calloc(sizeof(struct g_b_undo_header_struct),1);
}

void delete_g_b_undo_entry_struct(struct g_b_undo_entry_struct *entry){
    switch (entry->type) {
        case GB_UNDO_ENTRY_TYPE_MODIFY:
        case GB_UNDO_ENTRY_TYPE_MODIFY_ARRAY:
            {
                if (entry->d.ts) {
                    gb_del_ref_gb_transaction_save(entry->d.ts);
                }
            }
        default:
            break;
    }
    gbm_free_mem((char *)entry,sizeof (struct g_b_undo_entry_struct),GBM_UNDO);
}

void delete_g_b_undo_struct(struct g_b_undo_struct *u){
    struct g_b_undo_entry_struct *a,*next;
    for (a = u->entries; a; a = next){
        next = a->next;
        delete_g_b_undo_entry_struct(a);
    }
    free((char *)u);
}

void delete_g_b_undo_header_struct(struct g_b_undo_header_struct *uh){
    struct g_b_undo_struct *a,*next=0;
    for ( a= uh->stack; a; a = next){
        next = a->next;
        delete_g_b_undo_struct(a);
    }
    free((char *)uh);
}

/********************   check size *****************************/

char *g_b_check_undo_size2(struct g_b_undo_header_struct *uhs, long size, long max_cnt){
    long csize = 0;
    long ccnt = 0;
    struct g_b_undo_struct *us;

    for (us = uhs->stack; us && us->next ; us = us->next){
        csize += us->sizeof_this;
        ccnt ++;
        if ( (  (csize + us->next->sizeof_this) > size) ||
             (ccnt >= max_cnt ) ){ /* delete the rest */
            struct g_b_undo_struct *a,*next=0;

            for ( a = us->next; a; a = next){
                next = a->next;
                delete_g_b_undo_struct(a);
            }
            us->next = 0;
            uhs->sizeof_this = csize;
            break;
        }
    }
    return 0;
}

char *g_b_check_undo_size(GB_MAIN_TYPE *Main){
    char *error = 0;
    long maxsize = Main->undo->max_size_of_all_undos;
    error = g_b_check_undo_size2(Main->undo->u, maxsize/2,GB_MAX_UNDO_CNT);
    if (error) return error;
    error = g_b_check_undo_size2(Main->undo->r, maxsize/2,GB_MAX_REDO_CNT);
    if (error) return error;
    return 0;
}


void gb_free_undo_stack(struct gb_main_type *Main){
    delete_g_b_undo_header_struct(Main->undo->u);
    delete_g_b_undo_header_struct(Main->undo->r);
    free((char *)Main->undo);
}

/*****************************************************************************************
        real undo (redo)
******************************************************************************************/

GB_ERROR g_b_undo_entry(GB_MAIN_TYPE *Main,struct g_b_undo_entry_struct *ue){
    GB_ERROR error = 0;
    Main = Main;
    switch (ue->type) {
        case GB_UNDO_ENTRY_TYPE_CREATED:
            error = GB_delete(ue->source);
            break;
        case GB_UNDO_ENTRY_TYPE_DELETED:
            {
                GBDATA *gbd = ue->d.gs.gbd;
                int type = GB_TYPE(gbd);
                if (type == GB_DB) {
                    gbd = (GBDATA *)gb_make_pre_defined_container((GBCONTAINER *)ue->source,(GBCONTAINER *)gbd,-1, ue->d.gs.key);
                }else{
                    gbd = gb_make_pre_defined_entry((GBCONTAINER *)ue->source,gbd,-1, ue->d.gs.key);
                }
                GB_ARRAY_FLAGS(gbd).flags = ue->flag;
                gb_touch_header(GB_FATHER(gbd));
                gb_touch_entry((GBDATA *)gbd,gb_created);
            }
            break;
        case GB_UNDO_ENTRY_TYPE_MODIFY_ARRAY:
        case GB_UNDO_ENTRY_TYPE_MODIFY:
            {
                GBDATA *gbd = ue->source;
                int type  = GB_TYPE(gbd);
                if (type == GB_DB) {

                }else{
                    gb_save_extern_data_in_ts(gbd); /* check out and free string */
                    gb_assert(ue->d.ts);
                    
                    gbd->flags              = ue->d.ts->flags;
                    gbd->flags2.extern_data = ue->d.ts->flags2.extern_data;

                    GB_MEMCPY(&gbd->info,&ue->d.ts->info,sizeof(gbd->info)); /* restore old information */
                    if (type >= GB_BITS) {
                        if (gbd->flags2.extern_data){
                            SET_GB_EXTERN_DATA_DATA(gbd->info.ex, ue->d.ts->info.ex.data); /* set relative pointers correctly */
                        }

                        gb_del_ref_and_extern_gb_transaction_save(ue->d.ts);
                        ue->d.ts = 0;

                        GB_INDEX_CHECK_IN(gbd);
                    }

                }
                {
                    struct gb_header_flags *pflags = &GB_ARRAY_FLAGS(gbd);
                    if (pflags->flags != (unsigned)ue->flag){
                        GBCONTAINER *gb_father = GB_FATHER(gbd);
                        gbd->flags.saved_flags = pflags->flags;
                        pflags->flags = ue->flag;
                        if (GB_FATHER(gb_father)){
                            gb_touch_header(gb_father); /* dont touch father of main */
                        }
                    }
                }
                gb_touch_entry(gbd,gb_changed);
            }
            break;
        default:
            GB_internal_error("Undo stack corrupt:!!!");
            error = GB_export_error("shit 34345");
    }

    return error;
}



GB_ERROR g_b_undo(GB_MAIN_TYPE *Main, GBDATA *gb_main, struct g_b_undo_header_struct *uh){
    GB_ERROR error = 0;
    struct g_b_undo_struct *u;
    struct g_b_undo_entry_struct *ue,*next;
    if (!uh->stack) return GB_export_error("Sorry no more undos/redos available");

    GB_begin_transaction(gb_main);

    u=uh->stack;
    for (ue=u->entries; ue; ue = next) {
        next = ue->next;
        error = g_b_undo_entry(Main,ue);
        delete_g_b_undo_entry_struct(ue);
        u->entries = next;
        if (error) break;
    }
    uh->sizeof_this -= u->sizeof_this;  /* remove undo from list */
    uh->stack = u->next;
    delete_g_b_undo_struct(u);

    if (error){
        GB_abort_transaction(gb_main);
    }else{
        GB_commit_transaction(gb_main);
    }
    return error;
}

GB_CSTR g_b_read_undo_key_pntr(GB_MAIN_TYPE *Main, struct g_b_undo_entry_struct *ue){
    return Main->keys[ue->d.gs.key].key;
}

char *g_b_undo_info(GB_MAIN_TYPE *Main, GBDATA *gb_main, struct g_b_undo_header_struct *uh){
    void *res = GBS_stropen(1024);
    struct g_b_undo_struct *u;
    struct g_b_undo_entry_struct *ue;
    GBUSE(Main);GBUSE(gb_main);
    u=uh->stack;
    if (!u) return strdup("No more undos available");
    for (ue=u->entries; ue; ue = ue->next) {
        switch (ue->type) {
            case GB_UNDO_ENTRY_TYPE_CREATED:
                GBS_strcat(res,"Delete new entry: ");
                GBS_strcat(res,gb_read_key_pntr(ue->source));
                break;
            case GB_UNDO_ENTRY_TYPE_DELETED:
                GBS_strcat(res,"Rebuild deleted entry: ");
                GBS_strcat(res,g_b_read_undo_key_pntr(Main,ue));
                break;
            case GB_UNDO_ENTRY_TYPE_MODIFY_ARRAY:
            case GB_UNDO_ENTRY_TYPE_MODIFY:
                GBS_strcat(res,"Undo modified entry: ");
                GBS_strcat(res,gb_read_key_pntr(ue->source));
                break;
            default:
                break;
        }
        GBS_chrcat(res,'\n');
    }
    return GBS_strclose(res);
}

/*****************************************************************************************
        UNDO    exported  functions (to ARBDB)
******************************************************************************************/

/** start a new undoable transaction */
char *gb_set_undo_sync(GBDATA *gb_main)
{
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    char *error = g_b_check_undo_size(Main);
    struct g_b_undo_header_struct *uhs;
    if (error) return error;
    switch (Main->requested_undo_type) {    /* init the target undo stack */
        case GB_UNDO_UNDO:      /* that will undo but delete all redos */
            uhs         = Main->undo->u;
            break;
        case GB_UNDO_UNDO_REDO: uhs = Main->undo->u; break;
        case GB_UNDO_REDO:      uhs = Main->undo->r; break;
        case GB_UNDO_KILL:      gb_free_all_undos(gb_main);
        default:            uhs = 0;
    }
    if (uhs)
    {
        struct g_b_undo_struct *u = (struct g_b_undo_struct *) GB_calloc(sizeof(struct g_b_undo_struct) , 1);
        u->next = uhs->stack;
        u->father = uhs;
        uhs->stack = u;
        Main->undo->valid_u = u;
    }

    return gb_set_undo_type(gb_main,Main->requested_undo_type);
}

/* Remove all existing undos/redos */
char *gb_free_all_undos(GBDATA *gb_main){
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    struct g_b_undo_struct *a,*next;
    for ( a= Main->undo->r->stack; a; a = next){
        next = a->next;
        delete_g_b_undo_struct(a);
    }
    Main->undo->r->stack = 0;
    Main->undo->r->sizeof_this = 0;

    for ( a= Main->undo->u->stack; a; a = next){
        next = a->next;
        delete_g_b_undo_struct(a);
    }
    Main->undo->u->stack = 0;
    Main->undo->u->sizeof_this = 0;
    return 0;
}


/* called to finish an undoable section, called at end of gb_commit_transaction */
char *gb_disable_undo(GBDATA *gb_main){
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    struct g_b_undo_struct *u = Main->undo->valid_u;
    if (!u) return 0;
    if (!u->entries){       /* nothing to undo, just a read transaction */
        u->father->stack = u->next;
        delete_g_b_undo_struct(u);
    }else{
        if (Main->requested_undo_type == GB_UNDO_UNDO) {    /* remove all redos*/
            struct g_b_undo_struct *a,*next;
            for ( a= Main->undo->r->stack; a; a = next){
                next = a->next;
                delete_g_b_undo_struct(a);
            }
            Main->undo->r->stack = 0;
            Main->undo->r->sizeof_this = 0;
        }
    }
    Main->undo->valid_u = 0;
    return gb_set_undo_type(gb_main,GB_UNDO_NONE);
}

void gb_check_in_undo_create(GB_MAIN_TYPE *Main,GBDATA *gbd){
    struct g_b_undo_entry_struct *ue;
    if (!Main->undo->valid_u) return;
    ue = new_g_b_undo_entry_struct(Main->undo->valid_u);
    ue->type = GB_UNDO_ENTRY_TYPE_CREATED;
    ue->source = gbd;
    ue->gbm_index = GB_GBM_INDEX(gbd);
    ue->flag = 0;
}

void gb_check_in_undo_modify(GB_MAIN_TYPE *Main,GBDATA *gbd){
    long type = GB_TYPE(gbd);
    struct g_b_undo_entry_struct *ue;
    struct gb_transaction_save *old;

    if (!Main->undo->valid_u){
        GB_FREE_TRANSACTION_SAVE(gbd);
        return;
    }

    old = GB_GET_EXT_OLD_DATA(gbd);
    ue = new_g_b_undo_entry_struct(Main->undo->valid_u);
    ue->source = gbd;
    ue->gbm_index = GB_GBM_INDEX(gbd);
    ue->type = GB_UNDO_ENTRY_TYPE_MODIFY;
    ue->flag =      gbd->flags.saved_flags;

    if (type != GB_DB) {
        ue->d.ts = old;
        if (old) {
            gb_add_ref_gb_transaction_save(old);
            if (type >= GB_BITS && old->flags2.extern_data && old->info.ex.data ) {
                ue->type = GB_UNDO_ENTRY_TYPE_MODIFY_ARRAY;
                /* move external array from ts to undo entry struct */
                g_b_add_size_to_undo_entry(ue,old->info.ex.memsize);
            }
        }
    }
}

void gb_check_in_undo_delete(GB_MAIN_TYPE *Main,GBDATA *gbd, int deep){
    long type = GB_TYPE(gbd);
    struct g_b_undo_entry_struct *ue;
    if (!Main->undo->valid_u){
        gb_delete_entry(gbd);
        return;
    }

    if (type == GB_DB) {
        int             index;
        GBDATA         *gbd2;
        GBCONTAINER    *gbc = ((GBCONTAINER *) gbd);

        for (index = 0; (index < gbc->d.nheader); index++) {
            if (( gbd2 = GBCONTAINER_ELEM(gbc,index) )) {
                gb_check_in_undo_delete(Main,gbd2,deep+1);
            }
        };
    }else{
        GB_INDEX_CHECK_OUT(gbd);
        gbd->flags2.tisa_index = 0; /* never check in again */
    }
    gb_abort_entry(gbd);            /* get old version */

    ue = new_g_b_undo_entry_struct(Main->undo->valid_u);
    ue->type = GB_UNDO_ENTRY_TYPE_DELETED;
    ue->source = (GBDATA *)GB_FATHER(gbd);
    ue->gbm_index = GB_GBM_INDEX(gbd);
    ue->flag =      GB_ARRAY_FLAGS(gbd).flags;

    ue->d.gs.gbd = gbd;
    ue->d.gs.key = GB_KEY_QUARK(gbd);

    gb_pre_delete_entry(gbd);       /* get the core of the entry */

    if (type == GB_DB) {
        g_b_add_size_to_undo_entry(ue,sizeof(GBCONTAINER));
    }else{
        if (type >= GB_BITS && gbd->flags2.extern_data) {
            /* we have copied the data structures, now
               mark the old as deleted !!! */
            g_b_add_size_to_undo_entry(ue,GB_GETMEMSIZE(gbd));
        }
        g_b_add_size_to_undo_entry(ue,sizeof(GBDATA));
    }
}

/*****************************************************************************************
        UNDO    exported  functions (to USER)
******************************************************************************************/


/** define how to undo the next items, this function should be called just before opening
    a transaction, otherwise it's affect will be delayed
    posissible types are:   GB_UNDO_UNDO        enable undo
    *               GB_UNDO_NONE        disable undo
    *               GB_UNDO_KILL        disable undo and remove old undos !!
    */
GB_ERROR GB_request_undo_type(GBDATA *gb_main, GB_UNDO_TYPE type){
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    Main->requested_undo_type = type;
    if (!Main->local_mode) {
        if (type == GB_UNDO_NONE || type == GB_UNDO_KILL) {
            return gbcmc_send_undo_commands(gb_main,_GBCMC_UNDOCOM_REQUEST_NOUNDO);
        }else{
            return gbcmc_send_undo_commands(gb_main,_GBCMC_UNDOCOM_REQUEST_UNDO);
        }
    }
    return 0;
}

GB_UNDO_TYPE GB_get_requested_undo_type(GBDATA *gb_main){
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    return Main->requested_undo_type;
}


/** undo/redo the last transaction */
GB_ERROR GB_undo(GBDATA *gb_main,GB_UNDO_TYPE type) {
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    GB_UNDO_TYPE old_type = GB_get_requested_undo_type(gb_main);
    GB_ERROR error = 0;
    if (!Main->local_mode) {
        switch (type) {
            case GB_UNDO_UNDO:
                return gbcmc_send_undo_commands(gb_main,_GBCMC_UNDOCOM_UNDO);
            case GB_UNDO_REDO:
                return gbcmc_send_undo_commands(gb_main,_GBCMC_UNDOCOM_REDO);
            default:    GB_internal_error("unknown undo type in GB_undo");
                return GB_export_error("Internal UNDO error");
        }
    }
    switch (type){
        case GB_UNDO_UNDO:
            GB_request_undo_type(gb_main,GB_UNDO_REDO);
            error =  g_b_undo(Main,gb_main,Main->undo->u);
            GB_request_undo_type(gb_main,old_type);
            break;
        case GB_UNDO_REDO:
            GB_request_undo_type(gb_main,GB_UNDO_UNDO_REDO);
            error =  g_b_undo(Main,gb_main,Main->undo->r);
            GB_request_undo_type(gb_main,old_type);
            break;
        default:
            error = GB_export_error("GB_undo: unknown undo type specified");
    }
    return error;
}


/** get some information about the next undo */
char *GB_undo_info(GBDATA *gb_main,GB_UNDO_TYPE type) {
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    if (!Main->local_mode) {
        switch (type) {
            case GB_UNDO_UNDO:
                return gbcmc_send_undo_info_commands(gb_main,_GBCMC_UNDOCOM_INFO_UNDO);
            case GB_UNDO_REDO:
                return gbcmc_send_undo_info_commands(gb_main,_GBCMC_UNDOCOM_INFO_REDO);
            default:
                GB_internal_error("unknown undo type in GB_undo");
                GB_export_error("Internal UNDO error");
                return 0;
        }
    }
    switch (type){
        case GB_UNDO_UNDO:
            return g_b_undo_info(Main,gb_main,Main->undo->u);
        case GB_UNDO_REDO:
            return g_b_undo_info(Main,gb_main,Main->undo->r);
        default:
            GB_export_error("GB_undo_info: unknown undo type specified");
            return 0;
    }
}

/** set the maxmimum memory used for undoing */
GB_ERROR GB_set_undo_mem(GBDATA *gbd, long memsize){
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    if (memsize < _GBCMC_UNDOCOM_SET_MEM){
        return  GB_export_error("Too less UNDO memory specified: should be more than %i",
                                _GBCMC_UNDOCOM_SET_MEM);
    }
    Main->undo->max_size_of_all_undos = memsize;
    if (!Main->local_mode) {
        return gbcmc_send_undo_commands(gbd,(enum gb_undo_commands)memsize);
    }
    g_b_check_undo_size(Main);
    return 0;
}

