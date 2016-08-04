// ================================================================= //
//                                                                   //
//   File      : arb_mem.cxx                                         //
//   Purpose   : "Failsafe" memory handlers                          //
//               ("succeed or terminate"!)                           //
//                                                                   //
//   Coded by Elmar Pruesse and Ralf Westram                         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#include "arb_mem.h"
#include "arb_msg.h"
#include "arb_assert.h"

static char panicBuffer[500] = { 0 };

static __ATTR__NORETURN void alloc_failure_panic() {
    arb_assert(panicBuffer[0]); // sth should have been printed into buffer

    fputs("\n--------------------\n", stderr);
    fputs(panicBuffer, stderr);
    fputs("\n"
          "To avoid memory allocation problems\n"
          " - increase your swap space (see 'man 8 mkswap'),\n"
          " - reduce the amount of data (used for the failing operation)\n"
          " - or buy more memory.\n"
          "\n"
          "The program will terminate now!\n\n",
          stderr);

    GBK_terminate(panicBuffer);
}

void arb_mem::failed_to_allocate(const char *reason) {
    sprintf(panicBuffer, "Failed to allocate memory: %s", reason);
    alloc_failure_panic();
}
void arb_mem::failed_to_allocate(size_t nelem, size_t elsize) {
    sprintf(panicBuffer, "Failed to allocate memory (tried to get %zu*%zu bytes)", nelem, elsize);
    alloc_failure_panic();
}
void arb_mem::failed_to_allocate(size_t size) {
    sprintf(panicBuffer, "Failed to allocate memory (tried to get %zu bytes)", size);
    alloc_failure_panic();
}

void arb_mem::alloc_aligned(void **tgt, size_t alignment, size_t len) {
    int error = posix_memalign(tgt, alignment, len);
    if (error) failed_to_allocate(strerror(error));
}

void arb_mem::re_alloc(void **tgt, size_t oelem, size_t nelem, size_t elsize) {
    size_t nsize = nelem*elsize;

    void *mem = realloc(*tgt, nsize);
    if (!mem) arb_mem::failed_to_allocate(nelem, elsize);

    *tgt = mem;
}
void arb_mem::re_calloc(void **tgt, size_t oelem, size_t nelem, size_t elsize) {
    size_t nsize = nelem*elsize;

    void *mem = realloc(*tgt, nsize);
    if (!mem) arb_mem::failed_to_allocate(nelem, elsize);

    size_t osize = oelem*elsize;
    if (nsize>osize) memset(((char*)mem)+osize, 0, nsize-osize);

    *tgt = mem;
}
