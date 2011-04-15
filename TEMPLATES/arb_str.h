// ================================================================= //
//                                                                   //
//   File      : arb_str.h                                           //
//   Purpose   : inlined string functions                            //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2002        //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef ARB_STR_H
#define ARB_STR_H

#ifndef _GLIBCXX_CSTDDEF
#include <cstddef>
#endif
#ifndef _GLIBCXX_CCTYPE
#include <cctype>
#endif

inline int ARB_stricmp(const char *s1, const char *s2) {
    /*! Like strcmp but ignoring case */

    int    cmp = 0;
    size_t idx = 0;
    while (!cmp) {
        if (!s1[idx]) return s2[idx] ? -1 : 0;
        if (!s2[idx]) return 1;
        cmp = tolower(s1[idx]) - tolower(s2[idx]);
        ++idx;
    }
    return cmp;
}
inline int ARB_strscmp(const char *s1, const char *s2) {
    /*! compares the beginning of two strings
     * (Note: always returns 0 if one the the strings is empty)
     */

    int    cmp = 0;
    size_t idx = 0;
    while (!cmp) {
        if (!s1[idx] || !s2[idx]) break;
        cmp = s1[idx] - s2[idx];
        ++idx;
    }
    return cmp;
}

inline void ARB_strupper(char *s) { for (int i = 0; s[i]; ++i) s[i] = toupper(s[i]); } // strupr
inline void ARB_strlower(char *s) { for (int i = 0; s[i]; ++i) s[i] = tolower(s[i]); } // strlwr


#else
#error arb_str.h included twice
#endif // ARB_STR_H
