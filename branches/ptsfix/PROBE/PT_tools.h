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

typedef unsigned char  uint_8;
typedef unsigned short uint_16;
typedef unsigned int   uint_32;
typedef unsigned long  uint_64;

#if defined(ARB_64)
typedef uint_64 uint_big;
#else
typedef uint_32 uint_big;
#endif

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


// ----------------------------
//      read/write numbers

inline uint_8   PT_read_char (const void *fromMem)  { return *(uint_8*)fromMem; }
inline uint_16  PT_read_short(const void *fromMem)  { return bswap_16(*(uint_16*)fromMem); }
inline uint_32  PT_read_int  (const void *fromMem)  { return bswap_32(*(uint_32*)fromMem); }
#if defined(ARB_64)                                
inline uint_64  PT_read_long (const void *fromMem)  { return bswap_64(*(uint_64*)fromMem); }
inline uint_big PT_read_big  (const void *fromMem)  { return PT_read_long(fromMem); }
#else
inline uint_big PT_read_big  (const void *fromMem)  { return PT_read_int(fromMem); }
#endif

inline void PT_write_char (void *toMem, uint_8  i)  { *(uint_8*) toMem = i; }
inline void PT_write_short(void *toMem, uint_16 i)  { *(uint_16*)toMem = bswap_16(i); }
inline void PT_write_int  (void *toMem, uint_32 i)  { *(uint_32*)toMem = bswap_32(i); }
#if defined(ARB_64)
inline void PT_write_long (void *toMem, uint_64 i)  { *(uint_64*)toMem = bswap_64(i); }
inline void PT_write_big  (void *toMem, uint_big i) { PT_write_long(toMem, i); }
#else
inline void PT_write_big  (void *toMem, uint_big i) { PT_write_int(toMem, i); }
#endif

// ------------------------------------------------
//      compressed read/write positive numbers

#define PT_WRITE_NAT(ptr, i)                            \
    do {                                                \
        pt_assert((i) >= 0);                            \
        if ((i) >= 0x7FFE) {                            \
            PT_write_int((ptr), (i)|0x80000000);        \
            (ptr) += sizeof(int);                       \
        }                                               \
        else {                                          \
            PT_write_short((ptr), (i));                 \
            (ptr) += sizeof(short);                     \
        }                                               \
    } while (0)

#define PT_READ_NAT(ptr, i)                     \
    do {                                        \
        if (*(ptr) & 0x80) {                    \
            (i)    = PT_read_int(ptr);          \
            (ptr) += sizeof(int);               \
            (i)   &= 0x7fffffff;                \
        }                                       \
        else {                                  \
            (i)    = PT_read_short(ptr);        \
            (ptr) += sizeof(short);             \
        }                                       \
    } while (0)


// -----------------------------
//      read/write pointers


COMPILE_ASSERT(sizeof(void*) == sizeof(uint_big));

inline void *PT_read_void_pointer(const void *fromMem) { return (void*)PT_read_big(fromMem); }
inline void PT_write_pointer(void *toMem, const void *thePtr) { PT_write_big(toMem, (uint_big)thePtr); }

template<typename POINTED>
inline POINTED* PT_read_pointer(const void *fromMem) { return (POINTED*)PT_read_void_pointer(fromMem); }

inline void fflush_all() {
    fflush(stderr);
    fflush(stdout);
}


#else
#error PT_tools.h included twice
#endif // PT_TOOLS_H
