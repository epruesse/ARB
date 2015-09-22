// =============================================================== //
//                                                                 //
//   File      : ad_ext.cxx                                        //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "NT_local.h"
#include <db_scanner.hxx>
#include <awt_sel_boxes.hxx>
#include <aw_awars.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <aw_select.hxx>
#include <arbdbt.h>
#include <arb_strarray.h>
#include <arb_sort.h>

static void rename_SAI_cb(AW_window *aww) {
    AW_awar  *awar_sai = aww->get_root()->awar(AWAR_SAI_NAME);
    char     *sai_name = awar_sai->read_string();
    GB_ERROR  error    = 0;

    if (!sai_name || !sai_name[0]) error = "Select SAI to rename";
    else {
        char *new_name = aw_input("Rename SAI", "Enter new name of SAI", sai_name);
        if (new_name && new_name[0]) {
            error = GB_begin_transaction(GLOBAL.gb_main);
            if (!error) {
                GBDATA *gb_sai     = GBT_find_SAI(GLOBAL.gb_main, sai_name);
                if (!gb_sai) error = GBS_global_string("can't find SAI '%s'", sai_name);
                else {
                    GBDATA *gb_dest_exists    = GBT_find_SAI(GLOBAL.gb_main, new_name);
                    if (gb_dest_exists) error = GBS_global_string("There is already a SAI named '%s'", new_name);
                    else {
                        error = GBT_write_string(gb_sai, "name", new_name);
                        if (!error) awar_sai->write_string(new_name);
                    }
                }
            }
            error = GB_end_transaction(GLOBAL.gb_main, error);
        }
        free(new_name);
    }
    free(sai_name);

    if (error) aw_message(error);
}

static void copy_SAI_cb(AW_window *aww) {
    AW_awar  *awar_sai    = aww->get_root()->awar(AWAR_SAI_NAME);
    char     *source_name = awar_sai->read_string();
    GB_ERROR  error       = 0;

    if (!source_name || !source_name[0]) error = "Select SAI to duplicate";
    else {
        char *dest_name = aw_input("Copy SAI", "Enter name of new SAI", source_name);
        if (dest_name && dest_name[0]) {
            error = GB_begin_transaction(GLOBAL.gb_main);
            if (!error) {
                GBDATA *gb_sai_data   = GBT_get_SAI_data(GLOBAL.gb_main);
                GBDATA *gb_source     = GBT_find_SAI_rel_SAI_data(gb_sai_data, source_name);
                if (!gb_source) error = GBS_global_string("can't find SAI '%s'", source_name);
                else {
                    GBDATA *gb_dest_exists    = GBT_find_SAI_rel_SAI_data(gb_sai_data, dest_name);
                    if (gb_dest_exists) error = GBS_global_string("There is already a SAI named '%s'", dest_name);
                    else {
                        GBDATA *gb_dest     = GB_create_container(gb_sai_data, "extended");
                        if (!gb_dest) error = GB_await_error();
                        else {
                            error             = GB_copy(gb_dest, gb_source);
                            if (!error) {
                                error = GBT_write_string(gb_dest, "name", dest_name);
                                if (!error) awar_sai->write_string(dest_name);
                            }
                        }
                    }
                }
            }
            error = GB_end_transaction(GLOBAL.gb_main, error);
        }
        free(dest_name);
    }
    free(source_name);

    if (error) aw_message(error);
}

static void copy_SAI_to_species_cb(AW_window *aww) {
    AW_root  *aw_root  = aww->get_root();
    char     *sai_name = aw_root->awar(AWAR_SAI_NAME)->read_string();
    GB_ERROR  error    = 0;

    if (!sai_name || !sai_name[0]) error = "No SAI selected";
    else {
        char *species_name = aw_input("Copy SAI to species", "Enter target species name:", sai_name);

        if (species_name && species_name[0]) {
            error = GB_begin_transaction(GLOBAL.gb_main);

            if (!error) {
                GBDATA *gb_species_data = GBT_get_species_data(GLOBAL.gb_main);
                GBDATA *gb_dest         = GBT_find_species_rel_species_data(gb_species_data, species_name);

                if (gb_dest) error = GBS_global_string("Species '%s' already exists", species_name);
                else {
                    GBDATA *gb_sai = GBT_find_SAI(GLOBAL.gb_main, sai_name);

                    if (!gb_sai) error = GBS_global_string("SAI '%s' not found", sai_name);
                    else {
                        gb_dest             = GB_create_container(gb_species_data, "species");
                        if (!gb_dest) error = GB_await_error();
                        else {
                            error = GB_copy(gb_dest, gb_sai);
                            if (!error) {
                                error = GBT_write_string(gb_dest, "name", species_name);
                                if (!error) aw_root->awar(AWAR_SPECIES_NAME)->write_string(species_name);
                            }
                        }
                    }
                }
            }
            error = GB_end_transaction(GLOBAL.gb_main, error);
        }
        free(species_name);
    }
    free(sai_name);
    if (error) aw_message(error);
}

