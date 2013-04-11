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

#define REMOTE_BASE_LEN     11 // len of REMOTE_BASE
#define MAX_REMOTE_APP_LEN  30 // max len of application (e.g. "ARB_EDIT4")
#define MAX_REMOTE_ITEM_LEN 6  // max len of item in APP container ("action", "result", ...)

#define MAX_REMOTE_PATH_LEN (REMOTE_BASE_LEN + MAX_REMOTE_APP_LEN + 1 + MAX_REMOTE_ITEM_LEN)

// --------------------------------------------------------------------------------

#if defined(ASSERTION_USED)
void assert_valid_application_name(const char *application) {
    size_t alen = strlen(application);
    arb_assert(alen>0 && alen <= MAX_REMOTE_APP_LEN);
}
#endif

// --------------------------------------------------------------------------------

class remote_awars {
    mutable char name[MAX_REMOTE_PATH_LEN+1];
    int  length; // of awar-path inclusive last '/'

    const char *item(const char *itemname) const {
        arb_assert(strlen(itemname) <= MAX_REMOTE_ITEM_LEN);
        strcpy(name+length, itemname);
        return name;
    }
public:
    remote_awars(const char *application) {
        IF_ASSERTION_USED(assert_valid_application_name(application));
        length = sprintf(name, REMOTE_BASE "%s/", application);
        arb_assert((length+MAX_REMOTE_ITEM_LEN) <= MAX_REMOTE_PATH_LEN);
    }
    const char *action() const { return item("action"); }
    const char *result() const { return item("result"); }
    const char *awar() const   { return item("awar"); }
    const char *value() const  { return item("value"); }
};

#else
#error ad_remote.h included twice
#endif // AD_REMOTE_H
