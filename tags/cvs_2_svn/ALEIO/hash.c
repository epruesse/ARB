/* hash.c --- implementation of a string hashing table
   Jim Blandy <jimb@gnu.ai.mit.edu> --- July 1994 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "xmalloc.h"
#include "hash.h"

#ifdef __GNUC__
#define INLINE inline
#else
#define INLINE
#endif


/* data structures, forward declarations */

#define NUM_BUCKETS (503)

struct string_hash {
    /* An array of linked lists.  We use the hash function to decide
       which list to extend or search.  There are NUM_BUCKETS elements
       in this array.  */
    struct bucket_elt **buckets;
};

/* An element of one of the aforementioned linked lists.  The NEXT
   field points to the next element of the chain.  */
struct bucket_elt {
    char *string;
    size_t string_len;
    void *value;
    struct bucket_elt *next;
};


/* Copying strings.  */

static INLINE char *
copy (char *string, size_t string_len)
{
    char *copy = xmalloc (string_len);
    memcpy (copy, string, string_len);

    return copy;
}


/* creating and disposing of hash tables */

struct string_hash *
new_hash_table ()
{
    struct string_hash *table = (struct string_hash *) xmalloc (sizeof (*table));
    struct bucket_elt **buckets
        = (struct bucket_elt **) xmalloc (NUM_BUCKETS * sizeof (*buckets));

    memset (buckets, 0, NUM_BUCKETS * sizeof (*buckets));
    table->buckets = buckets;

    return table;
}


void
free_hash_table (struct string_hash *table)
{
    {
        int i;
        struct bucket_elt *b, *next;

        for (i = 0; i < NUM_BUCKETS; i++)
            for (b = table->buckets[i]; b; b = next)
            {
                next = b->next;
                free (b->string);
                free (b);
            }
    }

    free (table->buckets);
    free (table);
}


/* hashing strings and searching chains.  */

/* From _Compilers: Principles, Techniques, and Tools_, p. 436  */
static unsigned long
hashpjw (char *s, size_t length)
{
    int i;
    unsigned long g, h = 0;

    for (i = 0; i < length; i++)
    {
        h = (h << 4) + s[i];
        g = h & 0xf0000000;
        if (g != 0)
            h = (h ^ (g >> 24)) & 0x0fffffff;
    }

    return h;
}

static INLINE struct bucket_elt *
search_bucket (struct bucket_elt *b, char *s, size_t length)
{
    while (b)
        if (b->string_len == length
            && memcmp (b->string, s, length) == 0)
            return b;
        else
            b = b->next;

    return 0;
}



/* looking up strings */

/* Look up STRING (whose length is STRING_LEN) in TABLE.  If it is
   found, return the address of a void * cell associated with STRING;
   you can store whatever you want here.  If STRING is not found,
   create a new entry for it, and return a pointer to its cell.  */
void **
lookup_hash_table (struct string_hash *table,
                   char *string, size_t string_len)
{
    int bucket = hashpjw (string, string_len) % NUM_BUCKETS;
    struct bucket_elt *b = search_bucket (table->buckets[bucket],
                                          string, string_len);

    if (! b)
    {
        b = (struct bucket_elt *) xmalloc (sizeof (*b));
        b->string = copy (string, string_len);
        b->string_len = string_len;
        b->value = 0;
        b->next = table->buckets[bucket];
        table->buckets[bucket] = b;
    }

    return &(b->value);
}


/* Like lookup_hash_table, but don't create an entry if STRING isn't
   found; return 0 in that case.  */
void **
lookup_hash_table_soft (struct string_hash *table,
                        char *string, size_t string_len)
{
    int bucket = hashpjw (string, string_len) % NUM_BUCKETS;
    struct bucket_elt *b = search_bucket (table->buckets[bucket],
                                          string, string_len);

    if (! b)
        return 0;
    else
        return &(b->value);
}


/* mapping over tables */

/* Apply FUNC to every STRING/cell pair in TABLE.  */
void
map_hash_table (struct string_hash *table,
                void (*func) (char *string, size_t string_len,
                              void **cell,
                              void *closure),
                void *closure)
{
    int i;
    struct bucket_elt *b;

    for (i = 0; i < NUM_BUCKETS; i++)
        for (b = table->buckets[i]; b; b = b->next)
            (*func) (b->string, b->string_len, &(b->value), closure);
}
