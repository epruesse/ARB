#include <BufferedFileReader.h>
#include "fun.h"
#include "defs.h"
#include "global.h"
#include "reader.h"

#include <cstdarg>
#include <cerrno>

bool scan_token(char *to, const char *from) { // __ATTR__USERESULT
    return sscanf(from, "%s", to) == 1;
}

void scan_token_or_die(char *to, const char *from) {
    if (!scan_token(to, from)) {
        throw_error(88, "expected to see a token here");
    }
}
void scan_token_or_die(char *to, Reader& reader, int offset) {
    scan_token_or_die(to, reader.line()+offset);
}

void throw_error(int error_num, const char *error_message) { // __ATTR__NORETURN
    throw Convaln_exception(error_num, error_message);
}

char *strf(const char *format, ...) { // __ATTR__FORMAT(1)
    va_list parg;
    va_start(parg, format);

    const int BUFSIZE = 1000;
    char      buffer[BUFSIZE];
    int       printed = vsprintf(buffer, format, parg);
    ca_assert(printed <= BUFSIZE);

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

// --------------------------------------------------------------------------------

bool Warnings::show_warnings = true;

void warning(int warning_num, const char *warning_message) {
    // print out warning_message and continue execution.
    if (Warnings::shown())
        fprintf(stderr, "WARNING(%d): %s\n", warning_num, warning_message);
}
void warningf(int warning_num, const char *warning_messagef, ...) { // __ATTR__FORMAT(2)
    if (Warnings::shown()) {
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

int Skip_white_space(const char *line, int index) {
    // Skip white space from (index)th char of Str line.

    while (line[index] == ' ' || line[index] == '\t')
        ++index;
    return (index);
}

void Getstr(char *line, int linenum) {
    // Get input Str from terminal.
    char c;
    int  indi = 0;

    for (; (c = getchar()) != '\n' && indi < (linenum - 1); line[indi++] = c) {}

    line[indi] = '\0';
}

inline void append_known_len(char*& string1, int len1, const char *string2, int len2) {
    ca_assert(len2); // else no need to call, string1 already correct
    int newlen = len1+len2;
    ARB_realloc(string1, newlen+1);
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
    int len2 = str0len(string2);
    if (len2) append_known_len(string1, len1, string2, len2);
    else { string1[len1] = 0; }
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
void Append(char*& string1, char ch) {
    append_known_len(string1, str0len(string1), &ch, 1);
}

void upcase(char *str) {
    // Capitalize all char in the str.
    if (!str) return;
    for (int i = 0; str[i]; ++i) str[i] = toupper(str[i]);
}

int fputs_len(const char *str, int len, Writer& write) {
    // like fputs(), but does not print more than 'len' characters
    // returns number of chars written or throws write error
    for (int i = 0; i<len; ++i) {
        if (!str[i]) return i;
        write.out(str[i]);
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
    // search 'str' for the occurrence of any 'pattern'
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

int ___lookup_keyword(const char *keyword, const char * const *lookup_table, int lookup_table_size) {
    // returns the index [0..n-1] of 'keyword' in lookup_table
    // or -1 if not found.

    for (int i = 0; i<lookup_table_size; ++i) {
        if (str_equal(keyword, lookup_table[i])) return i;
    }
    return -1;
}

int parse_key_word(const char *line, char *key, const char *separator) {
    // Copy keyword starting at position 'index' of 'line' delimited by 'separator' into 'key'.
    // Do not copy more than 'TOKENSIZE-1' characters.
    // Returns length of keyword.

    int k = 0;
    if (line) {
        while (k<(TOKENSIZE-1) && line[k] && !occurs_in(line[k], separator)) {
            key[k] = line[k];
            ++k;
        }
    }
    key[k] = 0;
    return k;
}

// ----------------------
//      FormattedFile

FormattedFile::FormattedFile(const char *Name, Format Type)
    : name_(strdup(Name)),
      type_(Type)
{}
FormattedFile::~FormattedFile() {
    free(name_);
}

void FormattedFile::init(const char *Name, Format Type) {
    ca_assert(!name_); // do not init twice
    ca_assert(Name);
    ca_assert(Type != UNKNOWN);

    name_ = nulldup(Name);
    type_ = Type;
}

