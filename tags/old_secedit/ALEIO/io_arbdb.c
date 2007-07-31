/*
 * arbdb.c --- Ale Direct ARB Interface !!!!!!!! mainly written by Oliver
 * Strunk (Dec 95) rest is stolen from Jim Blandy <jimb@gnu.ai.mit.edu> ---
 * October 1994
 */

#include <stdio.h>
#include <string.h>
#include <arbdb.h>
#include <arbdbt.h>

#include "ring.h"
#include "getopt.h"
#include "xmalloc.h"
#include "lenstring.h"
#include "hash.h"
#include "careful.h"

void            give_usage(void);
void            give_help(void);
void            give_version(void);
void            print_and_die(const char *message);
void            arbdb_print_and_die(void);
void
put(char *keys_filename,
    char *annotations_filename,
    char *sequences_filename);
void
get(char *keys_filename,
    char *annotations_filename,
    char *sequences_filename);
void
choose(char *choose_filename,
       char *keys_filename,
       char *annotations_filename,
       char *sequences_filename);
void            list(char *keys_filename);
void            verify_index(void);

void            write_index_to_arbdb(void);
void            write_index_to_stream(FILE *);
void            make_index_hash(void);
void            index_input_key(lenstring *);
void            rebuild_index_string(void);

/* The name of the program, for use in error messages.  */
char           *progname;

/* The name of the GDBM file we're operating on, for error messages.  */
char           *arbdb_filename = 0;

/* The GDBM file we're operating on.  */
GBDATA         *g;

enum op {
    op_none = 1000,
    op_put, op_get, op_choose, op_list, op_verify_index
};

/*
 * An array of structures describing the arguments accepted by the program.
 * For use by getopt_long.
 */
struct option   longopts[] = {
    {"put", no_argument, 0, 'p'},
    {"get", no_argument, 0, 'g'},
    {"choose", optional_argument, 0, 'c'},
    {"list", no_argument, 0, 'l'},
    {"verify-index", no_argument, 0, op_verify_index},

    {"keys", required_argument, 0, 'k'},
    {"annotations", required_argument, 0, 'a'},
    {"sequences", required_argument, 0, 's'},

    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'v'},

    {0, 0, 0, 0}
};

int
main(int argc, char **argv)
{
    int             arg, longopts_ix;
    enum op         op = op_none;
    char           *choose_filename = 0;
    char           *keys_filename = 0;
    char           *annotations_filename = 0;
    char           *sequences_filename = 0;

    progname = careful_prog_name(argv[0]);

    while ((arg = getopt_long(argc, argv,
                  "pgc::lk:a:s:hv", longopts, &longopts_ix))
           != -1)
        switch (arg) {
        case 'p':
            op = op_put;
            break;
        case 'g':
            op = op_get;
            break;
        case 'c':
            op = op_choose;
            choose_filename = optarg;
            break;
        case 'l':
            op = op_list;
            break;
        case op_verify_index:
            op = op_verify_index;
            break;
        case 'k':
            keys_filename = optarg;
            break;
        case 'a':
            annotations_filename = optarg;
            break;
        case 's':
            sequences_filename = optarg;
            break;

        case 'h':
            give_help();
            break;
        case 'v':
            give_version();
            break;
        default:
            give_usage();
            break;
        }

    if (optind + 1 == argc)
        arbdb_filename = argv[optind];

    if (arbdb_filename == 0) {
        fprintf(stderr, "%s: no GDBM file specified\n", progname);
        give_usage();
    }
    if (op == op_none) {
        fprintf(stderr, "%s: no operation specified\n", progname);
        give_usage();
    }

    /* Open the GDBM file.  */
    /* g = GBT_open(arbdb_filename, "rw", "$(ARBHOME)/lib/pts/\*"); */
    g = GBT_open(":", "rw", "$(ARBHOME)/lib/pts/*");
    if (!g)
        arbdb_print_and_die();


    if (op == op_put)
        put(keys_filename, annotations_filename, sequences_filename);
    else if (op == op_get)
        get(keys_filename, annotations_filename, sequences_filename);
    else if (op == op_choose)
        choose(choose_filename,
           keys_filename, annotations_filename, sequences_filename);
    else if (op == op_list)
        list(keys_filename);
    else if (op == op_verify_index)
        verify_index();

    GB_close(g);

    return 0;
}




/* Set up index_string, by reading the index from the database.  */
/* Putting entries in a GDBM file.  */

