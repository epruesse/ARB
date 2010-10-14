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

// --------------------
//      Seq

#define INITSEQ 6000

class Seq {
    // - holds sequence data

    char *id;
    int   len;     // sequence length
    char *seq;     // sequence data
    int   max;     // space allocated for 'sequence'

public:
    Seq() {
        id  = NULL;
        len = 0;
        max = INITSEQ;
        seq = (char *)calloc(1, (unsigned)(sizeof(char) * INITSEQ + 1));

    }
    ~Seq() {
        ca_assert(seq); // otherwise 'this' is useless!
        freenull(id);
        freenull(seq);
    }

    void set_id(const char *id_) { freedup(id, id_); }
    const char *get_id() const { return id; }

    void add(char c) {
        if (len >= max) {
            max      = max*1.5+100;
            seq = (char*)Reallocspace(seq, max);
        }
        seq[len++] = c;
    }
    void zeroTerminate() { add(0); len--; }

    int get_len() const { return len; }
    bool is_empty() const { return len == 0; }

    const char *get_seq() const {
        if (max == len || seq[len]) { // not zero-terminated
            const_cast<Seq*>(this)->zeroTerminate();
        }
        return seq;
    }

    int checksum() const { return ::checksum(seq, len); }
};
typedef SmartPtr<Seq> SeqPtr;

class Alignment { // @@@ implement using SeqPtr-array
    char **ids;          // array of ids.
    char **seqs;         // array of sequence data
    int   *lengths;      // array of sequence lengths

    int allocated;       // for how many sequences space has been allocated
    int added;

    void resize(int wanted) {
        ca_assert(allocated<wanted);

        ids     = (char**) Reallocspace((char*)ids,     wanted * sizeof(*ids));
        seqs    = (char**) Reallocspace((char*)seqs,    wanted * sizeof(*seqs));
        lengths = (int*)   Reallocspace((char*)lengths, wanted * sizeof(*lengths));

        allocated = wanted;
    }

public:
    Alignment() {
        ids       = NULL;
        seqs      = NULL;
        lengths   = NULL;
        allocated = 0;
        added     = 0;
    }
    ~Alignment() {
        for (int i = 0; i<added; ++i) {
            free(ids[i]);
            free(seqs[i]);
        }
        free(ids);
        free(seqs);
        free(lengths);
    }

    const char *get_id(int idx) const {
        ca_assert(idx<added);
        return ids[idx];
    }
    const char *get_seq(int idx) const {
        ca_assert(idx<added);
        return seqs[idx];
    }
    int get_len(int idx) const {
        ca_assert(idx<added);
        return lengths[idx];
    }

    int get_count() const { return added; }
    int get_max_len() const {
        int maxlen = -1;
        for (int i = 0; i<added; ++i) maxlen = max(maxlen, get_len(i));
        return maxlen;
    }

    void add(const char *name, const char *seq, size_t seq_len) {
        ca_assert(strlen(seq) == seq_len);

        if (added >= allocated) resize(added*1.5+5);

        ids[added]     = strdup(name);
        seqs[added]    = strdup(seq);
        lengths[added] = seq_len;

        added++;
    }

    void add(const char *name, const char *seq) {
        add(name, seq, strlen(seq));
    }

    void add(SeqPtr seq) {
        add(seq->get_id(), seq->get_seq(), seq->get_len());
    }
};


// ----------------
//      Readers


class Reader {
    FILE *fp;

    void read() { eof = Fgetline(linebuf, LINESIZE, fbuf); }

protected:

    FILE_BUFFER  fbuf;
    char         linebuf[LINESIZE];
    char        *eof; // @@@ rename

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

    template<typename PRED>
    void skipOverLinesThat(PRED match_condition) {
        while (match_condition(line()))
            ++(*this);
    }
};

struct DataReader {
    virtual ~DataReader() {}
    virtual SeqPtr read_sequence_data() = 0;
};

inline const char *shorttimekeep(char *heapcopy) {
    static SmartMallocPtr(char) keep;
    keep = heapcopy;
    return &*keep;
}
inline const char *shorttimecopy(const char *nocopy) { return shorttimekeep(nulldup(nocopy)); }

class GenbankReader : public Reader, public DataReader {
public:
    GenbankReader(const char *inf) : Reader(inf) {}

    SeqPtr read_sequence_data() {
        SeqPtr seq = new Seq;
        eof = genbank_origin(*seq, linebuf, fbuf);
        return seq;
    }
    const char *get_key_word(int offset) {
        char key[TOKENSIZE];
        genbank_key_word(line() + offset, 0, key, TOKENSIZE);
        return shorttimecopy(key);
    }
};

class EmblSwissprotReader : public Reader, public DataReader {
public:
    EmblSwissprotReader(const char *inf) : Reader(inf) {}

    SeqPtr read_sequence_data() {
        SeqPtr seq = new Seq;
        eof = embl_origin(*seq, linebuf, fbuf);
        return seq;
    }
    const char *get_key_word(int offset) {
        char key[TOKENSIZE];
        embl_key_word(line() + offset, 0, key, TOKENSIZE);
        return shorttimecopy(key);
    }
};

#else
#error types.h included twice
#endif // TYPES_H
