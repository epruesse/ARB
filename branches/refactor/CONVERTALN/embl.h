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

class Embl : Noncopyable, public InputFormat {
    Refs<Emblref> refs;

public:

    void reinit_refs() { INPLACE_RECONSTRUCT(Refs<Emblref>, &refs); }
    void resize_refs(int new_size) { refs.resize(new_size); }
    int get_refcount() const  { return refs.get_count(); }
    
    const Emblref& get_ref(int num) const { return refs.get_ref(num); }
    const Emblref& get_latest_ref() const { return get_ref(get_refcount()-1); }
    Emblref& get_ref(int num) { return refs.get_ref(num); }
    Emblref& get_latest_ref() { return get_ref(get_refcount()-1); }

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
    SeqPtr get_data(FILE_BUFFER ifp);
    void reinit() { INPLACE_RECONSTRUCT(Embl, this); }
};

#else
#error embl.h included twice
#endif // EMBL_H
