#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
// #include <malloc.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_global.hxx>
#include <awt.hxx>
#include <inline.h>

#include "awti_export.hxx"
#include "awti_exp_local.hxx"
#include "aw_awars.hxx"

#include "xml.hxx"

using std::string;

export_format_struct::export_format_struct(void){
    memset((char *)this,0,sizeof(export_format_struct));
}

export_format_struct::~export_format_struct(void)
{
    struct export_format_struct *efo= this;
    free(efo->system);
    free(efo->new_format);
    free(efo->suffix);
}


char *awtc_read_export_format(export_format_struct * efo,char *file){
    char *fullfile = AWT_unfold_path(file,"ARBHOME");

    FILE *in = fopen(fullfile,"r");
    free(fullfile);
    sprintf(AW_ERROR_BUFFER,"Form %s not readable (select a form or check permissions)", file);
    if (!in) return AW_ERROR_BUFFER;
    char *s1=0,*s2=0;

    while (!awtc_read_string_pair(in,s1,s2)){
        if (!s1) {
            continue;
        }else if (!strcmp(s1,"SYSTEM")) {
            efo->system = s2; s2 = 0;
        }else if (!strcmp(s1,"INTERNAL")) {
            efo->internal_command = s2; s2 = 0;
        }else if (!strcmp(s1,"PRE_FORMAT")) {
            efo->new_format = s2; s2 = 0;
        }else if (!strcmp(s1,"SUFFIX")) {
            efo->suffix = s2; s2 = 0;
        }else if (!strcmp(s1,"BEGIN")) {
            break;
        }else{
            fprintf(stderr,"Unknown command in import format file: %s\n",s1);
        }
    }

    fclose(in);
    return 0;
}

//  -----------------------------
//      enum AWTI_EXPORT_CMD
//  -----------------------------
enum AWTI_EXPORT_CMD {
    AWTI_EXPORT_XML,

    AWTI_EXPORT_COMMANDS,       // counter
    AWTI_EXPORT_BY_FORM
};

static const char *internal_export_commands[AWTI_EXPORT_COMMANDS] = {
    "xml_write"
};

