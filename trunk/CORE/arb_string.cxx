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




