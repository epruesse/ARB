#ifndef ALI_H
#define ALI_H

#ifndef SEQ_H
#include "seq.h"
#endif
#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif

class Alignment {
    std::vector<SeqPtr> seq;

public:
    int get_count() const { return seq.size(); }
    bool valid(int idx) const { return idx >= 0 && idx<get_count(); }

    int get_len(int idx) const { ca_assert(valid(idx)); return seq[idx]->get_len(); }
    const Seq& get(int idx) const { ca_assert(valid(idx)); return *(seq[idx]); }
    SeqPtr getSeqPtr(int idx) { ca_assert(valid(idx)); return seq[idx]; }

    int get_max_len() const {
        int maxlen = -1;
        for (int i = 0; i<get_count(); ++i) maxlen = max(maxlen, get_len(i));
        return maxlen;
    }

    void add(SeqPtr sequence) { seq.push_back(sequence); }
    void add(const char *name, const char *sequence, size_t seq_len) { add(new Seq(name, sequence, seq_len)); }
    void add(const char *name, const char *sequence) { add(name, sequence, strlen(sequence)); }
};

#else
#error ali.h included twice
#endif // ALI_H
