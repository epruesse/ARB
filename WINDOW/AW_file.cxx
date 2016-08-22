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
#include "aw_select.hxx"
#include "aw_msg.hxx"
#include "aw_question.hxx"

#include <arbdbt.h>
#include <arb_file.h>
#include <arb_strbuf.h>
#include <arb_misc.h>
#include <arb_str.h>
#include <arb_strarray.h>

#include <sys/stat.h>
#include <dirent.h>
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

void AW_create_fileselection_awars(AW_root *awr, const char *awar_base, const char *directories, const char *filter, const char *file_name) {
    int        base_len     = strlen(awar_base);
    bool       has_slash    = awar_base[base_len-1] == '/';
    char      *awar_name    = new char[base_len+30]; // use private buffer, because caller will most likely use GBS_global_string for arguments
    AW_default default_file = AW_ROOT_DEFAULT;

    sprintf(awar_name, "%s%s", awar_base, "/directory"+int(has_slash));
    AW_awar *awar_dir = awr->awar_string(awar_name, directories, default_file);

    sprintf(awar_name, "%s%s", awar_base, "/filter"   + int(has_slash));
    AW_awar *awar_filter = awr->awar_string(awar_name, filter, default_file);

    sprintf(awar_name, "%s%s", awar_base, "/file_name"+int(has_slash));
    AW_awar *awar_filename = awr->awar_string(awar_name, file_name, default_file);

#if defined(ASSERTION_USED)
    bool is_tmp_awar = strncmp(awar_base, "tmp/", 4) == 0 || strncmp(awar_base, "/tmp/", 5) == 0;
    aw_assert(is_tmp_awar); // you need to use a temp awar for file selections
#endif

    awar_dir->write_string(directories);
    awar_filter->write_string(filter);
    awar_filename->write_string(file_name);

    // create all (default) directories
    {
        ConstStrArray dirs;
        GBT_split_string(dirs, directories, ":", true);
        for (unsigned i = 0; i<dirs.size(); ++i) {
            if (!GB_is_directory(dirs[i])) {
                fprintf(stderr, "Creating directory '%s'\n", dirs[i]);
                GB_ERROR error = GB_create_directory(dirs[i]);
                if (error) aw_message(GBS_global_string("Failed to create directory '%s' (Reason: %s)", dirs[i], error));
            }
        }
    }

    delete [] awar_name;
}

enum DirSortOrder {
    SORT_ALPHA,
    SORT_DATE,
    SORT_SIZE,

    DIR_SORT_ORDERS // order count
};

class LimitedTime {
    double       max_duration;
    time_t       start;
    mutable bool aborted;

public:
    LimitedTime(double max_duration_seconds) : max_duration(max_duration_seconds) { reset(); }
    void reset() {
        time(&start);
        aborted = false;
    }
    double allowed_duration() const { return max_duration; }
    bool finished_in_time() const { return !aborted; }
    bool available() const {
        if (!aborted) {
            time_t now;
            time(&now);
            aborted = difftime(now, start) > max_duration;
        }
        return !aborted;
    }
    void increase() { max_duration *= 2.5; }
};

class File_selection { // @@@ derive from AW_selection?
    AW_root *awr;

    AW_selection_list *filelist;

    char *def_name;
    char *def_dir;
    char *def_filter;

    char *pwd;
    char *pwdx;                                     // additional directories

    DirDisplay dirdisp;

    bool leave_wildcards;
    bool filled_by_wildcard; // last fill done with wildcard?

    bool show_subdirs;  // show or hide subdirs
    bool show_hidden;   // show or hide files/directories starting with '.'

    DirSortOrder sort_order;

    LimitedTime searchTime;

    int shown_name_len;

    void bind_callbacks();
    void execute_browser_command(const char *browser_command);
    void fill_recursive(const char *fulldir, int skipleft, const char *mask, bool recurse, bool showdir);

    void format_columns();

public:

    File_selection(AW_root *aw_root, const char *awar_prefix, const char *pwd_, DirDisplay disp_dirs, bool allow_wildcards)
        : awr(aw_root),
          filelist(NULL),
          pwd(strdup(pwd_)),
          pwdx(NULL),
          dirdisp(disp_dirs),
          leave_wildcards(allow_wildcards),
          filled_by_wildcard(false),
          show_subdirs(true),
          show_hidden(false),
          sort_order(SORT_ALPHA),
          searchTime(1.3)
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
        filelist = aws->create_selection_list(def_name, false);
    }

    void fill();

    void filename_changed(bool post_filter_change_HACK);

    GB_ULONG get_newest_dir_modtime() const {
        ConstStrArray dirs;
        GBT_split_string(dirs, awr->awar(def_dir)->read_char_pntr(), ":", true);
        unsigned long maxtof = 0;
        for (unsigned i = 0; i<dirs.size(); ++i) {
            unsigned long tof = GB_time_of_file(dirs[i]);
            if (tof>maxtof) maxtof = tof;
        }
        return maxtof;
    }

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

