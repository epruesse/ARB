// ============================================================= //
//                                                               //
//   File      : FileContent.h                                   //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in April 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef FILECONTENT_H
#define FILECONTENT_H

#ifndef ARB_STRARRAY_H
#include "arb_strarray.h"
#endif

class FileBuffer;

class FileContent : virtual Noncopyable {
    // provides simple load/modify/save for textfiles
    // (accepts any LF-variant)
    
    char     *path;
    GB_ERROR  error;
    StrArray  Lines;

    void init();

public:
    FileContent(const char *path_) : path(ARB_strdup(path_)), error(NULL) { init(); }
    ~FileContent() { free(path); }

    GB_ERROR has_error() const { return error; }

    StrArray& lines() { // intended to be modified
        arb_assert(!has_error());
        return Lines;
    }

    GB_ERROR save();
    GB_ERROR saveAs(const char *path_) { freedup(path, path_); return save(); }
};


#else
#error FileContent.h included twice
#endif // FILECONTENT_H

