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

// do not include here - just test
// insert includes at ../INCLUDE/arb_assert.h@WhyIncludeHere
#ifndef _STDARG_H
#error Need cstdarg included
#endif
#ifndef _STDIO_H
#error Need cstdio included
#endif
#ifndef _STDLIB_H
#error Need cstdlib included
#endif
#ifndef _ERRNO_H
#error Need cerrno included
#endif
#ifndef _STRING_H
#error Need cstring included
#endif

#if (UNIT_TESTS == 1)

# if defined(DEVEL_RELEASE)
#  error Unit testing not allowed in release
# endif

# ifdef __cplusplus

#  define SET_ASSERTION_FAILED_FLAG() arb_test::test_data().assertion_failed = true  
#  define PRINT_ASSERTION_FAILED_MSG(cond) arb_test::GlobalTestData::assertfailmsg(__FILE__, __LINE__, #cond)  

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
    class FlushedOutputNoLF {
        inline void flushall() { fflush(stdout); fflush(stderr); }
    public:
        FlushedOutputNoLF() { flushall(); }
        ~FlushedOutputNoLF() { flushall(); }
    };
    struct FlushedOutput : public FlushedOutputNoLF {
        ~FlushedOutput() { fputc('\n', stderr); }
    };

    class GlobalTestData {
        GlobalTestData()
            : annotation(NULL),
              show_warnings(true),
              assertion_failed(false),
              running_test(false),
              warnings(0)
        {}
        ~GlobalTestData() {
            unannotate();
        }

        static GlobalTestData *instance(bool erase) {
            static GlobalTestData *data = 0;             // singleton
            if (erase) {
                delete data;
                data = 0;
            }
            else if (!data) {
                static int allocation_count = 0;
                arb_assert(allocation_count == 0); // allocating GlobalTestData twice is a bug!
                allocation_count++;

                data = new GlobalTestData;
            }
            return data;
        }

        char *annotation; // shown in assertion-failure message

        void unannotate() {
            free(annotation);
            annotation = NULL;
        }

    public:
        bool show_warnings;
        bool assertion_failed;
        bool running_test;

        // counters
        size_t warnings;

        static GlobalTestData& get_instance() { return *instance(false); }
        static void erase_instance() { instance(true); }

        void annotate(const char *annotation_) {
            unannotate();
            annotation = annotation_ ? strdup(annotation_) : NULL;
        }

        static void print_annotation() {
            char*& annotation = get_instance().annotation;
            if (annotation) fprintf(stderr, " (%s)", annotation);
        }

        static void assertfailmsg(const char *filename, int lineno, const char *condition) {
            FlushedOutput yes;
            fprintf(stderr, "%s:%i: Assertion '%s' failed", filename, lineno, condition);
            print_annotation();
        }
    };

    inline GlobalTestData& test_data() { return GlobalTestData::get_instance(); }
};

// --------------------------------------------------------------------------------

#define TEST_ANNOTATE_ASSERT(annotation) arb_test::test_data().annotate(annotation)
#define RUNNING_TEST()                   arb_test::test_data().running_test

// special assert for unit tests (additionally to SEGV it sets a global flag)
#define test_assert(cond)                       \
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
