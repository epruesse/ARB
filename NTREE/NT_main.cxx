#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <adGene.h>

#include <servercntrl.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <aw_question.hxx>
#include <aw_global.hxx>
#include <awt_canvas.hxx>
#include <awt_advice.hxx>

#include <awt_tree.hxx>
#include <awt_dtree.hxx>
#include <awt.hxx>
#include <awti_import.hxx>

#include <mg_merge.hxx>
#include <seer.hxx>

#include "nt_concatenate.hxx"
#include "ntree.hxx"
#include "nt_cb.hxx"
#include <arb_version.h>

#include <set>
#include <string>

using namespace std;

AW_HEADER_MAIN

#define nt_assert(bed) arb_assert(bed)

#define NT_SERVE_DB_TIMER 50
#define NT_CHECK_DB_TIMER 200

GBDATA *gb_main;
NT_global nt = { 0, 0, 0, AW_FALSE };

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
    GBDATA               *gb_presets = GB_find(gb_main,"presets", 0, down_level);

    question.add_help("prompt/format_alignments.hlp");

    for (GBDATA *gb_ali = GB_find(gb_presets,"alignment",0,down_level);
         gb_ali && !err;
         gb_ali = GB_find(gb_ali,"alignment",0,this_level|search_next) )
    {
        GBDATA *gb_aligned = GB_search(gb_ali, "aligned", GB_INT);

        if (GB_read_int(gb_aligned) == 0) { // sequences in alignment are not formatted
            int format_action = 0;
            // 0 -> ask user
            // 1 -> format automatically w/o asking
            // 2 -> skip automatically w/o asking

            GBDATA     *gb_ali_name  = GB_find(gb_ali,"alignment_name",0,down_level);
            const char *ali_name     = GB_read_char_pntr(gb_ali_name);
            bool        is_ali_genom = strcmp(ali_name, GENOM_ALIGNMENT) == 0;

            GBDATA *gb_auto_format = GB_find(gb_ali, "auto_format", 0, down_level);
            if (gb_auto_format) {
                format_action = GB_read_int(gb_auto_format);

                if (is_ali_genom) {
                    if (format_action != 2) {
                        format_action = 2; // always skip ali_genom
                        GB_write_int(gb_auto_format, 2);
                    }
                }
            }
            else if (is_ali_genom) {
                gb_auto_format = GB_search(gb_ali, "auto_format", GB_INT);
                GB_write_int(gb_auto_format, 2); // always skip
                format_action          = 2;
            }

            bool perform_format = false;

            switch (format_action) {
                case 1: perform_format = true; break;
                case 2: perform_format = false; break;
                default: {
                    char *qtext  = GBS_global_string_copy("Alignment '%s' is not formatted. Format?", ali_name);
                    int   answer = question.get_answer(qtext, "Format,Skip,Always format,Always skip", "all", false);

                    switch (answer) {
                        case 2: {
                            GBDATA *gb_auto_format = GB_search(gb_ali, "auto_format", GB_INT);
                            GB_write_int(gb_auto_format, 1);
                            // fall-through
                        }
                        case 0:
                            perform_format = true;
                            break;
                        case 3: {
                            GBDATA *gb_auto_format = GB_search(gb_ali, "auto_format", GB_INT);
                            GB_write_int(gb_auto_format, 2);
                            break;
                        }
                    }

                    free(qtext);
                    break;
                }
            }

            if (perform_format) {
                aw_status(GBS_global_string("Formatting '%s'", ali_name));
                GB_push_my_security(gb_main);
                err = GBT_format_alignment(gb_main, ali_name);
                GB_pop_my_security(gb_main);
            }
        }
    }

    GB_pop_my_security(gb_main);

    if (err) GB_abort_transaction(gb_main);
    else GB_commit_transaction(gb_main);

    return err;
}

