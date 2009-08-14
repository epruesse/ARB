//  ==================================================================== //
//                                                                       //
//    File      : inline.h                                               //
//    Purpose   : general purpose inlined functions                      //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in June 2002             //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#ifndef INLINE_H
#define INLINE_H

#include <cctype>

#ifndef __cplusplus
// this header only works with c++
// those functions needed by ARBDB are duplicated in adstring.c (with GBS_-prefix)
#error inline.h may be used in C++ only
#endif

/** Like strcmp but ignoring case */
inline int ARB_stricmp(const char *s1, const char *s2) {
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
/** compares the beginning of two strings
    (Note: always returns 0 if one the the strings is empty) */
inline int ARB_strscmp(const char *s1, const char *s2) {
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

inline unsigned char safeCharIndex(char c) { return static_cast<unsigned char>(c); }

#else
#error inline.h included twice
#endif // INLINE_H

