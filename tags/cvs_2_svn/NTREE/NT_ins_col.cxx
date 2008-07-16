#include <stdio.h>
#include <stdlib.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>

extern GBDATA *GLOBAL_gb_main;     /* muss existieren */

void create_insertchar_variables(AW_root *root,AW_default db1)
{
    root->awar_int( AWAR_CURSOR_POSITION, 0  ,    (void*)GLOBAL_gb_main);
    root->awar_int( "insertchar/nchar", 0  ,    db1);
    root->awar("insertchar/nchar")->set_minmax( 0, 999000);
    root->awar_string( "insertchar/characters", ""  ,    db1);
}

void awt_inserchar_event(AW_window *aws,AW_CL awcl_mode)
{
    AW_root *root = aws->get_root();
    char *alignment;
    long    pos;
    long    nchar;
    char    *deletes;
    int mode = (int)awcl_mode;

    pos = root->awar(AWAR_CURSOR_POSITION)->read_int()-1;
    nchar = root->awar("insertchar/nchar")->read_int() * mode;
    deletes = root->awar("insertchar/characters")->read_string();

    GB_begin_transaction(GLOBAL_gb_main);
    alignment = GBT_get_default_alignment(GLOBAL_gb_main);

    if (alignment) {
        GB_ERROR error = GBT_insert_character(GLOBAL_gb_main,alignment,pos,nchar,deletes);

        if (error) {
            GB_abort_transaction(GLOBAL_gb_main);
            aw_message(error);
        }else{
            GBT_check_data(GLOBAL_gb_main,0);
            GB_commit_transaction(GLOBAL_gb_main);
        }
    }else{
        printf("ERROR no alignment found\n");
        GB_abort_transaction(GLOBAL_gb_main);
    }

    free(deletes);
    free(alignment);
}

AW_window *create_insertchar_window(AW_root *root, AW_default def)
{
    AWUSE(root);
    AWUSE(def);

    static AW_window_simple *aws = 0;
    if (aws) return aws;
    aws = new AW_window_simple;

    aws->init( root, "INSERT_COLOUM", "INSERT CHAR");

    aws->load_xfig("inschar.fig");
    aws->button_length(8);

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->callback( AW_POPUP_HELP,(AW_CL)"insdelchar.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","H");

    aws->label_length(27);

    aws->at("pos");
    aws->label("Sequence Position");
    aws->create_input_field(AWAR_CURSOR_POSITION,6); // @@@ mit EDIT4-Cursor-position linken

    aws->at("len");
    aws->label("How many Characters");
    aws->create_input_field("insertchar/nchar",6);

    aws->at("characters");
    aws->label("Delete Only (% = all)");
    aws->create_input_field("insertchar/characters",6);

    aws->callback(awt_inserchar_event,(AW_CL)1);
    aws->at("insert");
    aws->create_button("INSERT","INSERT","I");

    aws->callback(awt_inserchar_event,(AW_CL)-1);
    aws->at("delete");
    aws->create_button("DELETE","DELETE","D");

    return (AW_window *)aws;
}