void
put(char *keys_filename,
    char *annotations_filename,
    char *sequences_filename)
{
    FILE           *keys_file = careful_open(keys_filename, "r", stdin);
    FILE           *annotations_file = careful_open(annotations_filename, "r", stdin);
    FILE           *sequences_file = careful_open(sequences_filename, "r", stdin);
    GBDATA         *gb_species_data, *gb_species, *gb_sequence;
    char           *alignment_name;
    GB_ERROR    error;
    lenstring       key, annotation, sequence;
    GB_begin_transaction(g);
    GB_change_my_security(g, 6, 0); /* disable all security */
    alignment_name = GBT_get_default_alignment(g);
    gb_species_data = GB_search(g, "species_data", GB_CREATE_CONTAINER);
    for (;;) {
        {
            int             key_eof =
            (read_delimited_lenstring(&key, "\n", keys_file) == EOF);
            int             ann_eof =
            (read_delimited_lenstring(&annotation, "\f", annotations_file) == EOF);
            int             seq_eof =
            (read_delimited_lenstring(&sequence, "\n", sequences_file) == EOF);

            if (key_eof || ann_eof || seq_eof)
                break;
        }
        key.text[key.len] = 0;
        annotation.text[annotation.len] = 0;
        sequence.text[sequence.len] = 0;
        gb_species = GBT_find_species_rel_species_data(gb_species_data, key.text);
        if (!gb_species) {  /* This is an new entry */
            gb_species = GBT_create_species_rel_species_data(gb_species_data, key.text);
        }
        gb_sequence = GBT_add_data(gb_species, alignment_name, "data", GB_STRING);
        error = GB_write_string(gb_sequence, sequence.text);    /* written */
        if (error)
            fprintf(stderr, "%s\n", error);

        free(key.text);
        free(annotation.text);
        free(sequence.text);
    }

    careful_close(keys_file, keys_filename);
    careful_close(annotations_file, annotations_filename);
    careful_close(sequences_file, sequences_filename);
    GB_commit_transaction(g);
}



/* Retrieving all entries from the database.  */

void
get(char *keys_filename,
    char *annotations_filename,
    char *sequences_filename)
{
    FILE           *key_file = careful_open(keys_filename, "w", stdout);
    FILE           *annotation_file = careful_open(annotations_filename, "w", stdout);
    FILE           *sequence_file = careful_open(sequences_filename, "w", stdout);

    GBDATA    *gb_species, *gb_sequence;
    char      *alignment_name;
    /* char      *error; */
    /* lenstring  key, annotation, sequence; */

    GB_begin_transaction(g);
    GB_change_my_security(g, 6, 0); /* disable all security */
    alignment_name = GBT_get_default_alignment(g);
    for (gb_species = GBT_first_marked_species(g);
         gb_species;
         gb_species = GBT_next_marked_species(gb_species))
    {
        GBDATA *gb_name = GB_search(gb_species, "name", GB_FIND);
        if (!gb_name) continue;

        gb_sequence = GBT_read_sequence(gb_species, alignment_name);
        if (!gb_sequence) continue;

        fprintf(key_file, "%s\n", GB_read_char_pntr(gb_name));
        fprintf(annotation_file, "NO ANNOTATION\f");    /* no annotations in this
                                                         * version */
        fprintf(sequence_file, "%s\n", GB_read_char_pntr(gb_sequence));
    }
    GB_commit_transaction(g);

    careful_close(key_file, keys_filename);
    careful_close(annotation_file, annotations_filename);
    careful_close(sequence_file, sequences_filename);
}

/* Retrieving selected entries from the database.  */

void
choose(char *choose_filename,
       char *keys_filename,
       char *annotations_filename,
       char *sequences_filename)
{
    FILE           *choose_file = careful_open(choose_filename, "r", stdin);
    FILE           *key_file = careful_open(keys_filename, "w", stdout);
    FILE           *annotation_file = careful_open(annotations_filename, "w", stdout);
    FILE           *sequence_file = careful_open(sequences_filename, "w", stdout);

    lenstring       key;
    GBDATA         *gb_species_data, *gb_species, *gb_sequence;
    char           *alignment_name;
    /* char           *error; */
    GBDATA         *gb_name;
    GB_begin_transaction(g);
    GB_change_my_security(g, 6, 0); /* disable all security */
    alignment_name = GBT_get_default_alignment(g);
    gb_species_data = GB_search(g, "species_data", GB_CREATE_CONTAINER);

    while (read_delimited_lenstring(&key, "\n", choose_file) != EOF) {
        key.text[key.len] = 0;
        gb_species = GBT_find_species_rel_species_data(gb_species_data, key.text);
        if (!gb_species) {
            fprintf(stderr, "Cannot find species %s\n", key.text);
        }
        gb_name = GB_search(gb_species, "name", GB_FIND);
        if (!gb_name)
            continue;
        gb_sequence = GBT_read_sequence(gb_species, alignment_name);
        if (!gb_sequence)
            continue;
        fprintf(key_file, "%s\n", GB_read_char_pntr(gb_name));
        fprintf(annotation_file, "\f"); /* no annotations in this
                         * version */
        fprintf(sequence_file, "%s\n", GB_read_char_pntr(gb_sequence));
        free(key.text);
    }

    GB_commit_transaction(g);

    careful_close(choose_file, choose_filename);
    careful_close(key_file, keys_filename);
    careful_close(annotation_file, annotations_filename);
    careful_close(sequence_file, sequences_filename);
}

