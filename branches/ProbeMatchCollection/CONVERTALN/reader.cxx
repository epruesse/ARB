#include "reader.h"
#include <unistd.h>

Reader::Reader(const char *inf)
    : fp(fopen(inf, "rt")),
      file(NULL), 
      curr(NULL),
      failure(false)
{
    reset();
    try {
        if (!fp) {
            throw_errorf(1, "can't read input file '%s' (Reason: %s)", inf, strerror(errno));
        }
        file = new BufferedFileReader(inf, fp);
        file->showFilenameInLineError(true);
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
        exc->replace_msg(file->lineError(exc->get_msg()).c_str());
    }
    else {
        ca_assert(!curr); // reader did NOT read till EOF, why ?
    }
    delete file;
}

void Reader::read() {
    if (!curr) {
        failure = true; // read _beyond_ EOF
    }
    else {
        ca_assert(!failure); // attempt to read after failure

        string next_line;
        if (!file->getLine(next_line)) {
            curr = NULL;
        }
        else {
            size_t len = next_line.length();

            if (len >= (LINESIZE-1)) {
                char lbuf[200];
                memcpy(lbuf, next_line.c_str(), 200-4);
                strcpy(lbuf+200-4, "...");
                throw_errorf(148, "Line too long: '%s'", lbuf);
            }
            memcpy(linebuf, next_line.c_str(), len);
            linebuf[len]     = '\n';
            linebuf[len + 1] = 0;

            curr = linebuf;
        }
    }
}


// --------------------------------------------------------------------------------

FileWriter::FileWriter(const char *outname)
    : ofp(NULL),
      filename(NULL),
      written(0)
#if defined(ENFORCE_CHECKED_WRITTEN)
    , checked_written(false)
#endif
{
    ofp = fopen(outname, "wt");
    if (!ofp) {
        throw_errorf(2, "can't write output file '%s' (Reason: %s)", outname, strerror(errno));
    }
    filename = strdup(outname);
}
FileWriter::~FileWriter() {
    bool fine = is_fine();

#if defined(ENFORCE_CHECKED_WRITTEN)
    ca_assert(implicated(fine, checked_written)); // you have to call expect_written() on FileWriter before destruction
#endif

    if (ofp) fclose(ofp);
    if (!fine) unlink(filename);
    free(filename);

    log_processed(written);
}

void FileWriter::expect_written() {
#if defined(ENFORCE_CHECKED_WRITTEN)
    ca_assert(!checked_written); // checking twice is nonsense
    checked_written = true;
#endif
    if (is_fine() && !written) {
        throw_errorf(42, "No sequence has been written");
    }
}

void Writer::throw_write_error() const {
    throw_errorf(41, "Write error: %s(=%i) while writing %s",
                 strerror(errno), errno, name());
}
void FileWriter::out(char ch) {
    if (fputc(ch, ofp) == EOF) throw_write_error();
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

int Writer::out(const char *text) {
    int i = 0;
    while (text[i]) {
        out(text[i++]);
    }
    return i;
}

