// ================================================================ //
//                                                                  //
//   File      : AW_help.cxx                                        //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "aw_awar.hxx"
#include "aw_window.hxx"
#include "aw_edit.hxx"
#include "aw_root.hxx"
#include "aw_global_awars.hxx"
#include "aw_msg.hxx"
#include "aw_select.hxx"

#include <arb_file.h>
#include <arb_strarray.h>

#include <sys/stat.h>
#include <arb_str.h>


#define AWAR_HELP       "tmp/help/"
#define AWAR_HELPFILE   AWAR_HELP "file"
#define AWAR_HELPTEXT   AWAR_HELP "text"
#define AWAR_HELPSEARCH AWAR_HELP "search"


void AW_openURL(AW_root *aw_root, const char *url) {
    GB_CSTR  ka;
    char    *browser = aw_root->awar(AWAR_WWW_BROWSER)->read_string();

    while ((ka = GBS_find_string(browser, "$(URL)", 0))) {
        char *start       = GB_strpartdup(browser, ka-1);
        char *new_browser = GBS_global_string_copy("%s%s%s", start, url, ka+6);

        free(start);
        free(browser);

        browser = new_browser;
    }

    char *command = GBS_global_string_copy("(%s)&", browser);
    printf("Action: '%s'\n", command);
    if (system(command)) aw_message(GBS_global_string("'%s' failed", command));
    free(command);

    free(browser);
}

// --------------------
//      help window

static struct {
    AW_selection_list *uplinks;
    AW_selection_list *links;
    char              *history;
} HELP;

static char *get_full_qualified_help_file_name(const char *helpfile, bool path_for_edit = false) {
    GB_CSTR   result             = 0;
    char     *user_doc_path      = strdup(GB_getenvDOCPATH());
    char     *devel_doc_path     = strdup(GB_path_in_ARBHOME("HELP_SOURCE/oldhelp"));
    size_t    user_doc_path_len  = strlen(user_doc_path);
    size_t    devel_doc_path_len = strlen(devel_doc_path);

    const char *rel_path = 0;
    if (strncmp(helpfile, user_doc_path, user_doc_path_len) == 0 && helpfile[user_doc_path_len] == '/') {
        rel_path = helpfile+user_doc_path_len+1;
    }
    else if (strncmp(helpfile, devel_doc_path, devel_doc_path_len) == 0 && helpfile[devel_doc_path_len] == '/') {
        aw_assert(0);            // when does this happen ? never ?
        rel_path = helpfile+devel_doc_path_len+1;
    }

    if (helpfile[0]=='/' && !rel_path) {
        result = GBS_static_string(helpfile);
    }
    else {
        if (!rel_path) rel_path = helpfile;

        if (rel_path[0]) {
            if (path_for_edit) {
#if defined(DEBUG)
                char *gen_doc_path = strdup(GB_path_in_ARBHOME("HELP_SOURCE/genhelp"));

                char *devel_source = GBS_global_string_copy("%s/%s", devel_doc_path, rel_path);
                char *gen_source   = GBS_global_string_copy("%s/%s", gen_doc_path, rel_path);

                int devel_size = GB_size_of_file(devel_source);
                int gen_size   = GB_size_of_file(gen_source);

                aw_assert(devel_size <= 0 || gen_size <= 0); // only one of them shall exist

                if (gen_size>0) {
                    result = GBS_static_string(gen_source); // edit generated doc
                }
                else {
                    result = GBS_static_string(devel_source); // use normal help source (may be non-existing)
                }

                free(gen_source);
                free(devel_source);
                free(gen_doc_path);
#else
                result = GBS_global_string("%s/%s", GB_getenvDOCPATH(), rel_path); // use real help file in RELEASE
#endif // DEBUG
            }
            else {
                result = GBS_global_string("%s/%s", GB_getenvDOCPATH(), rel_path);
            }
        }
        else {
            result = "";
        }
    }

    free(devel_doc_path);
    free(user_doc_path);

    return strdup(result);
}

