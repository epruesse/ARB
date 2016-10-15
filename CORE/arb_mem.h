// ================================================================= //
//                                                                   //
//   File      : arb_mem.h                                           //
//   Purpose   :                                                     //
//                                                                   //
//   Coded by Elmar Pruesse in September 2014                        //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef ARB_MEM_H
#define ARB_MEM_H

#ifndef _GLIBCXX_CSTDLIB
#include <cstdlib>
#endif

int arb_alloc_aligned_intern(void **tgt, size_t len);

/* allocate 16 byte aligned memory and export error on failure */
template<class TYPE>
inline int arb_alloc_aligned(TYPE*& tgt, size_t len) { 
    return arb_alloc_aligned_intern((void**)&tgt, len * sizeof(TYPE));
}

#else
#error arb_mem.h included twice
#endif // ARB_MEM_H
