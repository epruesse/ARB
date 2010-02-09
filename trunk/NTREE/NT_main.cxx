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
#include <aw_advice.hxx>
#include <awt.hxx>
#include <aw_global.hxx>
#include <aw_question.hxx>
#include <aw_awars.hxx>
#include <aw_edit.hxx>
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

    aw_status("Checking alignments");
    err = GBT_check_data(gb_main, 0);

    AW_repeated_question  question;
    GBDATA               *gb_presets = GB_entry(gb_main, "presets");

    question.add_help("prompt/format_alignments.hlp");

    for (GBDATA *gb_ali = GB_entry(gb_presets, "alignment"); gb_ali && !err; gb_ali = GB_nextEntry(gb_ali)) {
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
                aw_status(GBS_global_string("Formatting '%s'", ali_name));
                GB_push_my_security(gb_main);
                err = GBT_format_alignment(gb_main, ali_name);
                GB_pop_my_security(gb_main);
            }
        }
    }

    GB_pop_my_security(gb_main);

    return GB_end_transaction(gb_main, err);
}


// --------------------------------------------------------------------------------



// called once on ARB_NTREE startup
//
static GB_ERROR nt_check_database_consistency() {
    aw_openstatus("Checking database...");

    GB_ERROR err = NT_format_all_alignments(GLOBAL_gb_main);
    if (!err) err = NT_repair_DB(GLOBAL_gb_main);

    aw_closestatus();
    return err;
}


void serve_db_interrupt(AW_root *awr) {
    bool success = GBCMS_accept_calls(GLOBAL_gb_main, false);
    while (success) {
        awr->check_for_remote_command((AW_default)GLOBAL_gb_main, AWAR_NT_REMOTE_BASE);
        success = GBCMS_accept_calls(GLOBAL_gb_main, true);
    }

    awr->add_timed_callback(NT_SERVE_DB_TIMER, (AW_RCB)serve_db_interrupt, 0, 0);
}

void check_db_interrupt(AW_root *awr) {
    awr->check_for_remote_command((AW_default)GLOBAL_gb_main, AWAR_NT_REMOTE_BASE);
    awr->add_timed_callback(NT_CHECK_DB_TIMER, (AW_RCB)check_db_interrupt, 0, 0);
}

GB_ERROR create_nt_window(AW_root *aw_root) {
    AW_window *aww;
    GB_ERROR error = GB_request_undo_type(GLOBAL_gb_main, GB_UNDO_NONE);
    if (error) aw_message(error);
    nt_create_all_awars(aw_root, AW_ROOT_DEFAULT);
    aww = create_nt_main_window(aw_root, 0);
    aww->show();
    error = GB_request_undo_type(GLOBAL_gb_main, GB_UNDO_UNDO);
    if (error) aw_message(error);
    return error;
}

// after intro
void nt_main_startup_main_window(AW_root *aw_root) {
    create_nt_window(aw_root);

    if (GB_read_clients(GLOBAL_gb_main)==0) { // i am the server
        GB_ERROR error = GBCMS_open(":", 0, GLOBAL_gb_main);
        if (error) {
            aw_message(
                       "THIS PROGRAM HAS PROBLEMS TO OPEN INTERCLIENT COMMUNICATION !!!\n"
                       "(MAYBE THERE IS ALREADY ANOTHER SERVER RUNNING)\n\n"
                       "You cannot use any EDITOR or other external SOFTWARE with this dataset!\n\n"
                       "Advice: Close ARB again, open a console, type 'arb_clean' and restart arb.\n"
                       "Caution: Any unsaved data in an eventually running ARB will be lost!\n");
        }
        else {
            aw_root->add_timed_callback(NT_SERVE_DB_TIMER, (AW_RCB)serve_db_interrupt, 0, 0);
            error = nt_check_database_consistency();
            if (error) aw_message(error);
        }
    }
    else {
        aw_root->add_timed_callback(NT_CHECK_DB_TIMER, (AW_RCB)check_db_interrupt, 0, 0);
    }
}

int main_load_and_startup_main_window(AW_root *aw_root) // returns 0 when successfull
{

    char *db_server = aw_root->awar(AWAR_DB_PATH)->read_string();
    GLOBAL_gb_main = GBT_open(db_server, "rw", "$(ARBHOME)/lib/pts/*");

    if (!GLOBAL_gb_main) {
        aw_popup_ok(GB_await_error());
        return -1;
    }

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

    free(db_server);
    nt_main_startup_main_window(aw_root);

    return 0;
}

