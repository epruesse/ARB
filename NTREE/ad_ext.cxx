#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
// #include <malloc.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <awt.hxx>
#include <awt_sel_boxes.hxx>
#include <aw_awars.hxx>
#include <db_scanner.hxx>

#define AWAR_SAI_DEST "tmp/focus/extended_dest"

extern GBDATA *GLOBAL_gb_main;

void NT_create_extendeds_var(AW_root *aw_root, AW_default aw_def) {
    aw_root->awar_string(AWAR_SAI_NAME, "", aw_def);
    aw_root->awar_string(AWAR_SAI_DEST, "", aw_def);
}

void extended_rename_cb(AW_window *aww){
    char     *source = aww->get_root()->awar(AWAR_SAI_NAME)->read_string();
    char     *dest   = aww->get_root()->awar(AWAR_SAI_DEST)->read_string();
    GB_ERROR  error  = GB_begin_transaction(GLOBAL_gb_main);

    if (!error) {
        GBDATA *gb_extended_data = GB_search(GLOBAL_gb_main,"extended_data",GB_CREATE_CONTAINER);

        if (!gb_extended_data) error = GB_get_error();
        else {
            GBDATA *gb_extended = GBT_find_SAI_rel_exdata(gb_extended_data,source);
            GBDATA *gb_dest     = GBT_find_SAI_rel_exdata(gb_extended_data,dest);
            if (gb_dest) {
                error = GBS_global_string("SAI '%s' already exists", dest);
            }
            else if (gb_extended) {
                GBDATA *gb_name     = GB_search(gb_extended,"name",GB_STRING);
                if (!gb_name) error = GB_get_error();
                else    error       = GB_write_string(gb_name,dest);
            }
            else {
                error = "Please select a SAI";
            }
        }
    }

    GB_end_transaction_show_error(GLOBAL_gb_main, error, aw_message);
    free(dest);
    free(source);
}

void extended_copy_cb(AW_window *aww) {
    char     *source = aww->get_root()->awar(AWAR_SAI_NAME)->read_string();
    char     *dest   = aww->get_root()->awar(AWAR_SAI_DEST)->read_string();
    GB_ERROR  error  = GB_begin_transaction(GLOBAL_gb_main);
    
    if (!error) {
        GBDATA *gb_extended_data = GB_search(GLOBAL_gb_main,"extended_data",GB_CREATE_CONTAINER);

        if (!gb_extended_data) error = GB_get_error();
        else {
            GBDATA *gb_extended = GBT_find_SAI_rel_exdata(gb_extended_data, source);
            GBDATA *gb_dest     = GBT_find_SAI_rel_exdata(gb_extended_data, dest);
            
            if (gb_dest) error = GBS_global_string("SAI '%s' already exists", dest);
            else if (gb_extended) {
                gb_dest             = GB_create_container(gb_extended_data,"extended");
                if (!gb_dest) error = GB_get_error();
                else {
                    error = GB_copy(gb_dest,gb_extended);
                    if (!error) {
                        GBDATA *gb_name = GB_search(gb_dest,"name",GB_STRING);
                        
                        if (!gb_name) error = GB_get_error();
                        else error          = GB_write_string(gb_name,dest);
                    }
                }
            }
            else error = "Please select a SAI";
        }
    }
    GB_end_transaction_show_error(GLOBAL_gb_main, error, aw_message);
    free(dest);
    free(source);
}

void move_to_sepcies(AW_window *aww) {
    char     *source = aww->get_root()->awar(AWAR_SAI_NAME)->read_string();
    GB_ERROR  error  = GB_begin_transaction(GLOBAL_gb_main);

    if (!error) {
        GBDATA *gb_species_data     = GB_search(GLOBAL_gb_main,"species_data",GB_CREATE_CONTAINER);
        if (!gb_species_data) error = GB_get_error();
        else {
            GBDATA *gb_dest     = GBT_find_species_rel_species_data(gb_species_data,source);
            GBDATA *gb_extended = GBT_find_SAI(GLOBAL_gb_main,source);

            if (gb_dest) {
                error = GBS_global_string("Species '%s' already exists", source);
            }
            else if (gb_extended) {
                gb_dest             = GB_create_container(gb_species_data,"species");
                if (!gb_dest) error = GB_get_error();
                else error          = GB_copy(gb_dest,gb_extended);
            }
            else {
                error = "Please select a SAI";
            }
        }
    }
    GB_end_transaction_show_error(GLOBAL_gb_main, error, aw_message);
    free(source);
}

AW_window *create_extended_rename_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "RENAME_SAI", "SAI RENAME");
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label");
    aws->create_autosize_button(0,"Please enter the new name\nof the SAI");

    aws->at("input");
    aws->create_input_field(AWAR_SAI_DEST,15);

    aws->at("ok");
    aws->callback(extended_rename_cb);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}

