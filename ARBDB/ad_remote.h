// ============================================================= //
//                                                               //
//   File      : ad_remote.h                                     //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in April 2013   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef AD_REMOTE_H
#define AD_REMOTE_H

#ifndef ARB_ASSERT_H
#include "arb_assert.h"
#endif
#ifndef ARBDBT_H
#include "arbdbt.h"
#endif
#ifndef ARB_SLEEP_H
#include <arb_sleep.h>
#endif

struct MacroTalkSleep : public ARB_inc_sleep {
    MacroTalkSleep() : ARB_inc_sleep(30, 250, MS, 20) {}
};

#define REMOTE_BASE_LEN     11 // len of REMOTE_BASE
#define MAX_REMOTE_APP_LEN  30 // max len of application (e.g. "ARB_EDIT4")
#define MAX_REMOTE_ITEM_LEN 7  // max len of item in APP container ("action", "result", ...)

#define MAX_REMOTE_PATH_LEN (REMOTE_BASE_LEN + MAX_REMOTE_APP_LEN + 1 + MAX_REMOTE_ITEM_LEN)

// --------------------------------------------------------------------------------

class remote_awars {
    mutable char  name[MAX_REMOTE_PATH_LEN+1];
    int           length; // of awar-path inclusive last '/'
    char         *app_id;

    const char *item(const char *itemname) const {
        arb_assert(strlen(itemname) <= MAX_REMOTE_ITEM_LEN);
        strcpy(name+length, itemname);
        return name;
    }

    void init() {
#if defined(ASSERTION_USED)
        size_t alen = strlen(app_id);
        arb_assert(alen>0 && alen <= MAX_REMOTE_APP_LEN);
#endif
        length = sprintf(name, REMOTE_BASE "%s/", app_id);
        arb_assert((length+MAX_REMOTE_ITEM_LEN) <= MAX_REMOTE_PATH_LEN);
    }

public:
    remote_awars(const char *application)   : app_id(strdup(application))  { init(); }
    remote_awars(const remote_awars& other) : app_id(strdup(other.app_id)) { init(); }
    DECLARE_ASSIGNMENT_OPERATOR(remote_awars);
    ~remote_awars() {
        free(app_id);
    }

    // definition of term as used here:
    // server = process running DB server ( = macro recorder/executor)
    // client = process performing remote actions (e.g. "ARB_EDIT4"; is identical with server when executing a remote cmd for "ARB_NTREE")
    // macro  = the executed perl macro

    // The following DB entries trigger GUI interaction in ../SL/MACROS/dbserver.cxx@check_for_remote_command
    // check_for_remote_command creates 'action', 'awar' and 'value' as soon as a client is ready to remote-execute.
    const char *action() const { return item("action"); } // contains name of GUI-action to be performed
    const char *result() const { return item("result"); } //
    const char *awar() const   { return item("awar"); }
    const char *value() const  { return item("value"); }

    // synchronization (needed to avoid that multiple clients with same application_id randomly accept remote-commands)
    const char *authReq() const  { return item("authReq"); } // == 1 -> a macro wants to remote-execute in some client [set to 1 by macro, reset to 0 by server when last macro terminates]
    const char *authAck() const  { return item("authAck"); } // == pid -> client received and accepted 'authReq' [set to its pid by client, reset to 0 by macro when granting authorization or by server when last macro terminates]
    const char *granted() const  { return item("granted"); } // == pid -> client with pid is authorized to remote-execute commands [set to the clients pid by macro, reset by server when last macro terminates]

    const char *recAuth() const  { return item("recAuth"); } // == pid -> client/server with pid is authorized to record commands [set/cleared by client/server; results in error if multiple pids try to authorize]

    const char *appID() const { return app_id; }
};

#else
#error ad_remote.h included twice
#endif // AD_REMOTE_H
