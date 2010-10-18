#include "reader.h"

Reader::Reader(const char *inf) {
    reset();
    try {
        fp   = open_input_or_die(inf);
        fbuf = create_FILE_BUFFER(inf, fp);
        read();
    }
    catch (Convaln_exception& ex) {
        failure = true;
        curr    = NULL;
        ca_assert(0); // what to do with exception ? 
    }
}

Reader::~Reader() {
    // if kicked by exception, decorate error-msg with current reader position
    if (const Convaln_exception *exc = Convaln_exception::exception_thrown()) {
        exc->replace_msg(FILE_BUFFER_make_error(fbuf, true, exc->get_msg()));
    }
    destroy_FILE_BUFFER(fbuf);
}



