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
#ifndef _CPP_CSTDARG
#include <cstdarg>
#endif

#define ENABLE_CRASH_TESTS // comment out this line to get rid of provoked SEGVs (e.g. while debugging test-code)

/* Note:
 * This file should not generate any static code.
 * Only place define's or inline functions here.
 * 
 * All macros named 'XXX__BROKEN' are intended to be used, when a
 * test is known to fail, but cannot be fixed atm for some reason
 * 
 */

namespace arb_test {
    inline bool strnullequal(const char *s1, const char *s2) {
        return (s1 == s2) || (s1 && s2 && strcmp(s1, s2) == 0);
    }

    inline int printf_flushed(const char *format, ...) {
        fflush(stdout);
        fflush(stderr);

        va_list parg;
        va_start(parg, format);
        int     printed = vfprintf(stderr, format, parg);
        va_end(parg);

        fflush(stderr);

        return printed;
    }

    inline bool is_equal(const char *s1, const char *s2) {
        bool equal = strnullequal(s1, s2);
        if (!equal) {
            printf_flushed("str_equal('%s',\n"
                           "          '%s') returns false\n", s1, s2);
        }
        return equal;
    }
    inline bool is_different(const char *s1, const char *s2) {
        bool different = !strnullequal(s1, s2);
        if (!different) {
            printf_flushed("str_different('%s', ..) returns false\n", s1);
        }
        return different;
    }

    inline bool is_equal(int n1, int n2) {
        bool equal = n1 == n2;
        if (!equal) {
            printf_flushed("numeric_equal(%i,%i) returns false\n", n1, n2);
        }
        return equal;
    }
    inline bool is_equal(size_t n1, size_t n2) {
        bool equal = n1 == n2;
        if (!equal) {
            printf_flushed("numeric_equal(%zu,%zu) returns false\n", n1, n2);
        }
        return equal;
    }
};

// --------------------------------------------------------------------------------

#define TEST_MSG(format, strarg)                                \
    arb_test::printf_flushed("%s:%i: " format "\n",             \
                             __FILE__, __LINE__, (strarg))

#define TEST_WARNING(format, strarg)                    \
    do {                                                \
        if (arb_test::test_data().show_warnings) {    \
            TEST_MSG("Warning: " format, strarg);       \
        }                                               \
    } while(0)

#define TEST_ERROR(format, strarg)              \
    do {                                        \
        TEST_MSG("Error: " format, strarg);     \
        ARB_SIGSEGV(0);                         \
    } while(0)


#define TEST_MSG2(format, strarg1, strarg2)             \
    fprintf(stderr, "%s:%i: " format "\n",              \
            __FILE__, __LINE__, (strarg1), (strarg2))

#define TEST_WARNING2(format, strarg1, strarg2)         \
    TEST_MSG2("Warning: " format, strarg1, strarg2)

#define TEST_ERROR2(format, strarg1, strarg2)           \
    do {                                                \
        TEST_MSG2("Error: " format, strarg1, strarg2);  \
        ARB_SIGSEGV(0);                                 \
    } while(0)

// --------------------------------------------------------------------------------

#define TEST_ASSERT(cond) test_assert(cond)