//  --------------------------------------------------------
//      static GB_ERROR AWTI_XML_recursive(GBDATA *gbd)
//  --------------------------------------------------------
static GB_ERROR AWTI_XML_recursive(GBDATA *gbd) {
    GB_ERROR    error    = 0;
    const char *key_name = GB_read_key_pntr(gbd);
    XML_Tag    *tag      = 0;

    if (strncmp(key_name, "ali_", 4) == 0)
    {
        tag = new XML_Tag("ALIGNMENT");
        tag->add_attribute("name", key_name+4);
    }
    else {
        tag = new XML_Tag(key_name);

        GBDATA *gb_name = GB_find(gbd, "name", 0, down_level);
        if (gb_name) {
            tag->add_attribute("name", GB_read_char_pntr(gb_name));
        }
    }

    switch (GB_read_type(gbd)) {
        case GB_DB: {
            for (GBDATA *gb_child = GB_find(gbd, 0, 0, down_level);
                 gb_child && !error;
                 gb_child = GB_find(gb_child, 0, 0, this_level|search_next))
            {
                const char *sub_key_name = GB_read_key_pntr(gb_child);

                if (strcmp(sub_key_name, "name") != 0) { // do not recurse for "name" (is handled above)
                    error = AWTI_XML_recursive(gb_child);
                }
            }
            break;
        }
        default: {
            char *content = GB_read_as_string(gbd);
            if (content) {
                XML_Text text(content);
            }
            else {
                tag->add_attribute("error", "unsavable");
            }
        }
    }

    delete tag;
    return error;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------
//      GB_ERROR AWTI_export_format(AW_root *aw_root, GBDATA *gb_main, char *formname, char *outname, int multiple, int openstatus, char **resulting_outname)
// --------------------------------------------------------------------------------------------------------------------------------------------------------------
GB_ERROR AWTI_export_format(AW_root *aw_root, GBDATA *gb_main, char *formname, char *outname, int multiple, int openstatus, char **resulting_outname)
    // if resulting_outname != NULL -> set *resulting_outname to filename with suffix appended
{
    char            *fullformname = AWT_unfold_path(formname,"ARBHOME");
    GB_ERROR         error        = 0;
    char            *form         = GB_read_file(fullformname);
    AWTI_EXPORT_CMD  cmd          = AWTI_EXPORT_BY_FORM;

    if (!form || form[0] == 0) {
        if (!formname || formname[0] == 0) error = GB_export_error("Please select a form");
        else error                               = GB_export_IO_error("loading export form", fullformname);
    }else{
        char *form2 = GBS_string_eval(form,"*\nBEGIN*\n*=*3:\\==\\\\\\=:*=\\*\\=*1:\\:=\\\\\\:",0);
        if (!form2) error = (char *)GB_get_error();
        free(form);
        form = form2;
    }

    if (openstatus) aw_openstatus("Exporting Data");

    export_format_struct *efo = 0;
    if (!error) {
        efo   = new export_format_struct;;
        error = awtc_read_export_format(efo,formname);
    }

    if (!error) {
        if (efo->system && !efo->new_format) {
            error = "Format File Error: You can only use the command SYSTEM "
                "when you use the command PRE_FORMAT";
        }
        else if (efo->internal_command) {
            if (efo->system) {
                error = "Format File Error: You can't use SYSTEM together with INTERNAL";
            }
            else if (efo->new_format) {
                error = "Format File Error: You can't use PRE_FORMAT together with INTERNAL";
            }
            else {
                for (int c = 0; c<AWTI_EXPORT_COMMANDS; ++c) {
                    if (strcmp(internal_export_commands[c], efo->internal_command) == 0) {
                        cmd = (AWTI_EXPORT_CMD)c;
                        break;
                    }
                }

                if (cmd == AWTI_EXPORT_COMMANDS) {
                    error = GB_export_error("Format File Error: Unknown INTERNAL command '%s'", efo->internal_command);
                }
            }
        }
        else if (efo->new_format) {
            if (efo->system) {
                char intermediate[1024];
                char srt[1024];

                {
                    const char *out_nameonly        = strrchr(outname, '/');
                    if (!out_nameonly) out_nameonly = outname;
                    sprintf(intermediate,"/tmp/%s_%i",out_nameonly,getpid());
                }

                char *intermediate_resulting = 0;
                error = AWTI_export_format(aw_root, gb_main, efo->new_format, intermediate, 0, 0, &intermediate_resulting);

                if (!error) {
                    sprintf(srt,"$<=%s:$>=%s",intermediate_resulting, outname);
                    char *sys = GBS_string_eval(efo->system,srt,0);
                    sprintf(AW_ERROR_BUFFER,"exec '%s'",efo->system);
                    aw_status(AW_ERROR_BUFFER);
                    if (system(sys)) {
                        sprintf(AW_ERROR_BUFFER,"Error in '%s'",sys);
                        delete sys; error = AW_ERROR_BUFFER;
                    }
                    free(sys);
                }
                free(intermediate_resulting);
            }else{
                error = AWTI_export_format(aw_root, gb_main, efo->new_format, outname, multiple, 0, 0);
            }
            goto end_of_AWTI_export_format;
        }
    }

    if (!error) {
        try {
            FILE         *out   = 0;
            int           count = 0;
            char          buffer[1024];
            XML_Document *xml   = 0;

            if (!error && !multiple) {
                char *existing_suffix = strrchr(outname, '.');

                if (existing_suffix && ARB_stricmp(existing_suffix+1, efo->suffix) == 0) strcpy(buffer, outname);
                else sprintf(buffer, "%s.%s", outname, efo->suffix);

                if (resulting_outname != 0) *resulting_outname = strdup(buffer);

                out = fopen(buffer,"w");
                if (!out) {
                    sprintf(AW_ERROR_BUFFER,"Error: I Cannot write to file %s",outname);
                    error = AW_ERROR_BUFFER;
                }
            }

            for (GBDATA *gb_species = GBT_first_marked_species(gb_main);
                 !error && gb_species;
                 gb_species = GBT_next_marked_species(gb_species))
            {
                if (multiple) {
                    char  buf[1024];
                    char *name = GBT_read_name(gb_species);

                    sprintf(buf,"%s_%s.%s",outname,name, efo->suffix);
                    delete name;
                    out = fopen(buf,"w");
                    if (!out){
                        sprintf(AW_ERROR_BUFFER,"Error: I Cannot write to file %s", buf);
                        error = AW_ERROR_BUFFER;
                        break;
                    }
                }

                switch (cmd) {
                    case AWTI_EXPORT_BY_FORM: {
                        if (count % 10 == 0) {
                            char *name = GBT_read_name(gb_species);
                            sprintf(buffer,"%s: %i",name, count);
                            if (aw_status(buffer)) break;
                            free(name);
                        }
                        char *pars = GBS_string_eval(" ",form,gb_species);
                        if (!pars) {
                            error = GB_get_error();
                            break;
                        }
                        char *p;
                        char *o = pars;
                        while ( (p = GBS_find_string(o,"$$DELETE_LINE$$",0)) ) {
                            char *l,*r;
                            for (l = p; l>o; l--) if (*l=='\n') break;
                            r = strchr(p,'\n'); if (!r) r = p +strlen(p);
                            fwrite(o,1,l-o,out);
                            o = r;
                        }
                        fprintf(out,"%s",o);
                        free(pars);

                        break;
                    }
                    case AWTI_EXPORT_XML: {
                        if (!xml) {
                            xml = new XML_Document("ARB_SEQ_EXPORT", "arb_seq_export.dtd", out);

                            {
                                char *db_name = aw_root->awar(AWAR_DB_NAME)->read_string();
                                xml->add_attribute("database", db_name);
                                free(db_name);
                            }
                            xml->add_attribute("export_date", AWT_date_string());

                            char *fulldtd = AWT_unfold_path("lib/dtd", "ARBHOME");
                            XML_Comment rem(GBS_global_string("There's a basic version of ARB_seq_export.dtd in %s\n"
                                                              "but you might need to expand it by yourself,\n"
                                                              "because the ARB-database may contain any kind of fields.",
                                                              fulldtd));
                            free(fulldtd);
                        }

                        error = AWTI_XML_recursive(gb_species);

                        if (multiple) {
                            delete xml;
                            xml = 0;
                        }
                        break;
                    }
                    default: {
                        gb_assert(0);
                        break;
                    }
                }

                if (multiple) {
                    if (out) fclose(out);
                    out = 0;
                    // @@@ FIXME: delete written file if error != 0
                }
                count ++;
            }

            delete xml;
            if (out) fclose(out);
        }
        catch (string& err) {
            error = GB_export_error("Error: %s (programmers error)", err.c_str());
        }
    }

 end_of_AWTI_export_format:
    if (openstatus) aw_closestatus();

    free(fullformname);
    free(form);
    delete efo;

    return error;
}


void AWTC_export_go_cb(AW_window *aww,GBDATA *gb_main){


    GB_transaction dummy(gb_main);

    AW_root *awr = aww->get_root();
    char    *formname = awr->awar(AWAR_EXPORT_FORM"/file_name")->read_string();
    int multiple = (int)awr->awar(AWAR_EXPORT_MULTIPLE_FILES)->read_int();
    GB_ERROR error = 0;

    char *outname      = awr->awar(AWAR_EXPORT_FILE"/file_name")->read_string();
    char *real_outname = 0;     // with suffix

    error = AWTI_export_format(awr, gb_main, formname, outname, multiple,1, &real_outname);
    if (real_outname) awr->awar(AWAR_EXPORT_FILE"/file_name")->write_string(real_outname);

    awt_refresh_selection_box(awr, AWAR_EXPORT_FILE);

    free(outname);
    free(formname);

    if (error) aw_message(error);
}

AW_window *open_AWTC_export_window(AW_root *awr,GBDATA *gb_main)
{
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *) aws;

    aw_create_selection_box_awars(awr, AWAR_EXPORT_FORM, AWT_path_in_ARBHOME("lib/export"), ".eft", "*");
    aw_create_selection_box_awars(awr, AWAR_EXPORT_FILE, "", "", "noname");
    
    awr->awar_string(AWAR_EXPORT_ALI,"16s",AW_ROOT_DEFAULT);
    awr->awar_int(AWAR_EXPORT_MULTIPLE_FILES,0,AW_ROOT_DEFAULT);

    aws = new AW_window_simple;

    aws->init( awr, "ARB_EXPORT", "ARB EXPORT");
    aws->load_xfig("awt/export_db.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"arb_export.hlp");
    aws->create_button("HELP", "HELP","H");

    awt_create_selection_box(aws,AWAR_EXPORT_FILE,"f" );

    awt_create_selection_box(aws,AWAR_EXPORT_FORM,"","ARBHOME", AW_FALSE );

    aws->at("multiple");
    aws->create_toggle(AWAR_EXPORT_MULTIPLE_FILES);

    aws->at("go");
    aws->highlight();
    aws->callback((AW_CB1)AWTC_export_go_cb,(AW_CL)gb_main);
    aws->create_button("GO", "GO","G");

    return (AW_window *) aws;
}
