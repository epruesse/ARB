#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
// #include <malloc.h>
#include <memory.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include "awt.hxx"
#include "awtlocal.hxx"

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>

#include <string>
#include <set>

#if defined(DEBUG)
// #define TRACE_FILEBOX
#endif // DEBUG

using namespace std;

static GB_CSTR awt_get_base_directory(const char *pwd_envar) {
    GB_CSTR res;

    if (strcmp(pwd_envar, "PWD") == 0) {
        res = GB_getcwd();
    }
    else if (strcmp(pwd_envar, "PT_SERVER_HOME") == 0) {
        res = GB_path_in_ARBLIB("pts", NULL);
    }
    else {
        res = GB_getenv(pwd_envar);
        if (!res) res = GB_getcwd(); // fallback to current working dir
    }

    return res;
}



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


char *AWT_fold_path(char *path, const char *pwd_envar) {
    char    *unfolded = AWT_unfold_path(path, pwd_envar);
    GB_CSTR  prefix   = awt_get_base_directory(pwd_envar);
    int      len      = strlen(prefix);

    if (strncmp(unfolded, prefix, len) == 0) { // unfolded starts with pwd
        if (unfolded[len] == '/') {
            strcpy(unfolded, unfolded+len+1);
        }
        else if (unfolded[len] == 0) { // unfolded is equal to pwd
            unfolded[0] = 0;
        }
    }

    return unfolded;
}

/* create a full path */

char *AWT_unfold_path(const char *path, const char *pwd_envar) {
    if (path[0] == '/' || path[0] == '~') return strdup(GB_get_full_path(path));
    return strdup(GB_concat_full_path(awt_get_base_directory(pwd_envar), path));
}

const char *AWT_valid_path(const char *path) {
    if (strlen(path)) return path;
    return ".";
}

int AWT_is_dir(const char *path) { // Warning : returns 1 for symbolic links to directories
    struct stat stt;
    if (stat(AWT_valid_path(path), &stt)) return 0;
    return !!S_ISDIR(stt.st_mode);
}
int AWT_is_file(const char *path) { // Warning : returns 1 for symbolic links to files
    struct stat stt;
    if (stat(AWT_valid_path(path), &stt)) return 0;
    return !!S_ISREG(stt.st_mode);
}
int AWT_is_link(const char *path) {
    struct stat stt;
    if (lstat(AWT_valid_path(path), &stt)) return 0;
    return !!S_ISLNK(stt.st_mode);
}

char *AWT_extract_directory(const char *path) {
    char *lslash = strrchr(path, '/');
    if (!lslash) return 0;

    char *result        = strdup(path);
    result[lslash-path] = 0;

    return result;
}

#define DIR_SORT_ORDERS 3
static const char *DIR_sort_order_name[DIR_SORT_ORDERS] = { "alpha", "date", "size" };
static int DIR_sort_order     = 0; // 0 = alpha; 1 = date; 2 = size;
static int DIR_subdirs_hidden = 0; // 1 -> hide sub-directories (by user-request)
static int DIR_show_hidden    = 0; // 1 -> show hidden (i.e. files/directories starting with '.')

