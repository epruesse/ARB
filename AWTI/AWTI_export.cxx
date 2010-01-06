#include "awti_export.hxx"
#include "awti_exp_local.hxx"

#include <aw_awars.hxx>
#include <aw_global.hxx>
#include <awt.hxx>
#include <awt_filter.hxx>
#include <AP_filter.hxx>
#include <xml.hxx>
#include <inline.h>

#define awte_assert(cond) arb_assert(cond)

using std::string;

// ---------------------------------
//      internal export commands

enum AWTI_EXPORT_CMD {
    // real formats
    AWTI_EXPORT_XML,
    
    AWTI_EXPORT_INVALID,
    AWTI_EXPORT_USING_FORM,        // default mode (has to be last entry in enum)
};

static const char *internal_export_commands[] = {
    "xml_write",
    NULL
};

AWTI_EXPORT_CMD check_internal(const char *command)  {
    AWTI_EXPORT_CMD cmd = AWTI_EXPORT_INVALID;
    for (int i = 0; internal_export_commands[i]; ++i) {
        if (strcmp(command, internal_export_commands[i]) == 0) {
            cmd = static_cast<AWTI_EXPORT_CMD>(i);
        }
    }
    return cmd;
}

// ----------------------
//      export_format

struct export_format {
    char *system;
    // char *internal_command;
    char *new_format;
    char *suffix;
    char *form; // transformed export expression (part behind 'BEGIN')

    enum AWTI_EXPORT_CMD export_mode;

    export_format();
    ~export_format();
};

export_format::export_format(void){
    memset((char *)this,0,sizeof(export_format));
}

export_format::~export_format(void) {
    free(system);
    free(new_format);
    free(suffix);
    free(form);
}

