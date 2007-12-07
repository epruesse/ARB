#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <arbdb.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <AW_rename.hxx>
#include "merge.hxx"

// --------------------------------------------------------------------------------

#define AWAR_MERGE_ADDID "tmp/merge1/addid"
#define AWAR_DEST_ADDID  "tmp/merge2/addid"

#define AWAR_ADDID_MATCH   "tmp/merge/addidmatch"
#define AWAR_RENAME_STATUS "tmp/merge/renamestat"
#define AWAR_ALLOW_DUPS    "tmp/merge/allowdups"

// --------------------------------------------------------------------------------

void MG_create_rename_awars(AW_root *aw_root, AW_default aw_def) {
    aw_root->awar_string(AWAR_ADDID_MATCH, "", aw_def);
    aw_root->awar_string(AWAR_RENAME_STATUS, "", aw_def);
    aw_root->awar_int(AWAR_ALLOW_DUPS, 0, aw_def);
}

// --------------------------------------------------------------------------------

static const char *addids_match_info(AW_root *aw_root) {
    char       *addid1 = aw_root->awar(AWAR_MERGE_ADDID)->read_string();
    char       *addid2 = aw_root->awar(AWAR_DEST_ADDID)->read_string();
    const char *result = (strcmp(addid1, addid2) == 0) ? "Ok" : "MISMATCH!";

    free(addid2);
    free(addid1);
    
    return result;
}

static void addids_match_info_refresh_cb(AW_root *aw_root) {
    aw_root->awar(AWAR_ADDID_MATCH)->write_string(addids_match_info(aw_root));
    MG_set_renamed(false, aw_root, "Needed (add.field changed)"); 
}

void MG_create_db_dependent_rename_awars(AW_root *aw_root, GBDATA *gb_merge, GBDATA *gb_dest) {
    static bool created = false;

    if (!created) {
        GB_transaction t1(gb_merge);
        GB_transaction t2(gb_dest);
        GB_ERROR       error = 0;

        // Awars for additional ID need to be mapped, cause they use same db-path in both DBs

        GBDATA     *gb_addid1 = GB_search(gb_merge, AWAR_NAMESERVER_ADDID, GB_STRING);
        GBDATA     *gb_addid2 = GB_search(gb_dest, AWAR_NAMESERVER_ADDID, GB_STRING);
        const char *addid1    = gb_addid1 ? GB_read_char_pntr(gb_addid1) : "";
        const char *addid2    = gb_addid2 ? GB_read_char_pntr(gb_addid2) : "";

        // use other as default (needed e.g. for import)
        if (gb_addid1 && !gb_addid2) {
            gb_addid2 = GB_create(gb_dest, AWAR_NAMESERVER_ADDID, GB_STRING);
            if (!gb_addid2) error = GB_get_error();
            else error = GB_write_string(gb_addid2, addid1);
        }
        else if (!gb_addid1 && gb_addid2) {
            gb_addid1 = GB_create(gb_merge, AWAR_NAMESERVER_ADDID, GB_STRING);
            if (!gb_addid1) error = GB_get_error();
            else error = GB_write_string(gb_addid1, addid2);
        }

        if (!error) {
            AW_awar *awar_addid1 = aw_root->awar_string(AWAR_MERGE_ADDID, "xxx", gb_merge);
            AW_awar *awar_addid2 = aw_root->awar_string(AWAR_DEST_ADDID, "xxx", gb_dest);

            awar_addid1->unmap(); awar_addid1->map(gb_addid1);
            awar_addid2->unmap(); awar_addid2->map(gb_addid2);

            awar_addid1->add_callback(addids_match_info_refresh_cb);
            awar_addid2->add_callback(addids_match_info_refresh_cb);

            addids_match_info_refresh_cb(aw_root);
        }
        
        if (error) {
            aw_message(error);
            t1.abort();
            t2.abort();
        }

        created = true;
    }
}

// --------------------------------------------------------------------------------

static bool was_renamed = false;

void MG_set_renamed(bool renamed, AW_root *aw_root, const char *reason) {
    aw_root->awar(AWAR_RENAME_STATUS)->write_string(reason);
    was_renamed = renamed;
}

GB_ERROR MG_expect_renamed() {
    GB_ERROR error = 0;
    if (!was_renamed) error = "First you have to rename species in both databases";
    return error;
}

// --------------------------------------------------------------------------------

static GB_ERROR renameDB(const char *which, GBDATA *gb_db, bool allowDups) {
    aw_openstatus(GBS_global_string("Generating new names in %s database", which));
    aw_status("Contacting name server");

    bool     isDuplicatesWarning;
    GB_ERROR error = AWTC_pars_names(gb_db, 1, &isDuplicatesWarning);

    if (error) {
        error = GBS_global_string("While renaming %s DB:\n%s", which, error);
        if (isDuplicatesWarning && allowDups) {
            aw_message(error);
            aw_message("Duplicates error ignored. Be careful during merge!");
            error = 0;
        }
    }

    aw_closestatus();
    return error;
}

static void rename_both_databases(AW_window *aww) {
    GB_ERROR  error     = 0;
    AW_root  *aw_root   = aww->get_root();
    char     *match     = aw_root->awar(AWAR_ADDID_MATCH)->read_string();
    bool      allowDups = aw_root->awar(AWAR_ALLOW_DUPS)->read_int();

    if (strcmp(match, "Ok") == 0) {
        error = renameDB("source", gb_merge, allowDups);
        if (!error) error = renameDB("destination", gb_dest, allowDups);
    }
    else {
        error = "Denying rename - additional fields have to match!";
    }
    free(match);

    if (error) {
        aw_message(error);
        MG_set_renamed(false, aw_root, "Failed");
    }
    else {
        MG_set_renamed(true, aw_root, "Ok");
    }
}

AW_window *MG_merge_names_cb(AW_root *awr){
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;
        aws->init( awr, "MERGE_AUTORENAME_SPECIES", "SYNCHRONIZE NAMES");
        aws->load_xfig("merge/names.fig");

        aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE","CLOSE","C");

        aws->at("help");
        aws->callback(AW_POPUP_HELP,(AW_CL)"mg_names.hlp");
        aws->create_button("HELP","HELP","H");

        aws->at("addid1");
        aws->create_input_field(AWAR_MERGE_ADDID, 10);

        aws->at("addid2");
        aws->create_input_field(AWAR_DEST_ADDID, 10);

        aws->at("dups");
        aws->label("Allow merging duplicates (dangerous! see HELP)");
        aws->create_toggle(AWAR_ALLOW_DUPS);

        aws->at("match");
        aws->button_length(12);
        aws->create_button("MATCH", AWAR_ADDID_MATCH, 0, "+");

        aws->at("status");
        aws->button_length(25);
        aws->label("Status:");
        aws->create_button("STATUS", AWAR_RENAME_STATUS, 0, "+");

        aws->at("rename");
        aws->callback(rename_both_databases);
        aws->create_autosize_button("RENAME_DATABASES","Rename species");
        
        aws->button_length(0);
        aws->shadow_width(1);
        aws->at("icon");
        aws->callback(AW_POPUP_HELP,(AW_CL)"mg_names.hlp");
        aws->create_button("HELP_MERGE", "#merge/icon.bitmap");
    }
    return aws;
}
