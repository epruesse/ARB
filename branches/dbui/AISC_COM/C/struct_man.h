/* This file is generated by aisc_mkpt.
 * Any changes you make here will be overwritten later!
 */

#ifndef STRUCT_MAN_H
#define STRUCT_MAN_H

/* define ARB attributes: */
#ifndef ATTRIBUTES_H
# include <attributes.h>
#endif


/* struct_man.c */

struct aisc_hash_node;

long aisc_read_hash(aisc_hash_node **table, const char *key);
const char *aisc_link(dllpublic_ext *father, dllheader_ext *object);
const char *aisc_unlink(dllheader_ext *object);
long aisc_find_lib(dllpublic_ext *parent, char *ident);
void trf_create(long old, long new_item);
void trf_link(long old, long *dest);
void trf_begin(void);
void trf_commit(int errors);

#else
#error struct_man.h included twice
#endif /* STRUCT_MAN_H */
