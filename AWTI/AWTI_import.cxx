#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <memory.h>
#include <limits.h>

#include <map>
#include <string>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_global.hxx>
#include <awt.hxx>
#include <awt_advice.hxx>
#include <awt_changekey.hxx>
#include <awt_sel_boxes.hxx>
#include <GEN.hxx>
#include <GenomeImport.h>
#include <AW_rename.hxx>
#include <awti_import.hxx>
#include <awti_imp_local.hxx>

#define awti_assert(cond) arb_assert(cond)

using namespace std;

struct awtcig_struct awtcig;
#define MAX_COMMENT_LINES 2000

// -------------------------------------------------------------------
//      static char * awtc_fgets(char *s, int size, FILE *stream)
// -------------------------------------------------------------------
// same as fgets but also works with file in MACOS format

static char *awtc_fgets(char *s, int size, FILE *stream) {
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

AW_BOOL awtc_read_string_pair(FILE *in, char *&s1, char *&s2, size_t& lineNr) {
    // helper function to read import/export filters.
    // returns AW_TRUE if sucessfully read
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
    } while (res && !s1); // read until EOF or something found

    return res;
}


GB_ERROR awtc_read_import_format(const char *file) {
    GB_ERROR  error    = 0;
    char     *fullfile = AWT_unfold_path(file,"ARBHOME");
    FILE     *in       = fopen(fullfile,"r");

    if (!in) error = GBS_global_string("Form not readable ('%s')", fullfile);
    else {
        struct input_format_struct   *ifo;
        struct input_format_per_line *pl = 0;
        

        if (awtcig.ifo) { delete awtcig.ifo; awtcig.ifo = 0; }
        ifo = awtcig.ifo = new input_format_struct;

        char   *s1, *s2;
        size_t  lineNumber = 0;
        while (!error && awtc_read_string_pair(in, s1, s2, lineNumber)) {
            if      (!strcmp(s1,"AUTODETECT"))               { ifo->autodetect     = s2; s2 = 0; }
            else if (!strcmp(s1,"SYSTEM"))                   { ifo->system         = s2; s2 = 0; }
            else if (!strcmp(s1,"NEW_FORMAT"))               { ifo->new_format     = s2; s2 = 0; }
            else if (!strcmp(s1,"KEYWIDTH"))                 { ifo->tab            = atoi(s2); }
            else if (!strcmp(s1,"BEGIN"))                    { ifo->begin          = s2; s2 = 0; }
            else if (!strcmp(s1,"SEQUENCESTART"))            { ifo->sequencestart  = s2; s2 = 0; ifo->read_this_sequence_line_too = 1; }
            else if (!strcmp(s1,"SEQUENCEAFTER"))            { ifo->sequencestart  = s2; s2 = 0; ifo->read_this_sequence_line_too = 0; }
            else if (!strcmp(s1,"FILETAG"))                  { ifo->filetag        = s2; s2 = 0; }
            else if (!strcmp(s1,"SEQUENCESRT"))              { ifo->sequencesrt    = s2; s2 = 0; }
            else if (!strcmp(s1,"SEQUENCEACI"))              { ifo->sequenceaci    = s2; s2 = 0; }
            else if (!strcmp(s1,"SEQUENCECOLUMN"))           { ifo->sequencecolumn = atoi(s2); }
            else if (!strcmp(s1,"SEQUENCEEND"))              { ifo->sequenceend    = s2; s2 = 0; }
            else if (!strcmp(s1,"END"))                      { ifo->end            = s2; s2 = 0; }
            else if (!strcmp(s1,"CREATE_ACC_FROM_SEQUENCE")) { ifo->autocreateacc = 1; }
            else if (!strcmp(s1,"DONT_GEN_NAMES")) { ifo->noautonames = 1; }
            else if (!strcmp(s1,"MATCH")) {
                pl = (struct input_format_per_line *)GB_calloc(1, sizeof(struct input_format_per_line));

                pl->start_line = lineNumber;
                pl->next       = ifo->pl;               // this concatenates the filters to the front -> that is corrected below
                ifo->pl        = pl;
                pl->match      = GBS_remove_escape(s2);
            }
            else if (pl && !strcmp(s1,"SRT"))    { pl->srt    = s2; s2 = 0; }
            else if (pl && !strcmp(s1,"ACI"))    { pl->aci    = s2; s2 = 0; }
            else if (pl && !strcmp(s1,"WRITE"))  { pl->write  = s2; s2 = 0; }
            else if (pl && !strcmp(s1,"APPEND")) { pl->append = s2; s2 = 0; }
            else if (pl && !strcmp(s1,"SETVAR")) {
                if (strlen(s2) != 1 || s2[0]<'a' || s2[0]>'z') {
                    error = "Allowed variable names are a-z";
                }
                else {
                    pl->setvar = s2;
                    s2         = 0;
                }
            }
            else if (pl && !strcmp(s1,"TAG")) {
                if (strlen(s2)>0) {
                    pl->tag = s2;
                    s2      = 0;
                }
            }
            else {
                error = GBS_global_string("Unknown%s command '%s'\n", (pl ? "" : " (or wrong placed)"), s1);
            }

            free(s1);
            free(s2);
        }

        // reverse order of match list (was appended backwards during creation)
        if (ifo->pl) ifo->pl = ifo->pl->reverse(0);

        if (error) error = GBS_global_string("%s\n(in import format file '%s')", error, fullfile);

        fclose(in);
    }

    free(fullfile);
    
    return error;
}

