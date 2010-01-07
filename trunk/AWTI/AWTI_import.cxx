#include <awti_imp_local.hxx>

#include <awt.hxx>
#include <awt_advice.hxx>
#include <awt_sel_boxes.hxx>
#include <awt_item_sel_list.hxx>

#include <aw_global.hxx>
#include <AW_rename.hxx>

#include <GenomeImport.h>
#include <GEN.hxx>

#include <climits>

using namespace std;

static awtcig_struct awtcig;
#define MAX_COMMENT_LINES 2000

inline const char *name_only(const char *fullpath) {
    const char *lslash = strrchr(fullpath, '/');
    return lslash ? lslash+1 : fullpath;
}

static char *awtc_fgets(char *s, int size, FILE *stream) {
    // same as fgets but also works with file in MacOS format
    int i;
    for (i = 0; i<(size-1); ++i) {
        int byte = fgetc(stream);
        if (byte == EOF) {
            if (i == 0) return 0;
            break;
        }

        s[i] = byte;
        if (byte == '\n' || byte == '\r') break;
    }
    s[i] = 0;

    return s;
}

bool awtc_read_string_pair(FILE *in, char *&s1, char *&s2, size_t& lineNr) {
    // helper function to read import/export filters.
    // returns true if successfully read
    // 
    // 's1' is set to a heap-copy of the first token on line
    // 's2' is set to a heap-copy of the rest of the line (or NULL if only one token is present)
    // 'lineNr' is incremented with each line read

    s1 = 0;
    s2 = 0;

    const int  BUFSIZE = 8000;
    char       buffer[BUFSIZE];
    char      *res;

    do {
        res = awtc_fgets(&buffer[0], BUFSIZE-1, in);
        if (res)  {
            char *p = buffer;

            lineNr++;

            while (*p == ' ' || *p == '\t') p++;
            if (*p != '#') {
                int len = strlen(p)-1;
                while  (len >= 0 && strchr("\t \n\r", p[len])) {
                    p[len--] = 0;
                }

                if (*p) {
                    char *e = strpbrk(p," \t");

                    if (e) {
                        *(e++) = 0;
                        s1     = strdup(p);

                        e += strspn(e, " \t"); // skip whitespace

                        if (*e == '"') {
                            char *k = strrchr(e,'"');
                            if (k!=e) {
                                e++;
                                *k=0;
                            }
                        }
                        s2 = strdup(e);
                    }
                    else { 
                        s1 = strdup(p);
                    }
                }
            }
        }
    } while (res && !s1);                           // read until EOF or something found

    awti_assert(!res == !s1);

    return res;
}

static GB_ERROR not_in_match_error(const char *cmd) {
    return GBS_global_string("Command '%s' may only appear after 'MATCH'", cmd);
}

