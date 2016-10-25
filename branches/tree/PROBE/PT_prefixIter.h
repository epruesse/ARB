// ================================================================ //
//                                                                  //
//   File      : PT_prefixIter.h                                    //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2012   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef PT_PREFIXITER_H
#define PT_PREFIXITER_H

class PrefixIterator {
    // iterates over prefixes of length given to ctor.
    //
    // PT_QU will only occur at end of prefix,
    // i.e. prefixes will be shorter than given length if PT_QU occurs

    PT_base low, high;
    int      len;

    char *part;
    int   plen;

    char END() const { return high+1; }
    size_t base_span() const { return high-low+1; }

    void inc() {
        if (plen) {
            do {
                part[plen-1]++;
                if (part[plen-1] == END()) {
                    --plen;
                }
                else {
                    for (int l = plen; l<len; ++l) {
                        part[l] = low;
                        plen    = l+1;
                        if (low == PT_QU) break;
                    }
                    break;
                }
            }
            while (plen);
        }
        else {
            part[0] = END();
        }
    }

public:

    void reset() {
        if (len) {
            for (int l = 0; l<len; ++l) part[l] = low;
        }
        else {
            part[0] = 0;
        }

        plen = (low == PT_QU && len) ? 1 : len;
    }

    PrefixIterator(PT_base low_, PT_base high_, int len_)
        : low(low_),
          high(high_),
          len(len_),
          part(ARB_alloc<char>(len ? len : 1))
    {
        reset();
    }
    PrefixIterator(const PrefixIterator& other)
        : low(other.low),
          high(other.high),
          len(other.len),
          part(ARB_alloc<char>(len ? len : 1)),
          plen(other.plen)
    {
        memcpy(part, other.part, plen);
    }
    DECLARE_ASSIGNMENT_OPERATOR(PrefixIterator);
    ~PrefixIterator() {
        free(part);
    }

    const char *prefix() const { return part; }
    size_t length() const { return plen; }
    size_t max_length() const { return len; }

    char *copy() const {
        pt_assert(!done());
        char *result = ARB_alloc<char>(plen+1);
        memcpy(result, part, plen);
        result[plen] = 0;
        return result;
    }

    bool done() const { return part[0] == END(); }

    const PrefixIterator& operator++() { // ++PrefixIterator
        pt_assert(!done());
        inc();
        return *this;
    }

    size_t steps() const {
        size_t count = 1;
        size_t bases = base_span();
        for (int l = 0; l<len; ++l) {
            if (low == PT_QU) {
                // PT_QU branches end instantly
                count = 1+(bases-1)*count;
            }
            else {
                count *= bases;
            }
        }
        return count;
    }

    bool matches_at(const char *probe) const {
        for (int p = 0; p<plen; ++p) {
            if (probe[p] != part[p]) {
                return false;
            }
        }
        return true;
    }
};

#else
#error PT_prefixIter.h included twice
#endif // PT_PREFIXITER_H
