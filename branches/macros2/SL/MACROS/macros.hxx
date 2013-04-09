// ============================================================= //
//                                                               //
//   File      : macros.hxx                                      //
//   Purpose   : macro interface                                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in March 2013   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef MACROS_HXX
#define MACROS_HXX

#ifndef AW_ROOT_HXX
#include <aw_root.hxx>
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

    GBDATA *get_gbmain() { return gbmain; }
};


class RecordingMacro;

class MacroRecorder : public BoundActionTracker { // derived from Noncopyable
    RecordingMacro *recording;
public:
    MacroRecorder(const char *application_id, GBDATA *gb_main_)
        : BoundActionTracker(application_id, gb_main_),
          recording(NULL)
    {}

    GB_ERROR start_recording(const char *file, const char *application_id, const char *stop_action_name, bool expand_existing);
    GB_ERROR stop_recording();
    GB_ERROR execute(GBDATA *gb_main, const char *file, AW_RCB1 execution_done_cb, AW_CL client_data);

    void track_action(const char *action_id) OVERRIDE;
    void track_awar_change(AW_awar *awar) OVERRIDE;
};

class ClientActionTracker : public BoundActionTracker {

public:
    ClientActionTracker(const char *application_id, GBDATA *gb_main_)
        : BoundActionTracker(application_id, gb_main_)
    {}

    void track_action(const char *action_id) OVERRIDE;
    void track_awar_change(AW_awar *awar) OVERRIDE;
};

// --------------------------------------------------------------------------------

BoundActionTracker *make_macro_recording_tracker(const char *client_id, GBDATA *gb_main);
void                configure_macro_recording(AW_root *aw_root, const char *client_id, GBDATA *gb_main);

inline BoundActionTracker *get_active_macro_recording_tracker(AW_root *aw_root) {
    UserActionTracker *tracker = aw_root->getTracker();
    return tracker ? dynamic_cast<BoundActionTracker*>(tracker) : NULL;
}

#if defined(ASSERTION_USED)
inline bool got_macro_ability(AW_root *aw_root) {
    // return true if aw_root has a BoundActionTracker
    return get_active_macro_recording_tracker(aw_root);
}
#endif


#else
#error macros.hxx included twice
#endif // MACROS_HXX
