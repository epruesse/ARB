// ================================================================= //
//                                                                   //
//   File      : types.h                                             //
//   Purpose   :                                                     //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2010   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef TYPES_H
#define TYPES_H

struct Convaln_exception {
    int         error_code;
    const char *error; // @@@ make this a copy

    Convaln_exception(int error_code_, const char *error_)
        : error_code(error_code_),
          error(error_)
    {}
};

// ----------------
//      Readers


class Reader {
    FILE *fp;

    void read() { eof = Fgetline(linebuf, LINESIZE, fbuf); }

protected:

    FILE_BUFFER  fbuf;
    char  linebuf[LINESIZE];
    char *eof;

public:
    Reader(const char *inf) {
        try {
            fp   = open_input_or_die(inf);
            fbuf = create_FILE_BUFFER(inf, fp);
            read();
        }
        catch (Convaln_exception& ex) {
            eof = NULL;
        }
    }
    virtual ~Reader() {
        destroy_FILE_BUFFER(fbuf);
    }

    const char *line() const { return eof; }
    Reader& operator++() { if (eof) read(); return *this; }

    virtual void read_sequence_data() = 0;
};


class GenbankReader : public Reader {
public:
    GenbankReader(const char *inf) : Reader(inf) {}

    void read_sequence_data() { eof = genbank_origin(linebuf, fbuf); }
    const char *get_key_word(int offset) {
        char key[TOKENSIZE];
        genbank_key_word(line() + offset, 0, key, TOKENSIZE);

        static char *keep = NULL;
        freedup(keep, key); // @@@
        return keep;
    }
};

class EmblSwissprotReader : public Reader {
public:
    EmblSwissprotReader(const char *inf) : Reader(inf) {}

    void read_sequence_data() { eof = embl_origin(linebuf, fbuf); }
    const char *get_key_word(int offset) {
        char key[TOKENSIZE];
        embl_key_word(line() + offset, 0, key, TOKENSIZE);

        static char *keep = NULL;
        freedup(keep, key); // @@@
        return keep;
    }
};

#else
#error types.h included twice
#endif // TYPES_H
