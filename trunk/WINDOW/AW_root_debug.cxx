// =========================================================== //
//                                                             //
//   File      : AW_root_debug.cxx                             //
//   Purpose   :                                               //
//                                                             //
//   Coded by Ralf Westram (coder@reallysoft.de) in May 2009   //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#include "aw_window.hxx"
#include "aw_Xm.hxx"
#include "aw_window_Xm.hxx"
#include "aw_root.hxx"
#include "aw_msg.hxx"

#include <arbdbt.h>
#include <arb_strarray.h>

#include <vector>
#include <iterator>
#include <string>
#include <algorithm>

// do includes above (otherwise depends depend on DEBUG)

#if defined(DEBUG)
// --------------------------------------------------------------------------------

using namespace std;

typedef vector<string> CallbackArray;
typedef CallbackArray::const_iterator CallbackIter;

static GB_HASH *dontCallHash      = 0;
static GB_HASH *alreadyCalledHash = 0;

static void forgetCalledCallbacks() {
    if (alreadyCalledHash) GBS_free_hash(alreadyCalledHash);
    alreadyCalledHash = GBS_create_hash(2500, GB_MIND_CASE);
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

static void forget_dontCallHash() {
    if (dontCallHash) {
        GBS_free_hash(dontCallHash);
        GBS_free_hash(alreadyCalledHash);
        dontCallHash = NULL;
    }
}

static void build_dontCallHash() {
    aw_assert(!dontCallHash);
    dontCallHash = GBS_create_hash(100, GB_MIND_CASE);
    forgetCalledCallbacks();

    atexit(forget_dontCallHash);

    // avoid program termination/restart/etc.
    GBS_write_hash(dontCallHash, "ARB_NT/QUIT",                 1);
    GBS_write_hash(dontCallHash, "quit",                        1);
    GBS_write_hash(dontCallHash, "new_arb",                     1);
    GBS_write_hash(dontCallHash, "restart_arb",                 1);
    GBS_write_hash(dontCallHash, "ARB_EDIT4/QUIT",              1);
    GBS_write_hash(dontCallHash, "ARB_INTRO/CANCEL",            1);
    GBS_write_hash(dontCallHash, "NEIGHBOUR_JOINING/CLOSE",     1);
    GBS_write_hash(dontCallHash, "MERGE_SELECT_DATABASES/QUIT", 1);
    GBS_write_hash(dontCallHash, "quitnstart",                  1);
    GBS_write_hash(dontCallHash, "PARS_PROPS/ABORT",            1);
    GBS_write_hash(dontCallHash, "ARB_PHYLO/QUIT",              1);
    GBS_write_hash(dontCallHash, "SELECT_ALIGNMENT/ABORT",      1);

    // avoid start of some external programs:
#if 1
    GBS_write_hash(dontCallHash, "GDE__User__Start_a_slave_ARB_on_a_foreign_host_/GO",         2);
    GBS_write_hash(dontCallHash, "NAME_SERVER_ADMIN/REMOVE_SUPERFLUOUS_ENTRIES_IN_NAMES_FILE", 2);
    GBS_write_hash(dontCallHash, "GDE__Print__Pretty_print_sequences_slow_/GO",                2);

    GBS_write_hash(dontCallHash, "ARB_NT/EDIT_SEQUENCES",             2);
    GBS_write_hash(dontCallHash, "merge_from",                        2);
    GBS_write_hash(dontCallHash, "CPR_MAIN/HELP",                     2);
    GBS_write_hash(dontCallHash, "HELP/BROWSE",                       2);
    GBS_write_hash(dontCallHash, "mailing_list",                      2);
    GBS_write_hash(dontCallHash, "bug_report",                        2);
    GBS_write_hash(dontCallHash, "HELP/EDIT",                         2);
    GBS_write_hash(dontCallHash, "MACROS/EDIT",                       2);
    GBS_write_hash(dontCallHash, "MACROS/EXECUTE",                    2);
    GBS_write_hash(dontCallHash, "NAME_SERVER_ADMIN/EDIT_NAMES_FILE", 2);
    GBS_write_hash(dontCallHash, "arb_dist",                          2);
    GBS_write_hash(dontCallHash, "arb_pars",                          2);
    GBS_write_hash(dontCallHash, "arb_pars_quick",                    2);
    GBS_write_hash(dontCallHash, "arb_phyl",                          2);
    GBS_write_hash(dontCallHash, "count_different_chars",             2);
    GBS_write_hash(dontCallHash, "corr_mutat_analysis",               2);
    GBS_write_hash(dontCallHash, "export_to_ARB",                     2);
    GBS_write_hash(dontCallHash, "new2_arb_edit4",                    2);
    GBS_write_hash(dontCallHash, "new_arb_edit4",                     2);
    GBS_write_hash(dontCallHash, "primer_design",                     2);
    GBS_write_hash(dontCallHash, "xterm",                             2);
    GBS_write_hash(dontCallHash, "SUBMIT_REG/SEND",                   2);
    GBS_write_hash(dontCallHash, "SUBMIT_BUG/SEND",                   2);
    GBS_write_hash(dontCallHash, "PRINT_CANVAS/PRINT",                2);
    GBS_write_hash(dontCallHash, "PT_SERVER_ADMIN/CREATE_TEMPLATE",   2);
    GBS_write_hash(dontCallHash, "PT_SERVER_ADMIN/EDIT_LOG",          2);
    GBS_write_hash(dontCallHash, "NAME_SERVER_ADMIN/CREATE_TEMPLATE", 2);
    GBS_write_hash(dontCallHash, "SELECT_CONFIGURATION/START",        2);
    GBS_write_hash(dontCallHash, "EXPORT_TREE_AS_XFIG/START_XFIG",    2);
    GBS_write_hash(dontCallHash, "EXPORT_NDS_OF_MARKED/PRINT",        2);
    GBS_write_hash(dontCallHash, "ALIGNER_V2/GO",                     2);
    GBS_write_hash(dontCallHash, "SINA/Start",                        2);
#endif

    // avoid saving
    GBS_write_hash(dontCallHash, "save_changes",           3);
    GBS_write_hash(dontCallHash, "save_props",             3);
    GBS_write_hash(dontCallHash, "save_alitype_props",     3);
    GBS_write_hash(dontCallHash, "save_alispecific_props", 3);
    GBS_write_hash(dontCallHash, "save_DB1",               3);
    GBS_write_hash(dontCallHash, "SAVE_DB/SAVE",           3);
    GBS_write_hash(dontCallHash, "ARB_NT/SAVE",            3);
    GBS_write_hash(dontCallHash, "ARB_NT/SAVE_AS",         3);
    GBS_write_hash(dontCallHash, "ARB_NT/QUICK_SAVE_AS",   3);

    GBS_write_hash(dontCallHash, "User1_search_1/SAVE",            3);
    GBS_write_hash(dontCallHash, "User2_search_1/SAVE",            3);
    GBS_write_hash(dontCallHash, "Probe_search_1/SAVE",            3);
    GBS_write_hash(dontCallHash, "Primer_local_search_1/SAVE",     3);
    GBS_write_hash(dontCallHash, "Primer_region_search_1/SAVE",    3);
    GBS_write_hash(dontCallHash, "Primer_global_search_1/SAVE",    3);
    GBS_write_hash(dontCallHash, "Signature_local_search_1/SAVE",  3);
    GBS_write_hash(dontCallHash, "Signature_region_search_1/SAVE", 3);
    GBS_write_hash(dontCallHash, "Signature_global_search_1/SAVE", 3);

    // avoid confusion by recording, executing or deleting macros
    GBS_write_hash(dontCallHash, "MACROS/DELETE",       1);
    GBS_write_hash(dontCallHash, "MACROS/EDIT",         1);
    GBS_write_hash(dontCallHash, "MACROS/EXECUTE",      1);
    GBS_write_hash(dontCallHash, "MACROS/macro_record", 1);

#if 1
#if defined(WARN_TODO)
#warning crashing - fix later
#endif
    GBS_write_hash(dontCallHash, "ARB_NT/view_probe_group_result", 4);
    GBS_write_hash(dontCallHash, "PT_SERVER_ADMIN/CHECK_SERVER",   4);
    GBS_write_hash(dontCallHash, "ARB_EDIT4/SECEDIT",              4);
    GBS_write_hash(dontCallHash, "sec_edit",                       4);
    GBS_write_hash(dontCallHash, "ARB_EDIT4/RNA3D",                4);
    GBS_write_hash(dontCallHash, "rna3d",                          4);
    GBS_write_hash(dontCallHash, "reload_config",                  4);
    GBS_write_hash(dontCallHash, "LOAD_OLD_CONFIGURATION/LOAD",    4);
    GBS_write_hash(dontCallHash, "table_admin",                    4); // disabled in userland atm
    GBS_write_hash(dontCallHash, "PARS_PROPS/GO",                  4); // has already been executed (designed to run only once)
    GBS_write_hash(dontCallHash, "ARB_PARSIMONY/POP",              4); // pop does not work correctly in all cases (see #528)
    GBS_write_hash(dontCallHash, "new_win",                        4); // 2nd editor window (blocked by #429)
    GBS_write_hash(dontCallHash, "ARB_NT/UNDO",                    4); // doesn't crash, but caused following commands to crash
#endif

    // do not open 2nd ARB_NT window (to buggy)
    GBS_write_hash(dontCallHash, "new_window", 4);

#if 1
#if defined(WARN_TODO)
#warning test callbacks asking questions again later
#endif
    GBS_write_hash(dontCallHash, "ARB_NT/tree_scale_lengths",                            5);
    GBS_write_hash(dontCallHash, "CREATE_USER_MASK/CREATE",                              5);
    GBS_write_hash(dontCallHash, "GDE__Import__Import_sequences_using_Readseq_slow_/GO", 5);
    GBS_write_hash(dontCallHash, "INFO_OF_ALIGNMENT/DELETE",                             5);
    GBS_write_hash(dontCallHash, "LOAD_SELECTION_BOX/LOAD",                              5);
    GBS_write_hash(dontCallHash, "MULTI_PROBE/CREATE_NEW_SEQUENCE",                      5);
    GBS_write_hash(dontCallHash, "PT_SERVER_ADMIN/KILL_ALL_SERVERS",                     5);
    GBS_write_hash(dontCallHash, "PT_SERVER_ADMIN/KILL_SERVER",                          5);
    GBS_write_hash(dontCallHash, "PT_SERVER_ADMIN/UPDATE_SERVER",                        5);
    GBS_write_hash(dontCallHash, "REALIGN_DNA/REALIGN",                                  5);
    // GBS_write_hash(dontCallHash, "SPECIES_QUERY/DELETE_LISTED",                          5);
    GBS_write_hash(dontCallHash, "SPECIES_QUERY/DELETE_LISTED_spec",                     5);
    GBS_write_hash(dontCallHash, "SPECIES_QUERY/SAVELOAD_CONFIG_spec",                   5);
    GBS_write_hash(dontCallHash, "SPECIES_SELECTIONS/RENAME",                            5);
    GBS_write_hash(dontCallHash, "SPECIES_SELECTIONS/STORE_0",                           5);
    GBS_write_hash(dontCallHash, "del_marked",                                           5);
    GBS_write_hash(dontCallHash, "create_group",                                         5);
    GBS_write_hash(dontCallHash, "dcs_threshold",                                        5);
    GBS_write_hash(dontCallHash, "NAME_SERVER_ADMIN/DELETE_OLD_NAMES_FILE",              5);
    GBS_write_hash(dontCallHash, "detail_col_stat",                                      5);
#endif

    // don't call some close-callbacks
    // (needed when they perform cleanup that makes other callbacks from the same window fail)
    GBS_write_hash(dontCallHash, "ARB_IMPORT/CLOSE", 6);

    GB_HASH *autodontCallHash = GBS_create_hash(100, GB_MIND_CASE);
    GBS_hash_do_loop(dontCallHash, auto_dontcall1, autodontCallHash);
    GBS_hash_do_loop(autodontCallHash, auto_dontcall2, dontCallHash);
    GBS_free_hash(autodontCallHash);
}

class StringVectorArray : public ConstStrArray {
    CallbackArray array;
public:
    StringVectorArray(const CallbackArray& a)
        : array(a)
    {
        reserve(a.size());
        for (CallbackArray::iterator id = array.begin(); id != array.end(); ++id) {
            put(id->c_str());
        }
    }
};

inline bool exclude_key(const char *key) {
    if (strncmp(key, "FILTER_SELECT_", 14) == 0) {
        if (strstr(key, "/2filter/2filter/2filter/") != 0) {
            return true;
        }
    }
    else {
        if (strstr(key, "SAVELOAD_CONFIG") != 0) return true;
    }
    return false;
}

inline bool is_wanted_callback(const char *key) {
    return
        GBS_read_hash(alreadyCalledHash, key) == 0 && // dont call twice
        !exclude_key(key); // skip some problematic  callbacks
}

static int sortedByCallbackLocation(const char *k0, long v0, const char *k1, long v1) {
    AW_cb *cbs0 = reinterpret_cast<AW_cb*>(v0);
    AW_cb *cbs1 = reinterpret_cast<AW_cb*>(v1);

    int cmp       = cbs0->compare(*cbs1);
    if (!cmp) cmp = strcmp(k0, k1);

    return cmp;
}

// ------------------------
//      get_action_ids

static long add_wanted_callbacks(const char *key, long value, void *cl_callbacks) {
    if (is_wanted_callback(key)) {
        CallbackArray *callbacks = reinterpret_cast<CallbackArray*>(cl_callbacks);
        callbacks->push_back(string(key));
    }
    return value;
}

ConstStrArray *AW_root::get_action_ids() {
    if (!dontCallHash) build_dontCallHash();
    CallbackArray callbacks;
    GBS_hash_do_sorted_loop(prvt->action_hash, add_wanted_callbacks, GBS_HCF_sortedByKey, &callbacks);
    return new StringVectorArray(callbacks);
}

// --------------------------
//      callallcallbacks

size_t AW_root::callallcallbacks(int mode) {
    // mode == -2 -> mark all as called
    // mode == -1 -> forget called
    // mode == 0 -> call all in alpha-order
    // mode == 1 -> call all in reverse alpha-order
    // mode == 2 -> call all in code-order
    // mode == 3 -> call all in reverse code-order
    // mode == 4 -> call all in random order
    // mode & 8 -> repeat until no uncalled callbacks left

    size_t count     = GBS_hash_elements(prvt->action_hash);
    size_t callCount = 0;

    aw_message(GBS_global_string("Found %zi callbacks", count));

    if (!dontCallHash) build_dontCallHash();

    if (mode>0 && (mode&8)) {
        aw_message("Calling callbacks iterated");
        for (int iter = 1; ; ++iter) { // forever
            size_t thisCount = callallcallbacks(mode&~8); // call all in wanted order
            aw_message(GBS_global_string("%zu callbacks were called (iteration %i)", thisCount, iter));
            if (!thisCount) {
                aw_message("No uncalled callbacks left");
                break;
            }

            callCount += thisCount;
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
                GBS_hash_do_sorted_loop(prvt->action_hash, add_wanted_callbacks, GBS_HCF_sortedByKey, &callbacks);
                break;
            case 2:
            case 3:
                GBS_hash_do_sorted_loop(prvt->action_hash, add_wanted_callbacks, sortedByCallbackLocation, &callbacks);
                break;
            case -2:
                aw_message("Marking all callbacks as \"called\"");
            case 4:
                GBS_hash_do_loop(prvt->action_hash, add_wanted_callbacks, &callbacks);
                break;
            default:
                aw_assert(0);
                break;
        }

        switch (mode) {
            case -2:
            case 0:
            case 2: break;                          // use this order
            case 1:
            case 3: reverse(callbacks.begin(), callbacks.end()); break; // use reverse order
            case 4: random_shuffle(callbacks.begin(), callbacks.end()); break; // use random order
            default: aw_assert(0); break;           // unknown mode
        }

        count = callbacks.size();
        aw_message(GBS_global_string("%zu callbacks were not called yet", count));

        CallbackIter end = callbacks.end();

        for (int pass = 1; pass <= 2; ++pass) {
            size_t       curr = 1;
            CallbackIter cb   = callbacks.begin();

            for (; cb != end; ++cb) {
                const char *remote_command      = cb->c_str();
                const char *remote_command_name = remote_command;
                {
                    const char *slash = strrchr(remote_command, '/');
                    if (slash) remote_command_name = slash+1;
                }

                char firstNameChar = remote_command_name[0];
                bool this_pass     = firstNameChar == '-' ? (pass == 2) : (pass == 1);

                if (this_pass) {
                    GBS_write_hash(alreadyCalledHash, remote_command, 1); // don't call twice

                    if (mode != -2) { // -2 means "only mark as called"
                        AW_cb *cbs    = (AW_cb *)GBS_read_hash(prvt->action_hash, remote_command);
                        bool   skipcb = firstNameChar == '!' || GBS_read_hash(dontCallHash, remote_command);

                        if (!skipcb) {
                            if (cbs->contains(AnyWinCB(AW_help_entry_pressed))) skipcb = true;
                        }

                        if (skipcb) {
                            fprintf(stderr, "Skipped callback %zu/%zu (%s)\n", curr, count, remote_command);
                        }
                        else {
                            fprintf(stderr, "Calling back %zu/%zu (%s)\n", curr, count, remote_command);

                            GB_clear_error();

                            cbs->run_callbacks();
                            callCount++;
                            process_pending_events();

                            if (GB_have_error()) {
                                fprintf(stderr, "Unhandled error in '%s': %s\n", remote_command, GB_await_error());
                            }
                        }
                    }
                }
                else {
                    if (pass == 1) {
                        fprintf(stderr, "Delayed callback %zu/%zu (%s)\n", curr, count, remote_command);
                    }
                }

                curr++;
            }

            if (pass == 1) fprintf(stderr, "Executing delayed callbacks:\n");
        }

        aw_message(GBS_global_string("%zu callbacks are marked as called now", GBS_hash_elements(alreadyCalledHash)));
    }

    return callCount;
}

// --------------------------------------------------------------------------------
#endif // DEBUG

