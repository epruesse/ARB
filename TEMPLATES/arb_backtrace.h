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
#ifndef ARBTOOLS_H
#include "arbtools.h"
#endif

#define ARB_CRASH_CODE(sig) (128+(sig))

class BackTraceInfo : virtual Noncopyable {
    void   **array;
    size_t   size;

public:

    static bool& suppress() {
        static bool suppress_ = false;
        return suppress_;
    }

    explicit BackTraceInfo(size_t skipFramesAtBottom) {
        void *tmp[MAX_BACKTRACE];
        size = backtrace(tmp, MAX_BACKTRACE);

        size_t wantedFrames = size-skipFramesAtBottom;

        arb_assert(wantedFrames>0); // skipped more than all frames

        size_t msize = wantedFrames*sizeof(*array);

        array = (void**)malloc(msize);
        // cppcheck-suppress arithOperationsOnVoidPointer (false positive: pointer-arithmetics on void** are completely standard compliant)
        memcpy(array, tmp+skipFramesAtBottom, msize);

        size = wantedFrames;
    }
    ~BackTraceInfo() { free(array); }

    bool dump(FILE *out, const char *message) const {
        // print out all the stack frames to 'out'
        // return true on success.

        if (fprintf(out, "\n-------------------- ARB-backtrace '%s':\n", message) < 0) return false;
        fflush(out);
        backtrace_symbols_fd(array, size, fileno(out));
        if (size == MAX_BACKTRACE) fputs("[stack truncated to avoid deadlock]\n", out);
        fputs("-------------------- End of backtrace\n", out);
        return fflush(out) == 0;
    }
};

inline void demangle_backtrace(const class BackTraceInfo& trace, FILE *out, const char *message) {
    if (!BackTraceInfo::suppress()) {
        static bool filtfailed = false;
        if (!filtfailed) {
            // @@@ Warning: this branch ignores parameter 'out'
            FILE *filt = popen("/usr/bin/c++filt", "w");
            if (filt) {
                filtfailed   = !trace.dump(filt, message);
                int exitcode = pclose(filt);
                if (exitcode != 0 && !filtfailed) filtfailed = true;
            }
            else filtfailed = true;
        }
        if (filtfailed) trace.dump(out, message);
    }
}



#else
#error arb_backtrace.h included twice
#endif // ARB_BACKTRACE_H
