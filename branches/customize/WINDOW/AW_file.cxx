// ================================================================ //
//                                                                  //
//   File      : AW_file.cxx                                        //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "aw_file.hxx"
#include "aw_awar.hxx"
#include "aw_root.hxx"
#include <arbdbt.h>
#include <arb_file.h>
#include "aw_select.hxx"
#include <arb_strbuf.h>
#include "aw_msg.hxx"
#include "aw_question.hxx"
#include <dirent.h>
#include <sys/stat.h>
#include <set>
#include <string>

using std::set;
using std::string;

#if defined(DEBUG)
// #define TRACE_FILEBOX
#endif // DEBUG

static GB_CSTR expand_symbolic_directories(const char *pwd_envar) { 
    GB_CSTR res;

    if (strcmp(pwd_envar, "PWD") == 0) {
        res = GB_getcwd();
    }
    else if (strcmp(pwd_envar, "PT_SERVER_HOME") == 0) {
        res = GB_path_in_ARBLIB("pts");
    }
    else {
        res = NULL;
    }

    return res;
}


char *AW_unfold_path(const char *pwd_envar, const char *path) {
    //! create a full path
    gb_getenv_hook  oldHook = GB_install_getenv_hook(expand_symbolic_directories);
    char           *result  = nulldup(GB_unfold_path(pwd_envar, path));
    GB_install_getenv_hook(oldHook);
    return result;
}

char *AW_extract_directory(const char *path) {
    const char *lslash = strrchr(path, '/');
    if (!lslash) return 0;

    char *result        = strdup(path);
    result[lslash-path] = 0;

    return result;
}

// -----------------------------
//      file selection boxes

void AW_create_fileselection_awars(AW_root *awr, const char *awar_base,
                                   const char *directory, const char *filter, const char *file_name,
                                   AW_default default_file, bool resetValues)
{
    int   base_len  = strlen(awar_base);
    bool  has_slash = awar_base[base_len-1] == '/';
    char *awar_name = new char[base_len+30]; // use private buffer, because caller will most likely use GBS_global_string for arguments

    sprintf(awar_name, "%s%s", awar_base, "/directory"+int(has_slash));
    AW_awar *awar_dir = awr->awar_string(awar_name, directory, default_file);

    sprintf(awar_name, "%s%s", awar_base, "/filter"   + int(has_slash));
    AW_awar *awar_filter = awr->awar_string(awar_name, filter, default_file);

    sprintf(awar_name, "%s%s", awar_base, "/file_name"+int(has_slash));
    AW_awar *awar_filename = awr->awar_string(awar_name, file_name, default_file);

    if (resetValues) {
        awar_dir->write_string(directory);
        awar_filter->write_string(filter);
        awar_filename->write_string(file_name);
    }
    else {
        char *stored_directory = awar_dir->read_string();
#if defined(DEBUG)
        if (strncmp(awar_base, "tmp/", 4) == 0) { // non-saved awar
            if (directory[0] != 0) { // accept empty dir (means : use current ? )
                aw_assert(GB_is_directory(directory)); // default directory does not exist
            }
        }
#endif // DEBUG

        if (strcmp(stored_directory, directory) != 0) { // does not have default value
#if defined(DEBUG)
            const char *arbhome    = GB_getenvARBHOME();
            int         arbhomelen = strlen(arbhome);

            if (strncmp(directory, arbhome, arbhomelen) == 0) { // default points into $ARBHOME
                aw_assert(resetValues); // should be called with resetValues == true
                // otherwise it's possible, that locations from previously installed ARB versions are used
            }
#endif // DEBUG

            if (!GB_is_directory(stored_directory)) {
                awar_dir->write_string(directory);
                fprintf(stderr,
                        "Warning: Replaced reference to non-existing directory '%s'\n"
                        "         by '%s'\n"
                        "         (Save properties to make this change permanent)\n",
                        stored_directory, directory);
            }
        }

        free(stored_directory);
    }

    char *dir = awar_dir->read_string();
    if (dir[0] && !GB_is_directory(dir)) {
        if (aw_ask_sure("create_directory", GBS_global_string("Directory '%s' does not exist. Create?", dir))) {
            GB_ERROR error = GB_create_directory(dir);
            if (error) aw_message(GBS_global_string("Failed to create directory '%s' (Reason: %s)", dir, error));
        }
    }
    free(dir);

    delete [] awar_name;
}

