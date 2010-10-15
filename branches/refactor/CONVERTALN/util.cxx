#include <FileBuffer.h>
#include "fun.h"
#include "defs.h"
#include "global.h"

#include <cstdarg>
#include <cerrno>


bool scan_token(const char *from, char *to) { // __ATTR__USERESULT
    return sscanf(from, "%s", to) == 1;
}

void scan_token_or_die(const char *from, char *to, FILE_BUFFER *fb) {
    if (!scan_token(from, to)) {
        const char *line_error;
        if (fb) {
            line_error = FILE_BUFFER_make_error(*fb, true, "expected a token, but found none");
        }
        else {
            line_error = "expected a token, but found none (source location unknown)";
        }

#if defined(ASSERTION_USED)
        fputs(line_error, stderr);
        fputc('\n', stderr);
        ca_assert(0);
#endif // ASSERTION_USED
        throw_error(88, line_error);
    }
}

void throw_error(int error_num, const char *error_message) { // __ATTR__NORETURN
    throw Convaln_exception(error_num, error_message);
}

char *strf(const char *format, ...) { // __ATTR__FORMAT(1)
    va_list parg;
    va_start(parg, format);
    char    buffer[LINESIZE];
    int     printed = vsprintf(buffer, format, parg);
    ca_assert(printed <= LINESIZE);
    va_end(parg);

    return strndup(buffer, printed);
}

void throw_errorf(int error_num, const char *error_messagef, ...) { // __ATTR__FORMAT(2) __ATTR__NORETURN
    va_list parg;
    va_start(parg, error_messagef);

    const int   BUFSIZE = 1000;
    static char buffer[BUFSIZE];
    int         printed = vsprintf(buffer, error_messagef, parg);

    va_end(parg);

    if (printed >= BUFSIZE) {
        throw_errorf(998, "Internal buffer overflow (while formatting error #%i '%s')", error_num, error_messagef);
    }
    throw_error(error_num, buffer);
}

void throw_cant_open_input(const char *filename) {
    throw_errorf(1, "can't read input file '%s' (Reason: %s)", filename, strerror(errno));
}
void throw_cant_open_output(const char *filename) {
    throw_errorf(2, "can't write output file '%s' (Reason: %s)", filename, strerror(errno));
}

FILE *open_input_or_die(const char *filename) {
    FILE *in = fopen(filename, "rt");
    if (!in) throw_cant_open_input(filename);
    return in;
}
FILE *open_output_or_die(const char *filename) {
    FILE *out;
    if (!filename[0]) out = stdout; // empty filename -> stdout
    else {
        out = fopen(filename, "wt");
        if (!out) throw_cant_open_output(filename);
    }
    return out;
}

/* --------------------------------------------------------------
 *  Function warning()
 *      print out warning_message and continue execution.
 */
// ------------------------------------------------------------- 

int warning_out = 1;

void warning(int warning_num, const char *warning_message) {
    if (warning_out)
        fprintf(stderr, "WARNING(%d): %s\n", warning_num, warning_message);
}
void warningf(int warning_num, const char *warning_messagef, ...) { // __ATTR__FORMAT(2)
    if (warning_out) {
        va_list parg;
        va_start(parg, warning_messagef);

        const int   BUFSIZE = 1000;
        static char buffer[BUFSIZE];
        int         printed = vsprintf(buffer, warning_messagef, parg);

        va_end(parg);

        if (printed >= BUFSIZE) {
            throw_errorf(997, "Internal buffer overflow (while formatting warning #%i '%s')", warning_num, warning_messagef);
        }
        warning(warning_num, buffer);
    }
}

/* ------------------------------------------------------------
 *   Function Reallocspace().
 *       Realloc a continue space, expand or shrink the
 *       original space.
 */
char *Reallocspace(void *block, unsigned int size) {
    char *temp;

    if (size <= 0) {
        free(block);
        return NULL;
    }
    
    if (block == NULL) {
        temp = (char *)calloc(1, size);
    }
    else {
        temp = (char *)realloc(block, size);
    }
    if (!temp) throw_error(999, "Run out of memory (Reallocspace)");
    return temp;
}

/* ---------------------------------------------------------
 *   Function Skip_white_space().
 *       Skip white space from (index)th char of Str line.
 */
int Skip_white_space(const char *line, int index) {
    // skip white space 

    while (line[index] == ' ' || line[index] == '\t')
        ++index;
    return (index);
}

/* ----------------------------------------------------------------
 *   Function Reach_white_space().
 *       Reach the next available white space of Str line.
 */
int Reach_white_space(char *line, int index) {
    int length;

    length = str0len(line);

    // skip to white space 
    for (; line[index] != ' ' && line[index] != '\t' && index < length; index++) {}

    return (index);
}

/* ---------------------------------------------------------------
 *   Function Fgetline().
 *       Get a line from assigned file, also checking for buffer
 *           overflow.
 */

char *Fgetline(char *line, size_t maxread, FILE_BUFFER fb) {
    size_t len;

    const char *fullLine = FILE_BUFFER_read(fb, &len);

    if (!fullLine)
        return 0;

    if (len <= (maxread - 2)) {
        memcpy(line, fullLine, len);
        line[len] = '\n';
        line[len + 1] = 0;
        return line;
    }

    fprintf(stderr, "Error(148): OVERFLOW LINE: %s\n", fullLine);
    return 0;
}

