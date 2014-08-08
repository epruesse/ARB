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
#include <aw_question.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <arbdbt.h>
#include <arb_strarray.h>

#include <unistd.h>

#define AWAR_ALI_NAME(db_nr) awar_name_tmp(db_nr, "alignment_name")
#define AWAR_ALI_DEST(db_nr) awar_name_tmp(db_nr, "alignment_dest")
#define AWAR_ALI_TYPE(db_nr) awar_name_tmp(db_nr, "alignment_type")
#define AWAR_ALI_LEN(db_nr)  awar_name_tmp(db_nr, "alignment_len")
#define AWAR_ALIGNED(db_nr)  awar_name_tmp(db_nr, "aligned")
#define AWAR_SECURITY(db_nr) awar_name_tmp(db_nr, "security")

#define AWAR_ALI_SRC AWAR_MERGE_TMP_SRC "alignment_name"
#define AWAR_ALI_DST AWAR_MERGE_TMP_DST "alignment_name"

static void MG_alignment_vars_callback(AW_root *aw_root, int db_nr) {
    GBDATA         *gb_main = get_gb_main(db_nr);
    GB_transaction  ta(gb_main);

    char   *use      = aw_root->awar(AWAR_ALI_NAME(db_nr))->read_string();
    GBDATA *ali_cont = GBT_get_alignment(gb_main, use);

    if (!ali_cont) {
        aw_root->awar(AWAR_ALI_TYPE(db_nr))->unmap();
        aw_root->awar(AWAR_ALI_LEN (db_nr))->unmap();
        aw_root->awar(AWAR_ALIGNED (db_nr))->unmap();
        aw_root->awar(AWAR_SECURITY(db_nr))->unmap();
    }
    else {
        GBDATA *ali_len      = GB_entry(ali_cont, "alignment_len");
        GBDATA *ali_aligned  = GB_entry(ali_cont, "aligned");
        GBDATA *ali_type     = GB_entry(ali_cont, "alignment_type");
        GBDATA *ali_security = GB_entry(ali_cont, "alignment_write_security");

        aw_root->awar(AWAR_ALI_TYPE(db_nr))->map(ali_type);
        aw_root->awar(AWAR_ALI_LEN (db_nr))->map(ali_len);
        aw_root->awar(AWAR_ALIGNED (db_nr))->map(ali_aligned);
        aw_root->awar(AWAR_SECURITY(db_nr))->map(ali_security);

    }
    free(use);
}


void MG_create_alignment_awars(AW_root *aw_root, AW_default aw_def) {
    for (int db_nr = 1; db_nr <= 2; ++db_nr) {
        aw_root->awar_string(AWAR_ALI_NAME(db_nr), "", aw_def);
        aw_root->awar_string(AWAR_ALI_DEST(db_nr), "", aw_def);
        aw_root->awar_string(AWAR_ALI_TYPE(db_nr), "", aw_def);
        aw_root->awar_int   (AWAR_ALI_LEN (db_nr), 0,  aw_def);
        aw_root->awar_int   (AWAR_ALIGNED (db_nr), 0,  aw_def);
        aw_root->awar_int   (AWAR_SECURITY(db_nr), 0,  aw_def);
    }
}

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

static void MG_ad_al_delete_cb(AW_window *aww, int db_nr) {
    if (aw_ask_sure("merge_delete_ali", "Are you sure to delete all data belonging to this alignment?")) {
        GBDATA *gb_main = get_gb_main(db_nr);
        char   *source  = aww->get_root()->awar(AWAR_ALI_NAME(db_nr))->read_string();
        {
            GB_transaction ta(gb_main);
            GB_ERROR       error = GBT_rename_alignment(gb_main, source, 0, 0, 1);

            error = ta.close(error);
            if (error) aw_message(error);
        }
        free(source);
    }
}


static void MG_ed_al_check_len_cb(AW_window *aww, int db_nr) {
    GBDATA *gb_main = get_gb_main(db_nr);
    char   *use     = aww->get_root()->awar(AWAR_ALI_NAME(db_nr))->read_string();

    GB_transaction ta(gb_main);

    GB_ERROR error = GBT_check_data(gb_main, use);
    error          = ta.close(error);

    aw_message_if(error);
    free(use);
}

static void MG_copy_delete_rename(AW_window * aww, int db_nr, int dele) {
    GBDATA *gb_main = get_gb_main(db_nr);

    AW_root *awr    = aww->get_root();
    char    *source = awr->awar(AWAR_ALI_NAME(db_nr))->read_string();
    char    *dest   = awr->awar(AWAR_ALI_DEST(db_nr))->read_string();

    GB_ERROR error = GB_begin_transaction(gb_main);

    if (!error) error = GBT_rename_alignment(gb_main, source, dest, (int)1, (int)dele);
    if (!error) error = GBT_add_new_changekey(gb_main, GBS_global_string("%s/data", dest), GB_STRING);

    error = GB_end_transaction(gb_main, error);
    aww->hide_or_notify(error);

    free(source);
    free(dest);
}