#if defined(WARN_TODO)
#warning derive File_selection from AW_selection
#endif

class File_selection {
    AW_root *awr;

    AW_selection_list *filelist;

    char *def_name;
    char *def_dir;
    char *def_filter;

    char *pwd;
    char *pwdx;                                     // additional directories

    char *previous_filename;

    DirDisplay dirdisp;

    bool leave_wildcards;

    void bind_callbacks();

public:

    File_selection(AW_root *aw_root, const char *awar_prefix, const char *pwd_, DirDisplay disp_dirs, bool allow_wildcards)
        : awr(aw_root),
          filelist(NULL),
          pwd(strdup(pwd_)),
          pwdx(NULL),
          previous_filename(NULL),
          dirdisp(disp_dirs),
          leave_wildcards(allow_wildcards)
    {
        {
            char *multiple_dirs_in_pwd = strchr(pwd, '^');
            if (multiple_dirs_in_pwd) {
                multiple_dirs_in_pwd[0] = 0;
                pwdx = multiple_dirs_in_pwd+1;
            }
        }

        def_name   = GBS_string_eval(awar_prefix, "*=*/file_name", 0);
        def_dir    = GBS_string_eval(awar_prefix, "*=*/directory", 0);
        def_filter = GBS_string_eval(awar_prefix, "*=*/filter", 0);

        aw_assert(!GB_have_error());

        bind_callbacks();
    }

    void create_gui_elements(AW_window *aws, const char *at_prefix) {
        aw_assert(!filelist);

        char buffer[1024];
        sprintf(buffer, "%sfilter", at_prefix);
        if (aws->at_ifdef(buffer)) {
            aws->at(buffer);
            aws->create_input_field(def_filter, 5);
        }

        sprintf(buffer, "%sfile_name", at_prefix);
        if (aws->at_ifdef(buffer)) {
            aws->at(buffer);
            aws->create_input_field(def_name, 20);
        }

        sprintf(buffer, "%sbox", at_prefix);
        aws->at(buffer);
        filelist = aws->create_selection_list(def_name, 2, 2);
    }

    void fill();

    void filename_changed();

    const char *get_dir() const { return awr->awar(def_dir)->read_char_pntr(); }
    GB_ULONG get_dir_modtime() const { return GB_time_of_file(get_dir()); }

    void trigger_refresh() { awr->awar(def_dir)->touch(); }
};

static GB_CSTR get_suffix(GB_CSTR fullpath) { // returns pointer behind '.' of suffix (or NULL if no suffix found)
    GB_CSTR dot = strrchr(fullpath, '.');
    if (!dot) return 0;

    GB_CSTR lslash = strrchr(fullpath, '/');
    if (lslash && lslash>dot) return 0; // no . behind last /
    return dot+1;
}


static char *set_suffix(const char *name, const char *suffix) {
    // returns "name.suffix" (name may contain path information)
    // - eliminates multiple dots
    // - sets name to 'noname' if no name part is given

    char *path, *fullname;
    GB_split_full_path(name, &path, &fullname, NULL, NULL);

    // remove dots and spaces from suffix:
    while (suffix[0] == '.' || suffix[0] == ' ') ++suffix;
    if (!suffix[0]) suffix = 0;

    GBS_strstruct *out = GBS_stropen(FILENAME_MAX+1);
    if (path) {
        GBS_strcat(out, path);
        GBS_chrcat(out, '/');
    }

    if (fullname) GBS_strcat(out, fullname);

    if (GB_is_directory(GBS_mempntr(out))) {
        // if 'out' contains a directory now, 'name' was lacking a filename
        // (in this case it was only a directory)
        GBS_strcat(out, "/noname"); // invent a name
    }

    if (suffix) {
        GBS_chrcat(out, '.');
        GBS_strcat(out, suffix);
    }

    free(path);
    free(fullname);

    return GBS_strclose(out);
}


inline const char *valid_path(const char *path) { return path[0] ? path : "."; }

inline bool AW_is_dir(const char *path) { return GB_is_directory(valid_path(path)); }
inline bool AW_is_file(const char *path) { return GB_is_regularfile(valid_path(path)); }
inline bool AW_is_link(const char *path) { return GB_is_link(valid_path(path)); }