/* ------------------------------------------------------------
 *   Function Getstr().
 *       Get input Str from terminal.
 */
void Getstr(char *line, int linenum) {
    char c;
    int indi = 0;

    for (; (c = getchar()) != '\n' && indi < (linenum - 1); line[indi++] = c) {}

    line[indi] = '\0';
}

inline void append_known_len(char*& string1, int len1, const char *string2, int len2) {
    ca_assert(len2); // else no need to call, string1 already correct 
    int newlen      = len1+len2;
    string1         = Reallocspace(string1, newlen+1);
    memcpy(string1+len1, string2, len2);
    string1[newlen] = 0;
}

void terminate_with(char*& str, char ch) {
    // append 'ch' to end of 'str' (before \n)
    // - if it's not already there and
    // - 'str' contains more than just '\n'

    int len = str0len(str);
    if (!len) return; 
    ca_assert(str[len-1] == '\n');
    if (len == 1) return;
    if (str[len-2] == ch) return;

    char temp[] = { ch, '\n' };
    append_known_len(str, len-1, temp, 2);
}

void skip_eolnl_and_append(char*& string1, const char *string2) {
    int len1 = str0len(string1);
    if (len1 && string1[len1-1] == '\n') len1--;
    append_known_len(string1, len1, string2, str0len(string2));
}

void skip_eolnl_and_append_spaced(char*& string1, const char *string2) {
    int len1 = str0len(string1);
    if (len1 && string1[len1-1] == '\n') string1[len1-1] = ' ';
    append_known_len(string1, len1, string2, str0len(string2));
}

void Append(char*& string1, const char *string2) {
    int len2 = str0len(string2);
    if (len2) append_known_len(string1, str0len(string1), string2, len2);
}



void upcase(char *str) {
    // Capitalize all char in the str.
    if (!str) return;
    for (int i = 0; str[i]; ++i) str[i] = toupper(str[i]);
}

void fputc_rep(char c, int repeat, FILE *fp) {
    for (int r = 0; r<repeat; ++r) {
        fputc(c, fp);
    }
}

int fputs_len(const char *str, int len, FILE *out) {
    // like fputs(), but does not print more than 'len' characters
    // returns number of chars written or EOF
    for (int i = 0; i<len; ++i) {
        if (!str[i]) return i;
        if (fputc(str[i], out) == EOF) return EOF;
    }
    return len;
}

// -------------------------
//      pattern matching

enum FindMode { FIND_START, FIND_SKIP_OVER };

static int findPattern(const char *text, const char *pattern, FindMode mode) {
    // Return offset of 'pattern' in 'text' or -1
    // (compares case insensitive)
    // return position after found 'pattern' in 'endPtr'

    if (text && pattern) {
        for (int t = 0; text[t]; ++t) {
            bool mismatch = false;

            int p = 0;
            for (; !mismatch && pattern[p]; ++p) {
                mismatch = tolower(text[t+p]) != tolower(pattern[p]);
            }
            if (!mismatch) {
                switch (mode) {
                    case FIND_START: return t;
                    case FIND_SKIP_OVER: return t+p;
                }
                ca_assert(0);
            }
        }
    }
    return -1;
}

static int findMultipattern(const char *str, const char** const& pattern, char expect_behind, FindMode mode) {
    // search 'str' for the occurrance of any 'pattern'
    // if 'expect_behind' != 0 -> expect that char behind the found pattern

    int offset = -1;

    if (str) {
        FindMode use = expect_behind ? FIND_SKIP_OVER : mode;
        
        int p;
        for (p = 0; offset == -1 && pattern[p]; ++p) {
            offset = findPattern(str, pattern[p], use);
        }

        if (offset != -1) {
            if (expect_behind) {
                if (str[offset] != expect_behind) { // mismatch
                    offset = findMultipattern(str+offset, pattern, expect_behind, mode);
                }
                else {
                    switch (mode) {
                        case FIND_START:
                            offset -= str0len(pattern[p]);
                            ca_assert(offset >= 0);
                            break;
                        case FIND_SKIP_OVER:
                            offset++;
                            break;
                    }
                }
            }
        }
    }
    return offset;
}

static int findSubspecies(const char *str, char expect_behind, FindMode mode) {
    const char *subspecies_pattern[] = { "subspecies", "sub-species", "subsp.", NULL };
    return findMultipattern(str, subspecies_pattern, expect_behind, mode);
}

static int findStrain(const char *str, char expect_behind, FindMode mode) {
    const char *strain_pattern[] = { "strain", "str.", NULL };
    return findMultipattern(str, strain_pattern, expect_behind, mode);
}

int find_pattern(const char *text, const char *pattern) { return findPattern(text, pattern, FIND_START); }
int skip_pattern(const char *text, const char *pattern) { return findPattern(text, pattern, FIND_SKIP_OVER); }

int find_subspecies(const char *str, char expect_behind) { return findSubspecies(str, expect_behind, FIND_START); }
int skip_subspecies(const char *str, char expect_behind) { return findSubspecies(str, expect_behind, FIND_SKIP_OVER); }

int find_strain(const char *str, char expect_behind) { return findStrain(str, expect_behind, FIND_START); }
int skip_strain(const char *str, char expect_behind) { return findStrain(str, expect_behind, FIND_SKIP_OVER); }

const char *stristr(const char *str, const char *substring) {
    int offset = find_pattern(str, substring);
    return offset >= 0 ? str+offset : NULL;
}


