// ================================================================ //
//                                                                  //
//   File      : arb_string.cxx                                     //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2010   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "arb_string.h"

#include <arb_assert.h>

#include <cstring>
#include <cstdlib>


char *GB_strduplen(const char *p, unsigned len) {
    // fast replacement for strdup, if len is known
    if (p) {
        char *neu;

        arb_assert(strlen(p) == len);
        // Note: Common reason for failure: a zero-char was manually printed by a GBS_global_string...-function

        neu = (char*)malloc(len+1);
        memcpy(neu, p, len+1);
        return neu;
    }
    return 0;
}

char *GB_strpartdup(const char *start, const char *end) {
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
            result = (char*)malloc(len+1);
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

char *GB_strndup(const char *start, int len) {
    return GB_strpartdup(start, start+len-1);
}




