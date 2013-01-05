/* This file is generated by aisc_mkpt.
 * Any changes you make here will be overwritten later!
 */

#ifndef CLIENT_H
#define CLIENT_H

/* define ARB attributes: */
#ifndef ATTRIBUTES_H
# include <attributes.h>
#endif


/* client.c */

#include <client_types.h>

aisc_com *aisc_open(const char *path, AISC_Object &main_obj, long magic, GB_ERROR *error);
int aisc_close(aisc_com *link, AISC_Object &object);
int aisc_get(aisc_com *link, int o_type, const AISC_Object &object, ...) __ATTR__SENTINEL;
long *aisc_debug_info(aisc_com *link, int o_type, const AISC_Object &object, int attribute);
int aisc_put(aisc_com *link, int o_type, const AISC_Object &object, ...) __ATTR__SENTINEL;
int aisc_nput(aisc_com *link, int o_type, const AISC_Object &object, ...) __ATTR__SENTINEL;
int aisc_create(aisc_com *link, int father_type, const AISC_Object &father, int attribute, int object_type, AISC_Object &object, ...) __ATTR__SENTINEL;

#else
#error client.h included twice
#endif /* CLIENT_H */
