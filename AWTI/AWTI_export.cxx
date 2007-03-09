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
#include <awt_tree.hxx>
#include <inline.h>

#include "awti_export.hxx"
#include "awti_exp_local.hxx"
#include "awt_sel_boxes.hxx"
#include "aw_awars.hxx"

#include "xml.hxx"

#define awte_assert(cond) arb_assert(cond)

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


char *awtc_read_export_format(export_format_struct * efo, const char *file){
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

// ----------------------------------------
// export sequence helper class

typedef GBDATA *(*FindSpeciesFunction)(GBDATA *);

class export_sequence_data {
    GBDATA *last_species_read;
    char   *seq;
    size_t  len;
    char   *error;

    GBDATA              *gb_main;
    char                *ali;
    FindSpeciesFunction  find_first, find_next;
    size_t               species_count;
    AP_filter           *filter;
    bool                 cut_stop_codon;
    int                  compress; // 0 = no;1 = vertical gaps; 2 = all gaps;

    size_t  max_ali_len;        // length of alignment
    int    *export_column;      // list of exported seq data positions
    int     columns;            // how many columns get exported

public:

    export_sequence_data(GBDATA *Gb_Main, bool only_marked, AP_filter* Filter, bool CutStopCodon, int Compress)
        : last_species_read(0)
        , seq(0)
        , len(0), error(0)
        , gb_main(Gb_Main), species_count(size_t(-1))
        , filter(Filter)
        , cut_stop_codon(CutStopCodon)
        , compress(Compress)
        , export_column(0)
        , columns(0)
    {
        ali         = GBT_get_default_alignment(gb_main);
        max_ali_len = GBT_get_alignment_len(gb_main, ali);

        if (cut_stop_codon) {
            GB_alignment_type ali_type = GBT_get_alignment_type(gb_main, ali);
            if (ali_type !=  GB_AT_AA) {
                aw_message("Cutting stop codon makes no sense - ignored");
                cut_stop_codon = false;
            }
        }
        awte_assert(filter);

        if (only_marked) {
            find_first = GBT_first_marked_species;
            find_next  = GBT_next_marked_species;
        }
        else {
            find_first = GBT_first_species;
            find_next  = GBT_next_species;
        }

        if (size_t(filter->filter_len) < max_ali_len) {
            aw_message(GBS_global_string("Warning: Your filter is shorter than the alignment (%li<%u)",
                                         filter->filter_len, max_ali_len));
            max_ali_len = filter->filter_len;
        }
    }

    ~export_sequence_data() {
        delete [] export_column;
        delete [] seq;
        free(error);
        free(ali);
    }

    const char *getAlignment() const { return ali; }

    GBDATA *first_species() const { return find_first(gb_main); }
    GBDATA *next_species(GBDATA *gb_prev) const { return find_next(gb_prev); }

    const char *get_seq_data(GBDATA *gb_species, size_t& slen, GB_ERROR& error) const ;
    static bool isGap(char c) { return c == '-' || c == '.'; }

    size_t count_species() {
        if (species_count == size_t(-1)) {
            species_count = 0;
            for (GBDATA *gb_species = find_first(gb_main); gb_species; gb_species = find_next(gb_species)) {
                species_count++;
            }
        }
        return species_count;
    }

    GB_ERROR    detectVerticalGaps();
    const char *get_export_sequence(GBDATA *gb_species, size_t& seq_len, GB_ERROR& error);
};

const char *export_sequence_data::get_seq_data(GBDATA *gb_species, size_t& slen, GB_ERROR& error) const {
    const char *data   = 0;
    GBDATA     *gb_seq = GBT_read_sequence(gb_species, ali);
    if (!gb_seq) {
        GBDATA     *gb_name = GB_find(gb_species, "name", 0, down_level);
        const char *name    = gb_name ? GB_read_char_pntr(gb_name) : "<unknown species>";
        error               = GBS_global_string_copy("No data in alignment '%s' of species '%s'", ali, name);
        slen                = 0;
    }
    else {
        data  = GB_read_char_pntr(gb_seq);
        slen  = GB_read_count(gb_seq);
        error = 0;
    }
    return data;
}


GB_ERROR export_sequence_data::detectVerticalGaps() {
    GB_ERROR error = 0;

    filter->calc_filter_2_seq();
    if (compress == 1) {        // compress vertical gaps!
        int  gap_columns        = filter->real_len;
        int *gap_column         = new int[gap_columns+1];
        memcpy(gap_column, filter->filterpos_2_seqpos, gap_columns*sizeof(*gap_column));
        gap_column[gap_columns] = max_ali_len;

        size_t species_count = count_species();
        size_t stat_update   = species_count/1000;

        if (stat_update == 0) stat_update = 1;

        size_t count         = 0;
        size_t next_stat     = count+stat_update;

        aw_status("Calculating vertical gaps");
        aw_status(0.0);

        for (GBDATA *gb_species = first_species();
             gb_species && !error;
             gb_species = next_species(gb_species))
        {
            size_t      slen;
            const char *sdata = get_seq_data(gb_species, slen, error);

            if (!error) {
                int j = 0;
                int i;
                for (i = 0; i<gap_columns; ++i) {
                    if (isGap(sdata[gap_column[i]])) {
                        gap_column[j++] = gap_column[i]; // keep gap column
                    }
                    // otherwise it's overwritten
                }

                int skipped_columns  = i-j;
                gap_columns      -= skipped_columns;
                awte_assert(gap_columns >= 0);
            }
            ++count;
            if (count >= next_stat) {
                if (aw_status(count/double(species_count))) error = "User abort";
                next_stat = count+stat_update;
            }
        }

        aw_status(1.0);

        if (!error) {
            columns       = filter->real_len - gap_columns;
            export_column = new int[columns];

            int gpos = 0;           // index into array of vertical gaps
            int epos = 0;           // index into array of exported columns
            int flen = filter->real_len;
            int a;
            for (a = 0; a<flen && gpos<gap_columns; ++a) {
                int fpos = filter->filterpos_2_seqpos[a];
                if (fpos == gap_column[gpos]) { // only gaps here -> skip column
                    gpos++;
                }
                else { // not only gaps -> use column
                    awte_assert(fpos<gap_column[gpos]);
                    awte_assert(epos < columns); // got more columns than expected
                    export_column[epos++] = fpos;
                }
            }
            for (; a<flen; ++a) {
                export_column[epos++] = filter->filterpos_2_seqpos[a];
            }

            awte_assert(epos == columns);
        }

        delete [] gap_column;
        delete [] filter->filterpos_2_seqpos;
        filter->filterpos_2_seqpos = 0;
    }
    else { // compress all or none (simply use filter)
        export_column              = filter->filterpos_2_seqpos;
        filter->filterpos_2_seqpos = 0;
        columns                    = filter->real_len;
    }

    seq = new char[columns+1];

    return error;
}

const char *export_sequence_data::get_export_sequence(GBDATA *gb_species, size_t& seq_len, GB_ERROR& err) {
    if (gb_species != last_species_read) {
        if (error) {
            free(error);
            error = 0;
        }

        // read + filter a new species
        GB_ERROR    curr_error;
        const char *data = get_seq_data(gb_species, len, curr_error);

        if (curr_error) {
            error = strdup(curr_error);
        }
        else {
            int          i;
            const uchar *simplify = filter->simplify;

            if (cut_stop_codon) {
                char *stop_codon = (char*)memchr(data, '*', len);
                if (stop_codon) {
                    len = stop_codon-data;
                }
            }

            if (compress == 2) { // compress all gaps
                int j = 0;
                for (i = 0; i<columns; ++i) {
                    size_t seq_pos = export_column[i];
                    if (seq_pos<len) {
                        char c = data[seq_pos];
                        if (!isGap(c)) {
                            seq[j++] = simplify[c];
                        }
                    }
                }
                seq[j] = 0;
                len    = j;
            }
            else { // compress vertical or compress none (simply use filter in both cases)
                for (i = 0; i<columns; ++i) {
                    size_t seq_pos = export_column[i];
                    if (seq_pos<len) {
                        seq[i] = simplify[data[seq_pos]];
                    }
                    else {
                        seq[i] = simplify['.'];
                    }
                }
                seq[i] = 0;
                len    = columns;
            }
        }
    }

    err = error;
    if (error) {
        seq_len  = 0;
        return 0;
    }

    seq_len  = len;
    return seq;
}

// ----------------------------------------
// exported_sequence is hooked into ACI temporary (provides result of command 'export_sequence')
// which is the sequence filtered and compressed according to settings in the export window

static export_sequence_data *esd = 0;

extern "C" const char *exported_sequence(GBDATA *gb_species, size_t *seq_len, GB_ERROR *error) {
    awte_assert(esd);
    return esd->get_export_sequence(gb_species, *seq_len, *error);
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

static GB_ERROR AWTI_XML_recursive(GBDATA *gbd) {
    GB_ERROR    error    = 0;
    const char *key_name = GB_read_key_pntr(gbd);
    XML_Tag    *tag      = 0;
    bool        descend  = true;

    if (strncmp(key_name, "ali_", 4) == 0)
    {
        awte_assert(esd);
        descend = false; // do not descend into alignments
        if (strcmp(esd->getAlignment(), key_name) == 0) { // the wanted alignment

            tag = new XML_Tag("ALIGNMENT");
            tag->add_attribute("name", key_name+4);

            GBDATA     *gb_species = GB_get_father(gbd);
            size_t      len;
            const char *seq        = exported_sequence(gb_species, &len, &error);

            if (seq) {
                XML_Tag dtag("data");
                { XML_Text seqText(seq); }
            }
        }
    }
    else {
        tag = new XML_Tag(key_name);

        GBDATA *gb_name = GB_find(gbd, "name", 0, down_level);
        if (gb_name) {
            tag->add_attribute("name", GB_read_char_pntr(gb_name));
        }
    }

    if (descend) {
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
    }

    delete tag;
    return error;
}

static GB_ERROR AWTI_export_format(AW_root *aw_root, GBDATA *gb_main,
                                   const char *formname, const char *outname,
                                   bool multiple, char **resulting_outname)
    // if resulting_outname != NULL -> set *resulting_outname to filename with suffix appended
    // (not done when saving to multiple files!)
{
    char            *fullformname = AWT_unfold_path(formname,"ARBHOME");
    GB_ERROR         error        = 0;
    char            *form         = GB_read_file(fullformname);
    AWTI_EXPORT_CMD  cmd          = AWTI_EXPORT_BY_FORM;

    if (!form || form[0] == 0) {
        if (!formname || formname[0] == 0) error = GB_export_error("Please select a form");
        else error                               = GB_export_IO_error("loading export form", fullformname);
    }
    else {
        // skip header till line starting with 'BEGIN'.
        // join lines that end with \ with next line
        // replace '=' and ':' by '\=' and '\:'
        //
        char *form2  = GBS_string_eval(form,"*\nBEGIN*\n*=*3:\\\\\n=:\\==\\\\\\=:*=\\*\\=*1:\\:=\\\\\\:",0);

        if (!form2) error = (char *)GB_get_error();
        free(form);
        form              = form2;
    }

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
                error = AWTI_export_format(aw_root, gb_main, efo->new_format, intermediate, false, &intermediate_resulting);

                if (!error) {
                    sprintf(srt,"$<=%s:$>=%s",intermediate_resulting, outname);
                    char *sys = GBS_string_eval(efo->system,srt,0);
                    sprintf(AW_ERROR_BUFFER,"exec '%s'",efo->system);
                    aw_status(AW_ERROR_BUFFER);
                    if (system(sys)) {
                        sprintf(AW_ERROR_BUFFER,"Error in '%s'",sys);
                        error = AW_ERROR_BUFFER;
                    }
                    free(sys);
                }
                free(intermediate_resulting);
            }else{
                error = AWTI_export_format(aw_root, gb_main, efo->new_format, outname, multiple, NULL);
            }
            goto end_of_AWTI_export_format;
        }
    }

    if (!error) {
        char       *outname_nosuffix = 0;
        const char *existing_suffix  = strrchr(outname, '.');

        if (existing_suffix && ARB_stricmp(existing_suffix+1, efo->suffix) == 0) {
            size_t nosuf_len            = existing_suffix-outname;
            outname_nosuffix            = (char*)malloc(nosuf_len+1);
            memcpy(outname_nosuffix, outname, nosuf_len);
            outname_nosuffix[nosuf_len] = 0;
        }
        else {
            outname_nosuffix = strdup(outname);
        }

        try {
            FILE         *out          = 0;
            char         *curr_outname = 0;
            XML_Document *xml          = 0;

            if (!error && !multiple) {
                curr_outname = GBS_global_string_copy("%s.%s", outname_nosuffix, efo->suffix);
                if (resulting_outname != 0) *resulting_outname = strdup(curr_outname);

                out = fopen(curr_outname, "wt");
                if (!out) error = GBS_global_string("Can't write to file '%s'", curr_outname);
            }

            size_t species_count        = esd->count_species();
            int    stat_mod             = species_count/400;
            if (stat_mod == 0) stat_mod = 1;

            int count     = 0;
            int next_stat = count+stat_mod;

            aw_status("Save data");
            aw_status(0.0);

            for (GBDATA *gb_species = esd->first_species();
                 !error && gb_species;
                 gb_species = esd->next_species(gb_species))
            {
                if (multiple) {
                    char *name = GBT_read_name(gb_species);

                    curr_outname = GBS_global_string_copy("%s_%s.%s",outname_nosuffix, name, efo->suffix);
                    free(name);

                    out = fopen(curr_outname, "wt");
                    if (!out) error = GBS_global_string("Can't write to file '%s'", curr_outname);
                }

                if (!error) {
                    switch (cmd) {
                        case AWTI_EXPORT_BY_FORM: {
                            char *pars = GBS_string_eval(" ", form, gb_species);
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
                            awte_assert(0);
                            break;
                        }
                    }
                }

                if (multiple) {
                    if (out) fclose(out);
                    out = 0;

                    if (error) unlink(curr_outname);

                    free(curr_outname);
                    curr_outname = 0;
                }

                count++;
                if (!error && count >= next_stat) {
                    if (aw_status(count/double(species_count))) error = "User abort";

                    next_stat = count+stat_mod;
                }
            }

            delete xml;
            if (out) fclose(out);
            free(curr_outname);
        }
        catch (string& err) {
            error = GB_export_error("Error: %s (programmers error)", err.c_str());
        }

        free(outname_nosuffix);
    }

 end_of_AWTI_export_format:

    free(fullformname);
    free(form);
    delete efo;

    return error;
}

