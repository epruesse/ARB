/* embl-tabl.c --- Converting EMBL files to tabl format
   Jim Blandy <jimb@gnu.ai.mit.edu> --- September 1994 */

/* For a description of the file format this program is intended to
   parse, look at:
   /ftp.embl-heidelberg.de:/pub/databases/embl/doc/usrman.doc
   That seems to be the official definition of the EMBL file format.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
embl-tabl transforms one or more concatenated EMBL entries into\n\
key/value pairs.  Calling sequence, with [defaults]:\n\
[ --embl-file FILE ]             ; read EMBL entries from FILE [stdin]\n\
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
   hash table.  read_EMBL can then check each entry name field against the
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


/* Dealing with EMBL files.  */

/* Find a line in BUFFER that starts with LINE_TYPE.
   Return its starting address.  */
char *
find_line (lenstring *buffer, const char *line_type)
{
    int pos = 0;

    do
        pos = search_lenstring (buffer, line_type, pos);
    while (pos > 0 && buffer->text[pos - 1] != '\n');

    if (pos == -1)
        return NULL;
    else
        return buffer->text + pos;
}


/* Convert sequence data from EMBL format to tabl format.  SEQ
   contains the sequence data, in EMBL format.  The conversion is
   done in-place, since tabl is always smaller than EMBL.  */
void
EMBL_to_tabl_sequence (lenstring *seq)
{
    char *source     = seq->text;
    char *source_end = seq->text + seq->len;
    char *dest = source;

    do
    {
        char c;
        char *line_end;

        /* The EMBL specs say that anything in column 73 or after is a
           line number.  Assume source is currently in column 1.  */
        line_end = source + (73 - 1);
        if (line_end > source_end)
            line_end = source_end;

        while (source < line_end && (c = *source) != '\n')
        {
            if (c != ' ')
                *dest++ = c;
            source++;
        }

        /* Find the start of the next line.  */
        if (source >= line_end)
        {
            while (source < source_end && *source++ != '\n')
                ;
        }
        else
            /* We must have hit a newline.  */
            source++;
    }
    while (source < source_end);

    /* Make seq point to the area we've re-formatted.  */
    seq->len = dest - seq->text;
}


void
read_EMBL (char *EMBL_filename,
           char *entry_name_filename,
           char *annotation_filename,
           char *sequence_filename,
           int include)
{
    FILE *EMBL_file;
    FILE *entry_name_file;
    FILE *annotation_file;
    FILE *sequence_file;

    lenstring annotation;
    lenstring entry_name;
    lenstring sequence;

    /* Buffer containing EMBL entry.  */
    lenstring buffer;
    lenstring unstripped_buffer;

    EMBL_file       = careful_open (EMBL_filename,       "r",  stdin);
    entry_name_file = careful_open (entry_name_filename, "w+", stdout);
    annotation_file = careful_open (annotation_filename, "w+", stdout);
    sequence_file   = careful_open (sequence_filename,   "w+", stdout);

    while (read_delimited_lenstring (&unstripped_buffer, "//", EMBL_file)
           != EOF)
    {
        /* Start of first sequence line, after an SQ or SA line.  */
        char *seq_start;

        /* First address after the buffer.  */
        char *buffer_end;

        strip_newlines (&buffer, &unstripped_buffer);

        /* Ignore newlines before EOF.  */
        if (buffer.len == 0 && feof (EMBL_file))
        {
            free (unstripped_buffer.text);
            break;
        }

        buffer_end = buffer.text + buffer.len;

        /* Find the sequence portion of the entry.  Look for an SQ line.  */
        seq_start = find_line (&buffer, "SQ");
        if (seq_start)
            seq_start = (char *) memchr (seq_start, '\n', buffer_end - seq_start);
        if (! seq_start)
        {
            /* If we couldn't find an SQ line, there may be an SA
               ("Sequence Alignment") line; look for that.  */
            seq_start = find_line (&buffer, "SA");
            if (seq_start)
                seq_start = (char *) memchr (seq_start, '\n',
                                             buffer_end - seq_start);
        }
        if (! seq_start)
        {
            fprintf (stderr, "%s: entry has neither a correct SQ or SA line\n",
                     EMBL_filename ? EMBL_filename : "stdin");
            exit (1);
        }

        /* seq_start should really sit *after* the newline.  */
        seq_start++;

        /* Make entry.annotation point at the annotation section of the
           buffer.  */
        annotation.text = buffer.text;
        annotation.len  = seq_start - buffer.text;

        /* Find the entry name.  */
        {
            char *p;
            char *start;

            if (! (p = find_line (&annotation, "ID")))
            {
                fprintf (stderr,
                         "%s: entry lacks a valid ID line\n",
                         EMBL_filename ? EMBL_filename : "stdin");
                exit (1);
            }

            /* Find the entry name on the ID line:
               "The first item on the ID line is the name of the database
               entry containing the sequence.  This name is the primary
               means of identifying the sequence within a given release,
               and is used in indexes built from the database.  The entry
               name consists of up to nine uppercase alphanumeric
               characters, and always begins with a letter."
               We'll actually accept any sequence of non-space characters.  */
            p += 2;                 /* skip "ID" */
            while (p < buffer_end && *p == ' ')
                p++;
            start = p;
            while (p < buffer_end && p < start + 9 && *p != ' ')
                p++;
            entry_name.text = start;
            entry_name.len  = p - start;
        }


        /* Convert sequence to EMBL format.  */
        sequence.text = seq_start;
        sequence.len  = buffer_end - seq_start;
        EMBL_to_tabl_sequence (&sequence);


        /* Write this entry's data.
           If we're including or excluding, only write the appropriate
           stuff.  */
        if (include == 0
            || (include == -1 && ! present_p (&entry_name))
            || (include ==  1 &&   present_p (&entry_name)))
        {
            /* what to print out and where */
            write_lenstring (&entry_name, entry_name_file);
            putc ('\n', entry_name_file);

            write_lenstring (&annotation, annotation_file);
            putc ('\f', annotation_file);

            write_lenstring (&sequence, sequence_file);
            putc ('\n', sequence_file);
        }

        check_file (entry_name_file, entry_name_filename,
                    "writing EMBL entry names");
        check_file (annotation_file, annotation_filename,
                    "writing EMBL annotations");
        check_file (sequence_file, sequence_filename,
                    "writing EMBL sequence data");

        free (unstripped_buffer.text);
    }

    careful_close (EMBL_file, EMBL_filename);
    careful_close (entry_name_file, entry_name_filename);
    careful_close (annotation_file, annotation_filename);
    careful_close (sequence_file, sequence_filename);
}



/* Processing command-line arguments.  */

int
main (int argc, char *argv[])
{
    char *EMBL_file = NULL;       /* Name of EMBL file */
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
                fputs ("embl-tabl: "
                       "`--include-keys' and `--exclude-keys' may not be\n"
                       "embl-tabl: combined or repeated\n",
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
                fputs ("embl-tabl: "
                       "`--include-keys' and `--exclude-keys' may not be\n"
                       "embl-tabl: combined or repeated\n",
                       stderr);
                exit (1);
            }
            i++;
            index_file = argv[i];
            include = -1;
        }
        else if (!strcmp (argv[i], "--embl-file"))
        {
            i++;
            EMBL_file = argv[i];
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

    read_EMBL (EMBL_file, entry_name_file, annotation_file, sequence_file,
               include);

    return 0;
}
