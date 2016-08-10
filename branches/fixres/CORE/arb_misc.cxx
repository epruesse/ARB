// =============================================================== //
//                                                                 //
//   File      : arb_misc.cxx                                      //
//   Purpose   : misc that doesnt fit elsewhere                    //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2012   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "arb_misc.h"
#include "arb_msg.h"
#include "arb_file.h"
#include "arb_string.h"

#include <cmath>

// AISC_MKPT_PROMOTE:#ifndef _GLIBCXX_CSTDLIB
// AISC_MKPT_PROMOTE:#include <cstdlib>
// AISC_MKPT_PROMOTE:#endif

const char *GBS_readable_size(unsigned long long size, const char *unit_suffix) {
    // return human readable size information
    // returned string is maximal 6+strlen(unit) characters long
    // (using "b" as 'unit_suffix' produces '### b', '### Mb' etc)

    if (size<1000) return GBS_global_string("%llu %s", size, unit_suffix);

    const char *units = "kMGTPEZY"; // kilo, Mega, Giga, Tera, ... should be enough forever
    int i;

    for (i = 0; units[i]; ++i) {
        char unit = units[i];
        if (size<1000*1024) {
            double amount = size/(double)1024;
            if (amount<10.0)  return GBS_global_string("%4.2f %c%s", amount+0.005, unit, unit_suffix);
            if (amount<100.0) return GBS_global_string("%4.1f %c%s", amount+0.05, unit, unit_suffix);
            return GBS_global_string("%i %c%s", (int)(amount+0.5), unit, unit_suffix);
        }
        size /= 1024; // next unit
    }
    return GBS_global_string("MUCH %s", unit_suffix);
}

const char *GBS_readable_timediff(size_t seconds) {
    size_t mins  = seconds/60; seconds -= mins  * 60;
    size_t hours = mins/60;    mins    -= hours * 60;
    size_t days  = hours/24;   hours   -= days  * 24;

    const int   MAXPRINT = 40;
    int         printed  = 0;
    static char buffer[MAXPRINT+1];

    if (days>0)               printed += sprintf(buffer+printed, "%zud", days);
    if (printed || hours>0)   printed += sprintf(buffer+printed, "%zuh", hours);
    if (printed || mins>0)    printed += sprintf(buffer+printed, "%zum", mins);

    printed += sprintf(buffer+printed, "%zus", seconds);

    arb_assert(printed>0 && printed<MAXPRINT);

    return buffer;
}

const char *ARB_float_2_ascii(const float f) {
    /*! calculate the "best" ascii representation for float 'f'
     *  - smaller conversion error is better
     *  - shorter representation is better (for equal conversion errors)
     */

    const int   MAXSIZE = 50;
    static char result[MAXSIZE];
    char        buffer[MAXSIZE];

    int   printed_e = snprintf(result, MAXSIZE, "%e", f); arb_assert(printed_e<MAXSIZE);
    float back_e    = strtof(result, NULL);
    float diff_e    = fabsf(f-back_e);

    int   printed_g = snprintf(buffer, MAXSIZE, "%g", f); arb_assert(printed_g<MAXSIZE);
    float back_g    = strtof(buffer, NULL);
    float diff_g    = fabsf(f-back_g);

    if (diff_g<diff_e || (diff_g == diff_e && printed_g<printed_e)) {
        printed_e = printed_g;
        back_e    = back_g;
        diff_e    = diff_g;
        memcpy(result, buffer, printed_g+1);
    }

    int   printed_f = snprintf(buffer, MAXSIZE, "%f", f); arb_assert(printed_f<MAXSIZE);
    float back_f    = strtof(buffer, NULL);
    float diff_f    = fabsf(f-back_f);

    if (diff_f<diff_e || (diff_f == diff_e && printed_f<printed_e)) {
        memcpy(result, buffer, printed_f+1);
    }

    return result;
}

const char *ARB_getenv_ignore_empty(const char *envvar) {
    const char *result = getenv(envvar);
    return (result && result[0]) ? result : 0;
}

char *ARB_executable(const char *exe_name, const char *path) {
    char       *buffer = ARB_alloc<char>(strlen(path)+1+strlen(exe_name)+1);
    const char *start  = path;
    int         found  = 0;

    while (!found && start) {
        const char *colon = strchr(start, ':');
        int         len   = colon ? (colon-start) : (int)strlen(start);

        memcpy(buffer, start, len);
        buffer[len] = '/';
        strcpy(buffer+len+1, exe_name);

        found = GB_is_executablefile(buffer);
        start = colon ? colon+1 : 0;
    }

    char *executable = found ? ARB_strdup(buffer) : 0;
    free(buffer);
    return executable;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

#if 0
// simple test
#define TEST_EXPECT_FLOAT_2_ASCII(f,a) TEST_EXPECT_EQUAL(ARB_float_2_ascii(f), a)
#else
// also test back-conversion (ascii->float->ascii) is stable
#define TEST_EXPECT_FLOAT_2_ASCII(f,a) do{                          \
        TEST_EXPECT_EQUAL(ARB_float_2_ascii(f), a);                 \
        TEST_EXPECT_EQUAL(ARB_float_2_ascii(strtof(a, NULL)), a);   \
    }while(0)
