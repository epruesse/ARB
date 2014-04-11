// ================================================================ //
//                                                                  //
//   File      : SequenceBuffer.cxx                                 //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in December 2006   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //
#include "SequenceBuffer.h"

using namespace std;

void CharCounter::clear() {
    for (int i = 0; i<256; i++) count[i] = 0;
    all = 0;
}

void CharCounter::countChars(const string& line) {
    string::const_iterator e = line.end();
    for (string::const_iterator i = line.begin(); i != e; ++i) {
        ++count[static_cast<unsigned char>(*i)];
        ++all;
    }
}

// --------------------------------------------------------------------------------

void BaseCounter::calcOverallCounter() {
    gi_assert(char_count.isNull()); // may not be used for line counted BaseCounter
    gi_assert(count[BC_ALL] == 0); // already have a value for overall counter

    size_t all                          = 0;
    for (int i = 0; i<BC_ALL; ++i) all += count[i];

    if (count[BC_ALL] == 0) {   // got no value for all counter -> use calculated value
        count[BC_ALL] = all;
    }
}

void BaseCounter::checkOverallCounter() const {
    catchUpWithLineCounter();

    size_t all = 0;

    for (int i = 0; i<BC_ALL; ++i) all += count[i];

    gi_assert(count[BC_ALL]>0); // forgot to call calcOverallCounter ?
    if (count[BC_ALL] != all) {
        throw GBS_global_string("Overall bp (=%zu) does not match sum (=%zu) of single bases (Occurrence: '%s')",
                                count[BC_ALL], all, source.c_str());
    }
}

void BaseCounter::catchUpWithLineCounter() const {
    if (!char_count.isNull()) {
        CharCounter& cc  = const_cast<CharCounter&>(*char_count);
        size_t       all = cc.getCount(); // all bases

        if (all>0) { // anything counted ?
            size_t  normal  = 0; // normal bases
            size_t *mucount = const_cast<size_t*>(count);

            static const unsigned char normalChars[] = "aAcCgGtTuU";
            static Base normalBase[] = { BC_A, BC_A, BC_C, BC_C, BC_G, BC_G, BC_T, BC_T, BC_T, BC_T };

            // count standard bases
            for (unsigned i = 0; normalChars[i]; ++i) {
                size_t bp               = cc.getCount(normalChars[i]);
                mucount[normalBase[i]] += bp;
                normal                 += bp;
            }

            mucount[BC_ALL]   += all;
            mucount[BC_OTHER] += all-normal;

            cc.clear(); // reset counter
        }
    }
}

void BaseCounter::startLineCounter() {
    gi_assert(char_count.isNull());
    char_count = new CharCounter;
}


void BaseCounter::expectEqual(const BaseCounter& other) const {
    // expect counters to be equal or throw error

    catchUpWithLineCounter();
    other.catchUpWithLineCounter();

    checkOverallCounter();
    other.checkOverallCounter();

    for (int i = 0; i<BC_COUNTERS; ++i) {
        if (count[i] != other.count[i]) { // mismatch in counter
            string error = "Base counter mismatch";
            error        = error+"\n  "+source+"<>"+other.source;

            for (; i<BC_COUNTERS; ++i) {
                if (count[i] != other.count[i]) { // mismatch in counter
                    string whichCounter;

                    if (i == BC_OTHER)      whichCounter = "other";
                    else if (i == BC_ALL)   whichCounter = "overall";
                    else                    whichCounter = "ACGT"[i];

                    error = error+"\n  "+whichCounter+": "+GBS_global_string("%zu <> %zu", count[i], other.count[i]);
                }
            }

            throw error;
        }
    }
}

// --------------------------------------------------------------------------------

SequenceBuffer::~SequenceBuffer() {
    delete [] seq;
}

const char *SequenceBuffer::getSequence() const {
    gi_assert(!seq);            // don't call twice!
    if (!seq) {
        size_t len = baseCounter.getCount(BC_ALL);
        seq        = new char[len+1];

        char              *sp = seq;
        stringVectorCIter  e  = lines.end();

        for (stringVectorCIter i = lines.begin(); i != e; ++i) {
            size_t ilen  = i->length();
            memcpy(sp, i->c_str(), ilen);
            sp         += ilen;
        }

#if defined(DEBUG)
        size_t stored = sp-seq;
        gi_assert(stored == len);
#endif // DEBUG

        seq[len] = 0;
    }
    return seq;
}


