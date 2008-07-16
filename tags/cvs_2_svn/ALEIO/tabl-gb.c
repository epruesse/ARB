#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>

#include "careful.h"

char *progname;

/* Most entries are smaller than this, so this value avoids a few
   calls to realloc.  */
#define INITIAL_BUF_LEN 4096

typedef struct
{
    char *text;
    size_t len;
}
    len_string;

#define FWRITE_LEN_STRING(len_string, stream)                           \
    (fwrite ((len_string).text, sizeof (char), (len_string).len, (stream)))


/* This is auxiliary function that used by -h or -help options
   Simply to print out information from tabl-gb.help file. */

void
get_help ()
{

    fputs ("\
Written by Pavel Slavin, pavel@darwin.life.uiuc.edu\n\
       and Jim Blandy,   jimb@gnu.ai.mit.edu\n\
$Id$ \n\
tabl-gb writes key/value pairs to a file or stdout in GenBank format.\n\n\
Calling sequence:\n\
[ -h | --help ]                      ; Displays this text\n\
[ --out-file file ]                  ; stdout if omitted\n\
[ --err-file errfile ]               ; stderr if omitted\n\
[ --annotation-file file ]           ; stdin if omitted\n\
  --annotation-end char              ; indicates end of an annotation\n\
                                     ; In ascii code\n\
[ --sequence-file file ]             ; stdin if omitted\n\
  --sequence-end char                ; indicates end of a sequence\n\
                                     ; In ascii code\n\
",
           stderr);

}

/* The name of the file to which we should write error messages, or
   zero for stderr.  */
char *error_file_name = 0;

/* The file to which we write error messages, or zero if we haven't
   opened one yet.  */
FILE *error_file = 0;



/*
 * This function will be called only in case when error
 * was met.  Error entry will get appended to an existing
 * file, or file will get created if there was no previous
 * error entries.
 * Make sure that file was empty (or did not exist) before
 * running the program.  The new entries will get appended
 * to a file
 */
void
error_entry (char *entry)
{
    if (! error_file)
    {
        if (!error_file_name)
            error_file = stderr;
        else
            error_file = fopen (error_file_name, "a+");
    }

    fputs (entry, error_file);
}


/* Signal an error if P is 0; otherwise, return P.  */
void *
check_ptr (void *p)
{
    if (! p)
    {
        error_entry ("virtual memory exhausted\n");
        exit (2);
    }
    else
        return p;
}



/* Read text from SOURCE until we find DELIMITER, or hit EOF.
   Set *BUF_PTR to a malloc'd buffer for the text, which the caller must free.
   The delimiting string or EOF is not included in the buffer.
   If EOF was the first non-newline character we found, return -1.
   Otherwise, return the length of the text read.  */
size_t
getdelim_str (FILE *source, char *delim, char **buf_ptr)
{
    size_t delim_len = strlen (delim);
    char delim_last_char;

    size_t buf_len = INITIAL_BUF_LEN;
    char *buf = (char *) check_ptr (malloc (buf_len));

    size_t i = 0;
    int c;

    if (delim_len == 0)
        abort ();
    delim_last_char = delim[delim_len - 1];

    while ((c = getc (source)) != EOF)
    {
        /* Do we need to enlarge the buffer?  */
        if (i >= buf_len)
        {
            buf_len *= 2;
            buf = (char *) check_ptr (realloc (buf, buf_len));
        }

        buf[i++] = c;

        /* Have we read the delimiter?  We check to see if we just
           stored delim_last_char; this is a quick, false-positive test.
           Then we check for the whole string; this is a slow but
           correct test.  */
        if (c == delim_last_char
            && i >= delim_len
            && ! memcmp (&buf[i - delim_len], delim, delim_len))
            break;
    }

    if (ferror (source))
    {
        perror (progname);
        exit (2);
    }

    *buf_ptr = buf;

    if (c == EOF)
    {
        /* Special case, as documented.  */
        if (i == 0)
        {
            free (buf);
            return -1;
        }
        else
            return i;
    }
    else
        return i - delim_len;
}


