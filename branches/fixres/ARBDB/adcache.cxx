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
#include "gb_main.h"
#include "gb_tune.h"

struct gb_cache_entry {
    GBENTRY      *gbe;
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
    gb_assert(entry.gbe->cache_index == index); // oops - cache error
    entry.gbe->cache_index  = 0;

    // insert deleted entry in free list
    entry.next            = cache.firstfree_entry;
    cache.firstfree_entry = index;

#if defined(GEN_CACHE_STATS)
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

gb_cache::gb_cache()
    : entries((gb_cache_entry *)ARB_calloc(sizeof(gb_cache_entry), GB_MAX_CACHED_ENTRIES)),
      firstfree_entry(1),
      newest_entry(0),
      oldest_entry(0),
      sum_data_size(0),
      max_data_size(GB_TOTAL_CACHE_SIZE),
      big_data_min_size(max_data_size / 4)
{
    for (gb_cache_idx i=0; i<GB_MAX_CACHED_ENTRIES-1; i++) {
        entries[i].next = i+1;
    }

#if defined(GEN_CACHE_STATS)
    not_reused = GBS_create_hash(1000, GB_MIND_CASE);
    reused     = GBS_create_hash(1000, GB_MIND_CASE);
    reuse_sum  = GBS_create_hash(1000, GB_MIND_CASE);
#endif // GEN_CACHE_STATS
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

gb_cache::~gb_cache() {
    if (entries) {
        gb_assert(newest_entry == 0); // cache has to be flushed before!
        gb_assert(sum_data_size == 0);
        freenull(entries);

#if defined(GEN_CACHE_STATS)
        size_t NotReUsed = 0;
        size_t ReUsed    = 0;
        size_t ReUseSum  = 0;

        GBS_hash_do_loop(reuse_sum, sum_hash_values, &ReUseSum);
        GBS_hash_do_loop(not_reused, sum_hash_values, &NotReUsed);
        GBS_hash_do_loop(reused, sum_hash_values, &ReUsed);

        size_t overall = NotReUsed+ReUsed;

        printf("Cache stats:\n"
               "Overall entries:  %zu\n"
               "Reused entries:   %zu (%5.2f%%)\n"
               "Mean reuse count: %5.2f\n",
               overall,
               ReUsed, (double)ReUsed/overall*100.0,
               (double)ReUseSum/ReUsed);

        printf("Not reused:\n");
        GBS_hash_do_sorted_loop(not_reused, list_hash_entries, GBS_HCF_sortedByKey, NULL);
        printf("Reused:\n");
        GBS_hash_do_sorted_loop(reused, list_hash_entries, GBS_HCF_sortedByKey, reuse_sum);

        GBS_free_hash(not_reused);
        GBS_free_hash(reused);
        GBS_free_hash(reuse_sum);
#endif // GEN_CACHE_STATS
    }
}

char *gb_read_cache(GBENTRY *gbe) {
    char         *cached_data = NULL;
    gb_cache_idx  index       = gbe->cache_index;

    if (index) {
        gb_cache&       cache = GB_MAIN(gbe)->cache;
        gb_cache_entry& entry = unlink_cache_entry(cache, index);
        gb_assert(entry.gbe == gbe);

        // check validity
        if (gbe->update_date() > entry.clock) {
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

void gb_free_cache(GB_MAIN_TYPE *Main, GBENTRY *gbe) {
    gb_cache_idx index = gbe->cache_index;

    if (index) {
        gb_cache& cache = Main->cache;
        unlink_cache_entry(cache, index);
        flush_cache_entry(cache, index);
    }
}

static void gb_uncache(GBCONTAINER *gbc);
void gb_uncache(GBENTRY *gbe) { gb_free_cache(GB_MAIN(gbe), gbe); }

inline void gb_uncache(GBDATA *gbd) {
    if (gbd->is_container()) gb_uncache(gbd->as_container());
    else                     gb_uncache(gbd->as_entry());
}
static void gb_uncache(GBCONTAINER *gbc) {
    for (GBDATA *gb_child = GB_child(gbc); gb_child; gb_child = GB_nextChild(gb_child)) {
        gb_uncache(gb_child);
    }
}
void GB_flush_cache(GBDATA *gbd) {
    // flushes cache of DB-entry or -subtree
    gb_uncache(gbd);
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

char *gb_alloc_cache_index(GBENTRY *gbe, size_t size) {
    gb_assert(gbe->cache_index == 0);

    gb_cache&  cache = GB_MAIN(gbe)->cache;
    char      *data  = cache_free_some_memory(cache, size);
    gb_cache_idx index = cache.firstfree_entry;

    gb_assert(index);
    gb_cache_entry& entry = cache.entries[index];

    cache.firstfree_entry = entry.next;             // remove free element from free-list
    entry.next            = 0;

    // create data
    if (!data) data = (char*)ARB_alloc(size);

    entry.sizeof_data = size;
    entry.data        = data;
    entry.gbe         = gbe;
    entry.clock       = gbe->update_date();
    
#if defined(GEN_CACHE_STATS)
    entry.reused     = 0;
    entry.dbpath     = ARB_strdup(GB_get_db_path(gbe));
#endif                                              // GEN_CACHE_STATS

    gbe->cache_index = index;

    link_cache_entry_to_top(cache, index);
    cache.sum_data_size += size; 

    return data;
}

char *GB_set_cache_size(GBDATA *gbd, size_t size) {
    gb_cache& cache = GB_MAIN(gbd)->cache;

    cache.max_data_size     = size;
    cache.big_data_min_size = cache.max_data_size / 4;
    return 0;
}

