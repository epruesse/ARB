// =========================================================== //
//                                                             //
//   File      : AW_debug.cxx                                  //
//   Purpose   :                                               //
//                                                             //
//   Coded by Ralf Westram (coder@reallysoft.de) in May 2009   //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#if defined(DEBUG)
// --------------------------------------------------------------------------------

#include <Xm/Xm.h>

#include "aw_root.hxx"
#include "aw_window.hxx"
#include "aw_Xm.hxx"
#include "aw_window_Xm.hxx"
#include <arbdbt.h>

#include <vector>
#include <iterator>
#include <string>
#include <algorithm>
using namespace std;

typedef vector<string> CallbackArray;
typedef CallbackArray::const_iterator CallbackIter;

static long collect_callbacks(const char *key, long value, void *cl_callbacks) {
    CallbackArray *callbacks = reinterpret_cast<CallbackArray*>(cl_callbacks);
    callbacks->push_back(string(key));
    return value;
}

void AW_root::callallcallbacks(int mode) {
    // mode == 0 -> call all in normal order
    // mode == 1 -> call all in reverse order
    // mode == 2 -> call all in random order
    // mode == 3 -> call all in random order (repeated, never returns)

    size_t count = GBS_hash_count_elems(prvt->action_hash);
    aw_message(GBS_global_string("Found %zi callbacks", count));

    if (mode == 3) {
        aw_message("Calling callbacks forever");
        for (int iter = 1; ; ++iter) { // forever
            callallcallbacks(2);
            aw_message(GBS_global_string("All callbacks were called (iteration %i)", iter));
        }
    }
    else {
        CallbackArray callbacks;
        GBS_hash_do_sorted_loop(prvt->action_hash, collect_callbacks, GBS_HCF_sortedByKey, &callbacks);

        switch (mode) {
            case 0: break;                          // use this order
            case 1: reverse(callbacks.begin(), callbacks.end()); break; // use reverse order
            case 2: random_shuffle(callbacks.begin(), callbacks.end()); break; // use random order
            default : gb_assert(0); break;          // unknown mode
        }

        CallbackIter end = callbacks.end();
        for (CallbackIter cb = callbacks.begin(); cb != end; ++cb) {
            const char *remote_command = cb->c_str();
            aw_message(GBS_global_string("Calling '%s'", remote_command));

            AW_cb_struct *cbs = (AW_cb_struct *)GBS_read_hash(prvt->action_hash, remote_command);
            cbs->run_callback();
        }
    }
}


// --------------------------------------------------------------------------------
#endif // DEBUG