#define DIR_SORT_ORDERS 3
static const char *DIR_sort_order_name[DIR_SORT_ORDERS] = { "alpha", "date", "size" };
static int DIR_sort_order     = 0; // 0 = alpha; 1 = date; 2 = size;
static int DIR_subdirs_hidden = 0; // 1 -> hide sub-directories (by user-request)
static int DIR_show_hidden    = 0; // 1 -> show hidden (i.e. files/directories starting with '.')

static void execute_browser_command(const char *browser_command) {
    if (strcmp(browser_command, "sort") == 0) {
        DIR_sort_order = (DIR_sort_order+1)%DIR_SORT_ORDERS;
    }
    else if (strcmp(browser_command, "hide") == 0) {
        DIR_subdirs_hidden = 1;
    }
    else if (strcmp(browser_command, "show") == 0) {
        DIR_subdirs_hidden = 0;
    }
    else if (strcmp(browser_command, "dot") == 0) {
        DIR_show_hidden ^= 1;
    }
    else {
        aw_message(GBS_global_string("Unknown browser command '%s'", browser_command));
    }
}

static void fill_fileselection_recursive(const char *fulldir, int skipleft, const char *mask, bool recurse, bool showdir, bool show_dots, AW_selection_list *filelist) {
    // see fill_fileselection_cb for meaning of 'sort_order'

    DIR *dirp = opendir(fulldir);

#if defined(TRACE_FILEBOX)
    printf("fill_fileselection_recursive for directory '%s'\n", fulldir);
#endif // TRACE_FILEBOX

    if (!dirp) {
        filelist->insert(GBS_global_string("x Your directory path is invalid (%s)", fulldir), "?");
        return;
    }

    struct dirent *dp;
    for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
        const char *entry       = dp->d_name;
        char       *nontruepath = GBS_global_string_copy("%s/%s", fulldir, entry);
        char       *fullname;

        if (strlen(fulldir)) fullname = strdup(GB_concat_full_path(fulldir, entry));
        else fullname                 = strdup(GB_canonical_path(entry));

        if (AW_is_dir(fullname)) {
            if (!(entry[0] == '.' && (!DIR_show_hidden || entry[1] == 0 || (entry[1] == '.' && entry[2] == 0)))) { // skip "." and ".." and dotdirs if requested
                if (showdir) {
                    filelist->insert(GBS_global_string("D %-18s(%s)", entry, fullname), fullname);
                }
                if (recurse && !AW_is_link(nontruepath)) { // don't follow links
                    fill_fileselection_recursive(nontruepath, skipleft, mask, recurse, showdir, show_dots, filelist);
                }
            }
        }
        else {
            if (GBS_string_matches(entry, mask, GB_IGNORE_CASE)) { // entry matches mask
                if ((entry[0] != '.' || DIR_show_hidden) && AW_is_file(fullname)) { // regular existing file
                    struct stat stt;

                    stat(fullname, &stt);

                    char       atime[256];
                    struct tm *tms = localtime(&stt.st_mtime);
                    strftime(atime, 255, "%Y/%m/%d %k:%M", tms);

                    long ksize    = (stt.st_size+512)/1024;
                    char typechar = AW_is_link(nontruepath) ? 'L' : 'F';

                    const char *sel_entry = 0;
                    switch (DIR_sort_order) {
                        case 0: // alpha
                            sel_entry = GBS_global_string("%c %-30s  %6lik  %s", typechar, nontruepath+skipleft, ksize, atime);
                            break;
                        case 1: // date
                            sel_entry = GBS_global_string("%c %s  %6lik  %s", typechar, atime, ksize, nontruepath+skipleft);
                            break;
                        case 2: // size
                            sel_entry = GBS_global_string("%c %6lik  %s  %s", typechar, ksize, atime, nontruepath+skipleft);
                            break;
                    }

                    filelist->insert(sel_entry, nontruepath);
                }
            }
        }
        free(fullname);
        free(nontruepath);
    }

    closedir(dirp);
}

class DuplicateLinkFilter {
    set<string> insertedDirectories;

public:
    DuplicateLinkFilter() {}

