#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <memory.h>

extern void bcopy ();
extern int  bcmp ();

#include "xmalloc.h"
#include "lenstring.h"
#include "hash.h"
#include "careful.h"

char *progname;

typedef struct
{
    lenstring locus_name;
    lenstring annotation;
    lenstring sequence;
}
    gb_entry;


/* Utilities.  */


/* Printing usage messages.  */

void
get_help ()
{

    fputs ("\
Written by Pavel Slavin, pavel@darwin.life.uiuc.edu\n\
       and Jim Blandy, jimb@gnu.ai.mit.edu\n\
$Id$ \n\
\n\
gb-tabl transforms one or more concatenated GenBank entries into\n\
locus/value pairs.  Calling sequence, with [defaults]:\n\
[ --gb-file FILE ]               ; read GenBank entries from FILE [stdin]\n\
[ --include-loci FILE ]          ; include only loci in FILE [all]\n\
[ --exclude-loci FILE ]          ; exclude all loci in FILE [none]\n\
[ --locus-name-out FILE ]        ; write locus names to FILE [stdout]\n\
[ --annotation-out FILE ]        ; write entire annotation to FILE [stdout]\n\
[ --sequence-out FILE ]          ; write sequence data to FILE [stdout]\n\
[ -h | --help ]                  ; Displays this text\n",
           stderr);

    exit (0);
}



/* A hash table for use in selecting subsets, and functions to use it.  */

struct string_hash *index_table;

/* Read the contents of a file full of loci (one per line) into the
   hash table.  read_GenBank can then check each locus field against the
   hash table to decide whether to exclude/include the locus's entry.  */
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


/* Dealing with GenBank entries.  */

/* Find a line in BUFFER[BUFFER_LEN] that starts with HEADER.
   Return its starting address.  */
char *
find_header (lenstring *buffer, const char *header)
{
    int pos = 0;

    for (;;)
    {
        pos = search_lenstring (buffer, header, pos);
        if (pos <= 0 || buffer->text[pos - 1] == '\n')
            break;
        pos++;
    }

    if (pos == -1)
        return NULL;
    else
        return buffer->text + pos;
}


/* Convert sequence data from GenBank format to tabl format.  SEQ
   contains the sequence data, in GenBank format.  The conversion is
   done in-place, since tabl is always smaller than GenBank.  */
void
gb_to_tabl_sequence (lenstring *seq)
{
    char *source     = seq->text;
    char *source_end = seq->text + seq->len;
    char *dest = source;

    do
    {
        char c;

        /* Skip zero or more blanks, zero or more digits, and then zero
           or more blanks.  Consume the largest such prefix possible.  */
        while (source < source_end
               && (*source == ' ' || *source == '\t'))
            source++;
        while (source < source_end
               && isascii (*source)
               && isdigit (*source))
            source++;
        while (source < source_end
               && (*source == ' ' || *source == '\t'))
            source++;

        /* Since we skip sections of text, we might not notice
           terminator characters in odd places, so we check against
           the ending address instead.  */
        while (source < source_end && (c = *source++) != '\n')
        {
            if (c != ' ')
                *dest++ = c;
        }
    }
    while (source < source_end);

    /* Make seq point to the area we've re-formatted.  */
    seq->len = dest - seq->text;
}


