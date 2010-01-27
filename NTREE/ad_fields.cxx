// =============================================================== //
//                                                                 //
//   File      : ad_fields.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <arbdbt.h>
#include <awt.hxx>

extern GBDATA *GLOBAL_gb_main;

void fields_vars_callback(AW_root *aw_root) {
    GB_push_transaction(gb_main);
    char    *use = aw_root->awar("tmp/fields/use")->read_string();
    GBDATA *ali_cont = GBT_get_fields(gb_main, use);
    if (!ali_cont) {
        aw_root->awar("tmp/fields/fields_name")->unmap();
        aw_root->awar("tmp/fields/fields_father")->unmap();
        aw_root->awar("tmp/fields/fields_type")->unmap();
        aw_root->awar("tmp/fields/fields_rem")->unmap();
        aw_root->awar("tmp/fields/security")->unmap();
    }
    else {
        GBDATA *ali_name     = GB_search(ali_cont, "fields_name",           GB_STRING);
        GBDATA *ali_father   = GB_search(ali_cont, "fields_father",         GB_STRING);
        GBDATA *ali_type     = GB_search(ali_cont, "fields_type",           GB_INT);
        GBDATA *ali_security = GB_search(ali_cont, "fields_write_security", GB_INT);
        GBDATA *ali_rem      = GB_search(ali_cont, "fields_rem",            GB_STRING);

        aw_root->awar("tmp/fields/fields_name")->map((void*)ali_name);
        aw_root->awar("tmp/fields/fields_type")->map((void*)ali_type);
        aw_root->awar("tmp/fields/fields_father")->map((void*)ali_father);
        aw_root->awar("tmp/fields/fields_rem")->map((void*)ali_rem);
        aw_root->awar("tmp/fields/security")->map((void*)ali_security);
    }
    GB_pop_transaction(gb_main);
    free(use);
}

void create_fields_vars(AW_root *aw_root) {
    GB_push_transaction(gb_main);

    aw_root->awar_string("tmp/fields/use",         "", aw_def);
    aw_root->awar_string("tmp/fields/fields_name", "", aw_def);
    aw_root->awar_string("tmp/fields/fields_rem");
    aw_root->awar_string("tmp/fields/fields_father", "", aw_def);
    aw_root->awar_int   ("tmp/fields/fields_type",   "", aw_def);
    aw_root->awar_int   ("tmp/fields/aligned",       0,  aw_def);
    aw_root->awar_int   ("tmp/fields/security",      0,  aw_def);

    aw_root->awar("tmp/fields/use")->add_callback(fields_vars_callback);
    fields_vars_callback(aw_root);
    GB_pop_transaction(gb_main);
}

AW_window *create_fields_window(AW_root *root, AW_default aw_def)
{
    AWUSE(aw_def);
    create_fields_vars(root);
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "INFO_OF_FIELD", "FIELDS INFORMATION", 100, 0);
    aws->load_xfig("ad_align.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(AW_POPUP_HELP, (AW_CL)"ad_align.hlp");
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    aws->button_length(13);

    aws->at("delete");
    aws->callback(ad_al_delete_cb);
    aws->create_button("DELETE", "DELETE", "D");

    aws->at("rename");
    aws->callback(AW_POPUP, (AW_CL)create_fields_rename_window, 0);
    aws->create_button("RENAME", "RENAME", "R");

    aws->at("create");
    aws->callback(AW_POPUP, (AW_CL)create_fields_create_window, 0);
    aws->create_button("CREATE", "CREATE", "N");

    aws->at("copy");
    aws->callback(AW_POPUP, (AW_CL)create_fields_copy_window, 0);
    aws->create_button("COPY", "COPY", "C");

    aws->at("check_len");
    aws->callback(ed_al_check_len_cb);
    aws->create_button("CHECK_LEN", "CHECK LEN", "L");

    aws->at("align");
    aws->callback(ed_al_align_cb);
    aws->create_button("FORMAT", "FORMAT", "F");

    aws->at("list");
    awt_create_selection_list_on_ad(gb_main, (AW_window *)aws, "tmp/fields/use", "*=");

    aws->at("aligned");
    aws->create_option_menu("tmp/fields/aligned");
    aws->callback(ed_al_check_len_cb); aws->insert_default_option("not formatted", "n", 0);
    aws->callback(ed_al_align_cb);     aws->insert_option        ("formatted",     "j", 1);
    aws->update_option_menu();

    aws->at("len");
    aws->create_input_field("tmp/fields/fields_len", 5);

    aws->at("type");
    aws->create_option_menu("tmp/fields/fields_type");
    aws->insert_option("dna", "d", "dna");
    aws->insert_option("rna", "r", "rna");
    aws->insert_option("pro", "p", "ami");
    aws->insert_default_option("???", "?", "usr");
    aws->update_option_menu();

    aws->at("security");
    aws->create_option_menu("tmp/fields/security");
    aws->callback(ed_al_check_len_cb); aws->insert_option("0", "0", 0);
    aws->callback(ed_al_check_len_cb); aws->insert_option("1", "1", 1);
    aws->callback(ed_al_check_len_cb); aws->insert_option("2", "2", 2);
    aws->callback(ed_al_check_len_cb); aws->insert_option("3", "3", 3);
    aws->callback(ed_al_check_len_cb); aws->insert_option("4", "4", 4);
    aws->callback(ed_al_check_len_cb); aws->insert_option("5", "5", 5);
    aws->callback(ed_al_check_len_cb); aws->insert_default_option("6", "6", 6);
    aws->update_option_menu();

    aws->at("rem");
    aws->create_text_field("tmp/fields/fields_rem");


    return (AW_window *)aws;
}
