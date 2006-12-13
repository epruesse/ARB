// ================================================================ //
//                                                                  //
//   File      : FileBuffer.cxx                                     //
//   Purpose   :                                                    //
//   Time-stamp: <Wed Dec/13/2006 19:04 MET Coder@ReallySoft.de>    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in December 2006   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //
#include "FileBuffer.h"


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

bool FileBuffer::getLine_intern(string& line)
{
    if (offset==read) return false;

    size_t lineEnd = offset;;
    for (; lineEnd<read && !is_EOL(buf[lineEnd]); ++lineEnd) ;

    if (lineEnd<read) { // found end of line char
        line   = string(buf+offset, lineEnd-offset);
        offset = lineEnd+1;
        while (offset<read && is_EOL(buf[offset])) ++offset;
        if (offset == read) {
            fillBuffer();
            while (offset<read && is_EOL(buf[offset])) ++offset;
        }
    }
    else {
        line = string(buf+offset, read-offset);
        fillBuffer();
        string rest;
        if (getLine_intern(rest)) line = line+rest;
    }

    return true;
}

string FileBuffer::lineError(const char *msg) {
    size_t         len       = strlen(msg)+filename.length()+100;
    static char   *buffer;
    static size_t  allocated = 0;

    if (len>allocated) {
        allocated = len;
        free(buffer);
        buffer    = (char*)malloc(allocated);
    }

    int printed = sprintf(buffer, "while reading %s (line #%li):\n%s", filename.c_str(), lineNumber, msg);
    gi_assert((size_t)printed < allocated);

    // return GBS_global_string("while reading %s (line #%li):\n%s", filename.c_str(), lineNumber, msg);
    return buffer;
}

// --------------------------------------------------------------------------------
// C interface

extern "C" FILE_BUFFER create_FILE_BUFFER(const char *filename, FILE *in) {
    FileBuffer *fb = new FileBuffer(filename, in);
    return reinterpret_cast<FILE_BUFFER>(fb);
}

extern "C" void destroy_FILE_BUFFER(FILE_BUFFER file_buffer) {
    FileBuffer *fb = reinterpret_cast<FileBuffer*>(file_buffer);
    delete fb;
}

extern "C" const char *FILE_BUFFER_read(FILE_BUFFER file_buffer) {
    FileBuffer    *fb = reinterpret_cast<FileBuffer*>(file_buffer);
    static string  line;

    if (fb->getLine(line)) {
        return line.c_str();
    }
    return 0;
}

extern "C" void FILE_BUFFER_back(FILE_BUFFER file_buffer, const char *backline) {
    FileBuffer *fb = reinterpret_cast<FileBuffer*>(file_buffer);

    static string line;
    line = backline;
    fb->backLine(line);
}
