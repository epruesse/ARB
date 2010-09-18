// ================================================================= //
//                                                                   //
//   File      : aw_status.hxx                                       //
//   Purpose   : Provide aw_status and related functions             //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2010   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef AW_STATUS_HXX
#define AW_STATUS_HXX

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif


#ifndef aw_assert
#define aw_assert(bed) arb_assert(bed)
#endif

int aw_status(const char *text); // return 1 if exit button is pressed + set statustext
int aw_status(double gauge);     // return 1 if exit button is pressed + set progress bar
int aw_status();                 // return 1 if exit button is pressed

// ---------------------
//      progress bar

void aw_initstatus();                  // call this function only once as early as possible
void aw_openstatus(const char *title); // show status
void aw_closestatus();                 // hide status

class aw_status_counter : Noncopyable { // progress object
    int counter;
    int maxcount;
    int autoUpdateEvery;
    int nextAutoUpdate;
    bool aborted;

    void track_abort(int aw_status_result) { if (aw_status_result) aborted = true; }
    void init(int overallCount) {
        counter         = 0;
        maxcount        = overallCount;
        autoUpdateEvery = overallCount <= 250 ? 1 : int(overallCount/250.0+0.5);
        nextAutoUpdate  = counter+autoUpdateEvery;
    }

public:
    aw_status_counter(int overallCount) : aborted(false) {
        init(overallCount);
    }

    void inc(int incr = 1) {
        counter += incr;
        aw_assert(counter <= maxcount);
        if (counter >= nextAutoUpdate) {
            update();
            nextAutoUpdate += autoUpdateEvery;
        }
    }
    void update() { track_abort(aw_status(counter/double(maxcount))); }

    bool aborted_by_user() const { return aborted; }
    void restart(int overallCount) { init(overallCount); update(); }
};


#else
#error aw_status.hxx included twice
#endif // AW_STATUS_HXX
