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
    COMPILE_ASSERT(sizeof(data)           == 4);
    COMPILE_ASSERT(sizeof(data.as_uint32) == 4);

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

inline void gb_put_number(long i, FILE *out) {
    // opposite of gb_get_number
    long j;
    if (i< 0x80) { putc((int)i, out); return; }
    if (i<0x4000) {
        j = (i>>8) | 0x80;
        putc((int)j, out);
        putc((int)i, out);
        return;
    }
    if (i<0x200000) {
        j = (i>>16) | 0xC0;
        putc((int)j, out);
        j = (i>>8);
        putc((int)j, out);
        putc((int)i, out);
        return;
    }
    if (i<0x10000000) {
        j = (i>>24) | 0xE0;
        putc((int)j, out);
        j = (i>>16);
        putc((int)j, out);
        j = (i>>8);
        putc((int)j, out);
        putc((int)i, out);
        return;
    }
    // gb_assert(0); // overflow
    return;
}

inline long gb_get_number(FILE *in) {
    // opposite of gb_put_number
    unsigned int c0 = getc(in);
    if (c0 & 0x80) {
        unsigned int c1 = getc(in);
        if (c0 & 0x40) {
            unsigned int c2 = getc(in);
            if (c0 & 0x20) {
                unsigned int c3 = getc(in);
                if (c0 &0x10) {
                    unsigned int c4 = getc(in);
                    return c4 | (c3<<8) | (c2<<16) | (c1<<8);
                }
                else {
                    return (c3) | (c2<<8) | (c1<<16) | ((c0 & 0x0f)<<24);
                }
            }
            else {
                return (c2) | (c1<<8) | ((c0 & 0x1f)<<16);
            }
        }
        else {
            return (c1) | ((c0 & 0x3f)<<8);
        }
    }
    else {
        return c0;
    }
}

#else
#error ad_io_inline.h included twice
#endif // AD_IO_INLINE_H
