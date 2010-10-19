// ============================================================== //
//                                                                //
//   File      : arb_backtrace.h                                  //
//   Purpose   :                                                  //
//                                                                //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2010   //
//   Institute of Microbiology (Technical University Munich)      //
//   http://www.arb-home.de/                                      //
//                                                                //
// ============================================================== //

#ifndef ARB_BACKTRACE_H
#define ARB_BACKTRACE_H

#define MAX_BACKTRACE 66

#ifndef _GLIBCXX_CSTDLIB
#include <cstdlib>
#endif
#ifndef _GLIBCXX_CSTDIO
#include <cstdio>
#endif
#ifndef _EXECINFO_H
#include <execinfo.h>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

class BackTraceInfo {
    void   **array;
    size_t  size;
public:
    BackTraceInfo(size_t skipFramesAtBottom) {
        void *tmp[MAX_BACKTRACE];
        size = backtrace(tmp, MAX_BACKTRACE);

        size_t ssize = skipFramesAtBottom*sizeof(*array);
        size_t msize = size*sizeof(*array) - ssize;

        arb_assert(msize>0);
        
        array = (void**)malloc(msize);
        memcpy(array, tmp+skipFramesAtBottom, msize);
    }
    ~BackTraceInfo() { free(array); }

    void dump(FILE *out, const char *message) {
        // print out all the frames to out
        fprintf(out, "\n-------------------- ARB-backtrace '%s':\n", message);
        backtrace_symbols_fd(array, size, fileno(out));
        if (size == MAX_BACKTRACE) fputs("[stack truncated to avoid deadlock]\n", out);
        fputs("-------------------- End of backtrace\n", out);
        fflush(out);
    }
};


#else
#error arb_backtrace.h included twice
#endif // ARB_BACKTRACE_H