    bool not_seen_yet(const string& dir) const { return insertedDirectories.find(dir) == insertedDirectories.end(); }
    void register_directory(const string& dir) {
        insertedDirectories.insert(dir);
    }
};


static void show_soft_link(AW_selection_list *filelist, const char *envar, DuplicateLinkFilter& unDup) {
    // adds a soft link (e.g. ARBMACROHOME or ARB_WORKDIR) into file selection box
    // if content of 'envar' matches 'cwd' nothing is inserted

    const char *expanded_dir        = expand_symbolic_directories(envar);
    if (!expanded_dir) expanded_dir = GB_getenv(envar);

    if (expanded_dir) {
        string edir(expanded_dir);

        if (unDup.not_seen_yet(edir)) {
            unDup.register_directory(edir);
            const char *entry = GBS_global_string("$ %-18s(%s)", GBS_global_string("'%s'", envar), expanded_dir);
            filelist->insert(entry, expanded_dir);
        }
    }
}

void File_selection::fill() {
    AW_root *aw_root = awr;
    filelist->clear();

    char *diru    = aw_root->awar(def_dir)->read_string();
    char *fulldir = AW_unfold_path(pwd, diru);
    char *filter  = aw_root->awar(def_filter)->read_string();
    char *name    = aw_root->awar(def_name)->read_string();

#if defined(TRACE_FILEBOX)
    printf("fill_fileselection_cb:\n"
           "- diru   ='%s'\n"
           "- fulldir='%s'\n"
           "- filter ='%s'\n"
           "- name   ='%s'\n",  diru, fulldir, filter, name);
#endif // TRACE_FILEBOX

    const char *name_only = 0;
    {
        char *slash = strrchr(name, '/');
        name_only   = slash ? slash+1 : name;
    }

    if (name[0] == '/' && AW_is_dir(name)) {
        freedup(fulldir, name);
        name_only = "";
    }

    DuplicateLinkFilter  unDup;
    unDup.register_directory(fulldir);

    bool is_wildcard = strchr(name_only, '*');

    if (dirdisp == ANY_DIR) {
        if (is_wildcard) {
            if (leave_wildcards) {
                filelist->insert((char *)GBS_global_string("  ALL '%s' in '%s'", name_only, fulldir), name);
            }
            else {
                filelist->insert((char *)GBS_global_string("  ALL '%s' in+below '%s'", name_only, fulldir), name);
            }
        }
        else {
            filelist->insert((char *)GBS_global_string("  CONTENTS OF '%s'", fulldir), fulldir);
        }

        if (filter[0] && !is_wildcard) {
            filelist->insert(GBS_global_string("! \' Search for\'     (*%s)", filter), "*");
        }
        if (strcmp("/", fulldir)) {
            filelist->insert("! \'PARENT DIR       (..)\'", "..");
        }
        if (DIR_subdirs_hidden == 0) {
            show_soft_link(filelist, pwd, unDup);

            if (pwdx) {        // additional directories
                char *start = pwdx;
                while (start) {
                    char *multiple = strchr(start, '^');
                    if (multiple) {
                        multiple[0] = 0;
                        show_soft_link(filelist, start, unDup);
                        multiple[0] = '^';
                        start       = multiple+1;
                    }
                    else {
                        show_soft_link(filelist, start, unDup);
                        start = 0;
                    }
                }
            }

            show_soft_link(filelist, "HOME", unDup);
            show_soft_link(filelist, "PWD", unDup);
            show_soft_link(filelist, "ARB_WORKDIR", unDup);
            show_soft_link(filelist, "PT_SERVER_HOME", unDup);

            filelist->insert("! \' Sub-directories (shown)\'", GBS_global_string("%s?hide?", name));
        }
        else {
            filelist->insert("! \' Sub-directories (hidden)\'", GBS_global_string("%s?show?", name));
        }
    }

    filelist->insert(GBS_global_string("! \' Sort order\'     (%s)", DIR_sort_order_name[DIR_sort_order]),
                     GBS_global_string("%s?sort?", name));

    filelist->insert(GBS_global_string("! \' %s%s\'",
                                       DIR_show_hidden ? "Hide dot-" : "Show hidden ",
                                       dirdisp == ANY_DIR ? "files/dirs" : "files"),
                     GBS_global_string("%s?dot?", name));

    bool insert_dirs = dirdisp == ANY_DIR && !DIR_subdirs_hidden;
    if (is_wildcard) {
        if (leave_wildcards) {
            fill_fileselection_recursive(fulldir, strlen(fulldir)+1, name_only, false, insert_dirs, DIR_show_hidden, filelist);
        }
        else {
            if (dirdisp == ANY_DIR) { // recursive wildcarded search
                fill_fileselection_recursive(fulldir, strlen(fulldir)+1, name_only, true, false, DIR_show_hidden, filelist);
            }
            else {
                char *mask = GBS_global_string_copy("%s*%s", name_only, filter);
                fill_fileselection_recursive(fulldir, strlen(fulldir)+1, mask, false, false, DIR_show_hidden, filelist);
                free(mask);
            }
        }
    }
    else {
        char *mask = GBS_global_string_copy("*%s", filter);

        fill_fileselection_recursive(fulldir, strlen(fulldir)+1, mask, false, insert_dirs, DIR_show_hidden, filelist);
        free(mask);
    }

    filelist->insert_default("", "");
    filelist->sort(false, true);
    filelist->update();

    free(name);
    free(fulldir);
    free(diru);
    free(filter);
}

