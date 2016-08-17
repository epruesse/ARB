// ================================================================= //
//                                                                   //
//   File      : arb_diff.cxx                                        //
//   Purpose   : code to compare/diff files                          //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2013   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

// AISC_MKPT_PROMOTE:#ifndef _GLIBCXX_CSTDLIB
// AISC_MKPT_PROMOTE:#include <cstdlib>
// AISC_MKPT_PROMOTE:#endif

#include "arb_diff.h"
#include "arb_match.h"
#include "arb_string.h"
#include "arb_msg.h"
#include "arb_file.h"

#include <arb_str.h>
#include <arbtools.h>
#include <smartptr.h>

#include <list>
#include <string>

#define MAX_REGS 13

class difflineMode : virtual Noncopyable {
    int mode;

    GBS_regex  *reg[MAX_REGS];
    bool        wordsOnly[MAX_REGS]; // only match if regexpr hits a word
    const char *replace[MAX_REGS];

    int              count;
    mutable GB_ERROR error;

    void add(bool onlyWords, const char *regEx, GB_CASE case_flag, const char *replacement) {
        if (!error) {
            arb_assert(count<MAX_REGS);
            reg[count] = GBS_compile_regexpr(regEx, case_flag, &error);
            if (!error) {
                replace[count]   = replacement;
                wordsOnly[count] = onlyWords;
                count++;
            }
        }
    }

    static bool is_word_char(char c) { return isalnum(c) || c == '_'; } // matches posix word def

    typedef SmartCustomPtr(GBS_regex, GBS_free_regexpr) GBS_regexPtr;

    mutable bool        may_involved;
    static GBS_regexPtr contains_May;

