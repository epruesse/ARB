/////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2000
// Ralf Westram
// Time-stamp: <Wed Dec/13/2006 19:05 MET Coder@ReallySoft.de>
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  Ralf Westram makes no
// representations about the suitability of this software for any
// purpose.  It is provided "as is" without express or implied warranty.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef FILEBUFFER_H
#define FILEBUFFER_H

#ifndef _STDIO_H
#include <stdio.h>
#endif

// --------------------------------------------------------------------------------
// c-interface

typedef void *FILE_BUFFER;

#ifdef __cplusplus
extern "C" {
#endif

    FILE_BUFFER  create_FILE_BUFFER(const char *filename, FILE *in);
    void         destroy_FILE_BUFFER(FILE_BUFFER file_buffer);
    const char  *FILE_BUFFER_read(FILE_BUFFER file_buffer);
    void         FILE_BUFFER_back(FILE_BUFFER file_buffer, const char *backline);

#ifdef __cplusplus
}
#endif

// --------------------------------------------------------------------------------
// c++-interface
#ifdef __cplusplus

#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef _CPP_STRING
#include <string>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

#define gi_assert(cond) arb_assert(cond)

using std::string;

const size_t BUFFERSIZE = 64*1024;

class FileBuffer : public Noncopyable {
private:
    char buf[BUFFERSIZE];
    size_t read; // chars in buf
    size_t offset; // offset to next line

    FILE *fp;

    string *next_line;

    long   lineNumber;          // current line number
    string filename;

    void fillBuffer();

    static inline bool is_EOL(char c) { return c == '\n' || c == '\r'; }

    bool getLine_intern(string& line);

public:
    FileBuffer(const string& filename_, FILE *in) {
        filename = filename_;
        fp       = in;

        arb_assert(fp);
        read = BUFFERSIZE;
        fillBuffer();

        next_line  = 0;
        lineNumber = 0;
    }
    virtual ~FileBuffer() {
        delete next_line;
        if (fp) fclose(fp);
    }

    bool good() { return fp!=0; }

    bool getLine(string& line) {
        lineNumber++;
        if (next_line) {
            line = *next_line;
            delete next_line;
            next_line = 0;
            return true;
        }
        return getLine_intern(line);
    }

    long getLineNumber() const { return lineNumber; }

    void backLine(const string& line) { // push line back
        gi_assert(next_line==0);
        next_line = new string(line);
        lineNumber--;
    }

    const string& getFilename() const { return filename; }

    string lineError(const char *msg);
    string lineError(const string& msg) { return lineError(msg.c_str()); }
};
#endif // __cplusplus

#else
#error FileBuffer.h included twice
#endif // FILEBUFFER_H