static GB_ERROR NT_fix_gene_data(GBDATA *gb_main) {
    GB_transaction ta(gb_main);
    int            deleted_gene_datas   = 0;
    int            generated_gene_datas = 0;
    GB_ERROR       error                = 0;

    for (GBDATA *gb_species = GBT_first_species(gb_main);
         gb_species && !error;
         gb_species = GBT_next_species(gb_species))
    {
        bool    is_organism  = (GB_find(gb_species, GENOM_ALIGNMENT, 0, down_level) != 0); // same test as GEN_is_organism, but w/o genome-db-assertion
        GBDATA *gb_gene_data = GEN_find_gene_data(gb_species);

        if (is_organism && !gb_gene_data) {
            gb_gene_data = GEN_findOrCreate_gene_data(gb_species);
            generated_gene_datas++;
        }
        else if (!is_organism && gb_gene_data) {
            GBDATA *gb_child = GB_find(gb_gene_data, 0, 0, down_level);
            if (!gb_child) {
                error = GB_delete(gb_gene_data);
                if (!error) deleted_gene_datas++;
            }
            else {
                const char *name    = "<unknown species>";
                GBDATA     *gb_name = GB_find(gb_species, "name", 0, down_level);
                if (gb_name) name = GB_read_char_pntr(gb_name);

                error = GBS_global_string("Non-empty 'gene_data' found for species '%s',\n"
                                          "which has no alignment '" GENOM_ALIGNMENT "',\n"
                                          "i.e. which is not regarded as full-genome organism.\n"
                                          "This causes problems - please fix!", name);
            }
        }
    }

    if (error) {
        ta.abort();
    }
    else {
        if (deleted_gene_datas>0) {
            aw_message(GBS_global_string("Deleted %i useless empty 'gene_data' entries.", deleted_gene_datas));
        }
        if (generated_gene_datas>0) {
            aw_message(GBS_global_string("Re-created %i missing 'gene_data' entries.\nThese organisms have no genes!", generated_gene_datas));
        }
    }
    return error;
}

// CheckedConsistencies provides an easy way to automatically correct flues in the database
// by calling a check routine exactly one.
// For an example see nt_check_database_consistency()

class CheckedConsistencies {
    GBDATA      *gb_main;
    set<string>  consistencies;

public:
    CheckedConsistencies(GBDATA *gb_main_) : gb_main(gb_main_) {
        GB_transaction ta(gb_main);
        GBDATA *gb_checks = GB_search(gb_main, "checks", GB_CREATE_CONTAINER);

        for (GBDATA *gb_check = GB_find(gb_checks, "check", 0, down_level);
             gb_check;
             gb_check = GB_find(gb_check, "check", 0, this_level|search_next))
        {
            consistencies.insert(GB_read_char_pntr(gb_check));
        }
    }

    bool was_performed(const string& check_name) const {
        return consistencies.find(check_name) != consistencies.end();
    }

    GB_ERROR register_as_performed(const string& check_name) {
        GB_ERROR error = 0;
        if (was_performed(check_name)) {
            printf("check '%s' already has been registered before. Duplicated check name?\n", check_name.c_str());
        }
        else {
            GB_transaction ta(gb_main);
            GBDATA *gb_checks = GB_search(gb_main, "checks", GB_CREATE_CONTAINER);
            GBDATA *gb_check  = GB_create(gb_checks, "check", GB_STRING);

            if (!gb_check) {
                error = GB_get_error();
            }
            else {
                error = GB_write_string(gb_check, check_name.c_str());
            }

            if (!error) consistencies.insert(check_name);
        }
        return error;
    }
};


// called once on ARB_NTREE startup
//
static GB_ERROR nt_check_database_consistency() {
    aw_openstatus("Checking database...");

    CheckedConsistencies check(gb_main);
    GB_ERROR             err = NT_format_all_alignments(gb_main);

    if (!err && !check.was_performed("fix gene_data")) {
        err = NT_fix_gene_data(gb_main);
        if (!err) check.register_as_performed("fix gene_data");
    }

    aw_closestatus();
    return err;
}


