// =============================================================== //
//                                                                 //
//   File      : admalloc.cxx                                      //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <unistd.h>
#include <climits>
#include <set>

#include <arb_backtrace.h>
#include "gb_storage.h"

// #define DUMP_MEMBLKS
// #define DUMP_MEMBLKS_AT_EXIT

#ifdef DEBUG
// #define TEST_MEMBLKS
// #define TRACE_ALLOCS
#if defined(WARN_TODO)
#warning unit tests fail when TRACE_ALLOCS is defined (due to wrong sized block during load(?). cant fix atm. see [6672])
#endif
#endif

#define GBM_MAGIC 0x74732876

#define GBM_SYSTEM_PAGE_SIZE 4096                   // 4k Tables
#define GBM_MALLOC_OVERHEAD  32                     // pointer for alloc
#define GBM_TABLE_SIZE       (GBM_SYSTEM_PAGE_SIZE-GBM_MALLOC_OVERHEAD) // usable size of table

#define GBM_ALIGNED    8
#define GBM_LD_ALIGNED 3

#define GBM_MAX_TABLES 16                           // n different sizes -> max = GBM_MAX_TABLES * GBM_ALIGNED
#define GBM_MAX_SIZE   (GBM_MAX_TABLES*GBM_ALIGNED)
#define GBM_MAX_INDEX  256                          // has to be 2 ^ x (with x = GBM_MAX_TABLES ? )


struct gbm_data {
    long      magic;                                // indicates free element
    gbm_data *next;                                 // next free element
};

struct gbm_table {                           // a block containing data
    gbm_table *next;
    gbm_data   data[1];
};

static struct gbm_pool {                                   // one pool for each memory index
    gbm_data  *gds;                                 // free data area
    size_t     size;                                // free size of current table
    size_t     allsize;                             // full size of all tables
    gbm_table *first;                               // link list of tables
    gbm_data  *tables[GBM_MAX_TABLES+1];            // free entries
    long       tablecnt[GBM_MAX_TABLES+1];          // number of free entries
    long       useditems[GBM_MAX_TABLES+1];         // number of used items (everything)
    size_t     extern_data_size;                    // not handled by this routine
    long       extern_data_items;
} gbm_pool4idx[GBM_MAX_INDEX];

static struct {
    char *old_sbrk;
} gbm_global;


#define GBB_INCR       11                           // memsize increment in percent between adjacent clusters
#define GBB_CLUSTERS   64                           // # of different clusters
#define GBB_ALIGN      GBM_LD_ALIGNED               // align memsize of clusters (# of bits)
#define GBB_MINSIZE    GBM_MAX_SIZE                 // minimal size of allocated big block
#define GBB_MAX_TRIALS 4                            // maximal number of clusters to search for an unused block
#define GBB_MAGIC      0x67823747

struct gbb_data;

struct gbb_freedata // part of gbb_data if it`s a free block
{
    // cppcheck-suppress unusedStructMember
    long      magic;
    gbb_data *next;  // next unused memblock
};

struct gbb_data {
    size_t       size;                              // real size of memblock (from `content` to end of block)
    long         allocFromSystem;                   // ==0 -> it`s a block imported by gbm_put_mem
    gbb_freedata content;                           // startposition of block returned to user or chain info for free blocks
};

#define GBB_HEADER_SIZE (sizeof(gbb_data)-sizeof(gbb_freedata))

static struct gbb_Cluster
{
    size_t    size;                                 // minimum size of memblocks in this cluster
    gbb_data *first;                                // first free block

} gbb_cluster[GBB_CLUSTERS+1];


#ifdef TRACE_ALLOCS

class AllocLogEntry {
    void   *block;
    size_t  size;
    long    index;

    mutable BackTraceInfo *trace;

public:
    AllocLogEntry(void *block_, size_t size_, long index_, bool do_trace)
        : block(block_)
        , size(size_)
        , index(index_)
        , trace(do_trace ? GBK_get_backtrace(5) : NULL)
    { }
    AllocLogEntry(const AllocLogEntry& other)
        : block(other.block)
        , size(other.size)
        , index(other.index)
        , trace(other.trace)
    {
        other.trace = NULL;
    }
    ~AllocLogEntry() { if (trace) GBK_free_backtrace(trace); }

