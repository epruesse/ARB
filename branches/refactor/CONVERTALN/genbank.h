#ifndef GENBANK_H
#define GENBANK_H

#ifndef INPUT_FORMAT_H
#include "input_format.h"
#endif
#ifndef RDP_INFO_H
#include "rdp_info.h"
#endif
#ifndef REFS_H
#include "refs.h"
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

struct GenBank : public InputFormat, public RefContainer<GenbankRef> {
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
    SeqPtr get_data(FILE_BUFFER ifp);
    void reinit() { INPLACE_RECONSTRUCT(GenBank, this); }
};

#else
#error genbank.h included twice
#endif // GENBANK_H
