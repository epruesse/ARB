#include <stdio.h>
#include <stdlib.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <awt.hxx>
#include <aw_awars.hxx>
#include "merge.hxx"

#define AWAR_EX_NAME1 "tmp/merge1/extended_name"
#define AWAR_EX_DEST1 "tmp/merge1/extended_dest"
#define AWAR_EX_NAME2 "tmp/merge2/extended_name"
#define AWAR_EX_DEST2 "tmp/merge2/extended_dest"

void MG_create_extendeds_var(AW_root *aw_root, AW_default aw_def)
{
    aw_root->awar_string( AWAR_EX_NAME1, "" ,   aw_def);
    aw_root->awar_string( AWAR_EX_DEST1, "" ,   aw_def);
    aw_root->awar_string( AWAR_EX_NAME2, "" ,   aw_def);
    aw_root->awar_string( AWAR_EX_DEST2, "" ,   aw_def);
}

void MG_extended_rename_cb(AW_window *aww,GBDATA *gbmain, int ex_nr){
    GB_ERROR error = 0;
    char tsource[256];
    char tdest[256];
    sprintf(tsource,"tmp/merge%i/extended_name",ex_nr);
    sprintf(tdest,"tmp/merge%i/extended_dest",ex_nr);

    char *source = aww->get_root()->awar(tsource)->read_string();
    char *dest = aww->get_root()->awar(tdest)->read_string();
    GB_begin_transaction(gbmain);
    GBDATA *gb_extended_data =GB_search(gbmain,"extended_data",GB_CREATE_CONTAINER);
    GBDATA *gb_extended =GBT_find_SAI_rel_exdata(gb_extended_data,source);
    GBDATA *gb_dest_sai =GBT_find_SAI_rel_exdata(gb_extended_data,dest);
    if (gb_dest_sai) {
        error = "Sorry: SAI already exists";
    }else   if (gb_extended) {
        GBDATA *gb_name =
            GB_search(gb_extended,"name",GB_STRING);
        error = GB_write_string(gb_name,dest);
    }else{
        error = "Please select a SAI first";
    }
    if (!error) GB_commit_transaction(gbmain);
    else    GB_abort_transaction(gbmain);
    if (error) aw_message(error);
    else aww->hide();
    delete source;
    delete dest;
}


AW_window *MG_create_extended_rename_window1(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "MERGE_RENAME_SAI_1", "SAI RENAME 1");
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label");
    aws->create_autosize_button(0,"Please enter the new name\nof the SAI");

    aws->at("input");
    aws->create_input_field(AWAR_EX_DEST1,15);

    aws->at("ok");
    aws->callback((AW_CB)MG_extended_rename_cb,(AW_CL)gb_merge,1);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}

AW_window *MG_create_extended_rename_window2(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "MERGE_RENAME_SAI_2", "SAI RENAME 2");
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label");
    aws->create_autosize_button(0,"Please enter the new name\nof the SAI");

    aws->at("input");
    aws->create_input_field(AWAR_EX_DEST2,15);

    aws->at("ok");
    aws->callback((AW_CB)MG_extended_rename_cb,(AW_CL)gb_dest,2);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}

void MG_extended_delete_cb(AW_window *aww, GBDATA *gbmain, int ex_nr){
    GB_ERROR error = 0;
    char tsource[256];
    sprintf(tsource,"tmp/merge%i/extended_name",ex_nr);
    char *source = aww->get_root()->awar(tsource)->read_string();
    GB_begin_transaction(gbmain);
    GBDATA *gb_extended = GBT_find_SAI(gbmain,source);

    if (gb_extended) {
        error = GB_delete(gb_extended);
    }else{
        error = "Please select a SAI first";
    }

    if (!error) GB_commit_transaction(gbmain);
    else    GB_abort_transaction(gbmain);

    if (error) aw_message(error);
    delete source;
}