static void delete_SAI_cb(AW_window *aww) {
    char     *sai_name      = aww->get_root()->awar(AWAR_SAI_NAME)->read_string();
    GB_ERROR  error       = GB_begin_transaction(GLOBAL.gb_main);

    if (!error) {
        GBDATA *gb_sai = GBT_find_SAI(GLOBAL.gb_main, sai_name);
        error          = gb_sai ? GB_delete(gb_sai) : "Please select a SAI";
    }
    GB_end_transaction_show_error(GLOBAL.gb_main, error, aw_message);
    free(sai_name);
}

static void map_SAI_to_scanner(AW_root *aw_root, DbScanner *scanner) {
    GB_transaction  ta(GLOBAL.gb_main);
    char           *sai_name = aw_root->awar(AWAR_SAI_NAME)->read_string();
    GBDATA         *gb_sai   = GBT_find_SAI(GLOBAL.gb_main, sai_name);

    map_db_scanner(scanner, gb_sai, CHANGE_KEY_PATH);
    free(sai_name);
}

static void edit_SAI_description(AW_window *aww) {
    AW_root  *awr      = aww->get_root();
    char     *sai_name = awr->awar(AWAR_SAI_NAME)->read_string();
    GB_ERROR  error    = 0;

    if (!sai_name || !sai_name[0]) error = "No SAI selected";
    else {
        GBDATA *gb_ali = 0;
        char   *type    = 0;
        {
            GB_transaction  ta(GLOBAL.gb_main);
            GBDATA         *gb_sai = GBT_find_SAI(GLOBAL.gb_main, sai_name);

            if (!gb_sai) error = GBS_global_string("SAI '%s' not found", sai_name);
            else {
                char *ali_name = GBT_get_default_alignment(GLOBAL.gb_main);

                gb_ali = GB_entry(gb_sai, ali_name);
                if (!gb_ali) error = GBS_global_string("SAI '%s' has no data in alignment '%s'", sai_name, ali_name);
                else {
                    GB_clear_error();
                    type = GBT_read_string(gb_ali, "_TYPE");
                    if (!type) {
                        if (GB_have_error()) {
                            error = GB_await_error();
                        }
                        else {
                            type = strdup("");
                        }
                    }
                }
            }
            error = ta.close(error);
        }

        if (!error) {
            nt_assert(gb_ali);
            char *new_type = aw_input("Change SAI description", type);
            if (new_type) {
                GB_transaction t2(GLOBAL.gb_main);

                if (new_type[0]) {
                    error = GBT_write_string(gb_ali, "_TYPE", new_type);
                }
                else { // empty description -> delete
                    GBDATA *gb_type = GB_entry(gb_ali, "_TYPE");
                    if (gb_type) error = GB_delete(gb_type);
                }
                error = t2.close(error);
                free(new_type);
            }
        }

        free(type);
    }

    if (error) aw_message(error);
}

static GB_ERROR set_SAI_group(GBDATA *gb_main, const char *sai_name, const char *group_name) {
    GB_transaction ta(gb_main);

    GB_ERROR  error  = NULL;
    GBDATA   *gb_sai = GBT_find_SAI(gb_main, sai_name);
    if (!gb_sai) {
        error = GBS_global_string("SAI '%s' not found", sai_name);
    }
    else {
        const char *old_group_name = GBT_read_char_pntr(gb_sai, "sai_group");
        nt_assert(group_name);
        if (!old_group_name || strcmp(old_group_name, group_name) != 0) {
            if (group_name[0]) { // assign group
                error = GBT_write_string(gb_sai, "sai_group", group_name);
            }
            else { // remove group
                GBDATA *gb_group    = GB_entry(gb_sai, "sai_group");
                if (gb_group) error = GB_delete(gb_group);
            }
        }
    }
    return error;
}

static const char *get_SAI_group(GBDATA *gb_main, const char *sai_name) {
    GB_transaction  ta(gb_main);
    GBDATA         *gb_sai     = GBT_find_SAI(gb_main, sai_name);
    const char     *group_name = "";
    if (gb_sai) group_name     = GBT_read_char_pntr(gb_sai, "sai_group");
    return group_name;
}

static void get_SAI_groups(GBDATA *gb_main, ConstStrArray& sai_groups) {
    GB_transaction ta(gb_main);
    for (GBDATA *gb_sai = GBT_first_SAI(gb_main); gb_sai; gb_sai = GBT_next_SAI(gb_sai)) {
        const char *group = GBT_read_char_pntr(gb_sai, "sai_group");
        if (group) sai_groups.put(group);
    }
    sai_groups.sort_and_uniq(GB_string_comparator, 0);
}

