// =============================================================== //
//                                                                 //
//   File      : PT_tools.h                                        //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2012   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef PT_TOOLS_H
#define PT_TOOLS_H

#ifndef STATIC_ASSERT_H
#include <static_assert.h>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

#define pt_assert(bed) arb_assert(bed)

typedef void * PT_PNTR;

// ----------------------
//      bswap for OSX

#if defined(DARWIN)

static inline unsigned short bswap_16(unsigned short x) {
    return (x>>8) | (x<<8);
}

static inline unsigned int bswap_32(unsigned int x) {
    return (bswap_16(x&0xffff)<<16) | (bswap_16(x>>16));
}

static inline unsigned long long bswap_64(unsigned long long x) {
    return (((unsigned long long)bswap_32(x&0xffffffffull))<<32) | (bswap_32(x>>32));
}

#else
#include <byteswap.h>
#endif // DARWIN

// ------------------------------------------------------------
// Note about bswap as used here:
//
// * MSB has to be at start of written byte-chain, cause the most significant bit is used to separate
//   between INT and SHORT
//
// * To use PT-server on a big-endian system it has to be skipped
// ------------------------------------------------------------

#if defined(ARB_64)

# define PT_READ_LONG(ptr, my_int_i)                            \
    do {                                                        \
        COMPILE_ASSERT(sizeof(my_int_i) == 8);                  \
        unsigned long *ulptr = (unsigned long*)(ptr);           \
        (my_int_i)           = (unsigned long)bswap_64(*ulptr); \
    } while (0)


# define PT_WRITE_LONG(ptr, my_int_i)                                   \
    do {                                                                \
        unsigned long *ulptr = (unsigned long*)(ptr);                   \
        *ulptr               = bswap_64((unsigned long)(my_int_i));     \
    } while (0)

#endif

// ---------------------------------
//      Read and write to memory

#define PT_READ_INT(ptr, my_int_i)                                      \
    do {                                                                \
        COMPILE_ASSERT(sizeof(my_int_i) >= 4);          \
        unsigned int *uiptr = (unsigned int*)(ptr);                     \
        (my_int_i)=(unsigned int)bswap_32(*uiptr);                      \
    } while (0)

#define PT_WRITE_INT(ptr, my_int_i)                                     \
    do {                                                                \
        unsigned int *uiptr = (unsigned int*)(ptr);                     \
        *uiptr              = bswap_32((unsigned int)(my_int_i));       \
    } while (0)

#define PT_READ_SHORT(ptr, my_int_i)                    \
    do {                                                \
        COMPILE_ASSERT(sizeof(my_int_i) >= 2);          \
        (my_int_i) = bswap_16(*(unsigned short*)(ptr)); \
    } while (0)

#define PT_WRITE_SHORT(ptr, my_int_i)                                   \
    do {                                                                \
        unsigned short *usptr = (unsigned short*)(ptr);                 \
        *usptr                = bswap_16((unsigned short)(my_int_i));   \
    } while (0)

#define PT_READ_CHAR(ptr, my_int_i)             \
    do {                                        \
        (my_int_i) = *(unsigned char *)(ptr);   \
    } while (0)

#define PT_WRITE_CHAR(ptr, my_int_i)            \
    do {                                        \
        *(unsigned char *)(ptr) = (my_int_i);   \
    } while (0)


// ------------------------------------------------
//      compressed read/write positive numbers

#define PT_WRITE_NAT(ptr, i)                    \
    do {                                        \
        pt_assert(i >= 0);                      \
        if (i >= 0x7FFE)                        \
        {                                       \
            PT_WRITE_INT(ptr, i|0x80000000);    \
            ptr += sizeof(int);                 \
        }                                       \
        else                                    \
        {                                       \
            PT_WRITE_SHORT(ptr, i);             \
            ptr += sizeof(short);               \
        }                                       \
    } while (0)

#define PT_READ_NAT(ptr, i)                                             \
    do {                                                                \
        if (*ptr & 0x80) {                                              \
            PT_READ_INT(ptr, i); ptr += sizeof(int); i &= 0x7fffffff;   \
        }                                                               \
        else {                                                          \
            PT_READ_SHORT(ptr, i); ptr += sizeof(short);                \
        }                                                               \
    } while (0)


// -----------------------------
//      read/write pointers


#if defined(ARB_64)
COMPILE_ASSERT(sizeof(void*) == sizeof(unsigned long));
# define PT_READ_PNTR(ptr,my_int_i)  PT_READ_LONG(ptr, my_int_i)
# define PT_WRITE_PNTR(ptr,my_int_i) PT_WRITE_LONG(ptr, my_int_i)
#else // !defined(ARB_64)
COMPILE_ASSERT(sizeof(void*) == sizeof(unsigned int));
# define PT_READ_PNTR(ptr,my_int_i)  PT_READ_INT(ptr, my_int_i)
# define PT_WRITE_PNTR(ptr,my_int_i) PT_WRITE_INT(ptr, my_int_i)
#endif

#else
#error PT_tools.h included twice
#endif // PT_TOOLS_H
