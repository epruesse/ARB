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

#ifndef BUFFEREDFILEREADER_H
#define BUFFEREDFILEREADER_H

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

class LineReader : virtual Noncopyable {
    /*! may represent any source that can be read line by line
     */

    size_t  lineNumber; // current line number
    string *next_line;
    bool   showFilename; // @@@ rename (not necessarily a file)

    virtual bool getLine_intern(string& line) = 0;

protected:
    void reset() {
        if (next_line) {
            delete next_line;
            next_line = NULL;
        }
        lineNumber = 0;
    }

public:
    LineReader()
        : lineNumber(0),
          next_line(NULL),
          showFilename(true)
    {}
    virtual ~LineReader() {
        delete next_line;
    }

    string lineError(const string& msg) const;
    string lineError(const char *msg) const { return lineError(string(msg)); }

    void showFilenameInLineError(bool show) { showFilename = show; } // @@@ rename (not necessarily a file)

    virtual bool getLine(string& line) {
        lineNumber++;
        if (next_line) {
            line = *next_line;
            delete next_line;
            next_line = NULL;
            return true;
        }
        return getLine_intern(line);
    }

    void backLine(const string& line) { // push line back
        fb_assert(next_line==0);
        next_line = new string(line);
        lineNumber--;
    }

    size_t getLineNumber() const { return lineNumber; }
    void setLineNumber(size_t line) { lineNumber = line; }

    virtual const string& getFilename() const = 0; // @@@ rename (not necessarily a file)

    void copyTo(FILE *out) {
        string line;
        while (getLine(line)) {
            fputs(line.c_str(), out);
            fputc('\n', out);
        }
    }
};

const size_t BUFFERSIZE = 64*1024;

class BufferedFileReader : public LineReader { // derived from Noncopyable
    char buf[BUFFERSIZE];
    size_t read; // chars in buf
    size_t offset; // offset to next line

    FILE *fp;

    string filename;

    void fillBuffer();

    bool getLine_intern(string& line) OVERRIDE;

public:
    BufferedFileReader(const string& filename_, FILE *in) {
        filename = filename_;
        fp       = in;

        fb_assert(fp);
        read = BUFFERSIZE;
        fillBuffer();
    }
    virtual ~BufferedFileReader() {
        if (fp) fclose(fp);
    }

    bool good() { return fp!=0; }
    void rewind();

    const string& getFilename() const OVERRIDE { return filename; }

};

#else
#error BufferedFileReader.h included twice
#endif // BUFFEREDFILEREADER_H
