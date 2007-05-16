#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifndef DARWIN
#include <malloc.h>
#endif
#include <string.h>
#include <limits.h>

#include "adlocal.h"
/*#include "arbdb.h"*/

/* #define DUMP_MEMBLKS */

#ifndef NDEBUG
/*  #define TEST_MEMBLKS */
#endif

int gbm_system_page_size = 4096;
#define GBM_MALLOC_OVERHEAD 32          /* pointer for alloc */

#define GBM_MAGIC 0x74732876

#define GBM_TABLE_SIZE  (gbm_system_page_size-GBM_MALLOC_OVERHEAD)  /* 4k Tables */

#define GBM_ALIGNED 8
#define GBM_LD_ALIGNED  3

#define GBM_MAX_TABLES  16      /* n different sizes -> max = GBM_MAX_TABLES * GBM_ALIGNED */
#define GBM_MAX_SIZE    (GBM_MAX_TABLES*GBM_ALIGNED)
#define GBM_MAX_INDEX   256     /* mussed be 2expx */


struct gbm_data_struct {
    long    magic;              /* indicates free element */
    struct  gbm_data_struct *next;      /* next free element */
};

struct gbm_table_struct {       /* a block containing data */
    struct gbm_table_struct *next;
    struct gbm_data_struct  data[1];
};

struct gbm_struct {
    struct gbm_data_struct  *gds;           /* free data area */
    size_t             size;            /* free size of current table */
    size_t             allsize;     /* full size of all tables */
    struct gbm_table_struct *first; /* linkes list of tables */
    struct gbm_data_struct  *tables[GBM_MAX_TABLES + 1];    /* free entries */
    long             tablecnt[GBM_MAX_TABLES + 1];  /* number of free entries */
    long             useditems[GBM_MAX_TABLES + 1]; /* number of used items (everything) */
    size_t      extern_data_size;       /* not handled by this routine */
    long        extern_data_items;
}               gbm_global[GBM_MAX_INDEX];

struct gbm_struct2 {
    char *old_sbrk;
} gbm_global2;


#define GBB_INCR        11      /* memsize increment in percent between
                                   adjacent clusters */
#define GBB_CLUSTERS    64      /* # of different clusters */
#define GBB_ALIGN       GBM_LD_ALIGNED  /* align memsize of clusters (# of bits) */
#define GBB_MINSIZE GBM_MAX_SIZE        /* minimal size of allocated big block */
#define GBB_MAX_TRIALS  4               /* maximal number of clusters to search
                                           for an unused block */
#define GBB_MAGIC   0x67823747

struct gbb_data;

struct gbb_freedata /* part of gbb_data if it`s a free block */
{
    long        magic;
    struct gbb_data *next;  /* next unused memblock */
};

struct gbb_data
{
    size_t  size;           /* real size of memblock
                               (from `content` to end of block) */
    long    allocFromSystem;    /* ==0 -> it`s a block imported by gbm_put_mem */

    struct  gbb_freedata content; /* startposition of block returned to user
                                     or chain info for free blocks */
};

#define GBB_HEADER_SIZE (sizeof(struct gbb_data)-sizeof(struct gbb_freedata))

static struct gbb_Cluster
{
    size_t size;  /* minimum size of memblocks in this cluster */
    struct gbb_data *first; /* first free block */

} gbb_cluster[GBB_CLUSTERS+1];

/* @@@ */
NOT4PERL void *GB_calloc(unsigned int nelem, unsigned int elsize)
{
    size_t size = nelem*elsize;
    void *mem = malloc(size);

    if (mem) {
        memset(mem,0,size);
    }
    else {
        fprintf(stderr,"Panic Error:    Unsufficient memory: tried to get %i*%i bytes : Increase Swap space\n",nelem,elsize);
    }
    return mem;
}



char *GB_strdup(const char *p) {
    /* does strdup(), but working with NULL */
    if (p) return GB_STRDUP(p);
    return NULL;
}

char *GB_strduplen(const char *p, unsigned len) {
    if (p) {
        char *neu;

        ad_assert(strlen(p) == len);
        neu = (char*)malloc(len+1);
        memcpy(neu, p, len+1);
        return neu;
    }
    return 0;
}

