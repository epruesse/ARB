#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <malloc.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_global.hxx>
#include <awt.hxx>


extern GBDATA *gb_main;

#define ARCG_HASH_SIZE 1024
#define MAX_EXCLUDE 8
#define MAX_INCLUDE 4

struct arb_arcg_struct {
    char *infile;
    char *outfile;
    char *fetchfile;
    char *fetch;
    char *textedit;
    char *fetchcommand;
    long    minbp;
    char    *exclude[MAX_EXCLUDE];
    char    *include[MAX_INCLUDE];
    char    buffer[256];
} arcg;

GB_ERROR arb_arcg()
{
    GB_ERROR error = 0;
    GB_HASH *hash;
    GBDATA  *gb_species_data;
    GBDATA  *gb_species;
    GBDATA  *gb_acc;
    char    *acc;
    char    *ac,*p;
    FILE    *in = 0,*out = 0,*fetch = 0;
    char    c;
    int bp;
    int i;
    char    *fetchbuffer = 0;

    hash = GBS_create_hash(ARCG_HASH_SIZE,0);
    for (   gb_species_data = GB_find(gb_main,"species_data",0,down_level);
            gb_species_data; gb_species_data = 0) {
        for (   gb_species = GB_find(gb_species_data,"species",0,down_level);
                gb_species;
                gb_species = GB_find(gb_species,"species",0,search_next|this_level)){
            gb_acc = GB_find(gb_species,"acc",0,down_level);
            if (gb_acc) {
                acc = GB_read_string(gb_acc);
                if (!acc) return GB_get_error();
                for (ac = acc; *ac; ) {
                    for (p = ac; *p==' '; p++) ;
                    ac = p;
                    while (*p && *p!=' ' && *p!=';') p++;
                    if (*p) *(p++) = 0;
                    GBS_write_hash(hash,ac,(long)gb_species);
                    ac = p;
                }
            }
        }
    }
    in = fopen(arcg.infile,"r");
    if (!in) error = GB_export_error("file '%s' not found",arcg.infile) ;

    if (!error ) {
        out = fopen(arcg.outfile,"w");
        if (!out) error = GB_export_error("Cannot open '%s'",arcg.outfile) ;
    }
    if (!error) {
        fetch = fopen(arcg.fetchfile,"w");
        if (!fetch) error = GB_export_error("Cannot open '%s'",arcg.fetchfile) ;
    }

    while (!error) {
        bp = getc(in);
        if (bp == EOF) break;
        arcg.buffer[0] = (char)bp;
        fgets(&arcg.buffer[1],250,in);
        for (ac = &arcg.buffer[0]; *ac&&(*ac!=' ')&&(*ac!='\t'); ac++);
        *ac = 0;
        delete fetchbuffer;
        fetchbuffer = GBS_string_eval(arcg.buffer, arcg.fetchcommand,0);
        *ac = ' ';
        for (;  *ac==' ' || *ac =='\t'; ac++);
        for (p = ac;*p&&*p!=' ';p++);
        c = *p; *p = 0;
        if (GBS_read_hash(hash,ac)) continue;   /* Accession number already in hash */
        *p = c;
        for (i=0;i<MAX_EXCLUDE;i++) {
            if (!strlen(arcg.exclude[i]))   continue;
            if (GBS_find_string(arcg.buffer,arcg.exclude[i],0)) break;
        }
        if (i<MAX_EXCLUDE){         // excluded item check include
            for (i=0;i<MAX_INCLUDE;i++) {
                if (!strlen(arcg.include[i]))   continue;
                if (GBS_find_string(arcg.buffer,arcg.include[i],0)) break;
            }
            if (i==MAX_INCLUDE) goto arcg_next; // not included
        }

        p = ac+strlen(ac)-1;
        while (*p && *p!=' ') {
            p--;
        }
        p = GBS_string_eval(p,".=:,=: =",0);
        bp = atoi(p);
        free(p);
        if (bp<arcg.minbp) continue;
        fprintf(out,"%s",arcg.buffer);
        fprintf(fetch,"%s\n",fetchbuffer);
    arcg_next:;
    }
    fclose(in);
    fclose(out);
    fclose(fetch);
    GBS_free_hash(hash);
    delete fetchbuffer;
    return error;

}

