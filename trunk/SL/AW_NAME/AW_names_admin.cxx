#include <stdio.h>
#include <stdlib.h>
#include <arbdb.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <awt.hxx>
#include "AW_rename.hxx"

void awtc_delete_names_file(AW_window *aws){
    AWUSE(aws);
    char *path = GBS_eval_env(NAMES_FILE_LOCATION);
    char *newpath = GBS_string_eval(path,"*=*%",0);
    GB_ERROR error = GB_rename_file(path,newpath);
    if (error) aw_message(error);
    delete path;
    delete newpath;
}

void awtc_edit_names_file(AW_window *aws){
    char *path = GBS_eval_env(NAMES_FILE_LOCATION);
    awt_edit(aws->get_root(),path,1000,700);
    delete path;
}

void awtc_remove_arb_acc(AW_window *aws){
    AWUSE(aws);
    char *path = GBS_eval_env(NAMES_FILE_LOCATION);
    char *newpath = GBS_string_eval(path,"*=*%",0);
    char command[1024];
    sprintf(command,"cp %s %s;arb_replace -l '*ACC {}*=:*ACC {TUM_*=:*ACC {ARB_*=' %s",
            path, newpath, path);
    delete path;
    delete newpath;
}



AW_window *create_awtc_names_admin_window( AW_root *root)  {
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "NAME_SERVER_ADMIN", "NAME_SERVER ADMIN");

    aws->load_xfig("awtc/names_admin.fig");


    aws->callback( AW_POPUP_HELP,(AW_CL)"namesadmin.hlp");
    aws->at("help");
    aws->create_button("HELP", "HELP","H");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE","C");

    aws->button_length(30);

    aws->at("delete");
    aws->callback(awtc_delete_names_file);
    aws->create_button("DELETE_OLD_NAMES_FILE", "Delete old names file");

    aws->at("edit");
    aws->callback(awtc_edit_names_file);
    aws->create_button("EDIT_NAMES_FILE", "Edit names file");

    aws->at("remove_arb");
    aws->callback(awtc_remove_arb_acc);
    aws->create_button("REMOVE_SUPERFLUOUS_ENTRIES_IN_NAMES_FILE",
                       "Remove all entries with an\n'ARB_XXXX' accession number\nfrom names file");

    return aws;
}
