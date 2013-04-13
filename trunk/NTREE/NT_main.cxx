// =============================================================== //
//                                                                 //
//   File      : NT_main.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ntree.hxx"
#include "nt_internal.h"
#include "nt_cb.hxx"

#include <mg_merge.hxx>
#include <awti_import.hxx>
#include <insdel.h>

#include <awt.hxx>
#include <awt_macro.hxx>

#include <aw_advice.hxx>
#include <aw_question.hxx>
#include <aw_awars.hxx>
#include <aw_edit.hxx>
#include <aw_file.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>

#include <arbdbt.h>
#include <adGene.h>

#include <arb_version.h>
#include <arb_progress.h>
#include <arb_file.h>
#include <awt_sel_boxes.hxx>

using namespace std;

AW_HEADER_MAIN

#define nt_assert(bed) arb_assert(bed)

#define NT_SERVE_DB_TIMER 50
#define NT_CHECK_DB_TIMER 200

NT_global GLOBAL; 

// NT_format_all_alignments may be called after any operation which causes
// unformatted alignments (e.g importing sequences)
//
// It tests all alignments whether they need to be formatted
// and asks the user if they should be formatted.

GB_ERROR NT_format_all_alignments(GBDATA *gb_main) {
    GB_ERROR err = 0;
    GB_push_transaction(gb_main);
    GB_push_my_security(gb_main);

    int ali_count = GBT_count_alignments(gb_main);
    if (ali_count) {
        arb_progress progress("Formatting alignments", ali_count);
        err = GBT_check_data(gb_main, 0);

        AW_repeated_question question;
        question.add_help("prompt/format_alignments.hlp");

        GBDATA *gb_presets = GBT_get_presets(gb_main);
        for (GBDATA *gb_ali = GB_entry(gb_presets, "alignment");
             gb_ali && !err;
             gb_ali        = GB_nextEntry(gb_ali))
        {
            GBDATA *gb_aligned = GB_search(gb_ali, "aligned", GB_INT);

            if (GB_read_int(gb_aligned) == 0) { // sequences in alignment are not formatted
                enum FormatAction {
                    FA_ASK_USER   = 0, // ask user
                    FA_FORMAT_ALL = 1, // format automatically w/o asking
                    FA_SKIP_ALL   = 2, // skip automatically w/o asking
                };
                FormatAction  format_action = FA_ASK_USER;
                GBDATA       *gb_ali_name   = GB_entry(gb_ali, "alignment_name");
                const char   *ali_name      = GB_read_char_pntr(gb_ali_name);

                {
                    bool    is_ali_genom   = strcmp(ali_name, GENOM_ALIGNMENT) == 0;
                    GBDATA *gb_auto_format = GB_entry(gb_ali, "auto_format");

                    if (gb_auto_format) {
                        format_action = FormatAction(GB_read_int(gb_auto_format));
                        if (is_ali_genom) {
                            if (format_action != FA_SKIP_ALL) {
                                format_action = FA_SKIP_ALL; // always skip ali_genom
                                err           = GB_write_int(gb_auto_format, FA_SKIP_ALL);
                            }
                        }
                    }
                    else if (is_ali_genom) {
                        format_action = FA_SKIP_ALL;
                        err           = GBT_write_int(gb_ali, "auto_format", FA_SKIP_ALL);  // always skip
                    }
                }

                bool perform_format = false;
                if (!err) {
                    switch (format_action) {
                        case FA_FORMAT_ALL: perform_format = true; break;
                        case FA_SKIP_ALL:   perform_format = false; break;
                        default: {
                            char *qtext  = GBS_global_string_copy("Alignment '%s' is not formatted. Format?", ali_name);
                            int   answer = question.get_answer("format_alignments", qtext, "Format,Skip,Always format,Always skip", "all", false);

                            switch (answer) {
                                case 2:
                                    err = GBT_write_int(gb_ali, "auto_format", FA_FORMAT_ALL);
                                    // fall-through
                                case 0:
                                    perform_format = true;
                                    break;

                                case 3:
                                    err = GBT_write_int(gb_ali, "auto_format", FA_SKIP_ALL);
                                    break;
                            }

                            free(qtext);
                            break;
                        }
                    }
                }
                if (!err && perform_format) {
                    GB_push_my_security(gb_main);
                    err = ARB_format_alignment(gb_main, ali_name);
                    GB_pop_my_security(gb_main);
                }
            }
            progress.inc_and_check_user_abort(err);
        }
    }

    GB_pop_my_security(gb_main);

    return GB_end_transaction(gb_main, err);
}


