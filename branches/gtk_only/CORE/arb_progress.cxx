// ================================================================ //
//                                                                  //
//   File      : arb_progress.cxx                                   //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2010   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include <arb_progress.h>
#include <arb_handlers.h>
#include <arb_msg.h>
#include <algorithm>

// ----------------
//      counter

struct null_counter: public arb_progress_counter {
    null_counter(arb_parent_progress *progress_) : arb_progress_counter(progress_) {}

    void inc() OVERRIDE {}
    void implicit_inc() OVERRIDE {}
    void inc_to(int) OVERRIDE {}
    void done() OVERRIDE {}
    void restart(int) OVERRIDE {}
    void force_update() OVERRIDE {}
    void auto_subtitles(const char *) OVERRIDE {}
    bool has_auto_subtitles() OVERRIDE { return false; }
    void child_updates_gauge(double ) OVERRIDE {
        arb_assert(0); // wont
    }

#if defined(DUMP_PROGRESS)
    void dump() OVERRIDE {
        fprintf(stderr, "null_counter\n");
    }
#endif

    arb_progress_counter* clone(arb_parent_progress *parent_, int ) const OVERRIDE { return new null_counter(parent_); }
};

struct no_counter : public null_counter {
    no_counter(arb_parent_progress *progress_) : null_counter(progress_) {}
    void inc() OVERRIDE {
        arb_assert(0); // this is no_counter - so explicit inc() is prohibited!
    }
    void child_updates_gauge(double gauge) OVERRIDE { progress->update_gauge(gauge); }
    
#if defined(DUMP_PROGRESS)
    void dump() OVERRIDE {
        fprintf(stderr, "no_counter (=wrapper)\n");
    }
#endif
};

class concrete_counter: public arb_progress_counter { // derived from a Noncopyable
    int     explicit_counter; // incremented by calls to inc() etc.
    int     implicit_counter; // incremented by child_done()
    int     maxcount;         // == 0 -> does not really count (just a wrapper for child progresses)
    double  autoUpdateEvery;
    double  nextAutoUpdate;
    char   *auto_subtitle_prefix;
    int     last_auto_counter;

    int dispositive_counter() const { return std::max(implicit_counter, explicit_counter); }

    void init(int overallCount) {
        arb_assert(overallCount>0);

        implicit_counter  = 0;
        explicit_counter  = 0;
        maxcount          = overallCount;
        autoUpdateEvery   = overallCount/500.0; // update status approx. 500 times
        nextAutoUpdate    = 0;
    }

    bool refresh_if_needed(double my_counter) {
        if (my_counter<nextAutoUpdate) return false;
        progress->update_gauge(my_counter/maxcount);
        if (auto_subtitle_prefix) {
            int count = int(my_counter+1);
            if (count>last_auto_counter && count <= maxcount) {
                const char *autosub = GBS_global_string("%s #%i/%i", auto_subtitle_prefix, count, maxcount);
                progress->set_text(LEVEL_SUBTITLE, autosub);
                last_auto_counter   = count;
            }
        }
        nextAutoUpdate += autoUpdateEvery;
        return true;
    }
    void update_display_if_needed() {
        int dcount = dispositive_counter();
        arb_assert(dcount <= maxcount);
        refresh_if_needed(dcount);
    }

    void force_update() OVERRIDE {
        int oldNext    = nextAutoUpdate;;
        nextAutoUpdate = 0;
        update_display_if_needed();
        nextAutoUpdate = oldNext;
    }

public:
    concrete_counter(arb_parent_progress *parent, int overall_count)
        : arb_progress_counter(parent),
          auto_subtitle_prefix(NULL),
          last_auto_counter(0)
    {
        arb_assert(overall_count>0);
        init(overall_count);
    }
    ~concrete_counter() OVERRIDE {
        free(auto_subtitle_prefix);
#if defined(TEST_COUNTERS)
        if (!progress->accept_invalid_counters) {
            arb_assert(implicit_counter || explicit_counter); // progress was never incremented
            
            arb_assert(implicit_counter <= maxcount); // overflow
            arb_assert(explicit_counter <= maxcount); // overflow

            arb_assert(dispositive_counter() == maxcount); // progress did not finish
        }
#endif
    }

#if defined(DUMP_PROGRESS)
    void dump() OVERRIDE {
        fprintf(stderr,
                "concrete_counter: explicit=%i, implicit=%i, maxcount=%i\n", 
                explicit_counter, implicit_counter, maxcount);
    }
#endif

    void auto_subtitles(const char *prefix) OVERRIDE {
        arb_assert(!auto_subtitle_prefix);
        freedup(auto_subtitle_prefix, prefix);
        force_update();
    }
    bool has_auto_subtitles() OVERRIDE { return auto_subtitle_prefix; }

    void inc()          OVERRIDE { explicit_counter += 1; update_display_if_needed(); }
    void implicit_inc() OVERRIDE { implicit_counter += 1; update_display_if_needed(); }
    void inc_to(int x) {
        explicit_counter = std::max(explicit_counter, x);
        if (maxcount) {
            explicit_counter = std::min(explicit_counter, maxcount);
        }
        update_display_if_needed();
    }
    
