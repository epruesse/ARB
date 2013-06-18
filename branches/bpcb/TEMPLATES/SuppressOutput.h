// ================================================================ //
//                                                                  //
//   File      : SuppressOutput.h                                   //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2010   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef SUPPRESSOUTPUT_H
#define SUPPRESSOUTPUT_H

#ifndef _GLIBCXX_CSTDIO
#include <cstdio>
#endif
#ifndef ARBTOOLS_H
#include "arbtools.h"
#endif


class SuppressOutput : virtual Noncopyable {
    // temporarily redirect stdout and stderr to /dev/null
    FILE *devnull;
    FILE *org_stdout;
    FILE *org_stderr;

    void flush() {
        fflush(stdout);
        fflush(stderr);
    }

public:
    void suppress() {
        if (stdout != devnull) {
            flush();
            stdout = devnull;
            stderr = devnull;
        }
    }
    void dont_suppress() {
        if (stdout == devnull) {
            flush();
            stdout = org_stdout;
            stderr = org_stderr;
        }
    }

    SuppressOutput()
        : devnull(fopen("/dev/null", "w"))
        , org_stdout(stdout)
        , org_stderr(stderr)
    {
        suppress();
    }
    ~SuppressOutput() {
        dont_suppress();
        fclose(devnull);
    }
};

#else
#error SuppressOutput.h included twice
#endif // SUPPRESSOUTPUT_H
