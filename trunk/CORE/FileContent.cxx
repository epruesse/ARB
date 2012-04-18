// ============================================================= //
//                                                               //
//   File      : FileContent.cxx                                 //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in April 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "FileContent.h"
#include "FileBuffer.h"
#include "arb_msg.h"
#include "arb_string.h"

using namespace std;

void FileContent::init() {
    FILE *fp = fopen(path, "rb"); // b!
    if (!fp) {
        error = GB_IO_error("loading", path);
    }
    else {
        FileBuffer buf(path, fp);
        string     line;
        
        while (buf.getLine(line)) {
            Lines.put(GB_strndup(line.c_str(), line.length()));
        }
    }
}

GB_ERROR FileContent::save() {
    arb_assert(!has_error());

    FILE *out        = fopen(path, "wt");
    bool  failed     = false;
    if (!out) failed = true;
    else {
        for (size_t i = 0; i<Lines.size(); ++i) {
            fputs(Lines[i], out);
            fputc('\n', out);
        }
        failed = (fclose(out) != 0) || failed;
    }
    if (failed) error = GB_IO_error("saving", path);
    return error;
}

