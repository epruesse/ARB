// =============================================================== //
//                                                                 //
//   File      : gb_hashindex.h                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GB_HASHINDEX_H
#define GB_HASHINDEX_H

#ifndef GB_LOCAL_H
#include "gb_local.h"
#endif

// -------------------------------
//      hash index calculation

extern const uint32_t crctab[];
//#define USE_DJB2_HASH
#define USE_ARB_CRC_HASH
#ifdef USE_ARB_CRC_HASH

#define GB_CALC_HASH_INDEX_CASE_SENSITIVE(string, index, size) do {     \
        const char *local_ptr = (string);                               \
        int local_i;                                                    \
        (index) = 0xffffffffL;                                          \
        while ((local_i=(*(local_ptr++)))) {                            \
            (index) = crctab[((int)(index)^local_i) & 0xff] ^ ((index) >> 8); \
        }                                                               \
        (index) = (index) % (size);                                     \
    } while (0)

#define GB_CALC_HASH_INDEX_CASE_IGNORED(string, index, size) do {       \
        const char *local_ptr = (string);                               \
        int local_i;                                                    \
        (index) = 0xffffffffL;                                          \
        while ((local_i = *(local_ptr++))) {                            \
            (index) = crctab[((int) (index) ^ toupper(local_i)) & 0xff] ^ ((index) >> 8); \
        }                                                               \
        (index) = (index) % (size);                                     \
    } while (0)

#elif defined(USE_DJB2_HASH)

#define GB_CALC_HASH_INDEX_CASE_SENSITIVE(string, index, size) do {     \
        const char *local_ptr = (string);                               \
        (index) = 5381;                                                 \
        int local_i;                                                    \
        while ((local_i = *(local_ptr++))) {                            \
            (index) = (((index) << 5) + (index))+ local_i;              \
        }                                                               \
        (index) = (index) % (size);                                     \
    } while (0)

#define GB_CALC_HASH_INDEX_CASE_IGNORED(string, index, size) do {       \
        const char *local_ptr = (string);                               \
        int local_i;                                                    \
        (index) = 5381;                                                 \
        while ((local_i = *(local_ptr++))) {                            \
            (index) = (((index) << 5) + (index))+ toupper(local_i);     \
        }                                                               \
        (index) = (index) % (size);                                     \
    } while (0)

#endif

#define GB_CALC_HASH_INDEX(string, index, size, caseSens) do {          \
        if ((caseSens) == GB_IGNORE_CASE)                               \
            GB_CALC_HASH_INDEX_CASE_IGNORED(string, index, size);       \
        else                                                            \
            GB_CALC_HASH_INDEX_CASE_SENSITIVE(string, index, size);     \
    } while (0)



#else
#error gb_hashindex.h included twice
#endif // GB_HASHINDEX_H
