// =============================================================== //
//                                                                 //
//   File      : adindex.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <cctype>

#include "gb_undo.h"
#include "gb_index.h"
#include "gb_hashindex.h"
#include "gb_ts.h"
#include "gb_storage.h"

#define GB_INDEX_FIND(gbf, ifs, quark)                                  \
    for (ifs = GBCONTAINER_IFS(gbf);                                    \
         ifs;                                                           \
         ifs = GB_INDEX_FILES_NEXT(ifs))                                \
    {                                                                   \
        if (ifs->key == quark) break;                                   \
    }

// write field in index table
char *gb_index_check_in(GBDATA *gbd)
{
    gb_index_files *ifs;
    GBQUARK         quark;
    unsigned long   index;
    GB_CSTR         data;
    GBCONTAINER    *gfather;

    gfather = GB_GRANDPA(gbd);
    if (!gfather)   return 0;

    quark = GB_KEY_QUARK(gbd);
    GB_INDEX_FIND(gfather, ifs, quark);
    if (!ifs) return 0;     // This key is not indexed

    if (GB_TYPE(gbd) != GB_STRING && GB_TYPE(gbd) != GB_LINK) return 0;

    if (gbd->flags2.is_indexed)
    {
        GB_internal_error("Double checked in");
        return 0;
    }

    data = GB_read_char_pntr(gbd);
    GB_CALC_HASH_INDEX(data, index, ifs->hash_table_size, ifs->case_sens);
    ifs->nr_of_elements++;
    {
        gb_if_entries *ifes;
        GB_REL_IFES   *entries = GB_INDEX_FILES_ENTRIES(ifs);

        ifes = (gb_if_entries *)gbm_get_mem(sizeof(gb_if_entries), GB_GBM_INDEX(gbd));

        SET_GB_IF_ENTRIES_NEXT(ifes, GB_ENTRIES_ENTRY(entries, index));
        SET_GB_IF_ENTRIES_GBD(ifes, gbd);
        SET_GB_ENTRIES_ENTRY(entries, index, ifes);
    }
    gbd->flags2.tisa_index = 1;
    gbd->flags2.is_indexed = 1;
    return 0;
}

// remove entry from index table
void gb_index_check_out(GBDATA *gbd) {
    if (gbd->flags2.is_indexed) {
        GB_ERROR        error   = 0;
        GBCONTAINER    *gfather = GB_GRANDPA(gbd);
        GBQUARK         quark   = GB_KEY_QUARK(gbd);
        gb_index_files *ifs;

        gbd->flags2.is_indexed = 0;
        GB_INDEX_FIND(gfather, ifs, quark);

        if (!ifs) error = "key is not indexed";
        else {
            error = GB_push_transaction(gbd);
            if (!error) {
                GB_CSTR data = GB_read_char_pntr(gbd);

                if (!data) {
                    error = GBS_global_string("can't read key value (%s)", GB_await_error());
                }
                else {
                    unsigned long index;
                    GB_CALC_HASH_INDEX(data, index, ifs->hash_table_size, ifs->case_sens);

                    gb_if_entries *ifes2   = 0;
                    GB_REL_IFES   *entries = GB_INDEX_FILES_ENTRIES(ifs);
                    gb_if_entries *ifes;

                    for (ifes = GB_ENTRIES_ENTRY(entries, index); ifes; ifes = GB_IF_ENTRIES_NEXT(ifes)) {
                        if (gbd == GB_IF_ENTRIES_GBD(ifes)) {       // entry found
                            if (ifes2) SET_GB_IF_ENTRIES_NEXT(ifes2, GB_IF_ENTRIES_NEXT(ifes));
                            else SET_GB_ENTRIES_ENTRY(entries, index, GB_IF_ENTRIES_NEXT(ifes));

                            ifs->nr_of_elements--;
                            gbm_free_mem(ifes, sizeof(gb_if_entries), GB_GBM_INDEX(gbd));
                            break;
                        }
                        ifes2 = ifes;
                    }
                }
            }
            error = GB_end_transaction(gbd, error);
        }

        if (error) {
            error = GBS_global_string("gb_index_check_out failed for key '%s' (%s)\n", GB_KEY(gbd), error);
            GB_internal_error(error);
        }
    }
}

