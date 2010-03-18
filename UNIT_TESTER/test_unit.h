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

namespace arb_test {
    inline bool strnullequal(const char *s1, const char *s2) {
        return (s1 == s2) || (s1 && s2 && strcmp(s1, s2) == 0);
    }

    inline bool str_equal(const char *s1, const char *s2) {
        bool equal = strnullequal(s1, s2);
        if (!equal) {
            fprintf(stderr, "str_equal('%s', '%s') returns false\n", s1, s2);
        }
        return equal;
    }
    inline bool str_different(const char *s1, const char *s2) {
        bool different = !strnullequal(s1, s2);
        if (!different) {
            fprintf(stderr, "str_different('%s', '%s') returns false\n", s1, s2);
        }
        return different;
    }
};

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

#define TEST_ASSERT(cond) test_assert(cond)

#define TEST_ASSERT_BROKEN(cond)                                        \
    do {                                                                \
        if (cond) TEST_ERROR("Formerly broken test '%s' succeeds", #cond); \
        else TEST_WARNING("Known broken behavior ('%s' fails)", #cond); \
    } while (0)


#define MISSING_TEST(description)                       \
    TEST_WARNING("Missing test '%s'", #description)


#define TEST_ASSERT_SEGFAULT(cb) TEST_ASSERT(GBK_raises_SIGSEGV(cb))

#define TEST_ASSERT_STRINGRESULT(s1, s2) TEST_ASSERT(arb_test::str_equal(s1, s2))

#else
#error test_unit.h included twice
#endif // TEST_UNIT_H