static char *get_full_qualified_help_file_name(AW_root *awr, bool path_for_edit = false) {
    char *helpfile = awr->awar(AWAR_HELPFILE)->read_string();
    char *qualified = get_full_qualified_help_file_name(helpfile, path_for_edit);
    free(helpfile);
    return qualified;
}

static char *get_local_help_url(AW_root *awr) {
    char   *helpfile          = get_full_qualified_help_file_name(awr, false);
    char   *user_doc_path     = strdup(GB_getenvDOCPATH());
    char   *user_htmldoc_path = strdup(GB_getenvHTMLDOCPATH());
    size_t  user_doc_path_len = strlen(user_doc_path);
    char   *result            = 0;

    if (strncmp(helpfile, user_doc_path, user_doc_path_len) == 0) { // "real" help file
        result            = GBS_global_string_copy("%s%s_", user_htmldoc_path, helpfile+user_doc_path_len);
        size_t result_len = strlen(result);

        aw_assert(result_len > 5);

        if (strcmp(result+result_len-5, ".hlp_") == 0) {
            strcpy(result+result_len-5, ".html");
        }
        else {
            freenull(result);
            GB_export_error("Can't browse that file type.");
        }
    }
    else { // on-the-fly-generated help file (e.g. search help)
        GB_export_error("Can't browse temporary help node");
    }

    free(user_htmldoc_path);
    free(user_doc_path);
    free(helpfile);

    return result;
}

#if defined(NDEBUG)
static void store_helpfile_in_tarball(const char *path, const char *mode) {
    GB_ERROR    error = NULL;
    const char *base  = GB_path_in_ARBLIB("help");

    if (ARB_strBeginsWith(path, base)) {
        char *cmd = GBS_global_string_copy("arb_help_useredit.sh %s %s", path+strlen(base)+1, mode);
        error     = GBK_system(cmd);
    }
    else {
        error = "Unexpected helpfile name (in store_helpfile_in_tarball)";
    }

    if (error) aw_message(error);
}
static void aw_helpfile_modified_cb(const char *path, bool fileWasChanged, bool editorTerminated) {
    static enum { UNMODIFIED, MODIFIED, NOTIFIED } state = UNMODIFIED;

    if (fileWasChanged) {
        store_helpfile_in_tarball(path, "end");
        if (state == UNMODIFIED) state = MODIFIED;
    }
    if (editorTerminated) {
        if (state == MODIFIED) {
            aw_message("Your changes to ARB help have been stored in an archive.\n"
                       "See console for what to send to ARB developers!");
            state = NOTIFIED;
        }
    }
}
#endif


static void aw_help_edit_help(AW_window *aww) {
    char *helpfile = get_full_qualified_help_file_name(aww->get_root(), true);

    if (GB_size_of_file(helpfile)<=0) {
#if defined(NDEBUG)
        const char *base = GB_path_in_ARBLIB("help");
#else
        const char *base = GB_path_in_ARBHOME("HELP_SOURCE/oldhelp");
#endif

        const char *copy_cmd = GBS_global_string("cp %s/FORM.hlp %s", base, helpfile); // uses_hlp_res("FORM.hlp"); see ../SOURCE_TOOLS/check_resources.pl@uses_hlp_res
        aw_message_if(GBK_system(copy_cmd));
    }

#if defined(NDEBUG)
    store_helpfile_in_tarball(helpfile, "start");

    if (!GB_is_writeablefile(helpfile)) {
        aw_message("Warning: you do not have the permission to save changes to that helpfile\n"
                   "(ask your admin to gain write access)");
    }

    GBDATA *get_globalawars_gbmain(void); // prototype
    GBDATA *gbmain = get_globalawars_gbmain(); // hack -- really need main ARB DB here (properties DB does not work with notifications)
    if (gbmain) {
        AW_edit(helpfile, aw_helpfile_modified_cb, aww, gbmain);
    }
    else {
        aw_message("Warning: Editing help not possible yet!\n"
                   "To make it possible:\n"
                   "- leave help window open,\n"
                   "- open a database and\n"
                   "- then click EDIT again.");
    }
#else
    AW_edit(helpfile);
#endif

    free(helpfile);
}