GB_ERROR GB_create_index(GBDATA *gbd, const char *key, GB_CASE case_sens, long estimated_size) {
    /* Create an index for a database.
     * Uses hash tables - collisions are avoided by using linked lists.
     */
    GB_ERROR error = 0;

    if (GB_TYPE(gbd) != GB_DB) {
        error = GB_export_error("GB_create_index used on non CONTAINER Type");
    }
    else if (GB_read_clients(gbd)<0) {
        error = GB_export_error("No index tables in DB clients allowed");
    }
    else {
        GBCONTAINER    *gbc       = (GBCONTAINER *)gbd;
        GBQUARK         key_quark = GB_key_2_quark(gbd, key);
        gb_index_files *ifs;

        GB_INDEX_FIND(gbc, ifs, key_quark);

        if (!ifs) { // if not already have index (e.g. if fast-loaded)
            GBDATA *gbf;

            ifs = (gb_index_files *)gbm_get_mem(sizeof(gb_index_files), GB_GBM_INDEX(gbc));
            SET_GB_INDEX_FILES_NEXT(ifs, GBCONTAINER_IFS(gbc));
            SET_GBCONTAINER_IFS(gbc, ifs);

            ifs->key             = key_quark;
            ifs->hash_table_size = gbs_get_a_prime(estimated_size);
            ifs->nr_of_elements  = 0;
            ifs->case_sens       = case_sens;

            SET_GB_INDEX_FILES_ENTRIES(ifs, (gb_if_entries **)gbm_get_mem(sizeof(void *)*(int)ifs->hash_table_size, GB_GBM_INDEX(gbc)));

            for (gbf = GB_find_sub_by_quark(gbd, -1, 0, 0);
                 gbf;
                 gbf = GB_find_sub_by_quark(gbd, -1, gbf, 0))
            {
                if (GB_TYPE(gbf) == GB_DB) {
                    GBDATA *gb2;

                    for (gb2 = GB_find_sub_by_quark(gbf, key_quark, 0, 0);
                         gb2;
                         gb2 = GB_find_sub_by_quark(gbf, key_quark, gb2, 0))
                    {
                        if (GB_TYPE(gb2) != GB_STRING && GB_TYPE(gb2) != GB_LINK) continue;
                        gb_index_check_in(gb2);
                    }
                }
            }
        }
    }
    return error;
}

void gb_destroy_indices(GBCONTAINER *gbc) {
    gb_index_files *ifs = GBCONTAINER_IFS(gbc);

    while (ifs) {
        GB_REL_IFES *if_entries = GB_INDEX_FILES_ENTRIES(ifs);

        for (int index = 0; index<ifs->hash_table_size; index++) {
            gb_if_entries *ifes = GB_ENTRIES_ENTRY(if_entries, index);

            while (ifes) {
                gb_if_entries *ifes_next = GB_IF_ENTRIES_NEXT(ifes);

                gbm_free_mem(ifes, sizeof(*ifes), GB_GBM_INDEX(gbc));
                ifes = ifes_next;
            }
        }
        gbm_free_mem(if_entries, sizeof(void *)*(int)ifs->hash_table_size, GB_GBM_INDEX(gbc));

        gb_index_files *ifs_next = GB_INDEX_FILES_NEXT(ifs);
        gbm_free_mem(ifs, sizeof(gb_index_files), GB_GBM_INDEX(gbc));
        ifs = ifs_next;
    }
}

#if defined(DEBUG)