    size_t get_size() const { return size; }
    long get_index() const { return index; }
    
    bool operator<(const AllocLogEntry& other) const { return block < other.block; }
    void dump(FILE *out, GB_CSTR message) const { GBK_dump_former_backtrace(trace, out, message); }
};

typedef std::set<AllocLogEntry> AllocLogEntries;

class AllocLogger {
    AllocLogEntries entries;

    const AllocLogEntry *existingEntry(void *block) {
        AllocLogEntries::const_iterator found = entries.find(AllocLogEntry(block, 0, 0, false));
        
        return found == entries.end() ? NULL : &*found;
    }

public:
    AllocLogger() {
    }
    ~AllocLogger() {
        size_t count = entries.size();
        if (count) {
            fprintf(stderr, "%zu non-freed blocks:\n", count);
            AllocLogEntries::const_iterator end = entries.end();
            for (AllocLogEntries::const_iterator entry = entries.begin(); entry != end; ++entry) {
                entry->dump(stderr, "block was allocated from here");
            }
        }
    }

    void allocated(void *block, size_t size, long index) {
        const AllocLogEntry *exists = existingEntry(block);
        if (exists) {
            GBK_dump_backtrace(stderr, "Block allocated again");
            exists->dump(stderr, "Already allocated from here");
        }
        else {
            entries.insert(AllocLogEntry(block, size, index, true));
        }
    }
    void freed(void *block, size_t size, long index) {
        const AllocLogEntry *exists = existingEntry(block);
        if (!exists) {
            if (!gb_isMappedMemory(block)) {
                gb_assert(0);
                // GBK_dump_backtrace(stderr, "Tried to free unallocated block");
            }
        }
        else {
            gb_assert(exists->get_size() == size);
            gb_assert(exists->get_index() == index);
            entries.erase(*exists);
        }
    }
};

static AllocLogger allocLogger;

#endif

inline void free_gbm_table(gbm_table *table) {
    while (table) {
        gbm_table *next = table->next;
        
        free(table);
        table = next;
    }
}

static bool gbm_mem_initialized = false;

void gbm_flush_mem() {
    gb_assert(gbm_mem_initialized);

    for (int i = 0; i<GBM_MAX_INDEX; ++i) {
        gbm_pool& gbm             = gbm_pool4idx[i];
        bool      have_used_items = false;

        for (int t = 0; t < GBM_MAX_TABLES; t++) {
            if (gbm.useditems[t]) {
                have_used_items = true;
                break;
            }
        }

        if (!have_used_items) {
            free_gbm_table(gbm.first);
            memset((char*)&gbm, 0, sizeof(gbm));
        }
    }
}

void gbm_init_mem() {
    if (!gbm_mem_initialized) {
        for (int i = 0; i<GBM_MAX_INDEX; ++i) {
            memset((char *)&gbm_pool4idx[i], 0, sizeof(gbm_pool));
            gbm_pool4idx[i].tables[0] = 0;        // CORE zero get mem
        }
        gbm_global.old_sbrk = (char *)sbrk(0);

        /* init GBB:
         * --------- */

        gbb_cluster[0].size  = GBB_MINSIZE;
        gbb_cluster[0].first = NULL;

        for (int i = 1; i<GBB_CLUSTERS; ++i) {
            long nextSize = gbb_cluster[i-1].size * (100+GBB_INCR);

            nextSize /= 100;
            nextSize >>= GBB_ALIGN;
            nextSize ++;
            nextSize <<= GBB_ALIGN;

            gbb_cluster[i].size  = nextSize;
            gbb_cluster[i].first = NULL;
        }

        // last cluster contains ALL bigger blocks

        gbb_cluster[GBB_CLUSTERS].size  = INT_MAX;
        gbb_cluster[GBB_CLUSTERS].first = NULL;

        gbm_mem_initialized = true;
    }
}

struct ARBDB_memory_manager {
    ARBDB_memory_manager() {
        gb_assert(!gbm_mem_initialized); // there may be only one instance!
        gbm_init_mem();
    }
    ~ARBDB_memory_manager() {
#if defined(DUMP_MEMBLKS_AT_EXIT)
        printf("memory at exit:\n");
        gbm_debug_mem();
#endif // DUMP_MEMBLKS_AT_EXIT
        gbm_flush_mem();
#if defined(DUMP_MEMBLKS_AT_EXIT)
        printf("memory at exit (after flush):\n");
        gbm_debug_mem();
#endif // DUMP_MEMBLKS_AT_EXIT
    }
};
static ARBDB_memory_manager memman;