// --------------------------------------------------------------------------------

static GB_ERROR check_for_remote_command(AW_root *aw_root, AW_default gb_maind, const char *rm_base) {
    // function has no Motif specific stuff in it :)
    GBDATA *gb_main = (GBDATA *)gb_maind;

    char *awar_action = GBS_global_string_copy("%s/action", rm_base);
    char *awar_value  = GBS_global_string_copy("%s/value", rm_base);
    char *awar_awar   = GBS_global_string_copy("%s/awar", rm_base);
    char *awar_result = GBS_global_string_copy("%s/result", rm_base);

    GB_push_transaction(gb_main);

    char *action   = GBT_readOrCreate_string(gb_main, awar_action, "");
    char *value    = GBT_readOrCreate_string(gb_main, awar_value, "");
    char *tmp_awar = GBT_readOrCreate_string(gb_main, awar_awar, "");

    if (tmp_awar[0]) {
        GB_ERROR error = 0;
        AW_awar *found_awar = aw_root->awar_no_error(tmp_awar);
        if (!found_awar) {
            error = GBS_global_string("Unknown variable '%s'", tmp_awar);
        }
        else {
            if (strcmp(action, "AWAR_REMOTE_READ") == 0) {
                char *read_value = aw_root->awar(tmp_awar)->read_as_string();
                GBT_write_string(gb_main, awar_value, read_value);
#if defined(DUMP_REMOTE_ACTIONS)
                printf("remote command 'AWAR_REMOTE_READ' awar='%s' value='%s'\n", tmp_awar, read_value);
#endif // DUMP_REMOTE_ACTIONS
                free(read_value);
                // clear action (AWAR_REMOTE_READ is just a pseudo-action) :
                action[0]        = 0;
                GBT_write_string(gb_main, awar_action, "");
            }
            else if (strcmp(action, "AWAR_REMOTE_TOUCH") == 0) {
                aw_root->awar(tmp_awar)->touch();
#if defined(DUMP_REMOTE_ACTIONS)
                printf("remote command 'AWAR_REMOTE_TOUCH' awar='%s'\n", tmp_awar);
#endif // DUMP_REMOTE_ACTIONS
                // clear action (AWAR_REMOTE_TOUCH is just a pseudo-action) :
                action[0] = 0;
                GBT_write_string(gb_main, awar_action, "");
            }
            else {
#if defined(DUMP_REMOTE_ACTIONS)
                printf("remote command (write awar) awar='%s' value='%s'\n", tmp_awar, value);
#endif // DUMP_REMOTE_ACTIONS
                error = aw_root->awar(tmp_awar)->write_as_string(value);
            }
        }
        GBT_write_string(gb_main, awar_result, error ? error : "");
        GBT_write_string(gb_main, awar_awar, ""); // tell perl-client call has completed (BIO::remote_awar and BIO:remote_read_awar)

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
            GBT_write_string(gb_main, awar_result, "");
        }
        else {
            aw_message(GB_export_errorf("Unknown action '%s' in macro", action));
            GBT_write_string(gb_main, awar_result, GB_await_error());
        }
        GBT_write_string(gb_main, awar_action, ""); // tell perl-client call has completed (remote_action)
    }

    free(tmp_awar);
    free(value);
    free(action);

    free(awar_result);
    free(awar_awar);
    free(awar_value);
    free(awar_action);

    return 0;
}

static GB_ERROR nt_check_database_consistency() {
    // called once on ARB_NTREE startup
    arb_progress("Checking consistency");

    GB_ERROR err  = NT_format_all_alignments(GLOBAL.gb_main);
    if (!err) err = NT_repair_DB(GLOBAL.gb_main);

    return err;
}


static void serve_db_interrupt(AW_root *awr) {
    bool success = GBCMS_accept_calls(GLOBAL.gb_main, false);
    while (success) {
        check_for_remote_command(awr, (AW_default)GLOBAL.gb_main, AWAR_NT_REMOTE_BASE);
        success = GBCMS_accept_calls(GLOBAL.gb_main, true);
    }

    awr->add_timed_callback(NT_SERVE_DB_TIMER, (AW_RCB)serve_db_interrupt, 0, 0);
}

static void check_db_interrupt(AW_root *awr) {
    check_for_remote_command(awr, (AW_default)GLOBAL.gb_main, AWAR_NT_REMOTE_BASE);
    awr->add_timed_callback(NT_CHECK_DB_TIMER, (AW_RCB)check_db_interrupt, 0, 0);
}

