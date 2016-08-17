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

typedef unsigned char  uint_8;  // @@@ use uint8_t, uint16_t, uint32_t, uint64_t here
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

// Note: number of least significant bit is 0 (in template parameters)

// compile-time generation of mask-values for bit B = [0..31]
template <int B> struct BitMask { enum { value = (1<<B) }; };

// compile-time generation of mask-values for all bits below bit B = [0..32]
template <int B> struct BitsBelowBit { enum { value = (BitMask<B>::value-1) }; };
template <> struct BitsBelowBit<32> { enum { value = 0xffffffff }; }; // avoid shift overflow
template <> struct BitsBelowBit<31> { enum { value = 0x7fffffff }; }; // avoid int overflow

// compile-time generation of mask-values for all bits in range [LB..HB]
// 
template <int HB, int LB> struct BitRange { enum { value = (BitsBelowBit<HB+1>::value-BitsBelowBit<LB>::value) }; };


template <int R> // R is number of reserved bits
inline void write_nat_with_reserved_bits(char*& toMem, uint_32 nat, uint_8 reserved_bits) {
    pt_assert((reserved_bits&BitsBelowBit<R>::value) == reserved_bits); // reserved_bits exceed allowed value-range
    reserved_bits = reserved_bits << (8-R);
    if (nat < BitMask<7-R>::value) {
        PT_write_char(toMem, nat|reserved_bits);
        toMem += 1;
    }
    else {
        nat -= BitMask<7-R>::value;
        if (nat < BitMask<6-R+8>::value) {
            PT_write_short(toMem, nat|BitMask<7-R+8>::value|(reserved_bits<<8));
            toMem += 2;
        }
        else {
            nat -= BitMask<6-R+8>::value;
            if (nat < BitMask<5-R+16>::value) {
                PT_write_char(toMem, (nat>>16)|BitRange<7-R,6-R>::value|reserved_bits);
                PT_write_short(toMem+1, nat&0xffff);
                toMem += 3;
            }
            else {
                nat -= BitMask<5-R+16>::value;
                if (nat < BitMask<4-R+24>::value) {
                    PT_write_int(toMem, nat|BitRange<7-R+24,5-R+24>::value|(reserved_bits<<24));
                    toMem += 4;
                }
                else {
                    PT_write_char(toMem, BitRange<7-R,4-R>::value|reserved_bits);
                    PT_write_int(toMem+1, nat);
                    toMem += 5;
                }
            }
        }
    }
}

template <int R> // R is number of reserved bits
inline uint_32 read_nat_with_reserved_bits(const char*& fromMem, uint_8& reserved_bits) {
    uint_32 nat   = PT_read_char(fromMem);
    reserved_bits = nat >> (8-R);
    nat           = nat & BitsBelowBit<8-R>::value;

    if (nat & BitMask<7-R>::value) {
        if (nat & BitMask<6-R>::value) {
            if (nat & BitMask<5-R>::value) {
                if (nat & BitMask<4-R>::value) { // 5 bytes
                    nat      = PT_read_int(fromMem+1);
                    fromMem += 5;
                }
                else { // 4 bytes
                    nat      = PT_read_int(fromMem) & (BitMask<4-R+24>::value-1);
                    fromMem += 4;
                }
                nat += BitMask<5-R+16>::value;
            }
            else { // 3 bytes
                nat      = ((nat&(BitMask<6-R>::value-1))<<16)|PT_read_short(fromMem+1);
                fromMem += 3;
            }
            nat += BitMask<6-R+8>::value;
        }
        else { // 2 bytes
            nat      = PT_read_short(fromMem) & (BitMask<7-R+8>::value-1);
            fromMem += 2;
        }
        nat += BitMask<7-R>::value;
    }
    else { // 1 byte
        ++fromMem;
    }
    return nat;
}

template <int R>
inline void write_int_with_reserved_bits(char*& toMem, int32_t i, uint_8 reserved_bits) {
    write_nat_with_reserved_bits<R+1>(toMem, i<0 ? -i-1 : i, (reserved_bits<<1)|(i<0));
}
template <int R>
inline int32_t read_int_with_reserved_bits(const char*& fromMem, uint_8& reserved_bits) {
    uint_32 nat   = read_nat_with_reserved_bits<R+1>(fromMem, reserved_bits);
    bool    isNeg = reserved_bits&1;
    reserved_bits = reserved_bits>>1;
    return isNeg ? -nat-1 : nat;
}

// read/write uint_32 using variable amount of mem (small numbers use less space than big numbers)
inline uint_32 PT_read_compact_nat(const char*& fromMem) { uint_8 nothing; return read_nat_with_reserved_bits<0>(fromMem, nothing); }
inline void PT_write_compact_nat(char*& toMem, uint_32 nat) { write_nat_with_reserved_bits<0>(toMem, nat, 0); }

// -----------------------------
//      read/write pointers


STATIC_ASSERT(sizeof(void*) == sizeof(uint_big));

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
