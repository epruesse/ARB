#ifndef SEQ_H
#define SEQ_H

#ifndef FUN_H
#include "fun.h"
#endif
#ifndef GLOBAL_H
#include "global.h"
#endif

#define INITSEQ 6000

struct BaseCounts {
    int a;
    int c;
    int g;
    int t;
    int other;

    BaseCounts()
        : a(0), c(0), g(0), t(0), other(0)
    {}

    void add(char ch) {
        switch (tolower(ch)) {
            case 'a': a++; break;
            case 'c': c++; break;
            case 'g': g++; break;
            case 'u':
            case 't': t++; break;
            default: other++; break;
        }
    }
};

class Seq {
    // - holds sequence data

    char *id;
    int   len;     // sequence length
    int   max;     // space allocated for 'sequence'
    char *seq;     // sequence data

    void zeroTerminate() { add(0); len--; }

public:
    Seq(const char *id_, const char *seq_, int len_)
        : id(strdup(id_)),
          len(len_),
          max(len+1),
          seq((char*)malloc(max))
    {
        memcpy(seq, seq_, len);
    }
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
    const char *get_id() const { ca_assert(id); return id; }

    void add(char c) {
        if (len >= max) {
            max      = max*1.5+100;
            seq = (char*)Reallocspace(seq, max);
        }
        seq[len++] = c;
    }

    int get_len() const { return len; }
    bool is_empty() const { return len == 0; }

    const char *get_seq() const {
        const_cast<Seq*>(this)->zeroTerminate();
        return seq;
    }

    void count(BaseCounts& counter) const {
        for (int i = 0; i<len; ++i)
            counter.add(seq[i]);
    }

    int count_gaps() const {
        int gaps = 0;
        for (int i = 0; i<len; ++i) {
            gaps += is_gapchar(seq[i]);
        }
        return gaps;
    }

    void out(Writer& write, Format outType) const;
};
typedef SmartPtr<Seq> SeqPtr;

#else
#error seq.h included twice
#endif // SEQ_H
