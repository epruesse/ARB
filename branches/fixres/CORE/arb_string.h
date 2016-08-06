#ifndef ARB_STRING_H
#define ARB_STRING_H

#ifndef ARB_MEM_H
#include "arb_mem.h"
#endif
#ifndef _GLIBCXX_CSTRING
#include <cstring>
#endif

inline char *ARB_strdup(const char *str) {
    char *dup = strdup(str);
    if (!dup) arb_mem::failed_to_allocate(strlen(str));
    return dup;
}

 // @@@ inline strduppers:
char *GB_strduplen(const char *p, unsigned len);
char *GB_strpartdup(const char *start, const char *end);
char *GB_strndup(const char *start, int len);

const char *GB_date_string(void);
const char *GB_dateTime_suffix(void);
const char *GB_keep_string(char *str);

#else
#error arb_string.h included twice
#endif /* ARB_STRING_H */