static bool filter_has_changed = false; // @@@ move into File_selection

void File_selection::filename_changed() {
    AW_root *aw_root = awr;
    char    *fname   = aw_root->awar(def_name)->read_string();

#if defined(TRACE_FILEBOX)
    printf("fileselection_filename_changed_cb:\n"
           "- fname='%s'\n", fname);
    printf("- previous_filename='%s'\n", previous_filename);
#endif // TRACE_FILEBOX

    if (fname[0]) {
        char *browser_command = 0;
        {
            // internal browser commands (e.g. '?sort?') are simply appended to the filename

            char *lquestion = strrchr(fname, '?');
            if (lquestion) {
                lquestion[0] = 0; // remove last '?' + everything behind
                lquestion    = strrchr(fname, '?');
                if (lquestion) {
                    browser_command = lquestion+1;
                    lquestion[0]    = 0; // completely remove the browser command
                }
            }
        }
        if (browser_command) {
            aw_root->awar(def_name)->write_string(fname); // re-write w/o browser_command
            execute_browser_command(browser_command);
            aw_root->awar(def_dir)->touch(); // force reinit
        }

        char *newName = 0;
        char *dir     = aw_root->awar(def_dir)->read_string();

        if (fname[0] == '/' || fname[0] == '~') {
            newName = strdup(GB_canonical_path(fname));
        }
        else {
            if (dir[0]) {
                if (dir[0] == '/') {
                    newName = strdup(GB_concat_full_path(dir, fname));
                }
                else {
                    char *fulldir = 0;

                    if (dir[0] == '.') fulldir = AW_unfold_path(pwd, dir);
                    else fulldir               = strdup(dir);

                    newName = strdup(GB_concat_full_path(fulldir, fname));
                    free(fulldir);
                }
            }
            else {
                newName = AW_unfold_path(pwd, fname);
            }
        }

        if (newName) {
            if (AW_is_dir(newName)) {
                aw_root->awar(def_name)->write_string("");
                aw_root->awar(def_dir)->write_string(newName);
                if (previous_filename) {
                    const char *slash              = strrchr(previous_filename, '/');
                    const char *name               = slash ? slash+1 : previous_filename;
                    const char *with_previous_name = GB_concat_full_path(newName, name);

                    if (!AW_is_dir(with_previous_name)) { // write as new name if not a directory
                        aw_root->awar(def_name)->write_string(with_previous_name);
                    }
                    else {
                        freenull(previous_filename);
                        aw_root->awar(def_name)->write_string(newName);
                    }

                    freeset(newName, aw_root->awar(def_name)->read_string());
                }
                else {
                    aw_root->awar(def_name)->write_string("");
                }
            }
            else {
                char *lslash = strrchr(newName, '/');
                if (lslash) {
                    if (lslash == newName) { // root directory
                        aw_root->awar(def_dir)->write_string("/"); // write directory part
                    }
                    else {
                        lslash[0] = 0;
                        aw_root->awar(def_dir)->write_string(newName); // write directory part
                        lslash[0] = '/';
                    }
                }

                // now check the correct suffix :
                {
                    char *filter = aw_root->awar(def_filter)->read_string();
                    if (filter[0]) {
                        char *pfilter = strrchr(filter, '.');
                        pfilter       = pfilter ? pfilter+1 : filter;

                        char *suffix = (char*)get_suffix(newName); // cast ok, since get_suffix points into newName

                        if (!suffix || strcmp(suffix, pfilter) != 0) {
                            if (suffix && filter_has_changed) {
                                if (suffix[-1] == '.') suffix[-1] = 0;
                            }
                            freeset(newName, set_suffix(newName, pfilter));
                        }
                    }
                    free(filter);
                }

                if (strcmp(newName, fname) != 0) {
                    aw_root->awar(def_name)->write_string(newName); // loops back if changed !!!
                }

                freeset(previous_filename, newName);
            }
        }
        free(dir);

        if (strchr(fname, '*')) { // wildcard -> search for suffix
            aw_root->awar(def_dir)->touch(); // force reinit
        }
    }

    filter_has_changed = false;

    free(fname);
}

