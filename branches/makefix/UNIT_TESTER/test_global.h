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
#ifndef __STDARG_H
#error Need cstdarg included
#endif
#endif
#if !defined(_STDIO_H) && !defined(_STDIO_H_)
#error Need cstdio included
#endif
#if !defined(_STDLIB_H) && !defined(_STDLIB_H_)
#error Need cstdlib included
#endif
#if !defined(_ERRNO_H) && !defined(_SYS_ERRNO_H_)
#error Need cerrno included
#endif
#if !defined(_STRING_H) && !defined(_STRING_H_)
#error Need cstring included
#endif

#ifdef UNIT_TESTS

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

#define SEGV_INSIDE_TEST_STOP_OTHERWISE(backtrace)              \
    do {                                                        \
        if (RUNNING_TEST()) {                                   \
            ARB_SIGSEGV(backtrace);                             \
        }                                                       \
        else {                                                  \
            ARB_STOP(backtrace);                                \
        }                                                       \
    } while(0)

# define TRIGGER_ASSERTION(backtrace)                           \
    do {                                                        \
        SET_ASSERTION_FAILED_FLAG();                            \
        SEGV_INSIDE_TEST_STOP_OTHERWISE(backtrace);             \
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

    enum FlagAction {
        FLAG_RAISE,
        FLAG_IS_RAISED,
    };

    inline const char *fakeenv(const char *var) {
        // override some environment variables for unittests
        if (strcmp(var, "HOME") == 0) return "./homefake"; // normally should be $ARBHOME/UNIT_TESTER/run/homefake
        return NULL;
    }


    class GlobalTestData {
        typedef bool (*FlagCallback)(FlagAction action, const char *name);

        FlagCallback flag_cb;

        GlobalTestData()
            : flag_cb(NULL),
              annotation(NULL),
              show_warnings(true),
              assertion_failed(false),
              running_test(false),
              warnings(0)
        {}
        ~GlobalTestData() {
            unannotate();
        }
        GlobalTestData(const GlobalTestData&); // do not synthesize
        GlobalTestData& operator=(const GlobalTestData&); // do not synthesize

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
        bool entered_mutex_loop; // helper to avoid race-condition

        // counters
        size_t warnings;

        void raiseLocalFlag(const char *name) const {
            if (flag_cb) {
                flag_cb(FLAG_RAISE, name);
            }
            else {
                fputs("cannot raise local flag (called from outside test-code?)\n", stderr);
            }
        }
        void init(FlagCallback fc) { flag_cb = fc; }

        bool not_covered_by_test() const { return !running_test; }

        static GlobalTestData& get_instance() { return *instance(false); }
        static void erase_instance() { instance(true); }

        void annotate(const char *annotation_) {
            unannotate();
            annotation = annotation_ ? strdup(annotation_) : NULL;
        }
        const char *get_annotation() const { return annotation; }

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

#define TEST_ANNOTATE(annotation) arb_test::test_data().annotate(annotation)
#define RUNNING_TEST()            arb_test::test_data().running_test

// special assert for unit tests (additionally to SEGV it sets a global flag)
#define test_assert(cond,backtrace)             \
    do {                                        \
        if (!(cond)) {                          \
            PRINT_ASSERTION_FAILED_MSG(cond);   \
            TRIGGER_ASSERTION(backtrace);       \
        }                                       \
    } while(0)

// Redefine arb_assert with test_assert when compiling for unit tests.
// 
// Always request a backtrace because these assertions are unexpected and
// might require to recompile tests w/o deadlockguard just to determine
// the callers (using a debugger).

# if defined(ASSERTION_USED)
#  undef arb_assert
#  define arb_assert(cond) test_assert(cond, true)
# endif

#define UNCOVERED() test_assert(arb_test::test_data().not_covered_by_test(), false)
// the opposite (i.e. COVERED()) would be quite nice, but is not trivial or even impossible

#else // !UNIT_TESTS 
#error test_global.h may only be included if UNIT_TESTS is defined
#endif

#else
#error test_global.h included twice
#endif // TEST_GLOBAL_H
