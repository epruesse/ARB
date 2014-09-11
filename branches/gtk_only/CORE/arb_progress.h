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
#ifndef ARB_ERROR_H
#include <arb_error.h>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif

#if defined(DEBUG)
#define TEST_COUNTERS
#if defined(DEVEL_RALF)
#define DUMP_PROGRESS
#endif
#endif


class  arb_parent_progress;
struct arb_status_implementation;

class arb_progress_counter : virtual Noncopyable {
protected:
    arb_parent_progress *progress;
public:
    arb_progress_counter(arb_parent_progress *progress_)
        : progress(progress_)
    {}
    virtual ~arb_progress_counter() {}

    virtual void inc()                              = 0;
    virtual void implicit_inc()                     = 0;
    virtual void inc_to(int x)                      = 0;
    virtual void child_updates_gauge(double gauge)  = 0;
    virtual void done()                             = 0;
    virtual void force_update()                     = 0;
    virtual void restart(int overall_count)         = 0;
    virtual void auto_subtitles(const char *prefix) = 0;
    virtual bool has_auto_subtitles()               = 0;

#if defined(DUMP_PROGRESS)
    virtual void dump() = 0;
#endif

    virtual arb_progress_counter *clone(arb_parent_progress *parent, int overall_count) const = 0;
};

const int LEVEL_TITLE    = 0;
const int LEVEL_SUBTITLE = 1;

class arb_parent_progress : virtual Noncopyable {
    arb_parent_progress *prev_recent;
    bool                 reuse_allowed; // display may be reused by childs

protected:
#if defined(DUMP_PROGRESS)
    char *name;
#endif
    
    bool                  has_title;
    arb_progress_counter *counter;

    static arb_parent_progress       *recent;
    static arb_status_implementation *impl; // defines implementation to display status

    virtual SmartPtr<arb_parent_progress> create_child_progress(const char *title, int overall_count) = 0;

    arb_parent_progress(arb_progress_counter *counter_, bool has_title_) 
        : reuse_allowed(false),
          has_title(has_title_),
          counter(counter_)
    {
        prev_recent = recent;
        recent      = this;
        
#if defined(DUMP_PROGRESS)
        name = NULL;
#endif

#if defined(TEST_COUNTERS)
        accept_invalid_counters = false;
#endif
    }
public:
#if defined(TEST_COUNTERS)
    bool accept_invalid_counters; // if true, do not complain about unfinished counters
#endif

    virtual ~arb_parent_progress() {
        delete counter;
        recent = prev_recent;
#if defined(DUMP_PROGRESS)
        free(name);
#endif
    }

    static SmartPtr<arb_parent_progress> create(const char *title, int overall_count);
    static SmartPtr<arb_parent_progress> create_suppressor();

    virtual bool aborted() const                       = 0;
    virtual void set_text(int level, const char *text) = 0;
    virtual void update_gauge(double gauge)            = 0;

#if defined(DUMP_PROGRESS)
    virtual void dump();
#endif

    void child_sets_text(int level, const char *text) {
        set_text(level+1-reuse_allowed+counter->has_auto_subtitles(), text);
    }
    void allow_title_reuse() { reuse_allowed = true; }

    void child_updates_gauge(double gauge) { counter->child_updates_gauge(gauge); }
    void child_terminated() { counter->implicit_inc(); }

    arb_progress_counter *clone_counter(int overall_count) { return counter->clone(this, overall_count); }
    void initial_update() { counter->force_update(); }
    void force_update() { counter->force_update(); }

    void inc() { counter->inc(); }
    void inc_to(int x) { counter->inc_to(x); }
    void done() { counter->done(); }
    void auto_subtitles(const char *prefix) { counter->auto_subtitles(prefix); }

    static void show_comment(const char *comment) {
        if (recent) recent->set_text(LEVEL_SUBTITLE, comment);
    }
};

class arb_progress {
    SmartPtr<arb_parent_progress> used;