static void fill_fileselection_cb(AW_root*, File_selection *cbs) {
    cbs->fill();
}
static void fileselection_filename_changed_cb(AW_root*, File_selection *cbs) {
    cbs->filename_changed();
}
static void fileselection_filter_changed_cb(AW_root*) {
    filter_has_changed = true;
#if defined(TRACE_FILEBOX)
    printf("fileselection_filter_changed_cb: marked as changed\n");
#endif // TRACE_FILEBOX
}

void File_selection::bind_callbacks() {
    awr->awar(def_name)  ->add_callback(makeRootCallback(fileselection_filename_changed_cb, this));
    awr->awar(def_filter)->add_callback(makeRootCallback(fileselection_filename_changed_cb, this));

    awr->awar(def_dir)   ->add_callback(makeRootCallback(fill_fileselection_cb, this));
    awr->awar(def_filter)->add_callback(makeRootCallback(fill_fileselection_cb, this));

    awr->awar(def_filter)->add_callback(fileselection_filter_changed_cb);
}

#define SELBOX_AUTOREFRESH_FREQUENCY 3000 // refresh every XXX ms

struct selbox_autorefresh_info {
    unsigned long            modtime;
    File_selection          *acbs;
    selbox_autorefresh_info *next;
};
static selbox_autorefresh_info *autorefresh_info = 0;

static unsigned autorefresh_selboxes(AW_root *) {
    selbox_autorefresh_info *check = autorefresh_info;

    while (check) {
        GB_ULONG mtime = check->acbs->get_dir_modtime();
        if (mtime != check->modtime) {
            check->modtime = mtime;
            check->acbs->trigger_refresh();
        }
        check = check->next;
    }

    // refresh again and again and again..
    return SELBOX_AUTOREFRESH_FREQUENCY;
}

static void selbox_install_autorefresh(AW_root *aw_root, File_selection *acbs) {
    if (!autorefresh_info) {    // when installing first selbox
        aw_root->add_timed_callback(SELBOX_AUTOREFRESH_FREQUENCY, makeTimedCallback(autorefresh_selboxes));
    }

    selbox_autorefresh_info *install = new selbox_autorefresh_info;

    install->acbs    = acbs;
    install->modtime = acbs->get_dir_modtime();

    install->next    = autorefresh_info;
    autorefresh_info = install;
}

void AW_create_fileselection(AW_window *aws, const char *awar_prefix, const char *at_prefix, const char *pwd, DirDisplay disp_dirs, bool allow_wildcards) {
    /*! Create a file selection box, this box needs 3 AWARS:
     *
     * 1. "$awar_prefix/filter"
     * 2. "$awar_prefix/directory"
     * 3. "$awar_prefix/file_name"
     *
     * (Note: The function AW_create_fileselection_awars() can be used to create them)
     *
     * the "$awar_prefix/file_name" contains the full filename
     * Use AW_get_selected_fullname() to read it.
     * 
     * The items are placed at
     * 
     * 1. "$at_prefix""filter"
     * 2. "$at_prefix""box"
     * 3. "$at_prefix""file_name"
     * 
     * if show_dir== true, then show directories and files
     * else only files
     * 
     * pwd is the name of a 'shell environment variable' which indicates the base directory
     * (e.g. 'PWD' or 'ARBHOME')
     */

    AW_root        *aw_root = aws->get_root();
    File_selection *acbs    = new File_selection(aw_root, awar_prefix, pwd, disp_dirs, allow_wildcards);

    acbs->create_gui_elements(aws, at_prefix);

    fill_fileselection_cb(0, acbs);
    fileselection_filename_changed_cb(0, acbs);    // this fixes the path name

    selbox_install_autorefresh(aw_root, acbs);
}

