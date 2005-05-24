#include <stdio.h>
#include <stdlib.h>
#include <arbdb.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <AW_rename.hxx>
#include "merge.hxx"

AW_window *MG_merge_names_cb(AW_root *awr){
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    aws = new AW_window_simple;
    aws->init( awr, "MERGE_AUTORENAME_SPECIES", "SYNCRONIZE NAMES");
    aws->load_xfig("merge/names.fig");

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"mg_names.hlp");
    aws->create_button("HELP","HELP","H");

    aws->button_length(24);

    aws->at("name1");
    aws->callback((AW_CB1)awt_rename_cb, (AW_CL)gb_merge);
    aws->create_button("RENAME_DATABASE_1","RENAME DATABASE 1");

    aws->at("name2");
    aws->callback((AW_CB1)awt_rename_cb, (AW_CL)gb_dest);
    aws->create_button("RENAME_DATABASE_2","RENAME DATABASE 2");

    aws->button_length(0);
    aws->shadow_width(1);
    aws->at("icon");
    aws->callback(AW_POPUP_HELP,(AW_CL)"mg_names.hlp");
    aws->create_button("HELP_MERGE", "#merge/icon.bitmap");

    return (AW_window *)aws;
}
