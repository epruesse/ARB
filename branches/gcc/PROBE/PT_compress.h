// ================================================================ //
//                                                                  //
//   File      : PT_compress.h                                      //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2012   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef PT_COMPRESS_H
#define PT_COMPRESS_H

inline size_t count_uint_32(uint32_t *seq, size_t seqsize, uint32_t cmp) {
    size_t count = 0;
    while (count<seqsize && seq[count] == cmp) count++;
    return count*4;
}

inline size_t count_char(const unsigned char *seq, size_t seqsize, unsigned char c, uint32_t c4) {
    if (seqsize>0 && seq[0] == c) {
        size_t count = 1+count_uint_32((uint32_t*)(seq+1), (seqsize-1)/4, c4);
        for (; count<seqsize && seq[count] == c; ++count) ;
        return count;
    }
    return 0;
}

inline size_t count_dots(const unsigned char *seq, int seqsize) { return count_char(seq, seqsize, '.', 0x2E2E2E2E); }
inline size_t count_gaps(const unsigned char *seq, int seqsize) { return count_char(seq, seqsize, '-', 0x2D2D2D2D); }

inline size_t count_gaps_and_dots(const unsigned char *seq, int seqsize) {
    pt_assert(seqsize>0);

    size_t count = 0;
    size_t count2;
    size_t count3;

    do {
        count2  = count_dots(seq+count, seqsize-count);
        count  += count2;
        count3  = count_gaps(seq+count, seqsize-count);
        count  += count3;
    }
    while (count2 || count3);
    return count;
}

// uncomment next line to count all bases compressed by ptserver and dump them when program terminates
// #define COUNT_COMPRESSES_BASES
#if defined(COUNT_COMPRESSES_BASES)
class BaseCounter {
    long count[PT_BASES];
public:
    BaseCounter() {
        for (int i = 0; i<PT_BASES; ++i) {
            count[i] = 0;
        }
    }
    ~BaseCounter() {
        fflush_all();
        fputs("\nBaseCounter:\n", stderr);
        for (int i = 0; i<PT_BASES; ++i) {
            fprintf(stderr, "count[%i]=%li\n", i, count[i]);
        }
    }

    void inc(uchar base) {
        pt_assert(base<PT_BASES);
        ++count[base];
    }
};
#endif

typedef unsigned char uchar;

class PT_compressed : virtual Noncopyable {
    size_t    max_input_length;
    uchar    *compressed;
    unsigned *base_offset; // to previous base
    size_t    size;

    static bool  translation_initialized;
    static uchar translate[256];

#if defined(COUNT_COMPRESSES_BASES)
    static BaseCounter base_counter;
#endif

    static void initTranslation() {
        memset(translate, PT_N, 256);

        translate['A'] = translate['a'] = PT_A;
        translate['C'] = translate['c'] = PT_C;
        translate['G'] = translate['g'] = PT_G;
        translate['T'] = translate['t'] = PT_T;
        translate['U'] = translate['u'] = PT_T;
        translate['.'] = PT_QU;

        translate[0] = PT_B_UNDEF;

        translation_initialized = true;
    }

public:
    PT_compressed(size_t max_input_length_)
        : max_input_length(max_input_length_),
          compressed(new uchar[max_input_length+1]),
          base_offset(new unsigned[max_input_length+1]),
          size(0)
    {
        if (!translation_initialized) initTranslation();
    }
    ~PT_compressed() {
        delete [] compressed;
        delete [] base_offset;
    }

    void createFrom(const unsigned char * const seq, const size_t length) {
        pt_assert(length <= max_input_length);

        size_t base_count       = 0;
        size_t last_base_offset = 0;

        uchar c = PT_N; // just not PT_QU
        for (size_t offset = 0; offset<length; ++offset) {
            if (c == PT_QU) {
                offset += count_gaps_and_dots(seq + offset, length - offset); // skip over gaps and dots (ignoring multiple dots)
            }
            else {
                offset += count_gaps(seq + offset, length - offset); // skip over gaps only (stops at next dot)
            }

            if (offset >= length) break;

            c = translate[seq[offset]];
            pt_assert(c != PT_B_UNDEF); // e.g. hit end of sequence

#if defined(COUNT_COMPRESSES_BASES)
            base_counter.inc(c);
#endif
            compressed[base_count] = c;
            base_offset[base_count++] = offset-last_base_offset;
            last_base_offset = offset;
        }

        if (base_count <= 0) { // may happen if empty (or gap-only) input sequence is passed
            size = 0;
            return;
        }

        // ensure last entry is a dot
        if (compressed[base_count-1] != PT_QU) {
#if defined(COUNT_COMPRESSES_BASES)
            base_counter.inc(PT_QU);
#endif
            compressed[base_count]  = PT_QU;
            base_offset[base_count] = 1; // one position behind prev base
            ++base_count;
        }

        // if first entry is a dot, reposition it before 1st non-dot-base
        if (compressed[0] == PT_QU && base_count>1) {
            pt_assert(compressed[1] != PT_QU); // many dots should always compress into 1 dot
            base_offset[0] = base_offset[1]-1;
            base_offset[1] = 1;
        }

        size = base_count;
        pt_assert(size <= (length+1));
#ifdef ARB_64
        pt_assert(!(size & 0xffffffff00000000)); // must fit into 32 bit
#endif

    }
    void createFrom(const char * const seq, const size_t length) {
        createFrom(reinterpret_cast<const unsigned char * const>(seq), length);
    }

    size_t get_size() const { return size; }
    const char *get_seq() const { return reinterpret_cast<const char *>(compressed); }
    const unsigned *get_offsets() const { return base_offset; }
};

#else
#error PT_compress.h included twice
#endif // PT_COMPRESS_H
