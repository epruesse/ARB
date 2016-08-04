// ================================================================= //
//                                                                   //
//   File      : arb_mem.h                                           //
//   Purpose   : "Failsafe" memory handlers                          //
//               ("succeed or terminate"!)                           //
//                                                                   //
//   Coded by Elmar Pruesse and Ralf Westram                         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef ARB_MEM_H
#define ARB_MEM_H

#ifndef _GLIBCXX_CSTDLIB
#include <cstdlib>
#endif
#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif

namespace arb_mem {
    void failed_to_allocate(const char *reason) __ATTR__NORETURN;
    void failed_to_allocate(size_t nelem, size_t elsize) __ATTR__NORETURN;
    void failed_to_allocate(size_t size) __ATTR__NORETURN;

    void alloc_aligned(void **tgt, size_t alignment, size_t len);
};

template<class TYPE>
inline void ARB_alloc_aligned(TYPE*& tgt, size_t nelems) {
    /*! allocate 16 byte aligned memory (terminates on failure) */
    arb_mem::alloc_aligned((void**)&tgt, 16, nelems * sizeof(TYPE));
}

inline void *ARB_alloc(size_t size) { // @@@ replace all uses of malloc with ARB_alloc
    void *mem = malloc(size);
    if (!mem) arb_mem::failed_to_allocate(size);
    return mem;
}
// @@@ add here: ARB_realloc

inline void *ARB_calloc(size_t nelem, size_t elsize) {
    void *mem = calloc(nelem, elsize);
    if (!mem) arb_mem::failed_to_allocate(nelem, elsize);
    return mem;
}
void *ARB_recalloc(void *ptr, size_t oelem, size_t nelem, size_t elsize); // @@@ make this a template (similar to ARB_alloc_aligned)

// ------------------------
//      compat helpers

inline void *GB_calloc(size_t nelem, size_t elsize) { // @@@ inline callers
    return ARB_calloc(nelem, elsize);
}
inline void *GB_recalloc(void *ptr, size_t oelem, size_t nelem, size_t elsize) { // @@@ inline callers; use template similar to ARB_alloc_aligned
    return ARB_recalloc(ptr, oelem, nelem, elsize);
}

#else
#error arb_mem.h included twice
#endif // ARB_MEM_H