static GB_ERROR startup_mainwindow_and_dbserver(AW_root *aw_root, bool install_client_callback, const char *autorun_macro) {
    GB_ERROR error = NULL;
    nt_create_main_window(aw_root);

    if (GB_read_clients(GLOBAL.gb_main) == 0) { // server
        error = GBCMS_open(":", 0, GLOBAL.gb_main);
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
            aw_root->add_timed_callback(NT_SERVE_DB_TIMER, (AW_RCB)serve_db_interrupt, 0, 0);
            error = nt_check_database_consistency();
        }
    }
    else { // client
        if (install_client_callback) {
            aw_root->add_timed_callback(NT_CHECK_DB_TIMER, (AW_RCB)check_db_interrupt, 0, 0);
        }
    }

    if (!error && autorun_macro) awt_execute_macro(GLOBAL.gb_main, aw_root, autorun_macro);
    return error;
}

static ARB_ERROR load_and_startup_main_window(AW_root *aw_root, const char *autorun_macro) {
    char *db_server = aw_root->awar(AWAR_DB_PATH)->read_string();
    GLOBAL.gb_main  = GBT_open(db_server, "rw");

    ARB_ERROR error;
    if (!GLOBAL.gb_main) {
        error = GB_await_error();
    }
    else {
        aw_root->awar(AWAR_DB_PATH)->write_string(db_server);

#define MAXNAMELEN 35
        int len = strlen(db_server);
        if (len>MAXNAMELEN) {
            char *nameOnly = strrchr(db_server, '/');
            if (nameOnly) {
                nameOnly++;
                len -= (nameOnly-db_server);
                memmove(db_server, nameOnly, len+1);
                if (len>MAXNAMELEN) {
                    strcpy(db_server+MAXNAMELEN-3, "...");
                }
            }
        }
#if defined(DEBUG)
        AWT_announce_db_to_browser(GLOBAL.gb_main, GBS_global_string("ARB database (%s)", db_server));
#endif // DEBUG

        GB_ERROR problem = startup_mainwindow_and_dbserver(aw_root, true, autorun_macro);
        aw_message_if(problem); // no need to terminate ARB
    }

    free(db_server);
    return error;
}

static void nt_delete_database(AW_window *aww) {
    AW_root *aw_root   = aww->get_root();
    char    *db_server = aw_root->awar(AWAR_DB_PATH)->read_string();

    if (strlen(db_server)) {
        if (aw_ask_sure(NULL, GBS_global_string("Are you sure to delete database %s\nNote: there is no way to undelete it afterwards", db_server))) {
            GB_ERROR error = GB_delete_database(db_server);
            if (error) {
                aw_message(error);
            }
            else {
                aw_root->awar(AWAR_DB_FILTER)->touch();
            }
        }
    }
    else {
        aw_message("No database selected");
    }
    free(db_server);
}

static void start_main_window_after_import(AW_root *aw_root) {
    GLOBAL.aw_root  = aw_root;
    aw_message_if(startup_mainwindow_and_dbserver(aw_root, false, NULL));
}

static void nt_intro_start_existing(AW_window *aw_intro) {
    aw_intro->hide();
    ARB_ERROR error = load_and_startup_main_window(aw_intro->get_root(), NULL);
    if (error) {
        aw_intro->show();
        aw_popup_ok(error.deliver());
    }
    else {
        error.expect_no_error();
    }
}

static void nt_intro_start_merge(AW_window *aw_intro) {
    AW_root    *aw_root    = aw_intro->get_root();
    const char *dir        = aw_root->awar("tmp/nt/arbdb/directory")->read_char_pntr();
    char       *merge_args = GBS_global_string_copy("'%s' '%s'", dir, dir);

    nt_restart(aw_root, merge_args, false); //  call arb_ntree as merge-tool on exit
}

static void nt_intro_start_import(AW_window *aw_intro) {
    aw_intro->hide();

    AW_root *aw_root = aw_intro->get_root();
    aw_root->awar_string(AWAR_DB_PATH)->write_string("noname.arb");
    aw_root->awar_int(AWAR_READ_GENOM_DB, IMP_PLAIN_SEQUENCE); // Default toggle  in window  "Create&import" is Non-Genom
    GLOBAL.gb_main   = open_AWTC_import_window(aw_root, "", true, 0, (AW_RCB)start_main_window_after_import, 0, 0);
}

