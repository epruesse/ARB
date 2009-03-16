// ================================================================ //
//                                                                  //
//   File      : SequenceBuffer.h                                   //
//   Purpose   :                                                    //
//   Time-stamp: <Thu Dec/07/2006 15:14 MET Coder@ReallySoft.de>    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in December 2006   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //
#ifndef SEQUENCEBUFFER_H
#define SEQUENCEBUFFER_H

#ifndef TYPES_H
#include "types.h"
#endif
#ifndef SMARTPTR_H
#include <smartptr.h>
#endif

class CharCounter {
    size_t count[256];
    size_t all;

public:
    CharCounter() { clear(); }

    size_t getCount(unsigned char idx) const { return count[idx]; }
    size_t getCount() const { return all; }
    void countChars(const string& line);

    void clear();
};


enum Base { BC_A, BC_C, BC_G, BC_T, BC_OTHER, BC_ALL, BC_COUNTERS };

class BaseCounter {
    string source;              // where does information originate from
    size_t count[BC_COUNTERS];  // number of occurances of single bases

    SmartPtr<CharCounter> char_count; // character counter (used by addLine)

    void catchUpWithLineCounter() const;

public:
    BaseCounter(const string& Source)
        : source(Source)
    {
        for (int i = 0; i<BC_COUNTERS; ++i) count[i] = 0;
    }

    void addCount(Base base, size_t amount) { count[base] += amount; }
    void checkOverallCounter() const;
    void calcOverallCounter();

    void startLineCounter();
    void addLine(const string& line) {
        gi_assert(!char_count.Null()); // call startLineCounter before!
        char_count->countChars(line);
    }

    void expectEqual(const BaseCounter& other) const;
    size_t getCount(Base base) const {
        catchUpWithLineCounter();
        return count[base];
    }
};

class SequenceBuffer : public Noncopyable {
    stringVector  lines;        // stores input lines
    BaseCounter   baseCounter;
    mutable char *seq;

public:
    SequenceBuffer(size_t expectedSize)
        : baseCounter("sequence data"), seq(0)
    {
        lines.reserve(expectedSize/60+1); // flatfiles use 60 bases per sequence line
        baseCounter.startLineCounter();
    }
    ~SequenceBuffer();

    void addLine(const string& line) {
        lines.push_back(line);
        baseCounter.addLine(line);
    }

    const BaseCounter& getBaseCounter() const { return baseCounter; }
    BaseCounter& getBaseCounter() { return baseCounter; }
    
    const char *getSequence() const;
};


#else
#error SequenceBuffer.h included twice
#endif // SEQUENCEBUFFER_H

