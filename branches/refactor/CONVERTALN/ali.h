#ifndef ALI_H
#define ALI_H

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

#else
#error ali.h included twice
#endif // ALI_H
