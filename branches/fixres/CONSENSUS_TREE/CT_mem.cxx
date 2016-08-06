// memory handling

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <arb_mem.h>

void *getmem(size_t size) { // @@@ replace by ARB_calloc
    void *p = ARB_alloc(size);
    memset(p, 0, size);
    return p;
}
