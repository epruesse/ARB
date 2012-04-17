// ==================================================================== //
//                                                                      //
//   File      : AWT_macro.cxx                                          //
//   Purpose   :                                                        //
//                                                                      //
//                                                                      //
// Coded by Ralf Westram (coder@reallysoft.de) in May 2005              //
// Copyright Department of Microbiology (Technical University Munich)   //
//                                                                      //
// Visit our web site at: http://www.arb-home.de/                       //
//                                                                      //
// ==================================================================== //

#include "awt_macro.hxx"

#include <arbdb.h>
#include <arb_file.h>

#include <aw_window.hxx>
#include <aw_edit.hxx>
#include <aw_file.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>

// ---------------
//      MACROS

#define AWAR_MACRO_BASE                 "tmp/macro"
#define AWAR_MACRO_RECORDING_MACRO_TEXT AWAR_MACRO_BASE"/button_label"
#define AWAR_MACRO_RECORD_ID            "macro_record"

static void awt_delete_macro_cb(AW_window *aww) {
    AW_root *awr       = aww->get_root();
    char    *macroName = AW_get_selected_fullname(awr, AWAR_MACRO_BASE);

    if (GB_unlink(macroName)<0) aw_message(GB_await_error());
    else AW_refresh_fileselection(awr, AWAR_MACRO_BASE);

    free(macroName);
}

static void macro_execution_finished(AW_root *awr, AW_CL cl_macroName) {
    char *macroName = (char*)cl_macroName;

#if defined(DEBUG)
    fprintf(stderr, "macro_execution_finished(%s)\n", macroName);
#endif

    AW_set_selected_fullname(awr, AWAR_MACRO_BASE, macroName); // reset selected macro (needed if macro calls other macro(s)) 
    
    free(macroName);
}

static void awt_exec_macro_cb(AW_window *aww, AW_CL cl_gb_main) {
    GBDATA  *gb_main   = (GBDATA*)cl_gb_main;
    AW_root *awr       = aww->get_root();
    char    *macroName = AW_get_selected_fullname(awr, AWAR_MACRO_BASE);


    GB_ERROR error = awr->execute_macro(gb_main, macroName, macro_execution_finished, (AW_CL)macroName);

    if (error) {
        aw_message(error);
        free(macroName); // only free in error-case (see macro_execution_finished)
    }
}

static void awt_start_macro_cb(AW_window *aww, const char *application_name_for_macros) {
    static int toggle = 0;

    AW_root  *awr = aww->get_root();
    GB_ERROR  error;

    if (!toggle) {
        char *sac       = GBS_global_string_copy("%s/%s", aww->window_defaults_name, AWAR_MACRO_RECORD_ID);
        char *macroName = AW_get_selected_fullname(awr, AWAR_MACRO_BASE);
        error = awr->start_macro_recording(macroName, application_name_for_macros, sac);
        free(macroName);
        free(sac);

        if (!error) {
            awr->awar(AWAR_MACRO_RECORDING_MACRO_TEXT)->write_string("STOP");
            toggle = 1;
        }
    }
    else {
        error = awr->stop_macro_recording();
        AW_refresh_fileselection(awr, AWAR_MACRO_BASE);
        awr->awar(AWAR_MACRO_RECORDING_MACRO_TEXT)->write_string("RECORD");
        toggle = 0;
    }
    if (error) aw_message(error);
}

static void awt_edit_macro_cb(AW_window *aww) {
    char *path = AW_get_selected_fullname(aww->get_root(), AWAR_MACRO_BASE);
    AW_edit(path);
    free(path);
}


void awt_popup_macro_window(AW_window *aww, const char *application_id, GBDATA *gb_main) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        AW_root *aw_root = aww->get_root();

        aws = new AW_window_simple;
        aws->init(aw_root, "MACROS", "MACROS");
        aws->load_xfig("macro_select.fig");

        AW_create_fileselection_awars(aw_root, AWAR_MACRO_BASE, ".", ".amc", "");

        aw_root->awar_string(AWAR_MACRO_RECORDING_MACRO_TEXT, "RECORD");

        aws->at("close"); aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("help"); aws->callback(AW_POPUP_HELP, (AW_CL)"macro.hlp");
        aws->create_button("HELP", "HELP");

        aws->at("start"); aws->callback((AW_CB1)awt_start_macro_cb, (AW_CL)application_id);
        aws->create_button(AWAR_MACRO_RECORD_ID, AWAR_MACRO_RECORDING_MACRO_TEXT);

        aws->at("delete"); aws->callback(awt_delete_macro_cb);
        aws->create_button("DELETE", "DELETE");

        aws->at("edit"); aws->callback(awt_edit_macro_cb);
        aws->create_button("EDIT", "EDIT");

        aws->at("exec"); aws->callback(awt_exec_macro_cb, (AW_CL)gb_main);
        aws->create_button("EXECUTE", "EXECUTE");

        AW_create_fileselection(aws, AWAR_MACRO_BASE, "", "ARBMACROHOME^ARBMACRO");
    }
    aws->activate();
}

inline char *find_macro_in(const char *dir, const char *macroname) {
    char *full = GBS_global_string_copy("%s/%s.amc", dir, macroname);
    if (!GB_is_readablefile(full)) freenull(full);
    return full;
}

void awt_execute_macro(GBDATA *gb_main, AW_root *root, const char *macroname) {
    char *fullname          = find_macro_in(GB_getenvARBMACROHOME(), macroname);
    if (!fullname) fullname = find_macro_in(GB_getenvARBMACRO(), macroname);

    GB_ERROR error       = 0;
    if (!fullname) error = "file not found";
    else     error       = root->execute_macro(gb_main, fullname, NULL, NULL);

    if (error) {
        aw_message(GBS_global_string("Can't execute macro '%s' (Reason: %s)", macroname, error));
    }

    free(fullname);
}
