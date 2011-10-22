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

struct gb_cache_entry {
    GBDATA       *gbd;
    gb_cache_idx  prev;
    gb_cache_idx  next;
    char         *data;
    long          clock;
    size_t        sizeof_data;

#if defined(GEN_CACHE_STATS)
    long  reused;
    char *dbpath;
#endif // GEN_CACHE_STATS
};

inline bool entry_is_linked(gb_cache& cache, gb_cache_idx index) {
    gb_cache_entry& entry = cache.entries[index];
    return
        (entry.prev || cache.newest_entry == index) &&
        (entry.next || cache.oldest_entry == index);
}

inline gb_cache_entry& unlink_cache_entry(gb_cache& cache, gb_cache_idx index) {
    gb_assert(entry_is_linked(cache, index));

    gb_cache_entry& entry = cache.entries[index];

    gb_cache_idx p = entry.prev;
    gb_cache_idx n = entry.next;

    if (index == cache.newest_entry) cache.newest_entry = n;
    if (index == cache.oldest_entry) cache.oldest_entry = p;

    cache.entries[n].prev = p;
    cache.entries[p].next = n;

    entry.prev = entry.next = 0;

    return entry;
}

inline void link_cache_entry_to_top(gb_cache& cache, gb_cache_idx index) {
    gb_assert(!entry_is_linked(cache, index));

    gb_cache_entry& entry = cache.entries[index];

    entry.prev = entry.next = 0;
    
    if (!cache.newest_entry) {                      // first entry
        gb_assert(!cache.oldest_entry);
        cache.newest_entry = cache.oldest_entry = index;
    }
    else if (entry.sizeof_data >= cache.big_data_min_size) {
        // Do NOT put big entries to top - instead put them to bottom!
        // This is done to avoid that reading a big entry flushes the complete cache.
        entry.prev                     = cache.oldest_entry;
        cache.entries[entry.prev].next = index;
        cache.oldest_entry             = index;
    }
    else {
        entry.next                     = cache.newest_entry;
        cache.entries[entry.next].prev = index;
        cache.newest_entry             = index;
    }
}

inline void flush_cache_entry(gb_cache& cache, gb_cache_idx index) {
    gb_assert(!entry_is_linked(cache, index));
    
    gb_cache_entry& entry = cache.entries[index];

    freenull(entry.data);
    cache.sum_data_size    -= entry.sizeof_data;
    gb_assert(entry.gbd->cache_index == index); // oops - cache error
    entry.gbd->cache_index  = 0;

    // insert deleted entry in free list
    entry.next            = cache.firstfree_entry;
    cache.firstfree_entry = index;

#if defined(GEN_CACHE_STATS)
    // const char *dbpath = GB_get_db_path(entry.gbd);
    if (entry.reused) {
        GBS_incr_hash(cache.reused, entry.dbpath);
        GBS_write_hash(cache.reuse_sum, entry.dbpath, GBS_read_hash(cache.reuse_sum, entry.dbpath)+entry.reused);
    }
    else {
        GBS_incr_hash(cache.not_reused, entry.dbpath);
    }
    freenull(entry.dbpath);
#endif // GEN_CACHE_STATS
}

void gb_init_cache(GB_MAIN_TYPE *Main) {
    gb_cache& cache = Main->cache;

    if (!cache.entries) {
        cache.entries = (gb_cache_entry *)GB_calloc(sizeof(gb_cache_entry), GB_MAX_CACHED_ENTRIES);

        cache.max_entries       = GB_MAX_CACHED_ENTRIES;
        cache.max_data_size     = GB_TOTAL_CACHE_SIZE;
        cache.big_data_min_size = cache.max_data_size / 4;

        for (gb_cache_idx i=0; i<GB_MAX_CACHED_ENTRIES-1; i++) {
            cache.entries[i].next = i+1;
        }
        cache.firstfree_entry = 1;

#if defined(GEN_CACHE_STATS)
        cache.not_reused = GBS_create_hash(1000, GB_MIND_CASE);
        cache.reused     = GBS_create_hash(1000, GB_MIND_CASE);
        cache.reuse_sum  = GBS_create_hash(1000, GB_MIND_CASE);
#endif // GEN_CACHE_STATS
    }
}

#if defined(GEN_CACHE_STATS)
static long sum_hash_values(const char */*key*/, long val, void *client_data) {
    size_t *sum  = (size_t*)client_data;
    *sum      += val;
    return val;
}
static long list_hash_entries(const char *key, long val, void *client_data) {
    if (client_data) {
        GB_HASH *reuse_sum_hash = (GB_HASH*)client_data;
        long     reuse_sum      = GBS_read_hash(reuse_sum_hash, key);

        printf("%s %li (%5.2f)\n", key, val, (double)reuse_sum/val);
    }
    else {
        printf("%s %li\n", key, val);
    }
    return val;
}
#endif // GEN_CACHE_STATS

