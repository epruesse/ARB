// =============================================================== //
//                                                                 //
//   File      : ad_ali.cxx                                        //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "NT_local.h"
#include <insdel.h>
#include <awt_sel_boxes.hxx>
#include <aw_root.hxx>
#include <aw_question.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <aw_awar_defs.hxx>
#include <arbdbt.h>

#define AWAR_ALI_NAME "presets/alignment_name"
#define AWAR_ALI_DEST "presets/alignment_dest"
#define AWAR_ALI_TYPE "presets/alignment_type"
#define AWAR_ALI_LEN  "presets/alignment_len"
#define AWAR_ALI_REM  "presets/alignment_rem"
#define AWAR_ALI_AUTO "presets/auto_format"
#define AWAR_ALIGNED  "presets/aligned"
#define AWAR_SECURITY "presets/security"

// ---------------------------
//      @@@ sync 108273910263

static void alignment_vars_callback(AW_root *aw_root) {
    nt_assert(!GB_have_error());

    GBDATA         *gb_main = GLOBAL.gb_main;
    GB_transaction  ta(gb_main);

    char    *use = aw_root->awar(AWAR_DEFAULT_ALIGNMENT)->read_string();
    GBDATA *ali_cont = GBT_get_alignment(gb_main, use);

    if (!ali_cont) {
        GB_clear_error();
        aw_root->awar(AWAR_ALI_NAME)->unmap();
        aw_root->awar(AWAR_ALI_TYPE)->unmap();
        aw_root->awar(AWAR_ALI_LEN)->unmap();
        aw_root->awar(AWAR_ALIGNED)->unmap();
        aw_root->awar(AWAR_SECURITY)->unmap();
        aw_root->awar(AWAR_ALI_REM)->unmap();
        aw_root->awar(AWAR_ALI_AUTO)->unmap();
    }
    else {
        GBDATA *ali_name        = GB_search(ali_cont, "alignment_name",           GB_STRING);
        GBDATA *ali_len         = GB_search(ali_cont, "alignment_len",            GB_INT);
        GBDATA *ali_aligned     = GB_search(ali_cont, "aligned",                  GB_INT);
        GBDATA *ali_type        = GB_search(ali_cont, "alignment_type",           GB_STRING);
        GBDATA *ali_security    = GB_search(ali_cont, "alignment_write_security", GB_INT);
        GBDATA *ali_rem         = GB_search(ali_cont, "alignment_rem",            GB_STRING);
        GBDATA *ali_auto_format = GB_search(ali_cont, "auto_format",              GB_INT);

        aw_root->awar(AWAR_ALI_NAME)->map(ali_name);
        aw_root->awar(AWAR_ALI_TYPE)->map(ali_type);
        aw_root->awar(AWAR_ALI_LEN) ->map(ali_len);
        aw_root->awar(AWAR_ALIGNED)       ->map(ali_aligned);
        aw_root->awar(AWAR_SECURITY)      ->map(ali_security);
        aw_root->awar(AWAR_ALI_REM) ->map(ali_rem);
        aw_root->awar(AWAR_ALI_AUTO)   ->map(ali_auto_format);
    }
    free(use);

    nt_assert(!GB_have_error());
}

void NT_create_alignment_vars(AW_root *aw_root, AW_default aw_def) {
    AW_awar *awar_def_ali = aw_root->awar_string(AWAR_DEFAULT_ALIGNMENT, "", aw_def);
    GB_push_transaction(GLOBAL.gb_main);

    GBDATA *use = GB_search(GLOBAL.gb_main, AWAR_DEFAULT_ALIGNMENT, GB_STRING);
    awar_def_ali->map(use);

    aw_root->awar_string(AWAR_ALI_NAME, "", aw_def) ->set_srt(GBT_ALI_AWAR_SRT);
    aw_root->awar_string(AWAR_ALI_DEST, "", aw_def) ->set_srt(GBT_ALI_AWAR_SRT);
    aw_root->awar_string(AWAR_ALI_TYPE, "", aw_def);
    aw_root->awar_string(AWAR_ALI_REM,  "", aw_def);

    aw_root->awar_int(AWAR_ALI_LEN,  0, aw_def);
    aw_root->awar_int(AWAR_ALIGNED,  0, aw_def);
    aw_root->awar_int(AWAR_SECURITY, 0, aw_def);
    aw_root->awar_int(AWAR_ALI_AUTO, 0, aw_def);

    awar_def_ali->add_callback(alignment_vars_callback);
    alignment_vars_callback(aw_root);
    GB_pop_transaction(GLOBAL.gb_main);
}

