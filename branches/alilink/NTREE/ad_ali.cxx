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
#include <arb_global_defs.h>
#include <AliAdmin.h>

#define AWAR_ALI_SELECT AWAR_DEFAULT_ALIGNMENT
#define AWAR_ALI_DEST   "presets/alignment_dest"
#define AWAR_ALI_TYPE   "presets/alignment_type"
#define AWAR_ALI_LEN    "presets/alignment_len"
#define AWAR_ALIGNED    "presets/aligned"
#define AWAR_SECURITY   "presets/security"
#define AWAR_ALI_REM    "presets/alignment_rem"
#define AWAR_ALI_AUTO   "presets/auto_format"

// ---------------------------
//      @@@ sync 108273910263

static void alignment_vars_callback(AW_root *aw_root, AliAdmin *admin) {
    nt_assert(!GB_have_error());

    GBDATA *gb_main = admin->get_gb_main();

    GB_transaction ta(gb_main);

    char    *use = aw_root->awar(AWAR_ALI_SELECT)->read_string();
    GBDATA *ali_cont = GBT_get_alignment(gb_main, use);

    if (!ali_cont) {
        GB_clear_error();
        aw_root->awar(AWAR_ALI_TYPE)->unmap();
        aw_root->awar(AWAR_ALI_LEN)->unmap();
        aw_root->awar(AWAR_ALIGNED)->unmap();
        aw_root->awar(AWAR_SECURITY)->unmap();
        aw_root->awar(AWAR_ALI_REM)->unmap();
        aw_root->awar(AWAR_ALI_AUTO)->unmap();
    }
    else {
        GBDATA *ali_len         = GB_search(ali_cont, "alignment_len",            GB_INT);
        GBDATA *ali_aligned     = GB_search(ali_cont, "aligned",                  GB_INT);
        GBDATA *ali_type        = GB_search(ali_cont, "alignment_type",           GB_STRING);
        GBDATA *ali_security    = GB_search(ali_cont, "alignment_write_security", GB_INT);
        GBDATA *ali_rem         = GB_search(ali_cont, "alignment_rem",            GB_STRING);
        GBDATA *ali_auto_format = GB_search(ali_cont, "auto_format",              GB_INT);

        aw_root->awar(AWAR_ALI_TYPE)->map(ali_type);
        aw_root->awar(AWAR_ALI_LEN) ->map(ali_len);
        aw_root->awar(AWAR_ALIGNED) ->map(ali_aligned);
        aw_root->awar(AWAR_SECURITY)->map(ali_security);
        aw_root->awar(AWAR_ALI_REM) ->map(ali_rem);
        aw_root->awar(AWAR_ALI_AUTO)->map(ali_auto_format);
    }
    free(use);

    nt_assert(!GB_have_error());
}

static void create_alignment_vars(AW_root *aw_root, AW_default aw_def, AliAdmin *admin) {

    AW_awar *awar_ali_select = aw_root->awar(AWAR_ALI_SELECT);

    aw_root->awar_string(AWAR_ALI_DEST, "", aw_def)->set_srt(GBT_ALI_AWAR_SRT);
    aw_root->awar_string(AWAR_ALI_TYPE, "", aw_def);
    aw_root->awar_string(AWAR_ALI_REM,  "", aw_def);

    aw_root->awar_int(AWAR_ALI_LEN,  0, aw_def);
    aw_root->awar_int(AWAR_ALIGNED,  0, aw_def);
    aw_root->awar_int(AWAR_SECURITY, 0, aw_def);
    aw_root->awar_int(AWAR_ALI_AUTO, 0, aw_def);

    RootCallback rcb = makeRootCallback(alignment_vars_callback, admin);
    awar_ali_select->add_callback(rcb);
    rcb(aw_root);
}

static void delete_ali_cb(AW_window *aww, AliAdmin *admin) {
    if (aw_ask_sure("delete_ali_data", "Are you sure to delete all data belonging to this alignment?")) {
        GBDATA *gb_main = admin->get_gb_main();
        char   *source  = aww->get_root()->awar(AWAR_ALI_SELECT)->read_string();
        {
            GB_transaction ta(gb_main);
            GB_ERROR       error = GBT_rename_alignment(gb_main, source, 0, 0, 1);

            error = ta.close(error);
            if (error) aw_message(error);
        }
        free(source);
    }
}

static void ali_checklen_cb(AW_window *aww, AliAdmin *admin) {
    GBDATA *gb_main = admin->get_gb_main();
    char   *use     = aww->get_root()->awar(AWAR_ALI_SELECT)->read_string();

    GB_transaction ta(gb_main);

    GB_ERROR error = GBT_check_data(gb_main, use);
    error          = ta.close(error);

    aw_message_if(error);
    free(use);
}

