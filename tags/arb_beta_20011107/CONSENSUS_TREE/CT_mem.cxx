/* Modul zur Verwaltung des Speicherplatzes */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void *getmem(size_t size)
{
    void *p;

    p = malloc(size);
    
    if(p == NULL) {
	fprintf(stderr, "Error occured in Module CONSENSUS! Not enough Memorz left\n");
	    }

    memset(p, 0, size);

    return p;

}
