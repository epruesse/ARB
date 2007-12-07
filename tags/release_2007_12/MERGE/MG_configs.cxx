//  ==================================================================== //
//                                                                       //
//    File      : MG_configs.cxx                                         //
//    Purpose   : Merge editor configurations                            //
//    Time-stamp: <Wed May/25/2005 17:23 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in July 2003             //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#include <stdio.h>
#include <stdlib.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <ad_config.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <awt.hxx>
#include <awt_sel_boxes.hxx>
#include "merge.hxx"

#define AWAR_CONFIG_NAME1 "tmp/merge1/config_name"
#define AWAR_CONFIG_DEST1 "tmp/merge1/config_dest"
#define AWAR_CONFIG_NAME2 "tmp/merge2/config_name"
#define AWAR_CONFIG_DEST2 "tmp/merge2/config_dest"

void MG_create_config_awar(AW_root *aw_root, AW_default aw_def)
{
    aw_root->awar_string( AWAR_CONFIG_NAME1, "" , aw_def);
    aw_root->awar_string( AWAR_CONFIG_DEST1, "" , aw_def);
    aw_root->awar_string( AWAR_CONFIG_NAME2, "" , aw_def);
    aw_root->awar_string( AWAR_CONFIG_DEST2, "" , aw_def);
}

void MG_config_rename_cb(AW_window *aww, GBDATA *gbd, int config_nr) {
    GB_ERROR    error   = 0;
    const char *tsource = config_nr == 1 ? AWAR_CONFIG_NAME1 : AWAR_CONFIG_NAME2;
    const char *tdest   = config_nr == 1 ? AWAR_CONFIG_DEST1 : AWAR_CONFIG_DEST2;

    char *source = aww->get_root()->awar(tsource)->read_string();
    char *dest   = aww->get_root()->awar(tdest)->read_string();

    error = GB_check_key(dest);
    if (!error) {
        GB_begin_transaction(gbd);
        GBDATA *gb_config_data = GB_search(gbd, AWAR_CONFIG_DATA, GB_CREATE_CONTAINER);
        GBDATA *gb_source_name = GB_find(gb_config_data, "name", source, down_2_level);
        GBDATA *gb_dest_name   = GB_find(gb_config_data, "name", dest, down_2_level);

        if (gb_dest_name) {
            error = "Sorry: Configuration already exists";
        }
        else if (gb_source_name) {
            error = GB_write_string(gb_source_name, dest);
        }
        else {
            error = "Please select a configuration first";
        }

        if (!error) GB_commit_transaction(gbd);
        else        GB_abort_transaction(gbd);
    }

    if (error) aw_message(error);
    else aww->hide();

    free(source);
    free(dest);
}

AW_window *MG_create_config_rename_window1(AW_root *root) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "MERGE_RENAME_CONFIG_1", "CONFIGURATION RENAME 1");
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label");
    aws->create_autosize_button(0,"Please enter the new name\nof the configuration");

    aws->at("input");
    aws->create_input_field(AWAR_CONFIG_DEST1,15);

    aws->at("ok");
    aws->callback((AW_CB)MG_config_rename_cb,(AW_CL)gb_merge,1);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}

AW_window *MG_create_config_rename_window2(AW_root *root) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "MERGE_RENAME_CONFIG_2", "CONFIGURATION RENAME 2");
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label");
    aws->create_autosize_button(0,"Please enter the new name\nof the configuration");

    aws->at("input");
    aws->create_input_field(AWAR_CONFIG_DEST2,15);

    aws->at("ok");
    aws->callback((AW_CB)MG_config_rename_cb,(AW_CL)gb_dest,2);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}