    void avoid_may_problems(const char *str) const {
        if (!may_involved) {
            if (GBS_regmatch_compiled(str, &*contains_May, NULL)) {
                // 'May' does not differ between short/long monthname
                // -> use less accurate tests in May
                fprintf(stderr, "Loosening month comparison: 'May' involved in '%s'\n", str);
                const_cast<difflineMode*>(this)->add(false, "<Month>", GB_MIND_CASE, "<MON>");
                may_involved = true;
            }
        }
    }

public:
    difflineMode(int mode_)
        : mode(mode_),
          count(0),
          error(NULL),
          may_involved(false)
    {
        memset(reg, 0, sizeof(reg));
        switch (mode) {
            case 0: break;
            case 1:  {
                add(false, "[0-9]{2}:[0-9]{2}:[0-9]{2}", GB_MIND_CASE, "<TIME>");
                add(true, "(Mon|Tue|Wed|Thu|Fri|Sat|Sun)", GB_IGNORE_CASE, "<DOW>");

                add(true, "(January|February|March|April|May|June|July|August|September|October|November|December)", GB_IGNORE_CASE, "<Month>");
                add(true, "(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)", GB_IGNORE_CASE, "<MON>");

                add(false, "<MON>[ -][0-9]{4}",   GB_IGNORE_CASE, "<MON> <YEAR>");
                add(false, "<Month>[ -][0-9]{4}", GB_IGNORE_CASE, "<Month> <YEAR>");

                add(false, "<TIME>[ -][0-9]{4}",  GB_IGNORE_CASE, "<TIME> <YEAR>");

                add(false, "<MON>[ -][0-9 ]?[0-9]",   GB_IGNORE_CASE, "<MON> <DAY>");
                add(false, "<Month>[ -][0-9 ]?[0-9]", GB_IGNORE_CASE, "<Month> <DAY>");

                add(false, "[0-9]{2}[ -\\.]+<MON>",   GB_IGNORE_CASE, "<DAY> <MON>");
                add(false, "[0-9]{2}[ -\\.]+<Month>", GB_IGNORE_CASE, "<DAY> <Month>");

                add(false, "<DAY>, [0-9]{4}", GB_IGNORE_CASE, "<DAY> <YEAR>");

                break;
            }
            default: arb_assert(0); break;
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

private:
    void replaceAll(char*& str) const {
        for (int i = 0; i<count; ++i) {
            size_t      matchlen;
            const char *matched = GBS_regmatch_compiled(str, reg[i], &matchlen);

            if (matched) {
                char       *prefix = ARB_strpartdup(str, matched-1);
                const char *suffix = matched+matchlen;

                bool do_repl = true;
                if (wordsOnly[i]) {
                    if      (prefix[0] != 0 && is_word_char(matched[-1])) do_repl = false;
                    else if (suffix[0] != 0 && is_word_char(suffix[0]))   do_repl = false;
                }

                if (do_repl) freeset(str, GBS_global_string_copy("%s%s%s", prefix, replace[i], suffix));

                free(prefix);
            }
        }
    }
public:
    void replaceAll(char*& str1, char*& str2) const {
        avoid_may_problems(str1);
        avoid_may_problems(str2);
        replaceAll(str1);
        replaceAll(str2);
    }
};

static GB_ERROR            static_error               = NULL;
difflineMode::GBS_regexPtr difflineMode::contains_May = GBS_compile_regexpr("May", GB_IGNORE_CASE, &static_error);

static void cutEOL(char *s) {
    char *lf      = strchr(s, '\n');
    if (lf) lf[0] = 0;
}

static bool test_accept_diff_lines(const char *line1, const char *line2, const difflineMode& mode) {
    if (*line1++ != '-') return false;
    if (*line2++ != '+') return false;

    char *dup1 = ARB_strdup(line1);
    char *dup2 = ARB_strdup(line2);

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

bool ARB_textfiles_have_difflines(const char *file1, const char *file2, int expected_difflines, int special_mode) {
    // special_mode: 0 = none, 1 = accept date and time changes as equal
    //
    // Warning: also returns true if comparing two EQUAL binary files.
    //          But it always fails if one file is binary and files differ

    const char *error   = NULL;

    if      (!GB_is_regularfile(file1)) error = GBS_global_string("No such file '%s'", file1);
    else if (!GB_is_regularfile(file2)) error = GBS_global_string("No such file '%s'", file2);
    else {
        char *cmd     = GBS_global_string_copy("/usr/bin/diff --unified %s %s", file1, file2);
        FILE *diffout = popen(cmd, "r");

        if (diffout) {
#define BUFSIZE 5000
            char         *buffer = ARB_alloc<char>(BUFSIZE);
            bool          inHunk = false;
            DiffLines     diff_lines;
            difflineMode  mode(special_mode);

            // TEST_EXPECT_NO_ERROR(mode.get_error());
            error = mode.get_error();

            while (!error && !feof(diffout)) {
                char *line = fgets(buffer, BUFSIZE, diffout);
                if (!line) break;

#if defined(ASSERTION_USED)
                size_t len = strlen(line);
                arb_assert(line && len<(BUFSIZE-1)); // increase BUFSIZE
#endif

                if (ARB_strBeginsWith(line, "Binary files")) {
                    if (strstr(line, "differ")) {
                        error = "attempt to compare binary files";
                    }
                }
                else {
                    bool remove_now = true;
                    if (strncmp(line, "@@", 2) == 0) inHunk = true;
                    else if (!inHunk && strncmp(line, "Index: ", 7) == 0) inHunk = false;
                    else if (inHunk) {
                        remove_now = !diff_lines.add(line);
                    }

                    if (remove_now) diff_lines.remove_accepted_lines(mode);
                }
            }

            if (!error) {
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
            }
            free(buffer);
            IF_ASSERTION_USED(int err =) pclose(diffout);
            arb_assert(err != -1);
#undef BUFSIZE
        }
        else {
            error = GBS_global_string("failed to run diff (%s)", cmd);
        }
        free(cmd);
    }
    // return result;
    if (error) printf("ARB_textfiles_have_difflines(%s, %s) fails: %s\n", file1, file2, error);
    return !error;
}

#ifdef UNIT_TESTS
#include <test_unit.h>

size_t ARB_test_mem_equal(const unsigned char *buf1, const unsigned char *buf2, size_t common, size_t blockStartAddress) {
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
        size_t       y2         = (x+2*DUMP)>common ? common : (x+2*DUMP);
        size_t       blockstart = blockStartAddress+equal_bytes-x;

        for (size_t y = y1; y <= y2; y++) {
            fprintf(stderr, "[0x%04zx]", blockstart+y);
            arb_test::print_pair(buf1[y], buf2[y]);
            fputc(' ', stderr);
            arb_test::print_hex_pair(buf1[y], buf2[y]);
            if (buf1[y] != buf2[y]) fputs("                     <- diff", stderr);
            fputc('\n', stderr);
        }
        if (y2 == common) {
            fputs("[end of block - truncated]\n", stderr);
        }
    }
    return equal_bytes;
}

bool ARB_files_are_equal(const char *file1, const char *file2) {
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
            const int      BLOCKSIZE   = 4096;
            unsigned char *buf1        = ARB_alloc<unsigned char>(BLOCKSIZE);
            unsigned char *buf2        = ARB_alloc<unsigned char>(BLOCKSIZE);
            size_t         equal_bytes = 0;

            while (!error) {
                int    read1  = fread(buf1, 1, BLOCKSIZE, fp1);
                int    read2  = fread(buf2, 1, BLOCKSIZE, fp2);
                size_t common = read1<read2 ? read1 : read2;

                if (!common) {
                    if (read1 != read2) error = "filesize differs";
                    break;
                }

                size_t thiseq = ARB_test_mem_equal(buf1, buf2, common, equal_bytes);
                if (thiseq != common) {
                    error = "content differs";
                }
                equal_bytes += thiseq;
            }

            if (error) printf("files_are_equal: equal_bytes=%zu\n", equal_bytes);
            arb_assert(error || equal_bytes); // comparing empty files is nonsense

            free(buf2);
            free(buf1);
            fclose(fp2);
        }
        fclose(fp1);
    }

