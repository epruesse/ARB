// ============================================================= //
//                                                               //
//   File      : AWTI_import.cxx                                 //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "awti_imp_local.hxx"

#include <seqio.hxx>
#include <awt.hxx>
#include <item_sel_list.h>
#include <aw_advice.hxx>
#include <aw_file.hxx>
#include <aw_awar.hxx>
#include <AW_rename.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <aw_question.hxx>
#include <GenomeImport.h>
#include <GEN.hxx>
#include <adGene.h>
#include <arb_progress.h>
#include <arb_strbuf.h>
#include <arb_file.h>
#include <arb_str.h>
#include <macros.hxx>

#include <climits>
#include <unistd.h>

using namespace std;

#define MAX_COMMENT_LINES 2000


inline const char *name_only(const char *fullpath) {
    const char *lslash = strrchr(fullpath, '/');
    return lslash ? lslash+1 : fullpath;
}


static GB_ERROR not_in_match_error(const char *cmd) {
    return GBS_global_string("Command '%s' may only appear after 'MATCH'", cmd);
}

static GB_ERROR read_import_format(const char *fullfile, import_format *ifo, bool *var_set, bool included) {
    GB_ERROR  error = 0;
    FILE     *in    = fopen(fullfile, "rt");

    import_match *m = ifo->match;

    if (!in) {
        const char *name = name_only(fullfile);

        if (included) {
            error = GB_export_IO_error("including", name);
        }
        else {
            error = strchr(name, '*')
                ? "Please use 'AUTO DETECT' or manually select an import format"
                : GB_IO_error("loading import filter", name);
        }
    }
    else {
        char   *s1, *s2;
        size_t  lineNumber    = 0;
        bool    include_error = false;

        while (!error && SEQIO_read_string_pair(in, s1, s2, lineNumber)) {

#define GLOBAL_COMMAND(cmd) (!error && strcmp(s1, cmd) == 0)
#define MATCH_COMMAND(cmd)  (!error && strcmp(s1, cmd) == 0 && (m || !(error = not_in_match_error(cmd))))

            if (GLOBAL_COMMAND("MATCH")) {
                m = new import_match;

                m->defined_at = GBS_global_string_copy("%zi,%s", lineNumber, name_only(fullfile));
                m->next       = ifo->match; // this concatenates the filters to the front -> the list is reversed below
                ifo->match    = m;
                m->match      = GBS_remove_escape(s2);
                m->type       = GB_STRING;

                if (ifo->autotag) m->mtag = strdup(ifo->autotag); // will be overwritten by TAG command
            }
            else if (MATCH_COMMAND("SRT"))         { reassign(m->srt, s2); }
            else if (MATCH_COMMAND("ACI"))         { reassign(m->aci, s2); }
            else if (MATCH_COMMAND("WRITE"))       { reassign(m->write, s2); }
            else if (MATCH_COMMAND("WRITE_INT"))   { reassign(m->write, s2); m->type = GB_INT; }
            else if (MATCH_COMMAND("WRITE_FLOAT")) { reassign(m->write, s2); m->type = GB_FLOAT; }
            else if (MATCH_COMMAND("APPEND"))      { reassign(m->append, s2); }
            else if (MATCH_COMMAND("SETVAR")) {
                if (strlen(s2) != 1 || s2[0]<'a' || s2[0]>'z') {
                    error = "Allowed variable names are a-z";
                }
                else {
                    var_set[s2[0]-'a'] = true;
                    reassign(m->setvar, s2);
                }
            }
            else if (MATCH_COMMAND("TAG")) {
                if (s2[0]) reassign(m->mtag, s2);
                else error = "Empty TAG is not allowed";
            }
            else if (GLOBAL_COMMAND("AUTODETECT"))               { reassign(ifo->autodetect,    s2); }
            else if (GLOBAL_COMMAND("SYSTEM"))                   { reassign(ifo->system,        s2); }
            else if (GLOBAL_COMMAND("NEW_FORMAT"))               { reassign(ifo->new_format,    s2); ifo->new_format_lineno = lineNumber; }
            else if (GLOBAL_COMMAND("BEGIN"))                    { reassign(ifo->begin,         s2); }
            else if (GLOBAL_COMMAND("FILETAG"))                  { reassign(ifo->filetag,       s2); }
            else if (GLOBAL_COMMAND("SEQUENCESRT"))              { reassign(ifo->sequencesrt,   s2); }
            else if (GLOBAL_COMMAND("SEQUENCEACI"))              { reassign(ifo->sequenceaci,   s2); }
            else if (GLOBAL_COMMAND("SEQUENCEEND"))              { reassign(ifo->sequenceend,   s2); }
            else if (GLOBAL_COMMAND("END"))                      { reassign(ifo->end,           s2); }
            else if (GLOBAL_COMMAND("AUTOTAG"))                  { reassign(ifo->autotag,       s2); }
            else if (GLOBAL_COMMAND("SEQUENCESTART"))            { reassign(ifo->sequencestart, s2); ifo->read_this_sequence_line_too = 1; }
            else if (GLOBAL_COMMAND("SEQUENCEAFTER"))            { reassign(ifo->sequencestart, s2); ifo->read_this_sequence_line_too = 0; }
            else if (GLOBAL_COMMAND("KEYWIDTH"))                 { ifo->tab            = atoi(s2); }
            else if (GLOBAL_COMMAND("SEQUENCECOLUMN"))           { ifo->sequencecolumn = atoi(s2); }
            else if (GLOBAL_COMMAND("CREATE_ACC_FROM_SEQUENCE")) { ifo->autocreateacc  = 1; }
            else if (GLOBAL_COMMAND("DONT_GEN_NAMES"))           { ifo->noautonames    = 1; }
            else if (GLOBAL_COMMAND("INCLUDE")) {
                char *dir         = AW_extract_directory(fullfile);
                char *includeFile = GBS_global_string_copy("%s/%s", dir, s2);

                error = read_import_format(includeFile, ifo, var_set, true);
                if (error) include_error = true;

                free(includeFile);
                free(dir);
            }
            else {
                bool ifnotset  = GLOBAL_COMMAND("IFNOTSET");
                bool setglobal = GLOBAL_COMMAND("SETGLOBAL");

                if (ifnotset || setglobal) {
                    if (s2[0]<'a' || s2[0]>'z') {
                        error = "Allowed variable names are a-z";
                    }
                    else {
                        int off = 1;
                        while (isblank(s2[off])) off++;
                        if (!s2[off]) error = GBS_global_string("Expected two arguments in '%s'", s2);
                        else {
                            char        varname = s2[0];
                            const char *arg2    = s2+off;

                            if (ifnotset) {
                                if (ifo->variable_errors.get(varname)) {
                                    error = GBS_global_string("Redefinition of IFNOTSET %c", varname);
                                }
                                else {
                                    ifo->variable_errors.set(varname, arg2);
                                }
                            }
                            else {
                                awti_assert(setglobal);
                                ifo->global_variables.set(varname, arg2);
                                var_set[varname-'a'] = true;
                            }
                        }
                    }
                }
                else if (!error) {
                    error = GBS_global_string("Unknown command '%s'", s1);
                }
            }

            free(s1);
            free(s2);
        }

        if (error) {
            error = GBS_global_string("%sin line %zi of %s '%s':\n%s",
                                      include_error ? "included " : "",
                                      lineNumber,
                                      included ? "file" : "import format",
                                      name_only(fullfile),
                                      error);
        }

        fclose(in);

#undef MATCH_COMMAND
#undef GLOBAL_COMMAND
    }

    return error;
}

