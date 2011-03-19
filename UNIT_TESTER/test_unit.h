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
#ifndef _GLIBCXX_CSTDARG
#include <cstdarg>
#endif
#ifndef _CPP_CSTDLIB
#include <cstdlib>
#endif
#ifndef ERRNO_H
#include <errno.h>
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

#define TEST_ASSERT(cond) test_assert(cond, false)

namespace arb_test {

    class StaticCode {
        static void vcompiler_msg(const char *filename, int lineno, const char *message_type, const char *format, va_list parg) {
            fprintf(stderr, "%s:%i: ", filename, lineno);
            if (message_type) fprintf(stderr, "%s: ", message_type);
            vfprintf(stderr, format, parg);
        }

#define WITHVALISTFROM(format,CODE)             do { va_list parg; va_start(parg, format); CODE; va_end(parg); } while(0)
#define VPRINTFORMAT(format)                    WITHVALISTFROM(format, vfprintf(stderr, format, parg))
#define VCOMPILERMSG(file,line,msgtype,format)  WITHVALISTFROM(format, vcompiler_msg(file, line, msgtype, format, parg))
        
    public:

        static void printf(const char *format, ...) __attribute__((format(printf, 1, 2))) {
            FlushedOutputNoLF yes;
            VPRINTFORMAT(format);
        }
        static void warningf(const char *filename, int lineno, const char *format, ...) __attribute__((format(printf, 3, 4))) {
            GlobalTestData& global = test_data();
            if (global.show_warnings) {
                FlushedOutput yes;
                VCOMPILERMSG(filename, lineno, "Warning", format);
                GlobalTestData::print_annotation();
                global.warnings++;
            }
        }
        static void errorf(const char *filename, int lineno, const char *format, ...) __attribute__((format(printf, 3, 4))) {
            {
                FlushedOutput yes;
                VCOMPILERMSG(filename, lineno, "Error", format);
                GlobalTestData::print_annotation();
            }
            TRIGGER_ASSERTION(false); // fake an assertion failure
        }
        static void ioerrorf(const char *filename, int lineno, const char *format, ...) __attribute__((format(printf, 3, 4))) {
            {
                FlushedOutput yes;
                VCOMPILERMSG(filename, lineno, "Error", format);
                fprintf(stderr, " (errno=%i='%s')", errno, strerror(errno));
                GlobalTestData::print_annotation();
            }
            TRIGGER_ASSERTION(false); // fake an assertion failure
        }
#undef VPRINTFORMAT
#undef VCOMPILERMSG
#undef WITHVALISTFROM

        static void print_readable_string(const char *s, FILE *out) {
            // quote like C does!
            if (s) {
                fputc('\"', out);
                for (int i_ = 0; s[i_]; ++i_) {
                    switch (s[i_]) {
                        case '\n': fputs("\\n", out); break;
                        case '\t': fputs("\\t", out); break;
                        case '\"': fputs("\\\"", out); break;
                        case '\\': fputs("\\\\", out); break;
                        default: fputc(s[i_], out); break;
                    }
                }
                fputc('\"', out);
            }
            else {
                fputs("(null)", out);
            }
        }
    };

    inline void print(int i)                 { fprintf(stderr, "%i", i); }
    inline void print_hex(int i)             { fprintf(stderr, "0x%x", i); }

    inline void print(long L)                { fprintf(stderr, "%li", L); }
    inline void print_hex(long L)            { fprintf(stderr, "0x%lx", L); }

    inline void print(const char *s)         { StaticCode::print_readable_string(s, stderr); }
    // no print_hex for strings

    inline void print(size_t z)              { fprintf(stderr, "%zu", z); }
    inline void print_hex(size_t z)          { fprintf(stderr, "0x%zx", z); }

    inline void print(unsigned char c)       { fprintf(stderr, "'%c'", c); }
    inline void print_hex(unsigned char c)   { print_hex(size_t(c)); }

    inline void print(char c)                { print((unsigned char)c); }
    inline void print_hex(char c)            { print_hex((unsigned char)c); }

    // dont dup size_t:
#ifdef ARB_64
    inline void print(unsigned u)            { fprintf(stderr, "%u", u); }
    inline void print_hex(unsigned u)        { fprintf(stderr, "0x%x", u); }
#else
    inline void print(long unsigned u)       { fprintf(stderr, "%lu", u); }
    inline void print_hex(long unsigned u)   { fprintf(stderr, "0x%lx", u); }
#endif

