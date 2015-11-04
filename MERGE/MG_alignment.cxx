// =============================================================== //
//                                                                 //
//   File      : MG_alignment.cxx                                  //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "merge.hxx"

#include <awt_sel_boxes.hxx>
#include <aw_root.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <arbdbt.h>
#include <arb_strarray.h>
#include <arb_global_defs.h>
#include <AliAdmin.h>

#include <unistd.h>

static void copy_and_check_alignments_ignoreResult() { MG_copy_and_check_alignments(); }
int MG_copy_and_check_alignments() {
    // returns 0 if alignments are ok for merging.
    // otherwise an error message is shown in message box.
    // checks types and names.

    GB_ERROR error = NULL;

    GB_begin_transaction(GLOBAL_gb_dst);
    GB_begin_transaction(GLOBAL_gb_src);

    ConstStrArray names;
    GBT_get_alignment_names(names, GLOBAL_gb_src);

    GBDATA *gb_dst_presets = NULL;

    for (int i = 0; names[i] && !error; ++i) {
        const char *name       = names[i];
        GBDATA     *gb_dst_ali = GBT_get_alignment(GLOBAL_gb_dst, name);

        if (!gb_dst_ali) {
            GB_clear_error(); // ignore "alignment not found"

            GBDATA *gb_src_ali = GBT_get_alignment(GLOBAL_gb_src, name);
            mg_assert(gb_src_ali);

            if (!gb_dst_presets) gb_dst_presets = GBT_get_presets(GLOBAL_gb_dst);

            gb_dst_ali = GB_create_container(gb_dst_presets, "alignment");
            GB_copy(gb_dst_ali, gb_src_ali);
            GBT_add_new_changekey(GLOBAL_gb_dst, (char *)GBS_global_string("%s/data", name), GB_STRING);
        }

        char *src_type = GBT_get_alignment_type_string(GLOBAL_gb_src, name);
        char *dst_type = GBT_get_alignment_type_string(GLOBAL_gb_dst, name);

        if (strcmp(src_type, dst_type) != 0) {
            error = GBS_global_string("The alignments '%s' have different types (%s != %s)", name, src_type, dst_type);
        }
        free(dst_type);
        free(src_type);
    }

    GB_commit_transaction(GLOBAL_gb_dst);
    GB_commit_transaction(GLOBAL_gb_src);

    if (error) aw_message(error);

    return !!error;
}

static void bindAdmin(AW_window *aws, const char *at_ali, const char *at_modify, const char *button_id, AdminType type) {
    const char *awarname_select[] = {
        NULL,
        AWAR_MERGE_TMP_SRC "alignment_name",
        AWAR_MERGE_TMP_DST "alignment_name"
    };
    const char *awarbase[] = {
        NULL,
        AWAR_MERGE_TMP_SRC,
        AWAR_MERGE_TMP_DST
    };

    AW_root *aw_root = aws->get_root();
    GBDATA  *gb_main = get_gb_main(type);

    aw_root->awar_string(awarname_select[type], NO_ALI_SELECTED, AW_ROOT_DEFAULT);
    AliAdmin *const admin = new AliAdmin(type, gb_main, awarname_select[type], awarbase[type]); // do not free (bound to callbacks)

    aws->at(at_ali);
    awt_create_ALI_selection_list(gb_main, aws, awarname_select[type], "*=");

    aws->at(at_modify);
    aws->callback(makeCreateWindowCallback(ALI_create_admin_window, admin));
    aws->create_button(button_id, "MODIFY");
}

AW_window *MG_create_merge_alignment_window(AW_root *awr) {
    AW_window_simple *aws = new AW_window_simple;

    aws->init(awr, "MERGE_ALIGNMENTS", "MERGE ALIGNMENTS");
    aws->load_xfig("merge/alignment.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("mg_alignment.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->at("check");
    aws->callback(makeWindowCallback(copy_and_check_alignments_ignoreResult));
    aws->create_button("CHECK", "Check");

    bindAdmin(aws, "ali1", "modify1", "MODIFY_DB1", SOURCE_ADMIN);
    bindAdmin(aws, "ali2", "modify2", "MODIFY_DB2", TARGET_ADMIN);

    aws->button_length(0);
    aws->shadow_width(1);
    aws->at("icon");
    aws->callback(makeHelpCallback("mg_alignment.hlp"));
    aws->create_button("HELP_MERGE", "#merge/icon.xpm");

    return aws;
}
