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
#ifndef _GLIBCXX_CSTRING
#include <cstring>
#endif
#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif

namespace arb_mem {
    void failed_to_allocate(const char *reason) __ATTR__NORETURN;
    void failed_to_allocate(size_t nelem, size_t elsize) __ATTR__NORETURN;
    void failed_to_allocate(size_t size) __ATTR__NORETURN;

    inline void alloc_aligned(void **tgt, size_t alignment, size_t len) {
        int error = posix_memalign(tgt, alignment, len);
        if (error) failed_to_allocate(strerror(error));
    }

    inline void re_alloc(void **tgt, size_t nelem, size_t elsize) {
        size_t nsize = nelem*elsize;

        void *mem = realloc(*tgt, nsize);
        if (!mem) arb_mem::failed_to_allocate(nelem, elsize);

        *tgt = mem;
    }
    inline void re_calloc(void **tgt, size_t oelem, size_t nelem, size_t elsize) {
        size_t nsize = nelem*elsize;

        void *mem = realloc(*tgt, nsize);
        if (!mem) arb_mem::failed_to_allocate(nelem, elsize);

        size_t osize = oelem*elsize;
        if (nsize>osize) memset(((char*)mem)+osize, 0, nsize-osize);

        *tgt = mem;
    }
};

template<class TYPE>
inline void ARB_alloc_aligned(TYPE*& tgt, size_t nelems) {
    /*! allocate 16 byte aligned memory (terminate on failure) */
    arb_mem::alloc_aligned((void**)&tgt, 16, nelems * sizeof(TYPE));
}

template<class TYPE>
inline void ARB_realloc(TYPE*& tgt, size_t nelem) {
    /*! reallocate memoryblock to fit 'nelem' entries (terminate on failure) */
    arb_mem::re_alloc((void**)&tgt, nelem, sizeof(TYPE));
}
template<class TYPE>
inline void ARB_recalloc(TYPE*& tgt, size_t oelem, size_t nelem) {
    /*! reallocate memoryblock to fit 'nelem' entries (partially cleared if 'oelem<nelem'; terminate on failure) */
    if (oelem != nelem) {
        arb_mem::re_calloc((void**)&tgt, oelem, nelem, sizeof(TYPE));
    }
}

inline void *ARB_alloc(size_t size) {
    void *mem = malloc(size);
    if (!mem) arb_mem::failed_to_allocate(size);
    return mem;
}
inline void *ARB_calloc(size_t nelem, size_t elsize) {
    void *mem = calloc(nelem, elsize);
    if (!mem) arb_mem::failed_to_allocate(nelem, elsize);
    return mem;
}

#else
#error arb_mem.h included twice
#endif // ARB_MEM_H