static AW_window *nt_create_intro_window(AW_root *awr) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(awr, "ARB_INTRO", "ARB INTRO");
    aws->load_xfig("arb_intro.fig");

    aws->callback(nt_exit, EXIT_SUCCESS);
    aws->at("close");
    aws->create_button("EXIT", "Exit", "x");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"arb_intro.hlp");
    aws->create_button("HELP", "HELP", "H");

    AW_create_fileselection(aws, "tmp/nt/arbdb");

    aws->button_length(0);

    aws->at("logo");
    aws->create_button(0, "#logo.xpm");

    aws->at("version");
    aws->create_button(0, GBS_global_string("Version " ARB_VERSION), 0); // version

    aws->at("copyright");
    aws->create_button(0, GBS_global_string("(C) 1993-" ARB_BUILD_YEAR), 0);

    aws->at("old");
    aws->callback(nt_intro_start_existing);
    aws->create_autosize_button("OPEN_SELECTED", "OPEN SELECTED", "O");

    aws->at("del");
    aws->callback(nt_delete_database);
    aws->create_autosize_button("DELETE_SELECTED", "DELETE SELECTED");

    aws->at("new_complex");
    aws->callback(nt_intro_start_import);
    aws->create_autosize_button("CREATE_AND_IMPORT", "CREATE AND IMPORT", "I");

    aws->at("merge");
    aws->callback((AW_CB1)nt_intro_start_merge, 0);
    aws->create_autosize_button("MERGE_TWO_DATABASES", "MERGE TWO ARB DATABASES", "O");

    aws->at("expert");
    aws->create_toggle(AWAR_EXPERT);

    return aws;
}

static void AWAR_DB_PATH_changed_cb(AW_root *awr) {
    static bool avoid_recursion = false;

    if (!avoid_recursion) {
        LocallyModify<bool> flag(avoid_recursion, true);

        char *value  = awr->awar(AWAR_DB_PATH)->read_string();
        char *lslash = strrchr(value, '/');

        char *name = lslash ? lslash+1 : value;
#if defined(DEBUG)
        printf("writing '%s' to AWAR_DB_NAME\n", name);
#endif // DEBUG
        awr->awar(AWAR_DB_NAME)->write_string(name);

        if (lslash) {               // update value of directory
            lslash[0] = 0;
            awr->awar(AWAR_DB"directory")->write_string(value);
            lslash[0] = '/';
        }

        free(value);
    }
}

class NtreeCommandLine : virtual Noncopyable {
    int               arg_count;
    char const*const* args;

    bool help_requested;
    bool do_import;
    
    const char *macro_name;

public:
    NtreeCommandLine(int argc_, char const*const* argv_)
        : arg_count(argc_-1),
          args(argv_+1),
          help_requested(false),
          do_import(false),
          macro_name(NULL)
    {}

    void shift() { ++args; --arg_count; }

    int free_args() const { return arg_count; }
    const char *get_arg(int num) const { return num<arg_count ? args[num] : NULL; }

    bool wants_help() const { return help_requested; }
    bool wants_import() const { return do_import; }
    bool wants_merge() const { return arg_count == 2; }

    const char *autorun_macro() const { return macro_name; }

    void print_help(FILE *out) const {
        fprintf(out,
                "\n"
                "arb_ntree version " ARB_VERSION "\n"
                "(C) 1993-" ARB_BUILD_YEAR " Lehrstuhl fuer Mikrobiologie - TU Muenchen\n"
                "http://www.arb-home.de/\n"
#if defined(SHOW_WHERE_BUILD)
                "(version build by: " ARB_BUILD_USER "@" ARB_BUILD_HOST ")\n"
#endif // SHOW_WHERE_BUILD
                "\n"
                "Possible usage:\n"
                "  arb_ntree               => start ARB (intro)\n"
                "  arb_ntree DB            => start ARB with DB\n"
                "  arb_ntree DB1 DB2       => merge from DB1 into DB2\n"
                "  arb_ntree --import FILE => import FILE into new database (FILE may be a filemask)\n"
                "\n"
                "Additional arguments possible with command lines above:\n"
                "  --execute macroname     => execute macro 'macroname' after startup\n"
                "\n"
                "Each DB argument may be one of the following:\n"
                "  database.arb            -> use existing or new database\n"
                "  \":\"                     -> connect to database of a running instance of ARB\n"
                "  directory               -> prompt for databases inside directory\n"
                "  filemask                -> also prompt for DB, but more specific (e.g. \"prot*.arb\")\n"
                "\n"
            );
    }