GB_ERROR ArbImporter::read_format(const char *file) {
    char *fullfile = strdup(GB_path_in_ARBHOME(file));

    delete ifo;
    ifo = new import_format;

    bool var_set[IFS_VARIABLES];
    for (int i = 0; i<IFS_VARIABLES; i++) var_set[i] = false;

    GB_ERROR error = read_import_format(fullfile, ifo, var_set, false);


    for (int i = 0; i<IFS_VARIABLES && !error; i++) {
        bool ifnotset = ifo->variable_errors.get(i+'a');
        if (var_set[i]) {
            bool isglobal = ifo->global_variables.get(i+'a');
            if (!ifnotset && !isglobal) { // don't warn if variable is global
                error = GBS_global_string("Warning: missing IFNOTSET for variable '%c'", 'a'+i);
            }
        }
        else {
            if (ifnotset) {
                error = GBS_global_string("Warning: useless IFNOTSET for unused variable '%c'", 'a'+i);
            }
        }
    }

    // reverse order of match list (was appended backwards during creation)
    if (ifo->match) ifo->match = ifo->match->reverse(0);

    free(fullfile);

    return error;
}

import_match::import_match() {
    memset((char *)this, 0, sizeof(*this));
}

import_match::~import_match() {
    free(match);
    free(aci);
    free(srt);
    free(mtag);
    free(append);
    free(write);
    free(setvar);
    free(defined_at);

    delete next;
}

import_format::import_format() {
    memset((char *)this, 0, sizeof(*this));
}

import_format::~import_format() {
    free(autodetect);
    free(system);
    free(new_format);
    free(begin);
    free(sequencestart);
    free(sequenceend);
    free(sequencesrt);
    free(sequenceaci);
    free(filetag);
    free(autotag);
    free(end);
    free(b1);
    free(b2);

    delete match;
}


static int cmp_ift(const void *p0, const void *p1, void *) {
    return ARB_stricmp((const char *)p0, (const char *)p1);
}

void ArbImporter::detect_format(AW_root *root) {
    StrArray files;
    {
        AW_awar       *awar_dirs = root->awar(AWAR_IMPORT_FORMATDIR);
        ConstStrArray  dirs;
        GBT_split_string(dirs, awar_dirs->read_char_pntr(), ":", true);
        for (unsigned i = 0; i<dirs.size(); ++i) GBS_read_dir(files, dirs[i], "*.ift");
    }
    files.sort(cmp_ift, NULL);

    char     buffer[AWTI_IMPORT_CHECK_BUFFER_SIZE+10];
    GB_ERROR error = 0;

    int matched       = -1;
    int first         = -1;
    int matched_count = 0;

    AW_awar *awar_filter   = root->awar(AWAR_IMPORT_FORMATNAME);
    char    *prev_selected = awar_filter->read_string();

    for (int idx = 0; files[idx] && !error; ++idx) {
        const char *filtername = files[idx];
        awar_filter->write_string(filtername);

        GB_ERROR form_err = read_format(filtername);
        if (form_err) {
            aw_message(form_err);
        }
        else {
            if (ifo->autodetect) {      // detectable
                char *autodetect = GBS_remove_escape(ifo->autodetect);
                delete ifo; ifo = 0;

                FILE *test = 0;
                {
                    char *f = root->awar(AWAR_IMPORT_FILENAME)->read_string();

                    if (f[0]) {
                        const char *com = GBS_global_string("cat %s 2>/dev/null", f);
                        test            = popen(com, "r");
                    }
                    free(f);
                    if (!test) error = "Cannot open any input file";
                }
                if (test) {
                    int size = fread(buffer, 1, AWTI_IMPORT_CHECK_BUFFER_SIZE, test);
                    pclose(test);

                    if (size>=0) {
                        buffer[size] = 0;
                        if (GBS_string_matches(buffer, autodetect, GB_MIND_CASE)) {
                            // format found!
                            matched_count++;
                            if (matched == -1) matched = idx; // remember first/next found format
                            if (first == -1) first     = idx; // remember first found format

                            if (strcmp(filtername, prev_selected) == 0) { // previously selected filter
                                matched = -1; // select next (or first.. see below)
                            }
                        }
                    }
                }
                free(autodetect);
            }
        }
    }

    const char *select = 0;
    switch (matched_count) {
        case 0:
            AW_advice("Not all formats can be auto-detected.\n"
                      "Some need to be selected manually.",
                       AW_ADVICE_TOGGLE,
                      "No format auto-detected",
                      "arb_import.hlp");

            select = "unknown.ift";
            break;

        default:
            AW_advice("Several import filters matched during auto-detection.\n"
                      "Click 'AUTO DETECT' again to select next matching import-filter.",
                      AW_ADVICE_TOGGLE,
                      "Several formats detected",
                      "arb_import.hlp");

            // fall-through
        case 1:
            if (matched == -1) {
                // wrap around to top (while searching next matching filter)
                // or re-select the one matching filter (if it was previously selected)
                awti_assert(first != -1);
                matched = first;
            }
            awti_assert(matched != -1);
            select = files[matched];  // select 1st matching filter
            break;
    }

    if (error) {
        select = "unknown.ift";
        aw_message(error);
    }

    free(prev_selected);
    awar_filter->write_string(select);
}

