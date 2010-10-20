#include "reader.h"

Reader::Reader(const char *inf)
    : fp(NULL),
      curr(NULL),
      failure(false),
      fbuf(NULL)
{
    reset();
    try {
        fp   = open_input_or_die(inf);
        fbuf = create_FILE_BUFFER(inf, fp);
        read();
    }
    catch (Convaln_exception& exc) {
        failure = true;
        curr    = NULL;
        throw;
    }
}

Reader::~Reader() {
    // if kicked by exception, decorate error-msg with current reader position
    if (const Convaln_exception *exc = Convaln_exception::exception_thrown()) {
        if (fbuf) {
            const char *new_msg = FILE_BUFFER_make_error(fbuf, true, exc->get_msg());
            exc->replace_msg(new_msg);
        }
    }
    if (fbuf) destroy_FILE_BUFFER(fbuf);
}

// --------------------------------------------------------------------------------

FileWriter::FileWriter(const char *outname)
    : ofp(NULL),
      filename(NULL),
      written(0)
{
    ofp      = open_output_or_die(outname);
    filename = strdup(outname);
}
FileWriter::~FileWriter() {
    bool fine      = ofp && !Convaln_exception::exception_thrown();
    bool die_empty = fine && !written;

    if (ofp) fclose(ofp);
    if (!fine || die_empty) unlink(filename);
    free(filename);

    if (die_empty) throw_errorf(42, "No sequence has been written");

    log_processed(written);
}

void Writer::throw_write_error() const {
    throw_errorf(41, "Write error: %s(=%i) while writing %s",
                 strerror(errno), errno, name());
}
void FileWriter::out(char ch) {
    if (fputc(ch, ofp) == EOF) throw_write_error();
}
void FileWriter::out(const char *text) {
    if (fputs(text, ofp) == EOF) throw_write_error();
}

int FileWriter::outf(const char *format, ...) {
    va_list parg;
    va_start(parg, format);
    int     printed = vfprintf(ofp, format, parg);
    va_end(parg);
    if (printed<0) throw_write_error();
    return printed;
}

int Writer::outf(const char *format, ...) {
    va_list parg;
    va_start(parg, format);
    char    buffer[LINESIZE];
    int     printed = vsprintf(buffer, format, parg);
    ca_assert(printed <= LINESIZE);
    va_end(parg);

    out(buffer);
    return printed;
}

void Writer::out(const char *text) {
    int i = 0;
    while (text[i]) {
        out(text[i++]);
    }
}