    if (error) printf("files_are_equal(%s, %s) fails: %s\n", file1, file2, error);
    return !error;
}

void TEST_diff_files() {
    // files are in ../UNIT_TESTER/run/diff
    const char *file              = "diff/base.input";
    const char *file_swapped      = "diff/swapped.input";
    const char *file_date_swapped = "diff/date_swapped.input";
    const char *file_date_changed = "diff/date_changed.input";

    TEST_EXPECT(ARB_textfiles_have_difflines(file, file, 0, 0)); // check identity

    // check if swapped lines are detected properly
    TEST_EXPECT(ARB_textfiles_have_difflines(file, file_swapped, 1, 0));
    TEST_EXPECT(ARB_textfiles_have_difflines(file, file_swapped, 1, 1));
    TEST_EXPECT(ARB_textfiles_have_difflines(file, file_date_swapped, 3, 0));
    TEST_EXPECT(ARB_textfiles_have_difflines(file, file_date_swapped, 3, 1));

    TEST_EXPECT(ARB_textfiles_have_difflines(file, file_date_changed, 0, 1));
    TEST_EXPECT(ARB_textfiles_have_difflines(file, file_date_changed, 6, 0));

    TEST_EXPECT(ARB_textfiles_have_difflines(file_date_swapped, file_date_changed, 6, 0));
    TEST_EXPECT(ARB_textfiles_have_difflines(file_date_swapped, file_date_changed, 0, 1));

    const char *binary  = "TEST_gpt.arb.expected";    // a binary arb DB
    const char *binary2 = "TEST_gpt.arb.pt.expected"; // a ptserver index
    const char *text    = file;

    // diff between text and binary should fail
    TEST_REJECT(ARB_textfiles_have_difflines(text,    binary,  0, 0));
    TEST_REJECT(ARB_textfiles_have_difflines(binary,  text,    0, 0));
    TEST_REJECT(ARB_textfiles_have_difflines(binary2, text,    0, 0));
    TEST_REJECT(ARB_textfiles_have_difflines(text,    binary2, 0, 0));

    // diff between two binaries shall fails as well ..
    TEST_REJECT(ARB_textfiles_have_difflines(binary,  binary2, 0, 0));
    TEST_REJECT(ARB_textfiles_have_difflines(binary2, binary,  0, 0));

    // when files do not differ, ARB_textfiles_have_difflines always
    // returns true - even if the files are binary
    // (unwanted but accepted behavior)
    TEST_EXPECT(ARB_textfiles_have_difflines(binary,  binary,  0, 0));
    TEST_EXPECT(ARB_textfiles_have_difflines(binary2, binary2, 0, 0));
    TEST_EXPECT(ARB_textfiles_have_difflines(text,    text,    0, 0));
}