int ArbImporter::next_file() {
    if (in) fclose(in);
    if (current_file_idx<0) current_file_idx = 0;

    int result = 1;
    while (result == 1 && filenames[current_file_idx]) {
        const char *origin_file_name = filenames[current_file_idx++];

        const char *sorigin   = strrchr(origin_file_name, '/');
        if (!sorigin) sorigin = origin_file_name;
        else sorigin++;

        GB_ERROR  error          = 0;
        char     *mid_file_name  = 0;
        char     *dest_file_name = 0;

        if (ifo2 && ifo2->system) {
            {
                const char *sorigin_nameonly            = strrchr(sorigin, '/');
                if (!sorigin_nameonly) sorigin_nameonly = sorigin;

                char *mid_name = GB_unique_filename(sorigin_nameonly, "tmp");
                mid_file_name  = GB_create_tempfile(mid_name);
                free(mid_name);

                if (!mid_file_name) error = GB_await_error();
            }

            if (!error) {
                char *srt = GBS_global_string_copy("$<=%s:$>=%s", origin_file_name, mid_file_name);
                char *sys = GBS_string_eval(ifo2->system, srt, 0);

                arb_progress::show_comment(GBS_global_string("exec '%s'", ifo2->system));

                error                        = GBK_system(sys);
                if (!error) origin_file_name = mid_file_name;

                free(sys);
                free(srt);
            }
        }

        if (!error && ifo->system) {
            {
                const char *sorigin_nameonly            = strrchr(sorigin, '/');
                if (!sorigin_nameonly) sorigin_nameonly = sorigin;

                char *dest_name = GB_unique_filename(sorigin_nameonly, "tmp");
                dest_file_name  = GB_create_tempfile(dest_name);
                free(dest_name);

                if (!dest_file_name) error = GB_await_error();
            }

            awti_assert(dest_file_name || error);

            if (!error) {
                char *srt = GBS_global_string_copy("$<=%s:$>=%s", origin_file_name, dest_file_name);
                char *sys = GBS_string_eval(ifo->system, srt, 0);

                arb_progress::show_comment(GBS_global_string("Converting File %s", ifo->system));

                error                        = GBK_system(sys);
                if (!error) origin_file_name = dest_file_name;

                free(sys);
                free(srt);
            }
        }

        if (!error) {
            in = fopen(origin_file_name, "r");

            if (in) {
                result = 0;
            }
            else {
                error = GBS_global_string("Error: Cannot open file %s\n", origin_file_name);
            }
        }

        if (mid_file_name) {
            awti_assert(GB_is_privatefile(mid_file_name, false));
            GB_unlink_or_warn(mid_file_name, &error);
            free(mid_file_name);
        }
        if (dest_file_name) {
            GB_unlink_or_warn(dest_file_name, &error);
            free(dest_file_name);
        }

        if (error) aw_message(error);
    }

    return result;
}

char *ArbImporter::read_line(int tab, char *sequencestart, char *sequenceend) {
    /* two modes:   tab == 0 -> read single lines,
       different files are separated by sequenceend,
       tab != 0 join lines that start after position tab,
       joined lines are separated by '|'
       except lines that match sequencestart
       (they may be part of sequence if read_this_sequence_line_too = 1 */

    static char *in_queue  = 0;     // read data
    static int   b2offset  = 0;
    const int    BUFSIZE   = 8000;
    const char  *SEPARATOR = "|";   // line separator

    if (!ifo->b1) ifo->b1 = (char*)calloc(BUFSIZE, 1);
    if (!ifo->b2) ifo->b2 = (char*)calloc(BUFSIZE, 1);
    if (!in) {
        if (next_file()) {
            if (in_queue) {
                char *s = in_queue;
                in_queue = 0;
                return s;
            }
            return 0;
        }
    }


    if (!tab) {
        if (in_queue) {
            char *s = in_queue;
            in_queue = 0;
            return s;
        }
        char *p = SEQIO_fgets(ifo->b1, BUFSIZE-3, in);
        if (!p) {
            sprintf(ifo->b1, "%s", sequenceend);
            if (in) { fclose(in); in = 0; }
            p = ifo->b1;
        }
        int len = strlen(p)-1;
        while (len>=0) {
            if (p[len] == '\n' || p[len] == 13) p[len--] = 0;
            else break;
        }
        return ifo->b1;
    }

    b2offset = 0;
    ifo->b2[0] = 0;

    if (in_queue) {
        b2offset = 0;
        strncpy(ifo->b2+b2offset, in_queue, BUFSIZE - 4- b2offset);
        b2offset += strlen(ifo->b2+b2offset);
        in_queue = 0;
        if (GBS_string_matches(ifo->b2, sequencestart, GB_MIND_CASE)) return ifo->b2;
    }
    while (1) {
        char *p = SEQIO_fgets(ifo->b1, BUFSIZE-3, in);
        if (!p) {
            if (in) { fclose(in); in = 0; }
            break;
        }
        int len = strlen(p)-1;
        while (len>=0) {
            if (p[len] == '\n' || p[len] == 13) p[len--] = 0;
            else break;
        }


        if (GBS_string_matches(ifo->b1, sequencestart, GB_MIND_CASE)) {
            in_queue = ifo->b1;
            return ifo->b2;
        }

        int i;
        for (i=0; i<=tab; i++) if (ifo->b1[i] != ' ') break;

        if (i < tab) {
            in_queue = ifo->b1;
            return ifo->b2;
        }
        strncpy(ifo->b2+b2offset, SEPARATOR, BUFSIZE - 4- b2offset);
        b2offset += strlen(ifo->b2+b2offset);

        p = ifo->b1;
        if (b2offset>0) while (*p==' ') p++;    // delete spaces in second line

        strncpy(ifo->b2+b2offset, p, BUFSIZE - 4- b2offset);
        b2offset += strlen(ifo->b2+b2offset);
    }
    in_queue = 0;
    return ifo->b2;
}