static void ad_al_delete_cb(AW_window *aww) {
    if (aw_ask_sure("delete_ali_data", "Are you sure to delete all data belonging to this alignment")) {
        GBDATA *gb_main = GLOBAL.gb_main;
        char *source = aww->get_root()->awar(AWAR_DEFAULT_ALIGNMENT)->read_string();
        {
            GB_transaction ta(gb_main);
            GB_ERROR       error = GBT_rename_alignment(gb_main, source, 0, 0, 1);

            error = ta.close(error);
            if (error) aw_message(error);
        }
        free(source);
    }
}

static void ed_al_check_len_cb(AW_window *aww) {
    GBDATA *gb_main = GLOBAL.gb_main;
    char   *use     = aww->get_root()->awar(AWAR_DEFAULT_ALIGNMENT)->read_string();

    GB_transaction ta(gb_main);

    GB_ERROR error = GBT_check_data(gb_main, use);
    error          = ta.close(error);

    aw_message_if(error);
    free(use);
}

static void ed_al_check_auto_format(AW_window *aww) {
    AW_root *awr = aww->get_root();
    char    *use = awr->awar(AWAR_DEFAULT_ALIGNMENT)->read_string();
    if (strcmp(use, "ali_genom") == 0) {
        awr->awar(AWAR_ALI_AUTO)->write_int(2); // ali_genom is always forced to "skip"
    }
}

static void ed_al_align_cb(AW_window *aww) {
    char     *use = aww->get_root()->awar(AWAR_DEFAULT_ALIGNMENT)->read_string();
    GB_begin_transaction(GLOBAL.gb_main);
    GB_ERROR  err = ARB_format_alignment(GLOBAL.gb_main, use);
    GB_commit_transaction(GLOBAL.gb_main);
    if (err) aw_message(err);
    free(use);
    ed_al_check_len_cb(aww);
}

// -----------------------------
//      @@@ sync 0273492431

static void aa_copy_delete_rename(AW_window *aww, int copy, int dele) {
    nt_assert(!GB_have_error());
    GBDATA *gb_main = GLOBAL.gb_main;

    AW_root *awr    = aww->get_root();
    char    *source = awr->awar(AWAR_DEFAULT_ALIGNMENT)->read_string();
    char    *dest   = awr->awar(AWAR_ALI_DEST)->read_string();

    GB_ERROR error = GB_begin_transaction(gb_main);

    if (!error) error = GBT_rename_alignment(gb_main, source, dest, copy, dele);
    if (!error) {
        char *nfield = GBS_global_string_copy("%s/data", dest);
        error        = GBT_add_new_changekey(gb_main, nfield, GB_STRING);
        free(nfield);
    }

    error = GB_end_transaction(gb_main, error);
    aww->hide_or_notify(error);

    free(source);
    free(dest);
    nt_assert(!GB_have_error());
}

static AW_window *create_alignment_copy_window(AW_root *root) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "COPY_ALIGNMENT", "ALIGNMENT COPY");
    aws->load_xfig("ad_al_si.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the new name\nof the alignment");

    aws->at("input");
    aws->create_input_field(AWAR_ALI_DEST, 15);

    aws->at("ok");
    aws->callback(makeWindowCallback(aa_copy_delete_rename, 1, 0));
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}
static AW_window *create_alignment_rename_window(AW_root *root) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "RENAME_ALIGNMENT", "ALIGNMENT RENAME");
    aws->load_xfig("ad_al_si.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the name\nof the new alignment");

    aws->at("input");
    aws->create_input_field(AWAR_ALI_DEST, 15);

    aws->at("ok");
    aws->callback(makeWindowCallback(aa_copy_delete_rename, 1, 1));
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}

static void aa_create_alignment(AW_window *aww) {
    GBDATA   *gb_main = GLOBAL.gb_main;
    GB_ERROR  error   = GB_begin_transaction(gb_main);
    if (!error) {
        char   *name         = aww->get_root()->awar(AWAR_ALI_DEST)->read_string();
        GBDATA *gb_alignment = GBT_create_alignment(gb_main, name, 0, 0, 0, "dna");

        if (!gb_alignment) error = GB_await_error();
        else {
            char *nfield = GBS_global_string_copy("%s/data", name);
            error        = GBT_add_new_changekey(gb_main, nfield, GB_STRING);
            free(nfield);
        }
        free(name);
    }
    GB_end_transaction_show_error(gb_main, error, aw_message);
}