NOT4PERL void GB_dump_indices(GBDATA *gbd) {
    // dump indices of container

    char *db_path = strdup(GB_get_db_path(gbd));

    if (GB_TYPE(gbd) != GB_DB) {
        fprintf(stderr, "'%s' (%s) is no container.\n", db_path, GB_get_type_name(gbd));
    }
    else {
        gb_index_files *ifs;
        int             index_count = 0;
        GBCONTAINER    *gbc         = (GBCONTAINER*)gbd;
        GB_MAIN_TYPE   *Main        = GBCONTAINER_MAIN(gbc);

        for (ifs = GBCONTAINER_IFS(gbc); ifs; ifs = GB_INDEX_FILES_NEXT(ifs)) {
            index_count++;
        }

        if (index_count == 0) {
            fprintf(stderr, "Container '%s' has no index.\n", db_path);
        }
        else {
            int pass;

            fprintf(stderr, "Indices for '%s':\n", db_path);
            for (pass = 1; pass <= 2; pass++) {
                if (pass == 2) {
                    fprintf(stderr, "\nDetailed index contents:\n\n");
                }
                index_count = 0;
                for (ifs = GBCONTAINER_IFS(gbc); ifs; ifs = GB_INDEX_FILES_NEXT(ifs)) {
                    fprintf(stderr,
                            "* Index %i for key=%s (%i), entries=%li, %s\n",
                            index_count,
                            Main->keys[ifs->key].key,
                            ifs->key,
                            ifs->nr_of_elements,
                            ifs->case_sens == GB_MIND_CASE
                            ? "Case sensitive"
                            : (ifs->case_sens == GB_IGNORE_CASE
                               ? "Case insensitive"
                               : "<Error in case_sens>")
                            );

                    if (pass == 2) {
                        gb_if_entries *ifes;
                        int            index;

                        fprintf(stderr, "\n");
                        for (index = 0; index<ifs->hash_table_size; index++) {
                            for (ifes = GB_ENTRIES_ENTRY(GB_INDEX_FILES_ENTRIES(ifs), index);
                                 ifes;
                                 ifes = GB_IF_ENTRIES_NEXT(ifes))
                            {
                                GBDATA     *igbd = GB_IF_ENTRIES_GBD(ifes);
                                const char *data = GB_read_char_pntr(igbd);

                                fprintf(stderr, "  - '%s' (@idx=%i)\n", data, index);
                            }
                        }
                        fprintf(stderr, "\n");
                    }
                    index_count++;
                }
            }
        }
    }

    free(db_path);
}

#endif // DEBUG


// find an entry in an hash table
GBDATA *gb_index_find(GBCONTAINER *gbf, gb_index_files *ifs, GBQUARK quark, const char *val, GB_CASE case_sens, int after_index) {
    unsigned long  index;
    GB_CSTR        data;
    gb_if_entries *ifes;
    GBDATA        *result = 0;
    long           min_index;

    if (!ifs) {
        GB_INDEX_FIND(gbf, ifs, quark);
        if (!ifs) {
            GB_internal_error("gb_index_find called, but no index table found");
            return 0;
        }
    }

    if (ifs->case_sens != case_sens) {
        GB_internal_error("case mismatch between index and search");
        return 0;
    }

    GB_CALC_HASH_INDEX(val, index, ifs->hash_table_size, ifs->case_sens);
    min_index = gbf->d.nheader;

    for (ifes = GB_ENTRIES_ENTRY(GB_INDEX_FILES_ENTRIES(ifs), index);
            ifes;
            ifes = GB_IF_ENTRIES_NEXT(ifes))
    {
        GBDATA *igbd = GB_IF_ENTRIES_GBD(ifes);
        GBCONTAINER *ifather = GB_FATHER(igbd);

        if (ifather->index < after_index) continue;
        if (ifather->index >= min_index) continue;
        data = GB_read_char_pntr(igbd);
        if (GBS_string_matches(data, val, case_sens)) { // entry found
            result    = igbd;
            min_index = ifather->index;
        }
    }
    return result;
}


/* UNDO functions
 *
 * There are three undo stacks:
 *
 * GB_UNDO_NONE    no undo
 * GB_UNDO_UNDO    normal undo stack
 * GB_UNDO_REDO    redo stack
 */