static char *aw_ref_to_title(const char *ref) {
    if (!ref) return 0;

    if (GBS_string_matches(ref, "*.ps", GB_IGNORE_CASE)) {   // Postscript file
        return GBS_global_string_copy("Postscript: %s", ref);
    }

    char *result = 0;
    char *file = 0;
    {
        char *helpfile = get_full_qualified_help_file_name(ref);
        file = GB_read_file(helpfile);
        free(helpfile);
    }

    if (file) {
        result = GBS_string_eval(file, "*\nTITLE*\n*=*2:\t=", 0);
        if (strcmp(file, result)==0) freenull(result);
        free(file);
    }
    else {
        GB_clear_error();
    }

    if (result==0) {
        result = strdup(ref);
    }

    return result;
}

static void aw_help_select_newest_in_history(AW_root *aw_root) {
    char *history = HELP.history;
    if (history) {
        const char *sep      = strchr(history, '#');
        char       *lastHelp = sep ? GB_strpartdup(history, sep-1) : strdup(history);

        aw_root->awar(AWAR_HELPFILE)->write_string(lastHelp);
        free(lastHelp);
    }
}

static void aw_help_back(AW_window *aww) {
    AW_root    *aw_root = aww->get_root();
    const char *history = HELP.history;
    if (history) {
        const char *currHelp = aw_root->awar(AWAR_HELPFILE)->read_char_pntr();
        if (currHelp[0]) { // if showing some help
            const char *sep = strchr(history, '#');
            if (sep) {
                char *first = GB_strpartdup(history, sep-1);
                freeset(HELP.history, GBS_global_string_copy("%s#%s", sep+1, first)); // wrap first to end
                free(first);
                aw_help_select_newest_in_history(aw_root);
            }
        }
        else { // e.g. inside history
            aw_help_select_newest_in_history(aw_root);
        }
    }
}

static void aw_help_history(AW_window *aww) {
    ConstStrArray entries;
    GBT_split_string(entries, HELP.history, '#');

    if (entries.size()) {
        AW_root    *aw_root  = aww->get_root();
        const char *currHelp = aw_root->awar(AWAR_HELPFILE)->read_char_pntr();

        if (currHelp[0]) {
            aw_root->awar(AWAR_HELPFILE)->write_string("");

            HELP.uplinks->clear();
            HELP.links->clear();

            for (size_t i = 0; i<entries.size(); ++i) {
                char *title = aw_ref_to_title(entries[i]);
                HELP.links->insert(title, entries[i]);
                free(title);
            }

            HELP.uplinks->insert_default("   ", ""); HELP.uplinks->update();
            HELP.links  ->insert_default("   ", ""); HELP.links  ->update();

            aw_root->awar(AWAR_HELPTEXT)->write_string("Your previously visited help topics are listed under 'Subtopics'");
        }
        else { // e.g. already in HISTORY
            aw_help_back(aww);
        }
    }
}

static GB_ERROR aw_help_show_external_format(const char *help_file, const char *viewer) {
    // Called to show *.ps or *.pdf in external viewer.
    // Can as well show *.suffix.gz (decompresses to temporary *.suffix)

    struct stat st;
    GB_ERROR    error = NULL;
    char        sys[1024];

    sys[0] = 0;

    if (stat(help_file, &st) == 0) { // *.ps exists
        GBS_global_string_to_buffer(sys, sizeof(sys), "%s %s &", viewer, help_file);
    }
    else {
        char *compressed = GBS_global_string_copy("%s.gz", help_file);

        if (stat(compressed, &st) == 0) { // *.ps.gz exists
            char *name_ext;
            GB_split_full_path(compressed, NULL, NULL, &name_ext, NULL);
            // 'name_ext' contains xxx.ps or xxx.pdf
            char *name, *suffix;
            GB_split_full_path(name_ext, NULL, NULL, &name, &suffix);

            char *tempname     = GB_unique_filename(name, suffix);
            char *uncompressed = GB_create_tempfile(tempname);

            GBS_global_string_to_buffer(sys, sizeof(sys),
                                        "(gunzip <%s >%s ; %s %s ; rm %s) &",
                                        compressed, uncompressed,
                                        viewer, uncompressed,
                                        uncompressed);

            free(uncompressed);
            free(tempname);
            free(name);
            free(suffix);
            free(name_ext);
        }
        else {
            error = GBS_global_string("Neither %s nor %s exists", help_file, compressed);
        }
        free(compressed);
    }

    if (sys[0] && !error) error = GBK_system(sys);

    return error;
}

