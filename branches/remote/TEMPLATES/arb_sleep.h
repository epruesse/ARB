// =========================================================== //
//                                                             //
//   File      : arb_sleep.h                                   //
//   Purpose   :                                               //
//                                                             //
//   Coded by Ralf Westram (coder@reallysoft.de) in May 2013   //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef ARB_SLEEP_H
#define ARB_SLEEP_H

#ifndef _UNISTD_H
#include <unistd.h>
#endif
#ifndef _GLIBCXX_ALGORITHM
#include <algorithm>
#endif
#ifndef _SYS_TIME_H
#include <sys/time.h>
#endif

enum TimeUnit { USEC = 1, MS = 1000, SEC = 1000*MS };

inline void GB_sleep(int amount, TimeUnit tu) { // @@@ rename into ARB_sleep
    usleep(amount*tu);
}

class ARB_inc_sleep {
    useconds_t curr_wait, max_wait, inc;

    void slowdown() { curr_wait = std::min(max_wait, curr_wait+inc); }
public:
    ARB_inc_sleep(int min_amount, int max_amount, TimeUnit tu, int increment)
        : curr_wait(min_amount*tu),
          max_wait(max_amount*tu),
          inc(increment*tu)
    {}

    void sleep() {
        fprintf(stderr, "pid %i waits %lu usec\n", getpid(), (unsigned long)curr_wait);
        usleep(curr_wait);
        slowdown();
    }
};

class ARB_timestamp {
    // timer which can be asked how much time passed since it was initialized
    timeval t1;

public:
    ARB_timestamp() { update(); }
    void update() { gettimeofday(&t1, NULL); }

    long usec_since() const {
        timeval t2;
        gettimeofday(&t2, NULL);
        return (t2.tv_sec - t1.tv_sec) * SEC + (t2.tv_usec - t1.tv_usec);
    }
    long ms_since() const { return usec_since()/MS; }
    long sec_since() const { return usec_since()/SEC; }
};

class ARB_timeout {
    // short timeout
    ARB_timestamp start;
    long          amount_usec;
public:
    ARB_timeout(int amount, TimeUnit tu)
        : amount_usec(amount*tu)
    {}
    bool passed() const {
        return start.usec_since()>=amount_usec;
    }
};


#else
#error arb_sleep.h included twice
#endif // ARB_SLEEP_H
