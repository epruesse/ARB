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

static GB_HASH *dontCallHash      = 0;
static GB_HASH *alreadyCalledHash = 0;

static void forgetCalledCallbacks() {
    if (alreadyCalledHash) GBS_free_hash(alreadyCalledHash);
    alreadyCalledHash = GBS_create_hash(5000, GB_MIND_CASE);
}

static long auto_dontcall1(const char *key, long value, void *cl_hash) {
    if (strncmp(key, "ARB_NT/", 7) == 0) {
        GB_HASH *autodontCallHash = (GB_HASH*)cl_hash;
        GBS_write_hash(autodontCallHash, GBS_global_string("ARB_NT_1/%s", key+7), value);
    }
    return value;
}
static long auto_dontcall2(const char *key, long value, void *) {
    GBS_write_hash(dontCallHash, key, value);
    return value;
}

static void build_dontCallHash() {
    aw_assert(!dontCallHash);
    dontCallHash = GBS_create_hash(30, GB_MIND_CASE);
    forgetCalledCallbacks();

    GBS_write_hash(dontCallHash, "ARB_NT/QUIT",    1);
    GBS_write_hash(dontCallHash, "quit",           1);
    GBS_write_hash(dontCallHash, "ARB_EDIT4/QUIT", 1);
    GBS_write_hash(dontCallHash, "ARB_INTRO/CANCEL", 1);

    // avoid start of some external programs:
#if 1    
    GBS_write_hash(dontCallHash, "ARB_NT/EDIT_SEQUENCES",                              2);
    GBS_write_hash(dontCallHash, "CHECK_GCG_LIST/SHOW_FILE",                           2);
    GBS_write_hash(dontCallHash, "CPR_MAIN/HELP",                                      2);
    GBS_write_hash(dontCallHash, "GDE__user__Start_a_slave_ARB_on_a_foreign_host_/GO", 2);
    GBS_write_hash(dontCallHash, "HELP/BROWSE",                                        2);
    GBS_write_hash(dontCallHash, "HELP/EDIT",                                          2);
    GBS_write_hash(dontCallHash, "MACROS/EDIT",                                        2);
    GBS_write_hash(dontCallHash, "MACROS/EXECUTE",                                     2);
    GBS_write_hash(dontCallHash, "NAME_SERVER_ADMIN/EDIT_NAMES_FILE",                  2);
    GBS_write_hash(dontCallHash, "arb_dist",                                           2);
    GBS_write_hash(dontCallHash, "arb_edit",                                           2);
    GBS_write_hash(dontCallHash, "arb_pars",                                           2);
    GBS_write_hash(dontCallHash, "arb_pars_quick",                                     2);
    GBS_write_hash(dontCallHash, "arb_phyl",                                           2);
    GBS_write_hash(dontCallHash, "count_different_chars",                              2);
    GBS_write_hash(dontCallHash, "export_to_ARB",                                      2);
    GBS_write_hash(dontCallHash, "new2_arb_edit4",                                     2);
    GBS_write_hash(dontCallHash, "new_arb_edit4",                                      2);
    GBS_write_hash(dontCallHash, "primer_design",                                      2);
    GBS_write_hash(dontCallHash, "xterm",                                              2);
    GBS_write_hash(dontCallHash, "SUBMIT_REG/SEND",                                              2);
    GBS_write_hash(dontCallHash, "SUBMIT_BUG/SEND",                                              2);
    GBS_write_hash(dontCallHash, "NAME_SERVER_ADMIN/REMOVE_SUPERFLUOUS_ENTRIES_IN_NAMES_FILE", 2);
    GBS_write_hash(dontCallHash, "PRINT_CANVAS/PRINT", 2);
    GBS_write_hash(dontCallHash, "PT_SERVER_ADMIN/CREATE_TEMPLATE", 2);
    GBS_write_hash(dontCallHash, "SELECT_CONFIFURATION/START", 2);
#endif
    
    // avoid saving
    GBS_write_hash(dontCallHash, "save_changes", 3);
    GBS_write_hash(dontCallHash, "save_props",   3);

#if 1
#warning crashing - fix later
    GBS_write_hash(dontCallHash, "ARB_NT/mark_duplicates",         4);
    GBS_write_hash(dontCallHash, "ARB_NT/view_probe_group_result", 4);
    GBS_write_hash(dontCallHash, "check_gcg_list",                 4);
    GBS_write_hash(dontCallHash, "PT_SERVER_ADMIN/CHECK_SERVER",   4);
#endif

#if 1
#warning test callbacks asking questions again later
    GBS_write_hash(dontCallHash, "ARB_NT/mark_deep_branches",                            5);
    GBS_write_hash(dontCallHash, "ARB_NT/mark_degen_branches",                           5);
    GBS_write_hash(dontCallHash, "ARB_NT/mark_long_branches",                            5);
    GBS_write_hash(dontCallHash, "ARB_NT/tree_scale_lengths",                            5);
    GBS_write_hash(dontCallHash, "CREATE_USER_MASK/CREATE",                              5);
    GBS_write_hash(dontCallHash, "GDE__import__Import_sequences_using_Readseq_slow_/GO", 5);
    GBS_write_hash(dontCallHash, "INFO_OF_ALIGNMENT/DELETE",                             5);
    GBS_write_hash(dontCallHash, "LOAD_SELECTION_BOX/LOAD",                              5);
    GBS_write_hash(dontCallHash, "MULTI_PROBE/CREATE_NEW_SEQUENCE",                      5);
    GBS_write_hash(dontCallHash, "NDS_PROPS/SAVELOAD_CONFIG",                            5);
    GBS_write_hash(dontCallHash, "PRIMER_DESIGN/SAVELOAD_CONFIG",                        5);
    GBS_write_hash(dontCallHash, "PROBE_DESIGN/SAVELOAD_CONFIG",                         5);
    GBS_write_hash(dontCallHash, "PT_SERVER_ADMIN/KILL_ALL_SERVERS",                     5);
    GBS_write_hash(dontCallHash, "PT_SERVER_ADMIN/KILL_SERVER",                          5);
    GBS_write_hash(dontCallHash, "PT_SERVER_ADMIN/UPDATE_SERVER",                        5);
    GBS_write_hash(dontCallHash, "SPECIES_QUERY/DELETE_LISTED",                          5);
    GBS_write_hash(dontCallHash, "SPECIES_QUERY/SAVELOAD_CONFIG",                        5);
    GBS_write_hash(dontCallHash, "SPECIES_SELECTIONS/RENAME",                            5);
    GBS_write_hash(dontCallHash, "SPECIES_SELECTIONS/STORE",                             5);
    GBS_write_hash(dontCallHash, "del_marked",                                           5);
    GBS_write_hash(dontCallHash, "REALIGN_DNA/REALIGN",                                  5);
    GBS_write_hash(dontCallHash, "TREE_PROPS/SAVELOAD_CONFIG",                           5);
    GBS_write_hash(dontCallHash, "WWW_PROPS/SAVELOAD_CONFIG",                            5);
#endif

    GB_HASH *autodontCallHash = GBS_create_hash(30, GB_MIND_CASE);
    GBS_hash_do_loop(dontCallHash, auto_dontcall1, autodontCallHash);
    GBS_hash_do_loop(autodontCallHash, auto_dontcall2, dontCallHash);
    GBS_free_hash(autodontCallHash);
}

