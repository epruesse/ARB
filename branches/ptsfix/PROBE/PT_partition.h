// =============================================================== //
//                                                                 //
//   File      : PT_partition.h                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2012   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef PT_PARTITION_H
#define PT_PARTITION_H

class Partition : virtual Noncopyable {
    PT_BASES low, high;
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
    Partition(PT_BASES low_, PT_BASES high_, int len_)
        : low(low_),
          high(high_),
          len(len_),
          part((char*)malloc(len ? len : 1))
    {
        if (len) {
            for (int l = 0; l<len; ++l) part[l] = low;
        }
        else {
            part[0] = 0;
        }

        plen = (low == PT_QU && len) ? 1 : len;
    }
    ~Partition() {
        free(part);
    }

    const char *partstring() const { return part; }
    size_t partlen() const { return plen; }
    bool follows() const { return part[0] != END(); }

    const Partition& operator++() { // ++Partition
        pt_assert(follows());
        inc();
        return *this;
    }

    size_t size() const {
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

    bool contains(const char *probe) const {
        for (int p = 0; p<plen; ++p) {
            if (probe[p] != part[p]) {
                return false;
            }
        }
        return true;
    }
};

#else
#error PT_partition.h included twice
#endif // PT_PARTITION_H
