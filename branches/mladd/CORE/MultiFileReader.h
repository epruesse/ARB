// ============================================================= //
//                                                               //
//   File      : MultiFileReader.h                               //
//   Purpose   : Read multiple files like one concatenated file  //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in March 2014   //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef MULTIFILEREADER_H
#define MULTIFILEREADER_H

#ifndef BUFFEREDFILEREADER_H
#include <BufferedFileReader.h>
#endif
#ifndef ARB_STRARRAY_H
#include <arb_strarray.h>
#endif

class MultiFileReader : public LineReader { // derived from Noncopyable
    const CharPtrArray&  files;
    BufferedFileReader  *reader;
    BufferedFileReader  *last_reader;

    string *error;
    size_t  at;    // idx into 'files'

    bool getLine_intern(string& line) OVERRIDE;

    void nextReader();
    FILE *open(int i);

public:
    MultiFileReader(const CharPtrArray& files_);
    ~MultiFileReader();

    GB_ERROR get_error() const { return error ? error->c_str() : NULL; }
    const string& getFilename() const OVERRIDE; // @@@ rename (not necessarily a file)
};

#else
#error MultiFileReader.h included twice
#endif // MULTIFILEREADER_H
