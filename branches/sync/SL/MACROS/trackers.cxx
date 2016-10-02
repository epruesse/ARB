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
#include "dbserver.hxx"

#include <aw_msg.hxx>
#include <arb_strarray.h>
#include <arb_sleep.h>
#include <ad_remote.h>
#include <unistd.h>
#include <ad_cb.h>

bool BoundActionTracker::reconfigure(const char *application_id, GBDATA *IF_ASSERTION_USED(gb_main)) {
    ma_assert(gb_main == gbmain);
    ma_assert(strcmp(id, "ARB_IMPORT") == 0); // currently only ARB_IMPORT-tracker gets reconfigured
    freedup(id, application_id);
    return true;
}

void BoundActionTracker::set_recording(bool recording) {
    GB_ERROR error = NULL;
    {
        GB_transaction ta(get_gbmain());
        remote_awars  remote(get_application_id());
        GBDATA       *gb_recAuth = GB_searchOrCreate_int(get_gbmain(), remote.recAuth(), 0);

        if (!gb_recAuth) {
            error = GB_await_error();
        }
        else {
            pid_t pid    = getpid();
            pid_t recPid = GB_read_int(gb_recAuth);

            if (recording) {
                if (recPid == 0) {
                    error = GB_write_int(gb_recAuth, pid); // allocate permission to record
                }
                else {
                    error = GBS_global_string("Detected two recording clients with id '%s'", get_application_id());
                }
            }
            else {
                if (recPid == pid) { // this is the authorized process
                    error = GB_write_int(gb_recAuth, 0); // clear permission
                }
            }
        }

        if (error) {
            error = GB_set_macro_error(get_gbmain(), error);
            if (error) GBK_terminatef("Failed to set macro-error: %s", error);
        }
    }
    set_tracking(recording);
}

static GB_ERROR announce_recording(GBDATA *gb_main, int record) {
    GB_transaction  ta(gb_main);
    GBDATA         *gb_recording = GB_searchOrCreate_int(gb_main, MACRO_TRIGGER_RECORDING, record);
    return gb_recording ? GB_write_int(gb_recording, record) : GB_await_error();
}

GB_ERROR MacroRecorder::start_recording(const char *file, const char *stop_action_name, bool expand_existing) {
    GB_ERROR error = NULL;
    if (is_tracking()) error = "Already recording macro";
    else {
        recording = new RecordingMacro(file, get_application_id(), stop_action_name, expand_existing);
        set_recording(true);

        error             = recording->has_error();
        if (!error) error = announce_recording(get_gbmain(), 1);
        if (error) {
            GB_ERROR stop_error = stop_recording();
            if (stop_error) fprintf(stderr, "Error while stopping macro recording: %s\n", stop_error);
        }
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
        set_recording(false);

        GB_ERROR ann_error = announce_recording(get_gbmain(), 0);
        if (error) {
            if (ann_error) fprintf(stderr, "Error in announce_recording: %s\n", ann_error);
        }
        else {
            error = ann_error;
        }
    }
    return error;
}

static void getKnownMacroClients(ConstStrArray& clientNames, GBDATA *gb_main) {
    GB_transaction ta(gb_main);

    GBDATA *gb_remote  = GB_search(gb_main, REMOTE_BASE, GB_FIND);
    GBDATA *gb_control = GB_search(gb_main, MACRO_TRIGGER_CONTAINER, GB_FIND);

    clientNames.erase();
    if (gb_remote) {
        for (GBDATA *gb_client = GB_child(gb_remote); gb_client; gb_client = GB_nextChild(gb_client)) {
            if (gb_client != gb_control) {
                const char *client_id = GB_read_key_pntr(gb_client);
                clientNames.put(client_id);
            }
        }
    }
}
__ATTR__USERESULT inline GB_ERROR setIntEntryToZero(GBDATA *gb_main, const char *entryPath) {
    GBDATA *gbe = GB_search(gb_main, entryPath, GB_INT);
    return gbe ? GB_write_int(gbe, 0) : GB_await_error();
}

__ATTR__USERESULT static GB_ERROR clearMacroExecutionAuthorization(GBDATA *gb_main) {
    // clear all granted client authorizations
    GB_transaction ta(gb_main);
    GB_ERROR       error = NULL;

    ConstStrArray clientNames;
    getKnownMacroClients(clientNames, gb_main);

    for (size_t i = 0; i<clientNames.size() && !error; ++i) {
        remote_awars remote(clientNames[i]);
        error             = setIntEntryToZero(gb_main, remote.authReq());
        if (!error) error = setIntEntryToZero(gb_main, remote.authAck());
        if (!error) error = setIntEntryToZero(gb_main, remote.granted());
        // if (!error) error = setIntEntryToZero(gb_main, remote.recAuth()); // @@@ clear elsewhere
    }
    if (error) error = GBS_global_string("error in clearMacroExecutionAuthorization: %s", error);
    return error;
}

class ExecutingMacro : virtual Noncopyable {
    RootCallback done_cb;

    ExecutingMacro        *next;
    static ExecutingMacro *head;

