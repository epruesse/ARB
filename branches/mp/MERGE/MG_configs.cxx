//  ==================================================================== //
//                                                                       //
//    File      : MG_configs.cxx                                         //
//    Purpose   : Merge editor configurations                            //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in July 2003             //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#include "merge.hxx"

#include <ad_config.h>
#include <awt_sel_boxes.hxx>
#include <aw_root.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <aw_window.hxx>
#include <arbdb.h>

#define AWAR_CONFIG_NAME_SRC AWAR_MERGE_TMP_SRC "name"
#define AWAR_CONFIG_NAME_DST AWAR_MERGE_TMP_DST "name"
#define AWAR_CONFIG_NEWNAME  AWAR_MERGE_TMP "newname"

void MG_create_config_awar(AW_root *aw_root, AW_default aw_def)
{
    aw_root->awar_string(AWAR_CONFIG_NAME_SRC, "",   aw_def);
    aw_root->awar_string(AWAR_CONFIG_NAME_DST, "",   aw_def);
    aw_root->awar_string(AWAR_CONFIG_NEWNAME, "",   aw_def);
}

static void MG_config_rename_cb(AW_window *aww, GBDATA *gbd, int db_nr) {
    const char *tsource = db_nr == 1 ? AWAR_CONFIG_NAME_SRC : AWAR_CONFIG_NAME_DST;
    char       *source  = aww->get_root()->awar(tsource)->read_string();
    char       *dest    = aww->get_root()->awar(AWAR_CONFIG_NEWNAME)->read_string();

    GB_ERROR error = GB_check_key(dest);
    if (!error) {
        error = GB_begin_transaction(gbd);
        if (!error) {
            GBDATA *gb_config_data     = GB_search(gbd, CONFIG_DATA_PATH, GB_CREATE_CONTAINER);
            if (!gb_config_data) error = GB_await_error();
            else {
                GBDATA *gb_dest_name    = GB_find_string(gb_config_data, "name", dest, GB_IGNORE_CASE, SEARCH_GRANDCHILD);
                if (gb_dest_name) error = GBS_global_string("Configuration '%s' already exists", dest);
                else {
                    GBDATA *gb_source_name    = GB_find_string(gb_config_data, "name", source, GB_IGNORE_CASE, SEARCH_GRANDCHILD);
                    if (gb_source_name) error = GB_write_string(gb_source_name, dest);
                    else    error             = "Please select a configuration";
                }
            }
        }
        error = GB_end_transaction(gbd, error);
    }

    aww->hide_or_notify(error);

    free(source);
    free(dest);
}

static AW_window *MG_create_config_rename_window(AW_root *root, AW_CL db_nr) {
    AW_window_simple *aws = new AW_window_simple;
    if (db_nr == 1) {
        aws->init(root, "MERGE_RENAME_CONFIG_1", "CONFIGURATION RENAME 1");
    }
    else {
        aws->init(root, "MERGE_RENAME_CONFIG_2", "CONFIGURATION RENAME 2");
    }
    aws->load_xfig("ad_al_si.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the new name\nof the configuration");

    aws->at("input");
    aws->create_input_field(AWAR_CONFIG_NEWNAME, 15);

    aws->at("ok");
    aws->callback((AW_CB)MG_config_rename_cb, (AW_CL)GLOBAL_gb_dst, db_nr);
    aws->create_button("GO", "GO", "G");

    return aws;
}

static void MG_config_delete_cb(AW_window *aww, AW_CL db_nr) {
    const char *config_name_awar = db_nr == 1 ? AWAR_CONFIG_NAME_SRC : AWAR_CONFIG_NAME_DST;
    char       *config_name      = aww->get_root()->awar(config_name_awar)->read_string();

    GBDATA   *gb_main = get_gb_main(db_nr);
    GB_ERROR  error   = GB_begin_transaction(gb_main);

    if (!error) {
        GBDATA *gb_config_data = GB_search(gb_main, CONFIG_DATA_PATH, GB_CREATE_CONTAINER);
        GBDATA *gb_config_name = GB_find_string(gb_config_data, "name", config_name, GB_IGNORE_CASE, SEARCH_GRANDCHILD);

        if (gb_config_name) {
            GBDATA *gb_config = GB_get_father(gb_config_name);
            error             = GB_delete(gb_config);
        }
        else {
            error = "Select a config to delete";
        }
    }

    GB_end_transaction_show_error(gb_main, error, aw_message);

    free(config_name);
}

static void MG_transfer_config(AW_window *aww) {
    AW_root *awr    = aww->get_root();
    char    *source = awr->awar(AWAR_CONFIG_NAME_SRC)->read_string();
    char    *dest   = awr->awar(AWAR_CONFIG_NAME_SRC)->read_string();

    GB_ERROR error = GB_begin_transaction(GLOBAL_gb_dst);
    if (!error) {
        error = GB_begin_transaction(GLOBAL_gb_src);
        if (!error) {
            GBDATA *gb_config_data1 = GB_search(GLOBAL_gb_src, CONFIG_DATA_PATH, GB_CREATE_CONTAINER);
            GBDATA *gb_config_data2 = GB_search(GLOBAL_gb_dst,  CONFIG_DATA_PATH, GB_CREATE_CONTAINER);
            GBDATA *gb_cfgname_1    = GB_find_string(gb_config_data1, "name", source, GB_IGNORE_CASE, SEARCH_GRANDCHILD);
            GBDATA *gb_cfgname_2    = GB_find_string(gb_config_data2, "name", dest,   GB_IGNORE_CASE, SEARCH_GRANDCHILD);

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
        }
    }
    error = GB_end_transaction(GLOBAL_gb_src, error);
    error = GB_end_transaction(GLOBAL_gb_dst, error);

    if (error) aw_message(error);

    free(source);
    free(dest);
}

AW_window *MG_merge_configs_cb(AW_root *awr) {
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    aws = new AW_window_simple;
    aws->init(awr, "MERGE_CONFIGS", "MERGE CONFIGS");
    aws->load_xfig("merge/configs.fig");

    aws->button_length(20);

    aws->at("close"); aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"mg_configs.hlp");
    aws->create_button("HELP", "HELP", "H");

    aws->at("configs1");
    awt_create_selection_list_on_configurations(GLOBAL_gb_src, aws, AWAR_CONFIG_NAME_SRC);

    aws->at("configs2");
    awt_create_selection_list_on_configurations(GLOBAL_gb_dst, aws, AWAR_CONFIG_NAME_DST);

    aws->at("delete1");
    aws->callback(MG_config_delete_cb, 1);
    aws->create_button("DELETE CONFIG_DB1", "Delete Config");

    aws->at("delete2");
    aws->callback(MG_config_delete_cb, 2);
    aws->create_button("DELETE_CONFIG_DB2", "Delete Config");

    aws->at("rename1");
    aws->callback(AW_POPUP, (AW_CL)MG_create_config_rename_window, 1);
    aws->create_button("RENAME_CONFIG_DB1", "Rename Config");

    aws->at("rename2");
    aws->callback(AW_POPUP, (AW_CL)MG_create_config_rename_window, 2);
    aws->create_button("RENAME_CONFIG_DB2", "Rename Config");

    aws->at("transfer");
    aws->callback(MG_transfer_config);
    aws->create_button("TRANSFER_CONFIG", "Transfer Config");

    aws->button_length(0);
    aws->shadow_width(1);
    aws->at("icon");
    aws->callback(AW_POPUP_HELP, (AW_CL)"mg_configs.hlp");
    aws->create_button("HELP_MERGE", "#merge/icon.bitmap");

    return (AW_window *)aws;
}

