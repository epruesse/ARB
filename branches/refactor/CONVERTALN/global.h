// =============================================================== //
//                                                                 //
//   File      : global.h                                          //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GLOBAL_H
#define GLOBAL_H

#ifndef _CPP_CSTDIO
#include <cstdio>
#endif
#ifndef FILEBUFFER_H
#include <FileBuffer.h>
#endif

class WrapMode;

enum WrapBug {
    // defines various wrapping bugs (produced by original code)
    WRAP_CORRECTLY = 0,

    // bit values:
    WRAPBUG_WRAP_AT_SPACE   = 1,
    WRAPBUG_WRAP_BEFORE_SEP = 2,
    WRAPBUG_UNIDENTIFIED    = 4,
};

struct OrgInfo;
struct SeqInfo;
struct Comments;
struct Emblref;
struct Embl;
struct GenbankRef;
struct GenBank;
struct Macke;
struct Paup;
struct Nbrf;
struct Seq;
struct Alignment;
class Reader;

#ifndef PROTOTYPES_H
#include "prototypes.h"
#endif

#define ca_assert(cond) arb_assert(cond)

#define UNCOVERED() ca_assert(0)

#define LINESIZE  126
#define LONGTEXT  5000
#define TOKENSIZE 80

// max. length of lines
#define EMBLMAXLINE  74
#define GBMAXLINE    78
#define MACKEMAXLINE 78

// indentation to part behind key
#define EMBLINDENT 5
#define GBINDENT   12

// indents for RDP-defined comments
#define RDP_SUBKEY_INDENT    2
#define RDP_CONTINUED_INDENT 6

#define p_nonkey_start 5

// --------------------

inline int min(int t1, int t2) { return t1<t2 ? t1 : t2; }
inline int max(int t1, int t2) { return t1>t2 ? t1 : t2; }

inline bool str_equal(const char *s1, const char *s2) { return strcmp(s1, s2) == 0; }
inline bool str_iequal(const char *s1, const char *s2) { return strcasecmp(s1, s2) == 0; }

inline int str0len(const char *str) {
    return str ? strlen(str) : 0;
}

inline char *strndup(const char *str, int len) {
    char *result = (char*)malloc(len+1);
    memcpy(result, str, len);
    result[len]  = 0;
    return result;
}

inline int count_spaces(const char *str) { return strspn(str, " "); }

inline bool occurs_in(char ch, const char *in) { return strchr(in, ch) != 0; }

inline bool is_end_mark(char ch) { return ch == '.' || ch == ';'; }

#define WORD_SEP ",.; ?:!)]}"

inline bool is_word_char(char ch) { return !occurs_in(ch, WORD_SEP); }

inline bool has_content(const char *field) { return field && (field[0] != 0 && field[1] != 0); }
inline char *no_content() {
    char *nothing = (char*)malloc(2);
    nothing[0]    = '\n';
    nothing[1]    = 0;
    return nothing;
}

inline void freedup_if_content(char*& entry, const char *content) {
    if (has_content(content)) freedup(entry, content);
}

// --------------------

template<typename PRED>
char *skipOverLinesThat(char *buffer, size_t buffersize, FILE_BUFFER& fb, PRED line_predicate) {
    // returns a pointer to the first non-matching line or NULL
    // @@@ WARNING: unconditionally skips the first line 
    char *result;

    for (result = Fgetline(buffer, buffersize, fb);
         result && line_predicate(result);
         result = Fgetline(buffer, buffersize, fb)) { }
    
    return result;
}
 
// --------------------

class WrapMode {
    char *separators;
public:
    WrapMode(const char *separators_) : separators(nulldup(separators_)) {}
    WrapMode(bool allowWrap) : separators(allowWrap ? strdup(WORD_SEP) : NULL) {} // true->wrap words, false->wrapping forbidden
    ~WrapMode() { free(separators); }

    bool allowed_to_wrap() const { return separators; }
    const char *get_seps() const { ca_assert(allowed_to_wrap()); return separators; }

    int wrap_pos(const char *str, int wrapCol) const;

};

#ifndef CONVERT_H
#include "convert.h"
#endif

// --------------------
// Logging

#if defined(DEBUG)
#define CALOG // be more verbose in debugging mode
#endif // DEBUG

#else
#error global.h included twice
#endif // GLOBAL_H