    ExecutingMacro(const RootCallback& execution_done_cb)
        : done_cb(execution_done_cb),
          next(head)
    {
        head = this;
    }

    void call() const { done_cb(AW_root::SINGLETON); }
    void destroy() { head = next; delete this; }

public:

    static void add(const RootCallback& execution_done_cb) { new ExecutingMacro(execution_done_cb); }
    static bool done() {
        // returns true if the last macro (of all recursively called macros) terminates
        if (head) {
            head->call();
            head->destroy();
        }
        return !head;
    }
    static void drop() {
        if (head) head->destroy();
    }
};

ExecutingMacro *ExecutingMacro::head = NULL;

static void macro_terminated(GBDATA *gb_terminated, GB_CB_TYPE IF_ASSERTION_USED(cb_type)) {
    ma_assert(cb_type == GB_CB_CHANGED);
    fprintf(stderr, "macro_terminated called\n");
    bool allMacrosTerminated = ExecutingMacro::done();
    if (allMacrosTerminated) {
        fprintf(stderr, "macro_terminated: allMacrosTerminated\n");
        GBDATA   *gb_main = GB_get_root(gb_terminated);
        GB_ERROR  error   = clearMacroExecutionAuthorization(gb_main);
        aw_message_if(error);

        // check for global macro error
        GB_ERROR macro_error = GB_get_macro_error(gb_main);
        if (macro_error) {
            aw_message_if(macro_error);
            aw_message("Warning: macro terminated (somewhere in the middle)");

            GB_ERROR clr_error = GB_clear_macro_error(gb_main);
            if (clr_error) fprintf(stderr, "Warning: failed to clear macro error (Reason: %s)\n", clr_error);
        }
    }
}

