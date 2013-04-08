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

#include <list>
#include <string>

#include <arb_sort.h>
#include <arb_str.h>
#include <arb_strarray.h>
#include <arb_file.h>

#include "gb_local.h"
#include "gb_load.h"

#define FILE_PATH_MAX PATH_MAX

GB_CSTR GB_getcwd() {
    // get the current working directory
    // (directory from which application has been started)
    RETURN_ONETIME_ALLOC(getcwd(0, FILE_PATH_MAX));
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

    DIR         *dirp;
    struct stat  st;
    char        *result = 0;

    dirp = opendir(dir);
    if (dirp) {
        GBS_string_matcher *matcher = GBS_compile_matcher(mask, GB_IGNORE_CASE);
        if (matcher) {
            for (dirent *dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
                if (GBS_string_matches_regexp(dp->d_name, matcher)) {
                    char buffer[FILE_PATH_MAX];
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
                    char buffer[FILE_PATH_MAX];
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
    // Search a file in '~/.arb_prop' or its default in '$ARBHOME/lib/arb_default'
    // Return NULL if neighter found

    char *result = nulldup(GB_existing_file(GB_path_in_arbprop(filename), warn_when_not_found));
    if (!result) result = GB_lib_file(warn_when_not_found, "arb_default", filename);
    return result;
}

void GBS_read_dir(StrArray& names, const char *dir, const char *mask) {
    /* Return names of files in directory 'dir'.
     * Filter through 'mask':
     * - mask == NULL -> return all files
     * -      in format '/expr/' -> use regular expression (case sensitive)
     * - else it does a simple string match with wildcards ("?*")
     *
     * Result are inserted into 'names' and 'names' is sorted alphanumerically.
     * Note: 'names' are not cleared, so several calls with the same StrArray get collected.
     *
     * In case of error, result is empty and error is exported.
     *
     * Special case: If 'dir' is the name of a file, return an array with file as only element
     */

    gb_assert(dir);             // dir == NULL was allowed before 12/2008, forbidden now!

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

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>

// GB_test_textfile_difflines + GB_test_files_equal are helper functions used
// by unit tests.
//
// @@@ GB_test_textfile_difflines + GB_test_files_equal -> ARBCORE

#define MAX_REGS 13

class difflineMode : virtual Noncopyable {
    int               mode;
    GBS_regex        *reg[MAX_REGS];
    const char       *replace[MAX_REGS];
    int               count;
    mutable GB_ERROR  error;

    static bool is_may;

    void add(const char *regEx, GB_CASE case_flag, const char *replacement) {
        if (!error) {
            gb_assert(count<MAX_REGS);
            reg[count] = GBS_compile_regexpr(regEx, case_flag, &error);
            if (!error) {
                replace[count] = replacement;
                count++;
            }
        }
    }

public:
    difflineMode(int mode_)
        : mode(mode_),
          count(0),
          error(NULL)
    {
        memset(reg, 0, sizeof(reg));
        switch (mode) {
            case 0: break;
            case 1:  {
                add("[0-9]{2}:[0-9]{2}:[0-9]{2}", GB_MIND_CASE, "<TIME>");
                add("(Mon|Tue|Wed|Thu|Fri|Sat|Sun)", GB_IGNORE_CASE, "<DOW>");

                add("(January|February|March|April|May|June|July|August|September|October|November|December)", GB_IGNORE_CASE, "<Month>");
                add("(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)", GB_IGNORE_CASE, "<MON>");

                add("<MON>[ -][0-9]{4}",   GB_IGNORE_CASE, "<MON> <YEAR>");
                add("<Month>[ -][0-9]{4}", GB_IGNORE_CASE, "<Month> <YEAR>");

                add("<TIME>[ -][0-9]{4}",  GB_IGNORE_CASE, "<TIME> <YEAR>");

                add("<MON>[ -][0-9 ]?[0-9]",   GB_IGNORE_CASE, "<MON> <DAY>");
                add("<Month>[ -][0-9 ]?[0-9]", GB_IGNORE_CASE, "<Month> <DAY>");

                add("[0-9]{2}[ -]<MON>",   GB_IGNORE_CASE, "<DAY> <MON>");
                add("[0-9]{2}[ -]<Month>", GB_IGNORE_CASE, "<DAY> <Month>");

                add("<DAY>, [0-9]{4}", GB_IGNORE_CASE, "<DAY> <YEAR>");

                if (is_may) {
                    // 'May' does not differ between short/long monthname
                    // -> use less accurate tests in May
                    add("<Month>", GB_MIND_CASE, "<MON>");
                }

                break;
            }
            default: gb_assert(0); break;
        }
    }
    ~difflineMode() {
        for (int i = 0; i<count; ++i) {
            GBS_free_regexpr(reg[i]);
            reg[i] = NULL;
        }
    }

    const char *get_error() const { return error; }
    int get_count() const { return count; }

    void replaceAll(char*& str) const {
        for (int i = 0; i<count; ++i) {
            size_t      matchlen;
            const char *matched = GBS_regmatch_compiled(str, reg[i], &matchlen);

            if (matched) {
                char       *prefix = GB_strpartdup(str, matched-1);
                const char *suffix = matched+matchlen;

                freeset(str, GBS_global_string_copy("%s%s%s", prefix, replace[i], suffix));
                free(prefix);
            }
        }
    }
    void replaceAll(char*& str1, char*& str2) const { replaceAll(str1); replaceAll(str2); }
};

bool difflineMode::is_may = strstr(GB_date_string(), "May") != 0;

static void cutEOL(char *s) {
    char *lf      = strchr(s, '\n');
    if (lf) lf[0] = 0;
}

static bool test_accept_diff_lines(const char *line1, const char *line2, const difflineMode& mode) {
    if (*line1++ != '-') return false;
    if (*line2++ != '+') return false;

    char *dup1 = strdup(line1);
    char *dup2 = strdup(line2);

    cutEOL(dup1); // side-effect: accepts missing trailing newline
    cutEOL(dup2);

    mode.replaceAll(dup1, dup2);

    bool equalNow = strcmp(dup1, dup2) == 0;
#if defined(DEBUG)
    // printf("dup1='%s'\ndup2='%s'\n", dup1, dup2); // uncomment this line to trace replaces
#endif // DEBUG

    free(dup2);
    free(dup1);

    return equalNow;
}

class DiffLines {
    typedef std::list<std::string> Lines;
    typedef Lines::iterator        LinesIter;
    typedef Lines::const_iterator  LinesCIter;

    Lines added_lines;
    Lines deleted_lines;

    LinesIter added_last_checked;
    LinesIter deleted_last_checked;

    static LinesIter next(LinesIter iter) { advance(iter, 1); return iter; }
    static LinesIter last(Lines& lines) { return (++lines.rbegin()).base(); }

    void set_checked() {
        added_last_checked   = last(added_lines);
        deleted_last_checked = last(deleted_lines);
    }

public:
    DiffLines() { set_checked(); }

    bool add(const char *diffline) {
        bool did_add = true;
        switch (diffline[0]) {
            case '-': deleted_lines.push_back(diffline); break;
            case '+': added_lines.push_back(diffline); break;
            default : did_add = false; break;
        }
        // fputs(diffline, stdout); // uncomment to show all difflines
        return did_add;
    }

    int added() const  { return added_lines.size(); }
    int deleted() const  { return deleted_lines.size(); }

    void remove_accepted_lines(const difflineMode& mode) {
        LinesIter d    = next(deleted_last_checked);
        LinesIter dEnd = deleted_lines.end();
        LinesIter a    = next(added_last_checked);
        LinesIter aEnd = added_lines.end();


        while (d != dEnd && a != aEnd) {
            if (test_accept_diff_lines(d->c_str(), a->c_str(), mode)) {
                d = deleted_lines.erase(d);
                a = added_lines.erase(a);
            }
            else {
                ++d;
                ++a;
            }
        }
        set_checked();
    }

    void print_from(FILE *out, LinesCIter a, LinesCIter d) const {
        LinesCIter dEnd = deleted_lines.end();
        LinesCIter aEnd = added_lines.end();

        while (d != dEnd && a != aEnd) {
            fputs(d->c_str(), out); ++d;
            fputs(a->c_str(), out); ++a;
        }
        while (d != dEnd) { fputs(d->c_str(), out); ++d; }
        while (a != aEnd) { fputs(a->c_str(), out); ++a; }
    }
    
    void print(FILE *out) const {
        LinesCIter d = deleted_lines.begin();
        LinesCIter a = added_lines.begin();
        print_from(out, a, d);
    }
};


bool GB_test_textfile_difflines(const char *file1, const char *file2, int expected_difflines, int special_mode) {
    // special_mode: 0 = none, 1 = accept date and time changes as equal
    const char *error   = NULL;

    if      (!GB_is_regularfile(file1)) error = GBS_global_string("No such file '%s'", file1);
    else if (!GB_is_regularfile(file2)) error = GBS_global_string("No such file '%s'", file2);
    else {
        char *cmd     = GBS_global_string_copy("/usr/bin/diff --unified %s %s", file1, file2);
        FILE *diffout = popen(cmd, "r");

        if (diffout) {
#define BUFSIZE 5000
            char         *buffer = (char*)malloc(BUFSIZE);
            bool          inHunk = false;
            DiffLines     diff_lines;
            difflineMode  mode(special_mode);

            TEST_EXPECT_NO_ERROR(mode.get_error());

            while (!feof(diffout)) {
                char *line = fgets(buffer, BUFSIZE, diffout);
                if (!line) break;

#if defined(ASSERTION_USED)
                size_t len = strlen(line);
                gb_assert(line && len<(BUFSIZE-1)); // increase BUFSIZE
#endif

                bool remove_now = true;
                if (strncmp(line, "@@", 2) == 0) inHunk = true;
                else if (!inHunk && strncmp(line, "Index: ", 7) == 0) inHunk = false;
                else if (inHunk) {
                    remove_now = !diff_lines.add(line);
                }

                if (remove_now) diff_lines.remove_accepted_lines(mode);
            }

            diff_lines.remove_accepted_lines(mode);

            int added   = diff_lines.added();
            int deleted = diff_lines.deleted();

            if (added != deleted) {
                error = GBS_global_string("added lines (=%i) differ from deleted lines(=%i)", added, deleted);
            }
            else if (added != expected_difflines) {
                error = GBS_global_string("files differ in %i lines (expected=%i)", added, expected_difflines);
            }
            if (error) {
                fputs("Different lines:\n", stdout);
                diff_lines.print(stdout);
                fputc('\n', stdout);
            }

            free(buffer);
            IF_ASSERTION_USED(int err =) pclose(diffout);
            gb_assert(err != -1);
#undef BUFSIZE
        }
        else {
            error = GBS_global_string("failed to run diff (%s)", cmd);
        }
        free(cmd);
    }
    // return result;
    if (error) printf("GB_test_textfile_difflines(%s, %s) fails: %s\n", file1, file2, error);
    return !error;
}

size_t GB_test_mem_equal(const unsigned char *buf1, const unsigned char *buf2, size_t common) { // used by unit tests
    size_t equal_bytes;
    if (memcmp(buf1, buf2, common) == 0) {
        equal_bytes = common;
    }
    else {
        equal_bytes = 0;
        size_t x    = 0;    // position inside memory
        while (buf1[x] == buf2[x]) {
            x++;
            equal_bytes++;
        }

        const size_t DUMP       = 7;
        size_t       y1         = x >= DUMP ? x-DUMP : 0;
        size_t       y2         = (x+DUMP)>common ? common : (x+DUMP);
        size_t       blockstart = equal_bytes-x;

        for (size_t y = y1; y <= y2; y++) {
            fprintf(stderr, "[0x%04zx]", blockstart+y);
            arb_test::print_pair(buf1[y], buf2[y]);
            fputc(' ', stderr);
            arb_test::print_hex_pair(buf1[y], buf2[y]);
            if (x == y) fputs("                     <- diff", stderr);
            fputc('\n', stderr);
        }
        if (y2 == common) {
            fputs("[end of block - truncated]\n", stderr);
        }
    }
    return equal_bytes;
}

bool GB_test_files_equal(const char *file1, const char *file2) {
    const char        *error = NULL;
    FILE              *fp1   = fopen(file1, "rb");

    if (!fp1) {
        printf("can't open '%s'\n", file1);
        error = "i/o error";
    }
    else {
        FILE *fp2 = fopen(file2, "rb");
        if (!fp2) {
            printf("can't open '%s'\n", file2);
            error = "i/o error";
        }
        else {
            const int      BLOCKSIZE    = 4096;
            unsigned char *buf1         = (unsigned char*)malloc(BLOCKSIZE);
            unsigned char *buf2         = (unsigned char*)malloc(BLOCKSIZE);
            int            equal_bytes  = 0;

            while (!error) {
                int    read1  = fread(buf1, 1, BLOCKSIZE, fp1);
                int    read2  = fread(buf2, 1, BLOCKSIZE, fp2);
                size_t common = read1<read2 ? read1 : read2;

                if (!common) {
                    if (read1 != read2) error = "filesize differs";
                    break;
                }

                size_t thiseq = GB_test_mem_equal(buf1, buf2, common);
                if (thiseq != common) {
                    error = "content differs";
                }
                equal_bytes += thiseq;
            }

            if (error) printf("test_files_equal: equal_bytes=%i\n", equal_bytes);
            gb_assert(error || equal_bytes); // comparing empty files is nonsense

            free(buf2);
            free(buf1);
            fclose(fp2);
        }
        fclose(fp1);
    }

    if (error) printf("test_files_equal(%s, %s) fails: %s\n", file1, file2, error);
    return !error;
}

void TEST_diff_files() {
    const char *file              = "diff/base.input";
    const char *file_swapped      = "diff/swapped.input";
    const char *file_date_swapped = "diff/date_swapped.input";
    const char *file_date_changed = "diff/date_changed.input";

    TEST_EXPECT(GB_test_textfile_difflines(file, file, 0, 0)); // check identity

    // check if swapped lines are detected properly
    TEST_EXPECT(GB_test_textfile_difflines(file, file_swapped, 1, 0));
    TEST_EXPECT(GB_test_textfile_difflines(file, file_swapped, 1, 1));
    TEST_EXPECT(GB_test_textfile_difflines(file, file_date_swapped, 3, 0));
    TEST_EXPECT(GB_test_textfile_difflines(file, file_date_swapped, 3, 1));

    TEST_EXPECT(GB_test_textfile_difflines(file, file_date_changed, 0, 1));
    TEST_EXPECT(GB_test_textfile_difflines(file, file_date_changed, 6, 0));
    
    TEST_EXPECT(GB_test_textfile_difflines(file_date_swapped, file_date_changed, 6, 0));
    TEST_EXPECT(GB_test_textfile_difflines(file_date_swapped, file_date_changed, 0, 1));
}

static char *remove_path(const char *fullname, void *cl_path) {
    const char *path = (const char *)cl_path;
    return strdup(fullname+(ARB_strBeginsWith(fullname, path) ? strlen(path) : 0));
}

static void GBT_transform_names(StrArray& dest, const StrArray& source, char *transform(const char *, void *), void *client_data) {
    for (int i = 0; source[i]; ++i) dest.put(transform(source[i], client_data));
}

#define TEST_JOINED_DIR_CONTENT_EQUALS(subdir,mask,expected) do {       \
        char     *fulldir = strdup(GB_path_in_ARBHOME(subdir));         \
        StrArray  contents;                                             \
        GBS_read_dir(contents, fulldir, mask);                          \
        StrArray  contents_no_path;                                     \
        GBT_transform_names(contents_no_path, contents, remove_path, (void*)fulldir); \
        char     *joined  = GBT_join_names(contents_no_path, '!');      \
        TEST_EXPECT_EQUAL(joined, expected);                            \
        free(joined);                                                   \
        free(fulldir);                                                  \
    } while(0)

void TEST_GBS_read_dir() {
    TEST_JOINED_DIR_CONTENT_EQUALS("GDE/CLUSTAL", "*.c",       "/amenu.c!/clustalv.c!/gcgcheck.c!/myers.c!/sequence.c!/showpair.c!/trees.c!/upgma.c!/util.c");
    TEST_JOINED_DIR_CONTENT_EQUALS("GDE/CLUSTAL", "/s.*\\.c/", "/clustalv.c!/myers.c!/sequence.c!/showpair.c!/trees.c");
    TEST_JOINED_DIR_CONTENT_EQUALS("GDE",         NULL,        "/Makefile!/README");
    TEST_JOINED_DIR_CONTENT_EQUALS("GDE",         "*",         "/Makefile!/README");
}

void TEST_find_file() {
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
}

// --------------------------------------------------------------------------------
// tests for global code included from central ARB headers (located in $ARBHOME/TEMPLATES)
#if defined(WARN_TODO)
#warning move to ARB_CORE library
#endif

// gcc reports: "warning: logical 'or' of collectively exhaustive tests is always true"
// for 'implicated(any, any)'. True, obviously. Nevertheless annoying.
#pragma GCC diagnostic ignored "-Wlogical-op"

void TEST_logic() {
#define FOR_ANY_BOOL(name) for (int name = 0; name<2; ++name)

    TEST_EXPECT(implicated(true, true));
    // for (int any = 0; any<2; ++any) {
    FOR_ANY_BOOL(any) {
        TEST_EXPECT(implicated(false, any)); // "..aus Falschem folgt Beliebiges.."
        TEST_EXPECT(implicated(any, any));

        TEST_EXPECT(correlated(any, any));
        TEST_REJECT(correlated(any, !any));
        TEST_REJECT(contradicted(any, any));
        TEST_EXPECT(contradicted(any, !any));
    }

    TEST_EXPECT(correlated(false, false));

    TEST_EXPECT(contradicted(true, false));
    TEST_EXPECT(contradicted(false, true));

    FOR_ANY_BOOL(kermitIsFrog) {
        FOR_ANY_BOOL(kermitIsGreen) {
            bool allFrogsAreGreen  = implicated(kermitIsFrog, kermitIsGreen);
            bool onlyFrogsAreGreen = implicated(kermitIsGreen, kermitIsFrog);

            TEST_EXPECT(implicated( kermitIsFrog  && allFrogsAreGreen,  kermitIsGreen));
            TEST_EXPECT(implicated(!kermitIsGreen && allFrogsAreGreen, !kermitIsFrog));

            TEST_EXPECT(implicated( kermitIsGreen && onlyFrogsAreGreen,  kermitIsFrog));
            TEST_EXPECT(implicated(!kermitIsFrog  && onlyFrogsAreGreen, !kermitIsGreen));

            TEST_EXPECT(implicated(kermitIsFrog  && !kermitIsGreen, !allFrogsAreGreen));
            TEST_EXPECT(implicated(kermitIsGreen && !kermitIsFrog,  !onlyFrogsAreGreen));

            TEST_EXPECT(correlated(
                            correlated(kermitIsGreen, kermitIsFrog), 
                            allFrogsAreGreen && onlyFrogsAreGreen
                            ));
        }
    }
}


#endif // UNIT_TESTS
