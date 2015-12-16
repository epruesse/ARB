// ============================================================= //
//                                                               //
//   File      : ChecksumCollector.h                             //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in March 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef CHECKSUMCOLLECTOR_H
#define CHECKSUMCOLLECTOR_H

#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif
#ifndef ARBDBT_H
#include <arbdbt.h>
#endif


class ChecksumCollector {
    // helper class to calculate checksums for complex custom data structures
    // 
    // Derive from this class,
    // traverse your structure in ctor and
    // collect() all data members.

    std::vector<unsigned char> data;

protected:
    
    void collect_byte(unsigned char c) { data.push_back(c); }
    void collect_mem(void *v, int len) {
        const unsigned char *c = (const unsigned char *)v;
        for (int p = 0; p<len; ++p) {
            collect_byte(c[p]);
        }
    }
    
    void collect(unsigned char c) { collect_byte(c); }
    void collect(char c) { collect_byte(c); }
    void collect(bool b) { collect_byte(b); }

    void collect(const float& f) { collect_mem((unsigned char *)&f, sizeof(f)); }
    void collect(const double& d) { collect_mem((unsigned char *)&d, sizeof(d)); }

    void collect(const char *s) {
        if (s) {
            for (int p = 0; s[p]; ++p) collect_byte(s[p]);
            collect_byte(0);
        }
        else {
            collect_byte(0);
            collect_byte(0);
        }
    }

public:
    ChecksumCollector() {}
    ~ChecksumCollector() {}

    uint32_t checksum() const {
        size_t         size   = data.size();
        unsigned char *buffer = (unsigned char *)malloc(size);
        for (size_t s = 0; s<size; ++s) buffer[s] = data[s];
        uint32_t cs = GB_checksum((char*)buffer, size, false, NULL);
        free(buffer);
        return cs;
    }
};


#else
#error ChecksumCollector.h included twice
#endif // CHECKSUMCOLLECTOR_H