    template <typename T1, typename T2> inline void print_pair(T1 t1, T2 t2) {
        print(t1);
        fputs(", ", stderr);
        print(t2);
    }
    template <typename T1, typename T2> inline void print_hex_pair(T1 t1, T2 t2) {
        print_hex(t1);
        fputc(',', stderr);
        print_hex(t2);
    }

    template <typename T> inline const char *nameoftype(T unspecialized) {
        return specialized_nameoftype(unspecialized);  // define instanciating type below!
    }

#define NAMEOFTYPE(type) template <> inline const char * nameoftype<>(type) { return #type; }
    NAMEOFTYPE(bool);
    NAMEOFTYPE(char);
    NAMEOFTYPE(unsigned char);
    NAMEOFTYPE(const char*);
    NAMEOFTYPE(int);
    NAMEOFTYPE(unsigned int);
    NAMEOFTYPE(long int);
    NAMEOFTYPE(long unsigned int);
#undef NAMEOFTYPE



#ifdef TRACE_IS_EQUAL
    template <typename T1, typename T2> inline bool bool_traced(const char *name, bool equal, T1 t1, T2 t2) {
        fprintf(stderr, "%-5s = %s(%s ", (equal ? "true" : "false"), name, nameoftype(t1));
        print(t1);
        fprintf(stderr, ",%s ", nameoftype(t2));
        print(t2);
        fprintf(stderr, ")\n");
        return equal;
    }
#else
    template <typename T1, typename T2> inline bool bool_traced(const char *, bool equal, T1 , T2 ) {
        return equal;
    }
#endif

    template <typename T1, typename T2> inline bool is_equal(T1 t1, T2 t2) {
        return bool_traced("is_equal", t1 == t2, t1, t2);
    }
    template<> inline bool is_equal<>(const char *s1, const char *s2) {
        return bool_traced("is_equal", (s1 == s2) || (s1 && s2 && strcmp(s1, s2) == 0), s1, s2);
    }
    template <typename T1, typename T2> inline bool is_less(T1 t1, T2 t2) {
        return bool_traced("is_less", t1 < t2, t1, t2);
    }


    template <typename T1, typename T2> inline void print_failed_compare(T1 t1, T2 t2, const char *prefix, const char *infix, const char *suffix) {
        FlushedOutput yes;
        fputs(prefix, stderr);
        print_pair(t1, t2);
        fputs(infix, stderr);
        print_hex_pair(t1, t2);
        fputs(suffix, stderr);
    }
    template <typename T1, typename T2> inline void print_failed_equal(T1 t1, T2 t2) {
        print_failed_compare(t1, t2, "test_equal(", ") (", ") returns false");
    }
    template <typename T1, typename T2> inline void print_failed_less_equal(T1 t1, T2 t2) {
        print_failed_compare(t1, t2, "test_less_equal(", ") (", ") returns false");
    }
    template <typename T1, typename T2> inline void print_failed_different(T1 t1, T2 t2) {
        print_failed_compare(t1, t2, "test_different(", ") (", ") returns false");
    }

    template<> inline void print_failed_equal<>(const char *s1, const char *s2) {
        FlushedOutput yes;
        fputs("test_equal(", stderr);
        print(s1);
        fputs(",\n           ", stderr);
        print(s2);
        fputs(") returns false", stderr);
    }
    template<> inline void print_failed_different<>(const char *s1, const char *) {
        FlushedOutput yes;
        fputs("test_different(", stderr);
        print(s1);
        fputs(", <same>", stderr);
        fputs(") returns false", stderr);
    }