static AW_window *create_alignment_copy_window(AW_root *root, int db_nr)
{
    AW_window_simple *aws = new AW_window_simple;
    char header[80];
    sprintf(header, "ALIGNMENT COPY %i", db_nr);
    aws->init(root, header, header);
    aws->load_xfig("ad_al_si.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the new name\nof the alignment");

    aws->at("input");
    aws->create_input_field(AWAR_ALI_DEST(db_nr), 15);

    aws->at("ok");
    aws->callback(makeWindowCallback(MG_copy_delete_rename, db_nr, 0));
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}
static AW_window *MG_create_alignment_rename_window(AW_root *root, int db_nr)
{
    AW_window_simple *aws = new AW_window_simple;
    char header[80];
    sprintf(header, "ALIGNMENT RENAME %i", db_nr);
    aws->init(root, header, header);
    aws->load_xfig("ad_al_si.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the name\nof the new alignment");

    aws->at("input");
    aws->create_input_field(AWAR_ALI_DEST(db_nr), 15);

    aws->at("ok");
    aws->callback(makeWindowCallback(MG_copy_delete_rename, db_nr, 1));
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}

static void MG_aa_create_alignment(AW_window *aww, int db_nr) {
    GBDATA   *gb_main      = get_gb_main(db_nr);
    char     *name         = aww->get_root()->awar(AWAR_ALI_DEST(db_nr))->read_string();
    GB_ERROR  error        = GB_begin_transaction(gb_main);
    GBDATA   *gb_alignment = GBT_create_alignment(gb_main, name, 0, 0, 0, "dna");

    if (!gb_alignment) error = GB_await_error();
    GB_end_transaction_show_error(gb_main, error, aw_message);
    free(name);
}

static AW_window *MG_create_alignment_create_window(AW_root *root, int db_nr) {
    AW_window_simple *aws = new AW_window_simple;
    char header[80];
    sprintf(header, "ALIGNMENT CREATE %i", db_nr);
    aws->init(root, header, header);
    aws->load_xfig("ad_al_si.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the new name\nof the alignment");

    aws->at("input");
    aws->create_input_field(AWAR_ALI_DEST(db_nr), 15);

    aws->at("ok");
    aws->callback(makeWindowCallback(MG_aa_create_alignment, db_nr));
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}



static AW_window *MG_create_alignment_window(AW_root *root, int db_nr) {
    GBDATA           *gb_main = get_gb_main(db_nr);
    AW_window_simple *aws = new AW_window_simple;
    char              header[80];

    sprintf(header, "ALIGNMENT CONTROL %i", db_nr);
    aws->init(root, header, header);
    aws->load_xfig("merge/ad_align.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(makeHelpCallback("ad_align.hlp"));
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    aws->button_length(13);

    aws->at("list");
    awt_create_selection_list_on_alignments(gb_main, aws, AWAR_ALI_NAME(db_nr), "*=");

    aws->at("delete");
    aws->callback(makeWindowCallback(MG_ad_al_delete_cb, db_nr));
    aws->create_button("DELETE", "DELETE", "D");

    aws->at("rename");
    aws->callback(makeCreateWindowCallback(MG_create_alignment_rename_window, db_nr));
    aws->create_button("RENAME", "RENAME", "R");

    aws->at("create");
    aws->callback(makeCreateWindowCallback(MG_create_alignment_create_window, db_nr));
    aws->create_button("CREATE", "CREATE", "N");

    aws->at("copy");
    aws->callback(makeCreateWindowCallback(create_alignment_copy_window, db_nr));
    aws->create_button("COPY", "COPY", "C");

    aws->at("aligned");
    aws->create_option_menu(AWAR_ALIGNED(db_nr), true);
    aws->insert_option("justified", "j", 1);
    aws->insert_default_option("not justified", "n", 0);
    aws->update_option_menu();

    aws->at("len");
    aws->create_input_field(AWAR_ALI_LEN(db_nr), 8);

    aws->at("type");
    aws->create_option_menu(AWAR_ALI_TYPE(db_nr), true);
    aws->insert_option("dna", "d", "dna");
    aws->insert_option("rna", "r", "rna");
    aws->insert_option("pro", "p", "ami");
    aws->insert_default_option("???", "?", "usr");
    aws->update_option_menu();

    aws->at("security");
    aws->callback(makeWindowCallback(MG_ed_al_check_len_cb, db_nr));
    aws->create_option_menu(AWAR_SECURITY(db_nr), true);
    aws->insert_option("0", "0", 0);
    aws->insert_option("1", "1", 1);
    aws->insert_option("2", "2", 2);
    aws->insert_option("3", "3", 3);
    aws->insert_option("4", "4", 4);
    aws->insert_option("5", "5", 5);
    aws->insert_default_option("6", "6", 6);
    aws->update_option_menu();

    return aws;
}

AW_window *MG_create_merge_alignment_window(AW_root *awr) {
    AW_window_simple *aws = new AW_window_simple;

    awr->awar(AWAR_ALI_SRC)->add_callback(makeRootCallback(MG_alignment_vars_callback, 1));
    awr->awar(AWAR_ALI_DST)->add_callback(makeRootCallback(MG_alignment_vars_callback, 2));

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