    ARB_ERROR parse() {
        ARB_ERROR error;

        while (!error && arg_count>0 && args[0][0] == '-') {
            const char *opt = args[0]+1;
            if (opt[0] == '-') ++opt; // accept '-' or '--'
            if (strcmp(opt, "help") == 0 || strcmp(opt, "h") == 0) { help_requested = true; }
            else if (strcmp(opt, "execute") == 0) { shift(); macro_name = *args; }
            else if (strcmp(opt, "import") == 0) { do_import = true; }
            else error = GBS_global_string("Unknown switch '%s'", args[0]);
            shift();
        }
        // non-switch arguments remain in arg_count/args
        if (!error) {
            if (do_import && arg_count != 1) error = "expected exactly one file-name or file-mask behind --import";
            else if (arg_count>2)            error = "too many stray arguments given (max. 2 accepted)";
        }
        return error;
    }
};

enum ArgType { // args that might be specified on command line (for DB or FILE; see above)
    RUNNING_DB,
    DIRECTORY,
    EXISTING_DB,
    NEW_DB,
    FILEMASK,
    EXISTING_FILE,
    UNKNOWN_ARG,
};

inline bool has_arb_suffix(const char *arg) {
    const char *suffix = strstr(arg, ".arb");
    if (suffix) {
        return strcmp(suffix, ".arb") == 0;
    }
    return false;
}

static ArgType detectArgType(const char *arg) {
    if (strcmp(arg, ":") == 0) return RUNNING_DB;
    if (GB_is_directory(arg)) return DIRECTORY;
    if (strpbrk(arg, "*?") != 0) return FILEMASK;
    if (has_arb_suffix(arg)) {
        return GB_is_regularfile(arg) ? EXISTING_DB : NEW_DB;
    }

    GB_ERROR load_file_err = GBT_check_arb_file(arg);
    if (!load_file_err) return EXISTING_DB;

    return GB_is_regularfile(arg) ? EXISTING_FILE : UNKNOWN_ARG;
}

enum RunMode { NORMAL, IMPORT, MERGE, BROWSE };

static ARB_ERROR check_argument_for_mode(const char *database, char *&browser_startdir, RunMode& mode) {
    // Check whether 'database' is a
    // - ARB database
    // - directory name
    // - file to import
    //
    // Modify 'mode' occordingly.
    // Set 'browser_startdir'

    ARB_ERROR error;
    if (mode != IMPORT) {
        if (!database) mode = BROWSE;
        else {
            GB_ERROR load_file_err = GBT_check_arb_file(database);
            if (load_file_err) {
                char *full_path = AW_unfold_path("PWD", database);

                enum Action { LOAD_DB, CONVERT_DB, BROWSE_DB, EXIT, NOIDEA };
                Action action = NOIDEA;
                if (GB_is_directory(full_path)) {
                    action = BROWSE_DB;
                }
                else {
                    if (!GB_is_regularfile(full_path)) {
                        const char *msg = GBS_global_string("'%s' is neither a known option nor a legal file- or directory-name.\n(Error: %s)",
                                                            full_path, load_file_err);

                        int ans = aw_question(NULL, msg, "Browser,Exit");
                        action  = ans ? EXIT : BROWSE_DB;
                    }
                    else {
                        const char *msg = GBS_global_string("Your file is not an original arb file\n(%s)", load_file_err);
                        action          = (Action)aw_question(NULL, msg, "Continue (dangerous),Start Converter,Browser,Exit");
                    }
                }

                switch (action) {
                    case CONVERT_DB:    mode = IMPORT; break;
                    case LOAD_DB:       break;
                    case NOIDEA:        nt_assert(0);
                    case EXIT:          error = "User abort"; break;
                    case BROWSE_DB: {
                        char *dir = nulldup(full_path);
                        while (dir && !GB_is_directory(dir)) freeset(dir, AW_extract_directory(dir));

                        if (dir) {
                            nt_assert(GB_is_directory(dir));
                            reassign(browser_startdir, dir);
                            mode = BROWSE;
                        }
                        free(dir);
                        break;
                    }
                
                }
                free(full_path);
            }
        }
    }
    
    return error;
}

class SelectedDatabase : virtual Noncopyable {
    GBDATA*&  gb_main;
    char     *name;
    ArgType   type;
    char     *role;