static void write_entry(GBDATA *gb_main, GBDATA *gbd, const char *key, const char *str, const char *tag, int append, GB_TYPES type) {
    if (!gbd) return;

    {
        while (str[0] == ' ' || str[0] == '\t' || str[0] == '|') str++;
        int i = strlen(str)-1;
        while (i >= 0 && (str[i] == ' ' || str[i] == '\t' || str[i] == '|' || str[i] == 13)) {
            i--;
        }

        if (i<0) return;

        i++;
        if (str[i]) { // need to cut trailing whitespace?
            char *copy = (char*)malloc(i+1);
            memcpy(copy, str, i);
            copy[i]    = 0;

            write_entry(gb_main, gbd, key, copy, tag, append, type);

            free(copy);
            return;
        }
    }

    GBDATA *gbk = GB_entry(gbd, key);

    if (type != GB_STRING) {
        if (!gbk) gbk=GB_create(gbd, key, type);
        switch (type) {
            case GB_INT:
                GB_write_int(gbk, atoi(str));
                break;
            case GB_FLOAT:
                GB_write_float(gbk, GB_atof(str));
                break;
            default:
                awti_assert(0);
                break;
        }
        return;
    }

    if (!gbk || !append) {
        if (!gbk) gbk=GB_create(gbd, key, GB_STRING);

        if (tag) {
            GBS_strstruct *s = GBS_stropen(10000);
            GBS_chrcat(s, '[');
            GBS_strcat(s, tag);
            GBS_strcat(s, "] ");
            GBS_strcat(s, str);
            char *val = GBS_strclose(s);
            GB_write_string(gbk, val);
            free(val);
        }
        else {
            if (strcmp(key, "name") == 0) {
                char *nstr = GBT_create_unique_species_name(gb_main, str);
                GB_write_string(gbk, nstr);
                free(nstr);
            }
            else {
                GB_write_string(gbk, str);
            }
        }
        return;
    }

    const char *strin = GB_read_char_pntr(gbk);

    int   len    = strlen(str) + strlen(strin);
    int   taglen = tag ? (strlen(tag)+2) : 0;
    char *buf    = (char *)GB_calloc(sizeof(char), len+2+taglen+1);

    if (tag) {
        char *regexp = (char*)GB_calloc(sizeof(char), taglen+3);
        sprintf(regexp, "*[%s]*", tag);

        if (!GBS_string_matches(strin, regexp, GB_IGNORE_CASE)) { // if tag does not exist yet
            sprintf(buf, "%s [%s] %s", strin, tag, str); // prefix with tag
        }
        free(regexp);
    }

    if (buf[0] == 0) {
        sprintf(buf, "%s %s", strin, str);
    }

    GB_write_string(gbk, buf);
    free(buf);
    return;
}

static string expandSetVariables(const SetVariables& variables, const string& source, bool& error_occurred, const import_format *ifo) {
    string                 dest;
    string::const_iterator norm_start = source.begin();
    string::const_iterator p          = norm_start;
    error_occurred                    = false;

    while (p != source.end()) {
        if (*p == '$') {
            ++p;
            if (*p == '$') { // '$$' -> '$'
                dest.append(1, *p);
            }
            else { // real variable
                const string *value = variables.get(*p);
                if (!value) {
                    const string *user_error = ifo->variable_errors.get(*p);

                    char *error = 0;
                    if (user_error) {
                        error = GBS_global_string_copy("%s (variable '$%c' not set yet)", user_error->c_str(), *p);
                    }
                    else {
                        error = GBS_global_string_copy("Variable '$%c' not set (missing SETVAR or SETGLOBAL?)", *p);
                    }

                    dest.append(GBS_global_string("<%s>", error));
                    GB_export_error(error);
                    free(error);
                    error_occurred = true;
                }
                else {
                    dest.append(*value);
                }
            }
            ++p;
        }
        else {
            dest.append(1, *p);
            ++p;
        }
    }
    return dest;
}

