#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <awt.hxx>

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define awtc_assert(bed) arb_assert(bed)


#define AWAR_PARSER "tmp/submission/parser"
#define AWAR_PARSED "tmp/submission/parsed"

void AWTC_create_submission_variables(AW_root *root,AW_default db1)
{
    root->awar_string( "submission/source", ""  ,    db1);
    root->awar_string( AWAR_PARSER, "press READ INFO"  ,    db1);
    root->awar_string( AWAR_PARSED, ""  ,    db1);
    root->awar_string( "submission/file", ""  ,    db1);
    root->awar_string( "submission/privat", "$(ADDRESS,    db1)=TUM Munich:\n"  );
}

static void ed_calltexe_event(AW_window *aww,char *varname)
{
    AW_root *aw_root = aww->get_root();
    char    *file;
    file = aw_root->awar(varname)->read_string();
    AWT_edit(file);
    free(file);
}

static AW_window *create_calltexe_window(AW_root *root, char *varname)
{
    AWUSE(root);

    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "SUBM_TEXTEDIT", "START TEXT EDITOR");

    aws->load_xfig("calltexe.fig");
    aws->label_length( 18 );
    aws->button_length( 8 );

    aws->at( "close" );
    aws->callback     ( (AW_CB0)AW_POPDOWN  );
    aws->create_button( "CLOSE", "CLOSE", "O" );

    aws->at("file");
    aws->create_input_field(varname,34);

    aws->at("edit");
    aws->callback( (AW_CB1)ed_calltexe_event,(AW_CL)varname);
    aws->create_button( "EDIT", "EDIT", "E" );

    return (AW_window *)aws;
}

#if 0
static long ed_submit_info_event_sequence(void *strstruct,char *alignment,long modu)
{
    int i;
    char    buffer[256];
    int size = strlen(alignment);
    int c;
    long    seq_len = 0;
    for (seq_len=i=0;i<size;i++) {
        c = alignment[i];
        if (c == '.') continue;
        if (c == '-') continue;

        if (c==':') c = ';';
        if (c=='=') c = '-';
        if ( !(seq_len%modu) ) {
            sprintf(buffer,"\n%8li ",seq_len+1);
            GBS_strcat(strstruct,buffer);
        }
        if ( !(seq_len%10) ) GBS_strcat(strstruct," ");
        seq_len++;
        GBS_chrcat(strstruct,c);
    }
    return seq_len;
}
#endif

static void ed_submit_info_event_rm(char *string)
{
    char *p = string;
    int c;
    while ((c=*p)!= 0 ) {
        if (c==':') *p = ';';
        if (c=='=') *p = '-';
        p++;
    }
}

extern GBDATA *gb_main;

static void ed_submit_info_event(AW_window *aww)
{
    AW_root *aw_root = aww->get_root();
    GB_transaction dummy(gb_main);
    char *key;

    GBDATA  *gb_species;
    GBDATA  *gb_entry;

    char    *parser;
    char    *species_name = aw_root->awar(AWAR_SPECIES_NAME)->read_string();
    gb_species = GBT_find_species(gb_main,species_name);
    void *strstruct = GBS_stropen(1000);

    if (gb_species) {
        for (   gb_entry = GB_find(gb_species,0,0,down_level);
                gb_entry;
                gb_entry = GB_find(gb_entry,0,0,this_level | search_next) ) {
            int type = GB_read_type(gb_entry);
            switch (type) {
                case GB_STRING:
                case GB_INT:
                case GB_BITS:
                case GB_FLOAT:
                    key  = GB_read_key(gb_entry);
                    GBS_strcat(strstruct,"$(");
                    GBS_strcat(strstruct,key);
                    GBS_strcat(strstruct,")=");
                    free(key);
                    key = GB_read_as_string(gb_entry);
                    ed_submit_info_event_rm(key);
                    GBS_strcat(strstruct,key);
                    free(key);
                    GBS_strcat(strstruct,":\n");
                    break;
                default:
                    break;
            }
        }
        char *seq_info = GBS_string_eval(" ","*="
                                         "$(SEQ_LEN)\\=*(|sequence|len(.-))\\:\n"
                                         "$(SEQ_A)\\=*(|sequence|count(aA))\\:\n"
                                         "$(SEQ_C)\\=*(|sequence|count(cC))\\:\n"
                                         "$(SEQ_G)\\=*(|sequence|count(gG))\\:\n"
                                         "$(SEQ_T)\\=*(|sequence|count(tT))\\:\n"
                                         "$(SEQ_U)\\=*(|sequence|count(uU))\\:\n"
                                         "$(SEQUENCE)\\=*(|sequence|remove(.-)|format_sequence(firsttab\\=12;tab\\=12;width\\=60;numleft;gap\\=10))\\:\n"
                                         ,gb_species);
        if (seq_info) {
            GBS_strcat(strstruct, seq_info);
            free(seq_info);
        }else{
            aw_message(GB_get_error());
        }
    }else{
        GBS_strcat(strstruct,"Species not found");
    }
    parser = GBS_strclose(strstruct);
    aw_root->awar(AWAR_PARSER)->write_string(parser);

    free(parser);
    free(species_name);
}

