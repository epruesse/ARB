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

inline const char *shorttimekeep(char *heapcopy) {
    static SmartCharPtr keep;
    keep = heapcopy;
    return &*keep;
}
inline const char *shorttimecopy(const char *nocopy) { return shorttimekeep(nulldup(nocopy)); }

struct InputFormatReader {
    virtual ~InputFormatReader() {}
    virtual bool read_one_entry(InputFormat& data, Seq& seq) = 0;
};

// --------------------------------------------------------------------------------

class Writer {
public:
    Writer() {}
    virtual ~Writer() {}

    virtual bool ok() const            = 0;
    virtual void out(char ch)          = 0;
    virtual const char *name() const   = 0;

    virtual void throw_write_error() const;
    virtual void out(const char *text);
    virtual int outf(const char *format, ...) __ATTR__FORMAT(2);

    void repeated(char ch, int repeat) { while (repeat--) out(ch); }
};

class FileWriter : public Writer {
    FILE *ofp;
    char *filename;
    int   written; // count written sequences

public:
    FileWriter(const char *outf);
    ~FileWriter();

    FILE *get_FILE() { return ofp; }

    bool ok() const { return ofp != NULL; }
    void out(char ch);
    void out(const char *text);
    const char *name() const { return filename; }
    
    int outf(const char *format, ...) __ATTR__FORMAT(2);

    void seq_done() { ++written; }
    void seq_done(int count) { ca_assert(count >= 0); written += count; }
};

#else
#error reader.h included twice
#endif // READER_H

