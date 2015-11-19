// ============================================================= //
//                                                               //
//   File      : arb_cs.cxx                                      //
//   Purpose   : Basics for client/server communication          //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in March 2011   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "arb_cs.h"
#include "arb_msg.h"
#include <smartptr.h>
#include <unistd.h>
#include <netdb.h>

void arb_gethostbyname(const char *name, struct hostent *& he, GB_ERROR& err) {
    he = gethostbyname(name);
    // Note: gethostbyname is marked obsolete.
    // replacement getnameinfo seems less portable atm.

    if (he) {
        err = NULL;
    }
    else {
        err = GBS_global_string("Cannot resolve hostname: '%s' (h_errno=%i='%s')",
                                name, h_errno, hstrerror(h_errno));
    }
}

const char *arb_gethostname() {
    static SmartCharPtr hostname;
    if (hostname.isNull()) {
        char buffer[4096];
        gethostname(buffer, 4095);
        hostname = strdup(buffer);
    }
    return &*hostname;
}

