/* xmalloc.h --- interface to never-fail memory allocators.
   Jim Blandy <jimb@gnu.ai.mit.edu> --- July 1994 */

#ifndef XMALLOC_H
#define XMALLOC_H

#include <stddef.h>
#include <stdlib.h>

extern void *xmalloc (size_t);
extern void *xrealloc (void *, size_t);

#endif /* XMALLOC_H */