GB_ERROR MacroRecorder::execute(const char *macroFile, bool loop_marked, const RootCallback& execution_done_cb) {
    GB_ERROR  error = NULL;
    {
        GBDATA         *gb_main = get_gbmain();
        GB_transaction  ta(gb_main);

        GBDATA *gb_term = GB_search(gb_main, MACRO_TRIGGER_TERMINATED, GB_FIND);
        if (!gb_term) {
            gb_term = GB_search(gb_main, MACRO_TRIGGER_TERMINATED, GB_INT);
            if (!gb_term) {
                error = GB_await_error();
            }
            else {
                GB_add_callback(gb_term, GB_CB_CHANGED, makeDatabaseCallback(macro_terminated));
            }
        }
        error = ta.close(error);
    }

    if (!error) {
        ExecutingMacro::add(execution_done_cb);
        error = GBT_macro_execute(macroFile, loop_marked, true);
        if (error) ExecutingMacro::drop(); // avoid double free
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

GB_ERROR MacroRecorder::handle_tracked_client_action(char *&tracked) {
    ma_assert(tracked && tracked[0]);

    GB_ERROR error = NULL;
    if (tracked && tracked[0]) {
        char       *saveptr = NULL;
        const char *app_id  = strtok_r(tracked, "*", &saveptr);
        const char *cmd     = strtok_r(NULL,    "*", &saveptr);
        char       *rest    = strtok_r(NULL,    "",  &saveptr);

        if (recording) {
            if (strcmp(cmd, "AWAR") == 0) {
                const char *awar_name = strtok_r(rest, "*", &saveptr);
                const char *content   = strtok_r(NULL, "",  &saveptr);

                if (!content) content = "";

                recording->write_awar_change(app_id, awar_name, content);
            }
            else if (strcmp(cmd, "ACTION") == 0) {
                recording->write_action(app_id, rest);
            }
            else {
                error = GBS_global_string("Unknown client action '%s'", cmd);
            }
        }
        else {
            fprintf(stderr, "Warning: tracked action '%s' from client '%s' (dropped because not recording)\n", cmd, app_id);
        }
    }

    return error;
}

// -----------------------------
//      ClientActionTracker

void ClientActionTracker::set_tracking_according_to(GBDATA *gb_recording) {
    bool recording = GB_read_int(gb_recording);
    if (is_tracking() != recording) set_recording(recording);
}

static void record_state_changed_cb(GBDATA *gb_recording, ClientActionTracker *cat) {
    cat->set_tracking_according_to(gb_recording);
}

void ClientActionTracker::bind_callbacks(bool install) {
    GB_transaction  ta(get_gbmain());
    GB_ERROR        error        = NULL;
    GBDATA         *gb_recording = GB_searchOrCreate_int(get_gbmain(), MACRO_TRIGGER_RECORDING, 0);

    if (!gb_recording) {
        error = GB_await_error();
    }
    else {
        if (install) {
            error = GB_add_callback(gb_recording, GB_CB_CHANGED, makeDatabaseCallback(record_state_changed_cb, this));
            record_state_changed_cb(gb_recording, this); // call once
        }
        else {
            GB_remove_callback(gb_recording, GB_CB_CHANGED, makeDatabaseCallback(record_state_changed_cb, this));
        }
    }

    if (error) {
        aw_message(GBS_global_string("Failed to %s ClientActionTracker: %s", install ? "init" : "cleanup", error));
    }
}


void ClientActionTracker::track_action(const char *action_id) {
    if (!action_id) {
        warn_unrecordable("anonymous GUI element");
    }
    else {
        ma_assert(strchr(action_id, '*') == 0);
        send_client_action(GBS_global_string("ACTION*%s", action_id));
    }
}

void ClientActionTracker::track_awar_change(AW_awar *awar) {
    // see also recmac.cxx@AWAR_CHANGE_TRACKING

    char *svalue = awar->read_as_string();
    if (!svalue) {
        warn_unrecordable(GBS_global_string("change of '%s'", awar->awar_name));
    }
    else {
        ma_assert(strchr(awar->awar_name, '*') == 0);
        send_client_action(GBS_global_string("AWAR*%s*%s", awar->awar_name, svalue));
        free(svalue);
    }
}

void ClientActionTracker::send_client_action(const char *action) {
    // action is either
    // "ACTION*<actionId>" or
    // "AWAR*<awarName>*<awarValue>"

    // send action
    GB_ERROR  error;
    GBDATA   *gb_clientTrack = NULL;
    {
        error = GB_begin_transaction(get_gbmain());
        if (!error) {
            gb_clientTrack = GB_searchOrCreate_string(get_gbmain(), MACRO_TRIGGER_TRACKED, "");
            if (!gb_clientTrack) error = GB_await_error();
            else {
                const char *prev_track = GB_read_char_pntr(gb_clientTrack);

                if (!prev_track) error        = GB_await_error();
                else if (prev_track[0]) error = GBS_global_string("Cant send_client_action: have pending client action (%s)", prev_track);

                if (!error) {
                    ma_assert(strchr(get_application_id(), '*') == 0);
                    error = GB_write_string(gb_clientTrack, GBS_global_string("%s*%s", get_application_id(), action));
                }
            }
        }
        error = GB_end_transaction(get_gbmain(), error);
    }

    if (!error) {
        // wait for recorder to consume action
        bool          consumed = false;
        int           count    = 0;
        MacroTalkSleep increasing;

        while (!consumed && !error) {
            increasing.sleep();
            ++count;
            if ((count%25) == 0) {
                fprintf(stderr, "[Waiting for macro recorder to consume action tracked by %s]\n", get_application_id());
            }

            error = GB_begin_transaction(get_gbmain());

            const char *track    = GB_read_char_pntr(gb_clientTrack);
            if (!track) error    = GB_await_error();
            else        consumed = !track[0];

            error = GB_end_transaction(get_gbmain(), error);
        }
    }

    if (error) {
        aw_message(GBS_global_string("Failed to record client action (Reason: %s)", error));
    }
}

void ClientActionTracker::ungrant_client_and_confirm_quit_action() {
    // shutdown macro client
    // - confirm action (needed in case the quit has been triggered by a macro; otherwise macro hangs forever)
    // - unauthorize this process for macro execution

    GBDATA         *gb_main = get_gbmain();
    GB_transaction  ta(gb_main);
    remote_awars    remote(get_application_id());
    GB_ERROR        error   = NULL;

    GBDATA *gb_granted = GB_search(gb_main, remote.granted(), GB_FIND);
    if (gb_granted) {
        pid_t pid         = getpid();
        pid_t granted_pid = GB_read_int(gb_granted);

        if (pid == granted_pid) { // this is the client with macro execution rights
            GBDATA *gb_action    = GB_search(gb_main, remote.action(), GB_FIND);
            if (gb_action) error = GB_write_string(gb_action, ""); // signal macro, that action was executed
            if (!error) error    = GB_write_int(gb_granted, 0);    // un-authorize this process
        }
    }

    if (error) {
        error = GB_set_macro_error(gb_main, GBS_global_string("error during client quit: %s", error));
        if (error) fprintf(stderr, "Error in ungrant_client_and_confirm_quit_action: %s\n", error);
    }

    if (is_tracking()) set_recording(false);
}

// -------------------------
//      tracker factory

static UserActionTracker *make_macro_recording_tracker(const char *client_id, GBDATA *gb_main) {
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

GB_ERROR configure_macro_recording(AW_root *aw_root, const char *client_id, GBDATA *gb_main) {
    ma_assert(aw_root);

    BoundActionTracker *existing = get_active_macro_recording_tracker(aw_root);
    GB_ERROR            error    = NULL;
    if (existing && existing->reconfigure(client_id, gb_main)) {
        error = reconfigure_dbserver(client_id, gb_main);
    }
    else {
        aw_root->setUserActionTracker(make_macro_recording_tracker(client_id, gb_main));
        error = startup_dbserver(aw_root, client_id, gb_main);
    }

    return error;
}

void shutdown_macro_recording(AW_root *aw_root) {
    BoundActionTracker *tracker = get_active_macro_recording_tracker(aw_root);
    if (tracker) tracker->release();
}

bool got_macro_ability(AW_root *aw_root) {
    // return true if aw_root has a BoundActionTracker
    return get_active_macro_recording_tracker(aw_root);
}