// --------------------------------------------------------------------------------
// tests for global code included from central ARB headers (located in $ARBHOME/TEMPLATES)

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

#include <cmath>
// #include <math.h>

void TEST_naninf() {
    double num  = 3.1415;
    double zero = num-num;
    double inf  = num/zero;
    double ninf = -inf;
    double nan  = 0*inf;

    TEST_EXPECT_DIFFERENT(inf, ninf);
    TEST_EXPECT_EQUAL(ninf, -1.0/zero);

    // decribe how isnan, isinf and isnormal shall behave:
#define TEST_ISINF(isinf_fun)           \
    TEST_EXPECT(isinf_fun(inf));        \
    TEST_EXPECT(!!isinf_fun(ninf));     \
    TEST_EXPECT(isinf_fun(INFINITY));   \
    TEST_EXPECT(!isinf_fun(nan));       \
    TEST_EXPECT(!isinf_fun(NAN));       \
    TEST_EXPECT(!isinf_fun(num));       \
    TEST_EXPECT(!isinf_fun(zero))

#define TEST_ISNAN(isnan_fun)           \
    TEST_EXPECT(isnan_fun(nan));        \
    TEST_EXPECT(isnan_fun(NAN));        \
    TEST_EXPECT(!isnan_fun(inf));       \
    TEST_EXPECT(!isnan_fun(ninf));      \
    TEST_EXPECT(!isnan_fun(INFINITY));  \
    TEST_EXPECT(!isnan_fun(zero));  \
    TEST_EXPECT(!isnan_fun(num))

#define TEST_ISNORMAL(isnormal_fun)             \
    TEST_EXPECT(isnormal_fun(num));             \
    TEST_EXPECT(!isnormal_fun(zero));           \
    TEST_EXPECT(!isnormal_fun(inf));            \
    TEST_EXPECT(!isnormal_fun(ninf));           \
    TEST_EXPECT(!isnormal_fun(nan));            \
    TEST_EXPECT(!isnormal_fun(INFINITY));       \
    TEST_EXPECT(!isnormal_fun(NAN))

    // check behavior of arb templates:
    TEST_ISNAN(is_nan);
    TEST_ISINF(is_inf);
    TEST_ISNORMAL(is_normal);

#if (GCC_PATCHLEVEL_CODE < 50301)
    // (section leads to compile error with gcc 5.3.1 - suggesting std::isnan and std::isinf)
    // document behavior of math.h macros:
    TEST_ISNAN(isnan);
# if (GCC_PATCHLEVEL_CODE >= 40403 && GCC_PATCHLEVEL_CODE <= 40407 && defined(DEBUG))
    // TEST_ISINF(isinf); // isinf macro is broken (gcc 4.4.3 up to gcc 4.4.7, if compiled with DEBUG)
    TEST_EXPECT__BROKEN(isinf(ninf)); // broken; contradicts http://www.cplusplus.com/reference/cmath/isinf/
# else
    TEST_ISINF(isinf);
# endif
    // TEST_ISNORMAL(isnormal); // ok if <math.h> included,  compile error if <cmath> included
#endif

    // check behavior of std-versions:
    TEST_ISNAN(std::isnan);
    TEST_ISINF(std::isinf);
    TEST_ISNORMAL(std::isnormal);
}

#endif // UNIT_TESTS