// ----------------------------------------

void AWTC_export_go_cb(AW_window *aww, AW_CL cl_gb_main, AW_CL res_from_awt_create_select_filter) {
    GBDATA         *gb_main = (GBDATA*)cl_gb_main;
    GB_transaction  dummy(gb_main);

    aw_openstatus("Exporting Data");

    AW_root  *awr            = aww->get_root();
    char     *formname       = awr->awar(AWAR_EXPORT_FORM"/file_name")->read_string();
    int       multiple       = awr->awar(AWAR_EXPORT_MULTIPLE_FILES)->read_int();
    int       marked_only    = awr->awar(AWAR_EXPORT_MARKED)->read_int();
    int       cut_stop_codon = awr->awar(AWAR_EXPORT_CUTSTOP)->read_int();
    int       compress       = awr->awar(AWAR_EXPORT_COMPRESS)->read_int();
    GB_ERROR  error          = 0;

    char *outname      = awr->awar(AWAR_EXPORT_FILE"/file_name")->read_string();
    char *real_outname = 0;     // with suffix

    AP_filter *filter = awt_get_filter(awr, res_from_awt_create_select_filter);
    esd               = new export_sequence_data(gb_main, marked_only, filter, cut_stop_codon, compress);
    GB_set_export_sequence_hook(exported_sequence);

    error = esd->detectVerticalGaps();
    if (!error) {
        error = AWTI_export_format(awr, gb_main, formname, outname, multiple, &real_outname);
    }
    GB_set_export_sequence_hook(0);
    delete esd;
    esd   = 0;

    aw_closestatus();
    if (error) aw_message(error);

    if (real_outname) awr->awar(AWAR_EXPORT_FILE"/file_name")->write_string(real_outname);

    awt_refresh_selection_box(awr, AWAR_EXPORT_FILE);

    free(real_outname);
    free(outname);
    free(formname);

}

