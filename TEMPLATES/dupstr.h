// =============================================================== //
//                                                                 //
//   File      : dupstr.h                                          //
//   Purpose   : C-string (heap-copies) handling                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in January 2010   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef DUPSTR_H
#define DUPSTR_H

/* --------------------------------------------------------------------------------
 * The following function handle char*'s, which either own a heap copy or are NULL.
 *
 * freeset:  assigns a heap-copy to a variable (variable is automatically freed)
 * freenull: assigns NULL to a variable (variable is automatically freed)
 * freedup:  similar to freeset, but strdup's the rhs-expression
 * reassign: similar to freeset, but rhs must be variable and will be set to NULL
 * nulldup:  like strdup, but pass-through NULL
 *
 * Notes:
 * - freeset, freedup and reassign may safely use the changed variable in the rhs-expression!
 * - freeset and freenull work with any pointer-type allocated using malloc et al.
 */


#ifndef _GLIBCXX_CSTRING
#include <cstring>
#endif
#ifndef _GLIBCXX_CSTDLIB
#include <cstdlib>
#endif

#ifdef __cplusplus

template<typename T>
inline void freenull(T *& var) {
    free(var);
    var = NULL;
}

template<typename T>
inline void freeset(T *& var, T *heapcopy) {
    free(var);
    var = heapcopy;
}

inline char *nulldup(const char *str) {
    return str ? strdup(str) : NULL;
}
inline void freedup(char *& strvar, const char *no_heapcopy) {
    freeset(strvar, nulldup(no_heapcopy));
}
inline void reassign(char *& dstvar, char *& srcvar) {
    freeset(dstvar, srcvar);
    srcvar = NULL;
}

#endif // __cplusplus

#else
#error dupstr.h included twice
#endif // DUPSTR_H