void etc_check_gcg_list_cb(AW_window *aww,char *filesuffix)
{
    AWUSE(aww);
    GB_ERROR error;

    delete arcg.outfile; arcg.outfile = GBS_string_eval(arcg.infile,"*=*.out",0);
    delete arcg.fetchfile; arcg.fetchfile = GBS_string_eval(arcg.infile,"*=*.fetch",0);
    delete arcg.fetchcommand; arcg.fetchcommand = GBS_string_eval(arcg.fetch,"*=\\*\\=* \\*",0);

    GB_begin_transaction(gb_main);
    error = arb_arcg();
    if (error) {
        aw_message(error);
        GB_abort_transaction(gb_main);
    }else{
        GB_commit_transaction(gb_main);
        char buffer[256];
        char *textedit = aww->get_root()->awar("etc_check_gcg/textedit")->read_string();
        sprintf(buffer,"%s %s.%s &",textedit,arcg.infile,filesuffix);
        system(buffer);
        delete textedit;
    }
}

void create_check_gcg_awars(AW_root *aw_root, AW_default aw_def)
{
    char buffer[256];memset(buffer,0,256);
    memset((char *)&arcg,0,sizeof(struct arb_arcg_struct));

    aw_create_selection_box_awars(aw_root, "tmp/etc_check_gcg", "", "", "", aw_def);
    aw_root->awar_string( "etc_check_gcg/fetch", "fetch",aw_def)        ->add_target_var(&arcg.fetch);
    aw_root->awar_string( "etc_check_gcg/textedit", "arb_textedit",aw_def)  ->add_target_var(&arcg.textedit);
    aw_root->awar_int( "etc_check_gcg/minlen", 400, aw_def)         ->add_target_var(&arcg.minbp);

    int i;
    for (i=0;i<MAX_EXCLUDE;i++) {
        sprintf(buffer,"etc_check_gcg/exclude_%i",i);
        aw_root->awar_string( buffer,   "", aw_def )->add_target_var(&arcg.exclude[i]);
    }
    for (i=0;i<MAX_INCLUDE;i++) {
        sprintf(buffer,"etc_check_gcg/include_%i",i);
        aw_root->awar_string( buffer,   "", aw_def)->add_target_var(&arcg.include[i]);
    }
}

void etc_check_gcg_list_show_cb(AW_window *aww){
    char buffer[256];
    char *fname = aww->get_root()->awar("tmp/etc_check_gcg/file_name")->read_string();
    char *textedit = aww->get_root()->awar("etc_check_gcg/textedit")->read_string();

    sprintf(buffer,"%s %s &",textedit,fname);
    system(buffer);

    delete textedit;
    delete fname;
}

AW_window *create_check_gcg_window(AW_root *root){
    char buffer[256];
    int i;
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "CHECK_GCG_LIST", "CHECK GCG LIST");
    aws->load_xfig("etc/checkgcg.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->callback( AW_POPUP_HELP,(AW_CL)"checkgcg.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","H");

    awt_create_selection_box((AW_window *)aws,"tmp/etc_check_gcg");

    for (i=0;i<MAX_EXCLUDE;i++) {
        sprintf(buffer,"ex%i",i+1);
        if (aws->at_ifdef(buffer)){
            aws->at(buffer);
            sprintf(buffer,"etc_check_gcg/exclude_%i",i);
            aws->create_input_field(buffer,4);
        }
    }

    for (i=0;i<MAX_INCLUDE;i++) {
        sprintf(buffer,"in%i",i+1);
        if (aws->at_ifdef(buffer)){
            aws->at(buffer);
            sprintf(buffer,"etc_check_gcg/include_%i",i);
            aws->create_input_field(buffer,4);
        }
    }

    aws->at("textedit");
    aws->create_input_field("etc_check_gcg/textedit",4);

    aws->at("fetch");
    aws->create_input_field("etc_check_gcg/fetch",4);

    aws->at("min");
    aws->create_input_field("etc_check_gcg/minlen",4);

    aws->callback(etc_check_gcg_list_show_cb);
    aws->at("go0");
    aws->create_button("SHOW_FILE", "Show\n\nfile","s");

    aws->callback((AW_CB1)etc_check_gcg_list_cb,(AW_CL)"fetch");
    aws->at("go1");
    aws->create_button("GO_AND_SHOW", "GO & show\n\nfile.fetch","f");


    aws->callback((AW_CB1)etc_check_gcg_list_cb,(AW_CL)"out");
    aws->at("go2");
    aws->create_button("GO_AND_SHOW_file.out", "GO & show\n\nfile.out","o");


    return (AW_window *)aws;
}