void GB_memerr()
{
    GB_internal_error("memory allocation error - maybe you're out of swap space?");
}

#ifdef TEST_MEMBLKS

#define TEST() testMemblocks(__FILE__, __LINE__)

void testMemblocks(const char *file, int line)
{
    int idx;

    for (idx=0; idx<GBB_CLUSTERS; idx++)
    {
        struct gbb_Cluster *cl  = &(gbb_cluster[idx]);
        gbb_data           *blk = cl->first;

        while (blk)
        {
            if (blk->size<cl->size)
            {
                fprintf(stderr, "Illegal block (size=%zu) in cluster %i (size=%zu) (%s,%i)\n", blk->size, idx, cl->size, file, line);
                gb_assert(0);
            }
            blk = blk->content.next;
        }
    }
}

#else
#   define TEST()
#endif


#if (MEMORY_TEST==0)

static void imemerr(const char *why)
{
    GB_internal_errorf("Dangerous internal error: '%s'\n"
                       "Inconsistent database: Do not overwrite old files with this database", why);
}

static int getClusterIndex(size_t size) /* searches the index of the
                                           lowest cluster for that:
                                           size <= cluster->size */
{
    int l, m, h;

    if (size<GBB_MINSIZE) return 0;

    l = 1;
    h = GBB_CLUSTERS;

    while (l!=h)
    {
        m = (l+h)/2;
        if (gbb_cluster[m].size < size)  l = m+1;
        else                 h = m;
    }

    gb_assert(l<=GBB_CLUSTERS);

    return l;
}

static void gbm_put_memblk(char *memblk, size_t size) {
    /* gives any memory block (allocated or not)
       into the responsibility of this module;
       the block has to be aligned!!! */

    gbb_data *block;
    int       idx;

    TEST();

#ifdef DUMP_MEMBLKS
    printf("put %p (%li bytes)\n", memblk, size);
#endif

    if (size<(GBB_HEADER_SIZE+GBB_MINSIZE))
    {
        GB_internal_errorf("gmb_put_memblk() called with size below %zu bytes",
                           GBB_HEADER_SIZE+GBB_MINSIZE);
        return;
    }

    block                  = (gbb_data *)memblk;
    block->size            = size-GBB_HEADER_SIZE;
    block->allocFromSystem = 0;

    idx = getClusterIndex(block->size)-1;
    if (idx<0) { // (silences warning in NDEBUG mode)
        gb_assert(0); // should be impossible
        return;
    }

    block->content.next     = gbb_cluster[idx].first;
    block->content.magic    = GBB_MAGIC;
    gbb_cluster[idx].first  = block;

    gb_assert(idx==GBB_CLUSTERS || block->size>=gbb_cluster[idx].size);
    TEST();
}

static char *gbm_get_memblk(size_t size) {
    gbb_data *block  = NULL;
    int       trials = GBB_MAX_TRIALS;
    int       idx;

    TEST();

    idx = getClusterIndex(size);
    gb_assert(gbb_cluster[idx].size>=size);

    while (trials--)    // search a cluster containing a block
    {
        if ((block = gbb_cluster[idx].first)!=NULL) break;  // found!
        if (idx==GBB_CLUSTERS) break;               // last cluster!
        idx++;
    }

    if (!block) // if no unused block -> allocate from system
    {
        int allocationSize;

    allocFromSys :

        allocationSize = (idx==GBB_CLUSTERS
                          ? (size_t)size
                          : (size_t)(gbb_cluster[idx].size)) + GBB_HEADER_SIZE;

        block  = (gbb_data *)GB_calloc(1, allocationSize);
        if (!block) { GB_memerr(); return NULL; }

        block->size = allocationSize-GBB_HEADER_SIZE;
        block->allocFromSystem = 1;

        gb_assert(block->size>=size);

#ifdef DUMP_MEMBLKS
        printf("allocated %li bytes\n", size);
#endif
    }
    else
    {
        gbb_data **blockPtr = &(gbb_cluster[idx].first);

        if (idx==GBB_CLUSTERS)  // last cluster (test for block size necessary)
        {
            while ((block=*blockPtr)!=NULL && block->size<size)
                blockPtr = &(block->content.next);

            if (!block) goto allocFromSys;
            gb_assert(block->size>=size);
        }

        if (block->content.magic!=GBB_MAGIC) { imemerr("bad magic number if free block"); return NULL; }
        *blockPtr = block->content.next;
        memset((char*)&(block->content), 0, size);  // act like calloc()

#ifdef DUMP_MEMBLKS
        printf("using unused block "
               "(add=%p,size=%li, block->size=%li,cluster->size=%li)\n",
               block, size, block->size, gbb_cluster[idx].size);
#endif

        gb_assert(block->size>=size);
    }

    gb_assert(block->size>=size);

    TEST();

    return (char*)&(block->content);
}

