#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
// #include <malloc.h>
#include <string.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>

#include <arbdb.h>
#include <arbdbt.h>
#include <awt.hxx>
extern GBDATA *gb_main;


void alignment_vars_callback(AW_root *aw_root)
{
    GB_push_transaction(gb_main);
    char    *use = aw_root->awar("presets/use")->read_string();
    GBDATA *ali_cont = GBT_get_alignment(gb_main,use);
    if (!ali_cont) {
        aw_root->awar("presets/alignment_name")->unmap();
        aw_root->awar("presets/alignment_type")->unmap();
        aw_root->awar("presets/alignment_len")->unmap();
        aw_root->awar("presets/alignment_rem")->unmap();
        aw_root->awar("presets/aligned")->unmap();
        aw_root->awar("presets/auto_format")->unmap();
        aw_root->awar("presets/security")->unmap();
    }else{

        GBDATA *ali_name        = GB_search(ali_cont, "alignment_name",           GB_STRING);
        GBDATA *ali_len         = GB_search(ali_cont, "alignment_len",            GB_INT   );
        GBDATA *ali_aligned     = GB_search(ali_cont, "aligned",                  GB_INT   );
        GBDATA *ali_auto_format = GB_search(ali_cont, "auto_format",              GB_INT   );
        GBDATA *ali_type        = GB_search(ali_cont, "alignment_type",           GB_STRING);
        GBDATA *ali_security    = GB_search(ali_cont, "alignment_write_security", GB_INT   );
        GBDATA *ali_rem         = GB_search(ali_cont, "alignment_rem",            GB_STRING);

        aw_root->awar("presets/alignment_name")->map((void*)ali_name);
        aw_root->awar("presets/alignment_type")->map((void*)ali_type);
        aw_root->awar("presets/alignment_len")->map((void*)ali_len);
        aw_root->awar("presets/alignment_rem")->map((void*)ali_rem);
        aw_root->awar("presets/aligned")->map((void*)ali_aligned);
        aw_root->awar("presets/auto_format")->map((void*)ali_auto_format);
        aw_root->awar("presets/security")->map((void*)ali_security);
    }
    GB_pop_transaction(gb_main);
    free(use);
}

void NT_create_alignment_vars(AW_root *aw_root,AW_default aw_def)
{
    aw_root->awar_string( "presets/use", "" ,   aw_def);
    GB_push_transaction(gb_main);

    GBDATA *use = GB_search(gb_main,"presets/use",GB_STRING);
    aw_root->awar("presets/use")->map(use);

    aw_root->awar_string( "presets/alignment_name", "" , aw_def) ->set_srt( GBT_ALI_AWAR_SRT);
    aw_root->awar_string( "presets/alignment_dest", "" , aw_def) ->set_srt( GBT_ALI_AWAR_SRT);

    aw_root->awar_string( "presets/alignment_type", "", aw_def);
    aw_root->awar_string( "presets/alignment_rem" );
    aw_root->awar_int( "presets/alignment_len", 0, aw_def);
    aw_root->awar_int( "presets/aligned", 0, aw_def);
    aw_root->awar_int( "presets/auto_format", 0, aw_def);
    aw_root->awar_int( "presets/security", 0, aw_def);

    aw_root->awar("presets/use")->add_callback( alignment_vars_callback);
    alignment_vars_callback(aw_root);
    GB_pop_transaction(gb_main);
}

void ad_al_delete_cb(AW_window *aww)
{
    if (aw_message("Are you sure to delete all data belonging to this alignment","OK,CANCEL"))return;

    GB_ERROR error = 0;
    char *source = aww->get_root()->awar("presets/use")->read_string();

    GB_begin_transaction(gb_main);

    error = GBT_rename_alignment(gb_main,source,0,0,1);

    if (!error) GB_commit_transaction(gb_main);
    else    GB_abort_transaction(gb_main);

    if (error) aw_message(error);
    free(source);
}


void ed_al_check_auto_format(AW_window *aww) {
    AW_root *awr = aww->get_root();
    char    *use = awr->awar("presets/use")->read_string();
    if (strcmp(use, "ali_genom") == 0) {
        awr->awar("presets/auto_format")->write_int(2); // ali_genom is always forced to "skip"
    }
}

void ed_al_check_len_cb(AW_window *aww)
{
    char *error = 0;
    char *use = aww->get_root()->awar("presets/use")->read_string();
    GB_begin_transaction(gb_main);
    if (!error) error = (char *)GBT_check_data(gb_main,use);
    GB_commit_transaction(gb_main);
    if (error) aw_message(error);
    free(use);
}
void ed_al_export_sec_cb(AW_window *aww)
{
    aw_message("This Function is not implemented,\nPlease press 'CHECK' to do this");
    AWUSE(aww);
}
void ed_al_align_cb(AW_window *aww)
{
    char     *use = aww->get_root()->awar("presets/use")->read_string();
    GB_begin_transaction(gb_main);
    GB_ERROR  err = GBT_format_alignment(gb_main, use);
    GB_commit_transaction(gb_main);
    if (err) aw_message(err);
    free(use);
    ed_al_check_len_cb(aww);
}

void aa_copy_delete_rename(AW_window *aww,AW_CL copy, AW_CL dele)
{
    GB_ERROR error = 0;
    char *source = aww->get_root()->awar("presets/use")->read_string();
    char *dest = aww->get_root()->awar("presets/alignment_dest")->read_string();

    GB_begin_transaction(gb_main);

    error = GBT_rename_alignment(gb_main,source,dest,(int)copy,(int)dele);

    if (!error){
        char *nfield = GBS_global_string_copy("%s/data",dest);
        awt_add_new_changekey( gb_main,nfield,GB_STRING);
        free(nfield);
        GB_commit_transaction(gb_main);
    }else{
        GB_abort_transaction(gb_main);
    }
    if (error) aw_message(error);
    free(source);
    free(dest);
    aww->hide();
}