    void done() OVERRIDE {
        implicit_counter = explicit_counter = maxcount;
        force_update();
    }

    void restart(int overallCount) OVERRIDE {
        init(overallCount);
        force_update();
    }

    arb_progress_counter* clone(arb_parent_progress *parent, int overall_count) const OVERRIDE {
        return new concrete_counter(parent, overall_count);
    }
    void child_updates_gauge(double child_gauge) OVERRIDE {
        refresh_if_needed(dispositive_counter()+child_gauge);
    }
};

// -----------------
//      progress


class child_progress : public arb_parent_progress { // derived from a Noncopyable
    arb_parent_progress *parent;

public:
    child_progress(arb_parent_progress *parent_, const char *title, int overall_count)
        : arb_parent_progress(overall_count
                              ? (arb_progress_counter*)new concrete_counter(this, overall_count)
                              : (arb_progress_counter*)new no_counter(this),
                              title),
          parent(parent_)
    {
        set_text(LEVEL_TITLE, title);
#if defined(DUMP_PROGRESS)
        freedup(name, GBS_global_string("child: %s", title));
#endif
    }
    ~child_progress() OVERRIDE {
        parent->child_terminated();
    }

    SmartPtr<arb_parent_progress> create_child_progress(const char *title, int overall_count) OVERRIDE {
        return new child_progress(this, title, overall_count);
    }

#if defined(DUMP_PROGRESS)
    void dump() OVERRIDE {
        arb_parent_progress::dump();
        fprintf(stderr, "is child of\n");
        parent->dump();
    }
#endif

    bool aborted() const OVERRIDE { return parent->aborted(); }
    void set_text(int level, const char *text) OVERRIDE { parent->child_sets_text(level+has_title-1, text); }
    
    void update_gauge(double gauge) OVERRIDE { parent->child_updates_gauge(gauge); }
};

class initial_progress: public arb_parent_progress {
    bool user_abort;
    
public:
    initial_progress(const char *title, arb_progress_counter *counter_)
        : arb_parent_progress(counter_, title),
          user_abort(false)
    {
        if (!title) title = "..."; // use fake title (arb_parent_progress got no title, so it will be replaced by child title)
        impl->openstatus(title);
#if defined(DUMP_PROGRESS)
        freedup(name, GBS_global_string("initial: %s", title));
#endif
    }
    ~initial_progress() OVERRIDE {
        update_gauge(1.0); // due to numeric issues it often only counts up to 99.999%
        impl->closestatus();
    }

    SmartPtr<arb_parent_progress> create_child_progress(const char *title, int overall_count) OVERRIDE {
        return new child_progress(this, title, overall_count);
    }

    bool aborted() const OVERRIDE { return user_abort; }
    void set_text(int level, const char *text) OVERRIDE {
        if (!text) return;
        switch (level+has_title-1) {
            case LEVEL_TITLE: impl->set_title(text); break;
            case LEVEL_SUBTITLE: impl->set_subtitle(text); break;
        }
    }

    void update_gauge(double gauge) OVERRIDE { user_abort = impl->set_gauge(gauge); }
};

struct initial_wrapping_progress: public initial_progress {
    initial_wrapping_progress(const char *title)
        : initial_progress(title, new no_counter(this)) {}
};
struct initial_counting_progress : public initial_progress {
    initial_counting_progress(const char *title, int overall_count)
        : initial_progress(title, new concrete_counter(this, overall_count)) {}
};

struct null_progress: public arb_parent_progress {
    null_progress(arb_progress_counter *counter_)
        : arb_parent_progress(counter_, false)
    {
#if defined(DUMP_PROGRESS)
        freedup(name, "null_progress");
#endif
    }

    SmartPtr<arb_parent_progress> create_child_progress(const char*, int overall_count) OVERRIDE {
        return new null_progress(clone_counter(overall_count));
    }
    bool aborted() const OVERRIDE { return false; }
    void set_text(int,const char*) OVERRIDE {}
    void update_gauge(double) OVERRIDE {}
};

// -------------------------
//      progress factory

arb_parent_progress       *arb_parent_progress::recent = NULL;
arb_status_implementation *arb_parent_progress::impl   = NULL; // defines implementation to display status

SmartPtr<arb_parent_progress> arb_parent_progress::create(const char *title, int overall_count) {
    if (recent) {
        return recent->create_child_progress(title, overall_count);
    }

    impl = &active_arb_handlers->status;

    if (overall_count == 0) return new initial_wrapping_progress(title);
    return new initial_counting_progress(title, overall_count);

}

SmartPtr<arb_parent_progress> arb_parent_progress::create_suppressor() {
    return new null_progress(new null_counter(NULL));
}

// --------------------------
//      progress dumpers

#if defined(DUMP_PROGRESS)

// not inlined in header (otherwise they are missing while debugging)

void arb_parent_progress::dump() {
    fprintf(stderr, "progress '%s'\n", name);
    fprintf(stderr, "counter: ");
    counter->dump();
}
void arb_progress::dump() {
    fprintf(stderr, "--------------------\n");
    used->dump();
}

#endif