char *GB_strpartdup(const char *start, const char *end) {
    /* strdup of a part of a string
     * 'end' may point behind end of string -> copy only till zero byte
     * if 'end'=('start'-1) -> return ""
     * if 'end'<('start'-1) -> return 0
     */

    int   len = end-start+1;
    char *result;

    if (len >= 0) {
        const char *eos = memchr(start, 0, len);

        if (eos) len = eos-start;
        result = malloc(len+1);
        memcpy(result, start, len);
        result[len] = 0;
    }
    else {
        result = 0;
    }
    
    return result;
}

char *GB_strndup(const char *start, int len) {
    return GB_strpartdup(start, start+len-1);
}

NOT4PERL void *GB_recalloc(void *ptr, unsigned int oelem, unsigned int nelem, unsigned int elsize)
{
    size_t nsize = nelem*elsize;
    void *mem = malloc(nsize);

    if (mem) {
        size_t osize = oelem*elsize;

        if (nsize>=osize) {
            memmove(mem, ptr, osize);
            if (nsize>osize) {
                memset(((char*)mem)+osize, 0, nsize-osize);
            }
        }
        else {
            memmove(mem, ptr, nsize);
        }
    }
    else {
        fprintf(stderr,"Panic Error:    Unsufficient memory: tried to get %i*%i bytes : Increase Swap space\n",nelem,elsize);
    }

    return mem;
}


void gbm_init_mem(void)
{
    int i;
    static int flag = 0;

    if (flag) return;

    flag = 1;
    for (i=0;i<GBM_MAX_INDEX;i++)
    {
        memset((char *)&gbm_global[i],0,sizeof(struct gbm_struct));
        gbm_global[i].tables[0] = 0;        /* CORE zero get mem */
    }
    gbm_global2.old_sbrk = (char *)sbrk(0);

    /* init GBB:
     * --------- */

    gbb_cluster[0].size  = GBB_MINSIZE;
    gbb_cluster[0].first = NULL;

    for (i=1; i<GBB_CLUSTERS; i++)
    {
        long nextSize = gbb_cluster[i-1].size * (100+GBB_INCR);

        nextSize /= 100;
        nextSize >>= GBB_ALIGN;
        nextSize ++;
        nextSize <<= GBB_ALIGN;

        gbb_cluster[i].size  = nextSize;
        gbb_cluster[i].first = NULL;

        /*printf("cluster %i: size=%i\n", i, gbb_cluster[i].size);*/
    }

    /* last cluster contains ALL bigger blocks */

    gbb_cluster[GBB_CLUSTERS].size  = INT_MAX;
    gbb_cluster[GBB_CLUSTERS].first = NULL;

    /* give some block to memory-management (testwise) */

#if (defined(DEBUG) && 0)
    {

        int i;

        for (i=200; i<3000; i+=1)
        {
            char *someMem = (char*)calloc(1,(size_t)i);

            if (someMem) gbb_put_memblk(someMem,i);
        }
    }
#endif
}

void GB_memerr(void)
{
    GB_internal_error("memory allocation error - maybe you're out of swap space?");
}

#ifdef TEST_MEMBLKS

#define TEST() testMemblocks(__FILE__,__LINE__)

