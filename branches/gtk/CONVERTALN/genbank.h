#ifndef GENBANK_H
#define GENBANK_H

#ifndef REFS_H
#include "refs.h"
#endif
#ifndef PARSER_H
#include "parser.h"
#endif


struct GenbankRef {
    char *ref;
    char *author;
    char *title;
    char *journal;
    char *standard;

    GenbankRef() {
        ref      = no_content();
        author   = no_content();
        title    = no_content();
        journal  = no_content();
        standard = no_content();
    }
    GenbankRef(const GenbankRef& other) {
        freedup(ref, other.ref);
        freedup(author, other.author);
        freedup(title, other.title);
        freedup(journal, other.journal);
        freedup(standard, other.standard);
    }
    ~GenbankRef() {
        freenull(ref);
        freenull(author);
        freenull(title);
        freenull(journal);
        freenull(standard);
    }
    DECLARE_ASSIGNMENT_OPERATOR(GenbankRef);
};

class GenBank : public InputFormat, public RefContainer<GenbankRef> { // derived from a Noncopyable
    char *create_id() const {
        char buf[TOKENSIZE];
        genbank_key_word(locus, 0, buf);
        return strdup(buf);
    }
public:
    char *locus;
    char *definition;
    char *accession;
    char *keywords;
    char *source;
    char *organism;

    RDP_comments comments;

    GenBank() {
        locus      = no_content();
        definition = no_content();
        accession  = no_content();
        keywords   = no_content();
        source     = no_content();
        organism   = no_content();
    }
    virtual ~GenBank() {
        freenull(locus);
        freenull(definition);
        freenull(accession);
        freenull(keywords);
        freenull(source);
        freenull(organism);
    }

    bool locus_contains_date() const { return str0len(locus) >= 60; }

    char *get_date() const {
        if (locus_contains_date()) return strndup(locus+50, 11);
        return strdup(genbank_date(today_date()));
    }

    // InputFormat interface
    void reinit() { INPLACE_RECONSTRUCT(GenBank, this); }
    Format format() const { return GENBANK; }
};

class GenbankReader : public SimpleFormatReader {
    GenBank data;
public:
    GenbankReader(const char *inf) : SimpleFormatReader(inf) {}

    const char *get_key_word(int offset) {
        char key[TOKENSIZE];
        genbank_key_word(line() + offset, 0, key);
        return shorttimecopy(key);
    }
    bool read_one_entry(Seq& seq) __ATTR__USERESULT;
    InputFormat& get_data() { return data; }
};

// ----------------------
//      GenbankParser

class GenbankParser : public Parser {
    GenBank& gbk;

    void parse_keyed_section(const char *key);
public:
    GenbankParser(GenBank& gbk_, Seq& seq_, GenbankReader& reader_) : Parser(seq_, reader_), gbk(gbk_) {}
    void parse_section();

    const GenBank& get_data() const { return gbk; }
};

#else
#error genbank.h included twice
#endif // GENBANK_H