    void fix_name() {
        if (type != RUNNING_DB && name[0]) {
            const char *changed;

            if (type == NEW_DB) {
                changed = GB_unfold_in_directory(GB_getcwd(), name);
            }
            else {
                changed = GB_canonical_path(name);
            }

            if (strcmp(name, changed) != 0) {
                reselect_file(changed);
            }
        }
    }

public:
    SelectedDatabase(GBDATA*& gb_main_, const char *name_, const char *role_)
        : gb_main(gb_main_),
          name(strdup(name_)),
          type(detectArgType(name)),
          role(strdup(role_))
    {
        fix_name();
    }
    ~SelectedDatabase() {
        free(role);
        free(name);
    }

    ArgType arg_type() const { return type; }

    bool needs_to_prompt() const {
        return type == DIRECTORY || type == FILEMASK || type == UNKNOWN_ARG;
    }

    const char *get_fullname() const { return name; }
    const char *get_nameonly() const {
        const char *slash = strrchr(name, '/');
        return slash ? slash+1 : name;
    }
    const char *get_role() const { return role; }
    const char *get_description() const { return GBS_global_string("%s (%s)", get_role(), get_nameonly()); }

    ArgType get_type() const { return type; }

    const char *get_dir() const {
        const char *dir = NULL;
        switch (type) {
            case DIRECTORY:
                dir = get_fullname();
                break;

            case NEW_DB:
            case FILEMASK:
            case EXISTING_FILE:
            case EXISTING_DB: {
                char *dir_copy;
                GB_split_full_path(name, &dir_copy, NULL, NULL, NULL);

                static SmartCharPtr dir_store;
                dir_store = dir_copy;

                dir = dir_copy;
                break;
            }
            case UNKNOWN_ARG:
            case RUNNING_DB:
                dir = ".";
                break;
        }
        return dir;
    }

    const char *get_mask() const {
        const char *mask;
        switch (type) {
            case FILEMASK: {
                char *mask_copy;
                GB_split_full_path(name, NULL, &mask_copy, NULL, NULL);

                static SmartCharPtr mask_store;
                mask_store = mask_copy;

                mask = mask_copy;
                break;
            }
            default:
                mask = ".arb";
                break;
        }
        return mask;
    }

    void reselect_file(const char *file) {
        freedup(name, file);
        type = detectArgType(name);
        fix_name();
    }
    void reselect_from_awar(AW_root *aw_root, const char *awar_name) {
        if (awar_name) {
            const char *file = aw_root->awar(awar_name)->read_char_pntr();
            if (file) reselect_file(file);
        }
    }

    GB_ERROR open_db_for_merge(bool is_source_db);
};

GB_ERROR SelectedDatabase::open_db_for_merge(bool is_source_db) {
    GB_ERROR    error    = NULL;
    const char *openMode = "rw";

    switch (get_type()) {
        case DIRECTORY:
        case FILEMASK:
            GBK_terminate("Program logic error (should have been prompted for explicit DB name)");
            break;

        case UNKNOWN_ARG:
        case EXISTING_FILE:
            error = GBS_global_string("'%s' is no arb-database", get_fullname());
            break;

        case NEW_DB:
            if (is_source_db) {
                error = GBS_global_string("'%s' has to exist", get_fullname());
                break;
            }
            openMode = "crw"; // allow to create DB
            break;

        case EXISTING_DB:
        case RUNNING_DB:
            break;
    }

    if (!error) {
        gb_main             = GBT_open(get_fullname(), openMode);
        if (!gb_main) error = GB_await_error();
        else {
            IF_DEBUG(AWT_announce_db_to_browser(gb_main, get_description()));
        }
    }

    return error;
}

struct merge_scheme {
    SelectedDatabase *src;
    SelectedDatabase *dst;

    char *awar_src;
    char *awar_dst;

    GB_ERROR error;

    merge_scheme(SelectedDatabase *src_, SelectedDatabase *dst_)
        : src(src_),
          dst(dst_),
          awar_src(NULL),
          awar_dst(NULL),
          error(NULL)
    {}

    void open_dbs() {
        if (!error) error = src->open_db_for_merge(true);
        if (!error) error = dst->open_db_for_merge(false);
    }

    void fix_dst(AW_root *aw_root) { dst->reselect_from_awar(aw_root, awar_dst); }
    void fix_src(AW_root *aw_root) { src->reselect_from_awar(aw_root, awar_src); }

    bool knows_dbs() const { return !src->needs_to_prompt() && !dst->needs_to_prompt(); }
};

