#include "reader.h"

Reader::Reader(const char *inf) {
    try {
        fp   = open_input_or_die(inf);
        fbuf = create_FILE_BUFFER(inf, fp);
        read();
    }
    catch (Convaln_exception& ex) {
        eof = NULL;
    }
}

Reader::~Reader() {
    destroy_FILE_BUFFER(fbuf);
}


