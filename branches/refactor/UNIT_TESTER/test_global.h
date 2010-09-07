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
        }
#undef VPRINTFORMAT
    };

};

#else
#error test_global.h included twice
#endif // TEST_GLOBAL_H
