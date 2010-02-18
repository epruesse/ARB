// ================================================================ //
//                                                                  //
//   File      : test_unit.h                                        //
//   Purpose   : include into test code                             //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in February 2010   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef TEST_UNIT_H
#define TEST_UNIT_H

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

/* Note:
 * This file should not generate any static code.
 * Only place define's or inline functions here.
 */

// --------------------------------------------------------------------------------

#define TEST_MSG(format, strarg)                                        \
    fprintf(stderr, "%s:%i: " format "\n", __FILE__, __LINE__, (strarg))

#define TEST_WARNING(format, strarg)            \
    TEST_MSG("Warning: " format, strarg)

#define TEST_ERROR(format, strarg)              \
    do {                                        \
        TEST_MSG("Error: " format, strarg);     \
        ARB_SIGSEGV(0);                         \
    } while(0)

// --------------------------------------------------------------------------------
    
#define TEST_ASSERT(cond) arb_assert(cond)

#define TEST_ASSERT_BROKEN(cond)                                        \
    do {                                                                \
        if (cond) TEST_ERROR("Formerly broken test '%s' succeeds", #cond); \
        else TEST_WARNING("Known broken behavior ('%s' fails)", #cond); \
    } while (0)


#define MISSING_TEST(description)                       \
    TEST_WARNING("Missing test '%s'", #description)

#else
#error test_unit.h included twice
#endif // TEST_UNIT_H