static char *gb_set_undo_type(GBDATA *gb_main, GB_UNDO_TYPE type) {
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    Main->undo_type = type;
    return 0;
}

static void g_b_add_size_to_undo_entry(g_b_undo_entry *ue, long size) {
    ue->sizeof_this                 += size;        // undo entry
    ue->father->sizeof_this         += size;        // one undo
    ue->father->father->sizeof_this += size;        // all undos
}

static g_b_undo_entry *new_g_b_undo_entry(g_b_undo_list *u) {
    g_b_undo_entry *ue = (g_b_undo_entry *)gbm_get_mem(sizeof(g_b_undo_entry), GBM_UNDO);

    ue->next   = u->entries;
    ue->father = u;
    u->entries = ue;

    g_b_add_size_to_undo_entry(ue, sizeof(g_b_undo_entry));

    return ue;
}



void gb_init_undo_stack(GB_MAIN_TYPE *Main) {
    Main->undo = (g_b_undo_mgr *)GB_calloc(sizeof(g_b_undo_mgr), 1);
    Main->undo->max_size_of_all_undos = GB_MAX_UNDO_SIZE;
    Main->undo->u = (g_b_undo_header *) GB_calloc(sizeof(g_b_undo_header), 1);
    Main->undo->r = (g_b_undo_header *) GB_calloc(sizeof(g_b_undo_header), 1);
}

static void delete_g_b_undo_entry(g_b_undo_entry *entry) {
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
    gbm_free_mem(entry, sizeof(g_b_undo_entry), GBM_UNDO);
}

static void delete_g_b_undo_list(g_b_undo_list *u) {
    g_b_undo_entry *a, *next;
    for (a = u->entries; a; a = next) {
        next = a->next;
        delete_g_b_undo_entry(a);
    }
    free(u);
}

static void delete_g_b_undo_header(g_b_undo_header *uh) {
    g_b_undo_list *a, *next=0;
    for (a = uh->stack; a; a = next) {
        next = a->next;
        delete_g_b_undo_list(a);
    }
    free(uh);
}

static char *g_b_check_undo_size2(g_b_undo_header *uhs, long size, long max_cnt) {
    long           csize = 0;
    long           ccnt  = 0;
    g_b_undo_list *us;

    for (us = uhs->stack; us && us->next;  us = us->next) {
        csize += us->sizeof_this;
        ccnt ++;
        if (((csize + us->next->sizeof_this) > size) ||
             (ccnt >= max_cnt)) {  // delete the rest
            g_b_undo_list *a, *next=0;

            for (a = us->next; a; a = next) {
                next = a->next;
                delete_g_b_undo_list(a);
            }
            us->next = 0;
            uhs->sizeof_this = csize;
            break;
        }
    }
    return 0;
}

static char *g_b_check_undo_size(GB_MAIN_TYPE *Main) {
    char *error = 0;
    long maxsize = Main->undo->max_size_of_all_undos;
    error = g_b_check_undo_size2(Main->undo->u, maxsize/2, GB_MAX_UNDO_CNT);
    if (error) return error;
    error = g_b_check_undo_size2(Main->undo->r, maxsize/2, GB_MAX_REDO_CNT);
    if (error) return error;
    return 0;
}


void gb_free_undo_stack(GB_MAIN_TYPE *Main) {
    delete_g_b_undo_header(Main->undo->u);
    delete_g_b_undo_header(Main->undo->r);
    free(Main->undo);
}

// -------------------------
//      real undo (redo)

