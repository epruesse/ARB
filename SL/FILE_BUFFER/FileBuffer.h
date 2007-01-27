/////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2000
// Ralf Westram
// Time-stamp: <Thu Dec/14/2006 18:31 MET Coder@ReallySoft.de>
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

struct ClassFileBuffer;
typedef struct ClassFileBuffer *FILE_BUFFER;

#ifdef __cplusplus
extern "C" {
#endif

    FILE_BUFFER  create_FILE_BUFFER(const char *filename, FILE *in);
    void         destroy_FILE_BUFFER(FILE_BUFFER file_buffer);
    const char  *FILE_BUFFER_read(FILE_BUFFER file_buffer, size_t *lengthPtr);
    void         FILE_BUFFER_back(FILE_BUFFER file_buffer, const char *backline);
    void         FILE_BUFFER_rewind(FILE_BUFFER file_buffer);

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

#define fb_assert(cond) arb_assert(cond)

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

    bool getLine_intern(string& line);

public:
    FileBuffer(const string& filename_, FILE *in) {
        filename = filename_;
        fp       = in;

        fb_assert(fp);
        read = BUFFERSIZE;
        fillBuffer();

        next_line  = 0;
        lineNumber = 0;
    }
    ~FileBuffer() {
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
        fb_assert(next_line==0);
        next_line = new string(line);
        lineNumber--;
    }

    void rewind();

    const string& getFilename() const { return filename; }

    string lineError(const char *msg);
    string lineError(const string& msg) { return lineError(msg.c_str()); }
};
#endif // __cplusplus

#else
#error FileBuffer.h included twice
#endif // FILEBUFFER_H