void serve_db_interrupt(AW_root *awr){
    GB_BOOL succes = GBCMS_accept_calls(gb_main,GB_FALSE);
    while (succes){
        awr->check_for_remote_command((AW_default)gb_main,AWAR_NT_REMOTE_BASE);
        succes = GBCMS_accept_calls(gb_main,GB_TRUE);
    }

    awr->add_timed_callback(NT_SERVE_DB_TIMER,(AW_RCB)serve_db_interrupt,0,0);
}

void check_db_interrupt(AW_root *awr){
    awr->check_for_remote_command((AW_default)gb_main,AWAR_NT_REMOTE_BASE);
    awr->add_timed_callback(NT_CHECK_DB_TIMER,(AW_RCB)check_db_interrupt,0,0);
}

GB_ERROR create_nt_window(AW_root *aw_root){
    AW_window *aww;
    GB_ERROR error = GB_request_undo_type(gb_main, GB_UNDO_NONE);
    if (error) aw_message(error);
    create_all_awars(aw_root,AW_ROOT_DEFAULT);
    if (GB_NOVICE){
        aw_root->set_sensitive(AWM_BASIC);
    }
    aww = create_nt_main_window(aw_root,0);
    aww->show();
    error = GB_request_undo_type(gb_main, GB_UNDO_UNDO);
    if (error) aw_message(error);
    return error;
}

// after intro
void nt_main_startup_main_window(AW_root *aw_root){
    create_nt_window(aw_root);

    if (GB_read_clients(gb_main)==0) { // i am the server
        GB_ERROR error = GBCMS_open(":",0,gb_main);
        if (error) {
            aw_message(
                       "THIS PROGRAM HAS PROBLEMS TO OPEN INTERCLIENT COMMUNICATION !!!\n"
                       "(MAYBE THERE IS ALREADY ANOTHER SERVER RUNNING)\n\n"
                       "You cannot use any EDITOR or other external SOFTWARE with this dataset!\n\n"
                       "Advice: Close ARB again, open a console, type 'arb_clean' and restart arb.\n"
                       "Caution: Any unsaved data in an eventually running ARB will be lost!\n");
        }else{
            aw_root->add_timed_callback(NT_SERVE_DB_TIMER,(AW_RCB)serve_db_interrupt,0,0);
            error = nt_check_database_consistency();
            if (error) aw_message(error);
        }
    }else{
        aw_root->add_timed_callback(NT_CHECK_DB_TIMER,(AW_RCB)check_db_interrupt,0,0);
    }
}

int main_load_and_startup_main_window(AW_root *aw_root) // returns 0 when successfull
{

    char *db_server = aw_root->awar(AWAR_DB_PATH)->read_string();
    gb_main = GBT_open(db_server,"rw","$(ARBHOME)/lib/pts/*");

    if (!gb_main) {
        aw_message(GB_get_error(),"OK");
        return -1;
    }

    AWT_announce_db_to_browser(gb_main, GBS_global_string("ARB database (%s)", db_server));

    aw_root->awar(AWAR_DB_PATH)->write_string(db_server);
    free(db_server);
    nt_main_startup_main_window(aw_root);

    return 0;
}

void nt_delete_database(AW_window *aww){
    char *db_server = aww->get_root()->awar(AWAR_DB_PATH)->read_string();
    if (strlen(db_server)){
        if (aw_message(GBS_global_string("Are you sure to delete database %s\nNote: there is no way to undelete it afterwards",db_server),"Delete Database,Cancel") == 0){
            GB_ERROR error = 0;
            error = GB_delete_database(db_server);
            if (error){
                aw_message(error);
            }else{
                aww->get_root()->awar(AWAR_DB"filter")->touch();
            }
        }
    }else{
        aw_message("No database selected");
    }
    delete db_server;
}

