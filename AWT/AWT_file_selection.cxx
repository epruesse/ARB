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

static GB_CSTR awt_get_base_directory(const char *pwd_envar) {
    GB_CSTR res;
    
    if (strcmp(pwd_envar, "PWD") == 0) {
        res = GB_getcwd();
    }
    else if (strcmp(pwd_envar, "PT_SERVER_HOME") == 0) {        
        static char *pt_server_home = GBS_global_string_copy("%s/lib/pts", GB_getenvARBHOME());
        res                         = pt_server_home;
    }
    else {
        res = GB_getenv(pwd_envar);
        if (!res) {
            res = GB_getcwd();
            aw_message(GBS_global_string("Environment variable '%s' not defined - using '%s' as base dir", pwd_envar, res));
        }
    }

    return res;
}

GB_CSTR AWT_get_full_path(const char *anypath) {
    static char buf[FILENAME_MAX+1];
    if (strlen(anypath) > FILENAME_MAX) GB_CORE;
    if (strncmp(anypath, "~/", 2) == 0) {
        GB_CSTR home    = GB_getenvHOME();
        GB_CSTR newpath = GBS_global_string("%s%s", home, anypath+1);
        realpath(newpath, buf);
    }
    else {
        realpath(anypath, buf);
    }
    return buf;
}

GB_CSTR AWT_concat_full_path(const char *anypath_left, const char *anypath_right) {
    static char buf[FILENAME_MAX+1];

    int len_left  = strlen(anypath_left);
    int len_right = strlen(anypath_right);
    if ((len_left+1+len_right) > FILENAME_MAX) GB_CORE;

    sprintf(buf, "%s/%s", anypath_left, anypath_right);
    return AWT_get_full_path(buf);
}

GB_CSTR AWT_get_suffix(const char *fullpath) { // returns pointer behind '.' of suffix (or NULL if no suffix found)
    GB_CSTR dot = strrchr(fullpath, '.');
    if (!dot) return 0;

    GB_CSTR lslash = strrchr(fullpath, '/');
    if (lslash && lslash>dot) return 0; // no . behind last /
    return dot+1;
}

GB_CSTR AWT_append_suffix(const char *name, const char *suffix) {
    // returns "name.suffix" (checks for multiple dots)
    static char  buf[FILENAME_MAX+1];
    strcpy(buf, name ? name : "noname"); // force a name!
    char *end = strchr(buf, 0);
    char *ep;

    for (ep = end-1; ep >= buf && ep[0] == '.'; --ep) ; // point before dot
    awt_assert(ep >= buf);      // name consist of dots only
    ++ep;

    if (suffix) {
        while (suffix[0] == '.') ++suffix; // search for real suffix start
        if (!suffix[0]) suffix = 0;
    }

    if (suffix) {
        *ep++ = '.';
        strcpy(ep, suffix);
    }
    else {
        ep[0] = 0; // no suffix
    }

    return buf;
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
    if (path[0] == '/' || path[0] == '~') return strdup(AWT_get_full_path(path));
    return strdup(AWT_concat_full_path(awt_get_base_directory(pwd_envar), path));
}

const char *AWT_valid_path(const char *path) {
    if (strlen(path)) return path;
    return ".";
}

int AWT_is_dir(const char *path) {
    struct stat stt;
    if (stat(AWT_valid_path(path), &stt)) return 0;
    if (S_ISDIR(stt.st_mode)) return 1;
    return 0;
}
int AWT_is_file(const char *path) {
    struct stat stt;
    if (stat(AWT_valid_path(path), &stt)) return 0;
    if (S_ISREG(stt.st_mode)) return 1;
    return 0;
}

char *AWT_extract_directory(const char *path) {
    char *lslash = strrchr(path, '/');
    if (!lslash) return 0;

    char *result        = GB_strdup(path);
    result[lslash-path] = 0;

    return result;
}

void awt_fill_selection_box_recursive(const char *fulldir, int skipleft, const char *mask, bool recurse, bool showdir, AW_window *aws, AW_selection_list *selid) {
    DIR *dirp = opendir(fulldir);

    if (!dirp) {
        aws->insert_selection(selid, GBS_global_string("x Your directory path is invalid (%s)", fulldir), "?");
        return;
    }

    struct dirent *dp;
    for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
        const char *entry = dp->d_name;
        char       *fullname;

        if (strlen(fulldir)) fullname = strdup(AWT_concat_full_path(fulldir, entry));
        else fullname                 = strdup(AWT_get_full_path(entry));

        if (AWT_is_dir(fullname)) {
            if (entry[0] == '.' && (entry[1] == 0 || (entry[1] == '.' && entry[2] == 0))) {
                // skip "." and ".."
            }
            else {
                if (showdir) aws->insert_selection(selid, GBS_global_string("D %s", entry), fullname);
                if (recurse) awt_fill_selection_box_recursive(fullname, skipleft, mask, recurse, showdir, aws, selid);
            }
        }
        else {
            if (GBS_string_cmp(entry, mask, 0) == 0) { // entry matches mask
                struct stat stt;
                if (stat(fullname, &stt) == 0 && S_ISREG(stt.st_mode)) { // regular existing file
                    char       atime[256];
                    struct tm *tms = localtime(&stt.st_mtime);
                    strftime( atime, 255,"%b %d %k:%M %Y",tms);

                    long ksize = stt.st_size/1024;
                    aws->insert_selection(selid, GBS_global_string("f %-30s   '%5lik %s'", fullname+skipleft, ksize, atime), fullname);
                }
            }
        }
        free(fullname);
    }

    closedir(dirp);
}

