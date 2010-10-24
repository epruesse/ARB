#ifndef EMBL_H
#define EMBL_H

#ifndef REFS_H
#include "refs.h"
#endif
#ifndef PARSER_H
#include "parser.h"
#endif

struct Emblref {
    char *author;
    char *title;
    char *journal;
    char *processing;

    Emblref() {
        author     = strdup("");
        journal    = strdup("");
        title      = strdup("");
        processing = strdup("");
    }
    ~Emblref() {
        freenull(author);
        freenull(journal);
        freenull(title);
        freenull(processing);
    }
    Emblref(const Emblref& other) {
        freedup(author, other.author);
        freedup(journal, other.journal);
        freedup(title, other.title);
        freedup(processing, other.processing);
    }
    DECLARE_ASSIGNMENT_OPERATOR(Emblref);
};

class Embl : public InputFormat, public RefContainer<Emblref> {
    char *create_id() const {
        char buf[TOKENSIZE];
        embl_key_word(ID, 0, buf);
        return strdup(buf);
    }

public:
    char *ID;                    // entry name
    char *dateu;                 // date of last updated
    char *datec;                 // date of created
    char *description;           // description line (DE)
    char *os;                    // Organism species
    char *accession;             // accession number(s)
    char *keywords;              // keyword
    char *dr;                    // database cross-reference

    RDP_comments  comments;          // comments

    Embl() {
        ID          = no_content();
        dateu       = no_content();
        datec       = no_content();
        description = no_content();
        os          = no_content();
        accession   = no_content();
        keywords    = no_content();
        dr          = no_content();
    }
    virtual ~Embl() {
        freenull(ID);
        freenull(dateu);
        freenull(datec);
        freenull(description);
        freenull(os);
        freenull(accession);
        freenull(keywords);
        freenull(dr);
    }

    // InputFormat interface
    void reinit() { INPLACE_RECONSTRUCT(Embl, this); }
    Format format() const { return EMBL; }
};

class EmblSwissprotReader : public SimpleFormatReader {
    Embl data;
public:
    EmblSwissprotReader(const char *inf) : SimpleFormatReader(inf) {}

    const char *get_key_word(int offset) {
        char key[TOKENSIZE];
        embl_key_word(line() + offset, 0, key);
        return shorttimecopy(key);
    }
    bool read_one_entry(Seq& seq) __ATTR__USERESULT;
    InputFormat& get_data() { return data; }
};

class EmblParser: public Parser {
    Embl& embl;

    void parse_keyed_section(const char *key);
public:
    EmblParser(Embl& embl_, Seq& seq_, Reader& reader_) : Parser(seq_, reader_), embl(embl_) {}
    void parse_section();

    const Embl& get_data() const { return embl; }
};

#else
#error embl.h included twice
#endif // EMBL_H