static bool merge_tool_running_as_client = true; // default to safe state (true avoids call of 'arb_clean' at exit)
static void exit_from_merge(const char *restart_args) {
    if (merge_tool_running_as_client) { // there is a main ARB running
        if (restart_args) nt_start(restart_args, true);
        exit(EXIT_SUCCESS); // exit w/o killing clients (as nt_exit() does)
    }
    else {
        nt_restart(AW_root::SINGLETON, restart_args ? restart_args : "", false);
        nt_assert(0);
    }
}

static void merge_startup_abort_cb(AW_window *, AW_CL) {
    fputs("Error: merge aborted by user\n", stderr);
    exit_from_merge(NULL);
}

static AW_window *merge_startup_error_window(AW_root *aw_root, AW_CL cl_error) {
    AW_window_message *aw_msg = new AW_window_message;
    GB_ERROR           error  = GB_ERROR(cl_error);

    aw_msg->init(aw_root, "ARB merge error", false);
    aw_msg->recalc_size_atShow(AW_RESIZE_DEFAULT); // force size recalc (ignores user size)

    aw_msg->at(10, 10);
    aw_msg->auto_space(10, 10);

    aw_msg->create_autosize_button(NULL, error, "", 2);
    aw_msg->at_newline();

    aw_msg->callback(merge_startup_abort_cb, 0);
    aw_msg->create_autosize_button("OK", "Ok", "O", 2);

    aw_msg->window_fit();

    return aw_msg;
}
static AW_window *startup_merge_main_window(AW_root *aw_root, AW_CL cl_merge_scheme) {
    merge_scheme *ms = (merge_scheme*)cl_merge_scheme;

    ms->fix_src(aw_root);
    ms->fix_dst(aw_root);

    if (ms->knows_dbs()) {
        ms->open_dbs();
    }
    else {
        ms->error = "Need two database to merge";
    }

    AW_window *aw_result;
    if (!ms->error) {
        MERGE_create_db_file_awars(aw_root, AW_ROOT_DEFAULT, ms->src->get_fullname(), ms->dst->get_fullname());
        bool dst_is_new = ms->dst->arg_type() == NEW_DB;
        aw_result       = MERGE_create_main_window(aw_root, dst_is_new, exit_from_merge);
    }
    else {
        aw_result = merge_startup_error_window(aw_root, AW_CL(ms->error));
    }
    delete ms; // was allocated in startup_gui()
    return aw_result;
}

static AW_window *startup_merge_prompting_for_nonexplicit_dst_db(AW_root *aw_root, AW_CL cl_merge_scheme) {
    merge_scheme *ms = (merge_scheme*)cl_merge_scheme;
    AW_window    *aw_result;

    if (ms->dst->needs_to_prompt()) {
        aw_result = awt_create_load_box(aw_root, "Select", ms->dst->get_role(),
                                        ms->dst->get_dir(), ms->dst->get_mask(),
                                        &(ms->awar_dst),
                                        0,
                                        startup_merge_main_window,
                                        merge_startup_abort_cb, "Cancel",
                                        cl_merge_scheme);
    }
    else {
        aw_result = startup_merge_main_window(aw_root, cl_merge_scheme);
    }
    return aw_result;
}

static AW_window *startup_merge_prompting_for_nonexplicit_dbs(AW_root *aw_root, AW_CL cl_merge_scheme) {
    // if src_spec or dst_spec needs to be prompted for -> startup prompters with callbacks bound to continue
    // otherwise just startup merge

    merge_scheme *ms = (merge_scheme*)cl_merge_scheme;
    AW_window    *aw_result;

    if (ms->src->needs_to_prompt()) {
        aw_result = awt_create_load_box(aw_root, "Select", ms->src->get_role(),
                                        ms->src->get_dir(), ms->src->get_mask(),
                                        &(ms->awar_src),
                                        0,
                                        startup_merge_prompting_for_nonexplicit_dst_db,
                                        merge_startup_abort_cb, "Cancel",
                                        cl_merge_scheme);
    }
    else {
        aw_result = startup_merge_prompting_for_nonexplicit_dst_db(aw_root, cl_merge_scheme);
    }
    return aw_result;
}