char *AW_get_selected_fullname(AW_root *awr, const char *awar_prefix) { // @@@ add flag to select whether wildcards are allowed
    char *file = awr->awar(GBS_global_string("%s/file_name", awar_prefix))->read_string();
    if (file[0] != '/') {
        // if name w/o directory was entered by hand (or by default) then append the directory :

        char    *awar_dir_name = GBS_global_string_copy("%s/directory", awar_prefix);
        AW_awar *awar_dir      = awr->awar_no_error(awar_dir_name);

        if (!awar_dir) {
            // file selection box was not active (happens e.g. for print tree)
            awar_dir = awr->awar_string(awar_dir_name, GB_getcwd());
        }

        aw_assert(awar_dir);

        char *dir = awar_dir->read_string();
        if (!dir[0]) {          // empty -> fillin current dir
            awar_dir->write_string(GB_getcwd());
            freeset(dir, awar_dir->read_string());
        }

        char *full = strdup(GB_concat_full_path(dir, file));

        free(dir);
        free(file);

        file = full;

        free(awar_dir_name);
    }

    return file;
}

void AW_set_selected_fullname(AW_root *awr, const char *awar_prefix, const char *to_fullname) {
    awr->awar(GBS_global_string("%s/file_name", awar_prefix))->write_string(to_fullname);
}

void AW_refresh_fileselection(AW_root *awr, const char *awar_prefix) {
    // call optionally to force instant refresh
    // (automatic refresh is done every SELBOX_AUTOREFRESH_FREQUENCY)

    awr->awar(GBS_global_string("%s/directory", awar_prefix))->touch();
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>

#define TEST_EXPECT_EQUAL_DUPPED(cs1, cs2)                              \
    do {                                                                \
        char *s1, *s2;                                                  \
        TEST_EXPECT_EQUAL(s1 = (cs1), s2 = (cs2));                      \
        free(s1);                                                       \
        free(s2);                                                       \
    } while(0)

void TEST_path_unfolding() {
    const char *currDir = GB_getcwd();
    {
        gb_getenv_hook old = GB_install_getenv_hook(expand_symbolic_directories);

        TEST_EXPECT_EQUAL(GB_getenv("PWD"), currDir);
        TEST_EXPECT_EQUAL_DUPPED(strdup(GB_getenv("PT_SERVER_HOME")), strdup(GB_path_in_ARBLIB("pts"))); // need to dup here - otherwise temp buffers get overwritten
        TEST_EXPECT_EQUAL(GB_getenv("ARBHOME"), GB_getenvARBHOME());
        TEST_EXPECT_NULL(GB_getenv("ARB_NONEXISTING_ENVAR"));

        GB_install_getenv_hook(old);
    }

    TEST_EXPECT_EQUAL_DUPPED(AW_unfold_path("PWD", "/bin"),                strdup("/bin"));
    TEST_EXPECT_EQUAL_DUPPED(AW_unfold_path("PWD", "../tests"),            strdup(GB_path_in_ARBHOME("UNIT_TESTER/tests")));
    {
        // test fails if
        char *pts_path_in_arblib = strdup(GB_path_in_ARBLIB("pts"));
        char *pts_unfold_path    = AW_unfold_path("PT_SERVER_HOME", ".");
        if (GB_is_directory(pts_path_in_arblib)) {
            TEST_EXPECT_EQUAL(pts_path_in_arblib, pts_unfold_path);
        }
        else {
            TEST_EXPECT_EQUAL__BROKEN(pts_path_in_arblib, pts_unfold_path, "???"); // fails if directory does not exist
        }
        free(pts_unfold_path);
        free(pts_path_in_arblib);
    }
    TEST_EXPECT_EQUAL_DUPPED(AW_unfold_path("ARB_NONEXISTING_ENVAR", "."), strdup(currDir));
}

#endif // UNIT_TESTS


