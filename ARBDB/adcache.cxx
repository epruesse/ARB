// =============================================================== //
//                                                                 //
//   File      : adcache.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "gb_storage.h"

void gb_init_cache(GB_MAIN_TYPE *Main){
    int i;
    if (Main->cache.entries) return;
    Main->cache.entries = (struct gb_cache_entry_struct *)GB_calloc(sizeof(struct gb_cache_entry_struct),
                                                                    GB_MAX_CACHED_ENTRIES);
    Main->cache.max_data_size = GB_TOTAL_CACHE_SIZE;
    Main->cache.max_entries = GB_MAX_CACHED_ENTRIES;
    for (i=0;i<GB_MAX_CACHED_ENTRIES-1;i++) {
        Main->cache.entries[i].next = i+1;
    }
    Main->cache.firstfree_entry = 1;
}

char *gb_read_cache(GBDATA *gbd) {
    GB_MAIN_TYPE *Main;
    struct gb_cache_struct *cs;
    long i;
    long n,p;
    if (!(i=gbd->cache_index)) return 0;
    Main = GB_MAIN(gbd);
    cs = &Main->cache;
    n = cs->entries[i].next; p = cs->entries[i].prev;
    // remove entry from list
    if (i == cs->newest_entry) cs->newest_entry = n;
    if (i == cs->oldest_entry) cs->oldest_entry = p;
    cs->entries[n].prev = p;
    cs->entries[p].next = n;
    // check validity
    if (GB_GET_EXT_UPDATE_DATE(gbd) > cs->entries[i].clock) {
        freeset(cs->entries[i].data, NULL);
        cs->sum_data_size -= cs->entries[i].sizeof_data;

        gbd->cache_index = 0;

        // insert deleted entry in free list
        cs->entries[i].next = cs->firstfree_entry;
        cs->firstfree_entry = i;
        return 0;
    }

    // insert entry on top of list
    cs->entries[i].next = cs->newest_entry;
    cs->entries[cs->newest_entry].prev = i;
    cs->newest_entry = i;
    cs->entries[i].prev = 0;
    if (!cs->oldest_entry) cs->oldest_entry = i;

    return cs->entries[i].data;
}

void *gb_free_cache(GB_MAIN_TYPE *Main, GBDATA *gbd) {
    struct gb_cache_struct *cs;
    long i;
    long n,p;
    if (!(i=gbd->cache_index)) return 0;
    cs = &Main->cache;
    n = cs->entries[i].next; p = cs->entries[i].prev;
    // remove entry from list
    if (i == cs->newest_entry) cs->newest_entry = n;
    if (i == cs->oldest_entry) cs->oldest_entry = p;
    cs->entries[n].prev = p;
    cs->entries[p].next = n;

    // free cache
    freeset(cs->entries[i].data, NULL);
    cs->sum_data_size -= cs->entries[i].sizeof_data;

    gbd->cache_index = 0;

    // insert deleted entry in free list
    cs->entries[i].next = cs->firstfree_entry;
    cs->firstfree_entry = i;
    return 0;
}

char *delete_old_cache_entries(struct gb_cache_struct *cs, long needed_size, long max_data_size)
{
    // call with max_data_size==0 to flush cache
    long n,p;
    long i;
    char *data = 0;

    while ( ( (!cs->firstfree_entry) || ( needed_size + cs->sum_data_size >= max_data_size))
            && cs->oldest_entry) {
        i = cs->oldest_entry;
        n = cs->entries[i].next; p = cs->entries[i].prev;
        // remove entry from list
        if (i == cs->newest_entry) cs->newest_entry = n;
        if (i == cs->oldest_entry) cs->oldest_entry = p;
        cs->entries[n].prev = p;
        cs->entries[p].next = n;

        // insert deleted entry in free list
        cs->entries[i].gbd->cache_index = 0;
        cs->entries[i].next = cs->firstfree_entry;
        cs->firstfree_entry = i;
        // delete all unused memory
        if (data || ( needed_size != cs->entries[i].sizeof_data)  ) {
            free(cs->entries[i].data);
        }else{
            data = cs->entries[i].data;
        }
        cs->sum_data_size -= cs->entries[i].sizeof_data;
        cs->entries[i].data = 0;
    }
    return data;
}

char *gb_flush_cache(GBDATA *gbd)
{
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    struct gb_cache_struct *cs = &Main->cache;

    delete_old_cache_entries(cs, 0, 0);
    return 0;
}

char *gb_alloc_cache_index(GBDATA *gbd,long size) {
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    struct gb_cache_struct *cs = &Main->cache;
    long i;
    char *data = 0;

    data = delete_old_cache_entries(cs, size, cs->max_data_size); // delete enough old memory

    i = cs->firstfree_entry;
    if (!i) {
        GB_internal_error("internal cache error");
        return 0;
    }

    // get free element
    cs->firstfree_entry = cs->entries[i].next;
    // insert it on top of used list
    cs->entries[i].next = cs->newest_entry;
    cs->entries[cs->newest_entry].prev = i;
    cs->newest_entry = i;
    cs->entries[i].prev = 0;
    if (!cs->oldest_entry) cs->oldest_entry = i;

    // create data
    cs->sum_data_size += size;
    if (!data) data = (char *) malloc((int)size);
    cs->entries[i].sizeof_data = (int)size;
    cs->entries[i].data = data;
    cs->entries[i].gbd = gbd;
    gbd->cache_index = (short)i;

    return data;
}

char *GB_set_cache_size(GBDATA *gbd, long size){
    GB_MAIN(gbd)->cache.max_data_size = size;
    return 0;
}