static void ed_save_var_to_file(AW_window *aww,char *data_var, char *file_var)
{
    AW_root *aw_root = aww->get_root();
    char * data = aw_root->awar(data_var)->read_string();
    char *file_name = aw_root->awar(file_var)->read_string();
    FILE *out = fopen(file_name,"w");
    if (out) fprintf(out,"%s",data);
    fclose(out);
    free(file_name);
    free(data);
}
static void ed_submit_parse_event(AW_window *aww)
{
    AW_root *aw_root = aww->get_root();
    char *parser = aw_root->awar(AWAR_PARSER)->read_string();
    char    *dest;
    char    *dest2;
    char    *privat;
    char    *p,*d;
    int c;

    for (d = p = parser; *p; p++) {
        if ((c=*p)==':') {
            if (p[1] == '\n');
            p++;            /* skip newline */
        }
        *(d++) = c;
    }
    *d = 0;
    char *sub_file = aw_root->awar("submission/source")->read_string();
    char *source = GB_read_file(sub_file);
    if (source) {
        dest = GBS_string_eval(source,parser,0);
        if (!dest) dest = strdup(GB_get_error());
    }
    else {
        dest = strdup("submission form not found");
    }

    awtc_assert(dest); // should contain partly filled form or error message

    privat = aw_root->awar("submission/privat")->read_string();
    for (d = p = privat; *p; p++) {
        if ((c=*p)==':') {
            if (p[1] == '\n');
            p++;            /* skip newline */
        }
        *(d++) = c;
    }
    *d = 0;

    dest2             = GBS_string_eval(dest,privat,0);
    if (!dest2) dest2 = strdup(GB_get_error());

    aw_root->awar(AWAR_PARSED)->write_string(dest2);

    free(dest);
    free(dest2);
    free(privat);
    free(source);
    free (sub_file);
    free(parser);
}

static void ed_submit_gen_event(AW_window *aww)
{
    AW_root *aw_root = aww->get_root();
    char buffer[256];
    char *name = aw_root->awar(AWAR_SPECIES_NAME)->read_string();
    sprintf(buffer,"%s.submit",name);
    free(name);
    aw_root->awar("submission/file")->write_string(buffer);
}


AW_window *AWTC_create_submission_window(AW_root *root)
{
    AWUSE(root);

    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "SUBMISSION", "SUBMISSION");

    aws->load_xfig("submiss.fig");
    aws->label_length( 18 );
    aws->button_length( 8 );

    aws->at( "close" );
    aws->callback     ( (AW_CB0)AW_POPDOWN  );
    aws->create_button( "CLOSE", "CLOSE", "O" );

    aws->callback( AW_POPUP_HELP,(AW_CL)"submission.hlp");
    aws->at("help");
    aws->create_button("HELP", "HELP","H");

    aws->button_length( 15 );

    aws->at("privat");
    aws->create_text_field("submission/privat",80,5);

    aws->at( "parsed_info" );
    aws->create_text_field( AWAR_PARSER, 80,6 );

    aws->at( "parsed" );
    aws->create_text_field( AWAR_PARSED, 80,13 );

    aws->at( "species" );
    aws->label( "Species Name:");
    aws->create_input_field( AWAR_SPECIES_NAME, 12 );

    aws->at( "submission" );
    char **submits = GBS_read_dir(0,"submit/*");
    if (submits) {
        aws->create_option_menu( "submission/source", "Select a Form", "s" );
        for (char **submit = submits; *submit; submit++){
            aws->insert_option(*submit,"",*submit);
        }
        aws->insert_default_option("default","d","default");
        aws->update_option_menu();

        GBS_free_names(submits);
    }

    aws->at("gen");
    aws->label("Gen File Name");
    aws->callback(ed_submit_gen_event);
    aws->create_button("GEN_FILE_NAME", "CREATE NAME","C");

    aws->at( "file" );
    aws->label( "or enter");
    aws->create_input_field( "submission/file", 30 );

    aws->at( "info" );
    aws->callback( ed_submit_info_event);
    aws->create_button("READ_INFO", "READ INFO","R");

    aws->at( "parse" );
    aws->callback((AW_CB0)ed_submit_parse_event);
    aws->create_button("FILL_OUT_FORM", "FILL THE FORM","F");

    aws->at( "write" );
    aws->callback( (AW_CB)ed_save_var_to_file,(AW_CL)AWAR_PARSED,(AW_CL)"submission/file");
    aws->create_button("SAVE", "SAVE TO FILE","S");

    aws->button_length( 20 );
    aws->at( "edit" );
    aws->callback(AW_POPUP,(AW_CL)create_calltexe_window,(AW_CL)"submission/source");
    aws->create_button("EDIT_FORM", "EDIT FORM","R");

    aws->at( "editresult" );
    aws->callback(AW_POPUP,(AW_CL)create_calltexe_window,(AW_CL)"submission/file");
    aws->create_button("EDIT_SAVED", "EDIT SAVED","S");

    aws->at("privatlabel");
    aws->create_button(0,"Your private data");

    return (AW_window *)aws;
}