static void show_soft_link(AW_window *aws, AW_selection_list *sel_id, const char *envar, const char *cwd) {
    // adds a soft link (e.g. ARBMACROHOME or ARB_WORKDIR) into file selection box
    // if content of 'envar' matches 'cwd' nothing is inserted

    const char *expanded_dir = awt_get_base_directory(envar);
    if (strcmp(expanded_dir, cwd)) {
        const char *entry = GBS_global_string("D %-*s(%s)", 18, GBS_global_string("'$%s'", envar), expanded_dir);
        aws->insert_selection(sel_id, entry, expanded_dir);
    }
}

static void show_soft_link_nodup(AW_window *aws, AW_selection_list *sel_id, const char *envar, const char *cwd) {
    // avoid duplicate soft links
    if (strstr(GBS_global_string(";%s;", envar), ";HOME;ARBHOME;ARB_WORKDIR;PT_SERVER_HOME;PWD;") == 0) {
        show_soft_link(aws, sel_id, envar, cwd);
    }
}


void awt_create_selection_box_cb(void *dummy, struct adawcbstruct *cbs) {
    AW_root *aw_root = cbs->aws->get_root();
    AWUSE(dummy);
    cbs->aws->clear_selection_list(cbs->id);

    char       *diru      = aw_root->awar(cbs->def_dir)->read_string();
    char       *fulldir   = AWT_unfold_path(diru,cbs->pwd);
    char       *filter    = aw_root->awar(cbs->def_filter)->read_string();
    char       *name      = aw_root->awar(cbs->def_name)->read_string();
    const char *name_only = 0;
    {
        char *slash = strrchr(name, '/');
        name_only   = slash ? slash+1 : name;
    }
    bool is_wildcard = strchr(name_only, '*');
    
    if (cbs->show_dir) {
        if (is_wildcard) {
            cbs->aws->insert_selection( cbs->id, (char *)GBS_global_string("ALL '%s' below '%s'", name_only, fulldir), name);
        }
        else {
            cbs->aws->insert_selection( cbs->id, (char *)GBS_global_string("CONTENTS OF '%s'",fulldir), fulldir );
        }

        if (strcmp("/", fulldir)) {
            cbs->aws->insert_selection( cbs->id, "D \' PARENT DIR      (..)\'", ".." );
        }

        if (filter[0] && !is_wildcard) {
            cbs->aws->insert_selection( cbs->id, GBS_global_string("D \' Search for\'     (*%s)", filter), "*" );
        }

        show_soft_link_nodup(cbs->aws, cbs->id, cbs->pwd, fulldir);

        if (cbs->pwdx) {        // additional directories
            char *start = cbs->pwdx;
            while (start) {
                char *multiple = strchr(start, '^');
                if (multiple) {
                    multiple[0] = 0;
                    show_soft_link_nodup(cbs->aws, cbs->id, start, fulldir);
                    multiple[0] = '^';
                    start       = multiple+1;
                }
                else {
                    show_soft_link_nodup(cbs->aws, cbs->id, start, fulldir);
                    start = 0;
                }
            }
        }

        show_soft_link(cbs->aws, cbs->id, "HOME", fulldir);
        show_soft_link(cbs->aws, cbs->id, "PWD", fulldir);
        show_soft_link(cbs->aws, cbs->id, "ARB_WORKDIR", fulldir);
        show_soft_link(cbs->aws, cbs->id, "PT_SERVER_HOME", fulldir);
    }

    if (is_wildcard) {
        awt_fill_selection_box_recursive(fulldir, strlen(fulldir)+1, name_only, true, false, cbs->aws, cbs->id);
    }
    else {
        char *mask = GBS_global_string_copy("*%s", filter);
        awt_fill_selection_box_recursive(fulldir, strlen(fulldir)+1, mask, false, cbs->show_dir, cbs->aws, cbs->id);
        free(mask);
    }

    cbs->aws->insert_default_selection( cbs->id, "", "" );
    cbs->aws->sort_selection_list(cbs->id, 0);
    cbs->aws->update_selection_list( cbs->id );

    free(name);
    free(fulldir);
    free(diru);
    free(filter);
}

static bool filter_has_changed = false;

void awt_create_selection_box_changed_filter(void *, struct adawcbstruct *) {
    filter_has_changed = true;
}