void
list(char *keys_filename)
{
    FILE           *keys_file = careful_open(keys_filename, "w", stdout);


    GBDATA         *gb_species;
    /* char           *error; */
    /* lenstring       key, annotation, sequence; */
    GB_begin_transaction(g);
    GB_change_my_security(g, 6, 0); /* disable all security */

    for (gb_species = GBT_first_marked_species(g);
         gb_species;
         gb_species = GBT_next_marked_species(gb_species)) {
        GBDATA         *gb_name = GB_search(gb_species, "name", GB_FIND);
        if (!gb_name)
            continue;
        fprintf(keys_file, "%s\n", GB_read_char_pntr(gb_name));
    }

    GB_commit_transaction(g);
    careful_close(keys_file, keys_filename);
}


/*
 * Verifying that the index lists all the relevant entries in the database.
 */

/*
 * Traverse the database's hash table, comparing entries against those in our
 * hash table, built from the index.  Print error messages about keys which
 * don't appear in both places.
 */
void
verify_index()
{
    return;
}

/* Usage messages, dying, general utilities  */

void
give_usage()
{
    static const char usage[] = "\
GDBM access program for the Ale gene editor.\n\
Usage: %s GDBM-FILE\n\
    [--put] [-p]\n\
    [--get] [-g]\n\
    [--choose[=CHOOSE-FILE]] [-c[=CHOOSE-FILE]]\n\
    [--list] [-l]\n\
    [--verify-index]\n\
\n\
    [-k KEY-FILE]        [--keys KEY-FILE]\n\
    [-a ANNOTATION-FILE] [--annotations ANNOTATION-FILE]\n\
    [-s SEQUENCE-FILE]   [--sequences SEQUENCE-FILE]\n\
\n\
    [--help] [-h]\n\
    [--version] [-v]\n\
";

    fprintf(stderr, usage, progname);
    exit(1);
}

void
give_help()
{
    static const char help[] = "\
GDBM access program for the Ale gene editor.\n\
\n\
Usage: %s GDBM-FILE\n\
    [--put] [-p]\n\
    [--get] [-g]\n\
    [--choose[=CHOOSE-FILE]] [-c[=CHOOSE-FILE]]\n\
    [--list] [-l]\n\
    [--verify-index]\n\
\n\
    [-k KEY-FILE]        [--keys KEY-FILE]\n\
    [-a ANNOTATION-FILE] [--annotations ANNOTATION-FILE]\n\
    [-s SEQUENCE-FILE]   [--sequences SEQUENCE-FILE]\n\
\n\
    [--help] [-h]\n\
    [--version] [-v]\n\
\n\
  -p, --put\n\
        Add sequences to GDBM-FILE.  Read keys, annotations, and\n\
        sequences from KEY-FILE, ANNOTATION-FILE, and SEQUENCE-FILE.\n\
  -g, --get\n\
        Get all the sequences from GDBM-FILE.  Write keys,\n\
        annotations, and sequences to KEY-FILE, ANNOTATION-FILE, and\n\
        SEQUENCE-FILE.\n\
  -c, --choose[=CHOOSE-FILE]\n\
        Get selected sequences from GDBM-FILE.  Read keys from\n\
        CHOOSE-FILE, and write the information filed under that key to\n\
        KEY-FILE, ANNOTATION-FILE, and SEQUENCE-FILE.  (KEY-FILE will\n\
        usually be the same as CHOOSE-FILE.)  If CHOOSE-FILE is\n\
        omitted, read keys from stdin.\n\
  -l, --list\n\
        List the keys of all the sequences present in GDBM-FILE to\n\
        KEY-FILE.  Since the program maintains an index of all the\n\
        keys present, this ought to be faster than using the --get-all\n\
        option and sending the annotations and sequences to /dev/null.\n\
  --verify-index\n\
        Compare the index of sequences used by `--list' against those\n\
        actually present in GDBM-FILE.  If we find any mismatches,\n\
        list the keys of the sequences involved, and update the index\n\
        to reflect those found in GDBM-FILE.\n\
\n\
Keys in KEY-FILE and CHOOSE-FILE are terminated by newlines.\n\
Annotations in ANNOTATION-FILE are terminated by form feeds.\n\
Sequences in SEQUENCE-FILE are terminated by newlines.\n\
\n\
CHOOSE-FILE, KEY-FILE, ANNOTATION-FILE, and SEQUENCE-FILE all default\n\
to the standard input or output, as appropriate.\n\
\n\
If more than one of those is coming from or going to the standard\n\
input, the key comes first, then the annotation, and lastly the\n\
sequence.\n\
\n\
  -h, --help\n\
        Display this message.\n\
  -v, --version\n\
        Display the version number of this program.\n\
";

    printf(help, progname);
    exit(1);
}

void
give_version()
{
    fputs("Ale readers version " "V1.0" "\n", stderr);
    fputs("RCS revision: $Id$\n", stderr);

    exit(1);
}

void
print_and_die(const char *message)
{
    fprintf(stderr, "%s: %s\n", progname, message);

    if (g)
        GB_close(g);

    exit(2);
}

void
arbdb_print_and_die()
{
    fprintf(stderr, "%s\n", GB_get_error());


    if (g)
        GB_close(g);

    exit(2);
}
