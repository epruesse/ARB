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
#include "MG_ali_admin.hxx"

#include <awt_sel_boxes.hxx>
#include <aw_root.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <arbdbt.h>
#include <arb_strarray.h>
#include <AliAdmin.h>

#include <unistd.h>

#define AWAR_ALI_SRC AWAR_MERGE_TMP_SRC "alignment_name"
#define AWAR_ALI_DST AWAR_MERGE_TMP_DST "alignment_name"

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

static AliAdmin *get_ali_admin(int db_nr) {
    static AliAdmin *admin[3] = { NULL, NULL, NULL };
    if (!admin[db_nr]) {
        admin[db_nr] = new AliAdmin(db_nr, get_gb_main(db_nr));
    }
    return admin[db_nr];
}

AW_window *MG_create_merge_alignment_window(AW_root *awr) {
    AW_window_simple *aws = new AW_window_simple;

    awr->awar(AWAR_ALI_SRC)->add_callback(makeRootCallback(MG_alignment_vars_callback, get_ali_admin(1))); // @@@ ntree version does this in NT_create_alignment_vars 
    awr->awar(AWAR_ALI_DST)->add_callback(makeRootCallback(MG_alignment_vars_callback, get_ali_admin(2)));

    aws->init(awr, "MERGE_ALIGNMENTS", "MERGE ALIGNMENTS");
    aws->load_xfig("merge/alignment.fig");

    aws->at("close"); aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("mg_alignment.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->at("check");
    aws->callback(makeWindowCallback(copy_and_check_alignments_ignoreResult));
    aws->create_button("CHECK", "Check");

    aws->at("ali1");
    awt_create_selection_list_on_alignments(GLOBAL_gb_src, aws, AWAR_ALI_SRC, "*=");

    aws->at("ali2");
    awt_create_selection_list_on_alignments(GLOBAL_gb_dst, aws, AWAR_ALI_DST, "*=");

    aws->at("modify1");
    aws->callback(makeCreateWindowCallback(MG_create_alignment_window, 1));
    aws->create_button("MODIFY_DB1", "MODIFY");

    aws->at("modify2");
    aws->callback(makeCreateWindowCallback(MG_create_alignment_window, 2));
    aws->create_button("MODIFY_DB2", "MODIFY");


    aws->button_length(0);
    aws->shadow_width(1);
    aws->at("icon");
    aws->callback(makeHelpCallback("mg_alignment.hlp"));
    aws->create_button("HELP_MERGE", "#merge/icon.xpm");

    return aws;
}
