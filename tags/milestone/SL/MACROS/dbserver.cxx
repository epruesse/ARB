// ============================================================= //
//                                                               //
//   File      : dbserver.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in April 2013   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "trackers.hxx"
#include "dbserver.hxx"
#include "macros.hxx"

#include <arbdbt.h>
#include <ad_remote.h>

#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <aw_window.hxx>

#include <unistd.h>

#if defined(DEBUG)
// # define DUMP_REMOTE_ACTIONS
// # define DUMP_AUTHORIZATION // see also ../../ARBDB/adtools.cxx@DUMP_AUTH_HANDSHAKE
#endif

#if defined(DUMP_REMOTE_ACTIONS)
# define IF_DUMP_ACTION(cmd) cmd
#else
# define IF_DUMP_ACTION(cmd)
#endif

#if defined(DUMP_AUTHORIZATION)
# define IF_DUMP_AUTH(cmd) cmd
#else
# define IF_DUMP_AUTH(cmd)
#endif



#define ARB_SERVE_DB_TIMER 50  // ms
#define ARB_CHECK_DB_TIMER 200 // ms

struct db_interrupt_data : virtual Noncopyable {
    remote_awars  remote;
    GBDATA       *gb_main;

    db_interrupt_data(GBDATA *gb_main_, const char *application_id)
        : remote(application_id),
          gb_main(gb_main_)
    {}


    GB_ERROR reconfigure(GBDATA *gb_main_, const char *application_id) {
        GB_ERROR error = 0;
        if (gb_main_ != gb_main) {
            error = "Attempt to reconfigure database interrupt with changed database";
        }
        else {
            remote = remote_awars(application_id);
        }
        return error;
    }
};

