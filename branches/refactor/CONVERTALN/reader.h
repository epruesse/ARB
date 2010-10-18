#ifndef READER_H
#define READER_H

#ifndef FILEBUFFER_H
#include <FileBuffer.h>
#endif
#ifndef DEFS_H
#include "defs.h"
#endif
#ifndef FUN_H
#include "fun.h"
#endif
#ifndef SEQ_H
#include "seq.h"
#endif
#ifndef INPUT_FORMAT_H
#include "input_format.h"
#endif


class Reader : Noncopyable {
    FILE *fp;
    char *curr;    // == NULL means "EOF reached"
    bool  failure;

    void reset() {
        curr    = linebuf;
        failure = false;
    }

    void set_curr(char *to) {
        curr = to; // @@@ use as breakpoint
    }

    void read() {
        if (!curr) {
            failure = true; // read _beyond_ EOF
        }
        else {
            ca_assert(!failure); // attempt to read after failure
            char *next_line = Fgetline(linebuf, LINESIZE, fbuf);
            set_curr(next_line);
        }
    }


protected:

    FILE_BUFFER fbuf;
    char        linebuf[LINESIZE];

public:
    Reader(const char *inf);
    virtual ~Reader();

#if 1
#define FBUF_DEPR __ATTR__DEPRECATED    
#else
#define FBUF_DEPR     
#endif
    FILE_BUFFER getfilebuf() FBUF_DEPR { return fbuf; }
    char *getlinebuf() FBUF_DEPR { return linebuf; }
    void setcurr(char *curr_) FBUF_DEPR { set_curr(curr_); }

    void rewind() { FILE_BUFFER_rewind(fbuf); reset(); read(); }

    Reader& operator++() { read(); return *this; }

    const char *line() const { return curr; }
    char *line_WRITABLE() { return curr; }

    bool failed() const { return failure; }
    bool ok() const { return !failure; }

    void abort() { failure = true; set_curr(NULL); }

    template<class PRED>
    void skipOverLinesThat(const PRED& match_condition) {
        while (line() && match_condition(line()))
            ++(*this);
    }
};

template<typename PRED>
char *skipOverLinesThat_DEPRECATED(char *buffer, size_t buffersize, FILE_BUFFER& fb, PRED line_predicate) {
    // returns a pointer to the first non-matching line or NULL
    // @@@ WARNING: unconditionally skips the first line
    char *result;

    for (result = Fgetline(buffer, buffersize, fb);
         result && line_predicate(result);
         result = Fgetline(buffer, buffersize, fb)) { }

    return result;
}

inline const char *shorttimekeep(char *heapcopy) {
    static SmartMallocPtr(char) keep;
    keep = heapcopy;
    return &*keep;
}
inline const char *shorttimecopy(const char *nocopy) { return shorttimekeep(nulldup(nocopy)); }

struct InputFormatReader {
    virtual ~InputFormatReader() {}

    virtual bool read_seq_data(Seq& seq) = 0;
    virtual bool read_one_entry(InputFormat& data, Seq& seq) = 0;
};

// --------------------------------------------------------------------------------

class Writer : Noncopyable {
    FILE *ofp;
    char *filename;
    int   written; // count written sequences

public:
    Writer(const char *outf);
    ~Writer();

    FILE *get_FILE() { return ofp; }

    bool ok() const { return ofp != NULL; }
    void throw_write_error();
    void seq_done() { ++written; }
    void seq_done(int count) { ca_assert(count >= 0); written += count; }

    void out(const char *text);
    void out(char ch);
    void outf(const char *format, ...) __ATTR__FORMAT(2);
};

#else
#error reader.h included twice
#endif // READER_H

