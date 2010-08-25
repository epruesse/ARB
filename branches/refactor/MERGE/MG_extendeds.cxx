// =============================================================== //
//                                                                 //
//   File      : MG_extendeds.cxx                                  //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "merge.hxx"
#include <db_scanner.hxx>
#include <awt.hxx>
#include <awt_sel_boxes.hxx>
#include <aw_awars.hxx>
#include <arbdbt.h>

#define AWAR_EX_NAME1 "tmp/merge1/extended_name"
#define AWAR_EX_DEST1 "tmp/merge1/extended_dest"
#define AWAR_EX_NAME2 "tmp/merge2/extended_name"
#define AWAR_EX_DEST2 "tmp/merge2/extended_dest"

void MG_create_extendeds_awars(AW_root *aw_root, AW_default aw_def)
{
    aw_root->awar_string(AWAR_EX_NAME1, "",     aw_def);
    aw_root->awar_string(AWAR_EX_DEST1, "",     aw_def);
    aw_root->awar_string(AWAR_EX_NAME2, "",     aw_def);
    aw_root->awar_string(AWAR_EX_DEST2, "",     aw_def);
}

void MG_extended_rename_cb(AW_window *aww, GBDATA *gbmain, int ex_nr) {
    char *source = aww->get_root()->awar(GBS_global_string("tmp/merge%i/extended_name", ex_nr))->read_string();
    char *dest   = aww->get_root()->awar(GBS_global_string("tmp/merge%i/extended_dest", ex_nr))->read_string();

    GB_ERROR error = GB_begin_transaction(gbmain);
    if (!error) {
        GBDATA *gb_sai_data     = GBT_get_SAI_data(gbmain);
        if (!gb_sai_data) error = GB_await_error();
        else {
            GBDATA *gb_sai      = GBT_find_SAI_rel_SAI_data(gb_sai_data, source);
            GBDATA *gb_dest_sai = GBT_find_SAI_rel_SAI_data(gb_sai_data, dest);

            if (gb_dest_sai) error  = GBS_global_string("SAI '%s' already exists", dest);
            else if (!gb_sai) error = "Please select a SAI";
            else error              = GBT_write_string(gb_sai, "name", dest);
        }
    }
    error = GB_end_transaction(gbmain, error);
    aww->hide_or_notify(error);

    free(dest);
    free(source);
}



AW_window *MG_create_extended_rename_window1(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "MERGE_RENAME_SAI_1", "SAI RENAME 1");
    aws->load_xfig("ad_al_si.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the new name\nof the SAI");

    aws->at("input");
    aws->create_input_field(AWAR_EX_DEST1, 15);

    aws->at("ok");
    aws->callback((AW_CB)MG_extended_rename_cb, (AW_CL)GLOBAL_gb_merge, 1);
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}

AW_window *MG_create_extended_rename_window2(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "MERGE_RENAME_SAI_2", "SAI RENAME 2");
    aws->load_xfig("ad_al_si.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the new name\nof the SAI");

    aws->at("input");
    aws->create_input_field(AWAR_EX_DEST2, 15);

    aws->at("ok");
    aws->callback((AW_CB)MG_extended_rename_cb, (AW_CL)GLOBAL_gb_dest, 2);
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}

void MG_extended_delete_cb(AW_window *aww, GBDATA *gbmain, int ex_nr) {
    GB_ERROR error = GB_begin_transaction(gbmain);

    if (!error) {
        const char *tsource = GBS_global_string("tmp/merge%i/extended_name", ex_nr);
        char       *source  = aww->get_root()->awar(tsource)->read_string();

        GBDATA *gb_sai    = GBT_find_SAI(gbmain, source);
        if (gb_sai) error = GB_delete(gb_sai);
        else    error     = "Please select a SAI first";

        free(source);
    }
    GB_end_transaction_show_error(gbmain, error, aw_message);
}

