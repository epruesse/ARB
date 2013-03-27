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
#ifndef _GLIBCXX_CSTRING
#include <cstring>
#endif


inline int ARB_stricmp(const char *s1, const char *s2) {
    //! Like strcmp but ignoring case

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

inline bool ARB_strBeginsWith(const char *str, const char *with) {
    /*! returns true if 'str' begins with 'with'
     */

    for (size_t idx = 0; with[idx]; ++idx) {
        if (str[idx] != with[idx]) return false;
    }
    return true;
}

inline int ARB_strNULLcmp(const char *s1, const char *s2) {
    /*! Like strcmp, but allows NULL as arguments
     * NULL is bigger than anything else (i.e. it sorts "missing" values to the end)
     */
    return
        s1
        ? (s2 ? strcmp(s1, s2) : -1)
        : (s2 ? 1 : 0);
}


inline void ARB_strupper(char *s) { for (int i = 0; s[i]; ++i) s[i] = toupper(s[i]); } // strupr
inline void ARB_strlower(char *s) { for (int i = 0; s[i]; ++i) s[i] = tolower(s[i]); } // strlwr

// ----------------------------------------
// define the following inlines only if we have string 
#ifdef _GLIBCXX_STRING

inline bool beginsWith(const std::string& str, const std::string& start) {
    return str.find(start) == 0;
}

inline bool endsWith(const std::string& str, const std::string& postfix) {
    size_t slen = str.length();
    size_t plen = postfix.length();

    if (plen>slen) { return false; }
    return str.substr(slen-plen) == postfix;
}

#else

#define beginsWith include_string_b4_arb_str_4_beginsWith
#define endsWith include_string_b4_arb_str_4_endsWith

#endif

#else
#error arb_str.h included twice
#endif // ARB_STR_H
