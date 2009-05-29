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

static GB_HASH *dontCallHash = 0;

static void build_dontCallHash() {
    aw_assert(!dontCallHash);
    dontCallHash = GBS_create_hash(30, GB_MIND_CASE);

    GBS_write_hash(dontCallHash, "ARB_NT/QUIT", 1);
    GBS_write_hash(dontCallHash, "quit",        1);

    // avoid start of some external programs:
    GBS_write_hash(dontCallHash, "xterm",                    2);
    GBS_write_hash(dontCallHash, "arb_dist",                 2);
    GBS_write_hash(dontCallHash, "arb_edit",                 2);
    GBS_write_hash(dontCallHash, "arb_phyl",                 2);
    GBS_write_hash(dontCallHash, "arb_pars",                 2);
    GBS_write_hash(dontCallHash, "arb_pars_quick",           2);
    GBS_write_hash(dontCallHash, "new_arb_edit4",            2);
    GBS_write_hash(dontCallHash, "new2_arb_edit4",           2);
    GBS_write_hash(dontCallHash, "ARB_NT/EDIT_SEQUENCES",    2);
    GBS_write_hash(dontCallHash, "count_different_chars",    2);
    GBS_write_hash(dontCallHash, "export_to_ARB",            2);
    GBS_write_hash(dontCallHash, "primer_design",            2);
    GBS_write_hash(dontCallHash, "CHECK_GCG_LIST/SHOW_FILE", 2);

#if defined(DEVEL_RALF)
#warning crashing - fix later
    GBS_write_hash(dontCallHash, "ARB_NT/mark_duplicates",         1);
    GBS_write_hash(dontCallHash, "ARB_NT/view_probe_group_result", 1);
#endif // DEVEL_RALF
}

static long collect_callbacks(const char *key, long value, void *cl_callbacks) {
    CallbackArray *callbacks = reinterpret_cast<CallbackArray*>(cl_callbacks);
    callbacks->push_back(string(key));
    return value;
}

int sortedByCallbackLocation(const char *k0, long v0, const char *k1, long v1) {
    AW_cb_struct *cbs0 = reinterpret_cast<AW_cb_struct*>(v0);
    AW_cb_struct *cbs1 = reinterpret_cast<AW_cb_struct*>(v1);

    int cmp = (AW_CL)(cbs1->f) - (AW_CL)cbs0->f; // compare address of function
    if (!cmp) {
        cmp = cbs1->get_cd1() - cbs0->get_cd1();
        if (!cmp) {
            cmp = cbs1->get_cd2() - cbs0->get_cd2();
            if (!cmp) cmp = strcmp(k0, k1);
        }
    }
    return cmp;
}

void AW_root::callallcallbacks(int mode) {
    // mode == 0 -> call all in alpha-order
    // mode == 1 -> call all in reverse alpha-order
    // mode == 2 -> call all in code-order
    // mode == 3 -> call all in reverse code-order
    // mode == 10 -> call all in random order
    // mode == 11-> call all in random order (repeated, never returns)

    size_t count = GBS_hash_count_elems(prvt->action_hash);
    aw_message(GBS_global_string("Found %zi callbacks", count));

    if (!dontCallHash) build_dontCallHash();

    if (mode == 11) {
        aw_message("Calling callbacks forever");
        for (int iter = 1; ; ++iter) { // forever
            callallcallbacks(2);
            aw_message(GBS_global_string("All callbacks were called (iteration %i)", iter));
        }
    }
    else {
        CallbackArray callbacks;
        switch (mode) {
            case 0:
            case 1:
                GBS_hash_do_sorted_loop(prvt->action_hash, collect_callbacks, GBS_HCF_sortedByKey, &callbacks);
                break;
            case 2:
            case 3:
                GBS_hash_do_sorted_loop(prvt->action_hash, collect_callbacks, sortedByCallbackLocation, &callbacks);
                break;
            default:
                GBS_hash_do_loop(prvt->action_hash, collect_callbacks, &callbacks);
                break;
        }

        switch (mode) {
            case 0: break;                          // use this order
            case 1: reverse(callbacks.begin(), callbacks.end()); break; // use reverse order
            case 10: random_shuffle(callbacks.begin(), callbacks.end()); break; // use random order
            default : gb_assert(0); break;          // unknown mode
        }

        CallbackIter end  = callbacks.end();
        int          curr = 1;

        for (int pass = 1; pass <= 2; ++pass) {
            CallbackIter cb = callbacks.begin();
#if defined(DEVEL_RALF)
            // skip first xxx callbacks (to go to bug quicker)
            int skip  = 0; // 150;                             
            advance(cb, skip);
            curr     += skip;
#endif // DEVEL_RALF

            for (; cb != end; ++cb) {
                const char *remote_command = cb->c_str();
                bool        this_pass      = remote_command[0] == '-' ? (pass == 2) : (pass == 1);

                if (this_pass) {
                    if (remote_command[0] == '!' || GBS_read_hash(dontCallHash, remote_command)) {
                        aw_message(GBS_global_string("Skipped callback %i/%i (%s)", curr, count, remote_command));
                    }
                    else {
                        aw_message(GBS_global_string("Calling back %i/%i (%s)", curr, count, remote_command));

                        AW_cb_struct *cbs = (AW_cb_struct *)GBS_read_hash(prvt->action_hash, remote_command);
                        GB_clear_error();

                        cbs->run_callback();
                        process_events();

                        if (GB_have_error()) {
                            aw_message(GBS_global_string("Unhandled error in '%s': %s", remote_command, GB_await_error()));
                        }

                        if (cbs->f == AW_POPUP) {
                            AW_window *awp = cbs->pop_up_window;
                            if (awp) {
                                aw_message(GBS_global_string("Popping down window '%s'", awp->get_window_id()));
                                awp->hide();
                                process_events();
                            }
                        }
                    }
                }
                else {
                    if (pass == 1) {
                        aw_message(GBS_global_string("Delayed callback %i/%i (%s)", curr, count, remote_command));
                    }
                }

                curr++;
            }

            if (pass == 1) aw_message("Executing delayed callbacks:");
        }
    }
}


// --------------------------------------------------------------------------------
#endif // DEBUG