input_format_struct::input_format_struct(void){
    memset((char *)this,0,sizeof(input_format_struct));
}

input_format_struct::~input_format_struct(void)
{
    struct input_format_struct *ifo= this;
    struct input_format_per_line *pl1 = 0;
    struct input_format_per_line *pl2 = 0;

    for (pl1 = ifo->pl; pl1; pl1=pl2){
        pl2 = pl1->next;
        free(pl1->match);
        free(pl1->srt);
        free(pl1->aci);
        free(pl1->tag);
        free(pl1->append);
        free(pl1->write);
        free(pl1);
    }

    free(ifo->autodetect);
    free(ifo->system);
    free(ifo->new_format);
    free(ifo->begin);
    free(ifo->sequencestart);
    free(ifo->sequenceend);
    free(ifo->sequencesrt);
    free(ifo->sequenceaci);
    free(ifo->end);
    free(ifo->b1);
    free(ifo->b2);
}

void awtc_check_input_format(AW_window *aww)
{
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
            gb_assert(GB_is_privatefile(mid_file_name, GB_FALSE));
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
       different files are seperated by sequenceend,
       tab != 0 join lines that start after position tab,
       joined lines are seperated by '|'
       except lines that match sequencestart
       (they may be part of sequence if read_this_sequence_line_too = 1 */

    static char *in_queue = 0;      // read data
    static int b2offset = 0;
    const int BUFSIZE = 8000;
    const char *SEPERATOR = "|";    // line seperator
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
        strncpy(ifo->b2+b2offset, SEPERATOR, BUFSIZE - 4- b2offset);
        b2offset += strlen(ifo->b2+b2offset);

        p = ifo->b1;
        if (b2offset>0) while (*p==' ') p++;    // delete spaces in second line

        strncpy(ifo->b2+b2offset, p, BUFSIZE - 4- b2offset);
        b2offset += strlen(ifo->b2+b2offset);
    }
    in_queue = 0;
    return ifo->b2;
}