static GB_ERROR undo_entry(g_b_undo_entry *ue) {
    GB_ERROR error = 0;
    switch (ue->type) {
        case GB_UNDO_ENTRY_TYPE_CREATED:
            error = GB_delete(ue->source);
            break;
        case GB_UNDO_ENTRY_TYPE_DELETED:
            {
                GBDATA *gbd = ue->d.gs.gbd;
                int type = GB_TYPE(gbd);
                if (type == GB_DB) {
                    gbd = (GBDATA *)gb_make_pre_defined_container((GBCONTAINER *)ue->source, (GBCONTAINER *)gbd, -1, ue->d.gs.key);
                }
                else {
                    gbd = gb_make_pre_defined_entry((GBCONTAINER *)ue->source, gbd, -1, ue->d.gs.key);
                }
                GB_ARRAY_FLAGS(gbd).flags = ue->flag;
                gb_touch_header(GB_FATHER(gbd));
                gb_touch_entry((GBDATA *)gbd, GB_CREATED);
            }
            break;
        case GB_UNDO_ENTRY_TYPE_MODIFY_ARRAY:
        case GB_UNDO_ENTRY_TYPE_MODIFY:
            {
                GBDATA *gbd = ue->source;
                int type  = GB_TYPE(gbd);
                if (type == GB_DB) {

                }
                else {
                    gb_save_extern_data_in_ts(gbd); // check out and free string

                    if (ue->d.ts) { // nothing to undo (e.g. if undoing GB_touch)
                        gbd->flags              = ue->d.ts->flags;
                        gbd->flags2.extern_data = ue->d.ts->flags2.extern_data;

                        memcpy(&gbd->info, &ue->d.ts->info, sizeof(gbd->info)); // restore old information
                        if (type >= GB_BITS) {
                            if (gbd->flags2.extern_data) {
                                SET_GB_EXTERN_DATA_DATA(gbd->info.ex, ue->d.ts->info.ex.data); // set relative pointers correctly
                            }

                            gb_del_ref_and_extern_gb_transaction_save(ue->d.ts);
                            ue->d.ts = 0;

                            GB_INDEX_CHECK_IN(gbd);
                        }
                    }
                }
                {
                    gb_header_flags *pflags = &GB_ARRAY_FLAGS(gbd);
                    if (pflags->flags != (unsigned)ue->flag) {
                        GBCONTAINER *gb_father = GB_FATHER(gbd);
                        gbd->flags.saved_flags = pflags->flags;
                        pflags->flags = ue->flag;
                        if (GB_FATHER(gb_father)) {
                            gb_touch_header(gb_father); // don't touch father of main
                        }
                    }
                }
                gb_touch_entry(gbd, GB_NORMAL_CHANGE);
            }
            break;
        default:
            GB_internal_error("Undo stack corrupt:!!!");
            error = GB_export_error("shit 34345");
    }

    return error;
}



static GB_ERROR g_b_undo(GBDATA *gb_main, g_b_undo_header *uh) { // goes to header: __ATTR__USERESULT
    GB_ERROR error = NULL;

    if (!uh->stack) {
        error = "Sorry no more undos/redos available";
    }
    else {
        g_b_undo_list  *u = uh->stack;
        g_b_undo_entry *ue, *next;

        error = GB_begin_transaction(gb_main);

        for (ue=u->entries; ue && !error; ue = next) {
            next = ue->next;
            error = undo_entry(ue);
            delete_g_b_undo_entry(ue);
            u->entries = next;
        }
        uh->sizeof_this -= u->sizeof_this;          // remove undo from list
        uh->stack        = u->next;

        delete_g_b_undo_list(u);
        error = GB_end_transaction(gb_main, error);
    }
    return error;
}

static GB_CSTR g_b_read_undo_key_pntr(GB_MAIN_TYPE *Main, g_b_undo_entry *ue) {
    return Main->keys[ue->d.gs.key].key;
}