void MG_transfer_extended(AW_window *aww, AW_CL force) {
    AW_root *awr    = aww->get_root();
    char    *source = awr->awar(AWAR_EX_NAME1)->read_string();
    char    *dest   = awr->awar(AWAR_EX_NAME1)->read_string();

    GB_ERROR error    = GB_begin_transaction(GLOBAL_gb_dest);
    if (!error) error = GB_begin_transaction(GLOBAL_gb_merge);

    if (!error) {
        GBDATA *gb_source = GBT_find_SAI(GLOBAL_gb_merge, dest);

        if (!gb_source) error = "Please select the SAI you want to transfer";
        else {
            GBDATA *gb_sai_data2     = GBT_get_SAI_data(GLOBAL_gb_dest);
            if (!gb_sai_data2) error = GB_await_error();
            else {
                GBDATA *gb_dest_sai = GBT_find_SAI_rel_SAI_data(gb_sai_data2, dest);
                if (gb_dest_sai) {
                    if (force) error = GB_delete(gb_dest_sai);
                    else error       = GBS_global_string("SAI '%s' exists, delete it first", dest);
                }
                if (!error) {
                    gb_dest_sai             = GB_create_container(gb_sai_data2, "extended");
                    if (!gb_dest_sai) error = GB_await_error();
                    else error              = GB_copy(gb_dest_sai, gb_source);
                }
            }
        }
    }

    error = GB_end_transaction(GLOBAL_gb_dest, error);
    error = GB_end_transaction(GLOBAL_gb_merge, error);
    if (error) aw_message(error);

    free(source);
    free(dest);
}

void MG_map_extended1(AW_root *aw_root, AW_CL scannerid)
{
    char *source = aw_root->awar(AWAR_EX_NAME1)->read_string();
    GB_push_transaction(GLOBAL_gb_merge);
    GBDATA *gb_sai = GBT_find_SAI(GLOBAL_gb_merge, source);
    map_db_scanner(scannerid, gb_sai, CHANGE_KEY_PATH);
    GB_pop_transaction(GLOBAL_gb_merge);
    free(source);
}
void MG_map_extended2(AW_root *aw_root, AW_CL scannerid)
{
    char *source = aw_root->awar(AWAR_EX_NAME2)->read_string();
    GB_push_transaction(GLOBAL_gb_dest);
    GBDATA *gb_sai = GBT_find_SAI(GLOBAL_gb_dest, source);
    map_db_scanner(scannerid, gb_sai, CHANGE_KEY_PATH);
    GB_pop_transaction(GLOBAL_gb_dest);
    free(source);
}


AW_window *MG_merge_extendeds_cb(AW_root *awr) {
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    aws = new AW_window_simple;
    aws->init(awr, "MERGE_SAI", "MERGE SAI");
    aws->load_xfig("merge/extended.fig");

    aws->at("close"); aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"mg_extendeds.hlp");
    aws->create_button("HELP", "HELP", "H");

    aws->at("ex1");
    awt_create_selection_list_on_extendeds(GLOBAL_gb_merge, (AW_window *)aws, AWAR_EX_NAME1);
    AW_CL scannerid = create_db_scanner(GLOBAL_gb_merge, aws, "info1", 0, 0, 0, DB_SCANNER, 0, 0, 0, &AWT_species_selector);
    aws->get_root()->awar(AWAR_EX_NAME1)->add_callback(MG_map_extended1, scannerid);

    aws->at("ex2");
    awt_create_selection_list_on_extendeds(GLOBAL_gb_dest, (AW_window *)aws, AWAR_EX_NAME2);
    scannerid = create_db_scanner(GLOBAL_gb_dest, aws, "info2", 0, 0, 0, DB_SCANNER, 0, 0, 0, &AWT_species_selector);
    aws->get_root()->awar(AWAR_EX_NAME2)->add_callback(MG_map_extended2, scannerid);

    aws->button_length(20);

    aws->at("delete1");
    aws->callback((AW_CB)MG_extended_delete_cb, (AW_CL)GLOBAL_gb_merge, 1);
    aws->create_button("DELETE_SAI_DB1", "Delete SAI");

    aws->at("delete2");
    aws->callback((AW_CB)MG_extended_delete_cb, (AW_CL)GLOBAL_gb_dest, 2);
    aws->create_button("DELETE_SAI_DB2", "Delete SAI");

    aws->at("rename1");
    aws->callback((AW_CB1)AW_POPUP, (AW_CL)MG_create_extended_rename_window1);
    aws->create_button("RENAME_SAI_DB1", "Rename SAI");

    aws->at("rename2");
    aws->callback((AW_CB1)AW_POPUP, (AW_CL)MG_create_extended_rename_window2);
    aws->create_button("RENAME_SAI_DB2", "Rename SAI");

    aws->at("transfer");
    aws->callback(MG_transfer_extended, 0);
    aws->create_button("TRANSFER_SAI", "Transfer SAI");

    aws->button_length(0);
    aws->shadow_width(1);
    aws->at("icon");
    aws->callback(AW_POPUP_HELP, (AW_CL)"mg_extendeds.hlp");
    aws->create_button("HELP_MERGE", "#merge/icon.bitmap");

    return (AW_window *)aws;
}
