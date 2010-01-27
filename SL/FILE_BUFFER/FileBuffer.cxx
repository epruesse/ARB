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

#include "FileBuffer.h"
#include <cstdlib>
#include <cstring>
#include <cerrno>

using namespace std;

void FileBuffer::fillBuffer()
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

bool FileBuffer::getLine_intern(string& line)
{
    if (offset==read) return false;

    size_t lineEnd;
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
            if (lineEnd>0 && buf[lineEnd-1] == eol[1]) {
                swap(eol[0], eol[1]);
                lineEnd--;
            }
        }
    }

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

string FileBuffer::lineError(const string& msg) const {
    static char   *buffer;
    static size_t  allocated = 0;

    size_t len;
    if (showFilename) {
        len = msg.length()+filename.length()+100;
    }
    else {
        len = msg.length()+100;
    }

    if (len>allocated) {
        allocated = len;
        free(buffer);
        buffer    = (char*)malloc(allocated);
    }

    if (showFilename) {
#if defined(DEBUG)
        int printed =
#endif // DEBUG
            sprintf(buffer, "%s:%zu: %s", filename.c_str(), lineNumber, msg.c_str());
        fb_assert((size_t)printed < allocated);
    }
    else {
#if defined(DEBUG)
        int printed =
#endif // DEBUG
            sprintf(buffer, "while reading line #%zu:\n%s", lineNumber, msg.c_str());
        fb_assert((size_t)printed < allocated);
    }
    
    return buffer;
}

void FileBuffer::rewind() {
    errno = 0;
    std::rewind(fp);
    fb_assert(errno == 0); // not handled yet
    
    read = BUFFERSIZE;
    fillBuffer();

    if (next_line) {
        delete next_line;
        next_line = 0;
    }
    lineNumber = 0;
}

// --------------------------------------------------------------------------------
// C interface

inline FileBuffer *to_FileBuffer(FILE_BUFFER fb) {
    FileBuffer *fileBuffer = reinterpret_cast<FileBuffer*>(fb);
    fb_assert(fileBuffer);
    fb_assert(fileBuffer->good());
    return fileBuffer;
}

FILE_BUFFER create_FILE_BUFFER(const char *filename, FILE *in) {
    FileBuffer *fb = new FileBuffer(filename, in);
    return reinterpret_cast<FILE_BUFFER>(fb);
}

void destroy_FILE_BUFFER(FILE_BUFFER file_buffer) {
    delete to_FileBuffer(file_buffer);
}

const char *FILE_BUFFER_read(FILE_BUFFER file_buffer, size_t *lengthPtr) {
    static string  line;

    if (to_FileBuffer(file_buffer)->getLine(line)) {
        if (lengthPtr) *lengthPtr = line.length();
        return line.c_str();
    }
    return 0;
}

void FILE_BUFFER_back(FILE_BUFFER file_buffer, const char *backline) {
    static string line;
    line = backline;
    to_FileBuffer(file_buffer)->backLine(line);
}

void FILE_BUFFER_rewind(FILE_BUFFER file_buffer) {
    to_FileBuffer(file_buffer)->rewind();
}