void gb_destroy_cache(GB_MAIN_TYPE *Main) {
    // only call this from gb_destroy_main()!
    gb_cache& cache = Main->cache;

    if (cache.entries) {
        gb_assert(cache.newest_entry == 0);         // cache is not flushed 
        gb_assert(cache.sum_data_size == 0);
        freenull(cache.entries);

#if defined(GEN_CACHE_STATS)
        size_t not_reused = 0;
        size_t reused     = 0;
        size_t reuse_sum  = 0;

        GBS_hash_do_loop(cache.reuse_sum, sum_hash_values, &reuse_sum);
        GBS_hash_do_loop(cache.not_reused, sum_hash_values, &not_reused);
        GBS_hash_do_loop(cache.reused, sum_hash_values, &reused);

        size_t overall = not_reused+reused;

        printf("Cache stats:\n"
               "Overall entries:  %zu\n"
               "Reused entries:   %zu (%5.2f%%)\n"
               "Mean reuse count: %5.2f\n",
               overall,
               reused, (double)reused/overall*100.0,
               (double)reuse_sum/reused);

        printf("Not reused:\n");
        GBS_hash_do_sorted_loop(cache.not_reused, list_hash_entries, GBS_HCF_sortedByKey, NULL);
        printf("Reused:\n");
        GBS_hash_do_sorted_loop(cache.reused, list_hash_entries, GBS_HCF_sortedByKey, cache.reuse_sum);

        GBS_free_hash(cache.not_reused);
        GBS_free_hash(cache.reused);
        GBS_free_hash(cache.reuse_sum);
#endif // GEN_CACHE_STATS
    }
}

char *gb_read_cache(GBDATA *gbd) {
    char         *cached_data = NULL;
    gb_cache_idx  index       = gbd->cache_index;

    if (index) {
        gb_cache&       cache = GB_MAIN(gbd)->cache;
        gb_cache_entry& entry = unlink_cache_entry(cache, index);
        gb_assert(entry.gbd == gbd);

        // check validity
        if (GB_GET_EXT_UPDATE_DATE(gbd) > entry.clock) {
            flush_cache_entry(cache, index);
        }
        else {
            link_cache_entry_to_top(cache, index);
            cached_data = entry.data;
#if defined(GEN_CACHE_STATS)
            entry.reused++;
#endif // GEN_CACHE_STATS
        }
    }

    return cached_data;
}

void gb_free_cache(GB_MAIN_TYPE *Main, GBDATA *gbd) {
    gb_cache_idx index = gbd->cache_index;

    if (index) {
        gb_cache& cache = Main->cache;
        unlink_cache_entry(cache, index);
        flush_cache_entry(cache, index);
    }
}

static char *cache_free_some_memory(gb_cache& cache, size_t needed_mem) {
    // free up cache entries until
    // - at least 'needed_mem' bytes are available and
    // - at least one free cache entry exists
    // (if one of the free'd entries has exactly 'needed_mem' bytes size,
    // it will be returned and can be re-used or has to be freed)

    long  avail_mem    = (long)cache.max_data_size - (long)cache.sum_data_size; // may be negative!
    long  need_to_free = needed_mem-avail_mem;
    char *data         = NULL;

    // ignore really big requests (such cache entries will
    // be appended to end of cache list and flushed quickly)
    if (need_to_free>(long)cache.sum_data_size) need_to_free = 0;

    while ((!cache.firstfree_entry || need_to_free>0) && cache.oldest_entry) {
        gb_cache_idx index = cache.oldest_entry;
        gb_assert(index);

        gb_cache_entry& entry = unlink_cache_entry(cache, index);

        need_to_free -= entry.sizeof_data;
        if (entry.sizeof_data == needed_mem) reassign(data, entry.data);
        flush_cache_entry(cache, index);
    }

    return data;
}

char *gb_alloc_cache_index(GBDATA *gbd, size_t size) {
    gb_assert(gbd->cache_index == 0);

    gb_cache&     cache = GB_MAIN(gbd)->cache;
    char         *data  = cache_free_some_memory(cache, size);
    gb_cache_idx  index = cache.firstfree_entry;

    gb_assert(index);
    gb_cache_entry& entry = cache.entries[index];

    cache.firstfree_entry = entry.next;             // remove free element from free-list
    entry.next            = 0;

    // create data
    if (!data) data = (char*)malloc(size);

    entry.sizeof_data = size;
    entry.data        = data;
    entry.gbd         = gbd;
    entry.clock       = GB_GET_EXT_UPDATE_DATE(gbd);
    
#if defined(GEN_CACHE_STATS)
    entry.reused     = 0;
    entry.dbpath     = strdup(GB_get_db_path(gbd));
#endif                                              // GEN_CACHE_STATS

    gbd->cache_index = index;

    link_cache_entry_to_top(cache, index);
    cache.sum_data_size += size; 

        return data;
}

void gb_flush_cache(GBDATA *gbd) {
    gb_cache& cache = GB_MAIN(gbd)->cache;

    while (cache.oldest_entry) {
        gb_cache_idx index = cache.oldest_entry;
        unlink_cache_entry(cache, index);
        flush_cache_entry(cache, index);
    }

    gb_assert(cache.sum_data_size == 0);
}

char *GB_set_cache_size(GBDATA *gbd, size_t size) {
    gb_cache& cache = GB_MAIN(gbd)->cache;

    cache.max_data_size     = size;
    cache.big_data_min_size = cache.max_data_size / 4;
    return 0;
}