static void fill_SAI_group_selection_list(AW_selection_list *sel, GBDATA *gb_main) {
    ConstStrArray sai_groups;
    get_SAI_groups(gb_main, sai_groups);
    sel->init_from_array(sai_groups, "<nogroup>", "");
}

#define AWAR_SAI_GROUP          "tmp/extended/group"
#define AWAR_SAI_REFRESH_GROUPS "tmp/extended/refresh"

static void refresh_grouplist(AW_root *aw_root) {
    aw_root->awar(AWAR_SAI_REFRESH_GROUPS)->touch(); // fill/refresh group selection list
}

static void set_SAI_group_cb(AW_root *aw_root, GBDATA *gb_main) {
    const char *sai_name   = aw_root->awar(AWAR_SAI_NAME)->read_char_pntr();
    const char *group_name = aw_root->awar(AWAR_SAI_GROUP)->read_char_pntr();
    aw_message_if(set_SAI_group(gb_main, sai_name, group_name));
    refresh_grouplist(aw_root);
}

static AW_selection_list *sai_group_sel = NULL;
static void refresh_SAI_groups_cb(AW_root*, GBDATA *gb_main) {
    if (sai_group_sel) fill_SAI_group_selection_list(sai_group_sel, gb_main);
}

static void refresh_group_cb(AW_root *aw_root, GBDATA *gb_main) {
    const char *sai_name   = aw_root->awar(AWAR_SAI_NAME)->read_char_pntr();
    const char *group_name = get_SAI_group(gb_main, sai_name);
    aw_root->awar(AWAR_SAI_GROUP)->write_string(group_name);
}

void NT_create_extendeds_vars(AW_root *aw_root, AW_default aw_def, GBDATA *gb_main) {
    const char *sai_name   = aw_root->awar(AWAR_SAI_NAME)->read_char_pntr();
    const char *group_name = get_SAI_group(gb_main, sai_name);

    aw_root->awar_string(AWAR_SAI_GROUP,          group_name, aw_def)->add_callback(makeRootCallback(set_SAI_group_cb,      gb_main));
    aw_root->awar_int   (AWAR_SAI_REFRESH_GROUPS, 0,          aw_def)->add_callback(makeRootCallback(refresh_SAI_groups_cb, gb_main));
    aw_root->awar       (AWAR_SAI_NAME)                              ->add_callback(makeRootCallback(refresh_group_cb,      gb_main));
}

static AW_window *create_SAI_group_window(AW_root *aw_root) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root, "SAI_GROUP", "Define SAI group");
    aws->load_xfig("ad_ext_group.fig");

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("ad_extended.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->at("sai");
    aws->create_button(NULL, AWAR_SAI_NAME);

    aws->at("group");
    sai_group_sel = awt_create_selection_list_with_input_field(aws, AWAR_SAI_GROUP, "list", "group");
    refresh_grouplist(aw_root);

    return aws;
}

AW_window *NT_create_extendeds_window(AW_root *aw_root) {
    static AW_window_simple *aws = 0;

    if (!aws) {
        aws = new AW_window_simple;
        aws->init(aw_root, "INFO_OF_SAI", "SAI INFORMATION");
        aws->load_xfig("ad_ext.fig");

        aws->at("close");
        aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("help");
        aws->callback(makeHelpCallback("ad_extended.hlp"));
        aws->create_button("HELP", "HELP", "H");

        aws->button_length(13);

        aws->at("delete");
        aws->callback(delete_SAI_cb);
        aws->create_button("DELETE", "DELETE", "D");

        aws->at("rename");
        aws->callback(rename_SAI_cb);
        aws->create_button("RENAME", "RENAME", "R");

        aws->at("copy");
        aws->callback(copy_SAI_cb);
        aws->create_button("COPY", "COPY", "C");

        aws->at("remark");
        aws->callback(edit_SAI_description);
        aws->create_button("EDIT_COMMENT", "EDIT COMMENT", "R");

        aws->at("group");
        aws->callback(create_SAI_group_window);
        aws->create_button("ASSIGN_GROUP", "ASSIGN GROUP", "R");

        aws->at("makespec");
        aws->callback(copy_SAI_to_species_cb);
        aws->create_button("COPY_TO_SPECIES", "COPY TO\nSPECIES", "C");

        aws->at("list");
        awt_create_SAI_selection_list(GLOBAL.gb_main, aws, AWAR_SAI_NAME, true);

        DbScanner *scanner = create_db_scanner(GLOBAL.gb_main, aws, "info", 0, 0, 0, DB_SCANNER, 0, 0, 0, SPECIES_get_selector());
        aws->get_root()->awar(AWAR_SAI_NAME)->add_callback(makeRootCallback(map_SAI_to_scanner, scanner));
    }
    aws->show();
    return aws;
}
