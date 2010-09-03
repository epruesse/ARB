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
// #define TRACE_IS_EQUAL // print calls to numerical is_equal()

/* Note:
 * This file should not generate any static code.
 * Only place define's or inline functions here.
 * 
 * All macros named 'XXX__BROKEN' are intended to be used, when a
 * test is known to fail, but cannot be fixed atm for some reason
 *
 */

namespace arb_test {
    class FlushedOutput {
        inline void flushall() { fflush(stdout); fflush(stderr); }
    public:
        FlushedOutput() { flushall(); }
        ~FlushedOutput() { flushall(); }
        
#define VPRINTFORMAT(format) do { va_list parg; va_start(parg, format); vfprintf(stderr, format, parg); va_end(parg); } while(0)
    
        static void printf(const char *format, ...) __attribute__((format(printf, 1, 2))) {
            FlushedOutput yes;
            VPRINTFORMAT(format);
        }
        static void messagef(const char *filename, int lineno, const char *format, ...) __attribute__((format(printf, 3, 4))) {
            FlushedOutput yes;
            fprintf(stderr, "%s:%i: ", filename, lineno);
            VPRINTFORMAT(format);
        }
        static void warningf(const char *filename, int lineno, const char *format, ...) __attribute__((format(printf, 3, 4))) {
            GlobalTestData& global = test_data();
            if (global.show_warnings) {
                FlushedOutput yes;
                fprintf(stderr, "%s:%i: Warning: ", filename, lineno);
                VPRINTFORMAT(format);
                global.warnings++;
            }
        }
        static void errorf(const char *filename, int lineno, const char *format, ...) __attribute__((format(printf, 3, 4))) {
            FlushedOutput yes;
            fprintf(stderr, "%s:%i: Error: ", filename, lineno);
            VPRINTFORMAT(format);
        }
#undef VPRINTFORMAT
    };

    inline void print(int i)                 { fprintf(stderr, "%i", i); }
    inline void print_hex(int i)             { fprintf(stderr, "0x%x", i); }

    inline void print(long L)                { fprintf(stderr, "%li", L); }
    inline void print_hex(long L)            { fprintf(stderr, "0x%lx", L); }

    inline void print(size_t z)              { fprintf(stderr, "%zu", z); }
    inline void print_hex(size_t z)          { fprintf(stderr, "0x%zx", z); }
#ifdef ARB_64
    inline void print(unsigned u)            { fprintf(stderr, "%u", u); }
    inline void print_hex(unsigned u)        { fprintf(stderr, "0x%ux", u); }
#else
    inline void print(long unsigned u)       { fprintf(stderr, "%lu", u); }
    inline void print_hex(long unsigned u)   { fprintf(stderr, "0x%lux", u); }
#endif
    
    template <typename T1, typename T2> void print_pair(T1 t1, T2 t2) {
        print(t1);
        fputc(',', stderr);
        print(t2);
    }
    template <typename T1, typename T2> void print_hex_pair(T1 t1, T2 t2) {
        print_hex(t1);
        fputc(',', stderr);
        print_hex(t2);
    }
    template <typename T1, typename T2> void print_failed_equal(T1 t1, T2 t2) {
        FlushedOutput yes;
        fputs("is_equal(", stderr);
        print_pair(t1, t2);
        fputs(") (", stderr);
        print_hex_pair(t1, t2);
        fputs(") returns false\n", stderr);
    }

    template <typename T> inline const char *nameoftype(T unspecialized) {
        return specialized_nameoftype(unspecialized);  // define instanciating type below!
    }

#define NAMEOFTYPE(type) template <> inline const char * nameoftype<>(type) { return #type; }
    NAMEOFTYPE(bool);
    NAMEOFTYPE(int);
    NAMEOFTYPE(unsigned int);
    NAMEOFTYPE(long int);
    NAMEOFTYPE(long unsigned int);
#undef NAMEOFTYPE

    
    inline bool strnullequal(const char *s1, const char *s2) {
        return (s1 == s2) || (s1 && s2 && strcmp(s1, s2) == 0);
    }

    template <typename T1, typename T2> inline bool is_equal(T1 t1, T2 t2) {
        bool equal = (t1 == t2);
#ifdef TRACE_IS_EQUAL
        fprintf(stderr, "%-5s = is_equal(%s ", (equal ? "true" : "false"), nameoftype(t1));
        print(t1);
        fprintf(stderr, ",%s ", nameoftype(t2));
        print(t2);
        fprintf(stderr, ")\n");
#endif
        if (!equal) print_failed_equal(t1, t2);
        return equal;
    }

    template<> inline bool is_equal<>(const char *s1, const char *s2) {
        bool equal = strnullequal(s1, s2);
        if (!equal) {
            FlushedOutput::printf("str_equal('%s',\n"
                                  "          '%s') returns false\n", s1, s2);
        }
        return equal;
    }
    template<> inline bool is_equal<>(char *s1, char *s2) { return is_equal((const char *)s1, (const char *)s2); }
    template<> inline bool is_equal<>(const char *s1, char *s2) { return is_equal(s1, (const char *)s2); }
    template<> inline bool is_equal<>(char *s1, const char *s2) { return is_equal((const char *)s1, s2); }

#ifdef ARB_64
    typedef long int NULLPTR;
#else    
    typedef int NULLPTR;
#endif

