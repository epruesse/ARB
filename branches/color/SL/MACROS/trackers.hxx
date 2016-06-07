// ============================================================= //
//                                                               //
//   File      : trackers.hxx                                    //
//   Purpose   : action trackers                                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in March 2013   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef TRACKERS_HXX
#define TRACKERS_HXX

#ifndef AW_ROOT_HXX
#include <aw_root.hxx>
#endif
#ifndef MACROS_LOCAL_HXX
#include "macros_local.hxx"
#endif

#define ma_assert(bed) arb_assert(bed)

class RequiresActionTracker : public UserActionTracker {
    // RequiresActionTracker is a failing placeholder for the explicit tracker,
    // that has to be added later (when DB is available)
    //
    // Always "records", i.e. fails instantly on first GUI-action-callback
    __ATTR__NORETURN inline void needs_to_be_replaced() {
        // this tracker needs to be replaced before any UI action is performed
        GBK_terminate("Broken macro recording ability (no valid tracker)");
    }
public:
    RequiresActionTracker() {
        set_tracking(true); // always track
    }
    __ATTR__NORETURN void track_action(const char *) OVERRIDE { needs_to_be_replaced(); }
    __ATTR__NORETURN void track_awar_change(AW_awar *) OVERRIDE { needs_to_be_replaced(); }
    bool is_replaceable() const OVERRIDE { return true; }
};

class BoundActionTracker : public UserActionTracker, virtual Noncopyable {
    char   *id;     // application id (e.g. "ARB_NT", "ARB_EDIT4", ...)
    GBDATA *gbmain; // DB used to record/playback macros

    void set_tracking(bool track) { UserActionTracker::set_tracking(track); }

protected:
    void set_recording(bool recording);

public:
    BoundActionTracker(const char *application_id, GBDATA *gb_main)
        : id(strdup(application_id)),
          gbmain(gb_main)
    {}
    virtual ~BoundActionTracker() OVERRIDE {
        free(id);
    }

    bool is_replaceable() const OVERRIDE { return false; }
    bool reconfigure(const char *application_id, GBDATA *gb_main);

    GBDATA *get_gbmain() { ma_assert(gbmain); return gbmain; }
    const char *get_application_id() const { return id; }

    void forgetDatabase() { gbmain = NULL; }
    virtual void release() = 0;
};


class RecordingMacro;

class MacroRecorder : public BoundActionTracker { // derived from Noncopyable
    RecordingMacro *recording;

public:
    MacroRecorder(const char *application_id, GBDATA *gb_main_)
        : BoundActionTracker(application_id, gb_main_),
          recording(NULL)
    {}
    ~MacroRecorder() {
        ma_assert(!recording);
    }

    GB_ERROR start_recording(const char *file, const char *stop_action_name, bool expand_existing);
    GB_ERROR stop_recording();
    GB_ERROR execute(const char *macroFile, bool loop_marked, const RootCallback& execution_done_cb);

    void track_action(const char *action_id) OVERRIDE;
    void track_awar_change(AW_awar *awar) OVERRIDE;

    GB_ERROR handle_tracked_client_action(char *&tracked); // dont use
    void release() OVERRIDE {
        if (is_tracking()) {
            GB_ERROR error = stop_recording();
            if (error) fprintf(stderr, "Error in stop_recording: %s (while exiting server)\n", error);
        }
        BoundActionTracker::forgetDatabase();
    }
};

class ClientActionTracker : public BoundActionTracker {
    bool released;

    void bind_callbacks(bool install);
    void send_client_action(const char *action);
    void ungrant_client_and_confirm_quit_action();
public:
    ClientActionTracker(const char *application_id, GBDATA *gb_main_)
        : BoundActionTracker(application_id, gb_main_),
          released(false)
    {
        bind_callbacks(true);
    }
    ~ClientActionTracker() {
        ma_assert(released); // you have to call release() before the dtor is called
    }

    void release() OVERRIDE {
        if (!released) {
            bind_callbacks(false);
            ungrant_client_and_confirm_quit_action();
            BoundActionTracker::forgetDatabase();
            released = true;
        }
    }

    void track_action(const char *action_id) OVERRIDE;
    void track_awar_change(AW_awar *awar) OVERRIDE;

    void set_tracking_according_to(GBDATA *gb_recording); // dont use
};

// --------------------------------------------------------------------------------

inline BoundActionTracker *get_active_macro_recording_tracker(AW_root *aw_root) {
    UserActionTracker *tracker = aw_root->getTracker();
    return tracker ? dynamic_cast<BoundActionTracker*>(tracker) : NULL;
}

inline MacroRecorder *getMacroRecorder(AW_root *aw_root) {
    BoundActionTracker *tracker = get_active_macro_recording_tracker(aw_root);
    ma_assert(tracker); // application is not able to handle macros
    return dynamic_cast<MacroRecorder*>(tracker);
}

// --------------------------------------------------------------------------------

#else
#error trackers.hxx included twice
#endif // TRACKERS_HXX