void File_selection::execute_browser_command(const char *browser_command) {
    if (strcmp(browser_command, "sort") == 0) {
        sort_order = DirSortOrder((sort_order+1)%DIR_SORT_ORDERS);
    }
    else if (strcmp(browser_command, "hide") == 0) {
        show_subdirs = false;
    }
    else if (strcmp(browser_command, "show") == 0) {
        show_subdirs = true;
    }
    else if (strcmp(browser_command, "dot") == 0) {
        show_hidden = !show_hidden;
    }
    else if (strcmp(browser_command, "inctime") == 0) {
        searchTime.increase();
    }
    else {
        aw_message(GBS_global_string("Unknown browser command '%s'", browser_command));
    }
}

inline int entryType(const char *entry) {
    const char *typechar = "DFL";
    for (int i = 0; typechar[i]; ++i) {
        if (entry[0] == typechar[i]) return i;
    }
    return -1;
}

void File_selection::format_columns() {
    const int FORMATTED_TYPES = 3;

    int maxlen[FORMATTED_TYPES] = { 17, 17, 17 };

    for (int pass = 1; pass<=2; ++pass) {
        AW_selection_list_iterator entry(filelist);
        while (entry) {
            const char *disp = entry.get_displayed();
            int         type = entryType(disp);

            if (type>=0) {
                const char *q1 = strchr(disp, '?');
                if (q1) {
                    const char *q2 = strchr(q1+1, '?');
                    if (q2) {
                        int len = q2-q1-1;
                        if (pass == 1) {
                            if (maxlen[type]<len) maxlen[type] = len;
                        }
                        else {
                            GBS_strstruct buf(200);
                            buf.ncat(disp, q1-disp);
                            buf.ncat(q1+1, len);
                            buf.nput(' ', maxlen[type]-len);
                            buf.cat(q2+1);
                            entry.set_displayed(buf.get_data());
                        }
                    }
                }
            }
            ++entry;
        }
    }
}

