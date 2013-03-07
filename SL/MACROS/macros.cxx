// ============================================================= //
//                                                               //
//   File      : macros.cxx                                      //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in March 2013   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "macros.hxx"
#include "recmac.hxx"

#include <aw_msg.hxx>
#include <arbdbt.h>

GB_ERROR MacroRecorder::start_recording(const char *file, const char *application_id, const char *stop_action_name, bool expand_existing) {
    GB_ERROR error = NULL;
    if (is_tracking()) error = "Already recording macro";
    else {
        recording = new RecordingMacro(file, application_id, stop_action_name, expand_existing);
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

GB_ERROR MacroRecorder::execute(GBDATA *gb_main, const char *file, AW_RCB1 execution_done_cb, AW_CL client_data) {
    GB_ERROR  error = NULL;
    char     *path;
    if (file[0] == '/') {
        path = strdup(file);
    }
    else {
        path = GBS_global_string_copy("%s/%s", GB_getenvARBMACROHOME(), file);
    }

    {
        GB_transaction ta(gb_main);

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

