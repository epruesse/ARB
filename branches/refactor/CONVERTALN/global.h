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

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#ifndef ARB_DEFS_H
#include <arb_defs.h>
#endif


#define ca_assert(cond) arb_assert(cond)

#if defined(DEVEL_RALF)
#define UNCOVERED() ca_assert(0)
#else
#define UNCOVERED()
#endif

// --------------------

class Convaln_exception {
    static Convaln_exception *thrown;

    int           code;
    mutable char *msg;

public:
    Convaln_exception(int error_code, const char *error_msg)
        : code(error_code),
          msg(strdup(error_msg))
    {
        ca_assert(!thrown); // 2 exceptions at the same time ? very exceptional! :)
        thrown = this;
    }
    ~Convaln_exception() {
        ca_assert(thrown);
        thrown = NULL;
        free(msg);
    }

    int get_code() const { return code; }
    const char *get_msg() const { return msg; }
    void replace_msg(const char *new_msg) const { freedup(msg, new_msg); }

    static const Convaln_exception *exception_thrown() { return thrown; }
};

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

inline bool is_sequence_terminator(const char *str) { return str[0] == '/' && str[1] == '/'; }

#define WORD_SEP ",.; ?:!)]}"

inline bool is_gapchar(char ch) { return occurs_in(ch, "-.~"); }
inline bool is_word_char(char ch) { return !occurs_in(ch, WORD_SEP); }

inline bool has_no_content(const char *field) {
    return !field ||
        !field[0] ||
        (field[0] == '\n' && !field[1]);
}
inline bool has_content(const char *field) { return !has_no_content(field); }

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

#define INPLACE_RECONSTRUCT(type,this)          \
    do {                                        \
        (this)->~type();                        \
        new(this) type();                       \
    } while(0)

#define INPLACE_COPY_RECONSTRUCT(type,this,other)       \
    do {                                                \
        (this)->~type();                                \
        new(this) type(other);                          \
    } while(0)

#define DECLARE_ASSIGNMENT_OPERATOR(T)                  \
    T& operator = (const T& other) {                    \
        INPLACE_COPY_RECONSTRUCT(T, this, other);       \
        return *this;                                   \
    }

// --------------------

#define lookup_keyword(keyword,table) ___lookup_keyword(keyword, table, ARRAY_ELEMS(table))

inline bool is_input_format(Format inType) { return inType <= LAST_INPUT_FORMAT; }

#else
#error global.h included twice
#endif // GLOBAL_H