static void awt_execute_browser_command(const char *browser_command) {
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

static void awt_fill_selection_box_recursive(const char *fulldir, int skipleft, const char *mask, bool recurse, bool showdir, bool show_dots, AW_window *aws, AW_selection_list *selid) {
    // see awt_create_selection_box_cb for meaning of 'sort_order'

    DIR *dirp = opendir(fulldir);

#if defined(TRACE_FILEBOX)
    printf("awt_fill_selection_box_recursive for directory '%s'\n", fulldir);
#endif // TRACE_FILEBOX

    if (!dirp) {
        aws->insert_selection(selid, GBS_global_string("x Your directory path is invalid (%s)", fulldir), "?");
        return;
    }

    struct dirent *dp;
    for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
        const char *entry       = dp->d_name;
        char       *nontruepath = GBS_global_string_copy("%s/%s", fulldir, entry);
        char       *fullname;

        if (strlen(fulldir)) fullname = strdup(GB_concat_full_path(fulldir, entry));
        else fullname                 = strdup(GB_get_full_path(entry));

        if (AWT_is_dir(fullname)) {
            if (!(entry[0] == '.' && (!DIR_show_hidden || entry[1] == 0 || (entry[1] == '.' && entry[2] == 0)))) { // skip "." and ".." and dotdirs if requested
                if (showdir) {
                    aws->insert_selection(selid, GBS_global_string("D %-18s(%s)", entry, fullname), fullname);
                }
                if (recurse && !AWT_is_link(nontruepath)) { // don't follow links
                    awt_fill_selection_box_recursive(nontruepath, skipleft, mask, recurse, showdir, show_dots, aws, selid);
                }
            }
        }
        else {
            if (GBS_string_matches(entry, mask, GB_IGNORE_CASE)) { // entry matches mask
                if ((entry[0] != '.' || DIR_show_hidden) && AWT_is_file(fullname)) { // regular existing file
                    struct stat stt;

                    stat(fullname, &stt);

                    char       atime[256];
                    struct tm *tms = localtime(&stt.st_mtime);
                    strftime( atime, 255,"%Y/%m/%d %k:%M",tms);

                    long ksize    = (stt.st_size+512)/1024;
                    char typechar = AWT_is_link(nontruepath) ? 'L' : 'F';

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

                    aws->insert_selection(selid, sel_entry, nontruepath);
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
        // printf("register_directory '%s'\n", dir.c_str());
        insertedDirectories.insert(dir);
    }
};


static void show_soft_link(AW_window *aws, AW_selection_list *sel_id, const char *envar, DuplicateLinkFilter& unDup) {
    // adds a soft link (e.g. ARBMACROHOME or ARB_WORKDIR) into file selection box
    // if content of 'envar' matches 'cwd' nothing is inserted

    const char *expanded_dir = awt_get_base_directory(envar);
    string      edir(expanded_dir);

    if (unDup.not_seen_yet(edir)) {
        // printf("New directory '%s' (deduced from '$%s')\n", expanded_dir, envar);
        unDup.register_directory(edir);
        const char *entry = GBS_global_string("$ %-18s(%s)", GBS_global_string("'%s'", envar), expanded_dir);
        aws->insert_selection(sel_id, entry, expanded_dir);
    }
    // else {
        // printf("Skipping directory '%s' (deduced from '$%s')\n", expanded_dir, envar);
    // }
}

static void awt_create_selection_box_cb(void *dummy, struct adawcbstruct *cbs) {
    AW_root             *aw_root = cbs->aws->get_root();
    AWUSE(dummy);
    cbs->aws->clear_selection_list(cbs->id);

    char *diru    = aw_root->awar(cbs->def_dir)->read_string();
    char *fulldir = AWT_unfold_path(diru,cbs->pwd);
    char *filter  = aw_root->awar(cbs->def_filter)->read_string();
    char *name    = aw_root->awar(cbs->def_name)->read_string();

#if defined(TRACE_FILEBOX)
    printf("awt_create_selection_box_cb:\n"
           "- diru   ='%s'\n"
           "- fulldir='%s'\n"
           "- filter ='%s'\n"
           "- name   ='%s'\n" , diru, fulldir, filter, name);
#endif // TRACE_FILEBOX

    const char *name_only = 0;
    {
        char *slash = strrchr(name, '/');
        name_only   = slash ? slash+1 : name;
    }

    if (name[0] == '/' && AWT_is_dir(name)) {
        freedup(fulldir, name);
        name_only = "";
    }

    DuplicateLinkFilter  unDup;
    unDup.register_directory(fulldir);

    bool is_wildcard = strchr(name_only, '*');

    if (cbs->show_dir) {
        if (is_wildcard) {
            if (cbs->leave_wildcards) {
                cbs->aws->insert_selection( cbs->id, (char *)GBS_global_string("  ALL '%s' in '%s'", name_only, fulldir), name);
            }
            else {
                cbs->aws->insert_selection( cbs->id, (char *)GBS_global_string("  ALL '%s' in+below '%s'", name_only, fulldir), name);
            }
        }
        else {
            cbs->aws->insert_selection( cbs->id, (char *)GBS_global_string("  CONTENTS OF '%s'",fulldir), fulldir );
        }

        if (filter[0] && !is_wildcard) {
            cbs->aws->insert_selection( cbs->id, GBS_global_string("! \' Search for\'     (*%s)", filter), "*" );
        }
        if (strcmp("/", fulldir)) {
            cbs->aws->insert_selection( cbs->id, "! \'PARENT DIR       (..)\'", ".." );
        }
        if (DIR_subdirs_hidden == 0) {
            show_soft_link(cbs->aws, cbs->id, cbs->pwd, unDup);

            if (cbs->pwdx) {        // additional directories
                char *start = cbs->pwdx;
                while (start) {
                    char *multiple = strchr(start, '^');
                    if (multiple) {
                        multiple[0] = 0;
                        show_soft_link(cbs->aws, cbs->id, start, unDup);
                        multiple[0] = '^';
                        start       = multiple+1;
                    }
                    else {
                        show_soft_link(cbs->aws, cbs->id, start, unDup);
                        start = 0;
                    }
                }
            }

            show_soft_link(cbs->aws, cbs->id, "HOME", unDup);
            show_soft_link(cbs->aws, cbs->id, "PWD", unDup);
            show_soft_link(cbs->aws, cbs->id, "ARB_WORKDIR", unDup);
            show_soft_link(cbs->aws, cbs->id, "PT_SERVER_HOME", unDup);

            cbs->aws->insert_selection( cbs->id, "! \' Sub-directories (shown)\'", GBS_global_string("%s?hide?", name));
        }
        else {
            cbs->aws->insert_selection( cbs->id, "! \' Sub-directories (hidden)\'", GBS_global_string("%s?show?", name));
        }
    }

    cbs->aws->insert_selection( cbs->id, GBS_global_string("! \' Sort order\'     (%s)", DIR_sort_order_name[DIR_sort_order]),
                                GBS_global_string("%s?sort?", name));

    cbs->aws->insert_selection( cbs->id,
                                GBS_global_string("! \' %s%s\'",
                                                  DIR_show_hidden ? "Hide dot-" : "Show hidden ",
                                                  cbs->show_dir ? "files/dirs" : "files"),
                                GBS_global_string("%s?dot?", name));

    if (is_wildcard) {
        if (cbs->leave_wildcards) {
            awt_fill_selection_box_recursive(fulldir, strlen(fulldir)+1, name_only, false, cbs->show_dir && !DIR_subdirs_hidden, DIR_show_hidden, cbs->aws, cbs->id);
        }
        else {
            if (cbs->show_dir) { // recursive wildcarded search
                awt_fill_selection_box_recursive(fulldir, strlen(fulldir)+1, name_only, true, false, DIR_show_hidden, cbs->aws, cbs->id);
            }
            else {
                char *mask = GBS_global_string_copy("%s*%s", name_only, filter);
                awt_fill_selection_box_recursive(fulldir, strlen(fulldir)+1, mask, false, false, DIR_show_hidden, cbs->aws, cbs->id);
                free(mask);
            }
        }
    }
    else {
        char *mask = GBS_global_string_copy("*%s", filter);
        awt_fill_selection_box_recursive(fulldir, strlen(fulldir)+1, mask, false, cbs->show_dir && !DIR_subdirs_hidden, DIR_show_hidden, cbs->aws, cbs->id);
        free(mask);
    }

    cbs->aws->insert_default_selection( cbs->id, "", "" );
    cbs->aws->sort_selection_list(cbs->id, 0, 1);
    cbs->aws->update_selection_list( cbs->id );

    free(name);
    free(fulldir);
    free(diru);
    free(filter);
}

static bool filter_has_changed = false;

void awt_create_selection_box_changed_filter(void *, struct adawcbstruct *) {
    filter_has_changed = true;
#if defined(TRACE_FILEBOX)
    printf("awt_create_selection_box_changed_filter: marked as changed\n");
#endif // TRACE_FILEBOX
}

static void awt_selection_box_changed_filename(void *, struct adawcbstruct *cbs) {
    AW_root *aw_root = cbs->aws->get_root();
    char    *fname   = aw_root->awar(cbs->def_name)->read_string();

#if defined(TRACE_FILEBOX)
    printf("awt_selection_box_changed_filename:\n"
           "- fname='%s'\n", fname);
    printf("- cbs->previous_filename='%s'\n", cbs->previous_filename);
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
            aw_root->awar(cbs->def_name)->write_string(fname); // re-write w/o browser_command
            awt_execute_browser_command(browser_command);
            aw_root->awar(cbs->def_dir)->touch(); // force reinit
        }

        char *newName = 0;
        char *dir     = aw_root->awar(cbs->def_dir)->read_string();

        if (fname[0] == '/' || fname[0] == '~') {
            newName = strdup(GB_get_full_path(fname));
        }
        else {
            if (dir[0]) {
                if (dir[0] == '/') {
                    newName = strdup(GB_concat_full_path(dir, fname));
                }
                else {
                    char *fulldir = 0;

                    if (dir[0] == '.') fulldir = AWT_unfold_path(dir, cbs->pwd);
                    else fulldir               = strdup(GB_get_full_path(dir));

                    newName = strdup(GB_concat_full_path(fulldir, fname));
                    free(fulldir);
                }
            }
            else {
                newName = strdup(GB_get_full_path(fname));
            }
        }

        if (newName) {
            if (AWT_is_dir(newName)) {
                aw_root->awar(cbs->def_name)->write_string("");
                aw_root->awar(cbs->def_dir)->write_string(newName);
                if (cbs->previous_filename) {
                    const char *slash              = strrchr(cbs->previous_filename, '/');
                    const char *name               = slash ? slash+1 : cbs->previous_filename;
                    const char *with_previous_name = GB_concat_full_path(newName, name);

                    if (!AWT_is_dir(with_previous_name)) { // write as new name if not a directory
                        aw_root->awar(cbs->def_name)->write_string(with_previous_name);
                    }
                    else {
                        freeset(cbs->previous_filename, 0);
                        aw_root->awar(cbs->def_name)->write_string(newName);
                    }

                    freeset(newName, aw_root->awar(cbs->def_name)->read_string());
                }
                else {
                    aw_root->awar(cbs->def_name)->write_string("");
                }
            }
            else {
                char *lslash = strrchr(newName, '/');
                if (lslash) {
                    if (lslash == newName) { // root directory
                        aw_root->awar(cbs->def_dir)->write_string("/"); // write directory part
                    }
                    else {
                        lslash[0] = 0;
                        aw_root->awar(cbs->def_dir)->write_string(newName); // write directory part
                        lslash[0] = '/';
                    }
                }

                // now check the correct suffix :
                {
                    char *filter = aw_root->awar(cbs->def_filter)->read_string();
                    if (filter[0]) {
                        char *pfilter = strrchr(filter,'.');
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
                    aw_root->awar(cbs->def_name)->write_string(newName); // loops back if changed !!!
                }

                freeset(cbs->previous_filename, newName);
            }
        }
        free(dir);

        if (strchr(fname, '*')) { // wildcard -> search for suffix
            aw_root->awar(cbs->def_dir)->touch(); // force reinit
        }
    }

    filter_has_changed = false;

    free(fname);
}

#define SELBOX_AUTOREFRESH_FREQUENCY 1000 // refresh once a second

struct selbox_autorefresh_info {
    unsigned long            modtime;
    adawcbstruct            *acbs;
    selbox_autorefresh_info *next;
};
static selbox_autorefresh_info *autorefresh_info = 0;

static GB_ULONG get_dir_modtime(adawcbstruct *acbs) {
    char     *dir   = acbs->awr->awar(acbs->def_dir)->read_string();
    GB_ULONG  mtime = GB_time_of_file(dir);
    free(dir);
    return mtime;
}

static void autorefresh_selboxes(AW_root *) {
    selbox_autorefresh_info *check = autorefresh_info;

    while (check) {
        GB_ULONG mtime = get_dir_modtime(check->acbs);
        if (mtime != check->modtime) {
            check->modtime = mtime;
            check->acbs->awr->awar(check->acbs->def_dir)->touch(); // refresh
        }
        check = check->next;
    }

    // refresh again and again and again..
    autorefresh_info->acbs->awr->add_timed_callback(SELBOX_AUTOREFRESH_FREQUENCY, autorefresh_selboxes);
}

static void awt_selbox_install_autorefresh(adawcbstruct *acbs) {
    if (!autorefresh_info) {    // when installing first selbox
        acbs->awr->add_timed_callback(SELBOX_AUTOREFRESH_FREQUENCY, autorefresh_selboxes);
    }

    selbox_autorefresh_info *install = new selbox_autorefresh_info;

    install->acbs    = acbs;
    install->modtime = get_dir_modtime(acbs);
    
    install->next    = autorefresh_info;
    autorefresh_info = install;
}

void awt_create_selection_box(AW_window *aws, const char *awar_prefix,const char *at_prefix,const  char *pwd, bool show_dir, bool allow_wildcards)
{
    AW_root             *aw_root = aws->get_root();
    struct adawcbstruct *acbs    = new adawcbstruct;
    memset(acbs, 0, sizeof(*acbs));

    acbs->aws = (AW_window *)aws;
    acbs->awr = aw_root;
    acbs->pwd = strdup(pwd);
    {
        char *multiple_dirs_in_pwd = strchr(acbs->pwd, '^');
        if (multiple_dirs_in_pwd) {
            multiple_dirs_in_pwd[0] = 0;
            acbs->pwdx = multiple_dirs_in_pwd+1;
        }
        else {
            acbs->pwdx = 0;
        }
    }

    acbs->show_dir          = show_dir;
    acbs->def_name          = GBS_string_eval(awar_prefix,"*=*/file_name",0);
    acbs->previous_filename = 0;
    acbs->leave_wildcards   = allow_wildcards;

    aw_root->awar(acbs->def_name)->add_callback((AW_RCB1)awt_selection_box_changed_filename,(AW_CL)acbs);

    acbs->def_dir = GBS_string_eval(awar_prefix,"*=*/directory",0);
    aw_root->awar(acbs->def_dir)->add_callback((AW_RCB1)awt_create_selection_box_cb,(AW_CL)acbs);

    acbs->def_filter = GBS_string_eval(awar_prefix,"*=*/filter",0);
    aw_root->awar(acbs->def_filter)->add_callback((AW_RCB1)awt_create_selection_box_changed_filter,(AW_CL)acbs);
    aw_root->awar(acbs->def_filter)->add_callback((AW_RCB1)awt_selection_box_changed_filename,(AW_CL)acbs);
    aw_root->awar(acbs->def_filter)->add_callback((AW_RCB1)awt_create_selection_box_cb,(AW_CL)acbs);

    char buffer[1024];
    sprintf(buffer,"%sfilter",at_prefix);
    if (aws->at_ifdef(buffer)){
        aws->at(buffer);
        aws->create_input_field(acbs->def_filter,5);
    }

    sprintf(buffer,"%sfile_name",at_prefix);
    if (aws->at_ifdef(buffer)){
        aws->at(buffer);
        aws->create_input_field(acbs->def_name,20);
    }

    sprintf(buffer,"%sbox",at_prefix);
    aws->at(buffer);
    acbs->id = aws->create_selection_list(acbs->def_name,0,"",2,2);
    
    awt_create_selection_box_cb(0,acbs);
    awt_selection_box_changed_filename(0, acbs);    // this fixes the path name

    awt_selbox_install_autorefresh(acbs);
}

char *awt_get_selected_fullname(AW_root *awr, const char *awar_prefix) {
    char *file = awr->awar(GBS_global_string("%s/file_name", awar_prefix))->read_string();
    if (file[0] != '/') {
        // if name w/o directory was entered by hand (or by default) then append the directory :

        char    *awar_dir_name = GBS_global_string_copy("%s/directory", awar_prefix);
        AW_awar *awar_dir      = awr->awar_no_error(awar_dir_name);

        if (!awar_dir) {
            // file selection box was not active (happens e.g. for print tree)
            awar_dir = awr->awar_string(awar_dir_name, GB_getcwd());
        }

        awt_assert(awar_dir);

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

void awt_refresh_selection_box(AW_root *awr, const char *awar_prefix) {
    awr->awar(GBS_global_string("%s/directory", awar_prefix))->touch();
}
