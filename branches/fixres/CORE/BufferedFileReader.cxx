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

#include "BufferedFileReader.h"
#include "arb_mem.h"

#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <smartptr.h>

using namespace std;

void BufferedFileReader::fillBuffer()
{
    if (read==BUFFERSIZE) {
        read = fread(buf, sizeof(buf[0]), BUFFERSIZE, fp);
        offset = 0;
    }
    else {
        offset = read;
    }
}

static char eol[3] = "\n\r";
static inline bool is_EOL(char c) { return c == eol[0] || c == eol[1]; }

bool BufferedFileReader::getLine_intern(string& line)
{
    if (offset==read) return false;

    size_t lineEnd = 0;
    {
        size_t  rest   = read-offset;
        char   *eolPos = (char*)memchr(buf+offset, eol[0], rest);

        if (!eolPos) {
            eolPos = (char*)memchr(buf+offset, eol[1], rest);
            if (!eolPos) {
                lineEnd = read;
            }
            else {
                swap(eol[0], eol[1]);
                lineEnd = eolPos-buf;
            }
        }
        else {
            lineEnd = eolPos-buf;
            if (lineEnd>0 && lineEnd>offset && buf[lineEnd-1] == eol[1]) {
                swap(eol[0], eol[1]);
                lineEnd--;
            }
        }
    }

    fb_assert(lineEnd >= offset);

    if (lineEnd<read) { // found end of line char
        line    = string(buf+offset, lineEnd-offset);
        char lf = buf[lineEnd];

        offset = lineEnd+1;
        if (offset == read) fillBuffer();

        if (offset<read) { // otherwise EOF!
            char nextChar = buf[offset];
            if (is_EOL(nextChar) && nextChar != lf) offset++; // skip DOS linefeed
            if (offset == read) fillBuffer();
        }
    }
    else { // reached end of buffer
        line = string(buf+offset, read-offset);
        fillBuffer();
        string rest;
        if (getLine_intern(rest)) line = line+rest;
    }

    return true;
}

string LineReader::lineError(const string& msg) const {
    static SmartCharPtr buffer;
    static size_t       allocated = 0;

    const string& source = getFilename();

    size_t len;
    if (showFilename) {
        len = msg.length()+source.length()+100;
    }
    else {
        len = msg.length()+100;
    }

    if (len>allocated) {
        allocated = len;
        buffer    = (char*)ARB_alloc(allocated);
    }

    if (showFilename) {
#if defined(ASSERTION_USED)
        int printed =
#endif // ASSERTION_USED
            sprintf(&*buffer, "%s:%zu: %s", source.c_str(), lineNumber, msg.c_str());
        fb_assert((size_t)printed < allocated);
    }
    else {
#if defined(ASSERTION_USED)
        int printed =
#endif // ASSERTION_USED
            sprintf(&*buffer, "while reading line #%zu:\n%s", lineNumber, msg.c_str());
        fb_assert((size_t)printed < allocated);
    }

    return &*buffer;
}

void BufferedFileReader::rewind() {
    errno = 0;
    std::rewind(fp);
    fb_assert(errno == 0); // not handled yet

    read = BUFFERSIZE;
    fillBuffer();
    reset();
}

