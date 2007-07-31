/* xmalloc.h --- never-fail memory allocators.
   Jim Blandy <jimb@gnu.ai.mit.edu> --- July 1994 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

/* Under FreeBSD 1.1.5.1, it is unfortunately necessary to #include
   this after <stdio.h>, because <malloc.h> assumes that, if you're
   using an ANSI C compiler, the FILE typedef will be defined.  Rah.  */
#include "xmalloc.h"

#ifdef __GNUC__
#define INLINE inline
#else
#define INLINE
#endif

static INLINE void *
check_ptr (void *p)
{
  if (! p)
    {
      fprintf (stderr, "out of memory\n");
      exit (2);
    }
  else
    return p;
}

void *
xmalloc (size_t n)
{
  return check_ptr (malloc (n));
}

void *
xrealloc (void *p, size_t n)
{
  if (p)
    return check_ptr (realloc (p, n));
  else
    return check_ptr (malloc (n));
}
