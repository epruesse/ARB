// ============================================================= //
//                                                               //
//   File      : ad_io_inline.h                                  //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in April 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef AD_IO_INLINE_H
#define AD_IO_INLINE_H

#ifndef STATIC_ASSERT_H
#include <static_assert.h>
#endif

inline void swap(unsigned char& c1, unsigned char& c2) { unsigned char c = c1; c1 = c2; c2 = c; }

inline uint32_t reverse_byteorder(uint32_t val) {
    union {
        uint32_t      as_uint32;
        unsigned char as_char[4];
    } data;
    STATIC_ASSERT(sizeof(data)           == 4);
    STATIC_ASSERT(sizeof(data.as_uint32) == 4);

    data.as_uint32 = val;

    swap(data.as_char[0], data.as_char[3]);
    swap(data.as_char[1], data.as_char[2]);

    return data.as_uint32;
}

inline void gb_write_out_uint32(uint32_t data, FILE *out) {
    // opposite of gb_read_in_uint32
    ASSERT_RESULT(size_t, 1, fwrite(&data, sizeof(data), 1, out));
}

inline uint32_t gb_read_in_uint32(FILE *in, bool reversed) {
    // opposite of gb_write_out_uint32
    uint32_t val;
    ASSERT_RESULT(size_t, 1, fread(&val, sizeof(val), 1, in));
    return reversed ? reverse_byteorder(val) : val;
}

inline void gb_put_number(long b0, FILE *out) {
    // opposite of gb_get_number

    if (b0 < 0) {
        // if this happens, we are in 32/64bit-hell
        // if it never fails, gb_put_number/gb_get_number should better work with uint32_t
        // see also ad_load.cxx@bit-hell
        GBK_terminate("32/64bit incompatibility detected in DB-engine, please inform devel@arb-home.de");
    }
    
    typedef unsigned char uc;
    if (b0 >= 0x80) {
        long b1 = b0>>8;
        if (b1 >= 0x40) {
            long b2 = b1>>8;
            if (b2 >= 0x20) {
                long b3 = b2>>8;
                if (b3 >= 0x10) putc(0xf0, out);
                else b3 |= 0xE0;
                putc(uc(b3), out);
            }
            else b2 |= 0xC0;
            putc(uc(b2), out);
        }
        else b1 |= 0x80;
        putc(uc(b1), out);
    }
    putc(uc(b0), out);
}

inline long gb_get_number(FILE *in) {
    // opposite of gb_put_number
    unsigned int b0  = getc(in);
    unsigned int RES = b0;
    if (b0 & 0x80) {
        RES = getc(in);
        if (b0 & 0x40) {
            RES = (RES << 8) | getc(in);
            if (b0 & 0x20) {
                RES = (RES << 8) | getc(in);
                if (b0 & 0x10) RES = (RES << 8) | getc(in);
                else RES |= (b0 & 0x0f) << 24;
            }
            else RES |= (b0 & 0x1f) << 16;
        }
        else RES |= (b0 & 0x3f) << 8;
    }
    return RES;
}


#else
#error ad_io_inline.h included twice
#endif // AD_IO_INLINE_H
