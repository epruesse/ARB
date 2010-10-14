#ifndef CONVERT_H
#define CONVERT_H

#ifndef SMARTPTR_H
#include <smartptr.h>
#endif
#ifndef TYPES_H
#include "types.h"
#endif


// data structure for each file format and sequences 

#define RNA     0
#define NONRNA  1

#define GENBANK   'g'
#define MACKE     'm'
#define SWISSPROT 't'
#define PHYLIP    'y'
#define PHYLIP2   '2'
#define NEXUS     'p'
#define EMBL      'e'
#define GCG       'c'
#define PRINTABLE 'r'
#define NBRF      'n'
#define STADEN    's'

#define UNKNOWN  (-1)
#define AUTHOR   1
#define JOURNAL  2
#define TITLE    3
#define STANDARD 4
#define PROCESS  5
#define REF      6
#define ALL      0

#define SEQLINE 60

// -------------------------------

#define INPLACE_RECONSTRUCT(type,this)                  \
    do {                                                \
        (this)->~type();                                \
        new(this) type();                               \
    } while(0)

#define INPLACE_COPY_RECONSTRUCT(type,this,other)       \
    do {                                                \
        (this)->~type();                                \
        new(this) type(other);                          \
    } while(0)

#define DECLARE_ASSIGNMENT_OPERATOR(T)                  \
    T& operator = (const T& other) {                    \
        INPLACE_COPY_RECONSTRUCT(T, this, other);       \
        return *this;                                   \
    }                                                   \


// -------------------------------
//      RDP-defined comments (Embl+GenBank)

struct OrgInfo {
    bool  exists;
    char *source;
    char *cultcoll;
    char *formname;
    char *nickname;
    char *commname;
    char *hostorg;

    OrgInfo() {
        exists = false;
        source   = no_content();
        cultcoll = no_content();
        formname = no_content();
        nickname = no_content();
        commname = no_content();
        hostorg  = no_content();
    }
    ~OrgInfo() {
        freenull(source);
        freenull(cultcoll);
        freenull(formname);
        freenull(nickname);
        freenull(commname);
        freenull(hostorg);
    }
    OrgInfo(const OrgInfo& other) {
        exists   = other.exists;
        source   = nulldup(other.source);
        cultcoll = nulldup(other.cultcoll);
        formname = nulldup(other.formname);
        nickname = nulldup(other.nickname);
        commname = nulldup(other.commname);
        hostorg  = nulldup(other.hostorg);
    }
    DECLARE_ASSIGNMENT_OPERATOR(OrgInfo);
};

struct SeqInfo {
    bool exists;
    char comp3;  // yes or no, y/n
    char comp5;  // yes or no, y/n

    char *RDPid;
    char *gbkentry;
    char *methods;

    SeqInfo() {
        exists   = false;
        comp3    = ' ';
        comp5    = ' ';
        RDPid    = no_content();
        gbkentry = no_content();
        methods  = no_content();
    }
    ~SeqInfo() {
        freenull(RDPid);
        freenull(gbkentry);
        freenull(methods);
    }
    SeqInfo(const SeqInfo& other) {
        exists   = other.exists;
        comp3    = other.comp3;
        comp5    = other.comp5;
        RDPid    = nulldup(other.RDPid);
        gbkentry = nulldup(other.gbkentry);
        methods  = nulldup(other.methods);
    }
    DECLARE_ASSIGNMENT_OPERATOR(SeqInfo);
};

struct Comments {
    OrgInfo  orginf;
    SeqInfo  seqinf;
    char    *others;

    Comments() { others = NULL; }
    Comments(const Comments& other) { others = nulldup(other.others); }
    ~Comments() { freenull(others); }
    DECLARE_ASSIGNMENT_OPERATOR(Comments);
};

// --------------------------------
//      Embl/GenBank references

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

#define USE_NEW_REF

template <typename R>
class References : Noncopyable {
    R   *ref;
    int  size;
public: 
    References() {
        ref  = NULL;
        size = 0;
    }
    ~References() {
        delete [] ref;
    }

    void resize(int new_size) {
        ca_assert(new_size >= size);
        if (new_size>size) {
            R *new_ref = new R[new_size];
            for (int i = 0; i<size; ++i) {
                new_ref[i] = ref[i];
            }
            delete [] ref;
            ref  = new_ref;
            size = new_size;
        }
    }

    int get_count() const { return size; }

    const R& get_ref(int num) const {
        ca_assert(num >= 0);
        ca_assert(num < get_count());
        return ref[num];
    }
    R& get_ref(int num) {
        ca_assert(num >= 0);
        ca_assert(num < get_count());
        return ref[num];
    }
};

// --------------------
//      InputFormat

struct InputFormat {
    virtual ~InputFormat() {}
    virtual SeqPtr get_data(FILE_BUFFER ifp) = 0;
    virtual void reinit() = 0;

