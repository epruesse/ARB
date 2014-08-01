// =============================================================== //
//                                                                 //
//   File      : PT_mem.h                                          //
//   Purpose   : memory handling for ptserver                      //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2012   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef PT_MEM_H
#define PT_MEM_H

#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef ARB_CORE_H
#include <arb_core.h>
#endif
#ifndef PT_TOOLS_H
#include "PT_tools.h"
#endif
#ifndef ARB_MISC_H
#include <arb_misc.h>
#endif

#define PTM_MANAGED_MEMORY // comment-out to use malloc/free => can use valgrind

#if defined(PTM_MANAGED_MEMORY)
// # define PTM_MEM_DUMP_STATS
// #  define PTM_MEM_CHECKED_FREE // careful: slow as hell!
#endif

#define PTM_MIN_SIZE (int(sizeof(PT_PNTR))+1) // see .@PTM_MIN_SIZE_RESTRICTED

#if defined(PTM_MANAGED_MEMORY)

#define PTM_TABLE_SIZE  (1024*256)

#define PTM_TABLE_COUNT     256
#define PTM_ELEMS_PER_BLOCK 256

#define PTM_MAX_SIZE (PTM_TABLE_COUNT+PTM_MIN_SIZE-1)

#define PTM_magic 0xf4

#if defined(PTM_MEM_DUMP_STATS)
const char *get_blocksize_description(int blocksize); // declare this elsewhere
#endif


class MemBlock : virtual Noncopyable {
    char     *data;
    MemBlock *next;
public:
    MemBlock(int size, MemBlock*& prev)
        : data((char*)malloc(size)),
          next(prev)
    {
        if (!data) GBK_terminate("out of memory");
        prev = NULL;
    }

    ~MemBlock() {
        if (next) {
            MemBlock *del = next;
            next          = NULL;

            while (del) {
                MemBlock *n = del->next;
                del->next   = NULL;

                delete del;
                del = n;
            }
        }
        free(data);
    }

    char *get_memory() { return data; }

    bool contains(char *somemem, int blocksize) const {
        pt_assert(somemem);
        pt_assert(blocksize>0);
        return
            (somemem >= data && somemem < (data+blocksize))
            ||
            (next && next->contains(somemem, blocksize));
    }

#if defined(PTM_MEM_DUMP_STATS)
    size_t count() const {
        const MemBlock *b = this;
        
        size_t cnt = 1;
        while (b->next) {
            b = b->next;
            ++cnt;
        }

        return cnt;
    }
#endif
};

class MemBlockManager : virtual Noncopyable {
    MemBlock *block[PTM_TABLE_COUNT];
    long      allsize;

    static int blocksize4size(int forsize) {
        return forsize * PTM_ELEMS_PER_BLOCK;
    }

public:
#if defined(PTM_MEM_DUMP_STATS)
    bool dump_stats;
#endif

    static int size2idx(int forsize) { return forsize-PTM_MIN_SIZE; }
    static int idx2size(int idx) { return idx+PTM_MIN_SIZE; }

    MemBlockManager()
        : allsize(0)
#if defined(PTM_MEM_DUMP_STATS)
        , dump_stats(true)
#endif
    {
        for (int b = 0; b<PTM_TABLE_COUNT; ++b) {
            block[b] = NULL;
        }
    }
    ~MemBlockManager() {
        clear();
    }

    char *get_block(int forsize) {
        int allocsize = blocksize4size(forsize);
        int tab       = size2idx(forsize);

        block[tab]  = new MemBlock(allocsize, block[tab]);
        allsize    += allocsize;

        return block[tab]->get_memory();
    }

#if defined(PTM_MEM_CHECKED_FREE)
    bool block_has_size(char *vblock, int size) const {
        int tab = size2idx(size);
        if (block[tab]) {
            if (block[tab]->contains(vblock, blocksize4size(size))) {
                return true;
            }
        }
        return false;
    }
#endif

#if defined(PTM_MEM_DUMP_STATS)
    void dump_max_memory_usage(FILE *out) const {
        fflush_all();
        long sum = 0;
        for (int b = 0; b<PTM_TABLE_COUNT; ++b) {
            if (block[b]) {
                int    size           = idx2size(b);
                size_t blocks         = block[b]->count();
                size_t allocated4size = blocks * blocksize4size(size);
                int    elemsPerBlock  = blocksize4size(size)/size;
                size_t maxElements    = blocks*elemsPerBlock;
                int    percent        = double(allocated4size)/double(allsize)*100+0.5;

                fprintf(out, "blocksize: %2i  allocated: %7s [~%2i%%]  ", size, GBS_readable_size(allocated4size, "b"), percent);
                fprintf(out, "<~%12s", GBS_readable_size(maxElements, "Blocks"));

                const char *desc = get_blocksize_description(size);
                if (desc) fprintf(out, " [%s]", desc);

                fputc('\n', out);

                sum += allocated4size;
            }
        }
        fprintf(out, "sum of above:  %s\n", GBS_readable_size(sum, "b"));
        fprintf(out, "overall alloc: %s\n", GBS_readable_size(allsize, "b"));
        fflush_all();
    }
#endif

