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

void arb_mem::re_alloc(void **tgt, size_t nelem, size_t elsize) {
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


// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

static void alloc_too_much() { ARB_alloc(-1); }
static void calloc_too_much() { ARB_calloc(-1, 1); }
static void realloc_too_much() { char *s = 0; ARB_realloc(s, -1); }
static void recalloc_too_much() { char *s = 0; ARB_recalloc(s, 0, -1); }

static bool mem_is_cleared(const char *mem, size_t size) {
    for (size_t s = 0; s<size; ++s) {
        if (mem[s]) return false;
    }
    return true;
}

void TEST_allocators() {
    const int  SIZE  = 100;
    const int  SIZE2 = 200;
    char      *s     = NULL;                   TEST_EXPECT_NULL(s);

    s = (char*)ARB_alloc(0);                   TEST_REJECT_NULL((void*)s); // allocating empty block != NULL
    freeset(s, (char*)ARB_alloc(SIZE));        TEST_REJECT_NULL((void*)s);

    freenull(s);                               TEST_EXPECT_NULL(s);

    ARB_realloc(s, 0);                         TEST_REJECT_NULL((void*)s);
    ARB_realloc(s, SIZE);                      TEST_REJECT_NULL((void*)s);
    // ARB_realloc(s, 0);                         TEST_REJECT_NULL(s); // fails

    freenull(s);                               TEST_EXPECT_NULL(s);

    s = (char*)ARB_calloc(0, 1);               TEST_REJECT_NULL((void*)s);
    freeset(s, (char*)ARB_calloc(SIZE, 1));    TEST_REJECT_NULL((void*)s); TEST_EXPECT(mem_is_cleared(s, SIZE));

    freenull(s);                               TEST_EXPECT_NULL(s);

    ARB_recalloc(s, 0, 1);                     TEST_REJECT_NULL((void*)s); TEST_EXPECT(mem_is_cleared(s, 1));
    ARB_recalloc(s, 1, SIZE);                  TEST_REJECT_NULL((void*)s); TEST_EXPECT(mem_is_cleared(s, SIZE));
    ARB_recalloc(s, SIZE, SIZE2);              TEST_REJECT_NULL((void*)s); TEST_EXPECT(mem_is_cleared(s, SIZE2));
    ARB_recalloc(s, SIZE2, SIZE);              TEST_REJECT_NULL((void*)s); TEST_EXPECT(mem_is_cleared(s, SIZE));
    ARB_recalloc(s, SIZE, 1);                  TEST_REJECT_NULL((void*)s); TEST_EXPECT(mem_is_cleared(s, 1));
    // ARB_recalloc(s, 1, 0);                     TEST_REJECT_NULL(s); // fails

    freenull(s);                               TEST_EXPECT_NULL(s);

#if !defined(LEAKS_SANITIZED)
    // test out-of-mem => terminate
    TEST_EXPECT_SEGFAULT(alloc_too_much);
    TEST_EXPECT_SEGFAULT(calloc_too_much);
    TEST_EXPECT_SEGFAULT(realloc_too_much);
    TEST_EXPECT_SEGFAULT(recalloc_too_much);
#endif
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

