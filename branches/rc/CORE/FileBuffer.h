// --------------------------------------------------------------------------------
// Copyright (C) 2000
// Ralf Westram
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  Ralf Westram makes no
// representations about the suitability of this software for any
// purpose.  It is provided "as is" without express or implied warranty.
// --------------------------------------------------------------------------------


#ifndef FILEBUFFER_H
#define FILEBUFFER_H

#ifndef _GLIBCXX_CSTDIO
#include <cstdio>
#endif
#ifndef _GLIBCXX_STRING
#include <string>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

#define fb_assert(cond) arb_assert(cond)

using std::string;

const size_t BUFFERSIZE = 64*1024;

class FileBuffer : virtual Noncopyable {
private:
    char buf[BUFFERSIZE];
    size_t read; // chars in buf
    size_t offset; // offset to next line

    FILE *fp;

    string *next_line;

    size_t lineNumber;                              // current line number
    string filename;
    bool   showFilename;

    void fillBuffer();

    bool getLine_intern(string& line);

public:
    FileBuffer(const string& filename_, FILE *in) {
        filename = filename_;
        fp       = in;

        showFilename = true;

        fb_assert(fp);
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

    virtual bool getLine(string& line) {
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

    void showFilenameInLineError(bool show) { showFilename = show; }
    string lineError(const string& msg) const;
    string lineError(const char *msg) const { return lineError(string(msg)); }

    size_t getLineNumber() { return lineNumber; }
    void setLineNumber(size_t line) { lineNumber = line; }

    void copyTo(FILE *out) {
        string line;
        while (getLine(line)) {
            fputs(line.c_str(), out);
            fputc('\n', out);
        }
    }
};

#else
#error FileBuffer.h included twice
#endif // FILEBUFFER_H
