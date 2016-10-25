#ifndef GENBANK_H
#define GENBANK_H

#ifndef REFS_H
#include "refs.h"
#endif
#ifndef PARSER_H
#include "parser.h"
#endif
#ifndef ARB_STRING_H
#include <arb_string.h>
#endif

struct GenbankRef {
    char *ref;
    char *author;
    char *title;
    char *journal;
    char *standard;

    GenbankRef()
        : ref(no_content()),
          author(no_content()),
          title(no_content()),
          journal(no_content()),
          standard(no_content())
    {}
    GenbankRef(const GenbankRef& other)
        : ref(ARB_strdup(other.ref)),
          author(ARB_strdup(other.author)),
          title(ARB_strdup(other.title)),
          journal(ARB_strdup(other.journal)),
          standard(ARB_strdup(other.standard))
    {}
    ~GenbankRef() {
        free(standard);
        free(journal);
        free(title);
        free(author);
        free(ref);
    }
    DECLARE_ASSIGNMENT_OPERATOR(GenbankRef);
};

class GenBank : public InputFormat, public RefContainer<GenbankRef> { // derived from a Noncopyable
    char *create_id() const OVERRIDE {
        char buf[TOKENSIZE];
        genbank_key_word(locus, 0, buf);
        return ARB_strdup(buf);
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
    ~GenBank() OVERRIDE {
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
        return ARB_strdup(genbank_date(today_date()));
    }

    // InputFormat interface
    void reinit() OVERRIDE { INPLACE_RECONSTRUCT(GenBank, this); }
    Format format() const OVERRIDE { return GENBANK; }
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
    bool read_one_entry(Seq& seq) OVERRIDE __ATTR__USERESULT;
    InputFormat& get_data() OVERRIDE { return data; }
};

// ----------------------
//      GenbankParser

class GenbankParser : public Parser {
    GenBank& gbk;

    void parse_keyed_section(const char *key);
public:
    GenbankParser(GenBank& gbk_, Seq& seq_, GenbankReader& reader_) : Parser(seq_, reader_), gbk(gbk_) {}
    void parse_section() OVERRIDE;

    const GenBank& get_data() const OVERRIDE { return gbk; }
};

#else
#error genbank.h included twice
#endif // GENBANK_H
