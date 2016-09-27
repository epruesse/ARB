// =============================================================== //
//                                                                 //
//   File      : MG_names.cxx                                      //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "merge.hxx"

#include <AW_rename.hxx>
#include <aw_root.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <aw_window.hxx>
#include <arb_progress.h>

// --------------------------------------------------------------------------------

#define AWAR_ADDID_SRC  AWAR_MERGE_TMP_SRC "addid"
#define AWAR_ADDID_DST  AWAR_MERGE_TMP_DST "addid"

#define AWAR_ADDID_MATCH   AWAR_MERGE_TMP "addidmatch"
#define AWAR_RENAME_STATUS AWAR_MERGE_TMP "renamestat"
#define AWAR_ALLOW_DUPS    AWAR_MERGE_TMP "allowdups"
#define AWAR_OVERRIDE      AWAR_MERGE_TMP "override"

// --------------------------------------------------------------------------------

void MG_create_rename_awars(AW_root *aw_root, AW_default aw_def) {
    aw_root->awar_string(AWAR_ADDID_MATCH, "", aw_def);
    aw_root->awar_string(AWAR_RENAME_STATUS, "", aw_def);
    aw_root->awar_int(AWAR_ALLOW_DUPS, 0, aw_def);
    aw_root->awar_int(AWAR_OVERRIDE, 0, aw_def);
}

// --------------------------------------------------------------------------------

static const char *addids_match_info(AW_root *aw_root) {
    char *src_addid = aw_root->awar(AWAR_ADDID_SRC)->read_string();
    char *dst_addid = aw_root->awar(AWAR_ADDID_DST)->read_string();

    const char *result = (strcmp(src_addid, dst_addid) == 0) ? "Ok" : "MISMATCH!";

    free(dst_addid);
    free(src_addid);

    return result;
}

static void addids_match_info_refresh_cb(AW_root *aw_root) {
    aw_root->awar(AWAR_ADDID_MATCH)->write_string(addids_match_info(aw_root));
    MG_set_renamed(false, aw_root, "Needed (add.field changed)");
}

void MG_create_db_dependent_rename_awars(AW_root *aw_root, GBDATA *gb_src, GBDATA *gb_dst) {
    static bool created = false;

    if (!created) {
        GB_transaction src_ta(gb_src);
        GB_transaction dst_ta(gb_dst);
        GB_ERROR       error = 0;

        // Awars for additional ID need to be mapped, cause they use same db-path in both DBs

        GBDATA *gb_src_addid = GB_search(gb_src, AWAR_NAMESERVER_ADDID, GB_STRING);
        GBDATA *gb_dst_addid = GB_search(gb_dst, AWAR_NAMESERVER_ADDID, GB_STRING);

        const char *src_addid = gb_src_addid ? GB_read_char_pntr(gb_src_addid) : "";
        const char *dst_addid = gb_dst_addid ? GB_read_char_pntr(gb_dst_addid) : "";

        // use other as default (needed e.g. for import)
        if (gb_src_addid && !gb_dst_addid) {
            gb_dst_addid             = GB_create(gb_dst, AWAR_NAMESERVER_ADDID, GB_STRING);
            if (!gb_dst_addid) error = GB_await_error();
            else error               = GB_write_string(gb_dst_addid, src_addid);
        }
        else if (!gb_src_addid && gb_dst_addid) {
            gb_src_addid             = GB_create(gb_src, AWAR_NAMESERVER_ADDID, GB_STRING);
            if (!gb_src_addid) error = GB_await_error();
            else error               = GB_write_string(gb_src_addid, dst_addid);
        }

        if (!error) {
            AW_awar *awar_src_addid = aw_root->awar_string(AWAR_ADDID_SRC, "xxx", gb_src);
            AW_awar *awar_dst_addid = aw_root->awar_string(AWAR_ADDID_DST, "xxx", gb_dst);

            awar_src_addid->unmap(); awar_src_addid->map(gb_src_addid);
            awar_dst_addid->unmap(); awar_dst_addid->map(gb_dst_addid);

            awar_src_addid->add_callback(addids_match_info_refresh_cb);
            awar_dst_addid->add_callback(addids_match_info_refresh_cb);

            addids_match_info_refresh_cb(aw_root);
        }

        if (error) {
            error = src_ta.close(error);
            error = dst_ta.close(error);
            aw_message(error);
        }
        else {
        created = true;
    }
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
    if (!was_renamed) error = "First you have to synchronize species IDs in both databases";
    return error;
}

// --------------------------------------------------------------------------------

static GB_ERROR renameDB(const char *which, GBDATA *gb_db, bool allowDups) {
    arb_progress progress(GBS_global_string("Generating new names in %s database", which));
    bool         isDuplicatesWarning;
    GB_ERROR     error = AWTC_pars_names(gb_db, &isDuplicatesWarning);

    if (error) {
        error = GBS_global_string("While renaming %s DB:\n%s", which, error);
        if (isDuplicatesWarning && allowDups) {
            aw_message(error);
            aw_message("Duplicates error ignored. Be careful during merge!");
            error = 0;
        }
    }

    return error;
}

static void rename_both_databases(AW_window *aww) {
    GB_ERROR  error     = 0;
    AW_root  *aw_root   = aww->get_root();
    char     *match     = aw_root->awar(AWAR_ADDID_MATCH)->read_string();
    bool      allowDups = aw_root->awar(AWAR_ALLOW_DUPS)->read_int();

    if (strcmp(match, "Ok") == 0) {
        error = renameDB("source", GLOBAL_gb_src, allowDups);
        if (!error) error = renameDB("destination", GLOBAL_gb_dst, allowDups);
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

static void override_toggle_cb(AW_root *aw_root) {
    bool override = aw_root->awar(AWAR_OVERRIDE)->read_int();
    MG_set_renamed(override, aw_root, override ? "Overridden" : "Not renamed");
}

AW_window *MG_create_merge_names_window(AW_root *awr) {
    AW_window_simple *aws = new AW_window_simple;

    aws->init(awr, "MERGE_AUTORENAME_SPECIES", "Synchronize IDs");
    aws->load_xfig("merge/names.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("mg_names.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->at("addid1");
    aws->create_input_field(AWAR_ADDID_SRC, 10);

    aws->at("addid2");
    aws->create_input_field(AWAR_ADDID_DST, 10);

    aws->at("dups");
    aws->label("Allow merging duplicates (dangerous! see HELP)");
    aws->create_toggle(AWAR_ALLOW_DUPS);

    aws->at("override");
    aws->label("Override (even more dangerous! see HELP)");
    awr->awar(AWAR_OVERRIDE)->add_callback(override_toggle_cb);
    aws->create_toggle(AWAR_OVERRIDE);

    aws->at("match");
    aws->button_length(12);
    aws->create_button(0, AWAR_ADDID_MATCH, 0, "+");

    aws->at("status");
    aws->button_length(25);
    aws->label("Status:");
    aws->create_button(0, AWAR_RENAME_STATUS, 0, "+");

    aws->at("rename");
    aws->callback(rename_both_databases);
    aws->create_autosize_button("RENAME_DATABASES", "Synchronize");

    aws->button_length(0);
    aws->shadow_width(1);
    aws->at("icon");
    aws->callback(makeHelpCallback("mg_names.hlp"));
    aws->create_button("HELP_MERGE", "#merge/icon.xpm");

    return aws;
}