static AW_window *create_alignment_create_window(AW_root *root) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "CREATE_ALIGNMENT", "ALIGNMENT CREATE");
    aws->load_xfig("ad_al_si.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the new name\nof the alignment");

    aws->at("input");
    aws->create_input_field(AWAR_ALI_DEST, 15);

    aws->at("ok");
    aws->callback(makeWindowCallback(aa_create_alignment));
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}

AW_window *NT_create_alignment_window(AW_root *root, AW_window *aw_popmedown) {
    // if 'aw_popmedown' points to a window, that window is popped down
    if (aw_popmedown) aw_popmedown->hide();

    static AW_window_simple *aws = 0;
    if (!aws) {
        GBDATA *gb_main = GLOBAL.gb_main;
        aws             = new AW_window_simple;

        aws->init(root, "INFO_OF_ALIGNMENT", "ALIGNMENT INFORMATION");

        aws->load_xfig("ad_align.fig"); // @@@ use same fig?

        aws->callback((AW_CB0)AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(makeHelpCallback("ad_align.hlp"));
        aws->at("help");
        aws->create_button("HELP", "HELP", "H");

        // button column
        aws->button_length(13);

        aws->at("delete");
        aws->callback(ad_al_delete_cb);
        aws->create_button("DELETE", "DELETE", "D");

        aws->at("rename");
        aws->callback(create_alignment_rename_window);
        aws->create_button("RENAME", "RENAME", "R");

        aws->at("create");
        aws->callback(create_alignment_create_window);
        aws->create_button("CREATE", "CREATE", "N");

        aws->at("copy");
        aws->callback(create_alignment_copy_window);
        aws->create_button("COPY", "COPY", "C");

        aws->at("check_len");
        aws->callback(ed_al_check_len_cb);
        aws->create_button("CHECK_LEN", "CHECK LEN", "L");

        aws->at("align");
        aws->callback(ed_al_align_cb);
        aws->create_button("FORMAT", "FORMAT", "F");

        // ali selection list
        aws->at("list");
        awt_create_selection_list_on_alignments(gb_main, aws, AWAR_DEFAULT_ALIGNMENT, "*=");

        // alignment settings
        aws->at("aligned");
        aws->create_option_menu(AWAR_ALIGNED, true);
        aws->callback(ed_al_check_len_cb); aws->insert_default_option("not formatted", "n", 0);
        aws->callback(ed_al_align_cb);     aws->insert_option("formatted", "j", 1);
        aws->update_option_menu();

        aws->at("len");
        aws->create_input_field(AWAR_ALI_LEN, 8);

        aws->at("type");
        aws->create_option_menu(AWAR_ALI_TYPE, true);
        aws->insert_option("dna", "d", "dna");
        aws->insert_option("rna", "r", "rna");
        aws->insert_option("pro", "p", "ami");
        aws->insert_default_option("???", "?", "usr");
        aws->update_option_menu();

        aws->at("security");
        aws->create_option_menu(AWAR_SECURITY, true);
        aws->callback(ed_al_check_len_cb); aws->insert_option("0", "0", 0);
        aws->callback(ed_al_check_len_cb); aws->insert_option("1", "1", 1);
        aws->callback(ed_al_check_len_cb); aws->insert_option("2", "2", 2);
        aws->callback(ed_al_check_len_cb); aws->insert_option("3", "3", 3);
        aws->callback(ed_al_check_len_cb); aws->insert_option("4", "4", 4);
        aws->callback(ed_al_check_len_cb); aws->insert_option("5", "5", 5);
        aws->callback(ed_al_check_len_cb); aws->insert_default_option("6", "6", 6);
        aws->update_option_menu();

        aws->at("auto_format");
        aws->create_option_menu(AWAR_ALI_AUTO, true);
        aws->callback(ed_al_check_auto_format); aws->insert_default_option("ask", "a", 0);
        aws->callback(ed_al_check_auto_format); aws->insert_option("always", "", 1);
        aws->callback(ed_al_check_auto_format); aws->insert_option("never", "", 2);
        aws->update_option_menu();

        aws->at("rem");
        aws->create_text_field(AWAR_ALI_REM);
    }

    return aws;
}