void MG_config_delete_cb(AW_window *aww,GBDATA *gbd, long config_nr) {
    GB_ERROR    error            = 0;
    const char *config_name_awar = config_nr == 1 ? AWAR_CONFIG_NAME1 : AWAR_CONFIG_NAME2;
    char       *config_name      = aww->get_root()->awar(config_name_awar)->read_string();

    GB_begin_transaction(gbd);

    GBDATA *gb_config_data = GB_search(gbd, AWAR_CONFIG_DATA, GB_CREATE_CONTAINER);
    GBDATA *gb_config_name = GB_find(gb_config_data, "name", config_name, down_2_level);

    if (gb_config_name) {
        GBDATA *gb_config = GB_get_father(gb_config_name);
        error             = GB_delete(gb_config);
    }
    else {
        error = "Select a config to delete";
    }

    if (!error) GB_commit_transaction(gbd);
    else        GB_abort_transaction(gbd);

    if (error) aw_message(error);
    free(config_name);
}

void MG_transfer_config(AW_window *aww) {
    AW_root  *awr    = aww->get_root();
    char     *source = awr->awar(AWAR_CONFIG_NAME1)->read_string();
    char     *dest   = awr->awar(AWAR_CONFIG_NAME1)->read_string();
    GB_ERROR  error  = 0;
    GB_begin_transaction(gb_dest);
    GB_begin_transaction(gb_merge);

    GBDATA *gb_config_data1 = GB_search(gb_merge, AWAR_CONFIG_DATA, GB_CREATE_CONTAINER);
    GBDATA *gb_config_data2 = GB_search(gb_dest,  AWAR_CONFIG_DATA, GB_CREATE_CONTAINER);
    GBDATA *gb_cfgname_1    = GB_find(gb_config_data1, "name", source, down_2_level);
    GBDATA *gb_cfgname_2    = GB_find(gb_config_data2, "name", dest, down_2_level);

    if (!gb_cfgname_1) {
        error = "Please select the configuration you want to transfer";
    }
    else if (gb_cfgname_2) {
        error = "To overwrite a configuration, delete it first!";
    }
    else {
        GBDATA *gb_cfg_1 = GB_get_father(gb_cfgname_1);
        GBDATA *gb_cfg_2 = GB_create_container(gb_config_data2, "configuration");
        error            = GB_copy(gb_cfg_2, gb_cfg_1);
    }

    if (!error) {
        GB_commit_transaction(gb_dest);
        GB_commit_transaction(gb_merge);
    }else{
        GB_abort_transaction(gb_dest);
        GB_abort_transaction(gb_merge);
    }
    if (error) aw_message(error);
    free(source);
    free(dest);
}

AW_window *MG_merge_configs_cb(AW_root *awr) {
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    aws = new AW_window_simple;
    aws->init( awr, "MERGE_CONFIGS", "MERGE CONFIGS");
    aws->load_xfig("merge/configs.fig");

    aws->button_length(20);

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"mg_configs.hlp");
    aws->create_button("HELP","HELP","H");

    aws->at("configs1");
    awt_create_selection_list_on_configurations(gb_merge,(AW_window *)aws,AWAR_CONFIG_NAME1);

    aws->at("configs2");
    awt_create_selection_list_on_configurations(gb_dest,(AW_window *)aws,AWAR_CONFIG_NAME2);

    aws->at("delete1");
    aws->callback((AW_CB)MG_config_delete_cb,(AW_CL)gb_merge,1);
    aws->create_button("DELETE CONFIG_DB1", "Delete Config");

    aws->at("delete2");
    aws->callback((AW_CB)MG_config_delete_cb,(AW_CL)gb_dest,2);
    aws->create_button("DELETE_CONFIG_DB2", "Delete Config");

    aws->at("rename1");
    aws->callback((AW_CB1)AW_POPUP,(AW_CL)MG_create_config_rename_window1);
    aws->create_button("RENAME_CONFIG_DB1", "Rename Config");

    aws->at("rename2");
    aws->callback((AW_CB1)AW_POPUP,(AW_CL)MG_create_config_rename_window2);
    aws->create_button("RENAME_CONFIG_DB2", "Rename Config");

    aws->at("transfer");
    aws->callback(MG_transfer_config);
    aws->create_button("TRANSFER_CONFIG", "Transfer Config");

    aws->button_length(0);
    aws->shadow_width(1);
    aws->at("icon");
    aws->callback(AW_POPUP_HELP,(AW_CL)"mg_configs.hlp");
    aws->create_button("HELP_MERGE", "#merge/icon.bitmap");

    return (AW_window *)aws;
}