static void startup_gui(NtreeCommandLine& cl, ARB_ERROR& error) {
    {
        char *message = strdup(GB_path_in_ARBLIB("message"));
        char *stamp   = strdup(GB_path_in_arbprop("msgtime"));
        if (GB_time_of_file(message)>GB_time_of_file(stamp)) {
            AW_edit(message);
            aw_message_if(GBK_system(GBS_global_string("touch %s", stamp)));
        }
        free(stamp);
        free(message);
    }

    // create some early awars
    // Note: normally you don't like to add your awar-init-function here, but into nt_create_all_awars()

    AW_root* aw_root = GLOBAL.aw_root;

    AW_create_fileselection_awars(aw_root, AWAR_DB, "", ".arb", "noname.arb");
    aw_root->awar_string(AWAR_DB_TYPE, "b");
    aw_root->awar_string(AWAR_DB_NAME, "noname.arb");
    aw_root->awar(AWAR_DB_PATH)->add_callback(AWAR_DB_PATH_changed_cb);

    aw_root->awar_int(AWAR_EXPERT, 0);

    AWT_install_cb_guards();

    ARB_declare_global_awars(aw_root, AW_ROOT_DEFAULT);

    if (!error) {
        nt_assert(!cl.wants_help());
                
        RunMode mode = NORMAL;

        if      (cl.wants_merge())  mode = MERGE;
        else if (cl.wants_import()) mode = IMPORT;

        if (mode == MERGE) {
            MERGE_create_all_awars(aw_root, AW_ROOT_DEFAULT);

            merge_scheme *ms = new merge_scheme( // freed in startup_merge_main_window()
                new SelectedDatabase(GLOBAL_gb_src, cl.get_arg(0), "Source DB for merge"),
                new SelectedDatabase(GLOBAL_gb_dst, cl.get_arg(1), "Destination DB for merge"));

            merge_tool_running_as_client = ms->src->arg_type() == RUNNING_DB || ms->dst->arg_type() == RUNNING_DB;

            if (ms->src->arg_type() == RUNNING_DB && ms->dst->arg_type() == RUNNING_DB) {
                error = "You cannot merge from running to running DB";
            }
            else {
                AW_window *aww = startup_merge_prompting_for_nonexplicit_dbs(aw_root, AW_CL(ms));

                nt_assert(contradicted(aww, error));

                if (!error) {
                    aww->show();
                    aw_root->main_loop();
                    nt_assert(0);
                }
            }
        }
        else {
            const char *database         = NULL;
            char       *browser_startdir = strdup(".");

            if (cl.free_args() > 0) database = cl.get_arg(0);

            error = check_argument_for_mode(database, browser_startdir, mode);
            if (!error) {
                if (mode == IMPORT) {
                    aw_root->awar_int(AWAR_READ_GENOM_DB, IMP_PLAIN_SEQUENCE);
                    GLOBAL.gb_main = open_AWTC_import_window(aw_root, database, true, 0, (AW_RCB)start_main_window_after_import, 0, 0);
                    aw_root->main_loop();
                }
                else if (mode == NORMAL) {
                    aw_root->awar(AWAR_DB_PATH)->write_string(database);
                    error = load_and_startup_main_window(aw_root, cl.autorun_macro());
                    if (!error) aw_root->main_loop();
                }
                else if (mode == BROWSE) {
                    aw_root->awar(AWAR_DB"directory")->write_string(browser_startdir);
                    char *latest = GB_find_latest_file(browser_startdir, "/\\.(arb|a[0-9]{2})$/");
                    if (latest) {
                        int l = strlen(latest);
                        strcpy(latest+l-3, "arb");
                        aw_root->awar(AWAR_DB_PATH)->write_string(latest);
                        free(latest);
                    }
                    AW_window *iws;
                    if (GLOBAL.window_creator) {
                        iws = GLOBAL.window_creator(aw_root, 0);
                    }
                    else {
                        iws = nt_create_intro_window(aw_root);
                    }
                    iws->show();
                    aw_root->main_loop();
                }
            }
            free(browser_startdir);
        }
    }

    if (error) aw_popup_ok(error.preserve());
    delete aw_root;
}

int ARB_main(int argc, char *argv[]) {
    aw_initstatus();
    GB_set_verbose();

    GB_shell shell;
    AW_root *aw_root = AWT_create_root("ntree.arb", "ARB_NT", &argc, &argv);

    GLOBAL.aw_root = aw_root;

    NtreeCommandLine cl(argc, argv);
    ARB_ERROR        error = cl.parse();

    if (!error) {
        if (cl.wants_help()) {
            cl.print_help(stderr);
        }
        else {
            startup_gui(cl, error);
        }
    }

    int exitcode = EXIT_SUCCESS;
    if (error) {
        exitcode = EXIT_FAILURE;
        fprintf(stderr, "arb_ntree: Error: %s\n", error.deliver());
    }
    else {
        error.expect_no_error();
    }

    return exitcode;
}