static char *g_b_undo_info(GB_MAIN_TYPE *Main, g_b_undo_header *uh) {
    GBS_strstruct  *res = GBS_stropen(1024);
    g_b_undo_list  *u;
    g_b_undo_entry *ue;

    u = uh->stack;
    if (!u) return strdup("No more undos available");
    for (ue=u->entries; ue; ue = ue->next) {
        switch (ue->type) {
            case GB_UNDO_ENTRY_TYPE_CREATED:
                GBS_strcat(res, "Delete new entry: ");
                GBS_strcat(res, gb_read_key_pntr(ue->source));
                break;
            case GB_UNDO_ENTRY_TYPE_DELETED:
                GBS_strcat(res, "Rebuild deleted entry: ");
                GBS_strcat(res, g_b_read_undo_key_pntr(Main, ue));
                break;
            case GB_UNDO_ENTRY_TYPE_MODIFY_ARRAY:
            case GB_UNDO_ENTRY_TYPE_MODIFY:
                GBS_strcat(res, "Undo modified entry: ");
                GBS_strcat(res, gb_read_key_pntr(ue->source));
                break;
            default:
                break;
        }
        GBS_chrcat(res, '\n');
    }
    return GBS_strclose(res);
}

static char *gb_free_all_undos(GBDATA *gb_main) {
    // Remove all existing undos/redos
    GB_MAIN_TYPE  *Main = GB_MAIN(gb_main);
    g_b_undo_list *a, *next;
    
    for (a = Main->undo->r->stack; a; a = next) {
        next = a->next;
        delete_g_b_undo_list(a);
    }
    Main->undo->r->stack = 0;
    Main->undo->r->sizeof_this = 0;

    for (a = Main->undo->u->stack; a; a = next) {
        next = a->next;
        delete_g_b_undo_list(a);
    }
    Main->undo->u->stack = 0;
    Main->undo->u->sizeof_this = 0;
    return 0;
}


char *gb_set_undo_sync(GBDATA *gb_main) {
    // start a new undoable transaction
    GB_MAIN_TYPE    *Main  = GB_MAIN(gb_main);
    char            *error = g_b_check_undo_size(Main);
    g_b_undo_header *uhs;

    if (error) return error;
    switch (Main->requested_undo_type) {    // init the target undo stack
        case GB_UNDO_UNDO:      // that will undo but delete all redos
            uhs         = Main->undo->u;
            break;
        case GB_UNDO_UNDO_REDO: uhs = Main->undo->u; break;
        case GB_UNDO_REDO:      uhs = Main->undo->r; break;
        case GB_UNDO_KILL:      gb_free_all_undos(gb_main);
        default:            uhs = 0;
    }
    if (uhs)
    {
        g_b_undo_list *u = (g_b_undo_list *) GB_calloc(sizeof(g_b_undo_list),  1);
        u->next = uhs->stack;
        u->father = uhs;
        uhs->stack = u;
        Main->undo->valid_u = u;
    }

    return gb_set_undo_type(gb_main, Main->requested_undo_type);
}

char *gb_disable_undo(GBDATA *gb_main) {
    // called to finish an undoable section, called at end of gb_commit_transaction
    GB_MAIN_TYPE  *Main = GB_MAIN(gb_main);
    g_b_undo_list *u    = Main->undo->valid_u;

    if (!u) return 0;
    if (!u->entries) {      // nothing to undo, just a read transaction
        u->father->stack = u->next;
        delete_g_b_undo_list(u);
    }
    else {
        if (Main->requested_undo_type == GB_UNDO_UNDO) {    // remove all redos
            g_b_undo_list *a, *next;

            for (a = Main->undo->r->stack; a; a = next) {
                next = a->next;
                delete_g_b_undo_list(a);
            }
            Main->undo->r->stack = 0;
            Main->undo->r->sizeof_this = 0;
        }
    }
    Main->undo->valid_u = 0;
    return gb_set_undo_type(gb_main, GB_UNDO_NONE);
}

void gb_check_in_undo_create(GB_MAIN_TYPE *Main, GBDATA *gbd) {
    g_b_undo_entry *ue;
    if (!Main->undo->valid_u) return;
    ue = new_g_b_undo_entry(Main->undo->valid_u);
    ue->type = GB_UNDO_ENTRY_TYPE_CREATED;
    ue->source = gbd;
    ue->gbm_index = GB_GBM_INDEX(gbd);
    ue->flag = 0;
}

