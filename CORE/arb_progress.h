// ================================================================ //
//                                                                  //
//   File      : arb_progress.h                                     //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2010   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef ARB_PROGRESS_H
#define ARB_PROGRESS_H

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

struct arb_status_implementation {
    void (*openstatus)(const char *title);   // opens the status window and sets title
    void (*closestatus)();                   // close the status window
    bool (*set_subtitle)(const char *title); // set the subtitle (return true on user abort)
    bool (*set_gauge)(double gauge);         // set the gauge (=percent) (return true on user abort)
    bool (*user_abort)();                    // return true on user abort
};

extern arb_status_implementation ARB_basic_status;
extern arb_status_implementation ARB_null_status;

class arb_progress {
    static arb_status_implementation *impl;
    static arb_progress *opened;

    int counter;
    int maxcount;
    int autoUpdateEvery;
    int nextAutoUpdate;
    bool user_abort;

    void track_abort(bool status_result) { if (status_result) user_abort = true; }
    void init(int overallCount) {
        counter         = 0;
        maxcount        = overallCount;
        autoUpdateEvery = overallCount <= 250 ? 1 : int(overallCount/250.0+0.5);
        nextAutoUpdate  = counter+autoUpdateEvery;
    }

    void update() { track_abort(impl->set_gauge(counter/double(maxcount))); }

public:
    arb_progress(const char *title, int overall_count)
        : user_abort(false)
    {
        init(overall_count);
        impl->openstatus(title);
        update();
    }
    ~arb_progress() { impl->closestatus(); }

    void inc(int incr = 1) {
        counter += incr;
        arb_assert(counter <= maxcount);
        if (counter >= nextAutoUpdate) {
            update();
            nextAutoUpdate += autoUpdateEvery;
        }
    }
    const arb_progress& operator++() { inc(); return *this; }
    bool aborted() const { return user_abort; }
    void restart(int overallCount) { init(overallCount); update(); }
};

#else
#error arb_progress.h included twice
#endif // ARB_PROGRESS_H
