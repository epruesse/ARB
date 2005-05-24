#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <awt.hxx>
#include <awt_changekey.hxx>
#include "merge.hxx"

#define AWAR1 "tmp/merge1/"
#define AWAR2 "tmp/merge2/"
#define AWAR_ALI1 AWAR1"alignment_name"
#define AWAR_ALI2 AWAR2"alignment_name"

void MG_alignment_vars_callback(AW_root *aw_root,GBDATA *gbd, long ali_nr)
{
    char buffer[256];

    GB_push_transaction(gbd);
    sprintf(buffer,"tmp/merge%li/alignment_name",ali_nr);
    char    *use = aw_root->awar(buffer)->read_string();
    GBDATA *ali_cont = GBT_get_alignment(gbd,use);
    if (!ali_cont) {
        sprintf(buffer,"tmp/merge%li/alignment_type",ali_nr);
        aw_root->awar(buffer)->unmap();

        sprintf(buffer,"tmp/merge%li/alignment_len",ali_nr);
        aw_root->awar(buffer)->unmap();

        sprintf(buffer,"tmp/merge%li/aligned",ali_nr);
        aw_root->awar(buffer)->unmap();

        sprintf(buffer,"tmp/merge%li/security",ali_nr);
        aw_root->awar(buffer)->unmap();
    }else{

        GBDATA *ali_len = GB_find(ali_cont,"alignment_len",0,down_level);
        GBDATA *ali_aligned = GB_find(ali_cont,"aligned",0,down_level);
        GBDATA *ali_type = GB_find(ali_cont,"alignment_type",0,down_level);
        GBDATA *ali_security = GB_find(ali_cont,"alignment_write_security",0,down_level);

        sprintf(buffer,"tmp/merge%li/alignment_type",ali_nr);
        aw_root->awar(buffer)->map((void*)ali_type);

        sprintf(buffer,"tmp/merge%li/alignment_len",ali_nr);
        aw_root->awar(buffer)->map((void*)ali_len);

        sprintf(buffer,"tmp/merge%li/aligned",ali_nr);
        aw_root->awar(buffer)->map((void*)ali_aligned);

        sprintf(buffer,"tmp/merge%li/security",ali_nr);
        aw_root->awar(buffer)->map((void*)ali_security);

    }
    GB_pop_transaction(gbd);
    free(use);
}


void MG_create_alignment_vars(AW_root *aw_root,AW_default aw_def)
{
    aw_root->awar_string( AWAR_ALI1, "" ,   aw_def);
    aw_root->awar_string( AWAR_ALI2, "" ,   aw_def);

    aw_root->awar_string( AWAR1"alignment_dest", "" ,   aw_def);
    aw_root->awar_string( AWAR2"alignment_dest", "" ,   aw_def);
    aw_root->awar_string( AWAR1"alignment_type", "" ,   aw_def);
    aw_root->awar_string( AWAR2"alignment_type", "" ,   aw_def);
    aw_root->awar_int( AWAR1"alignment_len", 0 ,    aw_def);
    aw_root->awar_int( AWAR2"alignment_len", 0 ,    aw_def);
    aw_root->awar_int( AWAR1"aligned", 0 ,  aw_def);
    aw_root->awar_int( AWAR2"aligned", 0 ,  aw_def);
    aw_root->awar_int( AWAR1"security", 0 , aw_def);
    aw_root->awar_int( AWAR2"security", 0 , aw_def);

}

int MG_check_alignment(AW_window *aww,int fast)
{
    AWUSE(aww);
    // check type and names !!!!
    char result[1024];
    result[0] = 0;
    if (!fast){
        aw_openstatus("Checking alignments");
        sleep(1);
    }
    GB_begin_transaction(gb_dest);
    GB_begin_transaction(gb_merge);
    char **names = GBT_get_alignment_names(gb_merge);
    char **name;
    GBDATA *gb_ali1;
    GBDATA *gb_ali2;
    GBDATA *gb_presets2;

    for (name = names; *name; name++) {
        if (! (gb_ali2 = GBT_get_alignment(gb_dest,*name)) ) {
            gb_ali1 = GBT_get_alignment(gb_merge,*name);
            gb_presets2 = GB_search(gb_dest,"presets",GB_CREATE_CONTAINER);
            gb_ali2 = GB_create_container(gb_presets2,"alignment");
            GB_copy(gb_ali2,gb_ali1);
            awt_add_new_changekey( gb_dest, (char *)GBS_global_string("%s/data",*name),GB_STRING);
        }
        char *type1 = GBT_get_alignment_type_string(gb_merge,*name);
        char *type2 = GBT_get_alignment_type_string(gb_dest,*name);
        if (strcmp(type1,type2)) {
            sprintf(result,"The alignments '%s' have different types (%s != %s)", *name,type1,type2);
            break;
        }
        delete(type1);
        delete(type2);
    }
    GBT_free_names(names);
    GB_commit_transaction(gb_dest);
    GB_commit_transaction(gb_merge);
    if (strlen(result)) aw_message(result);
    if (!fast){
        aw_closestatus();
    }
    return strlen(result);
}