void gb_check_in_undo_modify(GB_MAIN_TYPE *Main, GBDATA *gbd) {
    long                 type = GB_TYPE(gbd);
    g_b_undo_entry      *ue;
    gb_transaction_save *old;

    if (!Main->undo->valid_u) {
        GB_FREE_TRANSACTION_SAVE(gbd);
        return;
    }

    old = GB_GET_EXT_OLD_DATA(gbd);
    ue = new_g_b_undo_entry(Main->undo->valid_u);
    ue->source = gbd;
    ue->gbm_index = GB_GBM_INDEX(gbd);
    ue->type = GB_UNDO_ENTRY_TYPE_MODIFY;
    ue->flag =      gbd->flags.saved_flags;

    if (type != GB_DB) {
        ue->d.ts = old;
        if (old) {
            gb_add_ref_gb_transaction_save(old);
            if (type >= GB_BITS && old->flags2.extern_data && old->info.ex.data) {
                ue->type = GB_UNDO_ENTRY_TYPE_MODIFY_ARRAY;
                // move external array from ts to undo entry struct
                g_b_add_size_to_undo_entry(ue, old->info.ex.memsize);
            }
        }
    }
}

#if defined(DEVEL_RALF)
#warning change param for gb_check_in_undo_delete to GBDATA **
#endif // DEVEL_RALF

void gb_check_in_undo_delete(GB_MAIN_TYPE *Main, GBDATA *gbd, int deep) {
    long            type = GB_TYPE(gbd);
    g_b_undo_entry *ue;

    if (!Main->undo->valid_u) {
        gb_delete_entry(&gbd);
        return;
    }

    if (type == GB_DB) {
        int             index;
        GBDATA         *gbd2;
        GBCONTAINER    *gbc = ((GBCONTAINER *) gbd);

        for (index = 0; (index < gbc->d.nheader); index++) {
            if ((gbd2 = GBCONTAINER_ELEM(gbc, index))) {
                gb_check_in_undo_delete(Main, gbd2, deep+1);
            }
        };
    }
    else {
        GB_INDEX_CHECK_OUT(gbd);
        gbd->flags2.tisa_index = 0; // never check in again
    }
    gb_abort_entry(gbd);            // get old version

    ue = new_g_b_undo_entry(Main->undo->valid_u);

    ue->type      = GB_UNDO_ENTRY_TYPE_DELETED;
    ue->source    = (GBDATA *)GB_FATHER(gbd);
    ue->gbm_index = GB_GBM_INDEX(gbd);
    ue->flag      = GB_ARRAY_FLAGS(gbd).flags;

    ue->d.gs.gbd = gbd;
    ue->d.gs.key = GB_KEY_QUARK(gbd);

    gb_pre_delete_entry(gbd);       // get the core of the entry

    if (type == GB_DB) {
        g_b_add_size_to_undo_entry(ue, sizeof(GBCONTAINER));
    }
    else {
        if (type >= GB_BITS && gbd->flags2.extern_data) {
            /* we have copied the data structures, now
               mark the old as deleted !!! */
            g_b_add_size_to_undo_entry(ue, GB_GETMEMSIZE(gbd));
        }
        g_b_add_size_to_undo_entry(ue, sizeof(GBDATA));
    }
}

// ----------------------------------------
//      UNDO functions exported to USER

