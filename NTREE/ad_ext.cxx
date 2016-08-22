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
#include <arbdbt.h>
#include <arb_strbuf.h>

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

static char *getExistingSAIgroups() {
    // scan SAIs for existing groups.
    // return a string of ';'-separated group names (or NULL)

    GB_HASH       *groups = GBS_create_hash(GBT_get_SAI_count(GLOBAL.gb_main), GB_MIND_CASE);
    GBS_strstruct *out    = GBS_stropen(1000);
    int            count  = 0;
    GB_transaction ta(GLOBAL.gb_main);

    for (GBDATA *gb_sai = GBT_first_SAI(GLOBAL.gb_main); gb_sai; gb_sai = GBT_next_SAI(gb_sai)) {
        const char *group = GBT_read_char_pntr(gb_sai, "sai_group");
        if (group && !GBS_read_hash(groups, group)) {
            GBS_strcat(out, group);
            GBS_chrcat(out, ';');
            GBS_write_hash(groups, group, 1);
            count++;
        }
    }

    char *result = 0;
    if (count>0) {
        GBS_str_cut_tail(out, 1); // truncate final ';'
        result = GBS_strclose(out);
    }
    else {
        GBS_strforget(out);
    }
    GBS_free_hash(groups);

    return result;
}

static void assign_SAI_to_group(AW_window *aww) {
    AW_root  *awr      = aww->get_root();
    char     *sai_name = awr->awar(AWAR_SAI_NAME)->read_string();
    GB_ERROR  error    = 0;

    if (!sai_name || !sai_name[0]) error = "No SAI selected";
    else {
        GBDATA *gb_sai;
        {
            GB_transaction ta(GLOBAL.gb_main);
            gb_sai = GBT_find_SAI(GLOBAL.gb_main, sai_name);
        }

        if (!gb_sai) error = GBS_global_string("SAI '%s' not found", sai_name);
        else {
            bool        has_group = true;
            const char *group     = GBT_read_char_pntr(gb_sai, "sai_group");
            if (!group) {
                group     = "default_group";
                has_group = false;
            }

            char *new_group;
            {
                char *existingGroups = getExistingSAIgroups();
                if (existingGroups) {
                    new_group = aw_string_selection("Assign SAI to group", "Enter group name:", group, existingGroups, NULL);
                    free(existingGroups);
                }
                else {
                    new_group = aw_input("Assign SAI to group", "Enter group name:", group);
                }
            }

            if (new_group) {
                GB_transaction t2(GLOBAL.gb_main);
                if (new_group[0]) error = GBT_write_string(gb_sai, "sai_group", new_group);
                else if (has_group) {
                    GBDATA *gb_group = GB_entry(gb_sai, "sai_group");
                    if (gb_group) error = GB_delete(gb_group);
                }
            }
        }
    }

    if (error) aw_message(error);
    free(sai_name);
}

AW_window *NT_create_extendeds_window(AW_root *aw_root)
{
    static AW_window_simple *aws = 0;

    if (!aws) {
        aws = new AW_window_simple;
        aws->init(aw_root, "INFO_OF_SAI", "SAI INFORMATION");
        aws->load_xfig("ad_ext.fig");

        aws->callback((AW_CB0)AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(makeHelpCallback("ad_extended.hlp"));
        aws->at("help");
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
        aws->callback(assign_SAI_to_group);
        aws->create_button("ASSIGN_GROUP", "ASSIGN GROUP", "R");

        aws->at("makespec");
        aws->callback(copy_SAI_to_species_cb);
        aws->create_button("COPY_TO_SPECIES", "COPY TO\nSPECIES", "C");

        aws->at("list");
        awt_create_selection_list_on_sai(GLOBAL.gb_main, aws, AWAR_SAI_NAME, true);

        DbScanner *scanner = create_db_scanner(GLOBAL.gb_main, aws, "info", 0, 0, 0, DB_SCANNER, 0, 0, 0, SPECIES_get_selector());
        aws->get_root()->awar(AWAR_SAI_NAME)->add_callback(makeRootCallback(map_SAI_to_scanner, scanner));
    }
    aws->show();
    return aws;
}
