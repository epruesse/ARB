#ifndef EMBL_H
#define EMBL_H

#ifndef INPUT_FORMAT_H
#include "input_format.h"
#endif
#ifndef RDP_INFO_H
#include "rdp_info.h"
#endif
#ifndef REFS_H
#include "refs.h"
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

struct Embl : public InputFormat, public RefContainer<Emblref> {
    char *id;                    // entry name
    char *dateu;                 // date of last updated
    char *datec;                 // date of created
    char *description;           // description line (DE)
    char *os;                    // Organism species
    char *accession;             // accession number(s)
    char *keywords;              // keyword
    char *dr;                    // database cross-reference

    RDP_comments  comments;          // comments

    Embl() {
        id          = no_content();
        dateu       = no_content();
        datec       = no_content();
        description = no_content();
        os          = no_content();
        accession   = no_content();
        keywords    = no_content();
        dr          = no_content();
    }
    virtual ~Embl() {
        freenull(id);
        freenull(dateu);
        freenull(datec);
        freenull(description);
        freenull(os);
        freenull(accession);
        freenull(keywords);
        freenull(dr);
    }

    // InputFormat interface
    SeqPtr read_data(Reader& reader);
    void reinit() { INPLACE_RECONSTRUCT(Embl, this); }
    const char *get_id() const { return id; }
};

#ifndef READER_H
#include "reader.h"
#endif


class EmblSwissprotReader : public Reader, public InputFormatReader {
public:
    EmblSwissprotReader(const char *inf) : Reader(inf) {}

    bool read_seq_data(Seq& seq) {
        embl_origin(seq, *this);
        return ok();
    }

    const char *get_key_word(int offset) {
        char key[TOKENSIZE];
        embl_key_word(line() + offset, 0, key, TOKENSIZE);
        return shorttimecopy(key);
    }
    bool read_one_entry(InputFormat& data, Seq& seq) {
        Embl& embl = reinterpret_cast<Embl&>(data);
        embl_in(embl, seq, *this);
        return ok();
    }
};


#else
#error embl.h included twice
#endif // EMBL_H