void testMemblocks(const char *file, int line)
{
    int idx;

    for (idx=0; idx<GBB_CLUSTERS; idx++)
    {
        struct gbb_Cluster *cl = &(gbb_cluster[idx]);
        struct gbb_data *blk = cl->first;

        while (blk)
        {
            if (blk->size<cl->size)
            {
                fprintf(stderr, "Illegal block (size=%li) in cluster %i (size=%li) (%s,%i)\n", blk->size,idx,cl->size,file,line);
                ad_assert(0);
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
    GB_internal_error("Dangerous internal error: '%s'\n"
                      "Inconsistent database: Do not overwrite old files with this database",why);
}

static int getClusterIndex(size_t size) /* searches the index of the
                                           lowest cluster for that:
                                           size <= cluster->size */
{
    int l,m,h;

    if (size<GBB_MINSIZE) return 0;

    l = 1;
    h = GBB_CLUSTERS;

    while (l!=h)
    {
        m = (l+h)/2;
        if (gbb_cluster[m].size < size)  l = m+1;
        else                 h = m;
    }

    ad_assert(l<=GBB_CLUSTERS);

    return l;
}

void gbb_put_memblk(char *memblk, size_t size) /* gives any memory block (allocated or not)
                                                  into the responsibility of this module;
                                                  the block has to be aligned!!! */
{
    struct gbb_data  *block;
    int idx;

    TEST();

#ifdef DUMP_MEMBLKS
    printf("put %p (%li bytes)\n",memblk,size);
#endif

    if (size<(GBB_HEADER_SIZE+GBB_MINSIZE))
    {
        GB_internal_error("gmb_put_memblk() called with size below %i bytes",
                          GBB_HEADER_SIZE+GBB_MINSIZE);
        return;
    }

    block              = (struct gbb_data *)memblk;
    block->size            = size-GBB_HEADER_SIZE;
    block->allocFromSystem = 0;

    idx = getClusterIndex(block->size)-1;
    ad_assert(idx>=0);

    block->content.next     = gbb_cluster[idx].first;
    block->content.magic    = GBB_MAGIC;
    gbb_cluster[idx].first  = block;

    ad_assert(idx==GBB_CLUSTERS || block->size>=gbb_cluster[idx].size);
    TEST();
}

static char *gbb_get_memblk(size_t size)
{
    struct gbb_data  *block = NULL;
    int           trials = GBB_MAX_TRIALS,
        idx;

    TEST();

    idx = getClusterIndex(size);
    ad_assert(gbb_cluster[idx].size>=size);

    while (trials--)    /* search a cluster containing a block */
    {
        if ((block = gbb_cluster[idx].first)!=NULL) break;  /* found! */
        if (idx==GBB_CLUSTERS) break;               /* last cluster! */
        idx++;
    }

    if (!block) /* if no unused block -> allocate from system */
    {
        int allocationSize;

    allocFromSys:

        allocationSize = (idx==GBB_CLUSTERS
                          ? (size_t)size
                          : (size_t)(gbb_cluster[idx].size)) + GBB_HEADER_SIZE;

        block  = (struct gbb_data *)GB_calloc(1, allocationSize);
        if (!block) { GB_memerr(); return NULL; }

        block->size = allocationSize-GBB_HEADER_SIZE;
        block->allocFromSystem = 1;

        ad_assert(block->size>=size);

#ifdef DUMP_MEMBLKS
        printf("allocated %li bytes\n", size);
#endif
    }
    else
    {
        struct gbb_data **blockPtr = &(gbb_cluster[idx].first);

        if (idx==GBB_CLUSTERS)  /* last cluster (test for block size necessary) */
        {
            while ((block=*blockPtr)!=NULL && block->size<size)
                blockPtr = &(block->content.next);

            if (!block) goto allocFromSys;
            ad_assert(block->size>=size);
        }

        if (block->content.magic!=GBB_MAGIC) { imemerr("bad magic number if free block"); return NULL; }
        *blockPtr = block->content.next;
        memset((char*)&(block->content),0,size);    /* act like calloc() */

#ifdef DUMP_MEMBLKS
        printf("using unused block "
               "(add=%p,size=%li, block->size=%li,cluster->size=%li)\n",
               block, size, block->size,gbb_cluster[idx].size);
#endif

        ad_assert(block->size>=size);
    }

    ad_assert(block->size>=size);

    TEST();

    return (char*)&(block->content);
}


char *gbm_get_mem(size_t size, long index)
{
    register unsigned long    nsize, pos;
    char           *erg;
    struct gbm_data_struct *gds;
    struct gbm_struct *ggi;
    if (size < sizeof(struct gbm_data_struct)) size = sizeof(struct gbm_data_struct);
    index &= GBM_MAX_INDEX-1;
    ggi = & gbm_global[index];
    nsize = (size + (GBM_ALIGNED - 1)) & (-GBM_ALIGNED);

    if (nsize > GBM_MAX_SIZE)
    {
        ggi->extern_data_size += nsize;
        ggi->extern_data_items++;

        erg = gbb_get_memblk((size_t)nsize);
        return erg;
    }

    pos = nsize >> GBM_LD_ALIGNED;
    if ( (gds = ggi->tables[pos]) )
    {
        ggi->tablecnt[pos]--;
        erg = (char *)gds;
        if (gds->magic != GBM_MAGIC)
        {
            printf("%lX!= %lX\n",gds->magic,(long)GBM_MAGIC);
            GB_internal_error("Dangerous internal error: Inconsistent database: "
                              "Do not overwrite old files with this database");
        }
        ggi->tables[pos] = ggi->tables[pos]->next;
    }
    else
    {
        if (ggi->size < nsize)
        {
            struct gbm_table_struct *gts = (struct gbm_table_struct *)GB_MEMALIGN(gbm_system_page_size, GBM_TABLE_SIZE);

            if (!gts) { GB_memerr(); return NULL; }

            memset((char *)gts,0,GBM_TABLE_SIZE);
            ggi->gds = &gts->data[0];
            gts->next = ggi->first; /* link tables */
            ggi->first = gts;
            ggi->size = GBM_TABLE_SIZE - sizeof(void *);
            ggi->allsize += GBM_TABLE_SIZE;
        }
        erg = (char *)ggi->gds;
        ggi->gds = (struct gbm_data_struct *)(((char *)ggi->gds) + nsize);
        ggi->size -= (size_t)nsize;
    }

    ggi->useditems[pos]++;
    GB_MEMSET(erg,0,nsize);

    return erg;
}

void gbm_free_mem(char *data, size_t size, long index)
{
    register long    nsize, pos;
    struct gbm_struct *ggi;

    if (size < sizeof(struct gbm_data_struct)) size = sizeof(struct gbm_data_struct);

    index &= GBM_MAX_INDEX-1;
    ggi = & gbm_global[index];
    nsize = (size + (GBM_ALIGNED - 1)) & (-GBM_ALIGNED);

    if (nsize > GBM_MAX_SIZE)
    {
        struct gbb_data *block;

        if (gb_isMappedMemory(data))
        {
            block = (struct gbb_data *)data;

            block->size = size-GBB_HEADER_SIZE;
            block->allocFromSystem = 0;

            /* printf("put mapped Block (size=%li)\n", size); */

            if (size>=(GBB_HEADER_SIZE+GBB_MINSIZE))
                gbb_put_memblk((char*)block, size);

        }
        else
        {
            block = (struct gbb_data *)(data-GBB_HEADER_SIZE);

            ggi->extern_data_size -= (size_t)nsize;
            ggi->extern_data_items--;

            if (block->size<size) { imemerr("block size does not match"); return; }

            if (block->allocFromSystem)
            {
                /* printf("free %li bytes\n", size);  */
                free((char *)block);
            }
            else
            {
                /* printf("put unused block (size=%li block->size=%li)\n",
                   size,block->size); */
                gbb_put_memblk((char*)block,block->size + GBB_HEADER_SIZE);
            }
        }
    }
    else
    {
        if (gb_isMappedMemory(data)) return;    /*   @@@ reason: size may be shorter */
        if ( ((struct gbm_data_struct *)data)->magic == GBM_MAGIC)
            /* double free */
        {
            imemerr("double free");
            return;
        }

        pos = nsize >> GBM_LD_ALIGNED;
        ((struct gbm_data_struct *) data)->next = ggi->tables[pos];
        ((struct gbm_data_struct *) data)->magic = GBM_MAGIC;
        ggi->tables[pos] = ((struct gbm_data_struct *) data);
        ggi->tablecnt[pos]++;
        ggi->useditems[pos]--;
    }
}

#endif /* MEMORY_TEST==0 */

void gbm_debug_mem(GB_MAIN_TYPE *Main)
{
    int i;
    int index;
    long total = 0;
    long index_total;
    struct gbm_struct *ggi;

    printf("Memory Debug Information:\n");
    for (index = 0; index < GBM_MAX_INDEX; index++)
    {
        index_total = 0;
        ggi = &gbm_global[index];
        for (i = 0; i < GBM_MAX_TABLES; i++)
        {
            index_total += i * GBM_ALIGNED * (int) ggi->useditems[i];
            total += i * GBM_ALIGNED * (int) ggi->useditems[i];

            if (ggi->useditems[i] || ggi->tablecnt[i])
            {
                {
                    int j;
                    for (j = index; j < Main->keycnt; j+=GBM_MAX_INDEX ) {
                        if (Main->keys[j].key){
                            printf("%15s", Main->keys[j].key);
                        }else{
                            printf("%15s", "*** unused ****");
                        }
                    }
                }
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
        if ( ggi->extern_data_size)
        {
            index_total += ggi->extern_data_size;
            total += ggi->extern_data_size;
            printf("\t\t'I=%3i' External Data Items=%3li = Sum=%3li  'sum=%7li'  'total=%7li\n",
                   index,
                   ggi->extern_data_items,
                   (long)ggi->extern_data_size,
                   index_total,
                   total);
        }
    }

    {
        char *topofmem = (char *)sbrk(0);
        printf("spbrk %lx old %lx size %i\n",
               (long)topofmem,
               (long)gbm_global2.old_sbrk,
               topofmem-gbm_global2.old_sbrk);
    }
}
