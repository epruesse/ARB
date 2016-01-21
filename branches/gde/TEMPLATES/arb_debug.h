// =============================================================== //
//                                                                 //
//   File      : arb_debug.h                                       //
//   Purpose   : some debugging tools                              //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in May 2007       //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //
#ifndef ARB_DEBUG_H
#define ARB_DEBUG_H

#if defined(DEBUG)

// if you get the valgrind warning
// "Conditional jump or move depends on uninitialized value"
// for a complex statement, use TEST_EXPR_INITIALIZED to find out
// which part of the statement is the cause.

#define TEST_EXPR_INITIALIZED(expr) do {     \
        static bool t_i_b;              \
                                        \
        if (expr) t_i_b = false;        \
        else      t_i_b = true;         \
    } while (0)


template <int OFF, int SIZE>
static void CHECK_CMEM_INITIALIZED(const char *ptr) {
    arb_assert(SIZE); // SIZE == 0 is invalid
    if (SIZE == 1) {
        TEST_EXPR_INITIALIZED(ptr[OFF]);
    }
    else {
        const int HALF = SIZE/2;
        CHECK_CMEM_INITIALIZED<OFF,HALF>(ptr);
        CHECK_CMEM_INITIALIZED<OFF+HALF,SIZE-HALF>(ptr);
    }
}

template <int SIZE>
static void CHECK_MEM_INITIALIZED(const void *ptr) {
    CHECK_CMEM_INITIALIZED<0,SIZE>((const char*)ptr);
}

#define TEST_INITIALIZED(data) CHECK_MEM_INITIALIZED< sizeof(data) >( &(data) )

#else

// create warning for forgotten debug code
#define TEST_EXPR_INITIALIZED(expr) { int TEST_EXPR_INITIALIZED; }
#define TEST_INITIALIZED(data) { int TEST_INITIALIZED; }
#define THROW_VALGRIND_WARNING() { int THROW_VALGRIND_WARNING; }

#endif // DEBUG

#else
#error arb_debug.h included twice
#endif // ARB_DEBUG_H