#if defined(DEBUG)
# define TRACK_HELPFILE
#endif

#if defined(TRACK_HELPFILE)
// automatically update helpfile after changes in DEBUG mode

static bool          track_helpfile        = false;
static unsigned long helpfile_stamp        = 0;
static unsigned long helpfile_edited_stamp = 0;

const unsigned TRACK_FREQUENCY = 500; // ms

static unsigned autorefresh_helpfile(AW_root *awr) {
    char     *help_file   = get_full_qualified_help_file_name(awr);
    unsigned  callAgainIn = TRACK_FREQUENCY;

    if (help_file[0]) {
        unsigned long lastChanged = GB_time_of_file(help_file);

        if (lastChanged != helpfile_stamp) {
            awr->awar(AWAR_HELPFILE)->touch(); // reload
            aw_assert(helpfile_stamp == lastChanged);
        }
        else {
            char *edited_help_file = get_full_qualified_help_file_name(awr, true);
            if (strcmp(help_file, edited_help_file) != 0) {
                unsigned long editLastChanged = GB_time_of_file(edited_help_file);

                if (editLastChanged>helpfile_edited_stamp) {
                    GBK_system("cd $ARBHOME; make help");
                    helpfile_edited_stamp = editLastChanged;
                    callAgainIn           = 10;
                }
            }

            free(edited_help_file);
        }
    }
    free(help_file);

    return callAgainIn;
}

#endif

static void aw_help_helpfile_changed_cb(AW_root *awr) {
    char *help_file = get_full_qualified_help_file_name(awr);

#if defined(TRACK_HELPFILE)
    track_helpfile = false;
#endif

    if (!strlen(help_file)) {
        awr->awar(AWAR_HELPTEXT)->write_string("Select one of the topics from the lists on the left side or\n"
                                               "press the BACK button above.");
    }
    else if (GBS_string_matches(help_file, "*.ps", GB_IGNORE_CASE)) { // Postscript file
        GB_ERROR error = aw_help_show_external_format(help_file, GB_getenvARB_GS());
        if (error) aw_message(error);
        aw_help_select_newest_in_history(awr);
    }
    else if (GBS_string_matches(help_file, "*.pdf", GB_IGNORE_CASE)) { // PDF file
        GB_ERROR error = aw_help_show_external_format(help_file, GB_getenvARB_PDFVIEW());
        if (error) aw_message(error);
        aw_help_select_newest_in_history(awr);
    }
    else {
        if (HELP.history) {
            if (strncmp(help_file, HELP.history, strlen(help_file)) != 0) {
                // remove current help from history (if present) and prefix it to history
                char *comm = GBS_global_string_copy("*#%s*=*1*2:*=%s#*1", help_file, help_file);
                char *h    = GBS_string_eval(HELP.history, comm, 0);

                aw_assert(h);
                freeset(HELP.history, h);
                free(comm);
            }
        }
        else {
            HELP.history = strdup(help_file);
        }

#if defined(TRACK_HELPFILE)
        track_helpfile = true;
        helpfile_edited_stamp = helpfile_stamp = GB_time_of_file(help_file);
#endif

        char *helptext = GB_read_file(help_file);
        if (helptext) {
            char *ptr;
            char *h, *h2, *tok;

            ptr = strdup(helptext);
            HELP.uplinks->clear();
            h2 = GBS_find_string(ptr, "\nUP", 0);
            while ((h = h2)) {
                h2 = GBS_find_string(h2+1, "\nUP", 0);
                tok = strtok(h+3, " \n\t");  // now I got UP
                char *title = aw_ref_to_title(tok);
                if (tok) HELP.uplinks->insert(title, tok);
                free(title);
            }
            free(ptr);
            HELP.uplinks->insert_default("   ", "");
            HELP.uplinks->update();

            ptr = strdup(helptext);
            HELP.links->clear();
            h2 = GBS_find_string(ptr, "\nSUB", 0);
            while ((h = h2)) {
                h2 = GBS_find_string(h2+1, "\nSUB", 0);
                tok = strtok(h+4, " \n\t");  // now I got SUB
                char *title = aw_ref_to_title(tok);
                if (tok) HELP.links->insert(title, tok);
                free(title);
            }
            free(ptr);
            HELP.links->insert_default("   ", "");
            HELP.links->update();

            ptr = GBS_find_string(helptext, "TITLE", 0);
            if (!ptr) ptr = helptext;
            ptr = GBS_string_eval(ptr, "{*\\:*}=*2", 0);

            awr->awar(AWAR_HELPTEXT)->write_string(ptr);
            free(ptr);
            free(helptext);
        }
        else {
            char *msg = GBS_global_string_copy("I cannot find the help file '%s'\n\n"
                                               "Please help us to complete the ARB-Help by submitting\n"
                                               "this missing helplink via ARB_NT/File/About/SubmitBug\n"
                                               "Thank you.\n"
                                               "\n"
                                               "Details:\n"
                                               "%s",
                                               help_file, GB_await_error());
            awr->awar(AWAR_HELPTEXT)->write_string(msg);
            free(msg);
        }
    }
    free(help_file);
}