__ATTR__USERESULT static GB_ERROR check_for_remote_command(AW_root *aw_root, const db_interrupt_data& dib) { // @@@ split into several functions
    arb_assert(!GB_have_error());

    GB_ERROR  error   = 0;
    GBDATA   *gb_main = dib.gb_main;

    GB_push_transaction(gb_main); // @@@ begin required/possible here?

    if (GB_is_server(gb_main)) {
        char *client_action = GBT_readOrCreate_string(gb_main, MACRO_TRIGGER_TRACKED, "");
        if (client_action && client_action[0]) {
            UserActionTracker *tracker       = aw_root->get_tracker();
            MacroRecorder     *macroRecorder = dynamic_cast<MacroRecorder*>(tracker);

            error               = macroRecorder->handle_tracked_client_action(client_action);
            GB_ERROR trig_error = GBT_write_string(gb_main, MACRO_TRIGGER_TRACKED, ""); // tell client that action has been recorded
            if (!error) error   = trig_error;
        }
        free(client_action);

        if (!error) {
            GB_ERROR macro_error = GB_get_macro_error(gb_main);
            if (macro_error) {
                UserActionTracker *tracker       = aw_root->get_tracker();
                MacroRecorder     *macroRecorder = dynamic_cast<MacroRecorder*>(tracker);

                if (macroRecorder->is_tracking()) { // only handle recording error here
                    aw_message(GBS_global_string("%s\n"
                                                 "macro recording aborted", macro_error));
                    error = macroRecorder->stop_recording();

                    GB_ERROR clr_error = GB_clear_macro_error(gb_main);
                    if (!error) error  = clr_error;
                }
            }
        }
    }

    if (error) {
        GB_pop_transaction(gb_main);
    }
    else {
        const remote_awars& remote = dib.remote;

        long  *granted    = GBT_readOrCreate_int(gb_main, remote.granted(), 0);
        pid_t  pid        = getpid();
        bool   authorized = granted && *granted == pid;

        if (!authorized) {
            if (!granted) error = GBS_global_string("Failed to access '%s'", remote.granted());
            else {
                arb_assert(*granted != pid);

                // @@@ differ between *granted == 0 and *granted != 0?

                long *authReq       = GBT_readOrCreate_int(gb_main, remote.authReq(), 0);
                if (!authReq) error = GBS_global_string("Failed to access '%s'", remote.authReq());
                else if (*authReq) {
                    GBDATA *gb_authAck     = GB_searchOrCreate_int(gb_main, remote.authAck(), 0);
                    if (!gb_authAck) error = GBS_global_string("Failed to access '%s'", remote.authAck());
                    else {
                        pid_t authAck = GB_read_int(gb_authAck);
                        if (authAck == 0) {
                            // ack this process can execute remote commands
                            error = GB_write_int(gb_authAck, pid);
                            IF_DUMP_AUTH(fprintf(stderr, "acknowledging '%s' with pid %i\n", remote.authAck(), pid));
                        }
                        else if (authAck == pid) {
                            // already acknowledged -- wait
                        }
                        else { // another process with same app-id acknowledged faster
                            IF_DUMP_AUTH(fprintf(stderr, "did not acknowledge '%s' with pid %i (pid %i was faster)\n", remote.authAck(), pid, authAck));
                            const char *merr = GBS_global_string("Detected two clients with id '%s'", remote.appID());
                            error            = GB_set_macro_error(gb_main, merr);
                        }
                    }
                }
            }
            error = GB_end_transaction(gb_main, error);
        }
        else {
            char *action   = GBT_readOrCreate_string(gb_main, remote.action(), "");
            char *value    = GBT_readOrCreate_string(gb_main, remote.value(), "");
            char *tmp_awar = GBT_readOrCreate_string(gb_main, remote.awar(), "");

            if (tmp_awar[0]) {
                AW_awar *found_awar = aw_root->awar_no_error(tmp_awar);
                if (!found_awar) {
                    error = GBS_global_string("Unknown variable '%s'", tmp_awar);
                }
                else {
                    if (strcmp(action, "AWAR_REMOTE_READ") == 0) {
                        char *read_value = aw_root->awar(tmp_awar)->read_as_string();
                        GBT_write_string(gb_main, remote.value(), read_value);
                        IF_DUMP_ACTION(printf("remote command 'AWAR_REMOTE_READ' awar='%s' value='%s'\n", tmp_awar, read_value));
                        free(read_value);
                        action[0] = 0; // clear action (AWAR_REMOTE_READ is just a pseudo-action) :
                        GBT_write_string(gb_main, remote.action(), "");
                    }
                    else if (strcmp(action, "AWAR_REMOTE_TOUCH") == 0) {
                        GB_set_remote_action(gb_main, true);
                        aw_root->awar(tmp_awar)->touch();
                        GB_set_remote_action(gb_main, false);
                        IF_DUMP_ACTION(printf("remote command 'AWAR_REMOTE_TOUCH' awar='%s'\n", tmp_awar));
                        action[0] = 0; // clear action (AWAR_REMOTE_TOUCH is just a pseudo-action) :
                        GBT_write_string(gb_main, remote.action(), "");
                    }
                    else {
                        IF_DUMP_ACTION(printf("remote command (write awar) awar='%s' value='%s'\n", tmp_awar, value));
                        GB_set_remote_action(gb_main, true);
                        error = aw_root->awar(tmp_awar)->write_as_string(value);
                        GB_set_remote_action(gb_main, false);
                    }
                }
                GBT_write_string(gb_main, remote.result(), error ? error : "");
                GBT_write_string(gb_main, remote.awar(), ""); // tell perl-client call has completed (BIO::remote_awar and BIO:remote_read_awar)

                aw_message_if(error);
            }
            GB_pop_transaction(gb_main); // @@@ end required/possible here?
            arb_assert(!GB_have_error()); // error exported by some callback? (unwanted)

            if (action[0]) {
#if defined(ARB_GTK)
                AW_action* act = aw_root->action_try(action);
#else // ARB_MOTIF
                AW_cb *act = aw_root->search_remote_command(action);
#endif

                if (act) {
                    IF_DUMP_ACTION(printf("remote command (%s) found, running callback\n", action));
                    arb_assert(!GB_have_error());
                    GB_set_remote_action(gb_main, true);
#if defined(ARB_GTK)
                    act->user_clicked(NULL);
#else // ARB_MOTIF
                    act->run_callbacks();
#endif
                    GB_set_remote_action(gb_main, false);
                    arb_assert(!GB_have_error()); // error exported by callback (unwanted)
                    GBT_write_string(gb_main, remote.result(), "");
                }
                else {
                    IF_DUMP_ACTION(printf("remote command (%s) is unknown\n", action));
                    error = GBS_global_string("Unknown action '%s' in macro", action);
                    GBT_write_string(gb_main, remote.result(), error);
                }
                GBT_write_string(gb_main, remote.action(), ""); // tell perl-client call has completed (remote_action)
            }

            free(tmp_awar);
            free(value);
            free(action);
        }
    }

    arb_assert(!GB_have_error());
    if (error) fprintf(stderr, "Error in check_for_remote_command: %s\n", error);
    return error;
}