    template <typename T1, typename T2> inline bool test_equal(T1 t1, T2 t2) {
        bool equal = is_equal(t1, t2);
        if (!equal) print_failed_equal(t1, t2);
        return equal;
    }
    template <typename T1, typename T2> inline bool test_less_equal(T1 lower, T2 upper) {
        bool less_equal = is_equal(lower, upper) || is_less(lower, upper);
        if (!less_equal) print_failed_less_equal(lower, upper);
        return less_equal;
    }
    template <typename T1, typename T2> inline bool test_different(T1 t1, T2 t2) {
        bool different = !is_equal(t1, t2);
        if (!different) print_failed_different(t1, t2);
        return different;
    }

#define ACCEPT_NON_CONST_ARGUMENTS(FUN,TYPE) \
    template<> inline bool FUN<>(TYPE p1, TYPE p2) { return FUN((const TYPE)p1, (const TYPE)p2); } \
        template<> inline bool FUN<>(TYPE p1, const TYPE p2) { return FUN((const TYPE)p1, p2); } \
        template<> inline bool FUN<>(const TYPE p1, TYPE p2) { return FUN(p1, (const TYPE)p2); }

    ACCEPT_NON_CONST_ARGUMENTS(test_equal, char*);
    ACCEPT_NON_CONST_ARGUMENTS(test_different, char*);

#ifdef ARB_64
    typedef long int NULLPTR;
#else
    typedef int      NULLPTR;
#endif
    
#define ACCEPT_NULLPTR_ARGUMENTS(FUN,TYPE)         \
    template<> inline bool FUN<>(TYPE p1, NULLPTR p2) { TEST_ASSERT(!p2); return FUN((const TYPE)p1, (const TYPE)p2); } \
        template<> inline bool FUN<>(const TYPE p1, NULLPTR p2) { TEST_ASSERT(!p2); return FUN((const TYPE)p1, (const TYPE)p2); } \
        template<> inline bool FUN<>(NULLPTR p1, TYPE p2) { TEST_ASSERT(!p1); return FUN((const TYPE)p1, (const TYPE)p2); } \
        template<> inline bool FUN<>(NULLPTR p1, const TYPE p2) { TEST_ASSERT(!p1); return FUN((const TYPE)p1, (const TYPE)p2); }

    ACCEPT_NULLPTR_ARGUMENTS(test_equal, char*);
    ACCEPT_NULLPTR_ARGUMENTS(test_different, char*);

    inline bool test_similar(double d1, double d2, double epsilon) {
        double diff = d1-d2;
        if (diff<0.0) diff = -diff; // do not use fabs() here

        bool in_epsilon_range = diff < epsilon;
        if (!in_epsilon_range) {
            StaticCode::printf("test_similar(%f,%f,%f) returns false\n", d1, d2, epsilon);
        }
        return in_epsilon_range;
    }

    inline bool test_equal(double d1, double d2) { return test_similar(d1, d2, 0.000001); }

    // inline bool is_equal(double d1, double d2) {
        // return is_similar(d1, d2, 0.000001);
    // }

    inline bool files_are_equal(const char *file1, const char *file2) {
        const char    *error = NULL;
        FILE          *fp1   = fopen(file1, "rb");
        FlushedOutput  yes;

        if (!fp1) {
            StaticCode::printf("can't open '%s'", file1);
            error = "i/o error";
        }
        else {
            FILE *fp2 = fopen(file2, "rb");
            if (!fp2) {
                StaticCode::printf("can't open '%s'", file2);
                error = "i/o error";
            }
            else {
                const int      BLOCKSIZE    = 4096;
                unsigned char *buf1         = (unsigned char*)malloc(BLOCKSIZE);
                unsigned char *buf2         = (unsigned char*)malloc(BLOCKSIZE);
                int            equal_bytes  = 0;

                while (!error) {
                    int read1  = fread(buf1, 1, BLOCKSIZE, fp1);
                    int read2  = fread(buf2, 1, BLOCKSIZE, fp2);
                    int common = read1<read2 ? read1 : read2;

                    if (!common) {
                        if (read1 != read2) error = "filesize differs";
                        break;
                    }

                    if (memcmp(buf1, buf2, common) == 0) {
                        equal_bytes += common;
                    }
                    else {
                        int x = 0;
                        while (buf1[x] == buf2[x]) {
                            x++;
                            equal_bytes++;
                        }
                        error = "content differs";

                        // x is the position inside the current block
                        const int DUMP       = 7;
                        int       y1         = x >= DUMP ? x-DUMP : 0;
                        int       y2         = (x+DUMP)>common ? common : (x+DUMP);
                        int       blockstart = equal_bytes-x;

                        for (int y = y1; y <= y2; y++) {
                            fprintf(stderr, "[0x%04x]", blockstart+y);
                            print_pair(buf1[y], buf2[y]);
                            fputc(' ', stderr);
                            print_hex_pair(buf1[y], buf2[y]);
                            if (x == y) fputs("                     <- diff", stderr);
                            fputc('\n', stderr);
                        }
                        if (y2 == common) {
                            fputs("[end of block - truncated]\n", stderr);
                        }
                    }
                }

                if (error) StaticCode::printf("files_are_equal: equal_bytes=%i\n", equal_bytes);
                test_assert(error || equal_bytes, true); // comparing empty files is nonsense

                free(buf2);
                free(buf1);
                fclose(fp2);
            }
            fclose(fp1);
        }

        if (error) StaticCode::printf("files_are_equal(%s, %s) fails: %s\n", file1, file2, error);
        return !error;
    }
};