inline void *GB_MEMALIGN(size_t alignment, size_t size) {
    void *mem = NULL;
    int   err = posix_memalign(&mem, alignment, size);
    if (err) GBK_terminatef("ARBDB allocation error (errcode=%i)", err);
    return mem;
}

void *gbmGetMemImpl(size_t size, long index) {
    if (size < sizeof(gbm_data)) size = sizeof(gbm_data);
    index &= GBM_MAX_INDEX-1;

    gbm_pool      *ggi   = &gbm_pool4idx[index];
    unsigned long  nsize = (size + (GBM_ALIGNED - 1)) & (-GBM_ALIGNED);

    char *result;
    if (nsize > GBM_MAX_SIZE) {
        ggi->extern_data_size += nsize;
        ggi->extern_data_items++;

        result = gbm_get_memblk((size_t)nsize);
    }
    else {
        unsigned long  pos = nsize >> GBM_LD_ALIGNED;
        gbm_data      *gds = ggi->tables[pos];
        if (gds) {
            ggi->tablecnt[pos]--;
            result = (char *)gds;
            if (gds->magic != GBM_MAGIC) {
                printf("%lX!= %lX\n", gds->magic, (long)GBM_MAGIC);
                GB_internal_error("Dangerous internal error: Inconsistent database: "
                                  "Do not overwrite old files with this database");
            }
            ggi->tables[pos] = ggi->tables[pos]->next;
        }
        else {
            if (ggi->size < nsize) {
                gbm_table *gts = (gbm_table *)GB_MEMALIGN(GBM_SYSTEM_PAGE_SIZE, GBM_TABLE_SIZE);

                if (!gts) { GB_memerr(); return NULL; }

                memset((char *)gts, 0, GBM_TABLE_SIZE);
                ggi->gds = &gts->data[0];
                gts->next = ggi->first; // link tables
                ggi->first = gts;
                ggi->size = GBM_TABLE_SIZE - sizeof(void *);
                ggi->allsize += GBM_TABLE_SIZE;
            }
            result = (char *)ggi->gds;
            ggi->gds = (gbm_data *)(((char *)ggi->gds) + nsize);
            ggi->size -= (size_t)nsize;
        }

        ggi->useditems[pos]++;
        memset(result, 0, nsize); // act like calloc()
    }

#ifdef TRACE_ALLOCS
    allocLogger.allocated(erg, size, index);
#endif
    return result;
}

void gbmFreeMemImpl(void *data, size_t size, long index) {
    if (size < sizeof(gbm_data)) size = sizeof(gbm_data);
    index &= GBM_MAX_INDEX-1;
    
    gbm_pool *ggi   = &gbm_pool4idx[index];
    long      nsize = (size + (GBM_ALIGNED - 1)) & (-GBM_ALIGNED);

#ifdef TRACE_ALLOCS
    allocLogger.freed(data, size, index);
#endif

    if (nsize > GBM_MAX_SIZE) {
        gbb_data *block;

        if (gb_isMappedMemory(data)) {
            block = (gbb_data *)data;

            block->size = size-GBB_HEADER_SIZE;
            block->allocFromSystem = 0;

            if (size>=(GBB_HEADER_SIZE+GBB_MINSIZE)) {
                gbm_put_memblk((char*)block, size);
            }
        }
        else {
            block = (gbb_data *)((char*)data-GBB_HEADER_SIZE);

            ggi->extern_data_size -= (size_t)nsize;
            ggi->extern_data_items--;

            if (block->size<size) { imemerr("block size does not match"); return; }

            if (block->allocFromSystem) {
                free(block);
            }
            else {
                /* printf("put unused block (size=%li block->size=%li)\n",
                   size,block->size); */
                gbm_put_memblk((char*)block, block->size + GBB_HEADER_SIZE);
            }
        }
    }
    else
    {
        if (gb_isMappedMemory(data)) return;    //   @@@ reason: size may be shorter

        gbm_data *gdata = (gbm_data*)data;
        
        if (gdata->magic == GBM_MAGIC) {
            imemerr("double free");
            return;
        }

        long pos = nsize >> GBM_LD_ALIGNED;

        gdata->next      = ggi->tables[pos];
        gdata->magic     = GBM_MAGIC;
        ggi->tables[pos] = gdata;
        ggi->tablecnt[pos]++;
        ggi->useditems[pos]--;
    }
}

