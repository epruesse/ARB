//   Coded by Ralf Westram (coder@reallysoft.de)                 //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //

#ifndef AISC_INLINE_H
#define AISC_INLINE_H

#ifndef _STRING_H
#include <string.h>
#endif
#ifndef _STDLIB_H
#include <stdlib.h>
#endif

#define EOSTR    0
#define BEG_STR1 '('
#define BEG_STR2 '~'
#define END_STR1 '~'
#define END_STR2 ')'

inline bool is_SPACE(char c) { return c == ' ' || c == '\t'; }
inline bool is_SEP(char c)   { return c == ',' || c == ';'; }
inline bool is_LF(char c)    { return c == '\n'; }
inline bool is_EOS(char c)   { return c == EOSTR; }

inline bool is_SPACE_LF(char c)         { return is_SPACE(c) || is_LF(c); }
inline bool is_SPACE_LF_EOS(char c)     { return is_SPACE_LF(c) || is_EOS(c); }
inline bool is_SPACE_SEP_LF_EOS(char c) { return is_SPACE_LF_EOS(c) || is_SEP(c); }
inline bool is_LF_EOS(char c)           { return is_LF(c) || is_EOS(c); }
inline bool is_SEP_LF_EOS(char c)       { return is_SEP(c) || is_LF_EOS(c); }

inline void SKIP_SPACE_LF(const char *& var) { while (is_SPACE_LF(*var)) ++var; }
inline void SKIP_SPACE_LF(char *& var)       { while (is_SPACE_LF(*var)) ++var; }

inline void SKIP_SPACE_LF_BACKWARD(const char *& var, const char *strStart) { while (is_SPACE_LF(*var) && var>strStart) --var; }
inline void SKIP_SPACE_LF_BACKWARD(char *& var, const char *strStart)       { while (is_SPACE_LF(*var) && var>strStart) --var; }



inline char *strduplen(const char *s, int len) {
    char *d = (char*)malloc(len+1);
    memcpy(d, s, len);
    d[len] = 0;
    return d;
}

inline char *copy_string_part(const char *first, const char *last) {
    return strduplen(first, last-first+1);
}


#else
#error aisc_inline.h included twice
#endif // AISC_INLINE_H