// --------------------------------------------------------------------------------

#define TEST_WARNING(format,strarg)           arb_test::StaticCode::warningf(__FILE__, __LINE__, format, (strarg))
#define TEST_WARNING2(format,strarg1,strarg2) arb_test::StaticCode::warningf(__FILE__, __LINE__, format, (strarg1), (strarg2))

#define TEST_ERROR(format,strarg)           arb_test::StaticCode::errorf(__FILE__, __LINE__, format, (strarg))
#define TEST_ERROR2(format,strarg1,strarg2) arb_test::StaticCode::errorf(__FILE__, __LINE__, format, (strarg1), (strarg2))
#define TEST_IOERROR(format,strarg)         arb_test::StaticCode::ioerrorf(__FILE__, __LINE__, format, (strarg))

// --------------------------------------------------------------------------------

#define TEST_ASSERT__BROKEN(cond)                                       \
    do {                                                                \
        if (cond)                                                       \
            TEST_ERROR("Formerly broken test '%s' succeeds", #cond);    \
        else                                                            \
            TEST_WARNING("Known broken behavior ('%s' fails)", #cond);  \
    } while (0)


#define TEST_ASSERT_ZERO(cond)         TEST_ASSERT((cond)         == 0)
#define TEST_ASSERT_ZERO__BROKEN(cond) TEST_ASSERT__BROKEN((cond) == 0)

#define TEST_ASSERT_ZERO_OR_SHOW_ERRNO(iocond)                  \
    do {                                                        \
        if ((iocond))                                           \
            TEST_IOERROR("I/O-failure in '%s'", #iocond);       \
    } while(0)

// --------------------------------------------------------------------------------

#define MISSING_TEST(description)                       \
    TEST_WARNING("Missing test '%s'", #description)

// --------------------------------------------------------------------------------

#define TEST_ASSERT_NO_ERROR(error_cond)                                \
    do {                                                                \
        const char *error_ = (error_cond);                              \
        if (error_) {                                                   \
            TEST_ERROR2("call '%s' returns unexpected error '%s'",      \
                        #error_cond, error_);                           \
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

#define TEST_ASSERT_EQUAL(e1,t2)           TEST_ASSERT(arb_test::test_equal(e1, t2))
#define TEST_ASSERT_EQUAL__BROKEN(e1,t2)   TEST_ASSERT__BROKEN(arb_test::test_equal(e1, t2))

#define TEST_ASSERT_SIMILAR(e1,t2,epsilon)         TEST_ASSERT(arb_test::test_similar(e1, t2, epsilon))
#define TEST_ASSERT_SIMILAR__BROKEN(e1,t2,epsilon) TEST_ASSERT__BROKEN(arb_test::test_similar(e1, t2, epsilon))

#define TEST_ASSERT_DIFFERENT(e1,t2)         TEST_ASSERT(arb_test::test_different(e1, t2))
#define TEST_ASSERT_DIFFERENT__BROKEN(e1,t2) TEST_ASSERT__BROKEN(arb_test::test_different(e1, t2))

#define TEST_ASSERT_LOWER_EQUAL(lower,upper)  TEST_ASSERT(arb_test::test_less_equal(lower, upper))
#define TEST_ASSERT_LOWER(lower,upper) do { TEST_ASSERT_LOWER_EQUAL(lower, upper); TEST_ASSERT_DIFFERENT(lower, upper); } while(0)
#define TEST_ASSERT_IN_RANGE(val,lower,upper) do { TEST_ASSERT_LOWER_EQUAL(lower, val); TEST_ASSERT_LOWER_EQUAL(val, upper); } while(0)

// --------------------------------------------------------------------------------
// the following macros only work when
// - tested module depends on ARBDB and
// - some ARBDB header has been included
// Otherwise the section is skipped completely.
//
// @@@ ARBDB->ARBCORE later

#ifdef AD_PROT_H

namespace arb_test {
    inline bool test_files_equal(const char *file1, const char *file2) {
        FlushedOutputNoLF yes;
        return GB_test_files_equal(file1, file2);
    }
    inline bool test_textfile_difflines(const char *file1, const char *file2, int expected_difflines) {
        FlushedOutputNoLF yes;
        return GB_test_textfile_difflines(file1, file2, expected_difflines, 0);
    }
    inline bool test_textfile_difflines_ignoreDates(const char *file1, const char *file2, int expected_difflines) {
        FlushedOutputNoLF yes;
        return GB_test_textfile_difflines(file1, file2, expected_difflines, 1);
    }
};

#define TEST_ASSERT_TEXTFILE_DIFFLINES(f1,f2,diff)         TEST_ASSERT(arb_test::test_textfile_difflines(f1,f2, diff))
#define TEST_ASSERT_TEXTFILE_DIFFLINES__BROKEN(f1,f2,diff) TEST_ASSERT__BROKEN(arb_test::test_textfile_difflines(f1,f2, diff))

#define TEST_ASSERT_TEXTFILE_DIFFLINES_IGNORE_DATES(f1,f2,diff)         TEST_ASSERT(arb_test::test_textfile_difflines_ignoreDates(f1,f2, diff))
#define TEST_ASSERT_TEXTFILE_DIFFLINES_IGNORE_DATES__BROKEN(f1,f2,diff) TEST_ASSERT__BROKEN(arb_test::test_textfile_difflines_ignoreDates(f1,f2, diff))

#define TEST_ASSERT_FILES_EQUAL(f1,f2)         TEST_ASSERT(arb_test::test_files_equal(f1,f2))
#define TEST_ASSERT_FILES_EQUAL__BROKEN(f1,f2) TEST_ASSERT__BROKEN(arb_test::test_files_equal(f1,f2))

#define TEST_ASSERT_TEXTFILES_EQUAL(f1,f2)         TEST_ASSERT_TEXTFILE_DIFFLINES(f1,f2,0)
#define TEST_ASSERT_TEXTFILES_EQUAL__BROKEN(f1,f2) TEST_ASSERT_TEXTFILE_DIFFLINES__BROKEN(f1,f2,0)

#else

#define WARN_MISS_ADPROT() need_include__ad_prot_h__BEFORE__test_unit_h

#define TEST_ASSERT_TEXTFILE_DIFFLINES(f1,f2,diff)                      WARN_MISS_ADPROT()
#define TEST_ASSERT_TEXTFILE_DIFFLINES__BROKEN(f1,f2,diff)              WARN_MISS_ADPROT()
#define TEST_ASSERT_TEXTFILE_DIFFLINES_IGNORE_DATES(f1,f2,diff)         WARN_MISS_ADPROT()
#define TEST_ASSERT_TEXTFILE_DIFFLINES_IGNORE_DATES__BROKEN(f1,f2,diff) WARN_MISS_ADPROT()
#define TEST_ASSERT_FILES_EQUAL(f1,f2)                                  WARN_MISS_ADPROT()
#define TEST_ASSERT_FILES_EQUAL__BROKEN(f1,f2)                          WARN_MISS_ADPROT()
#define TEST_ASSERT_TEXTFILES_EQUAL(f1,f2)                              WARN_MISS_ADPROT()
#define TEST_ASSERT_TEXTFILES_EQUAL__BROKEN(f1,f2)                      WARN_MISS_ADPROT()

#endif // AD_PROT_H

// --------------------------------------------------------------------------------

#define TEST_SETUP_GLOBAL_ENVIRONMENT(modulename) TEST_ASSERT_NO_ERROR(GB_system("../test_environment setup " modulename))
// cleanup is done (by Makefile.suite) after all unit tests have been run

// --------------------------------------------------------------------------------

#else
#error test_unit.h included twice
#endif // TEST_UNIT_H