/* This function writes sequence in GenBank format. */
void
write_seq (len_string *sequence, FILE *out)
{
    char buf[80];
    size_t sequence_len = sequence->len;
    size_t line_start;

    for (line_start = 0;
         line_start < sequence_len;
         line_start += 60)
    {
        size_t line_end;
        size_t column_start;
        char *p;

        sprintf (buf, "%9d", line_start + 1);
        p = buf + 9;

        /* Where is the end of this line?  */
        line_end = line_start + 60;
        if (line_end > sequence_len)
            line_end = sequence_len;

        for (column_start = line_start;
             column_start < line_end;
             column_start += 10)
        {
            size_t column_len;

            /* Where is the end of this column?  */
            column_len = line_end - column_start;
            if (column_len > 10)
                column_len = 10;

            *p++ = ' ';
            memcpy (p, sequence->text + column_start, column_len);
            p += column_len;
        }

        fwrite (buf, sizeof (char), p - buf, out);
        putc ('\n', out);
    }
}


/*  This function puts back GenBank entries  */
void
put_gbfile (char *outfile, char *annotfile, char *seqfile,
            char annot_end, char seq_end)
{
    FILE *out;
    FILE *annot;
    FILE *seq;
    char annot_end_string[2];
    char seq_end_string  [2];
    len_string annotation;        /* place where each annotation will be held */
    len_string sequence;          /* place where each sequence will be held */

    /* pointers to a out-file, err-file.
       All files opened as read\write, and a new file created
       if one specified does not exist */
    if (!outfile)
        out = stdout;
    else
        out = fopen (outfile, "w+");

    /* pointer to annotation and sequence files.
       Opened as a read only */
    if (!annotfile)
        annot = stdin;
    else
        annot = fopen (annotfile, "r");
    if (!seqfile)
        seq = stdin;
    else
        seq = fopen (seqfile, "r");

    if (annot == NULL || seq == NULL)
    {
        error_entry ("Either annotation or sequence files you specified on\n");
        error_entry ("the command line do not exist.\n");
        exit (1);
    }

    annot_end_string[0] = annot_end;
    seq_end_string  [0] = seq_end;
    annot_end_string[1] = seq_end_string[1] = '\0';

    while (!feof (annot) && !feof (seq))
    {
        annotation.len = getdelim_str (annot, annot_end_string,
                                       &annotation.text);
        sequence.len   = getdelim_str (seq,   seq_end_string,
                                       &sequence.text);
        if (annotation.len != -1 && sequence.len != -1)
        {
            FWRITE_LEN_STRING (annotation, out);
            write_seq (&sequence, out);
            fputs ("//\n", out);
            free (annotation.text);
            free (sequence.text);
        }
        else
            break;

        check_file (out, outfile, "writing GenBank data");
    }

    if (!feof (annot) || !feof (seq))
    {
        error_entry ("Hallelujah!  You have more sequences than annotations, or ");
        error_entry ("vice versa.");
        exit (1);
    }

    careful_close (seq, seqfile);
    careful_close (annot, annotfile);
    careful_close (out, outfile);
}



int
main (int argc, char *argv[])
{
    char *outfile = NULL;         /* Name of output GenBank file */
    char *annotfile = NULL;       /* Name of annotation file */
    char *seqfile = NULL;         /* Name of sequence file */
    char annotend = 12;           /* Separator at the end of each annotation */
    char seqend = 10;             /* Separator at the end of each sequences */
    int i;

    progname = careful_prog_name (argv[0]);

    if (argc == 1)
    {
        get_help ();
        return (1);
    }

    for (i = 1; i < argc; i++)
    {
        if (!strcmp (argv[i], "--annotation-end"))
        {
            i++;
            annotend = toascii (atoi (argv[i]));
        }
        else if (!strcmp (argv[i], "--sequence-end"))
        {
            i++;
            seqend = toascii (atoi (argv[i]));
        }
        else if (!strcmp (argv[i], "--out-file"))
        {
            i++;
            outfile = argv[i];
        }
        else if (!strcmp (argv[i], "--err-file"))
        {
            i++;
            error_file_name = argv[i];
        }
        else if (!strcmp (argv[i], "--annotation-file"))
        {
            i++;
            annotfile = argv[i];
        }
        else if (!strcmp (argv[i], "--sequence-file"))
        {
            i++;
            seqfile = argv[i];
        }
        else if (!strcmp (argv[i], "-h") || !strcmp (argv[i], "--help"))
        {
            get_help ();
            return 1;
        }
        else
        {
            fputs ("\nYour calling sequence is incorrect.\
  Try tabl-gb --help option\n", stderr);
            return 1;
        }
    }

    put_gbfile (outfile, annotfile, seqfile, annotend, seqend);

    return 0;
}
