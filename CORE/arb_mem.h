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
};

template<class TYPE>
inline void ARB_alloc_aligned(TYPE*& tgt, size_t nelems) {
    /*! allocate 16 byte aligned memory (terminate on failure) */
    arb_mem::alloc_aligned((void**)&tgt, 16, nelems * sizeof(TYPE));
}

template<class TYPE>
inline void ARB_realloc(TYPE*& tgt, size_t nelem) {
    /*! reallocate memoryblock to fit 'nelem' entries (terminate on failure) */
    tgt = (TYPE*)realloc(tgt, nelem*sizeof(TYPE));
    if (!tgt) arb_mem::failed_to_allocate(nelem, sizeof(TYPE));
}
template<class TYPE>
inline void ARB_recalloc(TYPE*& tgt, size_t oelem, size_t nelem) {
    /*! reallocate memoryblock to fit 'nelem' entries (partially cleared if 'oelem<nelem'; terminate on failure) */
    ARB_realloc(tgt, nelem);
    if (nelem>oelem) memset(tgt+oelem, 0, (nelem-oelem)*sizeof(TYPE));
}

__ATTR__DEPRECATED("use ARB_alloc") inline void *ARB_alloc_ELIM(size_t size) {
    void *mem = malloc(size);
    if (!mem) arb_mem::failed_to_allocate(size);
    return mem;
}
template<class TYPE>
inline TYPE *ARB_alloc(size_t nelem) {
    /*! malloc memory for an array of 'nelem' instances of TYPE (terminates on failure).
     * @code
     * int *p = ARB_alloc<int>(7); // allocates space for 7 int
     * @endcode
     * @see ARB_alloc
     */

    TYPE *mem = (TYPE*)malloc(nelem*sizeof(TYPE));
    if (!mem) arb_mem::failed_to_allocate(nelem, sizeof(TYPE));
    return mem;
}
template<class TYPE>
inline void ARB_alloc(TYPE*& tgt, size_t nelem) {
    /*! malloc memory for an array of 'nelem' instances of TYPE and assign its address to 'tgt' (terminates on failure).
     * @code
     * int *p;
     * ARB_alloc(p, 7); // allocates space for 7 int
     * @endcode
     * @see ARB_alloc<TYPE>
     */
    tgt = ARB_alloc<TYPE>(nelem);
}

template<class TYPE>
inline TYPE *ARB_calloc(size_t nelem) {
    /*! calloc memory for an array of 'nelem' instances of TYPE (terminates on failure).
     * @code
     * int *p = ARB_calloc<int>(7); // allocates space for 7 int (initializes memory with 0)
     * @endcode
     * @see ARB_calloc
     */

    TYPE *mem = (TYPE*)calloc(nelem, sizeof(TYPE));
    if (!mem) arb_mem::failed_to_allocate(nelem, sizeof(TYPE));
    return mem;
}
template<class TYPE>
inline void ARB_calloc(TYPE*& tgt, size_t nelem) {
    /*! calloc memory for an array of 'nelem' instances of TYPE and assign its address to 'tgt' (terminates on failure).
     * @code
     * int *p;
     * ARB_calloc(p, 7); // allocates space for 7 int (initializes memory with 0)
     * @endcode
     * @see ARB_calloc<TYPE>
     */
    tgt = ARB_calloc<TYPE>(nelem);
}

#else
#error arb_mem.h included twice
#endif // ARB_MEM_H