static void awtc_write_entry(GBDATA *gbd,const char *key,char *str,const char *tag,int append)
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
            if (!strcmp(key,"name")){
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

typedef map<char, string> SetVariables;

static string expandSetVariables(const SetVariables& variables, const string& source, bool& error_occurred) {
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
                SetVariables::const_iterator var = variables.find(*p);
                if (var == variables.end()) {
                    char *error    = GBS_global_string_copy("Undefined variable '$%c'", *p);
                    dest.append(GBS_global_string("<%s>", error));
                    GB_export_error(error);
                    free(error);
                    error_occurred = true;
                    // return "<error>";
                }
                else {
                    dest.append(var->second);
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

GB_ERROR awtc_read_data(char *ali_name)
{
    char num[6];
    char text[100];
    static int  counter = 0;
    GBDATA *gb_species_data = GB_search(GB_MAIN,"species_data",GB_CREATE_CONTAINER);
    GBDATA *gb_species;
    char *p;
    struct input_format_struct *ifo;
    struct input_format_per_line *pl = 0;
    ifo = awtcig.ifo;

    while (1){              // go to the start
        p = awtc_read_line(0,ifo->sequencestart,ifo->sequenceend);
        if (!p || !ifo->begin || GBS_string_matches(p,ifo->begin,GB_MIND_CASE)) break;
    }
    if (!p) return "Cannot find start of file: Wrong format or empty file";

    // lets start !!!!!
    while(p){
        SetVariables variables;
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
                GB_ERROR msg = GB_export_error("A database entry in file '%s' is longer than %i lines.\n"
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
                            string expanded = expandSetVariables(variables, pl->srt, err_flag);
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
                            string expanded     = expandSetVariables(variables, pl->aci, err_flag);
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
                                string expanded_field = expandSetVariables(variables, string(pl->append ? pl->append : pl->write), err_flag);
                                if (err_flag) error   = GB_await_error();
                                else   field          = GBS_string_2_key(expanded_field.c_str());
                                if (error) what_error = "APPEND or WRITE";
                            }

                            if (!error && pl->tag) {
                                bool   err_flag;
                                string expanded_tag = expandSetVariables(variables, string(pl->tag), err_flag);
                                if (err_flag) error = GB_await_error();
                                else   tag          = GBS_string_2_key(expanded_tag.c_str());
                                if (error) what_error = "TAG";
                            }

                            if (!error) {
                                awtc_write_entry(gb_species, field, s, tag, pl->append != 0);
                            }
                            free(tag);
                            free(field);
                        }

                        if (!error && pl->setvar) {
                            variables[pl->setvar[0]]  = s;
                        }
                        free(dele);
                    }

                    if (error) {
                        error =  GBS_global_string("'%s' in %s of MATCH at #%i", error, what_error, pl->start_line);
                    }
                }
            }

            if (error) {
                return GBS_global_string("\"%s\" at line #%i of species #%i", error, line, counter);
            }

            if (GBS_string_matches(p,ifo->sequencestart,GB_MIND_CASE)) goto read_sequence;

            p = awtc_read_line(ifo->tab,ifo->sequencestart,ifo->sequenceend);
            if (!p) break;
        }
        return GB_export_error("No Start of Sequence found (%i lines read)", max_line);

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

            GBDATA *gb_data = GBT_add_data(gb_species,ali_name,"data", GB_STRING);
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
            GB_write_security_write(gb_data,4);

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

    if (!error) {
        char *ali_type;
        ali_type = awr->awar(AWAR_ALI_TYPE)->read_string();

        if (is_genom_db && strcmp(ali_type, "dna")!=0) {
            error = "You must set the alignment type to dna when importing genom sequences.";
        }
        else {
            int ali_protection = awr->awar(AWAR_ALI_PROTECTION)->read_int();
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
                        GB_warning("Import error: %s", error_this_file);
                        GB_abort_transaction(GB_MAIN);
                        failed_imports++;
                    }

                    aw_status((curr+1)/double(count));
                }

                if (!successfull_imports) {
                    error = "Nothing has been imported";
                }
                else {
                    GB_warning("%i of %i files were imported with success", successfull_imports, (successfull_imports+failed_imports));
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
                                error = "Only one level of form nesting (NEW_FORMAT) allowed";
                            }
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
                
                error = awtc_read_data(ali_name);

                if (error) {
                    error = GBS_global_string("Error: %s reading file %s", error, awtcig.current_file[-1]);
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
#endif // DEVEL_RALF
    awtcig.gb_main = GB_open("noname.arb","wc");
    awtcig.func    = func;
    awtcig.cd1     = cd1;
    awtcig.cd2     = cd2;

    awtcig.gb_other_main = gb_main;

    awtcig.doExit = do_exit; // change/set behavior of CLOSE button

    aw_create_selection_box_awars(awr, AWAR_FILE_BASE, ".", "", defname);
    aw_create_selection_box_awars(awr, AWAR_FORM, GB_path_in_ARBLIB("import", NULL), ".ift", "*");

    awr->awar_string(AWAR_ALI,"dummy"); // these defaults are never used
    awr->awar_string(AWAR_ALI_TYPE,"dummy"); // they are overwritten by AWTC_import_set_ali_and_type
    awr->awar_int(AWAR_ALI_PROTECTION, 0); // which is called via genom_flag_changed() below

    awr->awar(AWAR_READ_GENOM_DB)->add_callback(genom_flag_changed);
    genom_flag_changed(awr);

    if (aws){
        aws->show();
        return GB_MAIN;
    }

    aws = new AW_window_simple;

    aws->init( awr, "ARB_IMPORT","ARB IMPORT");
    aws->load_xfig("awt/import_db.fig");

    aws->at("close");
    aws->callback(import_window_close_cb);
    aws->create_button("CLOSE", "CLOSE","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"arb_import.hlp");
    aws->create_button("HELP", "HELP","H");

    awt_create_selection_box(aws, AWAR_FILE_BASE, "imp_", "PWD", AW_TRUE, AW_TRUE); // select import filename
    awt_create_selection_box(aws, AWAR_FORM, "", "ARBHOME", AW_FALSE, AW_FALSE); // select import filter

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
    aws->insert_toggle("Import genome data in EMBL, GenBank and DDBJ format","e", IMP_GENOME_FLATFILE);
    aws->insert_toggle("Import selected format","f",IMP_PLAIN_SEQUENCE);
    aws->update_toggle_field();

    aws->at("go");
    aws->callback(AWTC_import_go_cb);
    aws->highlight();
    aws->create_button("GO", "GO","G");

    aws->show();
    return GB_MAIN;
}
