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

/* --------------------------------------------------------------- *
 *   Function Freespace().
 *   Free the pointer if not NULL.
 */
void Freespace(void *pointer)
{
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
char *Reallocspace(void *block, unsigned int size)
{
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

/* -------------------------------------------------------------------
 *   Function Dupstr().
 *       Duplicate string and return the address of string.
 */
char *Dupstr(const char *string)
{
    char     *temp, answer;
    unsigned  size;

    if (string == NULL)
        return (NULL);
    else {
        size = strlen(string);
        if ((temp = (char *)calloc(1, size + 1)) == NULL) {
            const char *message = "Run out of memory (Dupstr)";
            warning(888, message);
            fprintf(stderr, "Are you converting to Paup or Phylip?(y/n) ");
            scanf("%c", &answer);
            if (answer == 'y') {
                return (NULL);
            }
            throw_error(888, message);
        }
        strcpy(temp, string);
        return (temp);
    }
}

/* ------------------------------------------------------------
 *  Function Catstr().
 *      Append string s2 to string s1.
 */
char *Catstr(char *s1, const char *s2)
{
    return ((char *)strcat(s1, s2));
}

/* ------------------------------------------------------------
 *   Function Lenstr().
 *       Length of the string s1.
 */
int Lenstr(const char *s1)
{
    if (s1 == NULL)
        return (0);
    else
        return (strlen(s1));
}

/* --------------------------------------------------------
 *  Function Cpystr().
 *      Copy string s2 to string s1.
 */
void Cpystr(char *s1, const char *s2)
{
    strcpy(s1, s2);
}

/* ---------------------------------------------------------
 *   Function Skip_white_space().
 *       Skip white space from (index)th char of string line.
 */
int Skip_white_space(char *line, int index)
{
    /* skip white space */

    while (line[index] == ' ' || line[index] == '\t')
        ++index;
    return (index);
}

/* ----------------------------------------------------------------
 *   Function Reach_white_space().
 *       Reach the next available white space of string line.
 */
int Reach_white_space(char *line, int index)
{
    int length;

    length = Lenstr(line);

    /* skip to white space */
    for (; line[index] != ' ' && line[index] != '\t' && index < length; index++) ;

    return (index);
}

/* ---------------------------------------------------------------
 *   Function Fgetline().
 *       Get a line from assigned file, also checking for buffer
 *           overflow.
 */

char *Fgetline(char *line, size_t maxread, FILE_BUFFER fb)
{
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
 *       Get input string from terminal.
 */
void Getstr(char *line, int linenum)
{

    char c;

    int indi = 0;

    for (; (c = getchar()) != '\n' && indi < (linenum - 1); line[indi++] = c) ;

    line[indi] = '\0';
}

/* ----------------------------------------------------------------
 *   Function Append_char().
 *       Append a char before '\n' if there is no such char.
 *           (Assume there is '\n' at the end of string
 *           already and length of string except '\n' must
 *           greater than 1.)
 */
void Append_char(char **string, char ch)
{
    char temp[10];

    if (Lenstr((*string)) <= 0 || (*string)[0] == '\n')
        return;

    if ((*string)[Lenstr((*string)) - 2] != ch) {
        sprintf(temp, "%c\n", ch);
        Append_rm_eoln(string, temp);
    }
}

/* ----------------------------------------------------------------
 *  Function Append_rm_eoln().
 *      Remove the eoln of string1 and then append string2
 *      to string1.
 */
void Append_rm_eoln(char **string1, const char *string2)
{
    int length;

    length = Lenstr(*string1);

    if (((length > 1) && (*string1)[length - 1] == '\n') || (*string1)[0] == '\n')
        length--;

    (*string1) = (char *)Reallocspace((*string1), (unsigned int)(sizeof(char) * (Lenstr(string2) + length + 1)));

    (*string1)[length] = '\0';

    Catstr((*string1), string2);
}

/* ----------------------------------------------------------------
 *   Function Append_rp_eoln().
 *       Replace the eoln by a blank in string1 and then append
 *       string2 to string1.
 */
void Append_rp_eoln(char **string1, char *string2)
{
    int length;

    length = Lenstr(*string1);
    if ((length > 1) && (*string1)[length - 1] == '\n')
        (*string1)[length - 1] = ' ';
    (*string1) = (char *)Reallocspace((*string1), (unsigned int)(sizeof(char) * (Lenstr(string2) + Lenstr(*string1) + 1)));
    (*string1)[length] = '\0';
    Catstr((*string1), string2);
}

/* ----------------------------------------------------------------
 *   Function Append().
 *       Append string2 to string1.
 */
void Append(char **string1, const char *string2)
{
    int length;

    length = Lenstr(*string1);
    if (length == 0 && Lenstr(string2) == 0)
        return;

    (*string1) = (char *)Reallocspace((*string1), (unsigned int)(sizeof(char) * (Lenstr(string2) + Lenstr(*string1) + 1)));
    (*string1)[length] = '\0';

    Catstr((*string1), string2);
}

/* ----------------------------------------------------------
 *   Function find_pattern()
 *   Return TRUE if the desired pattern in the input text
 */
int find_pattern(const char *text, const char *pattern)
{
    int indi, index, indj;

    int stop;

    stop = Lenstr(text) - Lenstr(pattern) + 1;
    for (indi = 0; indi < stop; indi++) {
        index = -2;
        for (indj = 0; index == -2 && pattern[indj] != '\0'; indj++)
            if (same_char(text[indi + indj], pattern[indj]) == 0)
                index = -1;
        if (index == -2)
            return (indi);
        /* find PARTIALLY matched pattern */
    }
    return (-1);
}

/* --------------------------------------------------------------
 *   Function not_ending_mark().
 *       Return true if the char is not a '.' nor ';'.
 */
int not_ending_mark(char ch)
{
    return ((ch != '.' && ch != ';'));
}

/* ----------------------------------------------------------------
 *   Function last_word().
 *       return true if the char is not white space.
 */
int last_word(char ch)
{
    int is_lastword;

    switch (ch) {
        case ',':
        case '.':
        case ';':
        case ' ':
        case '?':
        case ':':
        case '!':
        case ')':
        case ']':
        case '}':
            is_lastword = 1;
            break;
        default:
            is_lastword = 0;
            break;
    }
    return (is_lastword);
}

/* ----------------------------------------------------------------
 *   Function is_separator().
 *       Return 1 if ch is a separator in string separators.
 */
int is_separator(char ch, const char *separators)
{
    int indi, len, index;

    for (indi = index = 0, len = Lenstr(separators); indi < len && index == 0; indi++)
        if (ch == separators[indi])
            index = 1;

    return (index);
}

/* ------------------------------------------------------------
 *   Function same_char()
 *   Return TRUE if two characters are the same.
 *   case insensitive or not is decided by buffer.charcase.
 */
int same_char(char ch1, char ch2)
{
    if (ch1 == ch2)
        return (1);
    if ((ch1 - 'a') >= 0)
        ch1 = ch1 - 'a' + 'A';
    if ((ch2 - 'a') >= 0)
        ch2 = ch2 - 'a' + 'A';
    if (ch1 == ch2)
        return (1);
    else
        return (0);
}

/* --------------------------------------------------------------
 *   Function Upper_case().
 *       Capitalize all char in the string.
 */
void Upper_case(char *string)
{
    int len, indi;

    len = Lenstr(string);
    for (indi = 0; indi < len; indi++) {
        if (string[indi] >= 'a' && string[indi] <= 'z')
            string[indi] = string[indi] - 'a' + 'A';
    }
}

/* ---------------------------------------------------------------
 *   Function Blank_num().
 *       Count the number of blanks at the beginning of
 *           the string.
 */
int Blank_num(char *string)
{
    int index;

    for (index = 0; string[index] == ' '; index++) ;

    return (index);
}