    bool is_clear() const {
        for (int b = 0; b<PTM_TABLE_COUNT; ++b) {
            if (block[b]) return false;
        }
        return true;
    }

    void clear() {
#if defined(PTM_MEM_DUMP_STATS)
        if (dump_stats) dump_max_memory_usage(stderr);
#endif
        for (int b = 0; b<PTM_TABLE_COUNT; ++b) {
            delete block[b];
            block[b] = NULL;
        }
        allsize = 0;
    }

};

class Memory : virtual Noncopyable {
    MemBlockManager  manager;
    char            *free_data[PTM_TABLE_COUNT];

    void alloc_new_blocks(int forsize) {
        int tab = MemBlockManager::size2idx(forsize);
        pt_assert(free_data[tab] == NULL);

        char *prevPos = NULL;
        {
            char *block = manager.get_block(forsize);
            char *wp    = block;

            for (int b = 0; b<PTM_ELEMS_PER_BLOCK; ++b) {
                PT_write_pointer(wp, prevPos);
                wp[sizeof(PT_PNTR)] = PTM_magic;

                prevPos  = wp;
                wp      += forsize;
            }
        }

        free_data[tab] = prevPos;
    }

    void clear_tables() {
        for (int t = 0; t < PTM_TABLE_COUNT; ++t) {
            free_data[t] = NULL;
        }
    }

public:

    Memory() { clear_tables(); }

#if defined(PTM_MEM_DUMP_STATS)
    void dump_stats(bool dump) { manager.dump_stats = dump; }
#endif

    void *get(int size) {
        pt_assert(size >= PTM_MIN_SIZE);

        if (size > PTM_MAX_SIZE) {
            void *ptr = calloc(1, size);
            return (char *) ptr;
        }

        int   tab = MemBlockManager::size2idx(size);
        char *erg = free_data[tab];
        if (!erg) {
            alloc_new_blocks(size);
            erg = free_data[tab];
        }

        pt_assert(erg);
        free_data[tab] = PT_read_pointer<char>(free_data[tab]);
        memset(erg, 0, size);
        return erg;
    }

#if defined(PTM_MEM_CHECKED_FREE)
    bool block_has_size(void *vblock, int size) { return manager.block_has_size((char*)vblock, size); }
#endif

    void put(void *vblock, int size) {
        pt_assert(size >= PTM_MIN_SIZE);

        char *block = (char*)vblock;
        if (size > PTM_MAX_SIZE) {
            free(block);
        }
        else {
#if defined(PTM_MEM_CHECKED_FREE)
            if (!block_has_size(vblock, size)) {
                for (int idx = 0; idx<PTM_TABLE_COUNT; ++idx) {
                    int isize = manager.idx2size(idx);
                    if (isize != size && block_has_size(vblock, isize)) {
                        pt_assert(size == isize);
                    }
                }
            }
#endif
            int tab = MemBlockManager::size2idx(size);
            PT_write_pointer(block, free_data[tab]); // Note: PTM_MIN_SIZE_RESTRICTED by amount written here
            block[sizeof(PT_PNTR)] = PTM_magic;
            free_data[tab] = block;
        }
    }

    bool is_clear() const { return manager.is_clear(); }
    void clear() {
        clear_tables();
        manager.clear();
        pt_assert(is_clear());
    }
};
#else // !defined(PTM_MANAGED_MEMORY)

struct Memory { // plain version allowing to use memory-checker
    void clear() {}
    bool is_clear() const { return true; }
    void *get(int size) { return calloc(1, size); } // @@@ use malloc and initialize objects instead, then stop cleaning memory in Memory
    void put(void *block, int) { free(block); }
};

#endif

extern Memory MEM;

#else
#error PT_mem.h included twice
#endif // PT_MEM_H