void File_selection::fill_recursive(const char *fulldir, int skipleft, const char *mask, bool recurse, bool showdir) {
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
            if (!(entry[0] == '.' && (!show_hidden || entry[1] == 0 || (entry[1] == '.' && entry[2] == 0)))) { // skip "." and ".." and dotdirs if requested
                if (showdir) {
                    filelist->insert(GBS_global_string("D ?%s? (%s)", entry, fullname), fullname); // '?' used in format_columns()
                }
                if (recurse && !AW_is_link(nontruepath)) { // don't follow links
                    if (searchTime.available()) {
                        fill_recursive(nontruepath, skipleft, mask, recurse, showdir);
                    }
                }
            }
        }
        else {
            if (GBS_string_matches(entry, mask, GB_IGNORE_CASE)) { // entry matches mask
                if ((entry[0] != '.' || show_hidden) && AW_is_file(fullname)) { // regular existing file
                    struct stat stt;

                    stat(fullname, &stt);

                    char       atime[256];
                    struct tm *tms = localtime(&stt.st_mtime);
                    strftime(atime, 255, "%Y/%m/%d %k:%M", tms);

                    char *size     = strdup(GBS_readable_size(stt.st_size, "b"));
                    char  typechar = AW_is_link(nontruepath) ? 'L' : 'F';

                    const char *sel_entry = 0;
                    switch (sort_order) {
                        case SORT_ALPHA:
                            sel_entry = GBS_global_string("%c ?%s?  %7s  %s", typechar, nontruepath+skipleft, size, atime); // '?' used in format_columns()
                            break;
                        case SORT_DATE:
                            sel_entry = GBS_global_string("%c %s  %7s  %s", typechar, atime, size, nontruepath+skipleft);
                            break;
                        case SORT_SIZE:
                            sel_entry = GBS_global_string("%c %7s  %s  %s", typechar, size, atime, nontruepath+skipleft);
                            break;
                        case DIR_SORT_ORDERS: break;
                    }

                    filelist->insert(sel_entry, nontruepath);
                    free(size);
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

inline bool fileOrLink(const char *d) { return d[0] == 'F' || d[0] == 'L'; }
inline const char *gotounit(const char *d) {
    ++d;
    while (d[0] == ' ') ++d;
    while (d[0] != ' ') ++d;
    while (d[0] == ' ') ++d;
    return d;
}
static int cmpBySize(const char *disp1, const char *disp2) {
    if (fileOrLink(disp1) && fileOrLink(disp2)) {
        const char *u1 = gotounit(disp1);
        const char *u2 = gotounit(disp2);

        if (u1[0] != u2[0]) { // diff units
            static const char *units = "bkMGTPEZY"; // see also ../CORE/arb_misc.cxx@Tera

            const char *p1 = strchr(units, u1[0]);
            const char *p2 = strchr(units, u2[0]);
            if (p1 != p2) {
                return p1-p2;
            }
        }
    }
    return ARB_stricmp(disp1, disp2);
}

void File_selection::fill() {
    AW_root *aw_root = awr;
    filelist->clear();

    char *filter  = aw_root->awar(def_filter)->read_string();
    char *name    = aw_root->awar(def_name)->read_string();

    const char *name_only = 0;
    {
        char *slash = strrchr(name, '/');
        name_only   = slash ? slash+1 : name;
    }

    StrArray dirs;
    {
        char *diru = aw_root->awar(def_dir)->read_string();
        if (dirdisp == MULTI_DIRS) {
            ConstStrArray cdirs;
            GBT_split_string(cdirs, diru, ":", true);
            for (unsigned i = 0; i<cdirs.size(); ++i) dirs.put(strdup(cdirs[i]));
        }
        else {
            if (name[0] == '/' && AW_is_dir(name)) {
                dirs.put(strdup(name));
                name_only = "";
            }
            else {
                char *fulldir = AW_unfold_path(pwd, diru);
                dirs.put(fulldir);
            }
        }
        free(diru);
    }

    filled_by_wildcard = strchr(name_only, '*');

    if (dirdisp == ANY_DIR) {
        aw_assert(dirs.size() == 1);
        const char *fulldir    = dirs[0];

        DuplicateLinkFilter unDup;
        unDup.register_directory(fulldir);

        if (filled_by_wildcard) {
            if (leave_wildcards) {
                filelist->insert(GBS_global_string("  ALL '%s' in '%s'", name_only, fulldir), name);
            }
            else {
                filelist->insert(GBS_global_string("  ALL '%s' in+below '%s'", name_only, fulldir), name);
            }
        }
        else {
            filelist->insert(GBS_global_string("  CONTENTS OF '%s'", fulldir), fulldir);
            if (filter[0]) {
                filelist->insert(GBS_global_string("!  Find all         (*%s)", filter), "*");
            }
        }

        if (strcmp("/", fulldir)) {
            filelist->insert("! \'PARENT DIR\'      (..)", "..");
        }
        if (show_subdirs) {
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

            filelist->insert("!  Sub-directories  (shown)", GBS_global_string("%s?hide?", name));
        }
        else {
            filelist->insert("!  Sub-directories  (hidden)", GBS_global_string("%s?show?", name));
        }
        filelist->insert(GBS_global_string("!  Hidden           (%s)", show_hidden ? "shown" : "not shown"), GBS_global_string("%s?dot?", name));
    }
    else {
        aw_assert(dirdisp == MULTI_DIRS);
    }

    static const char *order_name[DIR_SORT_ORDERS] = { "alpha", "date", "size" };

    filelist->insert(GBS_global_string("!  Sort order       (%s)", order_name[sort_order]), GBS_global_string("%s?sort?", name));

    bool insert_dirs = dirdisp == ANY_DIR && show_subdirs;

    searchTime.reset(); // limits time spent in fill_recursive
    for (unsigned i = 0; i<dirs.size(); ++i) {
        const char *fulldir = dirs[i];
        if (filled_by_wildcard) {
            if (leave_wildcards) {
                fill_recursive(fulldir, strlen(fulldir)+1, name_only, false, insert_dirs);
            }
            else {
                if (dirdisp == ANY_DIR) { // recursive wildcarded search
                    fill_recursive(fulldir, strlen(fulldir)+1, name_only, true, false);
                }
                else {
                    char *mask = GBS_global_string_copy("%s*%s", name_only, filter);
                    fill_recursive(fulldir, strlen(fulldir)+1, mask, false, false);
                    free(mask);
                }
            }
        }
        else {
            char *mask = GBS_global_string_copy("*%s", filter);

            fill_recursive(fulldir, strlen(fulldir)+1, mask, false, insert_dirs);
            free(mask);
        }
    }

    if (!searchTime.finished_in_time()) {
        filelist->insert(GBS_global_string("!  Find aborted    (after %.1fs; click to search longer)", searchTime.allowed_duration()), GBS_global_string("%s?inctime?", name));
    }

    if (sort_order == SORT_SIZE) {
        filelist->sortCustom(cmpBySize);
    }
    else {
        filelist->sort(false, false);
    }
    format_columns();
    filelist->insert_default("", "");
    filelist->update();

    if (filled_by_wildcard && !leave_wildcards) { // avoid returning wildcarded filename (if !leave_wildcards)
        aw_root->awar(def_name)->write_string("");
    }

    free(name);
    free(filter);
}

void File_selection::filename_changed(bool post_filter_change_HACK) {
    AW_root *aw_root = awr;
    char    *fname   = aw_root->awar(def_name)->read_string();

#if defined(TRACE_FILEBOX)
    printf("fileselection_filename_changed_cb:\n"
           "- fname   ='%s'\n", fname);
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
            trigger_refresh();
        }
        else if (dirdisp != MULTI_DIRS) {
            char *newName = 0;
            char *dir     = aw_root->awar(def_dir)->read_string();

#if defined(TRACE_FILEBOX)
            printf("- dir     ='%s'\n", dir);
#endif // TRACE_FILEBOX

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

            // Allow to select symbolic links for files (i.e. do not automatically switch to link-target;
            // doing so made it impossible to load quicksaves done versus a master DB).
            if (newName && strcmp(fname, newName) != 0) {
                if (!GB_is_directory(fname) && !GB_is_directory(newName)) {
                    if (GB_is_link(fname) ) freenull(newName); // do not follow symlink!
                }
            }

            if (newName) {
#if defined(TRACE_FILEBOX)
                printf("- newName ='%s'\n", newName);
#endif // TRACE_FILEBOX

                if (AW_is_dir(newName)) {
                    aw_root->awar(def_name)->write_string("");
                    aw_assert(dirdisp != MULTI_DIRS); // overwriting content unwanted if displaying MULTI_DIRS
                    aw_root->awar(def_dir)->write_string(newName);
                    aw_root->awar(def_name)->write_string("");
                }
                else {
                    char *lslash = strrchr(newName, '/');
                    if (lslash) {
                        if (lslash == newName) { // root directory
                            aw_assert(dirdisp != MULTI_DIRS); // overwriting content unwanted if displaying MULTI_DIRS
                            aw_root->awar(def_dir)->write_string("/"); // write directory part
                        }
                        else {
                            lslash[0] = 0;
                            aw_assert(dirdisp != MULTI_DIRS); // overwriting content unwanted if displaying MULTI_DIRS
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
                                if (suffix && post_filter_change_HACK) {
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
                }
            }
            free(dir);

            if (strchr(fname, '*')) { // wildcard -> search for suffix
                trigger_refresh();
            }
        }
    }

    free(fname);
}

static bool avoid_multi_refresh = false;

static void fill_fileselection_cb(AW_root*, File_selection *cbs) {
    if (!avoid_multi_refresh) {
        LocallyModify<bool> flag(avoid_multi_refresh, true);
        cbs->fill();
    }
}
static void fileselection_filename_changed_cb(AW_root*, File_selection *cbs) {
    if (!avoid_multi_refresh) {
        LocallyModify<bool> flag(avoid_multi_refresh, true);
        cbs->filename_changed(false);
        cbs->fill();
    }
    else {
        cbs->filename_changed(false);
    }
}
static void fileselection_filter_changed_cb(AW_root*, File_selection *cbs) {
    if (!avoid_multi_refresh) {
        LocallyModify<bool> flag(avoid_multi_refresh, true);
        cbs->filename_changed(true);
        cbs->fill();
    }
    else {
        cbs->filename_changed(true);
    }
}

void File_selection::bind_callbacks() {
    awr->awar(def_name)  ->add_callback(makeRootCallback(fileselection_filename_changed_cb, this));
    awr->awar(def_dir)   ->add_callback(makeRootCallback(fill_fileselection_cb, this));
    awr->awar(def_filter)->add_callback(makeRootCallback(fileselection_filter_changed_cb, this));
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
        GB_ULONG mtime = check->acbs->get_newest_dir_modtime();
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
    install->modtime = acbs->get_newest_dir_modtime();

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
     * if disp_dirs == ANY_DIR, then show directories and files
     * if disp_dirs == MULTI_DIRS, then only show files, but from multiple directories
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
        TEST_EXPECT_EQUAL(GB_getenv("ARBHOME"), GB_getenvARBHOME());
        TEST_EXPECT_NULL(GB_getenv("ARB_NONEXISTING_ENVAR"));

        GB_install_getenv_hook(old);
    }

    TEST_EXPECT_EQUAL_DUPPED(AW_unfold_path("PWD", "/bin"),                strdup("/bin"));
    TEST_EXPECT_EQUAL_DUPPED(AW_unfold_path("PWD", "../tests"),            strdup(GB_path_in_ARBHOME("UNIT_TESTER/tests")));
    TEST_EXPECT_EQUAL_DUPPED(AW_unfold_path("ARB_NONEXISTING_ENVAR", "."), strdup(currDir));
}

#endif // UNIT_TESTS


