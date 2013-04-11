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

#include "dbserver.hxx"
#include "macros.hxx"

#include <arbtools.h>
#include <arbdbt.h>
#include <ad_remote.h>

#include <aw_root.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <aw_window.hxx>

#define ARB_SERVE_DB_TIMER 50
#define ARB_CHECK_DB_TIMER 200

struct db_interrupt_data : virtual Noncopyable {
    remote_awars  remote;
    GBDATA       *gb_main;

    db_interrupt_data(GBDATA *gb_main_, const char *application_id)
        : remote(application_id),
          gb_main(gb_main_)
    {}
};

static GB_ERROR check_for_remote_command(AW_root *aw_root, AW_default gb_maind, const remote_awars& remote) {
    GBDATA *gb_main = (GBDATA *)gb_maind;

    GB_push_transaction(gb_main);

    char *action   = GBT_readOrCreate_string(gb_main, remote.action(), "");
    char *value    = GBT_readOrCreate_string(gb_main, remote.value(), "");
    char *tmp_awar = GBT_readOrCreate_string(gb_main, remote.awar(), "");

    if (tmp_awar[0]) {
        GB_ERROR error = 0;
        AW_awar *found_awar = aw_root->awar_no_error(tmp_awar);
        if (!found_awar) {
            error = GBS_global_string("Unknown variable '%s'", tmp_awar);
        }
        else {
            if (strcmp(action, "AWAR_REMOTE_READ") == 0) {
                char *read_value = aw_root->awar(tmp_awar)->read_as_string();
                GBT_write_string(gb_main, remote.value(), read_value);
#if defined(DUMP_REMOTE_ACTIONS)
                printf("remote command 'AWAR_REMOTE_READ' awar='%s' value='%s'\n", tmp_awar, read_value);
#endif // DUMP_REMOTE_ACTIONS
                free(read_value);
                // clear action (AWAR_REMOTE_READ is just a pseudo-action) :
                action[0]        = 0;
                GBT_write_string(gb_main, remote.action(), "");
            }
            else if (strcmp(action, "AWAR_REMOTE_TOUCH") == 0) {
                aw_root->awar(tmp_awar)->touch();
#if defined(DUMP_REMOTE_ACTIONS)
                printf("remote command 'AWAR_REMOTE_TOUCH' awar='%s'\n", tmp_awar);
#endif // DUMP_REMOTE_ACTIONS
                // clear action (AWAR_REMOTE_TOUCH is just a pseudo-action) :
                action[0] = 0;
                GBT_write_string(gb_main, remote.action(), "");
            }
            else {
#if defined(DUMP_REMOTE_ACTIONS)
                printf("remote command (write awar) awar='%s' value='%s'\n", tmp_awar, value);
#endif // DUMP_REMOTE_ACTIONS
                error = aw_root->awar(tmp_awar)->write_as_string(value);
            }
        }
        GBT_write_string(gb_main, remote.result(), error ? error : "");
        GBT_write_string(gb_main, remote.awar(), ""); // tell perl-client call has completed (BIO::remote_awar and BIO:remote_read_awar)

        aw_message_if(error);
    }
    GB_pop_transaction(gb_main);

    if (action[0]) {
        AW_cb_struct *cbs = aw_root->search_remote_command(action);

#if defined(DUMP_REMOTE_ACTIONS)
        printf("remote command (%s) exists=%i\n", action, int(cbs != 0));
#endif // DUMP_REMOTE_ACTIONS
        if (cbs) {
            cbs->run_callback();
            GBT_write_string(gb_main, remote.result(), "");
        }
        else {
            aw_message(GB_export_errorf("Unknown action '%s' in macro", action));
            GBT_write_string(gb_main, remote.result(), GB_await_error());
        }
        GBT_write_string(gb_main, remote.action(), ""); // tell perl-client call has completed (remote_action)
    }

    free(tmp_awar);
    free(value);
    free(action);

    return 0;
}

static void serve_db_interrupt(AW_root *awr, AW_CL cl_db_interrupt_data) { // server
    db_interrupt_data *dib = (db_interrupt_data*)cl_db_interrupt_data;

    bool success = GBCMS_accept_calls(dib->gb_main, false);
    while (success) {
        check_for_remote_command(awr, dib->gb_main, dib->remote);
        success = GBCMS_accept_calls(dib->gb_main, true);
    }

    awr->add_timed_callback(ARB_SERVE_DB_TIMER, serve_db_interrupt, cl_db_interrupt_data);
}

static void check_db_interrupt(AW_root *awr, AW_CL cl_db_interrupt_data) { // client
    db_interrupt_data *dib = (db_interrupt_data*)cl_db_interrupt_data;

    check_for_remote_command(awr, dib->gb_main, dib->remote);
    awr->add_timed_callback(ARB_CHECK_DB_TIMER, check_db_interrupt, cl_db_interrupt_data);
}

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
            aw_root->add_timed_callback(ARB_SERVE_DB_TIMER, serve_db_interrupt, AW_CL(new db_interrupt_data(gb_main, application_id)));
        }
    }
    else { // client
        aw_root->add_timed_callback(ARB_CHECK_DB_TIMER, check_db_interrupt, AW_CL(new db_interrupt_data(gb_main, application_id)));
    }

    return error;
}




