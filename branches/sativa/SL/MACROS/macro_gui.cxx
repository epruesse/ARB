// ============================================================= //
//                                                               //
//   File      : macro_gui.cxx                                   //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in May 2005     //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "macros.hxx"
#include "macros_local.hxx"
#include "trackers.hxx"

#include <arbdb.h>
#include <arb_file.h>

#include <aw_window.hxx>
#include <aw_edit.hxx>
#include <aw_file.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <arbdbt.h>
#include <ad_cb.h>

#define ma_assert(bed) arb_assert(bed)

#define AWAR_MACRO_RECORDING_MACRO_TEXT AWAR_MACRO_BASE"/button_label"
#define AWAR_MACRO_RECORDING_EXPAND     AWAR_MACRO_BASE"/expand"
#define AWAR_MACRO_RECORDING_RUNB4      AWAR_MACRO_BASE"/runb4"

static void delete_macro_cb(AW_window *aww) {
    AW_root *awr       = aww->get_root();
    char    *macroName = AW_get_selected_fullname(awr, AWAR_MACRO_BASE);

    if (GB_unlink(macroName)<0) aw_message(GB_await_error());
    else AW_refresh_fileselection(awr, AWAR_MACRO_BASE);

    free(macroName);
}

static void macro_execution_finished(AW_root *awr, char *macroName) {
#if defined(DEBUG)
    fprintf(stderr, "macro_execution_finished(%s)\n", macroName);
#endif

    AW_set_selected_fullname(awr, AWAR_MACRO_BASE, macroName); // reset selected macro (needed if macro calls other macro(s)) 
    
    free(macroName);
}

static void exec_macro_cb(AW_window *aww, bool loop_marked) {
    AW_root  *awr       = aww->get_root();
    char     *macroName = AW_get_selected_fullname(awr, AWAR_MACRO_BASE); // @@@ instead use function returning plain name w/o dir
    GB_ERROR  error     = getMacroRecorder(awr)->execute(macroName, loop_marked, makeRootCallback(macro_execution_finished, macroName));
    if (error) {
        aw_message(error);
        free(macroName); // only free in error-case (see macro_execution_finished)
    }
}

inline void update_macro_record_button(AW_root *awr) {
    awr->awar(AWAR_MACRO_RECORDING_MACRO_TEXT)->write_string(awr->is_tracking() ? "STOP" : "RECORD");
}

static void start_macro_cb(AW_window *aww) {
    AW_root  *awr   = aww->get_root();
    GB_ERROR  error = NULL;

    if (awr->is_tracking()) {
        error = getMacroRecorder(awr)->stop_recording();
    }
    else {
        bool expand = awr->awar(AWAR_MACRO_RECORDING_EXPAND)->read_int();
        bool runb4  = expand && awr->awar(AWAR_MACRO_RECORDING_RUNB4)->read_int();

        char *macroName = AW_get_selected_fullname(awr, AWAR_MACRO_BASE);
        if (GB_is_directory(macroName)) {
            error = "Please specify name of macro to record";
        }
        else {
            if (runb4) exec_macro_cb(aww, false);

            char *sac = GBS_global_string_copy("%s/%s", aww->window_defaults_name, MACRO_RECORD_ID);
            error = getMacroRecorder(awr)->start_recording(macroName, sac, expand);
            free(sac);
        }
        free(macroName);
    }

    AW_refresh_fileselection(awr, AWAR_MACRO_BASE);
    update_macro_record_button(awr);
    if (error) aw_message(error);
}

static void edit_macro_cb(AW_window *aww) {
    char *path = AW_get_selected_fullname(aww->get_root(), AWAR_MACRO_BASE);
    AW_edit(path);
    free(path);
}

static void macro_recording_changed_cb() {
    update_macro_record_button(AW_root::SINGLETON);
}

static void create_macro_variables(AW_root *aw_root) {
    AW_create_fileselection_awars(aw_root, AWAR_MACRO_BASE, ".", ".amc", "");
    aw_root->awar_string(AWAR_MACRO_RECORDING_MACRO_TEXT, "RECORD");
    aw_root->awar_int(AWAR_MACRO_RECORDING_EXPAND, 0);
    aw_root->awar_int(AWAR_MACRO_RECORDING_RUNB4, 0);

    {
        UserActionTracker *tracker       = aw_root->get_tracker();
        MacroRecorder     *macroRecorder = dynamic_cast<MacroRecorder*>(tracker);

        GBDATA   *gb_main = macroRecorder->get_gbmain();
        GB_ERROR  error   = NULL;

        GB_transaction ta(gb_main);

        GBDATA *gb_recording     = GB_searchOrCreate_int(gb_main, MACRO_TRIGGER_RECORDING, 0);
        if (!gb_recording) error = GB_await_error();
        else    error            = GB_add_callback(gb_recording, GB_CB_CHANGED, makeDatabaseCallback(macro_recording_changed_cb));

        if (error) aw_message(GBS_global_string("Failed to bind macro_recording_changed_cb (Reason: %s)", error));
    }

    update_macro_record_button(aw_root);
}

static void popup_macro_window(AW_window *aww) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        AW_root *aw_root = aww->get_root();

        aws = new AW_window_simple;
        aws->init(aw_root, MACRO_WINDOW_ID, "MACROS");
        aws->load_xfig("macro_select.fig");

        create_macro_variables(aw_root);
        
        aws->at("close"); aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("help"); aws->callback(makeHelpCallback("macro.hlp"));
        aws->create_button("HELP", "HELP");

        aws->at("record"); aws->callback(start_macro_cb);
        aws->create_button(MACRO_RECORD_ID, AWAR_MACRO_RECORDING_MACRO_TEXT);

        aws->at("expand"); aws->create_toggle(AWAR_MACRO_RECORDING_EXPAND);
        aws->at("runb4");  aws->create_toggle(AWAR_MACRO_RECORDING_RUNB4);

        aws->at("edit");   aws->callback(edit_macro_cb);   aws->create_button("EDIT",   "EDIT");
        aws->at("delete"); aws->callback(delete_macro_cb); aws->create_button("DELETE", "DELETE");

        aws->at("exec");
        aws->callback(makeWindowCallback(exec_macro_cb, false));
        aws->create_button(MACRO_PLAYBACK_ID, "EXECUTE");

        aws->at("execWith");
        aws->callback(makeWindowCallback(exec_macro_cb, true));
        aws->create_autosize_button(MACRO_PLAYBACK_MARKED_ID, "Execute with each marked species");

        AW_create_fileselection(aws, AWAR_MACRO_BASE, "", "ARBMACROHOME^ARBMACRO", ANY_DIR, false);
    }
    aws->activate();
}

void insert_macro_menu_entry(AW_window *awm, bool prepend_separator) {
    if (getMacroRecorder(awm->get_root())) {
        if (prepend_separator) awm->sep______________();
        awm->insert_menu_topic("macros", "Macros ", "M", "macro.hlp", AWM_ALL, popup_macro_window);
    }
}

static void dont_announce_done(AW_root*) {}

void execute_macro(AW_root *root, const char *macroname) {
    // used to execute macro passed via CLI
    GB_ERROR error = 0;
    {
        // @@@ allow macro playback from client? (using server via AWAR)
        MacroRecorder *recorder = getMacroRecorder(root);
        if (!recorder) error    = "macro playback only available in server";
        else           error    = recorder->execute(macroname, false, makeRootCallback(dont_announce_done));
    }

    if (error) {
        aw_message(GBS_global_string("Can't execute macro '%s' (Reason: %s)", macroname, error));
    }
}
