/* fasta-tabl.c --- Converting FASTA files to tabl format
   Jim Blandy <jimb@gnu.ai.mit.edu> --- September 1994 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "xmalloc.h"
#include "lenstring.h"
#include "hash.h"
#include "careful.h"

char *progname;


/* Utilities.  */


/* Usage messages.  */

void
get_help ()
{

    fputs ("\
Written by Jim Blandy <jimb@gnu.ai.mit.edu>\n\
$Id$\n\
\n\
fasta-tabl transforms one or more concatenated FASTA entries into\n\
key/value pairs.  Calling sequence, with [defaults]:\n\
[ --fasta-file FILE ]            ; read FASTA entries from FILE [stdin]\n\
[ --include-keys FILE ]          ; include only keys in FILE [all]\n\
[ --exclude-keys FILE ]          ; exclude all keys in FILE [none]\n\
[ --entry-name-out FILE ]        ; write entry names to FILE [stdout]\n\
[ --annotation-out FILE ]        ; write entire annotation to FILE [stdout]\n\
[ --sequence-out FILE ]          ; write sequence data to FILE [stdout]\n\
[ -h | --help ]                  ; Displays this text\n",
           stderr);

    exit (0);
}


/* A hash table for use in selecting subsets, and functions to use it.  */

struct string_hash *index_table;

/* Read the contents of a file full of loci (one per line) into the
   hash table.  read_FASTA can then check each entry name field against the
   hash table to decide whether to exclude/include the entry.  */
void
read_index_file (char *index_file_name)
{
    FILE *index_file;
    lenstring buf;

    index_file = careful_open (index_file_name, "r", 0);
    index_table = new_hash_table ();

    while (read_delimited_lenstring (&buf, "\n", index_file) != EOF)
    {
        lookup_hash_table (index_table, buf.text, buf.len);
        free (buf.text);
    }

    careful_close (index_file, index_file_name);
}

/* Return non-zero iff INDEX is in the hash table.  */
int
present_p (lenstring *index)
{
    return lookup_hash_table_soft (index_table, index->text, index->len) != 0;
}


/* Dealing with FASTA files.  */

void
read_FASTA (char *FASTA_filename,
            char *entry_name_filename,
            char *annotation_filename,
            char *sequence_filename,
            int include)
{
    FILE *FASTA_file;
    FILE *entry_name_file;
    FILE *annotation_file;
    FILE *sequence_file;

    /* The state of the input.  Be careful to notice:
       sequence lines before any header lines
       consecutive header lines, with no intervening sequence  */
    enum input_state {
        top_of_file,
        after_header,
        after_some_sequence
    };
    enum input_state input_state = top_of_file;

    /* The state of the output --- do we have an unterminated sequence
       to finish?  */
    int unterminated_sequence = 0;

    /* True if the current sequence is to be included.  */
    int include_entry;

    lenstring buffer;

    FASTA_file      = careful_open (FASTA_filename,       "r",  stdin);
    entry_name_file = careful_open (entry_name_filename, "w+", stdout);
    annotation_file = careful_open (annotation_filename, "w+", stdout);
    sequence_file   = careful_open (sequence_filename,   "w+", stdout);

    while (read_delimited_lenstring (&buffer, "\n", FASTA_file) != EOF)
    {
        /* Is this a header line or a sequence line?  */
        if (buffer.len >= 1 && buffer.text[0] == '>')
            /* Process a header line.  */
        {
            char *buffer_end = buffer.text + buffer.len;
            char *p = buffer.text + 1;

            /* A guess at a decent entry name.  */
            lenstring entry_name;

            if (input_state == after_header)
            {
                /* We just had a null sequence (i.e. the line
                   immediately before this was a header line too.  */
                fprintf (stderr,
                         "%s: %s: FASTA file has two consecutive header lines\n"
                         "%s: %s: with no sequence between them\n",
                         progname, FASTA_filename,
                         progname, FASTA_filename);
                exit (2);
            }
            input_state = after_header;

            if (unterminated_sequence)
            {
                /* End any sequence line that came before this.  */
                putc ('\n', sequence_file);
                unterminated_sequence = 0; /* doesn't matter */
            }

            /* Skip blanks after the >.  */
            while (p < buffer_end && isspace (*p))
                p++;

            /* Guess that an entry name is a string of up to ten
               characters containing no spaces (or colons, because Gary
               Olsen says he likes to separate the entry name from other
               data with a colon, or commas, because ReadSeq writes
               FASTA files with commas).  */
            entry_name.text = p;
            while (p < buffer_end
                   && p - entry_name.text < 10
                   && ! isspace (*p)
                   && *p != ':'
                   && *p != ',')
                p++;
            entry_name.len = p - entry_name.text;

            /* Should we include this entry in the output?  */
            include_entry =
                (include == 0
                 || (include == -1 && ! present_p (&entry_name))
                 || (include ==  1 &&   present_p (&entry_name)));

            if (include_entry)
            {
                write_lenstring (&entry_name, entry_name_file);
                putc ('\n', entry_name_file);

                /* Treat the entire line as the annotation.  */
                write_lenstring (&buffer, annotation_file);
                putc ('\n', annotation_file);
                putc ('\f', annotation_file);

                check_file (entry_name_file, entry_name_filename,
                            "writing FASTA entry names");
                check_file (annotation_file, annotation_filename,
                            "writing FASTA annotations");
            }
        }
        else
            /* Process a sequence line.  */
        {
            if (input_state == top_of_file)
            {
                /* This is a headerless sequence.  */
                fprintf (stderr,
                         "%s: %s: FASTA file doesn't start with a header line\n",
                         progname, FASTA_filename);
                exit (1);
            }
            input_state = after_some_sequence;

            if (include_entry)
            {
                char *source = buffer.text;
                char *dest   = buffer.text;
                char *source_end = buffer.text + buffer.len;

                for (; source < source_end; source++)
                    if (! isspace (*source))
                        *dest++ = *source;

                buffer.len = dest - buffer.text;
                write_lenstring (&buffer, sequence_file);
                check_file (sequence_file, sequence_filename,
                            "writing FASTA sequence data");
                unterminated_sequence = 1;
            }
        }

        free (buffer.text);
    }

    /* Finish off any sequence line we were in the midst of.  */
    if (unterminated_sequence)
        putc ('\n', sequence_file);

    careful_close (FASTA_file, FASTA_filename);
    careful_close (entry_name_file, entry_name_filename);
    careful_close (annotation_file, annotation_filename);
    careful_close (sequence_file, sequence_filename);
}



