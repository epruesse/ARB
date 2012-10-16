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

#define PTM_TABLE_SIZE  (1024*256)

#define PTM_TABLE_COUNT 256

#define PTM_MIN_SIZE 1
#define PTM_MAX_SIZE (PTM_TABLE_COUNT-PTM_MIN_SIZE-1)

#define PTM_magic 0xf4

class Memory : public Noncopyable {
    char *data;
    int   data_size;
    long  allsize;

    char *tables[PTM_TABLE_COUNT];

    void          **alloc_ptr;
    unsigned long   alloc_counter;
    unsigned long   alloc_array_size;

    void add_alloc(void *ptr) {
        if (alloc_counter == alloc_array_size) {
            if (alloc_array_size == 0) {
                alloc_array_size = 1;
            }
            alloc_array_size = alloc_array_size * 2;
            void **new_array = (void **)malloc(alloc_array_size * sizeof(void*));

            if (!new_array) GBK_terminate("Out of memory");

            void *old_array = alloc_ptr;
            memcpy(new_array, old_array, alloc_counter * sizeof(void*));
            alloc_ptr = new_array;
            free(old_array);
        }
        alloc_ptr[alloc_counter++] = ptr;
    }
    
    char *get_new_mem(int nsize) {
        if (data_size < nsize) {
            data       = (char *) calloc(1, PTM_TABLE_SIZE);
            data_size  = PTM_TABLE_SIZE;
            allsize   += PTM_TABLE_SIZE;

#ifdef PTM_DEBUG_MEM
            static int less = 0;
            if ((less%10) == 0) {
                printf("Memory usage: %li byte\n", allsize);
            }
            ++less;
#endif

            add_alloc(data);
        }
        char *result = data;

        data      += nsize;
        data_size -= nsize;

        return result;
    }

    void clear_tables() {
        for (int t = 0; t < PTM_TABLE_COUNT; ++t) {
            tables[t] = NULL;
        }
    }
    
public:

    Memory()
        : data(NULL),
          data_size(0),
          allsize(0),
          alloc_ptr(NULL),
          alloc_counter(0),
          alloc_array_size(0)
    {
        clear_tables();
    }

    char *get(int size) {
        pt_assert(size >= PTM_MIN_SIZE);

        if (size > PTM_MAX_SIZE) {
            void *ptr = calloc(1, size);
            return (char *) ptr;
        }

        int   tab = size-PTM_MIN_SIZE;
        char *erg = tables[tab];
        if (erg) {
            long i;
            PT_READ_PNTR(((char *)tables[tab]), i);
            tables[tab] = (char *)i;
        }
        else {
            erg = get_new_mem(size);
        }
        memset(erg, 0, size);
        return erg;
    }

    void put(char *block, int size) {
        pt_assert(size >= PTM_MIN_SIZE);

        if (size > PTM_MAX_SIZE) {
            free(block);
        }
        else {
            int  tab = size-PTM_MIN_SIZE;
            long i   = (long)tables[tab];

            PT_WRITE_PNTR(block, i);
            block[sizeof(PT_PNTR)] = PTM_magic;

            tables[tab] = block;
        }
    }

    bool is_clear() const {
        return
            alloc_ptr        == NULL &&
            alloc_counter    == 0    &&
            alloc_array_size == 0    &&
            data             == NULL &&
            data_size        == 0    &&
            allsize          == 0;
    }

    void clear() {
        for (unsigned long i = 0; i < alloc_counter; ++i) {
            free(alloc_ptr[i]);
        }
        alloc_counter    = 0;
        alloc_array_size = 0;
        freenull(alloc_ptr);

        data      = NULL;
        data_size = 0;
        allsize   = 0;

        clear_tables();

        pt_assert(is_clear());
    }
};

extern struct Memory MEM;

#else
#error PT_mem.h included twice
#endif // PT_MEM_H