static long collect_callbacks(const char *key, long value, void *cl_callbacks) {
    if (GBS_read_hash(alreadyCalledHash, key) == 0) { // don't call twice
        CallbackArray *callbacks = reinterpret_cast<CallbackArray*>(cl_callbacks);
        callbacks->push_back(string(key));
    }
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
    // mode == -2 -> mark all as called
    // mode == -1 -> forget called
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
            callallcallbacks(10); // call all in random order
            aw_message(GBS_global_string("All callbacks were called (iteration %i)", iter));
        }
    }
    else if (mode == -1) {
        forgetCalledCallbacks();
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
            case -2:
            case 0: break;                          // use this order
            case 1: reverse(callbacks.begin(), callbacks.end()); break; // use reverse order
            case 10: random_shuffle(callbacks.begin(), callbacks.end()); break; // use random order
            default : gb_assert(0); break;          // unknown mode
        }

        count = callbacks.size();
        aw_message(GBS_global_string("%zi callbacks were not called yet", count));

        CallbackIter end = callbacks.end();

        for (int pass = 1; pass <= 2; ++pass) {
            size_t       curr = 1;
            CallbackIter cb   = callbacks.begin();

            for (; cb != end; ++cb) {
                const char *remote_command = cb->c_str();
                bool        this_pass      = remote_command[0] == '-' ? (pass == 2) : (pass == 1);

                if (this_pass) {
                    GBS_write_hash(alreadyCalledHash, remote_command, 1); // don't call twice

                    if (mode != -2) { // -2 means "only mark as called"
                        AW_cb_struct *cbs = (AW_cb_struct *)GBS_read_hash(prvt->action_hash, remote_command);
                        bool skipcb = remote_command[0] == '!' || GBS_read_hash(dontCallHash, remote_command);
                        if (!skipcb) {
                            if (cbs->f == (AW_CB)AW_help_entry_pressed) skipcb = true;
                        }
                        if (skipcb) {
                            fprintf(stderr, "Skipped callback %zu/%zu (%s)\n", curr, count, remote_command);
                        }
                        else {
                            fprintf(stderr, "Calling back %zu/%zu (%s)\n", curr, count, remote_command);
                        
                            GB_clear_error();

                            cbs->run_callback();
                            process_events();

                            if (GB_have_error()) {
                                fprintf(stderr, "Unhandled error in '%s': %s\n", remote_command, GB_await_error());
                            }

                            if (cbs->f == AW_POPUP) {
                                AW_window *awp = cbs->pop_up_window;
                                if (awp) {
                                    fprintf(stderr, "Popping down window '%s'\n", awp->get_window_id());
                                    awp->hide();
                                    process_events();
                                }
                            }
                        }
                    }
                }
                else {
                    if (pass == 1) {
                        fprintf(stderr, "Delayed callback %zu/%zu (%s)", curr, count, remote_command);
                    }
                }

                curr++;
            }

            if (pass == 1) fprintf(stderr, "Executing delayed callbacks:\n");
        }
    }
}


// --------------------------------------------------------------------------------
#endif // DEBUG

