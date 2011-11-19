/* memory handling */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void *getmem(size_t size)
{
    void *p;

    p = malloc(size);

    if (!p) {
        fprintf(stderr, "Error occurred in Module CONSENSUS! Not enough Memory left\n");
        return 0;
    }

    memset(p, 0, size);
    return p;
}