#endif // MEMORY_TEST==0

void gbm_debug_mem() {
    int       i;
    int       index;
    long      total = 0;
    long      index_total;
    gbm_pool *ggi;

    printf("Memory Debug Information:\n");
    for (index = 0; index < GBM_MAX_INDEX; index++)
    {
        index_total = 0;
        ggi = &gbm_pool4idx[index];
        for (i = 0; i < GBM_MAX_TABLES; i++)
        {
            index_total += i * GBM_ALIGNED * (int) ggi->useditems[i];
            total += i * GBM_ALIGNED * (int) ggi->useditems[i];

            if (ggi->useditems[i] || ggi->tablecnt[i]) {
                printf("\t'I=%3i' 'Size=%3i' * 'Items %4i' = 'size %7i'    'sum=%7li'   'totalsum=%7li' :   Free %3i\n",
                       index,
                       i * GBM_ALIGNED,
                       (int) ggi->useditems[i],
                       i * GBM_ALIGNED * (int) ggi->useditems[i],
                       index_total,
                       total,
                       (int) ggi->tablecnt[i]);
            }
        }
        if (ggi->extern_data_size) {
            index_total += ggi->extern_data_size;
            total += ggi->extern_data_size;
            printf("\t'I=%3i' External Data Items=%3li = Sum=%3li  'sum=%7li'  'total=%7li\n",
                   index,
                   ggi->extern_data_items,
                   (long)ggi->extern_data_size,
                   index_total,
                   total);
        }
    }

    {
        char *topofmem = (char *)sbrk(0);
        printf("spbrk %lx old %lx size %ti\n",
               (long)topofmem,
               (long)gbm_global.old_sbrk,
               topofmem-gbm_global.old_sbrk);
    }
}

// --------------------------------------------------------------------------------

#if defined(UNIT_TESTS) && 0

#include <test_unit.h>

void TEST_ARBDB_memory() { // not a real unit test - just was used for debugging
#define ALLOCS 69
    long *blocks[ALLOCS];

#if (MEMORY_TEST == 0)
    printf("Before allocations:\n");
    gbm_debug_mem();

#if 1    
    static int16_t non_alloc[75];                   // 150 byte
    gbm_put_memblk((char*)non_alloc, sizeof(non_alloc));

    printf("Added one non-allocated block:\n");
    gbm_debug_mem();
#endif
#endif

    for (int pass = 1; pass <= 2; ++pass) {
        long allocs = 0;

        for (size_t size = 10; size<5000; size += size/3) {
            for (long index = 0; index<3; ++index) {
                if (pass == 1) {
                    long *block = (long*)gbm_get_mem(size, index);

                    block[0] = size;
                    block[1] = index;

                    blocks[allocs++] = block;
                }
                else {
                    long *block = blocks[allocs++];
                    gbm_free_mem(block, (size_t)block[0], block[1]);
                }
            }
        }

#if (MEMORY_TEST == 0)
        if (pass == 1) {
            printf("%li memory blocks allocated:\n", allocs);
            gbm_debug_mem();
        }
        else {
            printf("Memory freed:\n");
            gbm_debug_mem();
        }
        printf("Memory flushed:\n");
        gbm_flush_mem();
        gbm_debug_mem();
#endif

        gb_assert(allocs == ALLOCS);
    }

    GBK_dump_backtrace(stderr, "test");
}


#endif // UNIT_TESTS
