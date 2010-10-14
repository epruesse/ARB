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

class Reader {
    FILE *fp;

    void read() { eof = Fgetline(linebuf, LINESIZE, fbuf); }

protected:

    FILE_BUFFER  fbuf;
    char         linebuf[LINESIZE];
    char        *eof; // @@@ rename

public:
    Reader(const char *inf);
    virtual ~Reader();

    const char *line() const { return eof; }
    Reader& operator++() { if (eof) read(); return *this; }

    template<typename PRED>
    void skipOverLinesThat(PRED match_condition) {
        while (match_condition(line()))
            ++(*this);
    }
};

struct DataReader {
    virtual ~DataReader() {}
    virtual SeqPtr read_sequence_data() = 0;
};

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

inline const char *shorttimekeep(char *heapcopy) {
    static SmartMallocPtr(char) keep;
    keep = heapcopy;
    return &*keep;
}
inline const char *shorttimecopy(const char *nocopy) { return shorttimekeep(nulldup(nocopy)); }


#else
#error reader.h included twice
#endif // READER_H