static void aw_help_browse(AW_window *aww) {
    char *help_url = get_local_help_url(aww->get_root());
    if (help_url) {
        AW_openURL(aww->get_root(), help_url);
        free(help_url);
    }
    else {
        aw_message(GBS_global_string("Can't detect URL of help file\n(Reason: %s)", GB_await_error()));
    }
}

static void aw_help_search(AW_window *aww) {
    GB_ERROR  error      = 0;
    char     *searchtext = aww->get_root()->awar(AWAR_HELPSEARCH)->read_string();

    if (searchtext[0]==0) error = "Empty searchstring";
    else {
        char        *helpfilename = 0;
        static char *last_help; // tempfile containing last search result

        // replace all spaces in 'searchtext' by '.*' (also eliminate multi-spaces)
        freeset(searchtext, GBS_string_eval(searchtext, "  = : =.*", 0));

        static GB_HASH *searchHash = GBS_create_dynaval_hash(20, GB_MIND_CASE, GBS_dynaval_free);

        // grep .hlp for occurrences of 'searchtext'.
        // write filenames of matching files into 'helpfilename'
        {
            const char *existingSearch = (const char*)GBS_read_hash(searchHash, searchtext);
            if (existingSearch) {
                helpfilename = strdup(existingSearch);
            }
            else {
                char *helpname = GB_unique_filename("arb", "hlp");
                helpfilename   = GB_create_tempfile(helpname);
                free(helpname);
            }

            if (!helpfilename) error = GB_await_error();
            else {
                char       *quotedSearchExpression = GBK_singlequote(GBS_global_string("^[^#]*%s", searchtext));
                char       *quotedDocpath          = GBK_singlequote(GB_getenvDOCPATH());
                const char *gen_help_tmpl          = "cd %s;grep -i %s `find . -name \"*.hlp\"` | arb_sed -e 'sI:.*IIg' -e 'sI^\\./IIg' | sort | uniq > %s";
                char       *gen_help_cmd           = GBS_global_string_copy(gen_help_tmpl, quotedDocpath, quotedSearchExpression, helpfilename);

                error = GBK_system(gen_help_cmd);

                free(gen_help_cmd);
                free(quotedDocpath);
                free(quotedSearchExpression);
                GB_remove_on_exit(helpfilename);
            }
        }

        if (!error) {
            char *result       = GB_read_file(helpfilename);
            if (!result) error = GB_await_error();
            else {
                // write temporary helpfile containing links to matches as subtopics

                FILE *helpfp       = fopen(helpfilename, "wt");
                if (!helpfp) error = GB_export_IO_error("writing helpfile", helpfilename);
                else {
                    fprintf(helpfp, "\nUP arb.hlp\n");
                    if (last_help) fprintf(helpfp, "UP %s\n", last_help);
                    fputc('\n', helpfp);

                    int   results = 0;
                    char *rp      = result;
                    while (1) {
                        char *eol = strchr(rp, '\n');
                        if (!eol) {
                            eol = rp;
                            while (*eol) ++eol;
                        }
                        if (eol>rp) {
                            char old = eol[0];
                            eol[0] = 0;
                            fprintf(helpfp, "SUB %s\n", rp);
                            results++;
                            eol[0] = old;
                        }
                        if (eol[0]==0) break; // all results inserted
                        rp = eol+1;
                    }

                    fprintf(helpfp, "\nTITLE\t\tResult of search for '%s'\n\n", searchtext);
                    if (results>0)  fprintf(helpfp, "\t\t%i results are shown as subtopics\n",  results);
                    else            fprintf(helpfp, "\t\tThere are no results.\n");

                    if (results>0) freedup(last_help, helpfilename);

                    fclose(helpfp);
                    aww->get_root()->awar(AWAR_HELPFILE)->write_string(helpfilename); // display results in aws
                }
                free(result);
                GBS_write_hash(searchHash, searchtext, (long)strdup(helpfilename));
            }
        }
        free(helpfilename);
    }

    if (error) aw_message(error);

    free(searchtext);
}