void nt_delete_database(AW_window *aww) {
    char *db_server = aww->get_root()->awar(AWAR_DB_PATH)->read_string();
    if (strlen(db_server)) {
        if (aw_ask_sure(GBS_global_string("Are you sure to delete database %s\nNote: there is no way to undelete it afterwards", db_server))) {
            GB_ERROR error = 0;
            error = GB_delete_database(db_server);
            if (error) {
                aw_message(error);
            }
            else {
                aww->get_root()->awar(AWAR_DB"filter")->touch();
            }
        }
    }
    else {
        aw_message("No database selected");
    }
    free(db_server);
}

// after import !!!!!
void main3(AW_root *aw_root)
{

    GLOBAL_NT.awr = aw_root;
    create_nt_window(aw_root);

    if (GB_read_clients(GLOBAL_gb_main)==0) {
        GB_ERROR error = GBCMS_open(":", 0, GLOBAL_gb_main);
        if (error) {
            aw_message("THIS PROGRAM IS NOT THE ONLY DB SERVER !!!\nDON'T USE ANY ARB PROGRAM !!!!");
        }
        else {
            aw_root->add_timed_callback(NT_SERVE_DB_TIMER, (AW_RCB)serve_db_interrupt, 0, 0);
            error = nt_check_database_consistency();
            if (error) aw_message(error);
        }
    }
    return;
}

void nt_intro_start_old(AW_window *aws)
{
    aws->hide();
    if (main_load_and_startup_main_window(aws->get_root())) {
        aws->show();
    }
}

void nt_intro_start_merge(AW_window *aws, AW_root *awr) {
    if (aws) awr = aws->get_root();
    create_MG_main_window(awr);
    if (aws) aws->hide();
}

void nt_intro_start_import(AW_window *aws)
{
    aws->hide();
    aws->get_root()->awar_string(AWAR_DB_PATH)->write_string("noname.arb");
    aws->get_root()->awar_int(AWAR_READ_GENOM_DB, IMP_PLAIN_SEQUENCE); // Default toggle  in window  "Create&import" is Non-Genom
    GLOBAL_gb_main = open_AWTC_import_window(aws->get_root(), "", true, 0, (AW_RCB)main3, 0, 0);
}

AW_window *nt_create_intro_window(AW_root *awr)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init(awr, "ARB_INTRO", "ARB INTRO");
    aws->load_xfig("arb_intro.fig");

    aws->callback((AW_CB0)exit);
    aws->at("close");
    aws->create_button("CANCEL", "CANCEL", "C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"arb_intro.hlp");
    aws->create_button("HELP", "HELP", "H");

    awt_create_fileselection(aws, "tmp/nt/arbdb");

    aws->button_length(0);

    aws->at("logo");
    aws->create_button(0, "#logo.xpm");

    aws->at("version");
    aws->create_button(0, GBS_global_string("Version " ARB_VERSION), 0); // version

    aws->at("copyright");
    aws->create_button(0, GBS_global_string("(C) 1993-" ARB_BUILD_YEAR), 0);

    aws->at("old");
    aws->callback(nt_intro_start_old);
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

    return (AW_window *)aws;
}

void AD_set_default_root(AW_root *aw_root);

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