    void setup(const char *title, int overall_count) {
        used = arb_parent_progress::create(title, overall_count);
        used->initial_update();
    }
    // cppcheck-suppress functionConst
    void accept_invalid_counters() {
#if defined(TEST_COUNTERS)
        used->accept_invalid_counters = true;
#endif
    }

public:
    // ------------------------------
    // recommended interface:

    arb_progress(const char *title, int overall_count) {
        // open a progress indicator
        // 
        // expects to be incremented 'overall_count' times
        //      incrementation is done either
        //      - explicitly by calling one of the inc...()-functions below or
        //      - implicitely by creating another arb_progress while this one remains
        //
        // if you can't ahead-determine the exact number of incrementations,
        // specify an upper-limit and call .done() before dtor.

        arb_assert(overall_count >= 0);
        setup(title, overall_count);
    }
    explicit arb_progress(const char *title) {
        // open a wrapping progress indicator
        //
        // expects NOT to be incremented explicitly!
        //      if arb_progresses are created while this exists, they reuse the progress window.
        //      Useful to avoid spamming the user with numerous popping-up progress windows.
        setup(title, 0);
    }
    explicit arb_progress(int overall_count) {
        // open counting progress (reuses text of parent progress).
        // 
        // Useful to separate text- from gauge-display or
        // to summarize several consecutive child-progresses into one.
        setup(NULL, overall_count);
    }
    arb_progress() {
        // plain wrapper (avoids multiple implicit increments of its parent).
        // 
        // usage-conditions:
        // * caller increments progress in a loop and
        // * loop calls one or more callees and callees open multiple progress bars.
        // 
        // in this case the parent progress would be implicitely incremented several times
        // per loop, resulting in wrong gauge.
        //
        // if you know the number of opened progresses, use arb_progress(int),
        // otherwise add one wrapper-progress into the loop.
        setup(NULL, 0);
    }

    void allow_title_reuse() { used->allow_title_reuse(); }

    void subtitle(const char *stitle) { used->set_text(LEVEL_SUBTITLE, stitle); }

    GB_ERROR error_if_aborted() {
        return aborted() ? "Operation aborted on user request" : NULL;
    }

    GB_ERROR inc_and_error_if_aborted() {
        inc();
        return error_if_aborted();
    }

    void inc_and_check_user_abort(GB_ERROR& error)  { if (!error) error = inc_and_error_if_aborted(); else accept_invalid_counters(); }
    void inc_and_check_user_abort(ARB_ERROR& error) { if (!error) error = inc_and_error_if_aborted(); else accept_invalid_counters(); }

    bool aborted() {
        // true if user pressed "Abort"
        bool aborted_ = used->aborted();
#if defined(TEST_COUNTERS)
        if (aborted_) accept_invalid_counters();
#endif
        return aborted_;
    }

    void auto_subtitles(const char *prefix) {
        // automatically update subtitles ("prefix #/#")
        // prefix = NULL -> switch off
        used->auto_subtitles(prefix);
    }
    static void show_comment(const char *comment) {
        // Like subtitle(), but w/o needing to know anything about a eventually open progress
        // e.g. used to show ARB is connecting to ptserver
        arb_parent_progress::show_comment(comment);
    }
    
    // ------------------------------
    // less recommended interface:

    void inc() { used->inc(); } // increment progress
    const arb_progress& operator++() { inc(); return *this; } // ++progress

    void inc_by(int count) { arb_assert(count>0); while (count--) inc(); }
    void inc_to(int x) { used->inc_to(x); }

    void sub_progress_skipped() { used->child_terminated(); }

    void done() { used->done(); } // set "done" (aka 100%). Useful when exiting some loop early
#if defined(DUMP_PROGRESS)
    void dump();
#endif
    void force_update() { used->force_update(); }
};

class arb_suppress_progress {
    SmartPtr<arb_parent_progress> suppressor;
public:
    arb_suppress_progress()
        : suppressor(arb_parent_progress::create_suppressor())
    {}
};

#else
#error arb_progress.h included twice
#endif // ARB_PROGRESS_H
