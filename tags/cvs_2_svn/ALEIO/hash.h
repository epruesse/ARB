/* hash.h --- interface to string hashing table
   Jim Blandy <jimb@gnu.ai.mit.edu> --- July 1994 */

#ifndef HASH_H
#define HASH_H

/* Create a new, empty string hash table.  */
extern struct string_hash *new_hash_table (void);

/* Look up STRING (whose length is STRING_LEN) in TABLE.  If it is
   found, return the address of a void * cell associated with STRING;
   you can store whatever you want here.  If STRING is not found,
   create a new entry for it, initialize its cell to zero, and return
   a pointer to the cell.  */
extern void **lookup_hash_table (struct string_hash *TABLE,
                                 char *STRING, size_t STRING_LEN);

/* Like lookup_hash_table, but don't create an entry if STRING isn't
   found; return 0 in that case.  */
extern void **lookup_hash_table_soft (struct string_hash *TABLE,
                                      char *STRING, size_t STRING_LEN);

/* Apply FUNC to every STRING/cell pair in TABLE, and CLOSURE.  */
extern void map_hash_table (struct string_hash *TABLE,
                            void (*func) (char *STRING, size_t STRING_LEN,
                                          void **CELL,
                                          void *CLOSURE),
                            void *CLOSURE);

/* Free all memory associated with TABLE.
   This frees everything except what the value cells point to (since
   that could be complicated).  */
extern void free_hash_table (struct string_hash *TABLE);


/* For each sort of thing you want to hash, it's probably a good idea
   to define macros for the update and index operations that do the
   right sort of casting.  */

#endif /* HASH_H */
