/* tabl-embl.c --- convert from tabl to EMBL format
   Jim Blandy <jimb@gnu.ai.mit.edu> --- Septemeber 1994  */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "xmalloc.h"
#include "lenstring.h"
#include "careful.h"

char *progname;


/* Utilities.  */


/* Assembling EMBL files.  */

#define EMBL_LEFT_SPACE (5)
#define EMBL_COLUMN_WIDTH (10)
#define EMBL_BASES_PER_LINE (60)
#define EMBL_NUMBER_COLUMN  (72)
#define EMBL_NUMBER_WIDTH   (8)
#define EMBL_LINE_WIDTH (EMBL_NUMBER_COLUMN + EMBL_NUMBER_WIDTH)

/* Given a tabl-style sequence in TABL, set EMBL to an equivalent
   EMBL-style sequence.  EMBL is freshly-allocated, and should be
   freed by the caller.  */
void
tabl_to_EMBL_sequence (lenstring *tabl, lenstring *EMBL)
{
    /* Figger the number of lines required (including the partial one at
       the end), and then allocate EMBL_LINE_WIDTH+1 bytes per each
       (that many cols and a newline).  */
    EMBL->len = (((tabl->len + EMBL_BASES_PER_LINE - 1) / EMBL_BASES_PER_LINE)
                 * (EMBL_LINE_WIDTH + 1));
    EMBL->text = (char *) xmalloc (EMBL->len);

    {
        char *source = tabl->text;
        char *source_end = tabl->text + tabl->len;
        char *line_end = source;
        char *column_end = source;
        char *stop;
        char *dest = EMBL->text;

        do
        {
            /* Should we create the beginning of the line?  */
            if (source >= line_end)
            {
                memset (dest, ' ', EMBL_LEFT_SPACE);
                dest += EMBL_LEFT_SPACE;
                line_end += EMBL_BASES_PER_LINE;
            }

            if (source >= column_end)
                column_end = source + EMBL_COLUMN_WIDTH;

            /* Where is the next stop?  */
            stop = column_end;
            if (stop > line_end) stop = line_end;
            if (stop > source_end) stop = source_end;

            memcpy (dest, source, stop - source);
            dest += stop - source;
            source = stop;

            /* Should we create an end-of-line?  */
            if (source >= source_end || source >= line_end)
            {
                /* Figure out what column we're in now in the output,
                   and use that to figure where the numbers should go.  */
                int dest_col = (dest - EMBL->text) % (EMBL_LINE_WIDTH + 1);
                char *numbers = dest + EMBL_NUMBER_COLUMN - dest_col;

                /* I hope we haven't rendered too far... */
                if (numbers < dest)
                    abort ();

                memset (dest, ' ', (numbers - dest) + EMBL_NUMBER_WIDTH);

                /* Draw the numbers.  We can't quite use sprintf, because we
                   need a hard limit on the number of digits rendered.  */
                {
                    int i, value;

                    for (i = EMBL_NUMBER_WIDTH - 1, value = source - tabl->text;
                         i >= 0 && value != 0;
                         i--, value /= 10)
                        numbers[i] = '0' + (value % 10);
                }

                dest = numbers + EMBL_NUMBER_WIDTH;
                *dest++ = '\n';
            }

            /* Should we create an end-of-column?  */
            else if (source >= column_end)
                *dest++ = ' ';
        }
        while (source < source_end);

        EMBL->len = dest - EMBL->text;
    }
}


void
write_EMBL (char *EMBL_filename,
            char *annotation_filename,
            char *sequence_filename)
{
    FILE *EMBL_file       = careful_open (EMBL_filename,       "w", stdout);
    FILE *annotation_file = careful_open (annotation_filename, "r", stdin);
    FILE *sequence_file   = careful_open (sequence_filename,   "r", stdin);

    for (;;)
    {
        lenstring annotation, tabl_sequence, EMBL_sequence;

        /* Try to read something from both input files.  This makes sure
           to set the flags that feof tests, so we can make sure both
           files ended at the same place.  */

        {
            int ann_eof =
                (read_delimited_lenstring (&annotation,    "\f", annotation_file)
                 == EOF);
            int seq_eof =
                (read_delimited_lenstring (&tabl_sequence, "\n", sequence_file)
                 == EOF);

            if (ann_eof || seq_eof)
                break;
        }

        tabl_to_EMBL_sequence (&tabl_sequence, &EMBL_sequence);

        write_lenstring (&annotation,    EMBL_file);
        write_lenstring (&EMBL_sequence, EMBL_file);
        fputs ("//\n", EMBL_file);

        check_file (EMBL_file, EMBL_filename, "writing EMBL data");

        free (annotation.text);
        free (tabl_sequence.text);
        free (EMBL_sequence.text);
    }

    if (!feof (annotation_file) || !feof (sequence_file))
    {
        fprintf (stderr,
                 "%s: %s: Hallelujah!  You have more sequences than\n"
                 "%s: %s: annotations, or vice versa.\n",
                 progname, EMBL_filename,
                 progname, EMBL_filename);
        exit (1);
    }

    careful_close (EMBL_file, EMBL_filename);
    careful_close (annotation_file, annotation_filename);
    careful_close (sequence_file, sequence_filename);
}


/* Processing command-line arguments.  */

void
usage ()
{
    fputs ("\
Written by Jim Blandy <jimb@gnu.ai.mit.edu>\n\
$Id$\n\
\n\
tabl-embl builds an EMBL file from two files, one containing headers\n\
separated by form feeds, and the other containing sequences, separated\n\
by newlines.\n\
Calling sequence, with [defaults]:\n\
[ --embl-file FILE ]             ; write EMBL entries to FILE [stdout]\n\
[ --annotation-in FILE ]         ; write entire annotation to FILE [stdin]\n\
[ --sequence-in FILE ]           ; write sequence data to FILE [stdin]\n\
[ -h | --help ]                  ; display this text\n",
           stderr);

    exit (0);
}

int
main (int argc, char *argv[])
{
    char *EMBL_file = NULL;       /* Name of EMBL file */
    char *annotation_file = NULL; /* name of output file for annotations */
    char *sequence_file = NULL;   /* name of output file for sequences */
    int i;

    progname = careful_prog_name (argv[0]);

    for (i = 1; i < argc; i++)
    {
        if (!strcmp (argv[i], "--embl-file"))
        {
            i++;
            EMBL_file = argv[i];
        }
        else if (!strcmp (argv[i], "--annotation-in"))
        {
            i++;
            annotation_file = argv[i];
        }
        else if (!strcmp (argv[i], "--sequence-in"))
        {
            i++;
            sequence_file = argv[i];
        }
        else if (!strcmp (argv[i], "-h") || !strcmp (argv[i], "--help"))
        {
            usage ();
            return 1;
        }
        else
        {
            fprintf (stderr,
                     "\nYour calling sequence is incorrect.  Try %s --help\n",
                     progname);
            return 1;
        }
    }

    write_EMBL (EMBL_file, annotation_file, sequence_file);

    return 0;
}
