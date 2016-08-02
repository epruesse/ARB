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

static char panicBuffer[500];

static void alloc_failure_advice() {
    fputs("\n"
          "To avoid memory allocation problems\n"
          " - increase your swap space (see 'man 8 mkswap'),\n"
          " - reduce the amount of data (used for the failing operation)\n"
          " - or buy more memory.\n"
          "\n"
          "The program will terminate now (reason below).\n",
          stderr);
}

void arb_mem::failed_to_allocate(const char *reason) {
    alloc_failure_advice();
    sprintf(panicBuffer, "Failed to allocate memory: %s", reason);
    GBK_terminate(panicBuffer);
}
void arb_mem::failed_to_allocate(unsigned int nelem, unsigned int elsize) {
    alloc_failure_advice();
    sprintf(panicBuffer, "Failed to allocate memory (tried to get %u*%u bytes)", nelem, elsize);
    GBK_terminate(panicBuffer);
}

void arb_mem::alloc_aligned(void** tgt, size_t len) {
    int error = posix_memalign(tgt, 16, len);
    if (error) failed_to_allocate(strerror(error));
}

void *ARB_calloc(size_t nelem, size_t elsize) { // @@@ move to header?
    void *mem = calloc(nelem, elsize);
    if (!mem) arb_mem::failed_to_allocate(nelem, elsize);
    return mem;
}

void *ARB_recalloc(void *ptr, size_t oelem, size_t nelem, size_t elsize) {
    size_t  nsize = nelem*elsize;
    void   *mem   = malloc(nsize);
    if (!mem) arb_mem::failed_to_allocate(nelem, elsize);

    size_t osize = oelem*elsize;
    if (nsize>osize) {
        memmove(mem, ptr, osize);
        memset(((char*)mem)+osize, 0, nsize-osize);
    }
    else {
        memmove(mem, ptr, nsize);
    }
    free(ptr);
    return mem;
}
