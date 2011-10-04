// =============================================================== //
//                                                                 //
//   File      : NT_main.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "nt_internal.h"
#include "ntree.hxx"
#include "nt_cb.hxx"

#include <mg_merge.hxx>
#include <awti_import.hxx>

#include <awt.hxx>
#include <awt_macro.hxx>

#include <aw_advice.hxx>
#include <aw_question.hxx>
#include <aw_awars.hxx>
#include <aw_edit.hxx>
#include <aw_file.hxx>
#include <aw_msg.hxx>
#include <arb_progress.h>
#include <aw_root.hxx>

#include <arbdbt.h>
#include <adGene.h>
#include <arb_version.h>

using namespace std;

AW_HEADER_MAIN

#define nt_assert(bed) arb_assert(bed)

#define NT_SERVE_DB_TIMER 50
#define NT_CHECK_DB_TIMER 200

GBDATA *GLOBAL_gb_main;                             // global gb_main for arb_ntree
NT_global GLOBAL_NT = { 0, 0, 0, false };

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

        AW_repeated_question  question;
        GBDATA               *gb_presets = GB_entry(gb_main, "presets");

        question.add_help("prompt/format_alignments.hlp");

        for (GBDATA *gb_ali = GB_entry(gb_presets, "alignment");
             gb_ali && !err;
             gb_ali = GB_nextEntry(gb_ali))
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
                            int   answer = question.get_answer(qtext, "Format,Skip,Always format,Always skip", "all", false);

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
                    err = GBT_format_alignment(gb_main, ali_name);
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

static GB_ERROR nt_check_database_consistency() {
    // called once on ARB_NTREE startup
    arb_progress("Checking consistency");

    GB_ERROR err  = NT_format_all_alignments(GLOBAL_gb_main);
    if (!err) err = NT_repair_DB(GLOBAL_gb_main);

    return err;
}


static void serve_db_interrupt(AW_root *awr) {
    bool success = GBCMS_accept_calls(GLOBAL_gb_main, false);
    while (success) {
        awr->check_for_remote_command((AW_default)GLOBAL_gb_main, AWAR_NT_REMOTE_BASE);
        success = GBCMS_accept_calls(GLOBAL_gb_main, true);
    }

    awr->add_timed_callback(NT_SERVE_DB_TIMER, (AW_RCB)serve_db_interrupt, 0, 0);
}

static void check_db_interrupt(AW_root *awr) {
    awr->check_for_remote_command((AW_default)GLOBAL_gb_main, AWAR_NT_REMOTE_BASE);
    awr->add_timed_callback(NT_CHECK_DB_TIMER, (AW_RCB)check_db_interrupt, 0, 0);
}

