#ifndef ARB_STRING_H
#define ARB_STRING_H

#ifndef ARB_MEM_H
#include "arb_mem.h"
#endif
#ifndef ARB_ASSERT_H
#include "arb_assert.h"
#endif
#ifndef _GLIBCXX_CSTRING
#include <cstring>
#endif

inline char *ARB_strdup(const char *str) {
    char *dup = strdup(str);
    if (!dup) arb_mem::failed_to_allocate(strlen(str));
    return dup;
}

 // @@@ rename prefixes (GB -> ARB)

inline char *GB_strduplen(const char *p, unsigned len) {
    // fast replacement for strdup, if len is known
    if (p) {
        char *neu;

        arb_assert(strlen(p) == len);
        // Note: Common reason for failure: a zero-char was manually printed by a GBS_global_string...-function

        neu = (char*)ARB_alloc(len+1);
        memcpy(neu, p, len+1);
        return neu;
    }
    return 0;
}

inline char *GB_strpartdup(const char *start, const char *end) {
    /* strdup of a part of a string (including 'start' and 'end')
     * 'end' may point behind end of string -> copy only till zero byte
     * if 'end'=('start'-1) -> return ""
     * if 'end'<('start'-1) -> return 0
     * if 'end' == NULL -> copy whole string
     */

    char *result;
    if (end) {
        int len = end-start+1;

        if (len >= 0) {
            const char *eos = (const char *)memchr(start, 0, len);

            if (eos) len = eos-start;
            result = (char*)ARB_alloc(len+1);
            memcpy(result, start, len);
            result[len] = 0;
        }
        else {
            result = 0;
        }
    }
    else { // end = 0 -> return copy of complete string
        result = nulldup(start);
    }

    return result;
}

inline char *GB_strndup(const char *start, int len) {
    return GB_strpartdup(start, start+len-1);
}


const char *GB_date_string(void);
const char *GB_dateTime_suffix(void);
const char *GB_keep_string(char *str);

#else
#error arb_string.h included twice
#endif /* ARB_STRING_H */
