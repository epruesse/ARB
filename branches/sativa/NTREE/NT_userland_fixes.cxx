// ================================================================ //
//                                                                  //
//   File      : NT_userland_fixes.cxx                              //
//   Purpose   : Repair situations caused by ARB bugs in userland   //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2013   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include <aw_msg.hxx>

#include <arbdb.h>
#include <arb_strarray.h>
#include <arb_file.h>

#include <time.h>
#include <vector>
#include <string>

using namespace std;

typedef void (*fixfun)();

const long DAYS   = 24*60*60L;
const long WEEKS  = 7 * DAYS;
const long MONTHS = 30 * DAYS;
const long YEARS  = 365 * DAYS;

class UserlandCheck {
    vector<fixfun> fixes;
    time_t         now;
public:

    UserlandCheck() { time(&now); }
    void Register(fixfun fix, long IF_DEBUG(addedAt), long IF_DEBUG(expire)) {
        fixes.push_back(fix);
#if defined(DEBUG)
        if (addedAt+expire < now) {
            aw_message(GBS_global_string("Warning: UserlandCheck #%zu has expired\n", fixes.size()));
        }
#endif
    }
    void Run() {
        for (vector<fixfun>::iterator f = fixes.begin(); f != fixes.end(); ++f) {
            (*f)();
        }
    }
};

// ------------------------------------------------------------

void NT_repair_userland_problems() {
    // place to repair problems caused by earlier bugs

    UserlandCheck checks;
    // generate timestamp: date "--date=2013/09/20" "+%s"
    //                     date +%s

    // no checks active atm

    checks.Run();
}