    static SmartPtr<InputFormat> create(char informat);
    static bool is_known(char informat) {
        return informat == GENBANK || informat == EMBL || informat == SWISSPROT || informat == MACKE;
    }
};

// -------------
//      Embl

class Embl : Noncopyable, public InputFormat {
    References<Emblref> refs;

public:

    void reinit_refs() { INPLACE_RECONSTRUCT(References<Emblref>, &refs); }
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

    Comments  comments;          // comments

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

// -----------------
//      GenBank

class GenBank : Noncopyable, public InputFormat {
    References<GenbankRef> refs;

public: 
    void reinit_refs() { INPLACE_RECONSTRUCT(References<GenbankRef>, &refs); }
    void resize_refs(int new_size) { refs.resize(new_size); }
    int get_refcount() const  { return refs.get_count(); }
    const GenbankRef& get_ref(int num) const { return refs.get_ref(num); }
    const GenbankRef& get_latest_ref() const { return get_ref(get_refcount()-1); }
    GenbankRef& get_ref(int num) { return refs.get_ref(num); }
    GenbankRef& get_latest_ref() { return get_ref(get_refcount()-1); }

    char *locus;
    char *definition;
    char *accession;
    char *keywords;
    char *source;
    char *organism;

    Comments comments;

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

// --------------
//      Macke

class Macke : public InputFormat {
    int    numofrem;            // num. of remarks
    char **remarks;             // remarks
    int    allocated;

public:

    char  *seqabbr;             // sequence abbrev.
    char  *name;                // sequence full name
    int    rna_or_dna;          // rna or dna
    char  *atcc;                // CC# of sequence
    char  *rna;                 // Sequence methods, old version entry
    char  *date;                // date of modification
    char  *nbk;                 // GenBank information -old version entry
    char  *acs;                 // accession number
    char  *author;              // author of the first reference
    char  *journal;             // journal of the first reference
    char  *title;               // title of the first reference
    char  *who;                 // who key in the data
    char  *strain;              // strain
    char  *subspecies;          // subspecies

    Macke() {
        seqabbr    = strdup("");
        name       = no_content();
        atcc       = no_content();
        rna        = no_content();
        date       = no_content();
        nbk        = no_content();
        acs        = no_content();
        who        = no_content();
        rna_or_dna = 'd'; // @@@ why ?
        journal    = no_content();
        title      = no_content();
        author     = no_content();
        strain     = no_content();
        subspecies = no_content();

        numofrem  = 0;
        remarks   = NULL;
        allocated = 0;
    }
    virtual ~Macke() {
        freenull(seqabbr);
        freenull(name);
        freenull(atcc);
        freenull(rna);
        freenull(date);
        freenull(nbk);
        freenull(acs);
        freenull(who);
        for (int indi = 0; indi < numofrem; indi++) {
            freenull(remarks[indi]);
        }
        freenull(remarks);
        freenull(journal);
        freenull(title);
        freenull(author);
        freenull(strain);
        freenull(subspecies);
    }

    void add_remark_nocopy(char *rem) {
        if (numofrem >= allocated) {
            allocated = allocated*1.5+10;
            remarks   = (char**)Reallocspace(remarks, sizeof(*remarks)*allocated);
        }
        ca_assert(allocated>numofrem);
        remarks[numofrem++] = rem;
    }
    void add_remark(const char *rem) { add_remark_nocopy(nulldup(rem)); }
    void add_remark(const char *key, const char *Str) {
        char *rem = nulldup(key);
        Append(rem, Str);
        add_remark_nocopy(rem);
    }

    int get_rem_count() const { return numofrem; }
    const char *get_rem(int idx) const {
        ca_assert(idx<numofrem);
        return remarks[idx];
    }

    void add_remark_if_content(const char *key, const char *Str) {
        if (has_content(Str)) add_remark(key, Str);
    }

    char *copy_multi_rem(int& idx, int offset) const {
        // create a heapcopy of a multiline-remark.
        // increments 'idx' to the last line.
        char *rem = nulldup(remarks[idx]+offset);
        while (++idx<numofrem && macke_is_continued_remark(remarks[idx])) {
            skip_eolnl_and_append_spaced(rem, remarks[idx]+3);
        }
        --idx;
        return rem;
    }
    
    // InputFormat interface
    SeqPtr get_data(FILE_BUFFER ifp);
    void reinit() { INPLACE_RECONSTRUCT(Macke, this); }
};

// -------------
//      Paup

struct Paup {
    int         ntax;           // number of sequences
    int         nchar;          // max number of chars per sequence
    const char *equate;         // equal meaning char
    char        gap;            // char of gap, default is '-'

    Paup() {
        ntax  = 0;
        nchar = 0;
        equate = "~=.|><";
        gap    = '-';
    }
};

#else
#error convert.h included twice
#endif // CONVERT_H