// after import !!!!!
void main3(AW_root *aw_root)
{

    nt.awr = aw_root;
    create_nt_window(aw_root);

    if (GB_read_clients(gb_main)==0) {
        GB_ERROR error = GBCMS_open(":",0,gb_main);
        if (error) {
            aw_message("THIS PROGRAM IS NOT THE ONLY DB SERVER !!!\nDONT USE ANY ARB PROGRAM !!!!");
        }else{
            aw_root->add_timed_callback(NT_SERVE_DB_TIMER, (AW_RCB)serve_db_interrupt,0,0);
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

void nt_intro_start_merge(AW_window *aws,AW_root *awr){
    if (aws) awr = aws->get_root();
    create_MG_main_window(awr);
    if (aws) aws->hide();
}

void nt_intro_start_import(AW_window *aws)
{
    aws->hide();
    aws->get_root()->awar_string( AWAR_DB_PATH )->write_string( "noname.arb");
    aws->get_root()->awar_int(AWAR_READ_GENOM_DB, IMP_PLAIN_SEQUENCE); // Default toggle  in window  "Create&import" is Non-Genom
    gb_main = open_AWTC_import_window(aws->get_root(), "", true, 0, (AW_RCB)main3, 0, 0);
}

AW_window *nt_create_intro_window(AW_root *awr)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( awr, "ARB_INTRO", "ARB INTRO");
    aws->load_xfig("arb_intro.fig");

    aws->callback( (AW_CB0)exit);
    aws->at("close");
    aws->create_button("CANCEL","CANCEL","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"arb_intro.hlp");
    aws->create_button("HELP","HELP","H");

    awt_create_selection_box(aws,"tmp/nt/arbdb");

    aws->button_length(0);

    aws->at("logo");
    aws->create_button(0,"#logo.bitmap");

    aws->at("version");
    aws->create_button(0, GBS_global_string("Version " ARB_VERSION), 0); // version

    aws->at("copyright");
    aws->create_button(0, GBS_global_string("(C) 1993-" DATE_YEAR), 0);

    //  aws->button_length(25);

    aws->at("old");
    aws->callback(nt_intro_start_old);
    aws->create_autosize_button("OPEN_SELECTED","OPEN SELECTED","O");

    aws->at("del");
    aws->callback(nt_delete_database);
    aws->create_autosize_button("DELETE_SELECTED","DELETE SELECTED");

    aws->at("new_complex");
    aws->callback(nt_intro_start_import);
    aws->create_autosize_button("CREATE_AND_IMPORT","CREATE AND IMPORT","I");

    aws->at("merge");
    aws->callback((AW_CB1)nt_intro_start_merge,0);
    aws->create_autosize_button("MERGE_TWO_DATABASES","MERGE TWO ARB DATABASES","O");

    aws->at("novice");
    aws->create_toggle("NT/GB_NOVICE");

    return (AW_window *)aws;
}

void AD_set_default_root(AW_root *aw_root);

//  ----------------------------------------------------------
//      static void AWAR_DB_PATH_changed_cb(AW_root *awr)
//  ----------------------------------------------------------
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

//  ---------------------------------------
//      int main(int argc, char **argv)
//  ---------------------------------------
int main(int argc, char **argv)
{
    AW_root *aw_root;
    AW_default aw_default;

    const char *db_server =":";

    unsigned long mtime = GB_time_of_file("$(ARBHOME)/lib/message");
    unsigned long rtime = GB_time_of_file("$(HOME)/.arb_prop/msgtime");
    if (mtime > rtime){
        AWT_edit("${ARBHOME}/lib/message");
        system("touch ${HOME}/.arb_prop/msgtime");
    }
    aw_initstatus();
    GB_set_verbose();

    aw_root = new AW_root;
    nt.awr  = aw_root;
    AD_set_default_root(aw_root); // set default for AD_map_viewer (as long as no info-box was opened)
    aw_default = aw_root->open_default(".arb_prop/ntree.arb");
    aw_root->init_variables(aw_default);
    aw_root->init("ARB_NT");

    aw_create_selection_box_awars(aw_root, AWAR_DB, "", ".arb", "noname.arb", aw_default);
    aw_root->awar_string( AWAR_DB"type", "b", aw_default);
    aw_root->awar_int( "NT/GB_NOVICE",      0, aw_default)  ->add_target_var(&GB_NOVICE);

    aw_root->awar_string(AWAR_DB_NAME, "noname.arb", aw_default);
    aw_root->awar(AWAR_DB_PATH)->add_callback(AWAR_DB_PATH_changed_cb);

    init_Advisor(aw_root, AW_ROOT_DEFAULT);

    NT_createConcatenationAwars(aw_root, aw_default); // creating AWARS for concatenation and merge simlar species function

    if (argc==3) {              // looks like merge
        if (argv[1][0] != '-') { // not if first argument is a switch 
            MG_create_all_awars(aw_root,aw_default,argv[1],argv[2]);
            nt_intro_start_merge(0,aw_root);
            aw_root->main_loop();
        }
    }

    bool  abort            = false;
    bool  start_db_browser = true;
    char *browser_startdir = GB_strdup(".");

    if (argc>=2) {
        start_db_browser = false;

        if (strcmp(argv[1], "--help")==0 ||
            strcmp(argv[1], "-help")==0 ||
            strcmp(argv[1], "-h")==0)
        {
            fprintf(stderr,
                    "\n"
                    "arb_ntree version " ARB_VERSION "\n"
                    "(C) 1993-" DATE_YEAR " Lehrstuhl fuer Mikrobiologie - TU Muenchen\n"
                    "http://www.arb-home.de/\n"
#if defined(SHOW_WHERE_BUILD)
                    "(version build by: " BUILD_USER "@" BUILD_HOST ")\n"
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

        if ( strcmp(argv[1],"-export")==0) {
            MG_create_all_awars(aw_root,aw_default,":","noname.arb");
            gb_merge = GBT_open(":","rw",0);
            if (!gb_merge) {
                aw_message(GB_get_error(),"OK");
                exit(0);
            }
            AWT_announce_db_to_browser(gb_dest, "Current database (:)");

            gb_dest = GBT_open("noname.arb","cw",0);
            AWT_announce_db_to_browser(gb_dest, "New database (noname.arb)");

            MG_start_cb2(0,aw_root);
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
                    answer          = aw_message(msg, "Browser,Exit");

                    switch (answer) { // map answer to codes used by aw_message below
                        case 0: answer = 2; break; // Browse
                        case 1: answer = 3; break; // Exit
                        default : nt_assert(0);
                    }
                }
                else {
                    const char *msg = GBS_global_string("Your file is not an original arb file\n(%s)", load_file_err);
                    answer          = aw_message(msg, "Continue (dangerous),Start Converter,Browser,Exit");
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
                    char *dir = GB_strdup(full_path);
                    while (dir && !AWT_is_dir(dir)) {
                        char *updir = AWT_extract_directory(dir);
                        free(dir);
                        dir         = updir;
                    }

                    if (dir) {
                        nt_assert(AWT_is_dir(dir));

                        free(browser_startdir);
                        browser_startdir = dir;
                        dir              = 0;
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
            gb_main = open_AWTC_import_window(aw_root, db_server, true, 0, (AW_RCB)main3, 0, 0);
            aw_root->main_loop();
        }
    }


    if (start_db_browser) {
        aw_root->awar(AWAR_DB"directory")->write_string(browser_startdir);
        char *latest = GB_find_latest_file(browser_startdir, "/\\.a[r0-9][b0-9]$/");
        if (latest){
            int l = strlen(latest);
            latest[l-1] = 'b';
            latest[l-2] = 'r';
            latest[l-3] = 'a';
            aw_root->awar(AWAR_DB_PATH)->write_string(latest);
            free(latest);
        }
        AW_window *iws;
        if (nt.window_creator){
            iws = nt.window_creator(aw_root,0);
        }else{
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