static GB_ERROR startup_mainwindow_and_dbserver(AW_root *aw_root, bool install_client_callback, const char *autorun_macro) {
    GB_ERROR error = NULL;
    nt_create_main_window(aw_root);

    if (GB_read_clients(GLOBAL_gb_main) == 0) { // server
        error = GBCMS_open(":", 0, GLOBAL_gb_main);
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

    if (!error && autorun_macro) awt_execute_macro(aw_root, autorun_macro);
    return error;
}

static ARB_ERROR load_and_startup_main_window(AW_root *aw_root, const char *autorun_macro) {
    char *db_server = aw_root->awar(AWAR_DB_PATH)->read_string();
    GLOBAL_gb_main  = GBT_open(db_server, "rw", "$(ARBHOME)/lib/pts/*");

    ARB_ERROR error;
    if (!GLOBAL_gb_main) {
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
        AWT_announce_db_to_browser(GLOBAL_gb_main, GBS_global_string("ARB database (%s)", db_server));
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
        if (aw_ask_sure(GBS_global_string("Are you sure to delete database %s\nNote: there is no way to undelete it afterwards", db_server))) {
            GB_ERROR error = 0;
            error = GB_delete_database(db_server);
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
    GLOBAL_NT.awr  = aw_root;
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

static void nt_intro_start_merge(AW_window *aw_intro, AW_root *aw_root) {
    nt_assert(contradicted(aw_intro, aw_root)); // one of them is passed!
    if (aw_intro) aw_root = aw_intro->get_root();
    create_MG_main_window(aw_root);
    if (aw_intro) aw_intro->hide();
}

static void nt_intro_start_import(AW_window *aw_intro) {
    aw_intro->hide();

    AW_root *aw_root = aw_intro->get_root();
    aw_root->awar_string(AWAR_DB_PATH)->write_string("noname.arb");
    aw_root->awar_int(AWAR_READ_GENOM_DB, IMP_PLAIN_SEQUENCE); // Default toggle  in window  "Create&import" is Non-Genom
    GLOBAL_gb_main   = open_AWTC_import_window(aw_root, "", true, 0, (AW_RCB)start_main_window_after_import, 0, 0);
}

static AW_window *nt_create_intro_window(AW_root *awr) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(awr, "ARB_INTRO", "ARB INTRO");
    aws->load_xfig("arb_intro.fig");

    aws->callback((AW_CB0)exit);
    aws->at("close");
    aws->create_button("CANCEL", "CANCEL", "C");

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
    static int avoid_recursion = 0;

    if (!avoid_recursion) {
        avoid_recursion = 1;

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

        avoid_recursion = 0;
    }
}

class NtreeCommandLine : virtual Noncopyable {
    int               arg_count;
    char const*const* args;

    bool help_requested;
    bool do_import;
    bool do_export;
    
    const char *macro_name;

public:
    NtreeCommandLine(int argc_, char const*const* argv_)
        : arg_count(argc_-1),
          args(argv_+1),
          help_requested(false),
          do_import(false),
          do_export(false),
          macro_name(NULL)
    {}

    void shift() { ++args; --arg_count; }

    int free_args() const { return arg_count; }
    const char *get_arg(int num) const { return num<arg_count ? args[num] : NULL; }

    bool wants_help() const { return help_requested; }
    bool wants_export() const { return do_export; }
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
                    "Known command line arguments:\n"
                    "\n"
                    "db.arb             => start ARB_NTREE with database db.arb\n"
                    ":                  => start ARB_NTREE and connect to existing db_server\n"
                    "db1.arb db2.arb    => merge databases db1.arb and db2.arb\n"
                    "w/o arguments      => start database browser in current directory\n"
                    "directory          => start database browser in 'directory'\n"
                    "-export            => connect to existing ARB server and export database to noname.arb\n"
                    "-import file       => import 'file' into new database\n"
                    "\n"
                    );
    }

    ARB_ERROR parse() {
        ARB_ERROR error;

        while (!error && arg_count>0 && args[0][0] == '-') {
            const char *opt = args[0]+1;
            if (opt[0] == '-') ++opt; // accept '-' or '--'
            if (strcmp(opt, "help") == 0 || strcmp(opt, "h") == 0) { help_requested = true; }
            else if (strcmp(opt, "export") == 0) { do_export = true; }
            else if (strcmp(opt, "import") == 0) { do_import = true; }
            else if (strcmp(opt, "execute") == 0) { shift(); macro_name = *args; }
            else {
                error = GBS_global_string("Unknown switch '%s'", args[0]);
            }
            shift();
        }
        // non-switch arguments remain in arg_count/args
        if (!error) {
            if (do_import && do_export) error           = "--import can't be used together with --export";
            else if (do_export && arg_count != 0) error = "superfluous argument behind --export";
            else if (do_import && arg_count != 1) error = "expected file-name or -mask name behind --import";
            else if (arg_count>2) error                 = "expected to see up to 2 arguments on command line";
        }
        return error;
    }
};

enum RunMode { NORMAL, IMPORT, EXPORT, MERGE, BROWSE };

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

                        int ans = aw_question(msg, "Browser,Exit");
                        action  = ans ? EXIT : BROWSE_DB;
                    }
                    else {
                        const char *msg = GBS_global_string("Your file is not an original arb file\n(%s)", load_file_err);
                        action          = (Action)aw_question(msg, "Continue (dangerous),Start Converter,Browser,Exit");
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

int main(int argc, char **argv) {
    unsigned long mtime = GB_time_of_file("$(ARBHOME)/lib/message");
    unsigned long rtime = GB_time_of_file("$(HOME)/.arb_prop/msgtime");
    if (mtime > rtime) {
        AW_edit("${ARBHOME}/lib/message");
        system("touch ${HOME}/.arb_prop/msgtime");
    }
    aw_initstatus();
    GB_set_verbose();

    GB_shell shell;
    AW_root *aw_root = AWT_create_root(".arb_prop/ntree.arb", "ARB_NT");

    GLOBAL_NT.awr = aw_root;

    // create some early awars
    // Note: normally you don't like to add your awar-init-function here, but into nt_create_all_awars()

    AW_create_fileselection_awars(aw_root, AWAR_DB, "", ".arb", "noname.arb");
    aw_root->awar_string(AWAR_DB_TYPE, "b");
    aw_root->awar_string(AWAR_DB_NAME, "noname.arb");
    aw_root->awar(AWAR_DB_PATH)->add_callback(AWAR_DB_PATH_changed_cb);

    aw_root->awar_int(AWAR_EXPERT, 0);

    init_Advisor(aw_root);
    AWT_install_cb_guards();

    NtreeCommandLine cl(argc, argv);
    ARB_ERROR        error = cl.parse();

    ARB_declare_global_awars(aw_root, AW_ROOT_DEFAULT);

    if (!error) {
        if (cl.wants_help()) {
            cl.print_help(stderr);
        }
        else {
            RunMode mode = NORMAL;

            if (cl.wants_export())      mode = EXPORT;
            else if (cl.wants_import()) mode = IMPORT;
            else if (cl.wants_merge())  mode = MERGE;

            if (mode == EXPORT) {
                MG_create_all_awars(aw_root, AW_ROOT_DEFAULT, ":", "noname.arb");
                GLOBAL_gb_merge = GBT_open(":", "rw", 0);
                if (!GLOBAL_gb_merge) {
                    error = GB_await_error();
                }
                else {
#if defined(DEBUG)
                    AWT_announce_db_to_browser(GLOBAL_gb_merge, "Current database (:)");
#endif // DEBUG

                    GLOBAL_gb_dest = GBT_open("noname.arb", "cw", 0);
#if defined(DEBUG)
                    AWT_announce_db_to_browser(GLOBAL_gb_dest, "New database (noname.arb)");
#endif // DEBUG

                    MG_start_cb2(NULL, aw_root, true, true);
                    aw_root->main_loop();
                }
            }
            else if (mode == MERGE) {
                MG_create_all_awars(aw_root, AW_ROOT_DEFAULT, cl.get_arg(0), cl.get_arg(1));
                nt_intro_start_merge(0, aw_root);
                aw_root->main_loop();
                nt_assert(0);
            }
            else {
                const char *database         = NULL;
                char       *browser_startdir = strdup(".");

                if (cl.free_args() > 0) database = cl.get_arg(0);

                error = check_argument_for_mode(database, browser_startdir, mode); 
                if (!error) {
                    if (mode == IMPORT) {
                        aw_root->awar_int(AWAR_READ_GENOM_DB, IMP_PLAIN_SEQUENCE);
                        GLOBAL_gb_main = open_AWTC_import_window(aw_root, database, true, 0, (AW_RCB)start_main_window_after_import, 0, 0);
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
                        if (GLOBAL_NT.window_creator) {
                            iws = GLOBAL_NT.window_creator(aw_root, 0);
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
    }

    int exitcode = error ? EXIT_FAILURE : EXIT_SUCCESS;

    if (error) aw_popup_ok(error.deliver());
    else error.expect_no_error();

    delete aw_root;

    return exitcode;
}