/* Processing command-line arguments.  */

int
main (int argc, char *argv[])
{
    char *FASTA_file = NULL;      /* Name of FASTA file */
    char *index_file = NULL; /* file of indices to extract */
    char include = 0;             /* should exclude named indices, or include */
    char *entry_name_file = NULL; /* Name of output file for keys */
    char *annotation_file = NULL; /* name of output file for annotations */
    char *sequence_file = NULL;   /* name of output file for sequences */
    int i = 0;                    /* counter for first for loop */

    progname = careful_prog_name (argv[0]);

    for (i = 1; i < argc; i++)
    {
        if (!strcmp (argv[i], "--include-keys"))
        {
            if (include != 0)
            {
                fputs ("fasta-tabl: "
                       "`--include-keys' and `--exclude-keys' may not be\n"
                       "fasta-tabl: combined or repeated\n",
                       stderr);
                exit (1);
            }
            i++;
            index_file = argv[i];
            include = 1;
        }
        else if (!strcmp (argv[i], "--exclude-keys"))
        {
            if (include != 0)
            {
                fputs ("fasta-tabl: "
                       "`--include-keys' and `--exclude-keys' may not be\n"
                       "fasta-tabl: combined or repeated\n",
                       stderr);
                exit (1);
            }
            i++;
            index_file = argv[i];
            include = -1;
        }
        else if (!strcmp (argv[i], "--fasta-file"))
        {
            i++;
            FASTA_file = argv[i];
        }
        else if (!strcmp (argv[i], "--entry-name-out"))
        {
            i++;
            entry_name_file = argv[i];
        }
        else if (!strcmp (argv[i], "--annotation-out"))
        {
            i++;
            annotation_file = argv[i];
        }
        else if (!strcmp (argv[i], "--sequence-out"))
        {
            i++;
            sequence_file = argv[i];
        }
        else if (!strcmp (argv[i], "-h") || !strcmp (argv[i], "--help"))
        {
            get_help ();
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

    if (include != 0)
        read_index_file (index_file);

    read_FASTA (FASTA_file, entry_name_file, annotation_file, sequence_file,
                include);

    return 0;
}