static void never_auto_format_ali_genom_cb(AW_window *aww) {
    AW_root *awr = aww->get_root();
    char    *use = awr->awar(AWAR_ALI_SELECT)->read_string();
    if (strcmp(use, "ali_genom") == 0) {
        awr->awar(AWAR_ALI_AUTO)->write_int(2); // ali_genom is always forced to "skip"
    }
}

static void ali_format_cb(AW_window *aww, AliAdmin *admin) {
    char     *use = aww->get_root()->awar(AWAR_ALI_SELECT)->read_string();
    GB_begin_transaction(GLOBAL.gb_main);
    GB_ERROR  err = ARB_format_alignment(GLOBAL.gb_main, use);
    GB_commit_transaction(GLOBAL.gb_main);
    if (err) aw_message(err);
    free(use);
    ali_checklen_cb(aww, admin);
}

// -----------------------------
//      @@@ sync 0273492431

static void copy_rename_cb(AW_window *aww, AliAdmin *admin, CopyRenameMode mode) {
    nt_assert(!GB_have_error());
    GBDATA *gb_main = admin->get_gb_main();

    AW_root *awr    = aww->get_root();
    char    *source = awr->awar(AWAR_ALI_SELECT)->read_string();
    char    *dest   = awr->awar(AWAR_ALI_DEST)->read_string();

    GB_ERROR error = GB_begin_transaction(gb_main);

    if (!error) error = GBT_rename_alignment(gb_main, source, dest, 1, mode == CRM_RENAME);
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

static AW_window *create_alignment_copy_window(AW_root *root, AliAdmin *admin) {
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
    aws->callback(makeWindowCallback(copy_rename_cb, admin, CRM_COPY));
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}

static AW_window *create_alignment_rename_window(AW_root *root, AliAdmin *admin) {
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
    aws->callback(makeWindowCallback(copy_rename_cb, admin, CRM_RENAME));
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}

static void create_alignment_cb(AW_window *aww, AliAdmin *admin) {
    GBDATA   *gb_main = admin->get_gb_main();
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

static AW_window *create_alignment_create_window(AW_root *root, AliAdmin *admin) {
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
    aws->callback(makeWindowCallback(create_alignment_cb, admin));
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}

// AISC_MKPT_PROMOTE:class AliAdmin;

AW_window *NT_create_AliAdmin_window(AW_root *root, AliAdmin *admin) {
    if (!admin->get_window()) {
        GBDATA           *gb_main = admin->get_gb_main();
        AW_window_simple *aws     = new AW_window_simple;

        admin->store_window(aws);

        create_alignment_vars(root, AW_ROOT_DEFAULT, admin);

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
        aws->callback(makeWindowCallback(delete_ali_cb, admin));
        aws->create_button("DELETE", "DELETE", "D");

        aws->at("rename");
        aws->callback(makeCreateWindowCallback(create_alignment_rename_window, admin));
        aws->create_button("RENAME", "RENAME", "R");

        aws->at("create");
        aws->callback(makeCreateWindowCallback(create_alignment_create_window, admin));
        aws->create_button("CREATE", "CREATE", "N");

        aws->at("copy");
        aws->callback(makeCreateWindowCallback(create_alignment_copy_window, admin));
        aws->create_button("COPY", "COPY", "C");

        aws->at("check_len");
        aws->callback(makeWindowCallback(ali_checklen_cb, admin));
        aws->create_button("CHECK_LEN", "CHECK LEN", "L");

        aws->at("align");
        aws->callback(makeWindowCallback(ali_format_cb, admin));
        aws->create_button("FORMAT", "FORMAT", "F");

        // ali selection list
        aws->at("list");
        awt_create_selection_list_on_alignments(gb_main, aws, AWAR_ALI_SELECT, "*=");

        // alignment settings
        aws->at("aligned");
        aws->create_option_menu(AWAR_ALIGNED, true);
        aws->callback(makeWindowCallback(ali_checklen_cb, admin)); aws->insert_default_option("not formatted", "n", 0);
        aws->callback(makeWindowCallback(ali_format_cb, admin));   aws->insert_option("formatted", "j", 1);
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
        aws->callback(makeWindowCallback(ali_checklen_cb, admin));
        aws->create_option_menu(AWAR_SECURITY, true);
        aws->insert_option("0", "0", 0);
        aws->insert_option("1", "1", 1);
        aws->insert_option("2", "2", 2);
        aws->insert_option("3", "3", 3);
        aws->insert_option("4", "4", 4);
        aws->insert_option("5", "5", 5);
        aws->insert_default_option("6", "6", 6);
        aws->update_option_menu();

        aws->at("auto_format");
        aws->callback(never_auto_format_ali_genom_cb);
        aws->create_option_menu(AWAR_ALI_AUTO, true);
        aws->insert_default_option("ask", "a", 0);
        aws->insert_option("always", "", 1);
        aws->insert_option("never", "", 2);
        aws->update_option_menu();

        aws->at("rem");
        aws->create_text_field(AWAR_ALI_REM);
    }

    return admin->get_window();
}

