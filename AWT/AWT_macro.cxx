// ==================================================================== //
//                                                                      //
//   File      : AWT_macro.cxx                                          //
//   Purpose   :                                                        //
//   Time-stamp: <Fri Apr/27/2007 12:26 MET Coder@ReallySoft.de>        //
//                                                                      //
//                                                                      //
// Coded by Ralf Westram (coder@reallysoft.de) in May 2005              //
// Copyright Department of Microbiology (Technical University Munich)   //
//                                                                      //
// Visit our web site at: http://www.arb-home.de/                       //
//                                                                      //
// ==================================================================== //

#include <stdlib.h>
#include <stdio.h>

#include <arbdb.h>

#include <aw_window.hxx>
#include <aw_global.hxx>

#include "awt.hxx"
#include "awt_macro.hxx"

// ---------------
//      MACROS
// ---------------

#define AWAR_MACRO_BASE                 "tmp/macro"
#define AWAR_MACRO_RECORDING_MACRO_TEXT AWAR_MACRO_BASE"/button_label"

static void awt_delete_macro_cb(AW_window *aww){
    AW_root *awr   = aww->get_root();
    char    *mn    = awt_get_selected_fullname(awr, AWAR_MACRO_BASE);
    int      error = GB_unlink(mn);

    if (error) aw_message(GB_export_IO_error("deleting", mn));
    awt_refresh_selection_box(awr, AWAR_MACRO_BASE);
    free(mn);
}



static void awt_exec_macro_cb(AW_window *aww){
    AW_root  *awr   = aww->get_root();
    char     *mn    = awt_get_selected_fullname(awr, AWAR_MACRO_BASE);
    GB_ERROR  error = awr->execute_macro(mn);

    if (error) aw_message(error);
    free(mn);
}

static void awt_start_macro_cb(AW_window *aww,const char *application_name_for_macros){
    static int toggle = 0;

    AW_root  *awr = aww->get_root();
    //     char     *mn  = awr->awar(AWAR_MACRO_FILENAME)->read_string();
    GB_ERROR  error;

    if (!toggle){
        char *sac = GBS_global_string_copy("%s/%s",aww->window_defaults_name,AWAR_MACRO_RECORDING_MACRO_TEXT);
        char *mn  = awt_get_selected_fullname(awr, AWAR_MACRO_BASE);
        error     = awr->start_macro_recording(mn,application_name_for_macros,sac);
        free(mn);
        free(sac);
        if (!error){
            awr->awar(AWAR_MACRO_RECORDING_MACRO_TEXT)->write_string("STOP");
            toggle = 1;
        }
    }
    else {
        error = awr->stop_macro_recording();
        awt_refresh_selection_box(awr, AWAR_MACRO_BASE);
        awr->awar(AWAR_MACRO_RECORDING_MACRO_TEXT)->write_string("RECORD");
        toggle = 0;
    }
    if (error) aw_message(error);
}

// static void awt_stop_macro_cb(AW_window *aww){
//     AW_root  *awr   = aww->get_root();
//     GB_ERROR  error = awr->stop_macro_recording();

//     awt_refresh_selection_box(awr, AWAR_MACRO_BASE);
//     if (error) aw_message(error);
// }

static void awt_edit_macro_cb(AW_window *aww){
    //     char *mn = aww->get_root()->awar(AWAR_MACRO_FILENAME)->read_string();
    //     char *path = 0;
    //     if (mn[0] == '/'){
    //         path = strdup(mn);
    //     }else{
    //         path = GBS_global_string_copy("%s/%s",GB_getenvARBMACROHOME(),mn);
    //     }
    char *path = awt_get_selected_fullname(aww->get_root(), AWAR_MACRO_BASE);
    AWT_edit(path);
    free(path);
    //     delete mn;
}


AW_window *awt_open_macro_window(AW_root *aw_root,const char *application_id){
    static AW_window_simple *aws = 0;
    if (aws) return aws;
    aws = new AW_window_simple;
    aws->init( aw_root, "MACROS", "MACROS");
    aws->load_xfig("macro_select.fig");

    aw_create_selection_box_awars(aw_root, AWAR_MACRO_BASE, ".", ".amc", "");

    aw_root->awar_string(AWAR_MACRO_RECORDING_MACRO_TEXT,"RECORD");

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE","C");

    aws->at("help");aws->callback(AW_POPUP_HELP,(AW_CL)"macro.hlp");
    aws->create_button("HELP", "HELP");

    aws->at("start");aws->callback((AW_CB1)awt_start_macro_cb,(AW_CL)application_id);
    aws->create_button(0, AWAR_MACRO_RECORDING_MACRO_TEXT);

    aws->at("delete");aws->callback(awt_delete_macro_cb);
    aws->create_button("DELETE", "DELETE");

    aws->at("edit");aws->callback(awt_edit_macro_cb);
    aws->create_button("EDIT", "EDIT");

    aws->at("exec");aws->callback(awt_exec_macro_cb);
    aws->create_button("EXECUTE", "EXECUTE");

    awt_create_selection_box((AW_window *)aws,AWAR_MACRO_BASE,"","ARBMACROHOME^ARBMACRO");

    return (AW_window *)aws;
}




