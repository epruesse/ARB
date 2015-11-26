// ============================================================= //
//                                                               //
//   File      : seq_export.cxx                                  //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "seqio.hxx"

#include <AP_filter.hxx>
#include <arbdbt.h>
#include <arb_strarray.h>
#include <arb_file.h>
#include <arb_diff.h>
#include <xml.hxx>
#include <arb_progress.h>
#include <unistd.h>

#define sio_assert(cond) arb_assert(cond)

using std::string;


// ---------------------------------
//      internal export commands

enum EXPORT_CMD {
    // real formats
    EXPORT_XML,

    EXPORT_INVALID,
    EXPORT_USING_FORM,        // default mode (has to be last entry in enum)
};

static const char *internal_export_commands[] = {
    "xml_write",
    NULL
};

static EXPORT_CMD check_internal(const char *command) {
    EXPORT_CMD cmd = EXPORT_INVALID;
    for (int i = 0; internal_export_commands[i]; ++i) {
        if (strcmp(command, internal_export_commands[i]) == 0) {
            cmd = static_cast<EXPORT_CMD>(i);
        }
    }
    return cmd;
}

// ----------------------
//      export_format

struct export_format : virtual Noncopyable {
    char *system;
    char *new_format;
    char *suffix;
    char *form; // transformed export expression (part behind 'BEGIN')

    EXPORT_CMD export_mode;

    export_format()
        : system(NULL),
          new_format(NULL),
          suffix(NULL),
          form(NULL),
          export_mode(EXPORT_XML)
    {}
    ~export_format() {
        free(system);
        free(new_format);
        free(suffix);
        free(form);
    }
};