void MG_ad_al_delete_cb(AW_window *aww,AW_CL db_nr)
{
    if (aw_message("Are you sure to delete all data belonging to this alignment","OK,CANCEL"))return;

    GB_ERROR error = 0;
    char buffer[256];
    sprintf(buffer,"tmp/merge%li/alignment_name",db_nr);
    GBDATA *gbd;
    if (db_nr == 1) gbd = gb_merge;
    else        gbd = gb_dest;

    char *source = aww->get_root()->awar(buffer)->read_string();

    GB_begin_transaction(gbd);

    error = GBT_rename_alignment(gbd,source,0,0,1);

    if (!error){
        GB_commit_transaction(gbd);
    }else{
        GB_abort_transaction(gbd);
    }
    if (error) aw_message(error);
    delete source;
}


void MG_ed_al_check_len_cb(AW_window *aww,AW_CL db_nr)
{
    char *error = 0;
    char buffer[256];
    sprintf(buffer,"tmp/merge%li/alignment_name",db_nr);
    GBDATA *gbd;
    if (db_nr == 1) gbd = gb_merge;
    else        gbd = gb_dest;

    char *use = aww->get_root()->awar(buffer)->read_string();

    GB_begin_transaction(gbd);
    if (!error) error = (char *)GBT_check_data(gbd,use);
    GB_commit_transaction(gbd);
    if (error) aw_message(error);
    delete use;
}

void MG_copy_delete_rename(AW_window *aww,AW_CL db_nr, AW_CL dele)
{
    GB_ERROR error = 0;
    char buffer[256];
    GBDATA *gbd;
    if (db_nr == 1) gbd = gb_merge;
    else        gbd = gb_dest;

    sprintf(buffer,"tmp/merge%li/alignment_name",db_nr);
    char *source = aww->get_root()->awar(buffer)->read_string();
    sprintf(buffer,"tmp/merge%li/alignment_dest",db_nr);
    char *dest = aww->get_root()->awar(buffer)->read_string();

    GB_begin_transaction(gbd);

    error = GBT_rename_alignment(gbd,source,dest,(int)1,(int)dele);

    if (!error){
        awt_add_new_changekey( gbd, GBS_global_string("%s/data",dest),GB_STRING);
        GB_commit_transaction(gbd);
    }else   GB_abort_transaction(gbd);

    if (error) aw_message(error);
    else aww->hide();
    delete source;
    delete dest;
}


AW_window *create_alignment_copy_window(AW_root *root,AW_CL db_nr)
{
    AW_window_simple *aws = new AW_window_simple;
    char header[80];
    sprintf(header,"ALIGNMENT COPY %li",db_nr);
    aws->init( root, header, header);
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label");
    aws->create_autosize_button(0,"Please enter the new name\nof the alignment");

    aws->at("input");
    char buffer[256];
    sprintf(buffer,"tmp/merge%li/alignment_dest",db_nr);
    aws->create_input_field(buffer,15);

    aws->at("ok");
    aws->callback(MG_copy_delete_rename,db_nr,0);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}
AW_window *MG_create_alignment_rename_window(AW_root *root,AW_CL db_nr)
{
    AW_window_simple *aws = new AW_window_simple;
    char header[80];
    sprintf(header,"ALIGNMENT RENAME %li",db_nr);
    aws->init( root, header,header);
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label");
    aws->create_autosize_button(0,"Please enter the name\nof the new alignment");

    aws->at("input");
    char buffer[256];
    sprintf(buffer,"tmp/merge%li/alignment_dest",db_nr);
    aws->create_input_field(buffer,15);

    aws->at("ok");
    aws->callback(MG_copy_delete_rename,db_nr,1);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}

void MG_aa_create_alignment(AW_window *aww,AW_CL db_nr)
{
    GB_ERROR error = 0;
    char buffer[256];
    GBDATA *gbd;
    if (db_nr == 1) gbd = gb_merge;
    else        gbd = gb_dest;

    sprintf(buffer,"tmp/merge%li/alignment_dest",db_nr);
    char *name = aww->get_root()->awar(buffer)->read_string();
    GBDATA *gb_alignment;
    GB_begin_transaction(gbd);

    gb_alignment = GBT_create_alignment(gbd,name,0,0,0,"dna");
    if (!gb_alignment) error = GB_get_error();

    if (!error) GB_commit_transaction(gbd);
    else    GB_abort_transaction(gbd);
    if (error) aw_message(error);
    delete name;
}

AW_window *MG_create_alignment_create_window(AW_root *root,AW_CL db_nr)
{
    AW_window_simple *aws = new AW_window_simple;
    char header[80];
    sprintf(header,"ALIGNMENT CREATE %li",db_nr);
    aws->init( root, header,header);
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label");
    aws->create_autosize_button(0,"Please enter the new name\nof the alignment");

    aws->at("input");
    char buffer[256];
    sprintf(buffer,"tmp/merge%li/alignment_dest",db_nr);
    aws->create_input_field(buffer,15);

    aws->at("ok");
    aws->callback(MG_aa_create_alignment,db_nr);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}