    template<> inline bool is_equal<>(const char *s1, NULLPTR null) { return is_equal(s1, (const char *)null); }
    template<> inline bool is_equal<>(char *s1, NULLPTR null) { return is_equal(s1, (const char *)null); }
    template<> inline bool is_equal<>(NULLPTR null, const char *s2) { return is_equal((const char *)null, s2); }
    template<> inline bool is_equal<>(NULLPTR null, char *s2) { return is_equal((const char *)null, s2); }

    inline bool is_different(const char *s1, const char *s2) {
        bool different = !strnullequal(s1, s2);
        if (!different) {
            FlushedOutput::printf("str_different('%s', ..) returns false\n", s1);
        }
        return different;
    }

    inline bool is_similar(double d1, double d2, double epsilon) {
        double diff = d1-d2;
        if (diff<0.0) diff = -diff; // do not use fabs() here

        bool in_epsilon_range = diff < epsilon;
        if (!in_epsilon_range) {
            FlushedOutput::printf("is_similar(%f,%f,%f) returns false\n", d1, d2, epsilon);
        }
        return in_epsilon_range;
    }

    inline bool is_equal(double d1, double d2) {
        return is_similar(d1, d2, 0.000001);
    }

};

// --------------------------------------------------------------------------------

#define TEST_MSG(format,strarg)           arb_test::FlushedOutput::messagef(__FILE__, __LINE__, format, (strarg))
#define TEST_MSG2(format,strarg1,strarg2) arb_test::FlushedOutput::messagef(__FILE__, __LINE__, format, (strarg1), (strarg2))

#define TEST_WARNING(format,strarg)           arb_test::FlushedOutput::warningf(__FILE__, __LINE__, format, (strarg))
#define TEST_WARNING2(format,strarg1,strarg2) arb_test::FlushedOutput::warningf(__FILE__, __LINE__, format, (strarg1), (strarg2))

#define TEST_ERROR(format,strarg)           do { arb_test::FlushedOutput::errorf(__FILE__, __LINE__, format, (strarg)); TEST_ASSERT(0); } while(0)
#define TEST_ERROR2(format,strarg1,strarg2) do { arb_test::FlushedOutput::errorf(__FILE__, __LINE__, format, (strarg1), (strarg2)); TEST_ASSERT(0); } while(0)

// --------------------------------------------------------------------------------

#define TEST_ASSERT(cond) test_assert(cond)

#define TEST_ASSERT__BROKEN(cond)                                       \
    do {                                                                \
        if (cond)                                                       \
            TEST_ERROR("Formerly broken test '%s' succeeds", #cond);    \
        else                                                            \
            TEST_WARNING("Known broken behavior ('%s' fails)", #cond);  \
    } while (0)


#define TEST_ASSERT_ZERO(cond)         TEST_ASSERT((cond)         == 0)
#define TEST_ASSERT_ZERO__BROKEN(cond) TEST_ASSERT__BROKEN((cond) == 0)

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
// TEST_ASSERT_SEGFAULT and TEST_ASSERT_CODE_ASSERTION_FAILS
// only work if binary is linked with ARBDB


#ifdef ENABLE_CRASH_TESTS
# ifdef ASSERTION_USED
#  define TEST_INTERNAL_SEGFAULT_ASSERTION(cb,wantAssert)               \
    do {                                                                \
         bool& assertion_failed =                                       \
             arb_test::test_data().assertion_failed;                    \
         bool old_state = assertion_failed;                             \
         assertion_failed = false;                                      \
         TEST_ASSERT(GBK_raises_SIGSEGV(cb,true));                      \
         if (!GBK_running_on_valgrind())                                \
             TEST_ASSERT(assertion_failed == wantAssert);               \
         assertion_failed = old_state;                                  \
    } while (0)
#  define TEST_ASSERT_CODE_ASSERTION_FAILS(cb) TEST_INTERNAL_SEGFAULT_ASSERTION(cb,true)
#  define TEST_ASSERT_SEGFAULT(cb)             TEST_INTERNAL_SEGFAULT_ASSERTION(cb,false)
# else // ENABLE_CRASH_TESTS but no ASSERTION_USED (test segfaults in NDEBUG mode)
#  define TEST_ASSERT_CODE_ASSERTION_FAILS(cb)
#  define TEST_ASSERT_SEGFAULT(cb)             TEST_ASSERT(GBK_raises_SIGSEGV(cb,true))
# endif
#else // not ENABLE_CRASH_TESTS (i.e. skip these tests completely)
# define TEST_ASSERT_CODE_ASSERTION_FAILS(cb)
# define TEST_ASSERT_SEGFAULT(cb)
#endif

// --------------------------------------------------------------------------------

#define TEST_ASSERT_EQUAL(t1,t2)           TEST_ASSERT(arb_test::is_equal(t1, t2))
#define TEST_ASSERT_EQUAL__BROKEN(t1,t2)   TEST_ASSERT__BROKEN(arb_test::is_equal(t1, t2))

#define TEST_ASSERT_SIMILAR(t1,t2,epsilon)         TEST_ASSERT(arb_test::is_similar(t1, t2, epsilon))
#define TEST_ASSERT_SIMILAR__BROKEN(t1,t2,epsilon) TEST_ASSERT__BROKEN(arb_test::is_similar(t1, t2, epsilon))

#else
#error test_unit.h included twice
#endif // TEST_UNIT_H