void awt_create_selection_box_changed_filename(void *, struct adawcbstruct *cbs) {
    AW_root *aw_root = cbs->aws->get_root();
    char    *fname   = aw_root->awar(cbs->def_name)->read_string();

    if (fname[0]) {
        char *newName = 0;
        char *dir     = aw_root->awar(cbs->def_dir)->read_string();

        if (fname[0] == '/' || fname[0] == '~') {
            newName = strdup(AWT_get_full_path(fname));
        }
        else {
            if (dir[0]) {
                if (dir[0] == '/') {
                    newName = strdup(AWT_concat_full_path(dir, fname));
                }
                else {
                    char *fulldir = 0;

                    if (dir[0] == '.') fulldir = AWT_unfold_path(dir, cbs->pwd);
                    else fulldir               = strdup(AWT_get_full_path(dir));

                    newName = strdup(AWT_concat_full_path(fulldir, fname));
                    free(fulldir);
                }
            }
            else {
                newName = strdup(AWT_get_full_path(fname));
            }
        }

        if (newName) {
            if (AWT_is_dir(newName)) {
                aw_root->awar(cbs->def_dir)->write_string(newName);
                if (cbs->previous_filename) {
                    const char *slash              = strrchr(cbs->previous_filename, '/');
                    const char *name               = slash ? slash+1 : cbs->previous_filename;
                    const char *with_previous_name = AWT_concat_full_path(newName, name);

                    if (!AWT_is_dir(with_previous_name)) { // write as new name if not a directory
                        aw_root->awar(cbs->def_name)->write_string(with_previous_name);
                    }
                    else {
                        aw_root->awar(cbs->def_name)->write_string(newName);
                    }

                    free(newName);
                    newName = aw_root->awar(cbs->def_name)->read_string();
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
                        char    *pfilter = strrchr(filter,'.');
                        pfilter          = pfilter ? pfilter+1 : filter;
                        GB_CSTR  suffix  = AWT_get_suffix(newName);

                        if (!suffix || strcmp(suffix, pfilter) != 0) {
                            if (suffix && filter_has_changed) {
                                *(char*)suffix = 0; // suffix points into new name - so changing is ok here
                            }

                            char *n = strdup(AWT_append_suffix(newName, pfilter));
                            free(newName);
                            newName = n;
                        }
                    }
                    free(filter);
                }

                if (strcmp(newName, fname) != 0) {
                    aw_root->awar(cbs->def_name)->write_string(newName); // loops back if changed !!!
                }
            }
        }
        free(cbs->previous_filename);
        cbs->previous_filename = newName; // this releases newName
        free(dir);

        if (strchr(fname, '*')) { // wildcard -> search for suffix
            aw_root->awar(cbs->def_dir)->touch(); // force reinit
        }
    }

    filter_has_changed = false;

    free(fname);
}


void awt_create_selection_box(AW_window *aws, const char *awar_prefix,const char *at_prefix,const  char *pwd, AW_BOOL show_dir )
{
    AW_root             *aw_root = aws->get_root();
    struct adawcbstruct *acbs    = new adawcbstruct;

    acbs->aws = (AW_window *)aws;
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

    aw_root->awar(acbs->def_name)->add_callback((AW_RCB1)awt_create_selection_box_changed_filename,(AW_CL)acbs);

    acbs->def_dir = GBS_string_eval(awar_prefix,"*=*/directory",0);
    aw_root->awar(acbs->def_dir)->add_callback((AW_RCB1)awt_create_selection_box_cb,(AW_CL)acbs);

    acbs->def_filter = GBS_string_eval(awar_prefix,"*=*/filter",0);
    aw_root->awar(acbs->def_filter)->add_callback((AW_RCB1)awt_create_selection_box_changed_filter,(AW_CL)acbs);
    aw_root->awar(acbs->def_filter)->add_callback((AW_RCB1)awt_create_selection_box_changed_filename,(AW_CL)acbs);
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
    awt_create_selection_box_changed_filename(0, acbs); // this fixes the path name
}

char *awt_get_selected_fullname(AW_root *awr, const char *awar_prefix) {
    char *file = awr->awar(GBS_global_string("%s/file_name", awar_prefix))->read_string();
    if (file[0] == '/') return file; // it's a full path name -> use it

    // if name w/o directory was entered by hand (or by default) then append the directory :

    char    *awar_name = GBS_global_string_copy("%s/directory", awar_prefix);
    AW_awar *awar_dir  = awr->awar_no_error(awar_name);
    if (!awar_dir) { // file selection box was not active (happens e.g. for print tree)
        awar_dir = awr->awar_string(awar_name, GB_getcwd());
    }
    free(awar_name);

    char *dir  = awr->awar(GBS_global_string("%s/directory", awar_prefix))->read_string();
    char *full = strdup(AWT_concat_full_path(dir, file));

    free(dir);
    free(file);

    return full;
}

void awt_refresh_selection_box(AW_root *awr, const char *awar_prefix) {
    awr->awar(GBS_global_string("%s/directory", awar_prefix))->touch();
}
