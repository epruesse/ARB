#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include "convert.h"
#include "global.h"
#include "types.h"

int warning_out = 1;

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

/* --------------------------------------------------------------- *
 *   Function Freespace().
 *   Free the pointer if not NULL.
 */
void Freespace(void *pointer) {
    char **the_pointer = (char **)pointer;

    if ((*the_pointer) != NULL) {
        free((*the_pointer));
    }
    (*the_pointer) = NULL;
}

void throw_error(int error_num, const char *error_message) { // __ATTR__NORETURN
    throw Convaln_exception(error_num, error_message);
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
/* ------------------------------------------------------------- */

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
    char *temp, answer;

    if ((block == NULL && size <= 0) || size <= 0)
        return (NULL);
    if (block == NULL) {
        temp = (char *)calloc(1, size);
    }
    else {
        temp = (char *)realloc(block, size);
    }
    if (temp == NULL) {
        const char *message = "Run out of memory (Reallocspace)";
        warning(999, message);
        fprintf(stderr, "Are you converting to Paup or Phylip?(y/n) ");
        scanf("%c", &answer);
        if (answer == 'y') {
            return (NULL);
        }
        throw_error(999, message);
    }
    return (temp);
}

/* ---------------------------------------------------------
 *   Function Skip_white_space().
 *       Skip white space from (index)th char of Str line.
 */
int Skip_white_space(char *line, int index) {
    /* skip white space */

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

    /* skip to white space */
    for (; line[index] != ' ' && line[index] != '\t' && index < length; index++) ;

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

    for (; (c = getchar()) != '\n' && indi < (linenum - 1); line[indi++] = c) ;

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

int find_pattern(const char *text, const char *pattern) {
    // Return offset of 'pattern' in 'text' or -1
    // (compares case insensitive)

    if (!text || !pattern) return -1;

    for (int t = 0; text[t]; ++t) {
        bool mismatch = false;
        for (int p = 0; !mismatch && pattern[p]; ++p) {
            mismatch = tolower(text[t+p]) != tolower(pattern[p]);
        }
        if (!mismatch) return t;
    }
    return -1;
}

void upcase(char *str) {
    // Capitalize all char in the str.
    if (!str) return;
    for (int i = 0; str[i]; ++i) str[i] = toupper(str[i]);
}

