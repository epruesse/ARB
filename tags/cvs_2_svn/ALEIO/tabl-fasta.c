/* tabl-fasta.c --- convert from tabl to FASTA format
   Jim Blandy <jimb@gnu.ai.mit.edu> --- Septemeber 1994  */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "xmalloc.h"
#include "lenstring.h"
#include "careful.h"

char *progname;


/* Utilities.  */


/* Assembling FASTA files.  */

#define FASTA_BASES_PER_LINE (50)
#define FASTA_INDENTATION (2)
#define FASTA_LINE_WIDTH (FASTA_BASES_PER_LINE + FASTA_INDENTATION)

/* Given a tabl-style sequence in TABL, set FASTA to an equivalent
   FASTA-style sequence.  FASTA is freshly-allocated, and should be
   freed by the caller.  */
void
tabl_to_FASTA_sequence (lenstring *tabl, lenstring *FASTA)
{
    /* Figger the number of lines required (including the partial one at
       the end), and then allocate FASTA_LINE_WIDTH+1 bytes per each
       (that many cols and a newline).  */
    FASTA->len = (((tabl->len + FASTA_BASES_PER_LINE - 1) / FASTA_BASES_PER_LINE)
                  * (FASTA_LINE_WIDTH + 1));
    FASTA->text = (char *) xmalloc (FASTA->len);

    {
        char *source = tabl->text;
        char *source_end = tabl->text + tabl->len;
        char *dest = FASTA->text;

        while (source < source_end)
        {
            int bases = FASTA_BASES_PER_LINE;

            /* Should we write a partial line?  */
            if (bases > source_end - source)
                bases = source_end - source;

            memset (dest, ' ', FASTA_INDENTATION);
            dest += FASTA_INDENTATION;

            memcpy (dest, source, bases);
            dest += bases;
            source += bases;

            *dest++ = '\n';
        }

        FASTA->len = dest - FASTA->text;
    }
}


void
write_FASTA (char *FASTA_filename,
             char *annotation_filename,
             char *sequence_filename)
{
    FILE *FASTA_file      = careful_open (FASTA_filename,      "w", stdout);
    FILE *annotation_file = careful_open (annotation_filename, "r", stdin);
    FILE *sequence_file   = careful_open (sequence_filename,   "r", stdin);

    for (;;)
    {
        lenstring annotation, tabl_sequence, FASTA_sequence;

        /* Try to read something from both input files.  This makes sure
           to set the flags that feof tests, so we can make sure both
           files ended at the same place.  */
        {
            int ann_eof =
                read_delimited_lenstring (&annotation,    "\f", annotation_file);
            int seq_eof =
                read_delimited_lenstring (&tabl_sequence, "\n", sequence_file);

            if (ann_eof || seq_eof)
                break;
        }

        tabl_to_FASTA_sequence (&tabl_sequence, &FASTA_sequence);

        write_lenstring (&annotation,    FASTA_file);
        write_lenstring (&FASTA_sequence, FASTA_file);

        check_file (FASTA_file, FASTA_filename, "writing FASTA data");

        free (annotation.text);
        free (tabl_sequence.text);
        free (FASTA_sequence.text);
    }

    if (!feof (annotation_file) || !feof (sequence_file))
    {
        fprintf (stderr,
                 "%s: %s: Hallelujah!  You have more sequences than\n"
                 "%s: %s: annotations, or vice versa.\n",
                 progname, FASTA_filename,
                 progname, FASTA_filename);
        exit (1);
    }

    careful_close (FASTA_file, FASTA_filename);
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
tabl-fasta builds an FASTA file from two files, one containing headers\n\
separated by form feeds, and the other containing sequences, separated\n\
by newlines.\n\
Calling sequence, with [defaults]:\n\
[ --fasta-file FILE ]            ; write FASTA entries to FILE [stdout]\n\
[ --annotation-in FILE ]         ; write entire annotation to FILE [stdin]\n\
[ --sequence-in FILE ]           ; write sequence data to FILE [stdin]\n\
[ -h | --help ]                  ; display this text\n",
           stderr);

    exit (0);
}

int
main (int argc, char *argv[])
{
    char *FASTA_file = NULL;      /* Name of FASTA file */
    char *annotation_file = NULL; /* name of output file for annotations */
    char *sequence_file = NULL;   /* name of output file for sequences */
    int i;

    progname = careful_prog_name (argv[0]);

    for (i = 1; i < argc; i++)
    {
        if (!strcmp (argv[i], "--fasta-file"))
        {
            i++;
            FASTA_file = argv[i];
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

    write_FASTA (FASTA_file, annotation_file, sequence_file);

    return 0;
}
