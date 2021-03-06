/* Internal toolkit.
 *
 * This file is generated by aisc_mkpt.
 * Any changes you make here will be overwritten later!
 */

#ifndef GB_T_PROT_H
#define GB_T_PROT_H

/* define ARB attributes: */
#ifndef ATTRIBUTES_H
# include <attributes.h>
#endif


/* adseqcompr.cxx */
char *gb_uncompress_by_sequence(GBDATA *gbd, const char *ss, size_t size, GB_ERROR *error, size_t *new_size);

#else
#error gb_t_prot.h included twice
#endif /* GB_T_PROT_H */
