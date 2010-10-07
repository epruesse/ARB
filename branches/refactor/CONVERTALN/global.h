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

#ifndef PROTOTYPES_H
#include "prototypes.h"
#endif

#define ca_assert(cond) arb_assert(cond)

#define LINESIZE       126
#define LONGTEXT       5000
#define TOKENSIZE      80
#define GBMAXCHAR      66 /* exclude the keyword(12 chars) */
#define EMBLMAXCHAR    68 /* exclude the keyword(6 chars) */
#define MACKEMAXCHAR   78 /* limit for each line */
#define COMMSKINDENT   2 /* indent for subkey line of RDP defined comment */
#define COMMCNINDENT   6 /* indent for continue line of RDP defined comment */
#define p_nonkey_start 5

// --------------------

class global_data {
    // - holds sequence data
    // - holds additional data for various formats

    bool initialized;

public:
    global_data() {
        initialized   = false;
    }

    void cleanup();
    void setup();

    int       numofseq; /* number of sequences */
    int       seq_length; /* sequence length */
    int       max;
    char     *sequence; /* sequence data */
    /* to read all the sequences into memory at one time (yes great idea!) */
    char    **ids; /* array of ids. */
    char    **seqs; /* array of sequence data */
    int      *lengths; /* array of sequence lengths */
    int       allocated; /* for how many sequences space has been allocated */
    /* NEXUS, PHYLIP, GCG, and PRINTABLE */
    GenBank   gbk; /* one GenBank entry */
    Macke     macke; /* one Macke entry */
    Paup      paup; /* one Paup entry */
    Embl      embl;
    Nbrf      nbrf;

#if (UNIT_TESTS==1)
    int test_counter;
#endif
};

extern struct global_data data;

inline void NOOP_global_data_was_previously_initialized_here() {
    // @@@ left this here cause it's called inside a loop from
    // to_paup_1x1(), to_phylip_1x1() and to_printable_1x1().
    //
    // these functions are just hackarounds. they try to avoid running
    // out of memory, which is caused by holding all sequences in memory.
    // Instead they should be written during read!

    ca_assert(0); // yeah - i found a test-case for this!
} 


// --------------------

inline bool str_equal(const char *s1, const char *s2) { return strcmp(s1, s2) == 0; }
inline bool str_iequal(const char *s1, const char *s2) { return strcasecmp(s1, s2) == 0; }

inline int str0len(const char *str) {
    data.test_counter++;
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

// --------------------
// Logging

#if defined(DEBUG)
#define CALOG // be more verbose in debugging mode
#endif // DEBUG

#else
#error global.h included twice
#endif // GLOBAL_H