inline bool remote_command_handler(AW_root *awr, const db_interrupt_data& dib) {
    // returns false in case of errors
    arb_assert(!GB_have_error());

    bool     ok    = true;
    GB_ERROR error = check_for_remote_command(awr, dib);
    if (error) {
        aw_message(error);
        ok = false;
    }
    return ok;
}

static unsigned serve_db_interrupt(AW_root *awr, db_interrupt_data *dib) { // server
    bool success = GBCMS_accept_calls(dib->gb_main, false);
    while (success) { // @@@ maybe abort this loop after some time? (otherwise: if a client polls to fast, the GUI of the server locks)
        success = remote_command_handler(awr, *dib) && GBCMS_accept_calls(dib->gb_main, true);
    }
    return ARB_SERVE_DB_TIMER;
}

static unsigned check_db_interrupt(AW_root *awr, db_interrupt_data *dib) { // client
    remote_command_handler(awr, *dib);
    return ARB_CHECK_DB_TIMER;
}

// --------------------------------------------------------------------------------

static db_interrupt_data *idle_interrupt = NULL;

GB_ERROR startup_dbserver(AW_root *aw_root, const char *application_id, GBDATA *gb_main) {
#if defined(DEBUG)
    static bool initialized = false;
    arb_assert(!initialized); // called twice - not able (yet)
    initialized = true;
#endif

    arb_assert(got_macro_ability(aw_root));

    GB_ERROR error = NULL;
    if (GB_read_clients(gb_main) == 0) { // server
        error = GBCMS_open(":", 0, gb_main);
        if (error) {
            error = GBS_global_string("THIS PROGRAM HAS PROBLEMS TO OPEN INTERCLIENT COMMUNICATION:\n"
                                      "Reason: %s\n"
                                      "(maybe there is already another server running)\n"
                                      "You cannot use any EDITOR or other external SOFTWARE from here.\n"
                                      "Advice: Close ARB again, open a console, type 'arb_clean' and restart arb.\n"
                                      "Caution: Any unsaved data in an eventually running ARB will be lost.\n",
                                      error);
        }
        else {
            idle_interrupt = new db_interrupt_data(gb_main, application_id);
            aw_root->add_timed_callback(ARB_SERVE_DB_TIMER, makeTimedCallback(serve_db_interrupt, idle_interrupt));
        }
    }
    else { // client
        idle_interrupt = new db_interrupt_data(gb_main, application_id);
        aw_root->add_timed_callback(ARB_CHECK_DB_TIMER, makeTimedCallback(check_db_interrupt, idle_interrupt));
    }

    if (!error) {
        // handle remote commands once (to create DB-entries; w/o they are created after startup of first DB-client)
        arb_assert(idle_interrupt);
        if (idle_interrupt) remote_command_handler(aw_root, *idle_interrupt);
    }

    return error;
}

GB_ERROR reconfigure_dbserver(const char *application_id, GBDATA *gb_main) {
    // reconfigures a running dbserver (started with startup_dbserver)
    arb_assert(idle_interrupt); // not started yet
    return idle_interrupt->reconfigure(gb_main, application_id);
}

