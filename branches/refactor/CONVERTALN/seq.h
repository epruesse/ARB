#ifndef SEQ_H
#define SEQ_H

#ifndef FUN_H
#include "fun.h"
#endif
#ifndef SMARTPTR_H
#include <smartptr.h>
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

    void count(BaseCounts& counter) const {
        for (int i = 0; i<len; ++i)
            counter.add(seq[i]);
    }
};
typedef SmartPtr<Seq> SeqPtr;

#else
#error seq.h included twice
#endif // SEQ_H
