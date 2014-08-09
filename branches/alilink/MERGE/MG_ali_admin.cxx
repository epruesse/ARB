// =============================================================== //
//                                                                 //
//   File      : MG_ali_admin.cxx                                  //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "merge.hxx" // @@@ unwanted, needed e.g. for get_gb_main

#include <awt_sel_boxes.hxx>
#include <aw_root.hxx>
#include <aw_question.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>

#include <arbdbt.h>
#include <arb_global_defs.h>

#define AWAR_ALI_NAME(db_nr) awar_name_tmp(db_nr, "alignment_name")
#define AWAR_ALI_DEST(db_nr) awar_name_tmp(db_nr, "alignment_dest")
#define AWAR_ALI_TYPE(db_nr) awar_name_tmp(db_nr, "alignment_type")
#define AWAR_ALI_LEN(db_nr)  awar_name_tmp(db_nr, "alignment_len")
#define AWAR_ALIGNED(db_nr)  awar_name_tmp(db_nr, "aligned")
#define AWAR_SECURITY(db_nr) awar_name_tmp(db_nr, "security")

// ---------------------------
//      @@@ sync 108273910263

void MG_alignment_vars_callback(AW_root *aw_root, int db_nr) {
    mg_assert(!GB_have_error());

    GBDATA         *gb_main = get_gb_main(db_nr);
    GB_transaction  ta(gb_main);

    char   *use      = aw_root->awar(AWAR_ALI_NAME(db_nr))->read_string();
    GBDATA *ali_cont = GBT_get_alignment(gb_main, use);

    if (!ali_cont) {
        GB_clear_error();
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

    mg_assert(!GB_have_error());
}

void MG_create_alignment_awars(AW_root *aw_root, AW_default aw_def) {
    for (int db_nr = 1; db_nr <= 2; ++db_nr) {
        aw_root->awar_string(AWAR_ALI_NAME(db_nr), NO_ALI_SELECTED, aw_def) ->set_srt(GBT_ALI_AWAR_SRT);
        aw_root->awar_string(AWAR_ALI_DEST(db_nr), "",              aw_def) ->set_srt(GBT_ALI_AWAR_SRT);
        aw_root->awar_string(AWAR_ALI_TYPE(db_nr), "",              aw_def);

        aw_root->awar_int(AWAR_ALI_LEN (db_nr), 0, aw_def);
        aw_root->awar_int(AWAR_ALIGNED (db_nr), 0, aw_def);
        aw_root->awar_int(AWAR_SECURITY(db_nr), 0, aw_def);
    }

    // @@@ nt-admin adds change-callback to ali-selection-list awar (and calls it once)
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

// -----------------------------
//      @@@ sync 0273492431

static void MG_copy_delete_rename(AW_window * aww, int db_nr, int dele) {
    mg_assert(!GB_have_error());
    GBDATA *gb_main = get_gb_main(db_nr);

    AW_root *awr    = aww->get_root();
    char    *source = awr->awar(AWAR_ALI_NAME(db_nr))->read_string();
    char    *dest   = awr->awar(AWAR_ALI_DEST(db_nr))->read_string();

    GB_ERROR error = GB_begin_transaction(gb_main);

    const int copy    = 1;
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
    mg_assert(!GB_have_error());
}

static AW_window *create_alignment_copy_window(AW_root *root, int db_nr) {
    AW_window_simple *aws = new AW_window_simple;
    {
        char header[80];
        sprintf(header, "ALIGNMENT COPY %i", db_nr);
        aws->init(root, header, header);
    }
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

static AW_window *MG_create_alignment_rename_window(AW_root *root, int db_nr) {
    AW_window_simple *aws = new AW_window_simple;
    {
        char header[80];
        sprintf(header, "ALIGNMENT RENAME %i", db_nr);
        aws->init(root, header, header);
    }
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
    GBDATA   *gb_main = get_gb_main(db_nr);
    GB_ERROR  error   = GB_begin_transaction(gb_main);
    if (!error) {
        char     *name         = aww->get_root()->awar(AWAR_ALI_DEST(db_nr))->read_string();
        GBDATA   *gb_alignment = GBT_create_alignment(gb_main, name, 0, 0, 0, "dna");

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

static AW_window *MG_create_alignment_create_window(AW_root *root, int db_nr) {
    AW_window_simple *aws = new AW_window_simple;
    {
        char header[80];
        sprintf(header, "ALIGNMENT CREATE %i", db_nr);
        aws->init(root, header, header);
    }
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

AW_window *MG_create_alignment_window(AW_root *root, int db_nr) {
    mg_assert(db_nr>=1 && db_nr<=2);

    static AW_window_simple *aws_exists[3] = { NULL, NULL, NULL };
    if (!aws_exists[db_nr]) {
        GBDATA           *gb_main = get_gb_main(db_nr);
        AW_window_simple *aws     = new AW_window_simple;

        aws_exists[db_nr] = aws;

        char header[80];
        sprintf(header, "ALIGNMENT CONTROL %i", db_nr);
        aws->init(root, header, header);

        aws->load_xfig("merge/ad_align.fig"); // @@@ use same fig?

        aws->callback((AW_CB0)AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(makeHelpCallback("ad_align.hlp"));
        aws->at("help");
        aws->create_button("HELP", "HELP", "H");

        // button column
        aws->button_length(13);

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

        // ali selection list
        aws->at("list");
        awt_create_selection_list_on_alignments(gb_main, aws, AWAR_ALI_NAME(db_nr), "*=");

        // alignment settings
        aws->at("aligned");
        aws->create_option_menu(AWAR_ALIGNED(db_nr), true);
        aws->insert_default_option("not justified", "n", 0);
        aws->insert_option("justified", "j", 1);
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
    }
    return aws_exists[db_nr];
}