AW_window *MG_create_alignment_window(AW_root *root,AW_CL db_nr)
{
    char buffer[256];
    GBDATA *gbd;
    if (db_nr == 1) gbd = gb_merge;
    else        gbd = gb_dest;


    AW_window_simple *aws = new AW_window_simple;
    char header[80];
    sprintf(header,"ALIGNMENT CONTROL %li",db_nr);
    aws->init( root, header, header);
    aws->load_xfig("merge/ad_align.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->callback( AW_POPUP_HELP,(AW_CL)"ad_align.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","H");

    aws->button_length(13);

    aws->at("list");
    sprintf(buffer,"tmp/merge%li/alignment_name",db_nr);
    awt_create_selection_list_on_ad(gbd,(AW_window *)aws,buffer,"*=");

    aws->at("delete");
    aws->callback(MG_ad_al_delete_cb,db_nr);
    aws->create_button("DELETE","DELETE","D");

    aws->at("rename");
    aws->callback(AW_POPUP,(AW_CL)MG_create_alignment_rename_window,db_nr);
    aws->create_button("RENAME","RENAME","R");

    aws->at("create");
    aws->callback(AW_POPUP,(AW_CL)MG_create_alignment_create_window,db_nr);
    aws->create_button("CREATE","CREATE","N");

    aws->at("copy");
    aws->callback(AW_POPUP,(AW_CL)create_alignment_copy_window,db_nr);
    aws->create_button("COPY","COPY","C");

    aws->at("aligned");
    sprintf(buffer,"tmp/merge%li/aligned",db_nr);
    aws->create_option_menu(buffer);
    aws->insert_option("justified","j",1);
    aws->insert_default_option("not justified","n",0);
    aws->update_option_menu();



    aws->at("len");
    sprintf(buffer,"tmp/merge%li/alignment_len",db_nr);
    aws->create_input_field(buffer,8);

    aws->at("type");
    sprintf(buffer,"tmp/merge%li/alignment_type",db_nr);
    aws->create_option_menu(buffer);
    aws->insert_option("dna","d","dna");
    aws->insert_option("rna","r","rna");
    aws->insert_option("pro","p","ami");
    aws->insert_default_option("???","?","usr");
    aws->update_option_menu();

    aws->at("security");
    sprintf(buffer,"tmp/merge%li/security",db_nr);
    //  aws->get_root()->awar(buffer)->add_callback(MG_ed_al_check_len_cb,db_nr);
    aws->callback(MG_ed_al_check_len_cb,db_nr);
    aws->create_option_menu(buffer);
    aws->insert_option("0","0",0);
    aws->insert_option("1","1",1);
    aws->insert_option("2","2",2);
    aws->insert_option("3","3",3);
    aws->insert_option("4","4",4);
    aws->insert_option("5","5",5);
    aws->insert_default_option("6","6",6);
    aws->update_option_menu();

    return (AW_window *)aws;

}

AW_window *MG_merge_alignment_cb(AW_root *awr){
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    awr->awar(AWAR_ALI1)->add_callback( (AW_RCB)MG_alignment_vars_callback,(AW_CL)gb_merge,1);
    awr->awar(AWAR_ALI2)->add_callback( (AW_RCB)MG_alignment_vars_callback,(AW_CL)gb_dest,2);

    aws = new AW_window_simple;
    aws->init( awr, "MERGE_ALIGNMENTS", "MERGE ALIGNMENTS");
    aws->load_xfig("merge/alignment.fig");

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"mg_alignment.hlp");
    aws->create_button("HELP","HELP","H");

    aws->at("check");
    aws->callback((AW_CB1)MG_check_alignment,0);
    aws->create_button("CHECK","Check");

    aws->at("ali1");
    awt_create_selection_list_on_ad(gb_merge,(AW_window *)aws,AWAR_ALI1,"*=");

    aws->at("ali2");
    awt_create_selection_list_on_ad(gb_dest,(AW_window *)aws,AWAR_ALI2,"*=");

    aws->at("modify1");
    aws->callback(AW_POPUP,(AW_CL)MG_create_alignment_window,1);
    aws->create_button("MODIFY_DB1","MODIFY");

    aws->at("modify2");
    aws->callback(AW_POPUP,(AW_CL)MG_create_alignment_window,2);
    aws->create_button("MODIFY_DB2","MODIFY");


    aws->button_length(0);
    aws->shadow_width(1);
    aws->at("icon");
    aws->callback(AW_POPUP_HELP,(AW_CL)"mg_alignment.hlp");
    aws->create_button("HELP_MERGE", "#merge/icon.bitmap");

    return (AW_window *)aws;
}