static GB_ERROR read_import_format(const char *fullfile, input_format_struct *ifo, bool *var_set, bool included) {
    GB_ERROR  error = 0;
    FILE     *in    = fopen(fullfile,"rt");

    input_format_per_line *pl = ifo->pl;

    if (!in) {
        const char *name = name_only(fullfile);

        if (included) {
            error = GB_export_IO_error("including", name);
        }
        else {
            error = strchr(name, '*')
                ? "Please use 'AUTO DETECT' or manually select an import format"
                : GB_export_IO_error("loading import filter", name);
        }
    }
    else {
        char   *s1, *s2;
        size_t  lineNumber    = 0;
        bool    include_error = false;

        while (!error && awtc_read_string_pair(in, s1, s2, lineNumber)) {

#define GLOBAL_COMMAND(cmd) (!error && strcmp(s1, cmd) == 0)
#define MATCH_COMMAND(cmd)  (!error && strcmp(s1, cmd) == 0 && (pl || !(error = not_in_match_error(cmd))))

            if (GLOBAL_COMMAND("MATCH")) {
                pl = new input_format_per_line;

                pl->defined_at = GBS_global_string_copy("%zi,%s", lineNumber, name_only(fullfile));
                pl->next       = ifo->pl;           // this concatenates the filters to the front -> the list is reversed below
                ifo->pl        = pl;
                pl->match      = GBS_remove_escape(s2);
                pl->type       = GB_STRING;

                if (ifo->autotag) pl->mtag = strdup(ifo->autotag); // will be overwritten by TAG command
            }
            else if (MATCH_COMMAND("SRT"))         { reassign(pl->srt, s2); }
            else if (MATCH_COMMAND("ACI"))         { reassign(pl->aci, s2); }
            else if (MATCH_COMMAND("WRITE"))       { reassign(pl->write, s2); }
            else if (MATCH_COMMAND("WRITE_INT"))   { reassign(pl->write, s2); pl->type = GB_INT; }
            else if (MATCH_COMMAND("WRITE_FLOAT")) { reassign(pl->write, s2); pl->type = GB_FLOAT; }
            else if (MATCH_COMMAND("APPEND"))      { reassign(pl->append, s2); }
            else if (MATCH_COMMAND("SETVAR")) {
                if (strlen(s2) != 1 || s2[0]<'a' || s2[0]>'z') {
                    error = "Allowed variable names are a-z";
                }
                else {
                    var_set[s2[0]-'a'] = true;
                    reassign(pl->setvar, s2);
                }
            }
            else if (MATCH_COMMAND("TAG")) {
                if (s2[0]) reassign(pl->mtag, s2);
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
                char *dir         = AWT_extract_directory(fullfile);
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

GB_ERROR awtc_read_import_format(const char *file) {
    char *fullfile = AWT_unfold_path(file,"ARBHOME");

    delete awtcig.ifo;
    awtcig.ifo = new input_format_struct;

    bool var_set[IFS_VARIABLES];
    for (int i = 0; i<IFS_VARIABLES; i++) var_set[i] = false;

    GB_ERROR error = read_import_format(fullfile, awtcig.ifo, var_set, false);


    for (int i = 0; i<IFS_VARIABLES && !error; i++) {
        bool ifnotset = awtcig.ifo->variable_errors.get(i+'a');
        if (var_set[i]) {
            bool isglobal = awtcig.ifo->global_variables.get(i+'a');
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
    if (awtcig.ifo->pl) awtcig.ifo->pl = awtcig.ifo->pl->reverse(0);

    free(fullfile);

    return error;
}

input_format_per_line::input_format_per_line() {
    memset((char *)this,0,sizeof(*this));
}

input_format_per_line::~input_format_per_line() {
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

input_format_struct::input_format_struct() {
    memset((char *)this,0,sizeof(*this));
}

input_format_struct::~input_format_struct() {
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

    delete pl;
}

void awtc_check_input_format(AW_window *aww) {
    AW_root   *root  = aww->get_root();
    char     **files = GBS_read_dir(GB_path_in_ARBLIB("import", NULL), "*.ift");
    char       buffer[AWTC_IMPORT_CHECK_BUFFER_SIZE+10];
    GB_ERROR   error = 0;

    int matched       = -1;
    int first         = -1;
    int matched_count = 0;

    AW_awar *awar_filter   = root->awar(AWAR_FORM"/file_name");
    char    *prev_selected = awar_filter->read_string();

    for (int idx = 0; files[idx] && !error; ++idx) {
        char *filtername = files[idx];
        awar_filter->write_string(filtername);

        GB_ERROR form_err = awtc_read_import_format(filtername);
        if (form_err) {
            aw_message(form_err);
        }
        else {
            if (awtcig.ifo->autodetect) {      // detectable
                char *autodetect = GBS_remove_escape(awtcig.ifo->autodetect);
                delete awtcig.ifo; awtcig.ifo = 0;

                FILE *in = 0;
                {
                    char *f = root->awar(AWAR_FILE)->read_string();

                    if (f[0]) {
                        const char *com = GBS_global_string("cat %s 2>/dev/null", f);
                        in              = popen(com,"r");
                    }
                    free(f);
                    if (!in) error = "Cannot open any input file";
                }
                if (in) {
                    int size = fread(buffer,1,AWTC_IMPORT_CHECK_BUFFER_SIZE,in);
                    pclose(in);
                    if (size>=0){
                        buffer[size] = 0;
                        if (GBS_string_matches(buffer,autodetect,GB_MIND_CASE)) {
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
            AWT_advice("Not all formats can be auto-detected.\n"
                       "Some need to be selected manually.",
                       AWT_ADVICE_TOGGLE,
                       "No format auto-detected",
                       "arb_import.hlp");

            select = "unknown.ift";
            break;

        default:
            AWT_advice("Several import filters matched during auto-detection.\n"
                       "Click 'AUTO DETECT' again to select next matching import-filter.",
                       AWT_ADVICE_TOGGLE,
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
    GBT_free_names(files);
}

static int awtc_next_file(void) {
    if (awtcig.in) fclose(awtcig.in);
    if (!awtcig.current_file) awtcig.current_file = awtcig.filenames;

    int result = 1;
    while (result == 1 && *awtcig.current_file) {
        const char *origin_file_name = *(awtcig.current_file++);
        const char *sorigin          = strrchr(origin_file_name,'/');
        if (!sorigin) sorigin  = origin_file_name;
        else sorigin++;

        GB_ERROR  error          = 0;
        char     *mid_file_name  = 0;
        char     *dest_file_name = 0;

        if (awtcig.ifo2 && awtcig.ifo2->system) {
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
                char *sys = GBS_string_eval(awtcig.ifo2->system, srt, 0);

                aw_status(GBS_global_string("exec '%s'", awtcig.ifo2->system));

                error                        = GB_system(sys);
                if (!error) origin_file_name = mid_file_name;

                free(sys);
                free(srt);
            }
        }

        if (!error && awtcig.ifo->system) {
            {
                const char *sorigin_nameonly            = strrchr(sorigin, '/');
                if (!sorigin_nameonly) sorigin_nameonly = sorigin;

                char *dest_name = GB_unique_filename(sorigin_nameonly, "tmp");
                dest_file_name  = GB_create_tempfile(dest_name);
                free(dest_name);

                if (!dest_file_name) error = GB_await_error();
            }
            
            if (!error) {
                char *srt = GBS_global_string_copy("$<=%s:$>=%s", origin_file_name, dest_file_name);
                char *sys = GBS_string_eval(awtcig.ifo->system, srt, 0);

                aw_status(GBS_global_string("Converting File %s", awtcig.ifo->system));

                error                        = GB_system(sys);
                if (!error) origin_file_name = dest_file_name;

                free(sys);
                free(srt);
            }
        }

        if (!error) {
            awtcig.in = fopen(origin_file_name,"r");   

            if (awtcig.in) {
                result = 0;
            }
            else {
                error = GBS_global_string("Error: Cannot open file %s\n", awtcig.current_file[-1]);
            }
        }

        if (mid_file_name)  {
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
char *awtc_read_line(int tab,char *sequencestart, char *sequenceend){
    /* two modes:   tab == 0 -> read single lines,
       different files are separated by sequenceend,
       tab != 0 join lines that start after position tab,
       joined lines are separated by '|'
       except lines that match sequencestart
       (they may be part of sequence if read_this_sequence_line_too = 1 */

    static char *in_queue = 0;      // read data
    static int b2offset = 0;
    const int BUFSIZE = 8000;
    const char *SEPARATOR = "|";    // line separator
    struct input_format_struct *ifo;
    ifo = awtcig.ifo;
    char *p;

    if (!ifo->b1) ifo->b1 = (char*)calloc(BUFSIZE,1);
    if (!ifo->b2) ifo->b2 = (char*)calloc(BUFSIZE,1);
    if (!awtcig.in){
        if (awtc_next_file()) {
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
        p = awtc_fgets(ifo->b1, BUFSIZE-3, awtcig.in);
        if (!p){
            sprintf(ifo->b1,"%s",sequenceend);
            if (awtcig.in) fclose(awtcig.in);awtcig.in= 0;
            p = ifo->b1;
        }
        int len = strlen(p)-1;
        while (len>=0){
            if (p[len] =='\n' || p[len] == 13) p[len--] = 0;
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
        if (GBS_string_matches(ifo->b2,sequencestart,GB_MIND_CASE)) return ifo->b2;
    }
    while (1) {
        p = awtc_fgets(ifo->b1, BUFSIZE-3, awtcig.in);
        if (!p){
            if (awtcig.in) fclose(awtcig.in);awtcig.in= 0;
            break;
        }
        int len = strlen(p)-1;
        while (len>=0){
            if (p[len] =='\n' || p[len] == 13) p[len--] = 0;
            else break;
        }


        if (GBS_string_matches(ifo->b1,sequencestart,GB_MIND_CASE)){
            in_queue = ifo->b1;
            return ifo->b2;
        }

        int i;
        for (i=0;i<=tab;i++) if (ifo->b1[i] != ' ') break;

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

static void awtc_write_entry(GBDATA *gbd,const char *key,char *str,const char *tag,int append, GB_TYPES type = GB_STRING)
{
    GBDATA *gbk;
    int len, taglen;
    char *buf;
    if (!gbd) return;
    while (str[0] == ' '|| str[0] == '\t'|| str[0] == '|') str++;
    len = strlen(str) -1;
    while (len >=0 &&
           (str[len] == ' ' || str[len] == '\t' || str[len] == '|' || str[len] == 13))
    {
        len--;
    }
    str[len+1] = 0;
    if (!str[0]) return;

    gbk = GB_entry(gbd,key);

    if (type != GB_STRING) {
        if (!gbk) gbk=GB_create(gbd,key,type);
        switch(type) {
            case GB_INT:
                GB_write_int(gbk, atoi(str));
                break;
            case GB_FLOAT:
                GB_write_float(gbk, atof(str));
                break;
            default:
                awti_assert(0);
                break;
        }
        return;
    }

    if (!gbk || !append){
        if (!gbk) gbk=GB_create(gbd,key,GB_STRING);

        if (tag){
            GBS_strstruct *s = GBS_stropen(10000);
            GBS_chrcat(s,'[');
            GBS_strcat(s,tag);
            GBS_strcat(s,"] ");
            GBS_strcat(s,str);
            char *val = GBS_strclose(s);
            GB_write_string(gbk,val);
            free(val);
        }
        else {
            if (strcmp(key,"name") == 0) {
                char *nstr = GBT_create_unique_species_name(awtcig.gb_main,str);
                GB_write_string(gbk,nstr);
                free(nstr);
            }
            else {
                GB_write_string(gbk,str);
            }
        }
        return;
    }

    const char *strin  = GB_read_char_pntr(gbk);
    len    = strlen(str) + strlen(strin);
    taglen = tag ? (strlen(tag)+2) : 0;
    buf    = (char *)GB_calloc(sizeof(char),len+2+taglen+1);

    if (tag) {
        char *regexp = (char*)GB_calloc(sizeof(char), taglen+3);
        sprintf(regexp, "*[%s]*", tag);

        if (!GBS_string_matches(strin, regexp, GB_IGNORE_CASE)) { // if tag does not exist yet
            sprintf(buf,"%s [%s] %s", strin, tag, str); // prefix with tag
        }
        free(regexp);
    }

    if (buf[0] == 0) {
        sprintf(buf,"%s %s",strin,str);
    }

    GB_write_string(gbk,buf);
    free(buf);
    return;
}

static string expandSetVariables(const SetVariables& variables, const string& source, bool& error_occurred, const input_format_struct *ifo) {
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

GB_ERROR awtc_read_data(char *ali_name, int security_write)
{
    char        num[6];
    char        text[100];
    static int  counter         = 0;
    GBDATA     *gb_species_data = GB_search(GB_MAIN,"species_data",GB_CREATE_CONTAINER);
    GBDATA     *gb_species;
    char       *p;

    input_format_struct   *ifo = awtcig.ifo;
    input_format_per_line *pl  = 0;

    while (1){              // go to the start
        p = awtc_read_line(0,ifo->sequencestart,ifo->sequenceend);
        if (!p || !ifo->begin || GBS_string_matches(p,ifo->begin,GB_MIND_CASE)) break;
    }
    if (!p) return "Cannot find start of file: Wrong format or empty file";

    // lets start !!!!!
    while(p) {
        SetVariables variables(ifo->global_variables);

        counter++;
        sprintf(text,"Reading species %i",counter);
        if (counter %10 == 0){
            if (aw_status(text)) break;
        }

        gb_species = GB_create_container(gb_species_data,"species");
        sprintf(text,"spec%i",counter);
        GBT_readOrCreate_char_pntr(gb_species,"name",text); // set default if missing
        if ( awtcig.filenames[1] ) {    // multiple files !!!
            char *f= strrchr(awtcig.current_file[-1],'/');
            if (!f) f= awtcig.current_file[-1];
            else f++;
            awtc_write_entry(gb_species,"file",f,ifo->filetag,0);
        }
        int line;
        static bool never_warn = false;
        int max_line = never_warn ? INT_MAX : MAX_COMMENT_LINES;

        for(line=0;line<=max_line;line++){
            sprintf(num,"%i  ",line);
            if (line == max_line){
                char *file = 0;
                if (awtcig.current_file[0]) file = awtcig.current_file[0];
                GB_ERROR msg = GB_export_errorf("A database entry in file '%s' is longer than %i lines.\n"
                                                "    This might be the result of a wrong input format\n"
                                                "    or a long comment in a sequence\n",file,line);

                switch (aw_question(msg,"Continue Reading,Continue Reading (Never ask again),Abort"))  {
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
            if (strlen(p) > ifo->tab){
                for (pl = ifo->pl; !error && pl; pl=pl->next) {
                    const char *what_error = 0;
                    if (GBS_string_matches(p,pl->match,GB_MIND_CASE)){
                        char *dup = p+ifo->tab;
                        while (*dup == ' ' || *dup == '\t') dup++;

                        char *s              = 0;
                        char *dele           = 0;

                        if (pl->srt){
                            bool   err_flag;
                            string expanded = expandSetVariables(variables, pl->srt, err_flag, ifo);
                            if (err_flag) error = GB_await_error();
                            else {
                                dele           = s = GBS_string_eval(dup,expanded.c_str(),gb_species);
                                if (!s) error  = GB_await_error();
                            }
                            if (error) what_error = "SRT";
                        }
                        else {
                            s = dup;
                        }

                        if (!error && pl->aci){
                            bool   err_flag;
                            string expanded     = expandSetVariables(variables, pl->aci, err_flag, ifo);
                            if (err_flag) error = GB_await_error();
                            else {
                                dup           = dele;
                                dele          = s = GB_command_interpreter(GB_MAIN, s,expanded.c_str(),gb_species, 0);
                                if (!s) error = GB_await_error();
                                free(dup);
                            }
                            if (error) what_error = "ACI";
                        }

                        if (!error && (pl->append || pl->write)) {
                            char *field = 0;
                            char *tag   = 0;

                            {
                                bool   err_flag;
                                string expanded_field = expandSetVariables(variables, string(pl->append ? pl->append : pl->write), err_flag, ifo);
                                if (err_flag) error   = GB_await_error();
                                else   field          = GBS_string_2_key(expanded_field.c_str());
                                if (error) what_error = "APPEND or WRITE";
                            }

                            if (!error && pl->mtag) {
                                bool   err_flag;
                                string expanded_tag = expandSetVariables(variables, string(pl->mtag), err_flag, ifo);
                                if (err_flag) error = GB_await_error();
                                else   tag          = GBS_string_2_key(expanded_tag.c_str());
                                if (error) what_error = "TAG";
                            }

                            if (!error) {
                                awtc_write_entry(gb_species, field, s, tag, pl->append != 0, pl->type);
                            }
                            free(tag);
                            free(field);
                        }

                        if (!error && pl->setvar) variables.set(pl->setvar[0], s);
                        free(dele);
                    }

                    if (error) {
                        error = GBS_global_string("'%s'\nin %s of MATCH (defined at #%s)", error, what_error, pl->defined_at);
                    }
                }
            }

            if (error) {
                return GBS_global_string("%s\nwhile parsing line #%i of species #%i", error, line, counter);
            }

            if (GBS_string_matches(p,ifo->sequencestart,GB_MIND_CASE)) goto read_sequence;

            p = awtc_read_line(ifo->tab,ifo->sequencestart,ifo->sequenceend);
            if (!p) break;
        }
        return GB_export_errorf("No Start of Sequence found (%i lines read)", max_line);

    read_sequence:
        {
            char          *sequence;
            GBS_strstruct *strstruct = GBS_stropen(5000);
            int            linecnt;
            
            for(linecnt = 0;;linecnt++) {
                if (linecnt || !ifo->read_this_sequence_line_too){
                    p = awtc_read_line(0,ifo->sequencestart,ifo->sequenceend);
                }
                if (!p) break;
                if (ifo->sequenceend && GBS_string_matches(p,ifo->sequenceend,GB_MIND_CASE)) break;
                if (strlen(p) <= ifo->sequencecolumn) continue;
                GBS_strcat(strstruct,p+ifo->sequencecolumn);
            }
            sequence = GBS_strclose(strstruct);

            GBDATA *gb_data = GBT_create_sequence_data(gb_species, ali_name, "data", GB_STRING, security_write);
            if (ifo->sequencesrt) {
                char *h = GBS_string_eval(sequence,ifo->sequencesrt,gb_species);
                if (!h) return GB_await_error();
                freeset(sequence, h);
            }

            if (ifo->sequenceaci) {
                char *h = GB_command_interpreter(GB_MAIN, sequence,ifo->sequenceaci,gb_species, 0);
                free(sequence);
                if (!h) return GB_await_error();
                sequence = h;
            }

            GB_write_string(gb_data,sequence);

            GBDATA *gb_acc = GB_search(gb_species,"acc",GB_FIND);
            if (!gb_acc && ifo->autocreateacc) {
                char buf[100];
                long id = GBS_checksum(sequence,1,".-");
                sprintf(buf,"ARB_%lX",id);
                gb_acc = GB_search(gb_species,"acc",GB_STRING);
                GB_write_string(gb_acc,buf);
            }
            free(sequence);
        }
        while (1){              // go to the start of an species
            if (!p || !ifo->begin || GBS_string_matches(p,ifo->begin,GB_MIND_CASE)) break;
            p = awtc_read_line(0,ifo->sequencestart,ifo->sequenceend);
        }
    }
    return 0;
}

void AWTC_import_go_cb(AW_window *aww) // Import sequences into new or existing database
{
    AW_root *awr                     = aww->get_root();
    bool     is_genom_db             = false;
    bool     delete_db_type_if_error = false; // delete db type (genome/normal) in case of error ?
    {
        bool           read_genom_db = (awr->awar(AWAR_READ_GENOM_DB)->read_int() != IMP_PLAIN_SEQUENCE);
        GB_transaction ta(GB_MAIN);

        delete_db_type_if_error = (GB_entry(GB_MAIN, GENOM_DB_TYPE) == 0);
        is_genom_db             = GEN_is_genome_db(GB_MAIN, read_genom_db);

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

    GB_change_my_security(GB_MAIN,6,"");
    
    GB_begin_transaction(GB_MAIN); // first transaction start
    char *ali_name;
    {
        char *ali = awr->awar(AWAR_ALI)->read_string();
        ali_name = GBS_string_eval(ali,"*=ali_*1:ali_ali_=ali_",0);
        free(ali);
    }

    error = GBT_check_alignment_name(ali_name);

    int ali_protection = awr->awar(AWAR_ALI_PROTECTION)->read_int();
    if (!error) {
        char *ali_type;
        ali_type = awr->awar(AWAR_ALI_TYPE)->read_string();

        if (is_genom_db && strcmp(ali_type, "dna")!=0) {
            error = "You must set the alignment type to dna when importing genom sequences.";
        }
        else {
            GBT_create_alignment(GB_MAIN,ali_name,2000,0,ali_protection,ali_type);
        }
        free(ali_type);
    }

    bool ask_generate_names = true;

    if (!error) {
        if (is_genom_db) {
            // import genome flatfile into ARB-genome database:

            char  *mask   = awr->awar(AWAR_FILE)->read_string();
            char **fnames = GBS_read_dir(mask, NULL);

            if (fnames[0] == 0) {
                error = GB_export_error("Cannot find selected file");
            }
            else {
                int successfull_imports = 0;
                int failed_imports      = 0;
                int count;

                for (count = 0; fnames[count]; ++count) ; // count filenames

                GBDATA *gb_species_data = GB_search(GB_MAIN, "species_data", GB_CREATE_CONTAINER);
                ImportSession import_session(gb_species_data, count*10);

                // close the above transaction and do each importfile in separate transaction
                // to avoid that all imports are undone by transaction abort happening in case of error
                GB_commit_transaction(GB_MAIN);

                aw_openstatus("Reading input files");

                for (int curr = 0; !error && fnames[curr]; ++curr) {
                    GB_ERROR error_this_file =  0;

                    GB_begin_transaction(GB_MAIN);
                    {
                        const char *lslash = strrchr(fnames[curr], '/');
                        aw_status(GBS_global_string("%i/%i: %s", curr+1, count, lslash ? lslash+1 : fnames[curr]));
                    }

#if defined(DEBUG)
                    fprintf(stderr, "import of '%s'\n", fnames[curr]);
#endif // DEBUG
                    error_this_file = GI_importGenomeFile(import_session, fnames[curr], ali_name);

                    if (!error_this_file) {
                        GB_commit_transaction(GB_MAIN);
                        // GB_warning("File '%s' successfully imported", fnames[curr]);
                        successfull_imports++;
                        delete_db_type_if_error = false;
                    }
                    else { // error occurred during import
                        error_this_file = GBS_global_string("'%s' not imported\nReason: %s", fnames[curr], error_this_file);
                        GB_warningf("Import error: %s", error_this_file);
                        GB_abort_transaction(GB_MAIN);
                        failed_imports++;
                    }

                    aw_status((curr+1)/double(count));
                }

                if (!successfull_imports) {
                    error = "Nothing has been imported";
                }
                else {
                    GB_warningf("%i of %i files were imported with success", successfull_imports, (successfull_imports+failed_imports));
                }

                aw_closestatus();

                // now open another transaction to "undo" the transaction close above
                GB_begin_transaction(GB_MAIN);
            }

            GBT_free_names(fnames);
            free(mask);
        }
        else {
            // import to non-genome ARB-db :

            {
                // load import filter: 
                char *file = awr->awar(AWAR_FORM"/file_name")->read_string();

                if (!strlen(file)) {
                    error = "Please select a form";
                }
                else {
                    error = awtc_read_import_format(file);
                    if (!error && awtcig.ifo->new_format) {
                        awtcig.ifo2 = awtcig.ifo;
                        awtcig.ifo  = 0;

                        error = awtc_read_import_format(awtcig.ifo2->new_format);
                        if (!error) {
                            if (awtcig.ifo->new_format) {
                                error = GBS_global_string("in line %zi of import filter '%s':\n"
                                                          "Only one level of form nesting (NEW_FORMAT) allowed",
                                                          awtcig.ifo->new_format_lineno, name_only(awtcig.ifo2->new_format));
                            }
                        }
                        if (error) {
                            error = GBS_global_string("in format used in line %zi of '%s':\n%s",
                                                      awtcig.ifo2->new_format_lineno, name_only(file), error);
                        }
                    }
                }
                free(file);
            }

            {
                char *f          = awr->awar(AWAR_FILE)->read_string();
                awtcig.filenames = GBS_read_dir(f,NULL);
                free(f);
            }

            if (awtcig.filenames[0] == 0){
                error = GB_export_error("Cannot find selected file(s)");
            }

            bool status_open = false;
            if (!error) {
                aw_openstatus("Reading input files");
                status_open = true;
                
                error = awtc_read_data(ali_name, ali_protection);

                if (error) {
                    error = GBS_global_string("Error: %s\nwhile reading file %s", error, awtcig.current_file[-1]);
                }
                else {
                    if (awtcig.ifo->noautonames || (awtcig.ifo2 && awtcig.ifo2->noautonames)) {
                        ask_generate_names = false;
                    }
                    else {
                        ask_generate_names = true;
                    }
                }
            }

            delete awtcig.ifo; awtcig.ifo = 0;
            delete awtcig.ifo2; awtcig.ifo2 = 0;

            if (awtcig.in) {
                fclose(awtcig.in);
                awtcig.in = 0;
            }
            
            GBT_free_names(awtcig.filenames);
            awtcig.filenames    = 0;
            awtcig.current_file = 0;

            if (status_open) aw_closestatus();
        }
    }
    free(ali_name);

    bool call_func = true; // shall awtcig.func be called ? 
    if (error) {
        GB_abort_transaction(GB_MAIN);

        if (delete_db_type_if_error) {
            // delete db type, if it was initialized above
            // (avoids 'can't import'-error, if file-type (genome-file/species-file) is changed after a failed try)
            GB_transaction  ta(GB_MAIN);
            GBDATA         *db_type = GB_entry(GB_MAIN, GENOM_DB_TYPE);
            if (db_type) GB_delete(db_type);
        }

        call_func = false;
    }
    else {
        aww->hide(); // import window stays open in case of error

        aw_openstatus("Checking and Scanning database");
        aw_status("Pass 1: Check entries");

        // scan for hidden/unknown fields :
        awt_selection_list_rescan(GB_MAIN, AWT_NDS_FILTER, AWT_RS_UPDATE_FIELDS);
        if (is_genom_db) awt_gene_field_selection_list_rescan(GB_MAIN, AWT_NDS_FILTER, AWT_RS_UPDATE_FIELDS);

        GBT_mark_all(GB_MAIN,1);
        sleep(1);
        aw_status("Pass 2: Check sequence lengths");
        GBT_check_data(GB_MAIN,0);
        sleep(1);

        GB_commit_transaction(GB_MAIN);

        if (ask_generate_names) {
            if (aw_question("You may generate short names using the full_name and accession entry of the species",
                            "Generate new short names (recommended),Use found names")==0)
            {
                aw_status("Pass 3: Generate unique names");
                error = AW_select_nameserver(GB_MAIN, awtcig.gb_other_main);
                if (!error) {
                    error = AWTC_pars_names(GB_MAIN,1);
                }
            }
        }

        aw_closestatus();
    }

    if (error) aw_message(error);
    
    GB_change_my_security(GB_MAIN,0,"");

    if (call_func) awtcig.func(awr, awtcig.cd1,awtcig.cd2);
}

class AliNameAndType {
    string name_, type_;
public:
    AliNameAndType(const char *ali_name, const char *ali_type) : name_(ali_name), type_(ali_type) {}
    AliNameAndType(const AliNameAndType& other) : name_(other.name_), type_(other.type_) {}
    
    const char *name() const { return name_.c_str(); }
    const char *type() const { return type_.c_str(); }
};

static AliNameAndType last_ali("ali_new", "rna"); // last selected ali for plain import (aka non-flatfile import)


void AWTC_import_set_ali_and_type(AW_root *awr, const char *ali_name, const char *ali_type, GBDATA *gbmain) {
    bool           switching_to_GENOM_ALIGNMENT = strcmp(ali_name, GENOM_ALIGNMENT) == 0;
    static GBDATA *last_valid_gbmain            = 0;

    if (gbmain) last_valid_gbmain = gbmain;

    AW_awar *awar_name = awr->awar(AWAR_ALI);
    AW_awar *awar_type = awr->awar(AWAR_ALI_TYPE);

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
        GB_transaction  ta(last_valid_gbmain);
        GBDATA         *gb_ali            = GBT_get_alignment(last_valid_gbmain, ali_name);
        int             protection_to_use = 4; // default protection

        if (gb_ali) {
            GBDATA *gb_write_security = GB_entry(gb_ali,"alignment_write_security");
            if (gb_write_security) {
                protection_to_use = GB_read_int(gb_write_security);
            }
        }
        awr->awar(AWAR_ALI_PROTECTION)->write_int(protection_to_use);
    }
}

static void genom_flag_changed(AW_root *awr) {
    if (awr->awar(AWAR_READ_GENOM_DB)->read_int() == IMP_PLAIN_SEQUENCE) {
        AWTC_import_set_ali_and_type(awr, last_ali.name(), last_ali.type(), 0);
        awr->awar_string(AWAR_FORM"/filter",".ift");
    }
    else {
        AWTC_import_set_ali_and_type(awr, GENOM_ALIGNMENT, "dna", 0);
        awr->awar_string(AWAR_FORM"/filter",".fit"); // *hack* to hide normal import filters
    }
}

static void import_window_close_cb(AW_window *aww) {
    if (awtcig.doExit) exit(EXIT_SUCCESS);
    else AW_POPDOWN(aww);
}

GBDATA *open_AWTC_import_window(AW_root *awr,const char *defname, bool do_exit, GBDATA *gb_main, AWTC_RCB(func), AW_CL cd1, AW_CL cd2)
{
    static AW_window_simple *aws = 0;

#if defined(DEVEL_RALF)
#warning where is awtcig.gb_main closed
    // it is either (currently not) closed by merge tool
    // or closed as main db in ARB_NTREE
#endif // DEVEL_RALF
    awtcig.gb_main = GB_open("noname.arb","wc");
    awtcig.func    = func;
    awtcig.cd1     = cd1;
    awtcig.cd2     = cd2;

#if defined(DEBUG)
    AWT_announce_db_to_browser(awtcig.gb_main, "New database (import)");
#endif // DEBUG

    awtcig.gb_other_main = gb_main;

    awtcig.doExit = do_exit; // change/set behavior of CLOSE button

    aw_create_selection_box_awars(awr, AWAR_FILE_BASE, ".", "", defname);
    aw_create_selection_box_awars(awr, AWAR_FORM, GB_path_in_ARBLIB("import", NULL), ".ift", "*");

    awr->awar_string(AWAR_ALI,"dummy"); // these defaults are never used
    awr->awar_string(AWAR_ALI_TYPE,"dummy"); // they are overwritten by AWTC_import_set_ali_and_type
    awr->awar_int(AWAR_ALI_PROTECTION, 0); // which is called via genom_flag_changed() below

    awr->awar(AWAR_READ_GENOM_DB)->add_callback(genom_flag_changed);
    genom_flag_changed(awr);

    if (!aws) {
        aws = new AW_window_simple;

        aws->init( awr, "ARB_IMPORT","ARB IMPORT");
        aws->load_xfig("awt/import_db.fig");

        aws->at("close");
        aws->callback(import_window_close_cb);
        aws->create_button("CLOSE", "CLOSE","C");

        aws->at("help");
        aws->callback(AW_POPUP_HELP,(AW_CL)"arb_import.hlp");
        aws->create_button("HELP", "HELP","H");

        awt_create_selection_box(aws, AWAR_FILE_BASE, "imp_", "PWD", true, true); // select import filename
        awt_create_selection_box(aws, AWAR_FORM, "", "ARBHOME", false, false); // select import filter

        aws->at("auto");
        aws->callback(awtc_check_input_format);
        aws->create_autosize_button("AUTO_DETECT", "AUTO DETECT","A");

        aws->at("ali");
        aws->create_input_field(AWAR_ALI,4);

        aws->at("type");
        aws->create_option_menu(AWAR_ALI_TYPE);
        aws->insert_option("dna","d","dna");
        aws->insert_option("rna","r","rna");
        aws->insert_option("protein","p","ami");
        aws->update_option_menu();

        aws->at("protect");
        aws->create_option_menu(AWAR_ALI_PROTECTION);
        aws->insert_option("0", "0", 0);
        aws->insert_option("1", "1", 1);
        aws->insert_option("2", "2", 2);
        aws->insert_option("3", "3", 3);
        aws->insert_default_option("4", "4", 4);
        aws->insert_option("5", "5", 5);
        aws->insert_option("6", "6", 6);
        aws->update_option_menu();

        aws->at("genom");
        aws->create_toggle_field(AWAR_READ_GENOM_DB);
        aws->sens_mask(AWM_EXP);
        aws->insert_toggle("Import genome data in EMBL, GenBank and DDBJ format","e", IMP_GENOME_FLATFILE);
        aws->sens_mask(AWM_ALL);
        aws->insert_toggle("Import selected format","f",IMP_PLAIN_SEQUENCE);
        aws->update_toggle_field();

        aws->at("go");
        aws->callback(AWTC_import_go_cb);
        aws->highlight();
        aws->create_button("GO", "GO","G");
    }
    aws->activate();
    return GB_MAIN;
}