void AW_help_popup(AW_window *, const char *help_file) {
    static AW_window_simple *aws = 0;

    AW_root *awr = AW_root::SINGLETON;

    if (!aws) {
        awr->awar_string(AWAR_HELPTEXT,   "", AW_ROOT_DEFAULT);
        awr->awar_string(AWAR_HELPSEARCH, "", AW_ROOT_DEFAULT);
        awr->awar_string(AWAR_HELPFILE,   "", AW_ROOT_DEFAULT);
        awr->awar(AWAR_HELPFILE)->add_callback(aw_help_helpfile_changed_cb);

        aws = new AW_window_simple;
        aws->init(awr, "HELP", "HELP WINDOW");
        aws->load_xfig("help.fig");

        aws->button_length(9);
        aws->auto_space(5, 5);

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(aw_help_back);
        aws->create_button("BACK", "BACK", "B");

        aws->callback(aw_help_history);
        aws->create_button("HISTORY", "HISTORY", "H");

        aws->at("expression");
#if defined(ARB_MOTIF)
        aws->d_callback(makeWindowCallback(aw_help_search)); // enable ENTER in searchfield to start search
#endif
        aws->create_input_field(AWAR_HELPSEARCH, 40);
        aws->callback(aw_help_search);
#if defined(ARB_GTK)
        aws->highlight(); // enable ENTER to start search
#endif
        aws->create_button("SEARCH", "SEARCH", "S");

        aws->at("browse");
        aws->callback(aw_help_browse);
        aws->create_button("BROWSE", "BROWSE", "B");

        aws->callback(aw_help_edit_help);
        aws->create_button("EDIT", "EDIT", "E");

        aws->at("super");
        HELP.uplinks = aws->create_selection_list(AWAR_HELPFILE, false);
        HELP.uplinks->insert_default("   ", "");
        HELP.uplinks->update();

        aws->at("sub");
        HELP.links = aws->create_selection_list(AWAR_HELPFILE, false);
        HELP.links->insert_default("   ", "");
        HELP.links->update();
        HELP.history = 0;

        aws->at("text");
        aws->create_text_field(AWAR_HELPTEXT, 3, 3);

#if defined(TRACK_HELPFILE)
        awr->add_timed_callback(TRACK_FREQUENCY, makeTimedCallback(autorefresh_helpfile));
#endif
    }

    aw_assert(help_file);

    awr->awar(AWAR_HELPFILE)->write_string(help_file);

    if (!GBS_string_matches(help_file, "*.ps", GB_IGNORE_CASE) &&
        !GBS_string_matches(help_file, "*.pdf", GB_IGNORE_CASE))
    { // don't open help if postscript or pdf file
        aws->activate();
    }
}



