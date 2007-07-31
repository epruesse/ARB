// ================================================================ //
//                                                                  //
//   File      : FileBuffer.cxx                                     //
//   Purpose   :                                                    //
//   Time-stamp: <Tue Mar/20/2007 23:52 MET Coder@ReallySoft.de>    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in December 2006   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //
#include "FileBuffer.h"


using namespace std;

#if (__GNUC__ < 2 || (__GNUC__==2 && __GNUC_MINOR__<96)) // gcc 2.95 or below
template <typename T>
inline void swap(T& c1, T& c2) { T c = c1; c1 = c2; c2 = c; }
#endif

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

        char nextChar = buf[offset];
        if (is_EOL(nextChar) && nextChar != lf) offset++; // skip DOS linefeed
        if (offset == read) fillBuffer();
    }
    else { // reached end of buffer
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

#if defined(DEBUG)
    int printed =
#endif // DEBUG
        sprintf(buffer, "while reading %s (line #%li):\n%s", filename.c_str(), lineNumber, msg);
    fb_assert((size_t)printed < allocated);

    // return GBS_global_string("while reading %s (line #%li):\n%s", filename.c_str(), lineNumber, msg);
    return buffer;
}

void FileBuffer::rewind() {
    std::rewind(fp);
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

extern "C" FILE_BUFFER create_FILE_BUFFER(const char *filename, FILE *in) {
    FileBuffer *fb = new FileBuffer(filename, in);
    return reinterpret_cast<FILE_BUFFER>(fb);
}

extern "C" void destroy_FILE_BUFFER(FILE_BUFFER file_buffer) {
    delete to_FileBuffer(file_buffer);
}

extern "C" const char *FILE_BUFFER_read(FILE_BUFFER file_buffer, size_t *lengthPtr) {
    static string  line;

    if (to_FileBuffer(file_buffer)->getLine(line)) {
        if (lengthPtr) *lengthPtr = line.length();
        return line.c_str();
    }
    return 0;
}

extern "C" void FILE_BUFFER_back(FILE_BUFFER file_buffer, const char *backline) {
    static string line;
    line = backline;
    to_FileBuffer(file_buffer)->backLine(line);
}

extern "C" void FILE_BUFFER_rewind(FILE_BUFFER file_buffer) {
    to_FileBuffer(file_buffer)->rewind();
}
