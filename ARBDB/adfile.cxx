// =============================================================== //
//                                                                 //
//   File      : adfile.cxx                                        //
//   Purpose   : various IO related functions                      //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <climits>
#include <cctype>

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#include "gb_local.h"
#include "gb_load.h"

#define FILE_PATH_MAX PATH_MAX

GB_CSTR GB_getcwd() {
    // get the current working directory
    // (directory from which application has been started)

    static char *lastcwd  = 0;
    if (!lastcwd) lastcwd = (char *)getcwd(0, FILE_PATH_MAX);
    return lastcwd;
}

GB_ERROR gb_scan_directory(char *basename, gb_scandir *sd) { // goes to header: __ATTR__USERESULT
    // look for quick saves (basename = yyy/xxx no arb ending !!!!)
    char        *path        = strdup(basename);
    const char  *fulldir     = ".";
    char        *file        = strrchr(path, '/');
    DIR         *dirp;
    int          curindex;
    char        *suffix;
    dirent      *dp;
    struct stat  st;
    const char  *oldstyle    = ".arb.quick";
    char         buffer[FILE_PATH_MAX];
    int          oldstylelen = strlen(oldstyle);
    int          filelen;

    if (file) {
        *(file++) = 0;
        fulldir = path;
    }
    else {
        file = path;
    }

    memset((char*)sd, 0, sizeof(*sd));
    sd->type = GB_SCAN_NO_QUICK;
    sd->highest_quick_index = -1;
    sd->newest_quick_index = -1;
    sd->date_of_quick_file = 0;

    dirp = opendir(fulldir);
    if (!dirp) {
        GB_ERROR error = GBS_global_string("Directory %s of file %s.arb not readable", fulldir, file);
        free(path);
        return error;
    }
    filelen = strlen(file);
    for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
        if (strncmp(dp->d_name, file, filelen)) continue;
        suffix = dp->d_name + filelen;
        if (suffix[0] != '.') continue;
        if (!strncmp(suffix, oldstyle, oldstylelen)) {
            if (sd->type == GB_SCAN_NEW_QUICK) {
                printf("Warning: Found new and old changes files, using new\n");
                continue;
            }
            sd->type = GB_SCAN_OLD_QUICK;
            curindex = atoi(suffix+oldstylelen);
            goto check_time_and_date;
        }
        if (strlen(suffix) == 4 &&
            suffix[0] == '.' &&
            suffix[1] == 'a' &&
            isdigit(suffix[2]) &&
            isdigit(suffix[3])) {
            if (sd->type == GB_SCAN_OLD_QUICK) {
                printf("Warning: Found new and old changes files, using new\n");
            }
            sd->type = GB_SCAN_NEW_QUICK;
            curindex = atoi(suffix+2);
            goto check_time_and_date;
        }
        continue;
    check_time_and_date :
        if (curindex > sd->highest_quick_index) sd->highest_quick_index = curindex;
        sprintf(buffer, "%s/%s", fulldir, dp->d_name);
        stat(buffer, &st);
        if ((unsigned long)st.st_mtime > sd->date_of_quick_file) {
            sd->date_of_quick_file = st.st_mtime;
            sd->newest_quick_index = curindex;
        }
        continue;
    }
    closedir(dirp);
    free(path);
    return 0;
}


char *GB_find_all_files(const char *dir, const char *mask, bool filename_only) {
    /* Returns a string containing the filenames of all files matching mask.
       The single filenames are separated by '*'.
       if 'filename_only' is true -> string contains only filenames w/o path

       returns 0 if no files found (or directory not found).
       in this case an error may be exported

       'mask' may contain wildcards (*?) or
       it may be a regular expression ('/regexp/')
    */

    DIR           *dirp;
    struct dirent *dp;
    struct stat    st;
    char          *result = 0;
    char           buffer[FILE_PATH_MAX];

    dirp = opendir(dir);
    if (dirp) {
        GBS_string_matcher *matcher = GBS_compile_matcher(mask, GB_IGNORE_CASE);
        if (matcher) {
            for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
                if (GBS_string_matches_regexp(dp->d_name, matcher)) {
                    sprintf(buffer, "%s/%s", dir, dp->d_name);
                    if (stat(buffer, &st) == 0  && S_ISREG(st.st_mode)) { // regular file ?
                        if (filename_only) strcpy(buffer, dp->d_name);
                        if (result) {
                            freeset(result, GBS_global_string_copy("%s*%s", result, buffer));
                        }
                        else {
                            result = strdup(buffer);
                        }
                    }
                }
            }
            GBS_free_matcher(matcher);
        }
        closedir(dirp);
    }

    return result;
}