int main(int argc, char **argv) {
    const char *db_server = ":";

    unsigned long mtime = GB_time_of_file("$(ARBHOME)/lib/message");
    unsigned long rtime = GB_time_of_file("$(HOME)/.arb_prop/msgtime");
    if (mtime > rtime) {
        AW_edit("${ARBHOME}/lib/message");
        system("touch ${HOME}/.arb_prop/msgtime");
    }
    aw_initstatus();
    GB_set_verbose();

    AW_root *aw_root = new AW_root;
    GLOBAL_NT.awr    = aw_root;
    AD_set_default_root(aw_root);                   // set default for AD_map_viewer (as long as no info-box was opened)

    AW_default aw_default = AWT_open_properties(aw_root, ".arb_prop/ntree.arb");
    aw_root->init_variables(aw_default);
    aw_root->init_root("ARB_NT", false);

    // create some early awars
    // Note: normally you don't like to add your awar-init-function here, but into nt_create_all_awars()

    aw_create_fileselection_awars(aw_root, AWAR_DB, "", ".arb", "noname.arb", aw_default);
    aw_root->awar_string(AWAR_DB"type", "b", aw_default);

    aw_root->awar_int(AWAR_EXPERT, 0, aw_default);

    aw_root->awar_string(AWAR_DB_NAME, "noname.arb", aw_default);
    aw_root->awar(AWAR_DB_PATH)->add_callback(AWAR_DB_PATH_changed_cb);

    init_Advisor(aw_root, aw_default);
    AWT_install_cb_guards();

    if (argc==3) {                                  // looks like merge
        if (argv[1][0] != '-') { // not if first argument is a switch
            MG_create_all_awars(aw_root, aw_default, argv[1], argv[2]);
            nt_intro_start_merge(0, aw_root);
            aw_root->main_loop();
        }
    }

    bool  abort            = false;
    bool  start_db_browser = true;
    char *browser_startdir = strdup(".");

    if (argc>=2) {
        start_db_browser = false;

        if (strcmp(argv[1], "--help")==0 ||
            strcmp(argv[1], "-help")==0 ||
            strcmp(argv[1], "-h")==0)
        {
            fprintf(stderr,
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
                    "-export            => connect to existing ARB server and export database to noname.arb\n"
                    "-import file       => import 'file' into new database\n"
                    "w/o arguments      => start database browser\n"
                    "\n"
                    );

            exit(1);
        }

        if (strcmp(argv[1], "-export")==0) {
            MG_create_all_awars(aw_root, aw_default, ":", "noname.arb");
            GLOBAL_gb_merge = GBT_open(":", "rw", 0);
            if (!GLOBAL_gb_merge) {
                aw_popup_ok(GB_await_error());
                exit(0);
            }
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

        bool run_importer = false;
        if (strcmp(argv[1], "-import") == 0) {
            argv++;
            run_importer = true;
        }

        db_server              = argv[1];
        GB_ERROR load_file_err = 0;
        if (!run_importer) load_file_err = GBT_check_arb_file(db_server);

        if (load_file_err) {
            int   answer    = -1;
            char *full_path = AWT_unfold_path(db_server);

            printf("load_file_err='%s'\n", load_file_err);

            if (AWT_is_dir(full_path)) answer = 2; // autoselect browser

            if (answer == -1) {
                if (!AWT_is_file(full_path)) {
                    const char *msg = GBS_global_string("'%s' is neither a known option nor a legal file- or directory-name.\n(Error: %s)",
                                                        full_path, load_file_err);
                    answer          = aw_question(msg, "Browser,Exit");

                    switch (answer) { // map answer to codes used by aw_message below
                        case 0: answer = 2; break; // Browse
                        case 1: answer = 3; break; // Exit
                        default: nt_assert(0);
                    }
                }
                else {
                    const char *msg = GBS_global_string("Your file is not an original arb file\n(%s)", load_file_err);
                    answer          = aw_question(msg, "Continue (dangerous),Start Converter,Browser,Exit");
                }
            }

            switch (answer) {
                case 0: {        // Continue
                    break;
                }
                case 1: {        // Start converter
                    run_importer =  true;
                    break;
                }
                case 2: {        // Browse
                    char *dir = nulldup(full_path);
                    while (dir && !AWT_is_dir(dir)) freeset(dir, AWT_extract_directory(dir));

                    if (dir) {
                        nt_assert(AWT_is_dir(dir));
                        reassign(browser_startdir, dir);
                        start_db_browser = true;
                    }
                    free(dir);
                    break;
                }
                case 3: {        // Exit
                    abort = true;
                    break;
                }
                default: {
                    break;
                }
            }
            free(full_path);
        }

        if (run_importer) {
            aw_root->awar_int(AWAR_READ_GENOM_DB, IMP_PLAIN_SEQUENCE);
            GLOBAL_gb_main = open_AWTC_import_window(aw_root, db_server, true, 0, (AW_RCB)main3, 0, 0);
            aw_root->main_loop();
        }
    }


    if (start_db_browser) {
        aw_root->awar(AWAR_DB"directory")->write_string(browser_startdir);
        char *latest = GB_find_latest_file(browser_startdir, "/\\.(arb|a[0-9]{2})$/");
        if (latest) {
            int l = strlen(latest);
            latest[l-1] = 'b';
            latest[l-2] = 'r';
            latest[l-3] = 'a';
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

    if (abort) {
        printf("Aborting.\n");
    }
    else {
        aw_root->awar(AWAR_DB_PATH)->write_string(db_server);
        if (main_load_and_startup_main_window(aw_root)) return -1;
        aw_root->main_loop();
    }

    return 0;
}