static GB_ERROR awtc_read_export_format(export_format *efo, const char *file, bool load_complete_form){
    GB_ERROR error = 0;

    if (!file || !file[0]) {
        error = "No export format selected";
    }
    else {
        char *fullfile = AWT_unfold_path(file,"ARBHOME");
        FILE *in       = fopen(fullfile,"r");

        if (!in) error = GB_export_IO_error("reading export form", fullfile);
        else {
            char   *s1, *s2;
            size_t  linenumber = 0;

            efo->export_mode = AWTI_EXPORT_USING_FORM; // default mode

            while (!error && awtc_read_string_pair(in, s1, s2, linenumber)) {
                if      (!strcmp(s1, "SYSTEM"))     { reassign(efo->system,     s2); }
                else if (!strcmp(s1, "PRE_FORMAT")) { reassign(efo->new_format, s2); }
                else if (!strcmp(s1, "SUFFIX"))     { reassign(efo->suffix,     s2); }
                else if (!strcmp(s1, "INTERNAL"))   {
                    efo->export_mode = check_internal(s2);
                    if (efo->export_mode == AWTI_EXPORT_INVALID) {
                        error = GBS_global_string("Unknown INTERNAL command '%s'", s2);
                    }
                }
                else if (!strcmp(s1, "BEGIN")) {
                    if (efo->export_mode != AWTI_EXPORT_USING_FORM) {
                        error = "'BEGIN' not allowed when 'INTERNAL' is used";
                    }
                    else {
                        break;
                    }
                }
                else {
                    error = GBS_global_string("Unknown command '%s'", s1);
                }

                // add error location
                if (error) error = GBS_global_string("%s in line #%zu", error, linenumber);

                free(s2);
                free(s1);
            }

            if (!error && load_complete_form && efo->export_mode == AWTI_EXPORT_USING_FORM) {
                // now 'in' points to line behind 'BEGIN'
                char *form = GB_read_fp(in); // read rest of file

                // Join lines that end with \ with next line.
                // Replace ' = ' and ':' by '\=' and '\:'
                efo->form  = GBS_string_eval(form, "\\\\\n=:\\==\\\\\\=:*=\\*\\=*1:\\:=\\\\\\:", 0);
                if (!efo->form) error = GB_failedTo_error("evaluate part below 'BEGIN'", NULL, GB_await_error());
                free(form);
            }

            // some checks for incompatible commands
            if (!error) {
                if      (efo->system && !efo->new_format) error = "Missing 'PRE_FORMAT' (needed by 'SYSTEM')";
                else if (efo->new_format && !efo->system) error = "Missing 'SYSTEM' (needed by 'PRE_FORMAT')";
                else if (efo->export_mode != AWTI_EXPORT_USING_FORM) {
                    if (efo->system)     error = "'SYSTEM' is not allowed together with 'INTERNAL'";
                    if (efo->new_format) error = "'PRE_FORMAT' is not allowed together with 'INTERNAL'";
                }
            }

            error = GB_failedTo_error("read export format", fullfile, error);
            fclose(in);
        }
        free(fullfile);
    }

    return error;
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

    size_t  max_ali_len;                            // length of alignment
    size_t *export_column;                          // list of exported seq data positions
    size_t  columns;                                // how many columns get exported

    GBDATA *single_species;     // if != NULL -> first/next only return that species (used to export to multiple files)

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
        , single_species(0)
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

        if (filter->get_length() < max_ali_len) {
            aw_message(GBS_global_string("Warning: Your filter is shorter than the alignment (%zu<%zu)",
                                         filter->get_length(), max_ali_len));
            max_ali_len = filter->get_length();
        }
    }

    ~export_sequence_data() {
        delete [] export_column;
        delete [] seq;
        free(error);
        free(ali);
    }

    const char *getAlignment() const { return ali; }

    void set_single_mode(GBDATA *gb_species) { single_species = gb_species; }
    bool in_single_mode() const { return single_species; }

    GBDATA *first_species() const { return single_species ? single_species : find_first(gb_main); }
    GBDATA *next_species(GBDATA *gb_prev) const { return single_species ? NULL : find_next(gb_prev); }

    const unsigned char *get_seq_data(GBDATA *gb_species, size_t& slen, GB_ERROR& error) const;
    static bool isGap(char c) { return c == '-' || c == '.'; }

    size_t count_species() {
        awte_assert(!in_single_mode());
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

const unsigned char *export_sequence_data::get_seq_data(GBDATA *gb_species, size_t& slen, GB_ERROR& err) const {
    const char *data   = 0;
    GBDATA     *gb_seq = GBT_read_sequence(gb_species, ali);

    if (!gb_seq) {
        err  = GBS_global_string_copy("No data in alignment '%s' of species '%s'", ali, GBT_read_name(gb_species));
        slen = 0;
    }
    else {
        data = GB_read_char_pntr(gb_seq);
        slen = GB_read_count(gb_seq);
        err  = 0;
    }
    return (const unsigned char *)data;
}


GB_ERROR export_sequence_data::detectVerticalGaps() {
    GB_ERROR err = 0;

    awte_assert(!in_single_mode());

    if (compress == 1) {        // compress vertical gaps!
        size_t  gap_columns = filter->get_filtered_length();
        size_t *gap_column  = new size_t[gap_columns+1];

        const size_t *filterpos_2_seqpos = filter->get_filterpos_2_seqpos();
        memcpy(gap_column, filterpos_2_seqpos, gap_columns*sizeof(*gap_column));
        gap_column[gap_columns] = max_ali_len;

        size_t spec_count  = count_species();
        size_t stat_update = spec_count/1000;

        if (stat_update == 0) stat_update = 1;

        size_t count         = 0;
        size_t next_stat     = count+stat_update;

        aw_status("Calculating vertical gaps");
        aw_status(0.0);

        for (GBDATA *gb_species = first_species();
             gb_species && !err;
             gb_species = next_species(gb_species))
        {
            size_t               slen;
            const unsigned char *sdata = get_seq_data(gb_species, slen, err);

            if (!err) {
                size_t j = 0;
                size_t i;
                for (i = 0; i<gap_columns; ++i) {
                    if (isGap(sdata[gap_column[i]])) {
                        gap_column[j++] = gap_column[i]; // keep gap column
                    }
                    // otherwise it's overwritten
                }

                awte_assert(i >= j);
                size_t skipped_columns  = i-j;
                awte_assert(gap_columns >= skipped_columns);
                gap_columns            -= skipped_columns;
            }
            ++count;
            if (count >= next_stat) {
                if (aw_status(count/double(spec_count))) err = "User abort";
                next_stat = count+stat_update;
            }
        }

        aw_status(1.0);

        if (!err) {
            columns       = filter->get_filtered_length() - gap_columns;
            export_column = new size_t[columns];

            size_t gpos = 0;           // index into array of vertical gaps
            size_t epos = 0;           // index into array of exported columns
            size_t flen = filter->get_filtered_length();
            size_t a;
            for (a = 0; a<flen && gpos<gap_columns; ++a) {
                size_t fpos = filterpos_2_seqpos[a];
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
                export_column[epos++] = filterpos_2_seqpos[a];
            }

            awte_assert(epos == columns);
        }

        delete [] gap_column;
    }
    else { // compress all or none (simply use filter)
        const size_t *filterpos_2_seqpos = filter->get_filterpos_2_seqpos();

        columns       = filter->get_filtered_length();
        export_column = new size_t[columns];

        memcpy(export_column, filterpos_2_seqpos, columns*sizeof(*filterpos_2_seqpos));
    }

    seq = new char[columns+1];

    return err;
}

const char *export_sequence_data::get_export_sequence(GBDATA *gb_species, size_t& seq_len, GB_ERROR& err) {
    if (gb_species != last_species_read) {
        freeset(error, 0);

        // read + filter a new species
        GB_ERROR             curr_error;
        const unsigned char *data = get_seq_data(gb_species, len, curr_error);

        if (curr_error) {
            error = strdup(curr_error);
        }
        else {
            size_t       i;
            const uchar *simplify = filter->get_simplify_table();

            if (cut_stop_codon) {
                const unsigned char *stop_codon = (const unsigned char *)memchr(data, '*', len);
                if (stop_codon) {
                    len = stop_codon-data;
                }
            }

            if (compress == 2) { // compress all gaps
                size_t j = 0;
                for (i = 0; i<columns; ++i) {
                    size_t seq_pos = export_column[i];
                    if (seq_pos<len) {
                        unsigned char c = data[seq_pos];
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
        
        const char *name = GBT_read_char_pntr(gbd, "name");
        if (name) tag->add_attribute("name", name);
    }

    if (descend) {
        switch (GB_read_type(gbd)) {
            case GB_DB: {
                for (GBDATA *gb_child = GB_child(gbd); gb_child && !error; gb_child = GB_nextChild(gb_child)) {
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

static GB_ERROR export_species_using_form(FILE *out, GBDATA *gb_species, const char *form) {
    GB_ERROR  error  = NULL;
    char     *pars   = GBS_string_eval(" ", form, gb_species);
    if (!pars) error = GB_await_error();
    else {
        char *p;
        char *o = pars;
        while ( (p = GBS_find_string(o,"$$DELETE_LINE$$",0)) ) {
            char *l,*r;
            for (l = p; l>o; l--) if (*l=='\n') break;
            r = strchr(p,'\n'); if (!r) r = p +strlen(p);
            fwrite(o,1,l-o,out);
            o = r;
        }
        fputs(o, out);
        free(pars);
    }
    return error;
}

static GB_ERROR AWTI_export_format(AW_root *aw_root, const char *formname, const char *outname, char **resulting_outname) {
    // Exports sequences specified by 'esd' (module global variable)
    // to format specified by 'formname'.
    //
    // if 'outname' == NULL -> export species to temporary file, otherwise to 'outname'.
    // Full path of generated file is returned in 'resulting_outname'

    static int export_depth     = 0;
    static int export_depth_max = 0;
    export_depth++;

    *resulting_outname = 0;

    export_format efo;
    GB_ERROR      error = awtc_read_export_format(&efo, formname, true);

    if (!error) {
        if (!outname) { // if no 'outname' is given -> export to temporary file
            *resulting_outname = GB_create_tempfile(GB_unique_filename("exported", efo.suffix));
            if (!*resulting_outname) error = GB_await_error();
        }
        else *resulting_outname = strdup(outname);
    }
    if (!error) {
        if (efo.new_format) {
            // Export data using format 'new_format'.
            // Afterwards convert to wanted format using 'system'.

            awte_assert(efo.system);
            
            char *intermediate_export;
            error = AWTI_export_format(aw_root, efo.new_format, NULL, &intermediate_export);
            if (!error) {
                awte_assert(GB_is_privatefile(intermediate_export, GB_FALSE));

                aw_status(GBS_global_string("Converting to %s", efo.suffix));

                char *srt = GBS_global_string_copy("$<=%s:$>=%s", intermediate_export, *resulting_outname);
                char *sys = GBS_string_eval(efo.system, srt, 0);

                aw_status(GBS_global_string("exec '%s'", efo.system));
                error = GB_system(sys);

                GB_unlink_or_warn(intermediate_export, &error);
                aw_status(1 - double(export_depth-1)/export_depth_max);

                free(sys);
                free(srt);
            }
            free(intermediate_export);
        }
        else {
            FILE *out       = fopen(*resulting_outname, "wt");
            if (!out) error = GB_export_IO_error("writing", *resulting_outname);
            else {
                XML_Document *xml = 0;

                aw_status("Saving data");
                export_depth_max = export_depth;

                if (efo.export_mode == AWTI_EXPORT_XML) {
                    xml = new XML_Document("ARB_SEQ_EXPORT", "arb_seq_export.dtd", out);
                    {
                        char *db_name = aw_root->awar(AWAR_DB_NAME)->read_string();
                        xml->add_attribute("database", db_name);
                        free(db_name);
                    }
                    xml->add_attribute("export_date", GB_date_string());
                    {
                        char *fulldtd = AWT_unfold_path("lib/dtd", "ARBHOME");
                        XML_Comment rem(GBS_global_string("There's a basic version of ARB_seq_export.dtd in %s\n"
                                                          "but you might need to expand it by yourself,\n"
                                                          "because the ARB-database may contain any kind of fields.",
                                                          fulldtd));
                        free(fulldtd);
                    }
                }

                int allCount    = 0;
                for (GBDATA *gb_species = esd->first_species();
                     gb_species && !error;
                     gb_species = esd->next_species(gb_species))
                {
                    allCount++;
                }

                int count       = 0;
                for (GBDATA *gb_species = esd->first_species();
                     gb_species && !error;
                     gb_species = esd->next_species(gb_species))
                {
                    aw_status(GBS_global_string("Saving species %i/%i", ++count, allCount));
                    switch (efo.export_mode) {
                        case AWTI_EXPORT_USING_FORM:
                            error = export_species_using_form(out, gb_species, efo.form);
                            break;

                        case AWTI_EXPORT_XML: 
                            error = AWTI_XML_recursive(gb_species);
                            break;

                        case AWTI_EXPORT_INVALID:
                            awte_assert(0);
                            break;
                    }
                    aw_status(double(count)/allCount);
                }

                delete xml;
                fclose(out);
            }
        }
    }

    if (error) {
        if (*resulting_outname) {
            GB_unlink_or_warn(*resulting_outname, NULL);
            freeset(*resulting_outname, 0);
        }
    }

    export_depth--;

    return error;
}

static GB_ERROR AWTI_export_format_multiple(AW_root *aw_root, const char *formname, const char *outname, bool multiple, char **resulting_outname) {
    GB_ERROR error = 0;

    aw_status(0.);
    
    if (multiple) {
        char *path, *name, *suffix;
        GB_split_full_path(outname, &path, NULL, &name, &suffix);

        *resulting_outname = NULL;

        size_t species_count = esd->count_species();
        size_t count         = 0;

        for (GBDATA *gb_species = esd->first_species();
             gb_species && !error;
             gb_species = esd->next_species(gb_species))
        {
            const char *species_name = GBT_read_char_pntr(gb_species, "name");
            if (!species_name) error = "Can't export unnamed species";
            else {
                const char *fname = GB_append_suffix(GBS_global_string("%s_%s", name, species_name), suffix);
                aw_status(fname);

                char *oname = strdup(GB_concat_path(path, fname));
                char *res_oname;

                esd->set_single_mode(gb_species); // means: only export 'gb_species'
                error = AWTI_export_format(aw_root, formname, oname, &res_oname);
                esd->set_single_mode(NULL);

                aw_status(++count/double(species_count));

                if (!*resulting_outname || // not set yet
                    (res_oname && strcmp(*resulting_outname, res_oname)>0)) // or smaller than set one
                {
                    reassign(*resulting_outname, res_oname);
                }

                free(res_oname);
                free(oname);
            }
        }
    }
    else {
        error = AWTI_export_format(aw_root, formname, outname, resulting_outname);
    }

    return error;
}

// ----------------------------------------

void AWTC_export_go_cb(AW_window *aww, AW_CL cl_gb_main, AW_CL res_from_awt_create_select_filter) {
    GBDATA           *gb_main = (GBDATA*)cl_gb_main;
    GB_transaction    dummy(gb_main);
    adfiltercbstruct *acbs    = (adfiltercbstruct*)res_from_awt_create_select_filter;

    aw_openstatus("Exporting data");

    AW_root  *awr            = aww->get_root();
    char     *formname       = awr->awar(AWAR_EXPORT_FORM"/file_name")->read_string();
    int       multiple       = awr->awar(AWAR_EXPORT_MULTIPLE_FILES)->read_int();
    int       marked_only    = awr->awar(AWAR_EXPORT_MARKED)->read_int();
    int       cut_stop_codon = awr->awar(AWAR_EXPORT_CUTSTOP)->read_int();
    int       compress       = awr->awar(AWAR_EXPORT_COMPRESS)->read_int();
    GB_ERROR  error          = 0;

    char *outname      = awr->awar(AWAR_EXPORT_FILE"/file_name")->read_string();
    char *real_outname = 0;     // with suffix (name of first file if multiple)

    AP_filter *filter = awt_get_filter(awr, acbs);
    esd               = new export_sequence_data(gb_main, marked_only, filter, cut_stop_codon, compress);
    GB_set_export_sequence_hook(exported_sequence);

    error = esd->detectVerticalGaps();
    if (!error) {
        error = AWTI_export_format_multiple(awr, formname, outname, multiple, &real_outname);
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
    aw_create_selection_box_awars(awr, AWAR_EXPORT_FORM, GB_path_in_ARBLIB("export", NULL), ".eft", "*", AW_ROOT_DEFAULT, true);
    aw_create_selection_box_awars(awr, AWAR_EXPORT_FILE, "", "", "noname");

    awr->awar_string(AWAR_EXPORT_ALI,"16s",def);
    awr->awar_int(AWAR_EXPORT_MULTIPLE_FILES, 0, def);

    awr->awar_int(AWAR_EXPORT_MARKED, 1, def); // marked only
    awr->awar_int(AWAR_EXPORT_COMPRESS, 1, def); // vertical gaps
    awr->awar_string(AWAR_EXPORT_FILTER_NAME, "none", def); // no default filter
    awr->awar_string(AWAR_EXPORT_FILTER_FILTER, "", def);
    AW_awar *awar_ali = awr->awar_string(AWAR_EXPORT_FILTER_ALI, "", def);
    awar_ali->map("presets/use"); // map to default alignment

    awr->awar_int(AWAR_EXPORT_CUTSTOP, 0, def); // don't cut stop-codon
}

static char *get_format_default_suffix(const char *formname, GB_ERROR& error) {
    export_format efs;
    error = awtc_read_export_format(&efs, formname, false);

    if (!error && efs.suffix) return strdup(efs.suffix);
    return NULL;
}

static void export_form_changed_cb(AW_root *aw_root) {
    // called when selected export format changes
    // -> automatically correct filename suffix
    // -> restrict view to suffix

    static char *previous_suffix = 0;

    GB_ERROR  error          = 0;
    AW_awar  *awar_form      = aw_root->awar(AWAR_EXPORT_FORM"/file_name");
    char     *current_format = awar_form->read_string();

    if (current_format) {
        if (GB_is_regularfile(current_format)) {
            char *current_suffix = get_format_default_suffix(current_format, error);
            if (!error) {
                // Note: current_suffix may be NULL.. is that ok? 
            
                // modify export filename and view

                AW_awar *awar_filter = aw_root->awar(AWAR_EXPORT_FILE"/filter");
                AW_awar *awar_export = aw_root->awar(AWAR_EXPORT_FILE"/file_name");

                awar_filter->write_string("");
                
                char *exportname = awar_export->read_string();

                {
                    char *path, *nameOnly, *suffix;
                    GB_split_full_path(exportname, &path, NULL, &nameOnly, &suffix);

                    if (suffix) {
                        if (previous_suffix && ARB_stricmp(suffix, previous_suffix) == 0) freedup(suffix, current_suffix); // remove old suffix
                        else freedup(suffix, GB_append_suffix(suffix, current_suffix)); // don't know existing suffix -> append
                    }
                    else suffix = strdup(current_suffix);

                    const char *new_exportname = GB_concat_path(path, GB_append_suffix(nameOnly, suffix));
                    if (new_exportname) awar_export->write_string(new_exportname);

                    free(suffix);
                    free(nameOnly);
                    free(path);
                }

                free(exportname);

                awar_filter->write_string(current_suffix);
                
                // remember last applied suffix
                reassign(previous_suffix, current_suffix);
            }

            free(current_suffix);
        }
        free(current_format);
    }

    if (error) aw_message(error);
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

    awt_create_selection_box(aws,AWAR_EXPORT_FORM,"","ARBHOME", false );

    aws->get_root()->awar(AWAR_EXPORT_FORM"/file_name")->add_callback(export_form_changed_cb);

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

    aws->at("seqfilter");
    adfiltercbstruct *filtercd = awt_create_select_filter(aws->get_root(), gb_main, AWAR_EXPORT_FILTER_NAME);
    aws->callback(AW_POPUP, (AW_CL)awt_create_select_filter_win, (AW_CL)filtercd);
    aws->create_button("SELECT_FILTER", AWAR_EXPORT_FILTER_NAME);

    aws->at("cutstop");
    aws->create_toggle(AWAR_EXPORT_CUTSTOP);

    aws->at("multiple");
    aws->create_toggle(AWAR_EXPORT_MULTIPLE_FILES);

    aws->at("go");
    aws->highlight();
    aws->callback(AWTC_export_go_cb,(AW_CL)gb_main, (AW_CL)filtercd);
    aws->create_button("GO", "GO","G");

    return aws;
}
