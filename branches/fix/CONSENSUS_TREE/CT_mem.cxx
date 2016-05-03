// memory handling

#include <cstdlib>
#include <cstring>
#include <cstdio>

void *getmem(size_t size) {
    void *p = malloc(size);
    if (!p) {
        fprintf(stderr, "Error occurred in Module CONSENSUS! Not enough Memory left\n");
        return 0;
    }

    memset(p, 0, size);
    return p;
}