static GB_ERROR read_export_format(export_format *efo, const char *file, bool load_complete_form) {
    GB_ERROR error = 0;

    if (!file || !file[0]) {
        error = "No export format selected";
    }
    else {
        char *fullfile = 0;
        if (GB_is_regularfile(file)) { // prefer files that are completely specified (full/rel path)
            fullfile = strdup(GB_canonical_path(file));
        }
        else {
            fullfile = nulldup(GB_path_in_ARBHOME(file)); // fallback to ARBHOME-relative specification
        }

        FILE *in = fopen(fullfile, "r");

        if (!in) error = GB_IO_error("reading export form", fullfile);
        else {
            efo->export_mode = EXPORT_USING_FORM; // default mode
            {
                bool    seen_BEGIN = false;
                char   *s1, *s2;
                size_t  linenumber = 0;

                while (!error && !seen_BEGIN && SEQIO_read_string_pair(in, s1, s2, linenumber)) {
                    if      (!strcmp(s1, "SYSTEM"))     { reassign(efo->system,     s2); }
                    else if (!strcmp(s1, "PRE_FORMAT")) { reassign(efo->new_format, s2); }
                    else if (!strcmp(s1, "SUFFIX"))     { reassign(efo->suffix,     s2); }
                    else if (!strcmp(s1, "INTERNAL")) {
                        efo->export_mode = check_internal(s2);
                        if (efo->export_mode == EXPORT_INVALID) {
                            error = GBS_global_string("Unknown INTERNAL command '%s'", s2);
                        }
                    }
                    else if (!strcmp(s1, "BEGIN")) {
                        if (efo->export_mode != EXPORT_USING_FORM) {
                            error = "'BEGIN' not allowed when 'INTERNAL' is used";
                        }
                        else {
                            seen_BEGIN = true;
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
            }

            if (!error && load_complete_form && efo->export_mode == EXPORT_USING_FORM) {
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
                else if (efo->export_mode != EXPORT_USING_FORM) {
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

char *SEQIO_exportFormat_get_outfile_default_suffix(const char *formname, GB_ERROR& error) {
    export_format efs;
    error = read_export_format(&efs, formname, false);
    return (!error && efs.suffix) ? strdup(efs.suffix) : NULL;
}



// ----------------------------------------
// export sequence helper class

typedef GBDATA *(*FindSpeciesFunction)(GBDATA *);

class export_sequence_data : virtual Noncopyable {
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

    long    max_ali_len;                            // length of alignment
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
                GB_warning("Cutting stop codon makes no sense - ignored");
                cut_stop_codon = false;
            }
        }
        sio_assert(filter);

        if (only_marked) {
            find_first = GBT_first_marked_species;
            find_next  = GBT_next_marked_species;
        }
        else {
            find_first = GBT_first_species;
            find_next  = GBT_next_species;
        }

        if (max_ali_len>=0 && filter->get_length() < size_t(max_ali_len)) {
            GB_warningf("Warning: Your filter is shorter than the alignment (%zu<%li)",
                        filter->get_length(), max_ali_len);
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
    long getAliLen() const { return max_ali_len; }

    void set_single_mode(GBDATA *gb_species) { single_species = gb_species; }
    bool in_single_mode() const { return single_species; }

    GBDATA *first_species() const { return single_species ? single_species : find_first(gb_main); }
    GBDATA *next_species(GBDATA *gb_prev) const { return single_species ? NULL : find_next(gb_prev); }

    const unsigned char *get_seq_data(GBDATA *gb_species, size_t& slen, GB_ERROR& error) const;
    static bool isGap(char c) { return c == '-' || c == '.'; }

    size_t count_species() {
        sio_assert(!in_single_mode());
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
    GBDATA     *gb_seq = GBT_find_sequence(gb_species, ali);

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

    sio_assert(!in_single_mode());

    if (compress == 1) {        // compress vertical gaps!
        size_t  gap_columns = filter->get_filtered_length();
        size_t *gap_column  = new size_t[gap_columns+1];

        const size_t *filterpos_2_seqpos = filter->get_filterpos_2_seqpos();
        memcpy(gap_column, filterpos_2_seqpos, gap_columns*sizeof(*gap_column));
        gap_column[gap_columns] = max_ali_len;

        arb_progress progress("Calculating vertical gaps", count_species());

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

                sio_assert(i >= j);
                size_t skipped_columns  = i-j;
                sio_assert(gap_columns >= skipped_columns);
                gap_columns            -= skipped_columns;
            }
            progress.inc_and_check_user_abort(err);
        }

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
                    sio_assert(fpos<gap_column[gpos]);
                    sio_assert(epos < columns); // got more columns than expected
                    export_column[epos++] = fpos;
                }
            }
            for (; a<flen; ++a) {
                export_column[epos++] = filterpos_2_seqpos[a];
            }

            sio_assert(epos == columns);
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
        freenull(error);

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

static const char *exported_sequence(GBDATA *gb_species, size_t *seq_len, GB_ERROR *error) {
    sio_assert(esd);
    return esd->get_export_sequence(gb_species, *seq_len, *error);
}

static GB_ERROR XML_recursive(GBDATA *gbd) {
    GB_ERROR    error    = 0;
    const char *key_name = GB_read_key_pntr(gbd);
    XML_Tag    *tag      = 0;
    bool        descend  = true;

    if (strncmp(key_name, "ali_", 4) == 0)
    {
        sio_assert(esd);
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

        if (GB_is_container(gbd)) {
            const char *name = GBT_read_char_pntr(gbd, "name");
            if (name) tag->add_attribute("name", name);
        }
    }

    if (descend) {
        switch (GB_read_type(gbd)) {
            case GB_DB: {
                for (GBDATA *gb_child = GB_child(gbd); gb_child && !error; gb_child = GB_nextChild(gb_child)) {
                    const char *sub_key_name = GB_read_key_pntr(gb_child);

                    if (strcmp(sub_key_name, "name") != 0) { // do not recurse for "name" (is handled above)
                        error = XML_recursive(gb_child);
                    }
                }
                break;
            }
            default: {
                char *content = GB_read_as_string(gbd);
                if (content) {
                    XML_Text text(content);
                    free(content);
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
        while ((p = GBS_find_string(o, "$$DELETE_LINE$$", 0))) {
            char *l, *r;
            for (l = p; l>o; l--) if (*l=='\n') break;
            r = strchr(p, '\n'); if (!r) r = p + strlen(p);
            fwrite(o, 1, l-o, out);
            o = r;
        }
        fputs(o, out);
        free(pars);
    }
    return error;
}

static GB_ERROR export_format_single(const char *db_name, const char *formname, const char *outname, char **resulting_outname) {
    // Exports sequences specified by 'esd' (module global variable)
    // to format specified by 'formname'.
    //
    // if 'outname' == NULL -> export species to temporary file, otherwise to 'outname'.
    // Full path of generated file is returned in 'resulting_outname'

    static int export_depth     = 0;
    export_depth++;

    *resulting_outname = 0;

    export_format efo;
    GB_ERROR      error = read_export_format(&efo, formname, true);

    if (!error) {
        if (!outname) {                             // if no 'outname' is given -> export to temporary file
            char *unique_outname = GB_unique_filename("exported", efo.suffix);
            *resulting_outname   = GB_create_tempfile(unique_outname);
            free(unique_outname);

            if (!*resulting_outname) error = GB_await_error();
        }
        else *resulting_outname = strdup(outname);
    }

    sio_assert(error || *resulting_outname);

    if (!error) {
        if (efo.new_format) {
            // Export data using format 'new_format'.
            // Afterwards convert to wanted format using 'system'.

            sio_assert(efo.system);

            char *intermediate_export;
            error = export_format_single(db_name, efo.new_format, NULL, &intermediate_export);
            if (!error) {
                sio_assert(GB_is_privatefile(intermediate_export, false));

                GB_informationf("Converting to %s", efo.suffix);

                char *srt = GBS_global_string_copy("$<=%s:$>=%s", intermediate_export, *resulting_outname);
                char *sys = GBS_string_eval(efo.system, srt, 0);

                GB_informationf("exec '%s'", efo.system);
                error = GBK_system(sys);

                GB_unlink_or_warn(intermediate_export, &error);

                free(sys);
                free(srt);
            }
            free(intermediate_export);
        }
        else {
            FILE *out       = fopen(*resulting_outname, "wt");
            if (!out) error = GB_IO_error("writing", *resulting_outname);
            else {
                XML_Document *xml = 0;

                int allCount    = 0;
                for (GBDATA *gb_species = esd->first_species();
                     gb_species && !error;
                     gb_species = esd->next_species(gb_species))
                {
                    allCount++;
                }

                arb_progress progress(allCount);
                progress.auto_subtitles("Saving species");

                if (efo.export_mode == EXPORT_XML) {
                    xml = new XML_Document("ARB_SEQ_EXPORT", "arb_seq_export.dtd", out);
                    {
                        xml->add_attribute("database", db_name);
                    }
                    xml->add_attribute("export_date", GB_date_string());
                    {
                        XML_Comment rem("There is a basic version of ARB_seq_export.dtd in $ARBHOME/lib/dtd\n"
                                        "but you might need to expand it by yourself,\n"
                                        "because the ARB-database may contain any kind of fields.");
                    }
                }

                for (GBDATA *gb_species = esd->first_species();
                     gb_species && !error;
                     gb_species = esd->next_species(gb_species))
                {
                    switch (efo.export_mode) {
                        case EXPORT_USING_FORM:
                            error = export_species_using_form(out, gb_species, efo.form);
                            break;

                        case EXPORT_XML:
                            error = XML_recursive(gb_species);
                            break;

                        case EXPORT_INVALID:
                            sio_assert(0);
                            break;
                    }
                    progress.inc_and_check_user_abort(error);
                }

                delete xml;
                fclose(out);
            }
        }
    }

    if (error) {
        if (*resulting_outname) {
            GB_unlink_or_warn(*resulting_outname, NULL);
            freenull(*resulting_outname);
        }
    }

    export_depth--;

    return error;
}

static GB_ERROR export_format_multiple(const char* dbname, const char *formname, const char *outname, bool multiple, char **resulting_outname) {
    GB_ERROR error = 0;

    if (multiple) {
        char *path, *name, *suffix;
        GB_split_full_path(outname, &path, NULL, &name, &suffix);
        *resulting_outname = NULL;

        arb_progress progress("Exporting data", esd->count_species());

        for (GBDATA *gb_species = esd->first_species();
             gb_species && !error;
             gb_species = esd->next_species(gb_species))
        {
            const char *species_name = GBT_read_char_pntr(gb_species, "name");
            if (!species_name) error = "Can't export unnamed species";
            else {
                const char *fname = GB_append_suffix(GBS_global_string("%s_%s", name, species_name), suffix);
                progress.subtitle(fname);

                char *oname = strdup(GB_concat_path(path, fname));
                char *res_oname;

                esd->set_single_mode(gb_species); // means: only export 'gb_species'
                error = export_format_single(dbname, formname, oname, &res_oname);
                esd->set_single_mode(NULL);

                if (!*resulting_outname || // not set yet
                    (res_oname && strcmp(*resulting_outname, res_oname)>0)) // or smaller than set one
                {
                    reassign(*resulting_outname, res_oname);
                }

                free(res_oname);
                free(oname);
            }

            progress.inc_and_check_user_abort(error);
        }

        free(suffix);
        free(name);
        free(path);
    }
    else {
        arb_progress progress("Exporting data");
        error = export_format_single(dbname, formname, outname, resulting_outname);
    }

    return error;
}

GB_ERROR SEQIO_export_by_format(GBDATA *gb_main, int marked_only, AP_filter *filter,
                                int cut_stop_codon, int compress, const char *dbname,
                                const char *formname, const char *outname, int multiple,
                                char **real_outname)
{
    sio_assert(!GB_have_error());

    GB_ERROR error = filter->is_invalid();
    if (!error) {
        esd = new export_sequence_data(gb_main, marked_only, filter, cut_stop_codon, compress);
        sio_assert(esd->getAliLen()>0);

        GB_set_export_sequence_hook(exported_sequence);

        error = esd->detectVerticalGaps();
        if (!error) {
            error = export_format_multiple(dbname, formname, outname, multiple, real_outname);
        }

        GB_set_export_sequence_hook(0);
    }
    delete esd;
    esd = 0;

    sio_assert(!GB_have_error());
    return error;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>

// uncomment to auto-update exported files
// (needed once after changing database or export formats)
// #define TEST_AUTO_UPDATE
#define TEST_AUTO_UPDATE_ONLY_MISSING // do auto-update only if file is missing 

#define TEST_EXPORT_FORMAT(filename,load_complete_form)                 \
    do {                                                                \
        export_format efo;                                              \
        TEST_EXPECT_NO_ERROR(read_export_format((&efo),                 \
                                                filename,               \
                                                load_complete_form));   \
    } while(0)                                                          \

void TEST_sequence_export() {
    GB_shell              shell;
    arb_suppress_progress silence;

    GBDATA   *gb_main    = GB_open("TEST_loadsave.arb", "r");
    char     *export_dir = nulldup(GB_path_in_ARBLIB("export"));
    StrArray  eft;
    GBS_read_dir(eft, export_dir, "*.eft");

    AP_filter *filter = NULL;
    {
        GB_transaction ta(gb_main);

        char   *ali    = GBT_get_default_alignment(gb_main);
        size_t  alilen = GBT_get_alignment_len(gb_main, ali);
        filter         = new AP_filter(alilen);

        GBT_mark_all(gb_main, 0);
        GBDATA *gb_species = GBT_find_species(gb_main, "MetMazei");
        TEST_REJECT_NULL(gb_species);

        GB_write_flag(gb_species, 1); // mark
        free(ali);
    }
    for (int e = 0; eft[e]; ++e) {
        for (int complete = 0; complete <= 1; ++complete) {
            TEST_EXPORT_FORMAT(eft[e], complete);
            if (complete) {
                const char *outname      = "impexp/exported";
                char       *used_outname = NULL;

                {
                    GB_transaction ta(gb_main);
                    TEST_EXPECT_NO_ERROR(SEQIO_export_by_format(gb_main, 1, filter, 0, 0, "DBname", eft[e], outname, 0, &used_outname));
                }

                const char *name = strrchr(eft[e], '/');
                TEST_REJECT_NULL(name);
                name++;

                char *expected = GBS_global_string_copy("impexp/%s.exported", name);

#if defined(TEST_AUTO_UPDATE)
#if defined(TEST_AUTO_UPDATE_ONLY_MISSING)
                if (GB_is_regularfile(expected)) {
                    TEST_EXPECT_TEXTFILE_DIFFLINES_IGNORE_DATES(expected, outname, 0);
                }
                else
#else
                {
                    TEST_COPY_FILE(outname, expected);
                }
#endif
#else
                TEST_EXPECT_TEXTFILE_DIFFLINES_IGNORE_DATES(expected, outname, 0);
                // see ../../UNIT_TESTER/run/impexp
#endif // TEST_AUTO_UPDATE
                TEST_EXPECT_ZERO_OR_SHOW_ERRNO(unlink(outname));

                free(expected);
                free(used_outname);
            }
        }
    }

    delete filter;
    free(export_dir);
    GB_close(gb_main);
}

#endif // UNIT_TESTS