GB_ERROR ArbImporter::read_data(char *ali_name, int security_write) {
    char        text[100];
    static int  counter         = 0;
    GBDATA     *gb_species_data = GBT_get_species_data(gb_import_main);
    GBDATA     *gb_species;
    char       *p;

    import_match *match = 0;

    while (1) {             // go to the start
        p = read_line(0, ifo->sequencestart, ifo->sequenceend);
        if (!p || !ifo->begin || GBS_string_matches(p, ifo->begin, GB_MIND_CASE)) break;
    }
    if (!p) return "Cannot find start of file: Wrong format or empty file";

    // lets start !!!!!
    while (p) {
        SetVariables variables(ifo->global_variables);

        counter++;
        if (counter % 10 == 0) {
            sprintf(text, "Reading species %i", counter);
            arb_progress::show_comment(text);
        }

        gb_species = GB_create_container(gb_species_data, "species");
        sprintf(text, "spec%i", counter);
        GBT_readOrCreate_char_pntr(gb_species, "name", text); // set default if missing
        if (filenames[1]) {      // multiple files !!!
            const char *f = strrchr(filenames[current_file_idx-1], '/');
            if (!f) f = filenames[current_file_idx-1];
            else f++;
            write_entry(gb_import_main, gb_species, "file", f, ifo->filetag, 0, GB_STRING);
        }
        int line;
        static bool never_warn = false;
        int max_line = never_warn ? INT_MAX : MAX_COMMENT_LINES;

        for (line=0; line<=max_line; line++) {
            if (line == max_line) {
                const char *file = NULL;
                if (filenames[current_file_idx]) file = filenames[current_file_idx];
                GB_ERROR msg = GB_export_errorf("A database entry in file '%s' is longer than %i lines.\n"
                                                "    This might be the result of a wrong input format\n"
                                                "    or a long comment in a sequence\n", file, line);

                switch (aw_question("import_long_lines", msg, "Continue Reading,Continue Reading (Never ask again),Abort")) {
                    case 0:
                        max_line *= 2;
                        break;
                    case 1:
                        max_line = INT_MAX;
                        never_warn = true;
                        break;
                    case 2:
                        break;
                }
            }
            GB_ERROR    error      = 0;
            if (strlen(p) > ifo->tab) {
                for (match = ifo->match; !error && match; match=match->next) {
                    const char *what_error = 0;
                    if (GBS_string_matches(p, match->match, GB_MIND_CASE)) {
                        char *dup = p+ifo->tab;
                        while (*dup == ' ' || *dup == '\t') dup++;

                        char *s              = 0;
                        char *dele           = 0;

                        if (match->srt) {
                            bool   err_flag;
                            string expanded = expandSetVariables(variables, match->srt, err_flag, ifo);
                            if (err_flag) error = GB_await_error();
                            else {
                                dele           = s = GBS_string_eval(dup, expanded.c_str(), gb_species);
                                if (!s) error  = GB_await_error();
                            }
                            if (error) what_error = "SRT";
                        }
                        else {
                            s = dup;
                        }

                        if (!error && match->aci) {
                            bool   err_flag;
                            string expanded     = expandSetVariables(variables, match->aci, err_flag, ifo);
                            if (err_flag) error = GB_await_error();
                            else {
                                dup           = dele;
                                dele          = s = GB_command_interpreter(gb_import_main, s, expanded.c_str(), gb_species, 0);
                                if (!s) error = GB_await_error();
                                free(dup);
                            }
                            if (error) what_error = "ACI";
                        }

                        if (!error && (match->append || match->write)) {
                            char *field = 0;
                            char *tag   = 0;

                            {
                                bool   err_flag;
                                string expanded_field = expandSetVariables(variables, string(match->append ? match->append : match->write), err_flag, ifo);
                                if (err_flag) error   = GB_await_error();
                                else   field          = GBS_string_2_key(expanded_field.c_str());
                                if (error) what_error = match->append ? "APPEND" : "WRITE";
                            }

                            if (!error && match->mtag) {
                                bool   err_flag;
                                string expanded_tag = expandSetVariables(variables, string(match->mtag), err_flag, ifo);
                                if (err_flag) error = GB_await_error();
                                else   tag          = GBS_string_2_key(expanded_tag.c_str());
                                if (error) what_error = "TAG";
                            }

                            if (!error) {
                                write_entry(gb_import_main, gb_species, field, s, tag, match->append != 0, match->type);
                            }
                            free(tag);
                            free(field);
                        }

                        if (!error && match->setvar) variables.set(match->setvar[0], s);
                        free(dele);
                    }

                    if (error) {
                        error = GBS_global_string("'%s'\nin %s of MATCH (defined at #%s)", error, what_error, match->defined_at);
                    }
                }
            }

            if (error) {
                return GBS_global_string("%s\nwhile parsing line #%i of species #%i", error, line, counter);
            }

            if (GBS_string_matches(p, ifo->sequencestart, GB_MIND_CASE)) goto read_sequence;

            p = read_line(ifo->tab, ifo->sequencestart, ifo->sequenceend);
            if (!p) break;
        }
        return GB_export_errorf("No Start of Sequence found (%i lines read)", max_line);

    read_sequence :
        {
            char          *sequence;
            GBS_strstruct *strstruct = GBS_stropen(5000);
            int            linecnt;

            for (linecnt = 0; ; linecnt++) {
                if (linecnt || !ifo->read_this_sequence_line_too) {
                    p = read_line(0, ifo->sequencestart, ifo->sequenceend);
                }
                if (!p) break;
                if (ifo->sequenceend && GBS_string_matches(p, ifo->sequenceend, GB_MIND_CASE)) break;
                if (strlen(p) <= ifo->sequencecolumn) continue;
                GBS_strcat(strstruct, p+ifo->sequencecolumn);
            }
            sequence = GBS_strclose(strstruct);

            GBDATA *gb_data = GBT_create_sequence_data(gb_species, ali_name, "data", GB_STRING, security_write);
            if (ifo->sequencesrt) {
                char *h = GBS_string_eval(sequence, ifo->sequencesrt, gb_species);
                if (!h) return GB_await_error();
                freeset(sequence, h);
            }

            if (ifo->sequenceaci) {
                char *h = GB_command_interpreter(gb_import_main, sequence, ifo->sequenceaci, gb_species, 0);
                free(sequence);
                if (!h) return GB_await_error();
                sequence = h;
            }

            GB_write_string(gb_data, sequence);

            GBDATA *gb_acc = GB_search(gb_species, "acc", GB_FIND);
            if (!gb_acc && ifo->autocreateacc) {
                char buf[100];
                long id = GBS_checksum(sequence, 1, ".-");
                sprintf(buf, "ARB_%lX", id);
                gb_acc = GB_search(gb_species, "acc", GB_STRING);
                GB_write_string(gb_acc, buf);
            }
            free(sequence);
        }
        while (1) {             // go to the start of an species
            if (!p || !ifo->begin || GBS_string_matches(p, ifo->begin, GB_MIND_CASE)) break;
            p = read_line(0, ifo->sequencestart, ifo->sequenceend);
        }
    }
    return 0;
}

