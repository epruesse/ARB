// =============================================================== //
//                                                                 //
//   File      : MG_ali_admin.cxx                                  //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <awt_sel_boxes.hxx>
#include <aw_root.hxx>
#include <aw_question.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <arbdbt.h>
#include <AliAdmin.h>

static void alignment_vars_callback(AW_root *, AliAdmin *admin) {
    ali_assert(!GB_have_error());

    GBDATA         *gb_main  = admin->get_gb_main();
    GB_transaction  ta(gb_main);
    GBDATA         *ali_cont = GBT_get_alignment(gb_main, admin->get_selected_ali());

    if (!ali_cont) {
        GB_clear_error();
        admin->type_awar()->unmap();
        admin->len_awar()->unmap();
        admin->aligned_awar()->unmap();
        admin->security_awar()->unmap();
    }
    else {
        GBDATA *ali_len      = GB_entry(ali_cont, "alignment_len");
        GBDATA *ali_aligned  = GB_entry(ali_cont, "aligned");
        GBDATA *ali_type     = GB_entry(ali_cont, "alignment_type");
        GBDATA *ali_security = GB_entry(ali_cont, "alignment_write_security");

        admin->type_awar()    ->map(ali_type);
        admin->len_awar()     ->map(ali_len);
        admin->aligned_awar() ->map(ali_aligned);
        admin->security_awar()->map(ali_security);
    }

    ali_assert(!GB_have_error());
}

static void create_admin_awars(AW_root *aw_root, AW_default aw_def, AliAdmin *admin) {
    aw_root->awar_string(admin->dest_name(), "", aw_def)->set_srt(GBT_ALI_AWAR_SRT);
    aw_root->awar_string(admin->type_name(), "", aw_def);

    aw_root->awar_int(admin->len_name(),      0, aw_def);
    aw_root->awar_int(admin->aligned_name(),  0, aw_def);
    aw_root->awar_int(admin->security_name(), 0, aw_def);

    RootCallback rcb = makeRootCallback(alignment_vars_callback, admin);
    admin->select_awar()->add_callback(rcb);
    rcb(aw_root);
}

static void delete_ali_cb(AW_window *, AliAdmin *admin) {
    if (aw_ask_sure("delete_ali_data", "Are you sure to delete all data belonging to this alignment?")) {
        GBDATA         *gb_main = admin->get_gb_main();
        GB_transaction  ta(gb_main);
        GB_ERROR        error   = GBT_rename_alignment(gb_main, admin->get_selected_ali(), 0, 0, 1);

        error = ta.close(error);
        if (error) aw_message(error);
    }
}

static void ali_checklen_cb(AW_window *, AliAdmin *admin) {
    GBDATA         *gb_main = admin->get_gb_main();
    GB_transaction  ta(gb_main);
    GB_ERROR        error   = GBT_check_data(gb_main, admin->get_selected_ali());

    error = ta.close(error);
    aw_message_if(error);
}

static void copy_rename_cb(AW_window *aww, AliAdmin *admin, CopyRenameMode mode) {
    ali_assert(!GB_have_error());

    GBDATA     *gb_main = admin->get_gb_main();
    const char *source  = admin->get_selected_ali();
    const char *dest    = admin->get_dest_ali();
    GB_ERROR    error   = GB_begin_transaction(gb_main);

    if (!error) error = GBT_rename_alignment(gb_main, source, dest, 1, mode == CRM_RENAME);
    if (!error) {
        char *nfield = GBS_global_string_copy("%s/data", dest);
        error        = GBT_add_new_changekey(gb_main, nfield, GB_STRING);
        free(nfield);
    }

    error = GB_end_transaction(gb_main, error);
    aww->hide_or_notify(error);

    ali_assert(!GB_have_error());
}

static AW_window *create_alignment_copy_window(AW_root *, AliAdmin *admin) {
    AW_window_simple *aws = new AW_window_simple;
    admin->window_init(aws, "COPY_%s", "Copy %s");
    aws->load_xfig("ad_al_si.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the new name\nof the alignment");

    aws->at("input");
    aws->create_input_field(admin->dest_name(), 15);

    aws->at("ok");
    aws->callback(makeWindowCallback(copy_rename_cb, admin, CRM_COPY));
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}

static AW_window *create_alignment_rename_window(AW_root *, AliAdmin *admin) {
    AW_window_simple *aws = new AW_window_simple;
    admin->window_init(aws, "RENAME_%s", "Rename %s");
    aws->load_xfig("ad_al_si.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the name\nof the new alignment");

    aws->at("input");
    aws->create_input_field(admin->dest_name(), 15);

    aws->at("ok");
    aws->callback(makeWindowCallback(copy_rename_cb, admin, CRM_RENAME));
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}

static void create_alignment_cb(AW_window *, AliAdmin *admin) {
    GBDATA   *gb_main = admin->get_gb_main();
    GB_ERROR  error   = GB_begin_transaction(gb_main);
    if (!error) {
        const char *name   = admin->get_dest_ali();
        GBDATA     *gb_ali = GBT_create_alignment(gb_main, name, 0, 0, 0, "dna");

        if (!gb_ali) error = GB_await_error();
        else {
            char *nfield = GBS_global_string_copy("%s/data", name);
            error        = GBT_add_new_changekey(gb_main, nfield, GB_STRING);
            free(nfield);
        }
    }
    GB_end_transaction_show_error(gb_main, error, aw_message);
}

static AW_window *create_alignment_create_window(AW_root *, AliAdmin *admin) {
    AW_window_simple *aws = new AW_window_simple;
    admin->window_init(aws, "CREATE_%s", "Create %s");
    aws->load_xfig("ad_al_si.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the new name\nof the alignment");

    aws->at("input");
    aws->create_input_field(admin->dest_name(), 15);

    aws->at("ok");
    aws->callback(makeWindowCallback(create_alignment_cb, admin));
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}

AW_window *MG_create_AliAdmin_window(AW_root *root, AliAdmin *admin) {
    if (!admin->get_window()) {
        GBDATA           *gb_main = admin->get_gb_main();
        AW_window_simple *aws     = new AW_window_simple;

        create_admin_awars(root, AW_ROOT_DEFAULT, admin);
        admin->store_window(aws);

        admin->window_init(aws, "INFO_OF_%s", "%s information");
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

        // ali selection list
        aws->at("list");
        awt_create_selection_list_on_alignments(gb_main, aws, admin->select_name(), "*=");

        // alignment settings
        aws->at("aligned");
        aws->create_option_menu(admin->aligned_name(), true);
        aws->insert_default_option("not formatted", "n", 0);
        aws->insert_option("formatted", "j", 1);
        aws->update_option_menu();

        aws->at("len");
        aws->create_input_field(admin->len_name(), 8);

        aws->at("type");
        aws->create_option_menu(admin->type_name(), true);
        aws->insert_option("dna", "d", "dna");
        aws->insert_option("rna", "r", "rna");
        aws->insert_option("pro", "p", "ami");
        aws->insert_default_option("???", "?", "usr");
        aws->update_option_menu();

        aws->at("security");
        aws->callback(makeWindowCallback(ali_checklen_cb, admin));
        aws->create_option_menu(admin->security_name(), true);
        aws->insert_option("0", "0", 0);
        aws->insert_option("1", "1", 1);
        aws->insert_option("2", "2", 2);
        aws->insert_option("3", "3", 3);
        aws->insert_option("4", "4", 4);
        aws->insert_option("5", "5", 5);
        aws->insert_default_option("6", "6", 6);
        aws->update_option_menu();
    }

    return admin->get_window();
}