AW_window *create_extended_copy_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "COPY_SAI", "SAI COPY");
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label");
    aws->create_autosize_button(0,"Please enter the name\nof the new SAI");

    aws->at("input");
    aws->create_input_field(AWAR_SAI_DEST,15);

    aws->at("ok");
    aws->callback(extended_copy_cb);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}

void ad_extended_delete_cb(AW_window *aww) {
    char     *source      = aww->get_root()->awar(AWAR_SAI_NAME)->read_string();
    GB_ERROR  error       = GB_begin_transaction(GLOBAL_gb_main);

    if (!error) {
        GBDATA *gb_extended    = GBT_find_SAI(GLOBAL_gb_main,source);
        if (gb_extended) error = GB_delete(gb_extended);
        else    error          = "Please select a SAI";
    }
    GB_end_transaction_show_error(GLOBAL_gb_main, error, aw_message);
    free(source);
}

void AD_map_extended(AW_root *aw_root, AW_CL scannerid)
{
    char   *source      = aw_root->awar(AWAR_SAI_NAME)->read_string();
    GB_push_transaction(GLOBAL_gb_main);
    GBDATA *gb_extended = GBT_find_SAI(GLOBAL_gb_main,source);
    awt_map_arbdb_scanner(scannerid,gb_extended,0, CHANGE_KEY_PATH);
    GB_pop_transaction(GLOBAL_gb_main);
    free(source);
}

void ad_ad_remark(AW_window *aww){
    AW_root *awr = aww->get_root();
    GB_transaction dummy(GLOBAL_gb_main);
    char *source = awr->awar(AWAR_SAI_NAME)->read_string();
    GBDATA *gb_sai = GBT_find_SAI(GLOBAL_gb_main,source);
    if (gb_sai){
        char *use = GBT_get_default_alignment(GLOBAL_gb_main);
        GBDATA *gb_ali = GB_search(gb_sai,use,GB_FIND);
        if (gb_ali){
            GBDATA *typ = GB_search(gb_ali,"_TYPE",GB_STRING);
            awr->awar_string("/tmp/ntree/sai/add_remark")->map(typ);
            char *error = aw_input("Change SAI description", "/tmp/ntree/sai/add_remark");
            free(error);
        }else{
            aw_message("Please select an alignment which is valid for the selected SAI");
        }
        free(use);
    }else{
        aw_message("Please select a SAI first");
    }
    free(source);
}

void ad_ad_group(AW_window *aww){
    AW_root *awr = aww->get_root();
    GB_transaction dummy(GLOBAL_gb_main);
    char *source = awr->awar(AWAR_SAI_NAME)->read_string();
    GBDATA *gb_sai = GBT_find_SAI(GLOBAL_gb_main,source);
    if (gb_sai){
        GBT_readOrCreate_char_pntr(gb_sai,"sai_group","default_group"); // set default if missing
        GBDATA *gb_gn = GB_search(gb_sai,"sai_group",GB_STRING);
        awr->awar_string("/tmp/ntree/sai/add_group")->map(gb_gn);
        char *res = aw_input("Assign Group to  SAI","/tmp/ntree/sai/add_group");
        if (!res || !strlen(res)){
            GB_delete(gb_gn);
        }
        free(res);
    }else{
        aw_message("Please select a SAI first");
    }
    free(source);
}

AW_window *NT_create_extendeds_window(AW_root *aw_root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "INFO_OF_SAI", "SAI INFORMATION");
    aws->load_xfig("ad_ext.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->callback( AW_POPUP_HELP,(AW_CL)"ad_extended.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","H");

    aws->button_length(13);

    aws->at("delete");
    aws->callback(ad_extended_delete_cb);
    aws->create_button("DELETE","DELETE","D");

    aws->at("rename");
    aws->callback(AW_POPUP,(AW_CL)create_extended_rename_window,0);
    aws->create_button("RENAME","RENAME","R");

    aws->at("copy");
    aws->callback(AW_POPUP,(AW_CL)create_extended_copy_window,0);
    aws->create_button("COPY","COPY","C");

    aws->at("remark");
    aws->callback(ad_ad_remark);
    aws->create_button("EDIT_COMMENT","EDIT COMMENT","R");

    aws->at("group");
    aws->callback(ad_ad_group);
    aws->create_button("ASSIGN_GROUP","ASSIGN GROUP","R");

    aws->at("makespec");
    aws->callback((AW_CB0)move_to_sepcies);
    aws->create_button("COPY_TO_SPECIES","COPY TO\nSPECIES","C");

    aws->at("list");
    awt_create_selection_list_on_extendeds(GLOBAL_gb_main,(AW_window *)aws,AWAR_SAI_NAME);

    AW_CL scannerid = awt_create_arbdb_scanner(GLOBAL_gb_main, aws, "info",0,0,0,AWT_SCANNER,0,0,0, &AWT_species_selector);
    aws->get_root()->awar(AWAR_SAI_NAME)->add_callback(AD_map_extended,scannerid);
    return (AW_window *)aws;
}