void ArbImporter::go(AW_window *aww) {
    // Import sequences into new or existing database
    AW_root *awr                     = aww->get_root();
    bool     is_genom_db             = false;
    bool     delete_db_type_if_error = false; // delete db type (genome/normal) in case of error ?
    {
        bool           read_genom_db = (awr->awar(AWAR_IMPORT_GENOM_DB)->read_int() != IMP_PLAIN_SEQUENCE);
        GB_transaction ta(gb_import_main);

        delete_db_type_if_error = (GB_entry(gb_import_main, GENOM_DB_TYPE) == 0);
        is_genom_db             = GEN_is_genome_db(gb_import_main, read_genom_db);

        if (read_genom_db!=is_genom_db) {
            if (is_genom_db) {
                aw_message("You can only import whole genom sequences into a genom database.");
            }
            else {
                aw_message("You can't import whole genom sequences into a non-genom ARB database.");
            }
            return;
        }
    }

    GB_ERROR error = 0;

    GB_change_my_security(gb_import_main, 6);

    GB_begin_transaction(gb_import_main); // first transaction start
    char *ali_name;
    {
        char *ali = awr->awar(AWAR_IMPORT_ALI)->read_string();
        ali_name = GBS_string_eval(ali, "*=ali_*1:ali_ali_=ali_", 0);
        free(ali);
    }

    error = GBT_check_alignment_name(ali_name);

    int ali_protection = awr->awar(AWAR_IMPORT_ALI_PROTECTION)->read_int();
    if (!error) {
        char *ali_type;
        ali_type = awr->awar(AWAR_IMPORT_ALI_TYPE)->read_string();

        if (is_genom_db && strcmp(ali_type, "dna")!=0) {
            error = "You must set the alignment type to dna when importing genom sequences.";
        }
        else {
            GBT_create_alignment(gb_import_main, ali_name, 2000, 0, ali_protection, ali_type);
        }
        free(ali_type);
    }

    bool ask_generate_names = true;

    if (!error) {
        if (is_genom_db) {
            // import genome flatfile into ARB-genome database:

            char     *mask = awr->awar(AWAR_IMPORT_FILENAME)->read_string();
            StrArray  fnames;
            GBS_read_dir(fnames, mask, NULL);

            if (fnames.empty()) {
                error = GB_export_error("Cannot find selected file");
            }
            else {
                int successfull_imports = 0;
                int failed_imports      = 0;
                int count;

                for (count = 0; fnames[count]; ++count) ; // count filenames

                GBDATA *gb_species_data = GBT_get_species_data(gb_import_main);
                ImportSession import_session(gb_species_data, count*10);

                // close the above transaction and do each importfile in separate transaction
                // to avoid that all imports are undone by transaction abort happening in case of error
                GB_commit_transaction(gb_import_main);

                arb_progress progress("Reading input files", count);

                for (int curr = 0; !error && fnames[curr]; ++curr) {
                    GB_ERROR error_this_file =  0;

                    GB_begin_transaction(gb_import_main);
                    {
                        const char *lslash = strrchr(fnames[curr], '/');
                        progress.subtitle(GBS_global_string("%i/%i: %s", curr+1, count, lslash ? lslash+1 : fnames[curr]));
                    }

#if defined(DEBUG)
                    fprintf(stderr, "import of '%s'\n", fnames[curr]);
#endif // DEBUG
                    error_this_file = GI_importGenomeFile(import_session, fnames[curr], ali_name);

                    if (!error_this_file) {
                        GB_commit_transaction(gb_import_main);
                        successfull_imports++;
                        delete_db_type_if_error = false;
                    }
                    else { // error occurred during import
                        error_this_file = GBS_global_string("'%s' not imported\nReason: %s", fnames[curr], error_this_file);
                        GB_warningf("Import error: %s", error_this_file);
                        GB_abort_transaction(gb_import_main);
                        failed_imports++;
                    }

                    progress.inc_and_check_user_abort(error);
                }

                if (!successfull_imports) {
                    error = "Nothing has been imported";
                }
                else {
                    GB_warningf("%i of %i files were imported with success", successfull_imports, (successfull_imports+failed_imports));
                }

                // now open another transaction to "undo" the transaction close above
                GB_begin_transaction(gb_import_main);
            }

            free(mask);
        }
        else {
            // import to non-genome ARB-db :

            {
                // load import filter:
                char *file = awr->awar(AWAR_IMPORT_FORMATNAME)->read_string();

                if (!strlen(file)) {
                    error = "Please select a form";
                }
                else {
                    error = read_format(file);
                    if (!error && ifo->new_format) {
                        ifo2 = ifo;
                        ifo  = 0;

                        error = read_format(ifo2->new_format);
                        if (!error) {
                            if (ifo->new_format) {
                                error = GBS_global_string("in line %zi of import filter '%s':\n"
                                                          "Only one level of form nesting (NEW_FORMAT) allowed",
                                                          ifo->new_format_lineno, name_only(ifo2->new_format));
                            }
                        }
                        if (error) {
                            error = GBS_global_string("in format used in line %zi of '%s':\n%s",
                                                      ifo2->new_format_lineno, name_only(file), error);
                        }
                    }
                }
                free(file);
            }

            {
                char *f = awr->awar(AWAR_IMPORT_FILENAME)->read_string();
                GBS_read_dir(filenames, f, NULL);
                free(f);
            }

            if (filenames[0] == 0) {
                error = "Cannot find selected file(s)";
            }

            if (!error) {
                arb_progress progress("Reading input files");

                error = read_data(ali_name, ali_protection);
                if (error) {
                    error = GBS_global_string("Error: %s\nwhile reading file %s", error, filenames[current_file_idx-1]);
                }
                else {
                    if (ifo->noautonames || (ifo2 && ifo2->noautonames)) {
                        ask_generate_names = false;
                    }
                    else {
                        ask_generate_names = true;
                    }
                }
            }

            delete ifo;  ifo  = 0;
            delete ifo2; ifo2 = 0;

            if (in) { fclose(in); in = 0; }

            filenames.erase();
            current_file_idx = 0;
        }
    }
    free(ali_name);

    bool call_back = true; // shall after_import_cb be called?
    if (error) {
        GB_abort_transaction(gb_import_main);

        if (delete_db_type_if_error) {
            // delete db type, if it was initialized above
            // (avoids 'can't import'-error, if file-type (genome-file/species-file) is changed after a failed try)
            GB_transaction  ta(gb_import_main);
            GBDATA         *db_type = GB_entry(gb_import_main, GENOM_DB_TYPE);
            if (db_type) GB_delete(db_type);
        }

        call_back = false;
    }
    else {
        aww->hide(); // import window stays open in case of error

        arb_progress progress("Checking and Scanning database", 2+ask_generate_names); // 2 or 3 passes
        progress.subtitle("Pass 1: Check entries");

        // scan for hidden/unknown fields :
        species_field_selection_list_rescan(gb_import_main, RESCAN_REFRESH);
        if (is_genom_db) gene_field_selection_list_rescan(gb_import_main, RESCAN_REFRESH);

        GBT_mark_all(gb_import_main, 1);
        progress.inc();
        progress.subtitle("Pass 2: Check sequence lengths");
        GBT_check_data(gb_import_main, 0);

        GB_commit_transaction(gb_import_main);
        progress.inc();

        if (ask_generate_names) {
            if (aw_question("generate_short_names",
                            "It's recommended to generate unique species identifiers now.\n",
                            "Generate unique species IDs,Use found IDs") == 0)
            {
                progress.subtitle("Pass 3: Generate unique species IDs");
                error = AW_select_nameserver(gb_import_main, gb_main_4_nameserver);
                if (!error) {
                    error = AWTC_pars_names(gb_import_main);
                }
            }
            progress.inc();
        }
    }

    if (error) aw_message(error);

    GB_change_my_security(gb_import_main, 0);

    gb_main_4_nameserver = NULL;
    if (call_back) after_import_cb(awr);
}

