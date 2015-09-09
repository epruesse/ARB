// =============================================================== //
//                                                                 //
//   File      : adfile.cxx                                        //
//   Purpose   : various IO related functions                      //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#include <arb_sort.h>
#include <arb_str.h>
#include <arb_strarray.h>
#include <arb_file.h>
#include <arb_pathlen.h>

#include "gb_local.h"
#include "gb_load.h"

GB_CSTR GB_getcwd() {
    // get the current working directory
    // (directory from which application has been started)
    RETURN_ONETIME_ALLOC(getcwd(0, ARB_PATH_MAX));
}

GB_ERROR gb_scan_directory(char *basename, gb_scandir *sd) {
    // goes to header: __ATTR__USERESULT_TODO
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
    char         buffer[ARB_PATH_MAX];
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

    DIR         *dirp;
    struct stat  st;
    char        *result = 0;

    dirp = opendir(dir);
    if (dirp) {
        GBS_string_matcher *matcher = GBS_compile_matcher(mask, GB_IGNORE_CASE);
        if (matcher) {
            for (dirent *dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
                if (GBS_string_matches_regexp(dp->d_name, matcher)) {
                    char buffer[ARB_PATH_MAX];
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

    DIR         *dirp;
    struct stat  st;
    char        *result = 0;

    dirp = opendir(dir);
    if (dirp) {
        GBS_string_matcher *matcher = GBS_compile_matcher(mask, GB_IGNORE_CASE);
        if (matcher) {
            GB_ULONG newest = 0;
            for (dirent *dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
                if (GBS_string_matches_regexp(dp->d_name, matcher)) {
                    char buffer[ARB_PATH_MAX];
                    sprintf(buffer, "%s/%s", dir, dp->d_name);
                    if (stat(buffer, &st) == 0 &&
                        S_ISREG(st.st_mode) &&
                        (GB_ULONG)st.st_mtime > newest) {
                            newest = st.st_mtime;
                            freedup(result, dp->d_name);
                    }
                }
            }
            GBS_free_matcher(matcher);
        }
        closedir(dirp);
    }
    return result;
}

static const char *GB_existing_file(const char *file, bool warn_when_not_found) {
    // return 'file' if it's an existing readable file
    // return NULL otherwise

    gb_assert(file);
    if (GB_is_readablefile(file)) return file;
    if (warn_when_not_found) GB_warningf("Could not find '%s'", file);
    return NULL;
}

char *GB_lib_file(bool warn_when_not_found, const char *libprefix, const char *filename) {
    // Search a file in '$ARBHOME/lib/libprefix'
    // Return NULL if not found
    return nulldup(GB_existing_file(GB_path_in_ARBLIB(libprefix, filename), warn_when_not_found));
}

char *GB_property_file(bool warn_when_not_found, const char *filename) {
    // Search a file in '$ARB_PROP' or its default in '$ARBHOME/lib/arb_default'
    // Return NULL if neighter found

    char *result = nulldup(GB_existing_file(GB_path_in_arbprop(filename), warn_when_not_found));
    if (!result) result = GB_lib_file(warn_when_not_found, "arb_default", filename);
    return result;
}

void GBS_read_dir(StrArray& names, const char *dir, const char *mask) {
    /* Return full pathnames of files in directory 'dir'.
     *
     * Filter through 'mask':
     * - mask == NULL -> return all files
     * -      in format '/expr/' -> use regular expression (case sensitive)
     * - else it does a simple string match with wildcards ("?*")
     *
     * Result are inserted into 'names' and 'names' is sorted alphanumerically.
     * Note: 'names' are not cleared, so several calls with the same StrArray get collected.
     *
     * In case of error, 'names' is empty and error is exported.
     *
     * Special case: If 'dir' is the name of a file, return an array with file as only element
     */

    gb_assert(dir);             // dir == NULL was allowed before 12/2008, forbidden now!

    if (dir[0]) {
        char *fulldir   = strdup(GB_canonical_path(dir));
        DIR  *dirstream = opendir(fulldir);

        if (!dirstream) {
            if (GB_is_readablefile(fulldir)) { // fixed: returned true for directories before (4/2012)
                names.put(strdup(fulldir));
            }
            else {
                // @@@ does too much voodoo here - fix
                char *lslash = strrchr(fulldir, '/');

                if (lslash) {
                    lslash[0]  = 0;
                    char *name = lslash+1;
                    if (GB_is_directory(fulldir)) {
                        GBS_read_dir(names, fulldir, name); // @@@ concat mask to name?
                    }
                    lslash[0] = '/';
                }

                if (names.empty()) GB_export_errorf("can't read directory '%s'", fulldir); // @@@ wrong place for error; maybe return error as result
            }
        }
        else {
            if (mask == NULL) mask = "*";

            GBS_string_matcher *matcher = GBS_compile_matcher(mask, GB_MIND_CASE);
            if (matcher) {
                dirent *entry;
                while ((entry = readdir(dirstream)) != 0) {
                    const char *name = entry->d_name;

                    if (name[0] == '.' && (name[1] == 0 || (name[1] == '.' && name[2] == 0))) {
                        ; // skip '.' and '..'
                    }
                    else {
                        if (GBS_string_matches_regexp(name, matcher)) {
                            const char *full = GB_concat_path(fulldir, name);
                            if (!GB_is_directory(full)) { // skip directories
                                names.put(strdup(full));
                            }
                        }
                    }
                }

                names.sort(GB_string_comparator, 0);

                GBS_free_matcher(matcher);
            }

            closedir(dirstream);
        }

        free(fulldir);
    }
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>

static char *remove_path(const char *fullname, void *cl_path) {
    const char *path = (const char *)cl_path;
    return strdup(fullname+(ARB_strBeginsWith(fullname, path) ? strlen(path) : 0));
}

static void GBT_transform_names(StrArray& dest, const StrArray& source, char *transform(const char *, void *), void *client_data) {
    for (int i = 0; source[i]; ++i) dest.put(transform(source[i], client_data));
}

#define TEST_JOINED_FULLDIR_CONTENT_EQUALS(fulldir,mask,expected) do {                  \
        StrArray  contents;                                                             \
        GBS_read_dir(contents, fulldir, mask);                                          \
        StrArray  contents_no_path;                                                     \
        GBT_transform_names(contents_no_path, contents, remove_path, (void*)fulldir);   \
        char     *joined  = GBT_join_strings(contents_no_path, '!');                    \
        TEST_EXPECT_EQUAL(joined, expected);                                            \
        free(joined);                                                                   \
    } while(0)

#define TEST_JOINED_DIR_CONTENT_EQUALS(subdir,mask,expected) do {       \
        char     *fulldir = strdup(GB_path_in_ARBHOME(subdir));         \
        TEST_JOINED_FULLDIR_CONTENT_EQUALS(fulldir,mask,expected);      \
        free(fulldir);                                                  \
    } while(0)

void TEST_GBS_read_dir() {
    TEST_JOINED_DIR_CONTENT_EQUALS("GDE/CLUSTAL", "*.c",       "/amenu.c!/clustalv.c!/gcgcheck.c!/myers.c!/sequence.c!/showpair.c!/trees.c!/upgma.c!/util.c");
    TEST_JOINED_DIR_CONTENT_EQUALS("GDE/CLUSTAL", "/s.*\\.c/", "/clustalv.c!/myers.c!/sequence.c!/showpair.c!/trees.c");

    // test a dir containing subdirectories
    TEST_JOINED_DIR_CONTENT_EQUALS("SL", NULL, "/Makefile!/README");
    TEST_JOINED_DIR_CONTENT_EQUALS("SL", "*",  "/Makefile!/README");

    TEST_JOINED_FULLDIR_CONTENT_EQUALS("", "", ""); // allow GBS_read_dir to be called with "" -> returns empty filelist
}

void TEST_find_file() {
    gb_getenv_hook old = GB_install_getenv_hook(arb_test::fakeenv);

    TEST_EXPECT_EQUAL(GB_existing_file("min_ascii.arb", false), "min_ascii.arb");
    TEST_EXPECT_NULL(GB_existing_file("nosuchfile", false));

    char *tcporg = GB_lib_file(false, "", "arb_tcp_org.dat");
    TEST_EXPECT_EQUAL(tcporg, GB_path_in_ARBHOME("lib/arb_tcp_org.dat"));
    TEST_EXPECT_NULL(GB_lib_file(true, "bla", "blub"));
    free(tcporg);

    char *status = GB_property_file(false, "status.arb");
    TEST_EXPECT_EQUAL(status, GB_path_in_ARBHOME("lib/arb_default/status.arb"));
    TEST_EXPECT_NULL(GB_property_file(true, "undhepp"));
    free(status);

    TEST_EXPECT_EQUAL((void*)arb_test::fakeenv, (void*)GB_install_getenv_hook(old));
}
TEST_PUBLISH(TEST_find_file);

#endif // UNIT_TESTS
