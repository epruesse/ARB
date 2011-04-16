// ============================================================= //
//                                                               //
//   File      : test_helpers.h                                  //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in March 2011   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#ifndef ARB_STRBUF_H
#include <arb_strbuf.h>
#endif

template<typename FUN>
inline void streamIntFunResults(GBS_strstruct *out, FUN fun, int start, int end, int width) {
    for (int i = start; i <= end; ++i) {
        int res = fun(i);
        GBS_strnprintf(out, 20, "%*i", width, res);
    }
}
template<typename FUN>
inline void streamIntFunResultsInBrackets(GBS_strstruct *out, FUN fun, int start, int end, int width) {
    for (int i = start; i <= end; ++i) {
        int  res = fun(i);
        char buffer[30];
        sprintf(buffer, "[%i]", res);
        GBS_strnprintf(out, 30, "%*s", width+2, buffer);
    }
}

template<typename FUN>
inline char *collectIntFunResults(FUN fun, int start, int end, int width, int try_underflow, int try_overflow) {
    GBS_strstruct *out = GBS_stropen(1000);
    if (try_underflow) streamIntFunResultsInBrackets(out, fun, start-try_underflow, start-1, width);
    streamIntFunResults(out, fun, start, end, width);
    if (try_overflow) streamIntFunResultsInBrackets(out, fun, end+1, end+try_overflow, width);
    return GBS_strclose(out);
}

#else
#error test_helpers.h included twice
#endif // TEST_HELPERS_H