AW_window *create_alignment_copy_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "COPY_ALIGNMENT", "ALIGNMENT COPY");
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label");
    aws->create_autosize_button(0,"Please enter the new name\nof the alignment");

    aws->at("input");
    aws->create_input_field("presets/alignment_dest",15);

    aws->at("ok");
    aws->callback(aa_copy_delete_rename,1,0);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}
AW_window *create_alignment_rename_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "RENAME_ALIGNMENT", "ALIGNMENT RENAME" );
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label");
    aws->create_autosize_button(0,"Please enter the name\nof the new alignment");

    aws->at("input");
    aws->create_input_field("presets/alignment_dest",15);

    aws->at("ok");
    aws->callback(aa_copy_delete_rename,1,1);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}

void aa_create_alignment(AW_window *aww)
{
    GB_ERROR error = 0;
    GBDATA *gb_alignment;
    char *name = aww->get_root()->awar("presets/alignment_dest")->read_string();
    GB_begin_transaction(gb_main);

    gb_alignment = GBT_create_alignment(gb_main,name,0,0,0,"dna");
    if (!gb_alignment) error = GB_get_error();

    if (!error){
        char *nfield = strdup(GBS_global_string("%s/data",name));
        awt_add_new_changekey( gb_main,nfield,GB_STRING);
        free(nfield);
        GB_commit_transaction(gb_main);
    }else{
        GB_abort_transaction(gb_main);
    }
    if (error) aw_message(error);
    free(name);
}

AW_window *create_alignment_create_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "CREATE_ALIGNMENT", "ALIGNMENT CREATE");
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label");
    aws->create_autosize_button(0,"Please enter the new name\nof the alignment");

    aws->at("input");
    aws->create_input_field("presets/alignment_dest",15);

    aws->at("ok");
    aws->callback(aa_create_alignment);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}

AW_window *NT_create_alignment_window(AW_root *root, AW_CL popmedown)
{
    // if popmedown points to a window, that window is popped down
    
    static AW_window_simple *aws = 0;

    AW_window *aw_popmedown = (AW_window*)popmedown;
    if (aw_popmedown) aw_popmedown->hide();

    if (aws) return aws; // do not duplicate

    aws = new AW_window_simple;

    aws->init( root, "INFO_OF_ALIGNMENT", "ALIGNMENT INFORMATION");
    aws->load_xfig("ad_align.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->callback( AW_POPUP_HELP,(AW_CL)"ad_align.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","H");

    aws->button_length(13);

    aws->at("delete");
    aws->callback(ad_al_delete_cb);
    aws->create_button("DELETE","DELETE","D");

    aws->at("rename");
    aws->callback(AW_POPUP,(AW_CL)create_alignment_rename_window,0);
    aws->create_button("RENAME","RENAME","R");

    aws->at("create");
    aws->callback(AW_POPUP,(AW_CL)create_alignment_create_window,0);
    aws->create_button("CREATE","CREATE","N");

    aws->at("copy");
    aws->callback(AW_POPUP,(AW_CL)create_alignment_copy_window,0);
    aws->create_button("COPY","COPY","C");

    aws->at("check_len");
    aws->callback(ed_al_check_len_cb);
    aws->create_button("CHECK_LEN","CHECK LEN","L");

    aws->at("align");
    aws->callback(ed_al_align_cb);
    aws->create_button("FORMAT","FORMAT","F");

    aws->at("list");
    awt_create_selection_list_on_ad(gb_main,(AW_window *)aws,"presets/use","*=");

    aws->at("aligned");
    aws->create_option_menu("presets/aligned");
    aws->callback(ed_al_check_len_cb);
    aws->insert_default_option("not formatted","n",0);
    aws->callback(ed_al_align_cb);
    aws->insert_option("formatted","j",1);
    aws->update_option_menu();

    aws->at("auto_format");
    aws->create_option_menu("presets/auto_format");
    aws->callback(ed_al_check_auto_format);
    aws->insert_default_option("ask","a",0);
    aws->callback(ed_al_check_auto_format);
    aws->insert_option("always","",1);
    aws->callback(ed_al_check_auto_format);
    aws->insert_option("never","",2);
    aws->update_option_menu();

    aws->at("len");
    aws->create_input_field("presets/alignment_len",7);

    aws->at("type");
    aws->create_option_menu("presets/alignment_type");
    aws->insert_option("dna","d","dna");
    aws->insert_option("rna","r","rna");
    aws->insert_option("pro","p","ami");
    aws->insert_default_option("???","?","usr");
    aws->update_option_menu();

    aws->at("security");
    aws->create_option_menu("presets/security");
    aws->callback(ed_al_check_len_cb);
    aws->insert_option("0","0",0);
    aws->callback(ed_al_check_len_cb);
    aws->insert_option("1","1",1);
    aws->callback(ed_al_check_len_cb);
    aws->insert_option("2","2",2);
    aws->callback(ed_al_check_len_cb);
    aws->insert_option("3","3",3);
    aws->callback(ed_al_check_len_cb);
    aws->insert_option("4","4",4);
    aws->callback(ed_al_check_len_cb);
    aws->insert_option("5","5",5);
    aws->callback(ed_al_check_len_cb);
    aws->insert_default_option("6","6",6);
    aws->update_option_menu();

    aws->at("rem");
    aws->create_text_field("presets/alignment_rem");

    return (AW_window *)aws;
}
