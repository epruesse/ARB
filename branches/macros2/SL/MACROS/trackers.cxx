// ============================================================= //
//                                                               //
//   File      : trackers.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in March 2013   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "trackers.hxx"
#include "macros.hxx"
#include "recmac.hxx"

#include <arbdbt.h>
#include <aw_msg.hxx>

bool BoundActionTracker::reconfigure(const char *application_id, GBDATA *gb_main) {
    ma_assert(gb_main == gbmain);
    ma_assert(strcmp(id, "ARB_IMPORT") == 0); // currently only ARB_IMPORT-tracker gets reconfigured
    freedup(id, application_id);
    return true;
}

GB_ERROR MacroRecorder::start_recording(const char *file, const char *stop_action_name, bool expand_existing) {
    GB_ERROR error = NULL;
    if (is_tracking()) error = "Already recording macro";
    else {
        recording = new RecordingMacro(file, get_application_id(), stop_action_name, expand_existing);
        set_tracking(true);

        error = recording->has_error();
        if (error) stop_recording();
    }
    return error;
}

GB_ERROR MacroRecorder::stop_recording() {
    GB_ERROR error = NULL;
    if (!is_tracking()) {
        error = "Not recording macro";
    }
    else {
        error = recording->stop();

        delete recording;
        recording = NULL;
        set_tracking(false);
    }
    return error;
}

class ExecutingMacro {
    AW_RCB1 done_cb;
    AW_CL   cd;

    ExecutingMacro        *next;
    static ExecutingMacro *head;

    ExecutingMacro(AW_RCB1 execution_done_cb, AW_CL client_data)
        : done_cb(execution_done_cb),
          cd(client_data),
          next(head)
    {
        head = this;
    }

    void call() const { done_cb(AW_root::SINGLETON, cd); }
    void destroy() { head = next; delete this; }

public:

    static void add(AW_RCB1 execution_done_cb, AW_CL client_data) { new ExecutingMacro(execution_done_cb, client_data); }
    static void done() {
        // ma_assert(head); // fails when a macro is called from command line
        if (head) {
            head->call();
            head->destroy();
        }
    }
};

ExecutingMacro *ExecutingMacro::head = NULL;

static void macro_terminated(GBDATA */*gb_terminated*/, int *, GB_CB_TYPE IF_ASSERTION_USED(cb_type)) {
    ma_assert(cb_type == GB_CB_CHANGED);
    ExecutingMacro::done();
}

static void dont_announce_done(AW_root*, AW_CL) {}

GB_ERROR MacroRecorder::execute(const char *file, AW_RCB1 execution_done_cb, AW_CL client_data) {
    GB_ERROR  error = NULL;
    char     *path;
    if (file[0] == '/') {
        path = strdup(file);
    }
    else {
        path = GBS_global_string_copy("%s/%s", GB_getenvARBMACROHOME(), file);
    }

    {
        GBDATA         *gb_main = get_gbmain();
        GB_transaction  ta(gb_main);

        GBDATA *gb_term = GB_search(gb_main, MACRO_TRIGGER_CONTAINER "/terminated", GB_FIND);
        if (!gb_term) {
            gb_term = GB_search(gb_main, MACRO_TRIGGER_CONTAINER "/terminated", GB_INT);
            if (!gb_term) {
                error = GB_await_error();
            }
            else {
                GB_add_callback(gb_term, GB_CB_CHANGED, macro_terminated, 0);
            }
        }
        error = ta.close(error);
    }

    if (!error) {
        if (!execution_done_cb) execution_done_cb = dont_announce_done;
        ExecutingMacro::add(execution_done_cb, client_data);

        const char *com = GBS_global_string("perl %s &", path);
        printf("[Action '%s']\n", com);
        if (system(com)) { // async(!) call to macro
            aw_message(GBS_global_string("Calling '%s' failed", com));
        }
        free(path);
    }
    return error;
}

void MacroRecorder::track_action(const char *action_id) {
    ma_assert(is_tracking());
    recording->track_action(action_id);
}

void MacroRecorder::track_awar_change(AW_awar *awar) {
    ma_assert(is_tracking());
    recording->track_awar_change(awar);
}


void ClientActionTracker::track_action(const char */*action_id*/) {
    ma_assert(0);
}

void ClientActionTracker::track_awar_change(AW_awar */*awar*/) {
    ma_assert(0);
}

// -------------------------
//      tracker factory

UserActionTracker *make_macro_recording_tracker(const char *client_id, GBDATA *gb_main) {
    // 'client_id' has to be a unique id (used to identify the program which will record/playback).
    // If multiple programs (or multiple instances of one) use the same id, macro recording shall abort.
    // If a program is used for different purposes by starting multiple instances (like e.g. arb_ntree),
    // each purpose/instance should use a different 'client_id'.

    ma_assert(gb_main);
    ma_assert(client_id && client_id[0]);

    BoundActionTracker *tracker;
    if (GB_is_server(gb_main)) {
        tracker = new MacroRecorder(client_id, gb_main);
    }
    else {
        tracker = new ClientActionTracker(client_id, gb_main);
    }
    return tracker;
}

UserActionTracker *need_macro_ability() {
    return new RequiresActionTracker;
}

void configure_macro_recording(AW_root *aw_root, const char *client_id, GBDATA *gb_main) {
    ma_assert(aw_root);

    BoundActionTracker *existing = get_active_macro_recording_tracker(aw_root);
    if (existing && existing->reconfigure(client_id, gb_main)) return;
    aw_root->setUserActionTracker(make_macro_recording_tracker(client_id, gb_main));
}

bool got_macro_ability(AW_root *aw_root) {
    // return true if aw_root has a BoundActionTracker
    return get_active_macro_recording_tracker(aw_root);
}

