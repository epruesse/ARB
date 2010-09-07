// ================================================================= //
//                                                                   //
//   File      : test_global.h                                       //
//   Purpose   : special assertion handling if arb is compiled       // 
//               with unit-tests                                     //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2010   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef TEST_GLOBAL_H
#define TEST_GLOBAL_H

#ifndef _CPP_CSTDARG
#include <cstdarg>
#endif
#ifndef _CPP_CSTDIO
#include <cstdio>
#endif
#ifndef _CPP_CERRNO
#include <cerrno>
#endif
#ifndef _CPP_CSTRING
#include <cstring>
#endif

#if (UNIT_TESTS == 1)

# if defined(DEVEL_RELEASE)
#  error Unit testing not allowed in release
# endif

# ifdef __cplusplus

#  define SET_ASSERTION_FAILED_FLAG() arb_test::test_data().assertion_failed = true  
#  define PRINT_ASSERTION_FAILED_MSG(cond) arb_test::FlushedOutput::assertfailmsg(__FILE__, __LINE__, #cond)  

# else
#  define SET_ASSERTION_FAILED_FLAG() // impossible in C (assertions in C code will be handled like normal SEGV)
#  define PRINT_ASSERTION_FAILED_MSG(cond)                      \
    do {                                                        \
        fflush(stdout);                                         \
        fflush(stderr);                                         \
        fprintf(stderr, "%s:%i: Assertion '%s' failed [C]\n",   \
                __FILE__, __LINE__, #cond);                     \
        fflush(stderr);                                         \
    } while(0)
# endif

# define TRIGGER_ASSERTION()                            \
    do {                                                \
        SET_ASSERTION_FAILED_FLAG();                    \
        ARB_SIGSEGV(0);                                 \
    } while(0)

namespace arb_test {
    class GlobalTestData {
        GlobalTestData()
            : show_warnings(true),
              assertion_failed(false),
              warnings(0)
        {}

        static GlobalTestData *instance(bool erase) {
            static GlobalTestData *data = 0; // singleton
            if (erase) {
                delete data;
                data = 0;
            }
            else {
                if (!data) data = new GlobalTestData;
            }
            return data;
        }

    public:
        bool show_warnings;
        bool assertion_failed;

        // counters
        size_t warnings;

        static GlobalTestData& get_instance() { return *instance(false); }
        static void erase_instance() { instance(true); }
    };

    inline GlobalTestData& test_data() { return GlobalTestData::get_instance(); }

    
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
        static void assertfailmsg(const char *filename, int lineno, const char *condition) {
            FlushedOutput yes;
            fprintf(stderr, "%s:%i: Assertion '%s' failed", filename, lineno, condition);
            fputc('\n', stderr);
        }
        static void messagef(const char *filename, int lineno, const char *format, ...) __attribute__((format(printf, 3, 4))) {
            FlushedOutput yes;
            fprintf(stderr, "%s:%i: ", filename, lineno);
            fputc('\n', stderr);
            VPRINTFORMAT(format);
        }
        static void warningf(const char *filename, int lineno, const char *format, ...) __attribute__((format(printf, 3, 4))) {
            GlobalTestData& global = test_data();
            if (global.show_warnings) {
                FlushedOutput yes;
                fprintf(stderr, "%s:%i: Warning: ", filename, lineno);
                VPRINTFORMAT(format);
                fputc('\n', stderr);
                global.warnings++;
            }
        }
        static void errorf(const char *filename, int lineno, const char *format, ...) __attribute__((format(printf, 3, 4))) {
            FlushedOutput yes;
            fprintf(stderr, "%s:%i: Error: ", filename, lineno);
            VPRINTFORMAT(format);
            fputc('\n', stderr);
            TRIGGER_ASSERTION(); // fake an assertion failure
        }
#undef VPRINTFORMAT
    };

};

// --------------------------------------------------------------------------------

// special assert for unit tests (additionally to SEGV it sets a global flag)
# define test_assert(cond)                      \
    do {                                        \
        if (!(cond)) {                          \
            PRINT_ASSERTION_FAILED_MSG(cond);   \
            TRIGGER_ASSERTION();                \
        }                                       \
    } while(0)


// redefine arb_assert with test_assert when compiling for unit tests
# if defined(ASSERTION_USED)
#  undef arb_assert
#  define arb_assert(cond) test_assert(cond)
# endif

#else // UNIT_TESTS != 1
#error test_global.h may only be included if UNIT_TESTS is 1
#endif

#else
#error test_global.h included twice
#endif // TEST_GLOBAL_H
