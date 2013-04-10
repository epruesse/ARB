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

#include "macros.hxx"

#include <arbtools.h>
#include <arbdbt.h>
#include <aw_root.hxx>

#define ARB_SERVE_DB_TIMER 50
#define ARB_CHECK_DB_TIMER 200

struct db_interrupt_data : virtual Noncopyable {
    char   *remote_awar_base;
    GBDATA *gb_main;

    db_interrupt_data(GBDATA *gb_main_, const char *application_id)
        : remote_awar_base(GBT_get_remote_awar_base(application_id)),
          gb_main(gb_main_)
    {}
    ~db_interrupt_data() { free(remote_awar_base); }
};

// @@@ move DB-interrupt code elsewhere (has to be available in any GUI application able to act as DB-server)
static void serve_db_interrupt(AW_root *awr, AW_CL cl_db_interrupt_data) { // server
    db_interrupt_data *dib = (db_interrupt_data*)cl_db_interrupt_data;

    bool success = GBCMS_accept_calls(dib->gb_main, false);
    while (success) {
        awr->check_for_remote_command(dib->gb_main, dib->remote_awar_base);
        success = GBCMS_accept_calls(dib->gb_main, true);
    }

    awr->add_timed_callback(ARB_SERVE_DB_TIMER, serve_db_interrupt, cl_db_interrupt_data);
}

static void check_db_interrupt(AW_root *awr, AW_CL cl_db_interrupt_data) { // client
    db_interrupt_data *dib = (db_interrupt_data*)cl_db_interrupt_data;

    awr->check_for_remote_command(dib->gb_main, dib->remote_awar_base);
    awr->add_timed_callback(ARB_CHECK_DB_TIMER, check_db_interrupt, cl_db_interrupt_data);
}

GB_ERROR startup_dbserver(AW_root *aw_root, const char *application_id, GBDATA *gb_main) {
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