class AliNameAndType {
    string name_, type_;
public:
    AliNameAndType(const char *ali_name, const char *ali_type) : name_(ali_name), type_(ali_type) {}

    const char *name() const { return name_.c_str(); }
    const char *type() const { return type_.c_str(); }
};

static AliNameAndType last_ali("ali_new", "rna"); // last selected ali for plain import (aka non-flatfile import)


void AWTI_import_set_ali_and_type(AW_root *awr, const char *ali_name, const char *ali_type, GBDATA *gbmain) {
    bool           switching_to_GENOM_ALIGNMENT = strcmp(ali_name, GENOM_ALIGNMENT) == 0;
    static GBDATA *last_valid_gbmain            = 0;

    if (gbmain) last_valid_gbmain = gbmain;

    AW_awar *awar_name = awr->awar(AWAR_IMPORT_ALI);
    AW_awar *awar_type = awr->awar(AWAR_IMPORT_ALI_TYPE);

    if (switching_to_GENOM_ALIGNMENT) {
        // read and store current settings
        char *curr_ali_name = awar_name->read_string();
        char *curr_ali_type = awar_type->read_string();

        last_ali = AliNameAndType(curr_ali_name, curr_ali_type);

        free(curr_ali_name);
        free(curr_ali_type);
    }

    awar_name->write_string(ali_name);
    awar_type->write_string(ali_type);

    if (last_valid_gbmain) { // detect default write protection for alignment
        GB_transaction ta(last_valid_gbmain);
        GBDATA *gb_ali            = GBT_get_alignment(last_valid_gbmain, ali_name);
        int     protection_to_use = 4;         // default protection

        if (gb_ali) {
            GBDATA *gb_write_security = GB_entry(gb_ali, "alignment_write_security");
            if (gb_write_security) {
                protection_to_use = GB_read_int(gb_write_security);
            }
        }
        else {
            GB_clear_error();
        }
        awr->awar(AWAR_IMPORT_ALI_PROTECTION)->write_int(protection_to_use);
    }
}

static void genom_flag_changed(AW_root *awr) {
    if (awr->awar(AWAR_IMPORT_GENOM_DB)->read_int() == IMP_PLAIN_SEQUENCE) {
        AWTI_import_set_ali_and_type(awr, last_ali.name(), last_ali.type(), 0);
        awr->awar(AWAR_IMPORT_FORMATFILTER)->write_string(".ift");
    }
    else {
        AWTI_import_set_ali_and_type(awr, GENOM_ALIGNMENT, "dna", 0);
        awr->awar(AWAR_IMPORT_FORMATFILTER)->write_string(".fit"); // *hack* to hide normal import filters
    }
}

// --------------------------------------------------------------------------------

static ArbImporter *importer = NULL;