void AWTC_create_export_awars(AW_root *awr, AW_default def) {
    aw_create_selection_box_awars(awr, AWAR_EXPORT_FORM, AWT_path_in_ARBHOME("lib/export"), ".eft", "*");
    aw_create_selection_box_awars(awr, AWAR_EXPORT_FILE, "", "", "noname");

    awr->awar_string(AWAR_EXPORT_ALI,"16s",def);
    awr->awar_int(AWAR_EXPORT_MULTIPLE_FILES, 0, def);

    awr->awar_int(AWAR_EXPORT_MARKED, 1, def); // marked only
    awr->awar_int(AWAR_EXPORT_COMPRESS, 1, def); // vertical gaps
    awr->awar_string(AWAR_EXPORT_FILTER_NAME, "none", def); // no default filter
    awr->awar_string(AWAR_EXPORT_FILTER_FILTER, "", def);
    AW_awar *awar_ali = awr->awar_string(AWAR_EXPORT_FILTER_ALI, "", def);
    awar_ali->map("presets/use"); // map to default alignment

    awr->awar_int(AWAR_EXPORT_CUTSTOP, 0, def); // dont cut stop-codon
}

AW_window *open_AWTC_export_window(AW_root *awr,GBDATA *gb_main)
{
    static AW_window_simple *aws = 0;
    if (aws) return aws;

    AWTC_create_export_awars(awr, AW_ROOT_DEFAULT);

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

    aws->at("allmarked");
    aws->create_option_menu(AWAR_EXPORT_MARKED);
    aws->insert_option("all", "a", 0);
    aws->insert_option("marked", "m", 1);
    aws->update_option_menu();

    aws->at("compress");
    aws->create_option_menu(AWAR_EXPORT_COMPRESS);
    aws->insert_option("no", "n", 0);
    aws->insert_option("vertical gaps", "v", 1);
    aws->insert_option("all gaps", "a", 2);
    aws->update_option_menu();

    aws->at("filter");
    AW_CL filtercd = awt_create_select_filter(aws->get_root(), gb_main, AWAR_EXPORT_FILTER_NAME);
    aws->callback(AW_POPUP, (AW_CL)awt_create_select_filter_win, filtercd);
    aws->create_button("SELECT_FILTER", AWAR_EXPORT_FILTER_NAME);

    aws->at("cutstop");
    aws->create_toggle(AWAR_EXPORT_CUTSTOP);

    aws->at("multiple");
    aws->create_toggle(AWAR_EXPORT_MULTIPLE_FILES);

    aws->at("go");
    aws->highlight();
    aws->callback(AWTC_export_go_cb,(AW_CL)gb_main, filtercd);
    aws->create_button("GO", "GO","G");

    return aws;
}