/*  This function reads GenBank file ( entry-by-entry ). */
void
read_GenBank (char *GenBank_filename,
              char *locus_name_filename,
              char *annotation_filename,
              char *sequence_filename,
              int include)
{
    gb_entry entry;         /* entry is a var of type gb_entry (see above) */
    FILE *GenBank_file;
    FILE *locus_name_file;
    FILE *annotation_file;
    FILE *sequence_file;

    /* Buffer containing GenBank entry.  */
    lenstring buffer;
    lenstring unstripped_buffer;

    GenBank_file    = careful_open (GenBank_filename,    "r", stdin);
    locus_name_file = careful_open (locus_name_filename, "w+", stdout);
    annotation_file = careful_open (annotation_filename, "w+", stdout);
    sequence_file   = careful_open (sequence_filename,   "w+", stdout);

    while (read_delimited_lenstring (&unstripped_buffer, "//", GenBank_file)
           != EOF)
    {
        /* start of line after ORIGIN line.  */
        char *sequence_start;

        /* First address after the buffer.  */
        char *buffer_end;

        strip_newlines (&buffer, &unstripped_buffer);

        /* Ignore newlines before EOF.  */
        if (buffer.len == 0 && feof (GenBank_file))
        {
            free (unstripped_buffer.text);
            break;
        }

        buffer_end = buffer.text + buffer.len;

        /* sequence_start is the first line after the ORIGIN record.  */
        sequence_start = find_header (&buffer, "ORIGIN");
        if (sequence_start)
            sequence_start = (char *) memchr (sequence_start, '\n',
                                              buffer_end - sequence_start);
        if (! sequence_start)
        {
            fprintf (stderr, "%s: entry lacks a correct ORIGIN line\n",
                     GenBank_filename ? GenBank_filename : "stdin");
            exit (1);
        }

        /* sequence_start should really sit *after* the newline.  */
        sequence_start ++;

        /* Make entry.annotation point at the annotation section of the
           buffer.  */
        entry.annotation.text = buffer.text;
        entry.annotation.len  = sequence_start - buffer.text;

        /* Find the locus field.  */
        {
            char *p;
            char *start;

            if (! (p = find_header (&entry.annotation, "LOCUS")))
            {
                fprintf (stderr,
                         "%s: entry lacks a correct LOCUS line\n",
                         GenBank_filename ? GenBank_filename : "stdin");
                exit (1);
            }

            /* Find the name on the LOCUS line.  We assume it's the
               first string of non-spaces after the "LOCUS" string.  */
            p += 12;
            while (p < buffer_end && *p != '\n' && isspace (*p))
                p++;
            start = p;
            while (p < buffer_end && ! isspace (*p))
                p++;
            entry.locus_name.text = start;
            entry.locus_name.len  = p - start;
        }


        /* Convert sequence to GenBank format.  */
        entry.sequence.text = sequence_start;
        entry.sequence.len  = buffer_end - sequence_start;
        gb_to_tabl_sequence (&entry.sequence);


        /* Write this entry's data.  */
        {
            lenstring *locus = &entry.locus_name;

            /* If we're including or excluding, only write the appropriate
               stuff.  */
            if (include == 0
                || (include == -1 && ! present_p (locus))
                || (include ==  1 &&   present_p (locus)))
            {
                /* what to print out and where */
                write_lenstring (&entry.locus_name, locus_name_file);
                putc ('\n', locus_name_file);
                check_file (locus_name_file, locus_name_filename,
                            "writing GenBank locus names");

                write_lenstring (&entry.annotation, annotation_file);
                putc ('\f', annotation_file);
                check_file (annotation_file, annotation_filename,
                            "writing GenBank annotations");

                write_lenstring (&entry.sequence, sequence_file);
                putc ('\n', sequence_file);
                check_file (sequence_file, sequence_filename,
                            "writing GenBank sequence data");
            }
        }

        free (unstripped_buffer.text);
    }

    careful_close (GenBank_file, GenBank_filename);
    careful_close (locus_name_file, locus_name_filename);
    careful_close (annotation_file, annotation_filename);
    careful_close (sequence_file, sequence_filename);
}


/* Parsing command-line arguments.  */

int
main (int argc, char *argv[])
{
    char *GenBank_file = NULL;    /* Name of gen_bank file */
    char *index_file = NULL; /* file of indices to extract */
    char include = 0;             /* should exclude named indices, or include */
    char *locus_name_file = NULL; /* Name of output file for loci */
    char *annotation_file = NULL; /* name of output file for annotations */
    char *sequence_file = NULL;   /* name of output file for sequences */
    int i = 0;                    /* counter for first for loop */

    progname = careful_prog_name (argv[0]);

    for (i = 1; i < argc; i++)
    {
        if (!strcmp (argv[i], "--include-loci"))
        {
            if (include != 0)
            {
                fputs ("gb-tabl: "
                       "`--include-loci' and `--exclude-loci' may not be\n"
                       "gb-tabl: combined or repeated\n",
                       stderr);
                exit (1);
            }
            i++;
            index_file = argv[i];
            include = 1;
        }
        else if (!strcmp (argv[i], "--exclude-loci"))
        {
            if (include != 0)
            {
                fputs ("gb-tabl: "
                       "`--include-loci' and `--exclude-loci' may not be\n"
                       "gb-tabl: combined or repeated\n",
                       stderr);
                exit (1);
            }
            i++;
            index_file = argv[i];
            include = -1;
        }
        else if (!strcmp (argv[i], "--gb-file"))
        {
            i++;
            GenBank_file = argv[i];
        }
        else if (!strcmp (argv[i], "--locus-name-out"))
        {
            i++;
            locus_name_file = argv[i];
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
            fputs ("\nYour calling sequence is incorrect.  Try gb-tabl --help\n",
                   stderr);
            return 1;
        }
    }

    if (include != 0)
        read_index_file (index_file);

    read_GenBank (GenBank_file, locus_name_file, annotation_file, sequence_file,
                  include);

    return 0;
}
