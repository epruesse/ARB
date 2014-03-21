#ifndef READER_H
#define READER_H

#ifndef BUFFEREDFILEREADER_H
#include <BufferedFileReader.h>
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


class Reader : virtual Noncopyable {
    FILE               *fp;
    BufferedFileReader *file;
    char                linebuf[LINESIZE];
    const char         *curr; // == NULL means "EOF reached"
    bool                failure;

    void reset() {
        curr    = linebuf;
        failure = false;
    }

    void read();

public:
    Reader(const char *inf);
    virtual ~Reader();

    void rewind() { file->rewind(); reset(); read(); }

    Reader& operator++() { read(); return *this; }

    const char *line() const { return curr; }

    void set_line(const char *new_line)  {
        ca_assert(new_line);
        ca_assert(strlen(new_line)<LINESIZE);
        strcpy(linebuf, new_line);
    }

    bool failed() const { return failure; }
    bool ok() const { return !failure; }

    void abort() { failure = true; curr = NULL; }
    void ignore_rest_of_file() { curr = NULL; }

    template<class PRED>
    void skipOverLinesThat(const PRED& match_condition) {
        while (line() && match_condition(line()))
            ++(*this);
    }
};

inline const char *shorttimekeep(char *heapcopy) { RETURN_LOCAL_ALLOC(heapcopy); }
inline const char *shorttimecopy(const char *nocopy) { return shorttimekeep(nulldup(nocopy)); }

struct FormatReader {
    virtual ~FormatReader() {}
    virtual bool read_one_entry(Seq& seq) __ATTR__USERESULT = 0;
    virtual bool failed() const = 0;
    virtual void ignore_rest_of_file() = 0;
    virtual void rewind() = 0;
    virtual InputFormat& get_data() = 0;

    static SmartPtr<FormatReader> create(const FormattedFile& in);
};

typedef SmartPtr<FormatReader> FormatReaderPtr;

struct SimpleFormatReader : public Reader, public FormatReader {
    SimpleFormatReader(const char *inf) : Reader(inf) {}
    bool failed() const OVERRIDE { return Reader::failed(); }
    void ignore_rest_of_file() OVERRIDE { Reader::ignore_rest_of_file(); }
    void rewind() OVERRIDE { Reader::rewind(); }
};

// --------------------------------------------------------------------------------

#if defined(ASSERTION_USED)
#define ENFORCE_CHECKED_WRITTEN
#endif

struct Writer {
    Writer() {}
    virtual ~Writer() {}

    virtual bool ok() const          = 0;
    virtual void out(char ch)        = 0;
    virtual const char *name() const = 0;

    virtual void throw_write_error() const __ATTR__NORETURN;
    virtual int out(const char *text);
    virtual int outf(const char *format, ...) __ATTR__FORMAT_MEMBER(1);

    void repeated(char ch, int repeat) { while (repeat--) out(ch); }
};

class FileWriter : public Writer, virtual Noncopyable {
    FILE *ofp;
    char *filename;
    int   written; // count written sequences

#if defined(ENFORCE_CHECKED_WRITTEN)
    bool checked_written;
#endif

    bool is_fine() const { return ofp && !Convaln_exception::exception_thrown(); }

public:
    FileWriter(const char *outf);
    ~FileWriter() OVERRIDE;

    FILE *get_FILE() { return ofp; }

    bool ok() const OVERRIDE { return ofp != NULL; }
    void out(char ch) OVERRIDE;
    const char *name() const OVERRIDE { return filename; }

    int out(const char *text) OVERRIDE { return Writer::out(text); }
    int outf(const char *format, ...) OVERRIDE __ATTR__FORMAT_MEMBER(1);

    void seq_done() { ++written; }
    void seq_done(int count) { ca_assert(count >= 0); written += count; }

    void expect_written();
};

#else
#error reader.h included twice
#endif // READER_H

