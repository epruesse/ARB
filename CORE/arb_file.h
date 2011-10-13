/* This file is generated by aisc_mkpt.
 * Any changes you make here will be overwritten later!
 */

#ifndef ARB_FILE_H
#define ARB_FILE_H

/* define ARB attributes: */
#ifndef ATTRIBUTES_H
# include <attributes.h>
#endif


/* arb_file.cxx */

#ifndef ARB_CORE_H
#include "arb_core.h"
#endif

long GB_size_of_file(const char *path);
long GB_size_of_FILE(FILE *in);
long GB_mode_of_file(const char *path);
long GB_mode_of_link(const char *path);
bool GB_is_regularfile(const char *path);
bool GB_is_link(const char *path);
bool GB_is_executablefile(const char *path);
bool GB_is_privatefile(const char *path, bool read_private);
bool GB_is_readablefile(const char *filename);
bool GB_is_directory(const char *path);
long GB_getuid_of_file(const char *path);
int GB_unlink(const char *path);
void GB_unlink_or_warn(const char *path, GB_ERROR *error);
GB_ERROR GB_symlink(const char *target, const char *link);
GB_ERROR GB_set_mode_of_file(const char *path, long mode);
char *GB_follow_unix_link(const char *path);
GB_ERROR GB_rename_file(const char *oldpath, const char *newpath);

#else
#error arb_file.h included twice
#endif /* ARB_FILE_H */