void MG_transfer_extended(AW_window *aww, AW_CL force){
    AW_root *awr = aww->get_root();
    char *source = awr->awar(AWAR_EX_NAME1)->read_string();
    char *dest = awr->awar(AWAR_EX_NAME1)->read_string();
    GB_ERROR error = 0;
    GB_begin_transaction(gb_dest);
    GB_begin_transaction(gb_merge);

    GBDATA *gb_source = GBT_find_SAI(gb_merge,dest);
    GBDATA *gb_extended_data2 = GB_search(gb_dest,"extended_data",GB_CREATE_CONTAINER);
    GBDATA *gb_dest_sai = GBT_find_SAI_rel_exdata(gb_extended_data2,dest);

    if (!gb_source) error = "Please select an SAI you want to transfer !!!";
    else if(gb_dest_sai) {
        if (force) {
            error = GB_delete(gb_dest_sai);
        }else{
            error = "Sorry, You cannot destroy an old SAI, delete it first";
        }
    }
    if (!error) {
        gb_dest_sai = GB_create_container(gb_extended_data2,"extended");
        error = GB_copy(gb_dest_sai,gb_source);
    }

    if (!error) {
        GB_commit_transaction(gb_dest);
        GB_commit_transaction(gb_merge);
    }else{
        GB_abort_transaction(gb_dest);
        GB_abort_transaction(gb_merge);
    }
    if (error) aw_message(error);
    delete source;
    delete dest;
}

void MG_map_extended1(AW_root *aw_root, AW_CL scannerid)
{
    char *source = aw_root->awar(AWAR_EX_NAME1)->read_string();
    GB_push_transaction(gb_merge);
    GBDATA *gb_extended = GBT_find_SAI(gb_merge,source);
    awt_map_arbdb_scanner(scannerid,gb_extended,0, CHANGE_KEY_PATH);
    GB_pop_transaction(gb_merge);
    delete source;
}
void MG_map_extended2(AW_root *aw_root, AW_CL scannerid)
{
    char *source = aw_root->awar(AWAR_EX_NAME2)->read_string();
    GB_push_transaction(gb_dest);
    GBDATA *gb_extended = GBT_find_SAI(gb_dest,source);
    awt_map_arbdb_scanner(scannerid,gb_extended,0, CHANGE_KEY_PATH);
    GB_pop_transaction(gb_dest);
    delete source;
}


AW_window *MG_merge_extendeds_cb(AW_root *awr){
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    aws = new AW_window_simple;
    aws->init( awr, "MERGE_SAI", "MERGE SAI");
    aws->load_xfig("merge/extended.fig");

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"mg_extendeds.hlp");
    aws->create_button("HELP","HELP","H");

    aws->at("ex1");
    awt_create_selection_list_on_extendeds(gb_merge,(AW_window *)aws,AWAR_EX_NAME1);
    AW_CL scannerid = awt_create_arbdb_scanner(gb_merge, aws, "info1",0,0,0,AWT_SCANNER,0,0,0, &AWT_species_selector);
    aws->get_root()->awar(AWAR_EX_NAME1)->add_callback(MG_map_extended1,scannerid);

    aws->at("ex2");
    awt_create_selection_list_on_extendeds(gb_dest,(AW_window *)aws,AWAR_EX_NAME2);
    scannerid = awt_create_arbdb_scanner(gb_dest, aws, "info2",0,0,0,AWT_SCANNER,0,0,0, &AWT_species_selector);
    aws->get_root()->awar(AWAR_EX_NAME2)->add_callback(MG_map_extended2,scannerid);

    aws->button_length(20);

    aws->at("delete1");
    aws->callback((AW_CB)MG_extended_delete_cb,(AW_CL)gb_merge,1);
    aws->create_button("DELETE_SAI_DB1", "Delete SAI");

    aws->at("delete2");
    aws->callback((AW_CB)MG_extended_delete_cb,(AW_CL)gb_dest,2);
    aws->create_button("DELETE_SAI_DB2", "Delete SAI");

    aws->at("rename1");
    aws->callback((AW_CB1)AW_POPUP,(AW_CL)MG_create_extended_rename_window1);
    aws->create_button("RENAME_SAI_DB1", "Rename SAI");

    aws->at("rename2");
    aws->callback((AW_CB1)AW_POPUP,(AW_CL)MG_create_extended_rename_window2);
    aws->create_button("RENAME_SAI_DB2", "Rename SAI");

    aws->at("transfer");
    aws->callback(MG_transfer_extended,0);
    aws->create_button("TRANSFER_SAI", "Transfer SAI");

    aws->button_length(0);
    aws->shadow_width(1);
    aws->at("icon");
    aws->callback(AW_POPUP_HELP,(AW_CL)"mg_extendeds.hlp");
    aws->create_button("HELP_MERGE", "#merge/icon.bitmap");

    return (AW_window *)aws;
}