char *GB_find_latest_file(const char *dir, const char *mask) {
    /* returns the name of the newest file in dir 'dir' matching 'mask'
     * or NULL (in this case an error may be exported)
     *
     * 'mask' may contain wildcards (*?) or
     * it may be a regular expression ('/regexp/')
     */

    DIR           *dirp;
    struct dirent *dp;
    char           buffer[FILE_PATH_MAX];
    struct stat    st;
    GB_ULONG       newest = 0;
    char          *result = 0;

    dirp = opendir(dir);
    if (dirp) {
        GBS_string_matcher *matcher = GBS_compile_matcher(mask, GB_IGNORE_CASE);
        if (matcher) {
            for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
                if (GBS_string_matches_regexp(dp->d_name, matcher)) {
                    sprintf(buffer, "%s/%s", dir, dp->d_name);
                    if (stat(buffer, &st) == 0) {
                        if ((GB_ULONG)st.st_mtime > newest) {
                            newest = st.st_mtime;
                            freedup(result, dp->d_name);
                        }
                    }
                }
            }
            GBS_free_matcher(matcher);
        }
        closedir(dirp);
    }
    return result;
}

char *GBS_find_lib_file(const char *filename, const char *libprefix, int warn_when_not_found) {
    // Searches files in current dir, $HOME, $ARBHOME/lib/libprefix

    char *result = 0;

    if (GB_is_readablefile(filename)) {
        result = strdup(filename);
    }
    else {
        const char *slash = strrchr(filename, '/'); // look for last slash

        if (slash && filename[0] != '.') { // have absolute path
            filename = slash+1; // only use filename part
            slash    = 0;
        }

        const char *fileInHome = GB_concat_full_path(GB_getenvHOME(), filename);

        if (fileInHome && GB_is_readablefile(fileInHome)) {
            result = strdup(fileInHome);
        }
        else {
            if (slash) filename = slash+1; // now use filename only, even if path starts with '.'

            const char *fileInLib = GB_path_in_ARBLIB(libprefix, filename);

            if (fileInLib && GB_is_readablefile(fileInLib)) {
                result = strdup(fileInLib);
            }
            else {
                if (warn_when_not_found) {
                    GB_warningf("Don't know where to find '%s'\n"
                                "  searched in '.'\n"
                                "  searched in $(HOME) (for '%s')\n"
                                "  searched in $(ARBHOME)/lib/%s (for '%s')\n",
                                filename, fileInHome, libprefix, fileInLib);
                }
            }
        }
    }

    return result;
}

char **GBS_read_dir(const char *dir, const char *mask) {
    /* Return names of files in directory 'dir'.
     * Filter through 'mask':
     * - mask == NULL -> return all files
     * -      in format '/expr/' -> use regular expression (case sensitive)
     * - else it does a simple string match with wildcards ("?*")
     *
     * Result is a NULL terminated array of char* (sorted alphanumerically)
     * Use GBT_free_names() to free the result.
     *
     * In case of error, result is NULL and error is exported
     *
     * Special case: If 'dir' is the name of a file, return an array with file as only element
     */

    gb_assert(dir);             // dir == NULL was allowed before 12/2008, forbidden now!

    char  *fulldir   = nulldup(GB_get_full_path(dir));
    DIR   *dirstream = opendir(fulldir);
    char **names     = NULL;

    if (!dirstream) {
        if (GB_is_readablefile(fulldir)) {
            names    = (char**)malloc(2*sizeof(*names));
            names[0] = strdup(fulldir);
            names[1] = NULL;
        }
        else {
            char *lslash = strrchr(fulldir, '/');

            if (lslash) {
                char *name;
                lslash[0] = 0;
                name      = lslash+1;
                if (GB_is_directory(fulldir)) {
                    names = GBS_read_dir(fulldir, name);
                }
                lslash[0] = '/';
            }

            if (!names) GB_export_errorf("can't read directory '%s'", fulldir);
        }
    }
    else {
        if (mask == NULL) mask = "*";

        GBS_string_matcher *matcher = GBS_compile_matcher(mask, GB_MIND_CASE);
        if (matcher) {
            int allocated = 100;
            int entries   = 0;
            names         = (char**)malloc(100*sizeof(*names));

            struct dirent *entry;
            while ((entry = readdir(dirstream)) != 0) {
                const char *name = entry->d_name;

                if (name[0] == '.' && (name[1] == 0 || (name[1] == '.' && name[2] == 0))) {
                    ; // skip '.' and '..'
                }
                else {
                    if (GBS_string_matches_regexp(name, matcher)) {
                        const char *full = GB_concat_path(fulldir, name);
                        if (!GB_is_directory(full)) { // skip directories
                            if (entries == allocated) {
                                allocated += allocated>>1; // * 1.5
                                names      = (char**)realloc(names, allocated*sizeof(*names));
                            }
                            names[entries++] = strdup(full);
                        }
                    }
                }
            }

            names          = (char**)realloc(names, (entries+1)*sizeof(*names));
            names[entries] = NULL;

            GB_sort((void**)names, 0, entries, GB_string_comparator, 0);

            GBS_free_matcher(matcher);
        }

        closedir(dirstream);
    }

    free(fulldir);

    return names;
}

