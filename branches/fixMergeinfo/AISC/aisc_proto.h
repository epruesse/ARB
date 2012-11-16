/* This file is generated by aisc_mkpt.
 * Any changes you make here will be overwritten later!
 */

#ifndef AISC_PROTO_H
#define AISC_PROTO_H

/* define ARB attributes: */
#ifndef ATTRIBUTES_H
# include <attributes.h>
#endif


/* aisc.c */

#ifndef AISC_DEF_H
#include "aisc_def.h"
#endif

char *read_aisc_file(const char *path, const Location *loc);

/* aisc_commands.c */
const char *formatted(const char *format, ...) __ATTR__FORMAT(1);

/* aisc_var_ref.c */
char *get_var_string(const Data &data, char *var, bool allow_missing_var);

#else
#error aisc_proto.h included twice
#endif /* AISC_PROTO_H */