void AWTI_cleanup_importer() {
    if (importer) {
#if defined(DEBUG)
        AWT_browser_forget_db(importer->peekImportDB());
#endif
        delete importer; // closes the import DB if it still is owned by the 'importer'
        importer = NULL;
    }
}

static void import_window_close_cb(AW_window *aww) {
    if (importer) {
        bool doExit = importer->doExit;
        AWTI_cleanup_importer();

        if (doExit) exit(EXIT_SUCCESS);
        else AW_POPDOWN(aww);
    }
    else {
        AW_POPDOWN(aww);
    }
}

static void import_go_cb(AW_window *aww) { importer->go(aww); }
static void detect_input_format_cb(AW_window *aww) {
    if (aww->get_root()->awar(AWAR_IMPORT_GENOM_DB)->read_int() == IMP_PLAIN_SEQUENCE) {
        importer->detect_format(aww->get_root());
    }
    else {
        aw_message("Only works together with 'Import selected format'");
    }
}

GBDATA *AWTI_peek_imported_DB() {
    awti_assert(importer);
    awti_assert(importer->gb_import_main);
    return importer->peekImportDB();
}
GBDATA *AWTI_take_imported_DB_and_cleanup_importer() {
    awti_assert(importer);
    awti_assert(importer->gb_import_main);
    GBDATA *gb_imported_main = importer->takeImportDB();
    AWTI_cleanup_importer();
    return gb_imported_main;
}

void AWTI_open_import_window(AW_root *awr, const char *defname, bool do_exit, GBDATA *gb_main, const RootCallback& after_import_cb) {
    static AW_window_simple *aws = 0;

    awti_assert(!importer);
    importer = new ArbImporter(after_import_cb);

    importer->gb_import_main = GB_open("noname.arb", "wc");  // @@@ move -> ArbImporter-ctor

#if defined(DEBUG)
    AWT_announce_db_to_browser(importer->gb_import_main, "New database (import)");
#endif // DEBUG

    importer->gb_main_4_nameserver = gb_main;

    if (!gb_main) {
        // control macros via temporary import DB (if no main DB available)
        configure_macro_recording(awr, "ARB_IMPORT", importer->gb_import_main); // @@@ use result
    }
    else {
        awti_assert(got_macro_ability(awr));
    }

    importer->doExit = do_exit; // change/set behavior of CLOSE button

    {
        GBS_strstruct path(500);
        path.cat(GB_path_in_arbprop("filter"));
        path.put(':');
        path.cat(GB_path_in_ARBLIB("import"));

        AW_create_fileselection_awars(awr, AWAR_IMPORT_FILEBASE,   ".",             "",     defname);
        AW_create_fileselection_awars(awr, AWAR_IMPORT_FORMATBASE, path.get_data(), ".ift", "*");
    }

    awr->awar_string(AWAR_IMPORT_ALI,      "dummy"); // these defaults are never used
    awr->awar_string(AWAR_IMPORT_ALI_TYPE, "dummy"); // they are overwritten by AWTI_import_set_ali_and_type
    awr->awar_int(AWAR_IMPORT_ALI_PROTECTION, 0);    // which is called via genom_flag_changed() below

    awr->awar_int(AWAR_IMPORT_GENOM_DB, IMP_PLAIN_SEQUENCE);
    awr->awar_int(AWAR_IMPORT_AUTOCONF, 1);

    awr->awar(AWAR_IMPORT_GENOM_DB)->add_callback(genom_flag_changed);
    genom_flag_changed(awr);

    if (!aws) {
        aws = new AW_window_simple;

        aws->init(awr, "ARB_IMPORT", "ARB IMPORT");
        aws->load_xfig("awt/import_db.fig");

        aws->at("close");
        aws->callback(import_window_close_cb);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("help");
        aws->callback(makeHelpCallback("arb_import.hlp"));
        aws->create_button("HELP", "HELP", "H");

        AW_create_fileselection(aws, AWAR_IMPORT_FILEBASE,   "imp_", "PWD",     ANY_DIR,    true);  // select import filename
        AW_create_fileselection(aws, AWAR_IMPORT_FORMATBASE, "",     "ARBHOME", MULTI_DIRS, false); // select import filter

        aws->at("auto");
        aws->callback(detect_input_format_cb);
        aws->create_autosize_button("AUTO_DETECT", "AUTO DETECT", "A");

        aws->at("ali");
        aws->create_input_field(AWAR_IMPORT_ALI, 4);

        aws->at("type");
        aws->create_option_menu(AWAR_IMPORT_ALI_TYPE, true);
        aws->insert_option        ("dna",     "d", "dna");
        aws->insert_default_option("rna",     "r", "rna");
        aws->insert_option        ("protein", "p", "ami");
        aws->update_option_menu();

        aws->at("protect");
        aws->create_option_menu(AWAR_IMPORT_ALI_PROTECTION, true);
        aws->insert_option("0", "0", 0);
        aws->insert_option("1", "1", 1);
        aws->insert_option("2", "2", 2);
        aws->insert_option("3", "3", 3);
        aws->insert_default_option("4", "4", 4);
        aws->insert_option("5", "5", 5);
        aws->insert_option("6", "6", 6);
        aws->update_option_menu();

        aws->at("genom");
        aws->create_toggle_field(AWAR_IMPORT_GENOM_DB);
        aws->sens_mask(AWM_EXP);
        aws->insert_toggle("Import genome data in EMBL, GenBank and DDBJ format", "e", IMP_GENOME_FLATFILE);
        aws->sens_mask(AWM_ALL);
        aws->insert_toggle("Import selected format", "f", IMP_PLAIN_SEQUENCE);
        aws->update_toggle_field();

        aws->at("autoconf");
        aws->create_toggle(AWAR_IMPORT_AUTOCONF);

        aws->at("go");
        aws->callback(import_go_cb);
        aws->highlight();
        aws->create_button("GO", "GO", "G");
    }
    aws->activate();
}