#define TEST_ASSERT__BROKEN(cond)                                       \
    do {                                                                \
        if (cond)                                                       \
            TEST_ERROR("Formerly broken test '%s' succeeds", #cond);    \
        else                                                            \
            TEST_WARNING("Known broken behavior ('%s' fails)", #cond);  \
    } while (0)


// --------------------------------------------------------------------------------

#define MISSING_TEST(description)                       \
    TEST_WARNING("Missing test '%s'", #description)

// --------------------------------------------------------------------------------

#define TEST_ASSERT_NO_ERROR(error_cond)                                \
    do {                                                                \
        const char *error = (error_cond);                               \
        if (error) {                                                    \
            TEST_ERROR2("call '%s' returns unexpected error '%s'",      \
                        #error_cond, error);                            \
        }                                                               \
    } while (0)

#define TEST_ASSERT_ERROR(error_cond)                                   \
    do {                                                                \
        if (!(error_cond)) {                                            \
            TEST_ERROR("Expected to see error from '%s'", #error_cond); \
        }                                                               \
    } while (0)


#define TEST_ASSERT_NO_ERROR__BROKEN(error_cond)                        \
    do {                                                                \
        const char *error = (error_cond);                               \
        if (error) {                                                    \
            TEST_WARNING2("Known broken behavior ('%s' reports error '%s')", \
                         #error_cond, error);                           \
        }                                                               \
        else {                                                          \
            TEST_ERROR("Formerly broken test '%s' succeeds (reports no error)", \
                       #error_cond);                                    \
        }                                                               \
    } while (0)

#define TEST_ASSERT_ERROR__BROKEN(error_cond)                           \
    do {                                                                \
        const char *error = (error_cond);                               \
        if (!error) {                                                    \
            TEST_WARNING("Known broken behavior ('%s' fails to report error)", \
                         #error_cond);                                  \
        }                                                               \
        else {                                                          \
            TEST_ERROR2("Former broken test '%s' succeeds (reports error '%s')", \
                       #error_cond, error);                             \
        }                                                               \
    } while (0)


// --------------------------------------------------------------------------------

#define TEST_EXPORTED_ERROR() (GB_have_error() ? GB_await_error() : NULL)

#define TEST_CLEAR_EXPORTED_ERROR()                                     \
    do {                                                                \
        const char *error = TEST_EXPORTED_ERROR();                      \
        if (error) {                                                    \
            TEST_WARNING("detected and cleared exported error '%s'",    \
                         error);                                        \
        }                                                               \
    } while (0)

#define TEST_ASSERT_NORESULT__ERROREXPORTED(create_result)              \
    do {                                                                \
        TEST_CLEAR_EXPORTED_ERROR();                                    \
        bool have_result = (create_result);                             \
        const char *error = TEST_EXPORTED_ERROR();                      \
        if (have_result) {                                              \
            if (error) {                                                \
                TEST_WARNING("Error '%s' exported (when result returned)", \
                             error);                                    \
            }                                                           \
            TEST_ERROR("Expected '%s' to return NULL", #create_result); \
        }                                                               \
        else if (!error) {                                              \
            TEST_ERROR("'%s' (w/o result) should always export error",  \
                       #create_result);                                 \
        }                                                               \
    } while (0)


#define TEST_ASSERT_RESULT__NOERROREXPORTED(create_result)              \
    do {                                                                \
        TEST_CLEAR_EXPORTED_ERROR();                                    \
        bool have_result = (create_result);                             \
        const char *error = TEST_EXPORTED_ERROR();                      \
        if (have_result) {                                              \
            if (error) {                                                \
                TEST_ERROR("Error '%s' exported (when result returned)", \
                           error);                                      \
            }                                                           \
        }                                                               \
        else {                                                          \
            if (!error) {                                               \
                TEST_WARNING("'%s' (w/o result) should always export error", \
                             #create_result);                           \
            }                                                           \
            else {                                                      \
                TEST_WARNING("exported error is '%s'", error);          \
            }                                                           \
            TEST_ERROR("Expected '%s' to return sth", #create_result);  \
        }                                                               \
    } while (0)

// --------------------------------------------------------------------------------

#ifdef ENABLE_CRASH_TESTS
#define TEST_ASSERT_SEGFAULT(cb) TEST_ASSERT(GBK_raises_SIGSEGV(cb))
#else
#define TEST_ASSERT_SEGFAULT(cb) 
#endif

#if defined(ASSERTION_USED)
#define TEST_ASSERT_CODE_ASSERTION_FAILS(cb) TEST_ASSERT_SEGFAULT(cb)
#else
#define TEST_ASSERT_CODE_ASSERTION_FAILS(cb)
#endif // ASSERTION_USED


// --------------------------------------------------------------------------------

#define TEST_ASSERT_EQUAL(t1, t2)         TEST_ASSERT(arb_test::is_equal(t1, t2))
#define TEST_ASSERT_EQUAL__BROKEN(t1, t2) TEST_ASSERT__BROKEN(arb_test::is_equal(t1, t2))

#else
#error test_unit.h included twice
#endif // TEST_UNIT_H