GB_ERROR GB_request_undo_type(GBDATA *gb_main, GB_UNDO_TYPE type) { // goes to header: __ATTR__USERESULT
    /*! Define how to undo DB changes.
     *
     * This function should be called just before opening a transaction,
     * otherwise its effect will be delayed.
     *
     * Possible types are:
     *      GB_UNDO_UNDO        enable undo
     *      GB_UNDO_NONE        disable undo
     *      GB_UNDO_KILL        disable undo and remove old undos !!
     *
     * Note: if GB_request_undo_type returns an error, local undo type remains unchanged
     */

    GB_MAIN_TYPE *Main  = GB_MAIN(gb_main);
    GB_ERROR      error = NULL;

    if (!Main->local_mode) {
        enum gb_undo_commands cmd = (type == GB_UNDO_NONE || type == GB_UNDO_KILL)
            ? _GBCMC_UNDOCOM_REQUEST_NOUNDO
            : _GBCMC_UNDOCOM_REQUEST_UNDO;
        error = gbcmc_send_undo_commands(gb_main, cmd);
    }
    if (!error) Main->requested_undo_type = type;

    return error;
}

GB_UNDO_TYPE GB_get_requested_undo_type(GBDATA *gb_main) {
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    return Main->requested_undo_type;
}


GB_ERROR GB_undo(GBDATA *gb_main, GB_UNDO_TYPE type) { // goes to header: __ATTR__USERESULT
    // undo/redo the last transaction

    GB_MAIN_TYPE *Main  = GB_MAIN(gb_main);
    GB_ERROR      error = 0;

    if (!Main->local_mode) {
        switch (type) {
            case GB_UNDO_UNDO:
                error = gbcmc_send_undo_commands(gb_main, _GBCMC_UNDOCOM_UNDO);
                break;

            case GB_UNDO_REDO:
                error = gbcmc_send_undo_commands(gb_main, _GBCMC_UNDOCOM_REDO);
                break;

            default:
                GB_internal_error("unknown undo type in GB_undo");
                error = "Internal UNDO error";
                break;
        }
    }
    else {
        GB_UNDO_TYPE old_type = GB_get_requested_undo_type(gb_main);
        switch (type) {
            case GB_UNDO_UNDO:
                error = GB_request_undo_type(gb_main, GB_UNDO_REDO);
                if (!error) {
                    error = g_b_undo(gb_main, Main->undo->u);
                    ASSERT_NO_ERROR(GB_request_undo_type(gb_main, old_type));
                }
                break;

            case GB_UNDO_REDO:
                error = GB_request_undo_type(gb_main, GB_UNDO_UNDO_REDO);
                if (!error) {
                    error = g_b_undo(gb_main, Main->undo->r);
                    ASSERT_NO_ERROR(GB_request_undo_type(gb_main, old_type));
                }
                break;

            default:
                error = "GB_undo: unknown undo type specified";
                break;
        }
    }

    return error;
}


char *GB_undo_info(GBDATA *gb_main, GB_UNDO_TYPE type) {
    // get some information about the next undo

    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    if (!Main->local_mode) {
        switch (type) {
            case GB_UNDO_UNDO:
                return gbcmc_send_undo_info_commands(gb_main, _GBCMC_UNDOCOM_INFO_UNDO);
            case GB_UNDO_REDO:
                return gbcmc_send_undo_info_commands(gb_main, _GBCMC_UNDOCOM_INFO_REDO);
            default:
                GB_internal_error("unknown undo type in GB_undo");
                GB_export_error("Internal UNDO error");
                return 0;
        }
    }
    switch (type) {
        case GB_UNDO_UNDO:
            return g_b_undo_info(Main, Main->undo->u);
        case GB_UNDO_REDO:
            return g_b_undo_info(Main, Main->undo->r);
        default:
            GB_export_error("GB_undo_info: unknown undo type specified");
            return 0;
    }
}

GB_ERROR GB_set_undo_mem(GBDATA *gbd, long memsize) {
    // set the maximum memory used for undoing

    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    if (memsize < _GBCMC_UNDOCOM_SET_MEM) {
        return GB_export_errorf("Not enough UNDO memory specified: should be more than %i",
                                _GBCMC_UNDOCOM_SET_MEM);
    }
    Main->undo->max_size_of_all_undos = memsize;
    if (!Main->local_mode) {
        return gbcmc_send_undo_commands(gbd, (enum gb_undo_commands)memsize);
    }
    g_b_check_undo_size(Main);
    return 0;
}