#endif

     void TEST_float_2_ascii() {
    TEST_EXPECT_FLOAT_2_ASCII(3.141592e+00, "3.141592");
    TEST_EXPECT_FLOAT_2_ASCII(3.141592,     "3.141592");
    TEST_EXPECT_FLOAT_2_ASCII(3.14159,      "3.14159");

    TEST_EXPECT_FLOAT_2_ASCII(0.1,           "0.1");
    TEST_EXPECT_FLOAT_2_ASCII(0.01,          "0.01");
    TEST_EXPECT_FLOAT_2_ASCII(0.001,         "0.001");
    TEST_EXPECT_FLOAT_2_ASCII(0.0001,        "0.0001");
    TEST_EXPECT_FLOAT_2_ASCII(0.00001,       "1e-05");
    TEST_EXPECT_FLOAT_2_ASCII(0.000001,      "1e-06");
    TEST_EXPECT_FLOAT_2_ASCII(0.0000001,     "1e-07");
    TEST_EXPECT_FLOAT_2_ASCII(0.00000001,    "1e-08");
    TEST_EXPECT_FLOAT_2_ASCII(0.000000001,   "1e-09");
    TEST_EXPECT_FLOAT_2_ASCII(0.0000000001,  "1e-10");
    TEST_EXPECT_FLOAT_2_ASCII(0.00000000001, "1e-11");

    TEST_EXPECT_FLOAT_2_ASCII(10,         "10");
    TEST_EXPECT_FLOAT_2_ASCII(100,        "100");
    TEST_EXPECT_FLOAT_2_ASCII(1000,       "1000");
    TEST_EXPECT_FLOAT_2_ASCII(10000,      "10000");
    TEST_EXPECT_FLOAT_2_ASCII(100000,     "100000");
    TEST_EXPECT_FLOAT_2_ASCII(1000000,    "1e+06");
    TEST_EXPECT_FLOAT_2_ASCII(10000000,   "1e+07");
    TEST_EXPECT_FLOAT_2_ASCII(100000000,  "1e+08");
    TEST_EXPECT_FLOAT_2_ASCII(1000000000, "1e+09");

    TEST_EXPECT_FLOAT_2_ASCII(3141592,      "3.141592e+06");
    TEST_EXPECT_FLOAT_2_ASCII(314159.2,     "3.141592e+05");
    TEST_EXPECT_FLOAT_2_ASCII(31415.92,     "3.141592e+04");
    TEST_EXPECT_FLOAT_2_ASCII(3141.592,     "3141.592041");
    TEST_EXPECT_FLOAT_2_ASCII(3141.592041,  "3141.592041");
    TEST_EXPECT_FLOAT_2_ASCII(314.1592,     "314.159210");
    TEST_EXPECT_FLOAT_2_ASCII(314.159210,   "314.159210");
    TEST_EXPECT_FLOAT_2_ASCII(31.41592,     "31.415920");
    TEST_EXPECT_FLOAT_2_ASCII(3.141592,     "3.141592");
    TEST_EXPECT_FLOAT_2_ASCII(.3141592,     "3.141592e-01");
    TEST_EXPECT_FLOAT_2_ASCII(.03141592,    "3.141592e-02");
    TEST_EXPECT_FLOAT_2_ASCII(.003141592,   "3.141592e-03");
    TEST_EXPECT_FLOAT_2_ASCII(.0003141592,  "3.141592e-04");
    TEST_EXPECT_FLOAT_2_ASCII(.00003141592, "3.141592e-05");
    TEST_EXPECT_FLOAT_2_ASCII(M_PI,         "3.141593");

    TEST_EXPECT_FLOAT_2_ASCII(1/2.0, "0.5");
    TEST_EXPECT_FLOAT_2_ASCII(1/3.0, "3.333333e-01");
    TEST_EXPECT_FLOAT_2_ASCII(1/4.0, "0.25");
    TEST_EXPECT_FLOAT_2_ASCII(1/5.0, "0.2");
    TEST_EXPECT_FLOAT_2_ASCII(1/6.0, "1.666667e-01");

    TEST_EXPECT_FLOAT_2_ASCII(37550000.0,  "3.755e+07");
    TEST_EXPECT_FLOAT_2_ASCII(3755000.0,   "3.755e+06");
    TEST_EXPECT_FLOAT_2_ASCII(375500.0,    "375500");
    TEST_EXPECT_FLOAT_2_ASCII(37550.0,     "37550");
    TEST_EXPECT_FLOAT_2_ASCII(3755.0,      "3755");
    TEST_EXPECT_FLOAT_2_ASCII(375.5,       "375.5");
    TEST_EXPECT_FLOAT_2_ASCII(37.55,       "37.55");
    TEST_EXPECT_FLOAT_2_ASCII(3.755,       "3.755");
    TEST_EXPECT_FLOAT_2_ASCII(0.3755,      "0.3755");
    TEST_EXPECT_FLOAT_2_ASCII(0.03755,     "0.03755");
    TEST_EXPECT_FLOAT_2_ASCII(0.003755,    "0.003755");
    TEST_EXPECT_FLOAT_2_ASCII(0.0003755,   "0.0003755");
    TEST_EXPECT_FLOAT_2_ASCII(0.00003755,  "3.755e-05");
    TEST_EXPECT_FLOAT_2_ASCII(0.000003755, "3.755e-06");

    TEST_EXPECT_FLOAT_2_ASCII(1000.0*1000.0*1000.0,    "1e+09");
    TEST_EXPECT_FLOAT_2_ASCII(25000.0*25000.0*25000.0, "1.5625e+13");
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

