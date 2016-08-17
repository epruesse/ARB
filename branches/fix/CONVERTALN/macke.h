// ================================================================= //
//                                                                   //
//   File      : macke.h                                             //
//   Purpose   :                                                     //
//                                                                   //
// ================================================================= //

#ifndef MACKE_H
#define MACKE_H

#ifndef READER_H
#include "reader.h"
#endif
#ifndef ARB_STRING_H
#include <arb_string.h>
#endif

class Macke : public InputFormat { // derived from a Noncopyable
    int    numofrem;            // num. of remarks
    char **remarks;             // remarks
    int    allocated;

    char *create_id() const OVERRIDE { return ARB_strdup(seqabbr); }

    void add_remark_nocopy(char *rem) {
        if (numofrem >= allocated) {
            allocated = allocated*1.5+10;
            ARB_realloc(remarks, allocated);
        }
        ca_assert(allocated>numofrem);
        remarks[numofrem++] = rem;
    }

    void add_remark_if_content(const char *key, const char *Str) {
        if (has_content(Str)) add_remark(key, Str);
    }
    void add_remarks_from(const GenbankRef& ref);
    void add_remarks_from(const RDP_comments& comments);
    void add_remarks_from(const OrgInfo& orginf);
    void add_remarks_from(const SeqInfo& seqinf);
    void add_35end_remark(char end35, char yn);

    static bool macke_is_continued_remark(const char *str) {
        /* If there is 3 blanks at the beginning of the line, it is continued line.
         *
         * The comment above is lying:
         *      The function always only tested for 2 spaces
         *      and the converter only produced 2 spaces.
         */
        return strncmp(str, ":  ", 3) == 0;
    }

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
        seqabbr    = ARB_strdup("");
        name       = no_content();
        atcc       = no_content();
        rna        = no_content();
        date       = no_content();
        nbk        = no_content();
        acs        = no_content();
        who        = no_content();
        rna_or_dna = 'd'; // @@@ why? (never is changed anywhere)
        journal    = no_content();
        title      = no_content();
        author     = no_content();
        strain     = no_content();
        subspecies = no_content();

        numofrem  = 0;
        remarks   = NULL;
        allocated = 0;
    }
    virtual ~Macke() OVERRIDE {
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
    void add_remarks_from(const GenBank& gbk);

    // InputFormat interface
    void reinit() OVERRIDE { INPLACE_RECONSTRUCT(Macke, this); }
    const char *get_id() const { return seqabbr; }
    Format format() const OVERRIDE { return MACKE; }
};

// --------------------
//      MackeReader

class MackeReader : public FormatReader, virtual Noncopyable {
    Macke data;

    char   *inName;
    char*&  seqabbr; // = Macke.seqabbr
    char   *dummy;

    Reader  *r1, *r2, *r3;
    Reader **using_reader; // r1, r2 or r3

    void usingReader(Reader*& r) {
        using_reader = &r;
    }

    bool macke_in(Macke& macke);

    void abort() {
        r1->abort();
        r2->abort();
        r3->abort();
    }
    bool ok() {
        return r1->ok() && r2->ok() && r3->ok();
    }

    bool read_seq_data(Seq& seq) {
        ca_assert(seqabbr);
        usingReader(r2);
        macke_origin(seq, seqabbr, *r2);
        if (seq.is_empty()) abort();
        return r2->ok();
    }

    void read_to_start();

public:

    MackeReader(const char *inName_);
    ~MackeReader() OVERRIDE;

    bool read_one_entry(Seq& seq) OVERRIDE __ATTR__USERESULT;
    bool failed() const OVERRIDE { return r1->failed() || r2->failed() || r3->failed(); }
    void ignore_rest_of_file() OVERRIDE { r1->ignore_rest_of_file(); r2->ignore_rest_of_file(); r3->ignore_rest_of_file(); }
    InputFormat& get_data() OVERRIDE { return data; }
    void rewind() OVERRIDE {
        r1->rewind();
        r2->rewind();
        r3->rewind();
        read_to_start();
    }
};

inline bool isMackeHeader(const char *line)    { return line[0] == '#'; }
inline bool isMackeSeqHeader(const char *line) { return line[0] == '#' && line[1] == '='; }
inline bool isMackeSeqInfo(const char *line)   { return line[0] == '#' && line[1] == ':'; }

inline bool isMackeNonSeq(const char *line)    { return line[0] == '#' || line[0] == '\n' || line[0] == ' '; }

class Not {
    typedef bool (*LinePredicate)(const char *line);
    LinePredicate p;
public:
    Not(LinePredicate p_) : p(p_) {}
    bool operator()(const char *line) const { return !p(line); }
};

#else
#error macke.h included twice
#endif // MACKE_H
