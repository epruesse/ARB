/* lenstring.h --- interface to lenstring type.
   Jim Blandy <jimb@gnu.ai.mit.edu> --- September 1994 */


/* In order to process data which may contain null ('\0') characters,
   it's often helpful to use a representation of strings which stores
   the length separately from the data.  Like this:  */
typedef struct
  {
    char *text;
    int len;
  }
lenstring;


/* Write STRING to STREAM.
   Return value same as for stdio's fwrite.  */
extern size_t write_lenstring (lenstring *STRING, FILE *STREAM);


/* Write STRING to STREAM, left-justified in a field of FIELD spaces.
   If STRING is longer than FIELD, truncate it.  */
extern void display_clipped_lenstring (lenstring *string,
                       int field,
                       FILE *stream);

/* Read text from SOURCE until we find DELIMITER, or hit EOF.
   Set *STRING to the text we read; the delimiting string or EOF is
   not included in STRING.

   If the text was terminated by EOF, and is empty (i.e. its length is
   zero), set STRING->text to zero and return EOF.  In this case, the
   caller should not free STRING->text.

   Otherwise, the text is stored in memory obtained via malloc, and
   should be freed by the caller.

   If the string was terminated by DELIMITER, return 0.
   If the string was non-empty and terminated by EOF, return 1.
   If an error occurred reading the string, print an error message
   and exit.  */
extern int read_delimited_lenstring (lenstring *string, const char *delimiter, FILE *stream);

/* Search STRING for an occurrence of SUBSTRING starting not before
   START, and return its starting position, or -1 if SUBSTRING is not
   a substring of STRING.  */
extern size_t search_lenstring (lenstring *STRING,
                const char *SUBSTRING,
                size_t START);


/* Strip newlines from IN, leaving the result in OUT.
   OUT points to the same text as IN.  */
extern void strip_newlines (lenstring *OUT, lenstring *IN);


/* Append STR2 to the end of STR1.  Assume STR1 has sufficient space
   allocated.  */
extern void append_lenstring (lenstring *STR1, lenstring *STR2);

/* Append character C to the end of STR.  Assume STR has sufficient
   space allocated.  */
#define APPEND_CHAR(str, c) ((str)->text[(str)->len++] = (c))
