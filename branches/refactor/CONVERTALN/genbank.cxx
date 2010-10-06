/* -------------- genbank related subroutines ----------------- */

#include <stdio.h>
#include <ctype.h>
#include "convert.h"
#include "global.h"

#define NOPERIOD    0
#define PERIOD      1

extern int warning_out;

/* ------------------------------------------------------------
 *  Function reinit_genbank().
 *  Initialize genbank entry.
 */
void cleanup_genbank() {
    freenull(data.gbk.locus);
    freenull(data.gbk.definition);
    freenull(data.gbk.accession);
    freenull(data.gbk.keywords);
    freenull(data.gbk.source);
    freenull(data.gbk.organism);
    for (int indi = 0; indi < data.gbk.numofref; indi++) {
        cleanup_reference(&data.gbk.reference[indi]);
    }
    freenull(data.gbk.reference);
    freenull(data.gbk.comments.orginf.source);
    freenull(data.gbk.comments.orginf.cc);
    freenull(data.gbk.comments.orginf.formname);
    freenull(data.gbk.comments.orginf.nickname);
    freenull(data.gbk.comments.orginf.commname);
    freenull(data.gbk.comments.orginf.hostorg);
    freenull(data.gbk.comments.seqinf.RDPid);
    freenull(data.gbk.comments.seqinf.gbkentry);
    freenull(data.gbk.comments.seqinf.methods);
    freenull(data.gbk.comments.others);
}

void reinit_genbank() {
    /* initialize genbank format */
    cleanup_genbank();

    data.gbk.locus = strdup("\n");
    data.gbk.definition = strdup("\n");
    data.gbk.accession = strdup("\n");
    data.gbk.keywords = strdup("\n");
    data.gbk.source = strdup("\n");
    data.gbk.organism = strdup("\n");
    data.gbk.numofref = 0;
    data.gbk.comments.orginf.exist = 0;
    data.gbk.comments.orginf.source = strdup("\n");
    data.gbk.comments.orginf.cc = strdup("\n");
    data.gbk.comments.orginf.formname = strdup("\n");
    data.gbk.comments.orginf.nickname = strdup("\n");
    data.gbk.comments.orginf.commname = strdup("\n");
    data.gbk.comments.orginf.hostorg = strdup("\n");
    data.gbk.comments.seqinf.exist = 0;
    data.gbk.comments.seqinf.RDPid = strdup("\n");
    data.gbk.comments.seqinf.gbkentry = strdup("\n");
    data.gbk.comments.seqinf.methods = strdup("\n");
    data.gbk.comments.others = NULL;
    data.gbk.comments.seqinf.comp5 = ' ';
    data.gbk.comments.seqinf.comp3 = ' ';
}

/* -----------------------------------------------------------
 *  Function genbank_in().
 *      Read in one genbank entry.
 */
char genbank_in(FILE_BUFFER fp) {
    char        line[LINESIZE], key[TOKENSIZE];
    const char *eof;
    char        eoen;

    eoen = ' ';
    /* end-of-entry, set to be 'y' after '//' is read */
    for (eof = Fgetline(line, LINESIZE, fp); eof != NULL && eoen != 'y';) {
        if (str0len(line) <= 1) {
            eof = Fgetline(line, LINESIZE, fp);
            continue;           /* empty line, skip */
        }

        genbank_key_word(line, 0, key, TOKENSIZE);

        eoen = 'n';

        if (str_equal(key, "LOCUS")) {
            eof = genbank_one_entry_in(data.gbk.locus, line, fp);
            if (str0len(data.gbk.locus) < 61)
                warning(14, "LOCUS data might be incomplete");
        }
        else if (str_equal(key, "DEFINITION")) {
            eof = genbank_one_entry_in(data.gbk.definition, line, fp);

            /* correct missing '.' at the end */
            terminate_with(data.gbk.definition, '.');
        }
        else if (str_equal(key, "ACCESSION")) {
            eof = genbank_one_entry_in(data.gbk.accession, line, fp);
            genbank_verify_accession();
        }
        else if (str_equal(key, "KEYWORDS")) {
            eof = genbank_one_entry_in(data.gbk.keywords, line, fp);
            genbank_verify_keywords();
        }
        else if (str_equal(key, "SOURCE")) {
            eof = genbank_source(line, fp);
            /* correct missing '.' at the end */
            terminate_with(data.gbk.source, '.');
            terminate_with(data.gbk.organism, '.');
        }
        else if (str_equal(key, "REFERENCE")) {
            eof = genbank_reference(line, fp);
        }
        else if (str_equal(key, "COMMENTS")) {
            eof = genbank_comments(line, fp);
        }
        else if (str_equal(key, "COMMENT")) {
            eof = genbank_comments(line, fp);
        }
        else if (str_equal(key, "ORIGIN")) {
            eof = genbank_origin(line, fp);
            eoen = 'y';
        }
        else {                  /* unidentified key word */
            eof = genbank_skip_unidentified(line, fp, 2);
        }
        /* except "ORIGIN", at the end of all the other cases,
         * a new line has already read in, so no further read
         * is necessary */
    }                           /* for loop to read an entry line by line */

    if (eoen == 'n') {
        warning(86, "Reach EOF before sequence data is read.");
        return (EOF);
    }
    if (eof == NULL)
        return (EOF);
    else
        return (EOF + 1);
}

/* -------------------------------------------------------------
 *  Function genbank_key_word().
 *      Get the key_word from line beginning at index.
 */
void genbank_key_word(const char *line, int index, char *key, int length) {
    // @@@ similar to embl_key_word and macke_key_word
    int indi, indj;

    if (line == NULL) {
        key[0] = '\0';
        return;
    }

    for (indi = index, indj = 0; (index - indi) < length && line[indi] != ' ' && line[indi] != '\t' && line[indi] != '\n' && line[indi] != '\0' && indi < 12; indi++, indj++)
        key[indj] = line[indi];
    key[indj] = '\0';
}

/* -------------------------------------------------------------
 *  Function genbank_comment_subkey_word().
 *      Get the subkey_word in comment lines beginning
 *          at index.
 */
int genbank_comment_subkey_word(char *line, int index, char *key, int length) {
    int indi, indj;

    if (line == NULL) {
        key[0] = '\0';
        return (index);
    }

    for (indi = index, indj = 0;
         (index - indi) < length && line[indi] != ':' && line[indi] != '\t' && line[indi] != '\n' && line[indi] != '\0' && line[indi] != '('; indi++, indj++) {
        key[indj] = line[indi];
    }

    if (line[indi] == ':')
        key[indj++] = ':';
    key[indj] = '\0';

    return (indi + 1);
}

/* ------------------------------------------------------------
 *  Function genbank_check_blanks().
 *      Check if there is (numb) of blanks at beginning
 *          of line.
 */
int genbank_check_blanks(char *line, int numb) {
    int blank = 1, indi, indk;

    for (indi = 0; blank && indi < numb; indi++) {
        if (line[indi] != ' ' && line[indi] != '\t')
            blank = 0;
        if (line[indi] == '\t') {
            indk = indi / 8 + 1;
            indi = 8 * indk + 1;
        }
    }

    return (blank);
}

/* ----------------------------------------------------------------
 *  Function genbank_continue_line().
 *      if there are (numb) of blanks at the beginning
 *          of line, it is a continue line of the
 *          current command.
 */
char *genbank_continue_line(char*& Str, char *line, int numb, FILE_BUFFER fp) {
    /* numb = number of blanks needed to define a continue line */
    int   ind;
    char *eof, temp[LINESIZE];

    /* check continue lines */
    for (eof = Fgetline(line, LINESIZE, fp);
         eof != NULL && (genbank_check_blanks(line, numb) || line[0] == '\n');
         eof = Fgetline(line, LINESIZE, fp))
    {
        if (line[0] == '\n')
            continue;           /* empty line is allowed */
        /* remove end-of-line, if there is any */
        ind = Skip_white_space(line, 0);
        strcpy(temp, (line + ind));
        skip_eolnl_and_append_spaced(Str, temp);
    }                           /* end of continue line checking */

    return (eof);
}

/* ------------------------------------------------------------
 *  Function genbank_one_entry_in().
 *      Read in genbank one entry lines.
 */
char *genbank_one_entry_in(char*& datastring, char *line, FILE_BUFFER fp) {
    int   index;
    char *eof;

    index = Skip_white_space(line, 12);
    freenull(datastring);
    datastring = str0dup(line + index);
    eof = genbank_continue_line(datastring, line, 12, fp);

    return (eof);
}

/* ------------------------------------------------------------
 *  Function genbank_one_comment_entry().
 *      Read in one genbank sub-entry in comments lines.
 */
char *genbank_one_comment_entry(char*& datastring, char *line, int start_index, FILE_BUFFER fp) {
    int   index;
    char *eof;

    index = Skip_white_space(line, start_index);
    freenull(datastring);
    datastring = str0dup(line + index);
    eof = genbank_continue_line(datastring, line, 20, fp);
    return (eof);
}

/* --------------------------------------------------------------
 *  Function genbank_source()
 *      Read in genbank SOURCE lines and also ORGANISM
 *          lines.
 */
char *genbank_source(char *line, FILE_BUFFER fp) {
    int   index;
    char *eof;
    char *dummy, key[TOKENSIZE];

    eof = genbank_one_entry_in(data.gbk.source, line, fp);
    genbank_key_word(line, 2, key, TOKENSIZE);
    if (str_equal(key, "ORGANISM")) {
        index = Skip_white_space(line, 12);
        data.gbk.organism = str0dup(line + index);
        dummy = strdup("\n");
        eof = genbank_continue_line(dummy, line, 12, fp);
        freenull(dummy);
    }
    return (eof);
}

/* --------------------------------------------------------------
 *  Function genbank_reference().
 *      Read in genbank REFERENCE lines.
 */
char *genbank_reference(char *line, FILE_BUFFER fp) {
#define AUTH 0
#define TIT  1
#define JOUR 2
    char *eof, key[TOKENSIZE];
    int   refnum;
    int   acount = 0, tcount = 0, jcount = 0, scount = 0;

    ASSERT_RESULT(int, 1, sscanf(line + 12, "%d", &refnum));
    if (refnum <= data.gbk.numofref) {
        warningf(17, "Might redefine reference %d", refnum);
        eof = genbank_skip_unidentified(line, fp, 12);
    }
    else {
        data.gbk.numofref = refnum;
        data.gbk.reference = (Reference *) Reallocspace((char *)data.gbk.reference, (unsigned)(sizeof(Reference) * (data.gbk.numofref)));
        /* initialize the buffer */
        reinit_reference(&(data.gbk.reference[refnum - 1]));
        eof = genbank_one_entry_in(data.gbk.reference[refnum - 1].ref, line, fp);
    }
    /* find the reference listings */
    for (; eof != NULL && line[0] == ' ' && line[1] == ' ';) {
        /* find the key word */
        genbank_key_word(line, 2, key, TOKENSIZE);
        /* skip white space */
        if (str_equal(key, "AUTHORS")) {
            eof = genbank_one_entry_in(data.gbk.reference[refnum - 1].author, line, fp);

            /* add '.' if missing at the end */
            terminate_with(data.gbk.reference[refnum - 1].author, '.');

            if (acount == 0) acount = 1;
            else {
                warningf(10, "AUTHORS of REFERENCE %d is redefined", refnum);
            }
        }
        else if (str_equal(key, "TITLE")) {
            eof = genbank_one_entry_in(data.gbk.reference[refnum - 1].title, line, fp);
            if (tcount == 0) tcount = 1;
            else {
                warningf(11, "TITLE of REFERENCE %d is redefined", refnum);
            }
        }
        else if (str_equal(key, "JOURNAL")) {
            eof = genbank_one_entry_in(data.gbk.reference[refnum - 1].journal, line, fp);

            if (jcount == 0) jcount = 1;
            else {
                warningf(12, "JOURNAL of REFERENCE %d is redefined", refnum);
            }
        }
        else if (str_equal(key, "STANDARD")) {
            eof = genbank_one_entry_in(data.gbk.reference[refnum - 1].standard, line, fp);

            if (scount == 0) scount = 1;
            else {
                warningf(13, "STANDARD of REFERENCE %d is redefined", refnum);
            }
        }
        else {
            warningf(18, "Unidentified REFERENCE subkeyword: %s#", key);
            eof = genbank_skip_unidentified(line, fp, 12);
        }
    }                           /* for loop */
    return (eof);
}

/* --------------------------------------------------------------
 *  Function genbank_comments().
 *      Read in genbank COMMENTS lines.
 */
const char *genbank_comments(char *line, FILE_BUFFER fp) {
    int         index, indi, ptr;
    const char *eof;
    char        key[TOKENSIZE];

    if (str0len(line) <= 12) {
        if ((eof = Fgetline(line, LINESIZE, fp)) == NULL)
            return (eof);
    }
    /* make up data to match the logic reasoning for next statement */
    for (indi = 0; indi < 12; line[indi++] = ' ') ;
    eof = "NONNULL";

    for (; eof != NULL && (genbank_check_blanks(line, 12) || line[0] == '\n');) {
        if (line[0] == '\n') {  /* skip empty line */
            eof = Fgetline(line, LINESIZE, fp);
            continue;
        }

        ptr = index = 12;

        index = Skip_white_space(line, index);
#if defined(DEBUG)
        if (index >= TOKENSIZE) {
            printf("big index %i after Skip_white_space\n", index);
        }
#endif /* DEBUG */
        index = genbank_comment_subkey_word(line, index, key, TOKENSIZE);
#if defined(DEBUG)
        if (index >= TOKENSIZE) {
            printf("big index %i after genbank_comment_subkey_word\n", index);
        }
#endif /* DEBUG */

        if (str_equal(key, "Source of strain:")) {
            eof = genbank_one_comment_entry(data.gbk.comments.orginf.source, line, index, fp);
        }
        else if (str_equal(key, "Culture collection:")) {
            eof = genbank_one_comment_entry(data.gbk.comments.orginf.cc, line, index, fp);
        }
        else if (str_equal(key, "Former name:")) {
            eof = genbank_one_comment_entry(data.gbk.comments.orginf.formname, line, index, fp);
        }
        else if (str_equal(key, "Alternate name:")) {
            eof = genbank_one_comment_entry(data.gbk.comments.orginf.nickname, line, index, fp);
        }
        else if (str_equal(key, "Common name:")) {
            eof = genbank_one_comment_entry(data.gbk.comments.orginf.commname, line, index, fp);
        }
        else if (str_equal(key, "Host organism:")) {
            eof = genbank_one_comment_entry(data.gbk.comments.orginf.hostorg, line, index, fp);
        }
        else if (str_equal(key, "RDP ID:")) {
            eof = genbank_one_comment_entry(data.gbk.comments.seqinf.RDPid, line, index, fp);
        }
        else if (str_equal(key, "Corresponding GenBank entry:")) {
            eof = genbank_one_comment_entry(data.gbk.comments.seqinf.gbkentry, line, index, fp);
        }
        else if (str_equal(key, "Sequencing methods:")) {
            eof = genbank_one_comment_entry(data.gbk.comments.seqinf.methods, line, index, fp);
        }
        else if (str_equal(key, "5' end complete:")) {
            scan_token_or_die(line + index, key, &fp);
            if (key[0] == 'Y')
                data.gbk.comments.seqinf.comp5 = 'y';
            else
                data.gbk.comments.seqinf.comp5 = 'n';
            eof = Fgetline(line, LINESIZE, fp);
        }
        else if (str_equal(key, "3' end complete:")) {
            scan_token_or_die(line + index, key, &fp);
            if (key[0] == 'Y')
                data.gbk.comments.seqinf.comp3 = 'y';
            else
                data.gbk.comments.seqinf.comp3 = 'n';
            eof = Fgetline(line, LINESIZE, fp);
        }
        else if (str_equal(key, "Sequence information ")) {
            /* do nothing */
            data.gbk.comments.seqinf.exist = 1;
            eof = Fgetline(line, LINESIZE, fp);
        }
        else if (str_equal(key, "Organism information")) {
            /* do nothing */
            data.gbk.comments.orginf.exist = 1;
            eof = Fgetline(line, LINESIZE, fp);
        }
        else {                  /* other comments */
            ca_assert(ptr == 12);
            if (data.gbk.comments.others == NULL) {
                data.gbk.comments.others = str0dup(line + ptr);
            }
            else {
                Append(data.gbk.comments.others, line + ptr);
            }

            eof = Fgetline(line, LINESIZE, fp);
        }
    }                           /* for loop */

    return (eof);
}

/* --------------------------------------------------------------
 *   Function genbank_origin().
 *       Read in genbank sequence data.
 */
char *genbank_origin(char *line, FILE_BUFFER fp) {
    char *eof;
    int   index;

    data.seq_length = 0;
    data.sequence[data.seq_length] = '\0';      // needed if sequence data is empty

    /* read in whole sequence data */
    for (eof = Fgetline(line, LINESIZE, fp); eof != NULL && line[0] != '/' && line[1] != '/'; eof = Fgetline(line, LINESIZE, fp)) {
        /* empty line, skip */
        if (str0len(line) <= 1)
            continue;
        for (index = 9; line[index] != '\n' && line[index] != '\0'; index++) {
            if (line[index] != ' ' && data.seq_length >= data.max) {
                data.max += 100;

                data.sequence = Reallocspace(data.sequence, (unsigned)(sizeof(char) * data.max));
            }
            if (line[index] != ' ')
                data.sequence[data.seq_length++] = line[index];
        }
        if (data.seq_length >= data.max) {
            data.max += 100;

            data.sequence = Reallocspace(data.sequence, (unsigned)(sizeof(char) * data.max));
        }
        data.sequence[data.seq_length] = '\0';
    }

    return (eof);
}

/* ---------------------------------------------------------------
 *  Function genbank_skip_unidentified().
 *      Skip the lines of unidentified keyword.
 */
char *genbank_skip_unidentified(char *line, FILE_BUFFER fp, int blank_num) {
    char *eof;
    for (eof = Fgetline(line, LINESIZE, fp); eof != NULL && genbank_check_blanks(line, blank_num); eof = Fgetline(line, LINESIZE, fp)) ;
    return (eof);
}

/* ---------------------------------------------------------------
 *   Function genbank_verify_accession().
 *       Verify accession information.
 */
void genbank_verify_accession() {
    int indi, len, count, remainder;

    if (str_equal(data.gbk.accession, "No information\n"))
        return;
    len = str0len(data.gbk.accession);
    if ((len % 7) != 0) {
        if (warning_out)
            fprintf(stderr, "\nACCESSION: %s", data.gbk.accession);
        warning(136, "Each accession number should be a six-character identifier.");
    }
    for (indi = count = 0; indi < len - 1; indi++) {
        remainder = indi % 7;
        switch (remainder) {
            case 0:
                count++;
                if (count > 9) {
                    if (warning_out)
                        fprintf(stderr, "\nACCESSION: %s", data.gbk.accession);
                    warning(137, "No more than 9 accession numbers are allowed in ACCESSION line.");
                    data.gbk.accession[indi - 1] = '\n';
                    data.gbk.accession[indi] = '\0';
                    data.gbk.accession = Reallocspace(data.gbk.accession, (unsigned)(sizeof(char) * indi));
                    return;
                }
                if (!isalpha(data.gbk.accession[indi])) {
                    warningf(138, "The %d(th) accession number must start with a letter.", count);
                }
                break;
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
                if (!isdigit(data.gbk.accession[indi])) {
                    warningf(140, "The last 5 characters of the %d(th) accession number should be all digits.", count);
                }
                break;
            case 6:
                if ((indi != (len - 1) && data.gbk.accession[indi] != ' ')
                    || (indi == (len - 1) && data.gbk.accession[indi] != '\n')) {
                    if (warning_out)
                        fprintf(stderr, "\nACCESSION: %s", data.gbk.accession);
                    warning(139, "Accession numbers should be separated by a space.");
                    data.gbk.accession[indi] = ' ';
                }
                break;
            default:;
        }
    }                           /* check every char of ACCESSION line. */
}

/* ------------------------------------------------------------------
 *   Function genbank_verify_keywords().
 *       Verify keywords.
 */
void genbank_verify_keywords() {
    int indi, count, len;

    /* correct missing '.' at the end */
    terminate_with(data.gbk.keywords, '.');

    for (indi = count = 0, len = str0len(data.gbk.keywords); indi < len; indi++)
        if (data.gbk.keywords[indi] == '.')
            count++;

    if (count != 1) {
        if (warning_out)
            fprintf(stderr, "\nKEYWORDS: %s", data.gbk.keywords);
        warning(141, "No more than one period is allowed in KEYWORDS line.");
    }
}

/* ---------------------------------------------------------------
 *   Function genbank_in_locus().
 *       Read in next genbank locus and sequence only.
 *       For use of converting to simple format(read in only simple
 *       information instead of whole records).
 */
char genbank_in_locus(FILE_BUFFER fp) {
    char  line[LINESIZE], key[TOKENSIZE];
    char *eof, eoen;

    eoen = ' ';                 /* end-of-entry, set to be 'y' after '//' is read */
    for (eof = Fgetline(line, LINESIZE, fp); eof != NULL && eoen != 'y';) {
        genbank_key_word(line, 0, key, TOKENSIZE);
        if (str_equal(key, "ORIGIN")) {
            eof = genbank_origin(line, fp);
            eoen = 'y';
        }
        else if (str_equal(key, "LOCUS")) {
            eof = genbank_one_entry_in(data.gbk.locus, line, fp);
        }
        else
            eof = Fgetline(line, LINESIZE, fp);
    }                           /* for loop to read an entry line by line */

    if (eoen == 'n')
        throw_error(9, "Reach EOF before one entry is read");

    if (eof == NULL)
        return (EOF);
    else
        return (EOF + 1);
}

/* ---------------------------------------------------------------
 *   Function genbank_out().
 *       Output in a genbank format.
 */
void genbank_out(FILE * fp) {
    int indi, length;
    int base_a, base_t, base_g, base_c, base_other;

    /* Assume the last char of each field is '\n' */
    genbank_out_one_entry(fp, data.gbk.locus, "LOCUS       ", SEPNODEFINED, "", NOPERIOD);
    genbank_out_one_entry(fp, data.gbk.definition, "DEFINITION  ", SEPNODEFINED, "", PERIOD);
    genbank_out_one_entry(fp, data.gbk.accession, "ACCESSION   ", SEPNODEFINED, "", NOPERIOD);
    genbank_out_one_entry(fp, data.gbk.keywords, "KEYWORDS    ", SEPDEFINED, ";", PERIOD);

    if (str0len(data.gbk.source) > 1) {
        fprintf(fp, "SOURCE      ");
        genbank_print_lines(fp, data.gbk.source, SEPNODEFINED, "");
        if (str0len(data.gbk.organism) > 1) {
            fprintf(fp, "  ORGANISM  ");
            genbank_print_lines(fp, data.gbk.organism, SEPNODEFINED, "");
        }
        else
            fprintf(fp, "  ORGANISM  No information.\n");
    }
    else if (str0len(data.gbk.organism) > 1) {
        fprintf(fp, "SOURCE      No information.\n  ORGANISM  ");
        genbank_print_lines(fp, data.gbk.organism, SEPNODEFINED, "");
    }
    else
        fprintf(fp, "SOURCE      No information.\n  ORGANISM  No information.\n");

    if (data.gbk.numofref > 0) {
        for (indi = 0; indi < data.gbk.numofref; indi++) {
            if (str0len(data.gbk.reference[indi].ref) > 1) {
                fprintf(fp, "REFERENCE   ");
                genbank_print_lines(fp, data.gbk.reference[indi].ref, SEPNODEFINED, "");
            }
            else
                fprintf(fp, "REFERENCE   %d\n", indi + 1);

            genbank_out_one_entry(fp, data.gbk.reference[indi].author, "  AUTHORS   ", SEPDEFINED, " ", NOPERIOD);

            if (str0len(data.gbk.reference[indi].title) > 1) {
                fprintf(fp, "  TITLE     ");
                genbank_print_lines(fp, data.gbk.reference[indi].title, SEPNODEFINED, "");
            }

            genbank_out_one_entry(fp, data.gbk.reference[indi].journal, "  JOURNAL   ", SEPNODEFINED, "", NOPERIOD);

            genbank_out_one_entry(fp, data.gbk.reference[indi].standard, "  STANDARD  ", SEPNODEFINED, "", NOPERIOD);
        }                       /* subkey loop */
    }
    else {
        fprintf(fp, "REFERENCE   1\n");
        fprintf(fp, "  AUTHORS   No information\n");
        fprintf(fp, "  JOURNAL   No information\n");
        fprintf(fp, "  TITLE     No information\n");
        fprintf(fp, "  STANDARD  No information\n");
    }

    if (data.gbk.comments.orginf.exist == 1 || data.gbk.comments.seqinf.exist == 1 || str0len(data.gbk.comments.others) > 0) {
        fprintf(fp, "COMMENTS    ");

        if (data.gbk.comments.orginf.exist == 1) {
            fprintf(fp, "Organism information\n");

            genbank_out_one_comment(fp, data.gbk.comments.orginf.source, "Source of strain: ", COMMSKINDENT, COMMCNINDENT);

            genbank_out_one_comment(fp, data.gbk.comments.orginf.cc, "Culture collection: ", COMMSKINDENT, COMMCNINDENT);

            genbank_out_one_comment(fp, data.gbk.comments.orginf.formname, "Former name: ", COMMSKINDENT, COMMCNINDENT);

            genbank_out_one_comment(fp, data.gbk.comments.orginf.nickname, "Alternate name: ", COMMSKINDENT, COMMCNINDENT);

            genbank_out_one_comment(fp, data.gbk.comments.orginf.commname, "Common name: ", COMMSKINDENT, COMMCNINDENT);

            genbank_out_one_comment(fp, data.gbk.comments.orginf.hostorg, "Host organism: ", COMMSKINDENT, COMMCNINDENT);

            if (data.gbk.comments.seqinf.exist == 1 || str0len(data.gbk.comments.others) > 0)
                fprintf(fp, "            ");
        }                       /* organism information */

        if (data.gbk.comments.seqinf.exist == 1) {
            fprintf(fp, "Sequence information (bases 1 to %d)\n", data.seq_length);
        }

        genbank_out_one_comment(fp, data.gbk.comments.seqinf.RDPid, "RDP ID: ", COMMSKINDENT, COMMCNINDENT);

        genbank_out_one_comment(fp, data.gbk.comments.seqinf.gbkentry, "Corresponding GenBank entry: ", COMMSKINDENT, COMMCNINDENT);

        genbank_out_one_comment(fp, data.gbk.comments.seqinf.methods, "Sequencing methods: ", COMMSKINDENT, COMMCNINDENT);

        if (data.gbk.comments.seqinf.comp5 == 'n')
            fprintf(fp, "              5' end complete: No\n");

        else if (data.gbk.comments.seqinf.comp5 == 'y')
            fprintf(fp, "              5' end complete: Yes\n");

        if (data.gbk.comments.seqinf.comp3 == 'n')
            fprintf(fp, "              3' end complete: No\n");

        else if (data.gbk.comments.seqinf.comp3 == 'y')
            fprintf(fp, "             3' end complete: Yes\n");

        /* print 12 spaces of the first line */
        if (str0len(data.gbk.comments.others) > 0)
            fprintf(fp, "            ");

        if (str0len(data.gbk.comments.others) > 0) {
            length = str0len(data.gbk.comments.others);
            for (indi = 0; indi < length; indi++) {
                fprintf(fp, "%c", data.gbk.comments.others[indi]);

                /* if another line, print 12 spaces first */
                if (data.gbk.comments.others[indi] == '\n' && data.gbk.comments.others[indi + 1] != '\0')

                    fprintf(fp, "            ");
            }
        }                       /* other comments */
    }                           /* comment */

    count_base(&base_a, &base_t, &base_g, &base_c, &base_other);

    /* don't write 0 others in this base line */
    if (base_other > 0)
        fprintf(fp, "BASE COUNT  %6d a %6d c %6d g %6d t %6d others\n", base_a, base_c, base_g, base_t, base_other);
    else
        fprintf(fp, "BASE COUNT  %6d a %6d c %6d g %6d t\n", base_a, base_c, base_g, base_t);

    genbank_out_origin(fp);
}

/* ------------------------------------------------------------
 *  Function genbank_out_one_entry().
 *      Print out key and Str if Str length > 1
 *      otherwise print key and "No information" w/wo
 *      period at the end depending on flag period.
 */
void genbank_out_one_entry(FILE * fp, char *Str, const char *key, int flag, const char *patterns, int period) {
    if (str0len(Str) > 1) {
        fprintf(fp, "%s", key);
        genbank_print_lines(fp, Str, flag, patterns);
    }
    else if (period)
        fprintf(fp, "%sNo information.\n", key);
    else
        fprintf(fp, "%sNo information\n", key);
}

/* -------------------------------------------------------------
 *  Function genbank_out_one_comment().
 *      print out one genbank comment sub-keyword.
 */
void genbank_out_one_comment(FILE * fp, char *Str, const char *key, int skindent, int cnindent) {
    /* skindent = subkeyword indent */
    /* cnindent = continue line indent */
    if (str0len(Str) > 1)
        genbank_print_comment(fp, key, Str, skindent, cnindent);
}

/* --------------------------------------------------------------
 *  Function genbank_print_lines().
 *      Print one genbank line, wrap around if over
 *          column 80.
 */
void genbank_print_lines(FILE * fp, char *Str, int flag, const char *separators) {
    int first_time = 1, indi, indj, indk;

    int ibuf, len;

    len = str0len(Str) - 1;
    /* indi: first char of the line */
    /* num of char, excluding the first char, of the line */
    for (indi = 0; indi < len; indi += (indj + 1)) {
        indj = GBMAXCHAR;
        if ((str0len(Str + indi)) > GBMAXCHAR) {
            /* search for proper termination of a line */
            ibuf = indj;

            if (flag) {
                for (; indj > 0 && !occurs_in(Str[indj + indi], separators); indj--) ;
            }
            else {
                for (; indj > 0 && is_word_char(Str[indj + indi]); indj--) ;
            }

            if (indj == 0) indj = ibuf;
            else if (Str[indi + indj + 1] == ' ') indj++;

            /* print left margin */
            if (!first_time) fprintf(fp, "            ");
            else first_time = 0;

            for (indk = 0; indk < indj; indk++) fprintf(fp, "%c", Str[indi + indk]);

            /* leave out the last space, if there is any */
            if (Str[indi + indj] != ' ' && Str[indi + indj] != '\n') fprintf(fp, "%c", Str[indi + indj]);
            fprintf(fp, "\n");
        }
        else if (first_time)
            fprintf(fp, "%s", Str + indi);
        else
            fprintf(fp, "            %s", Str + indi);
    }
}

/* --------------------------------------------------------------
 *  Function genbank_print_comment().
 *      Print one genbank line, wrap around if over
 *          column 80.
 */
void genbank_print_comment(FILE * fp, const char *key, char *Str, int offset, int indent) {
    int first_time = 1, indi, indj, indk, indl;

    int len;

    len = str0len(Str) - 1;
    for (indi = 0; indi < len; indi += (indj + 1)) {
        if (first_time)
            indj = GBMAXCHAR - offset - str0len(key) - 1;
        else
            indj = GBMAXCHAR - offset - indent - 1;

        fprintf(fp, "            ");

        if (!first_time) {
            for (indl = 0; indl < (offset + indent); indl++)
                fprintf(fp, " ");
        }
        else {
            for (indl = 0; indl < offset; indl++)
                fprintf(fp, " ");
            fprintf(fp, "%s", key);
            first_time = 0;
        }
        if (str0len(Str + indi) > indj) {
            /* search for proper termination of a line */
            for (; indj >= 0 && is_word_char(Str[indj + indi]); indj--) ;

            /* print left margin */
            if (Str[indi] == ' ')
                indk = 1;
            else
                indk = 0;

            for (; indk < indj; indk++)
                fprintf(fp, "%c", Str[indi + indk]);

            /* leave out the last space, if there is any */
            if (Str[indi + indj] != ' ')
                fprintf(fp, "%c", Str[indi + indj]);
            fprintf(fp, "\n");
        }
        else
            fprintf(fp, "%s", Str + indi);
    }                           /* for each char */
}

/* ---------------------------------------------------------------
 *   Function genbank_out_origin().
 *       Output sequence data in genbank format.
 */
void genbank_out_origin(FILE * fp) {
    int indi, indj, indk;

    fprintf(fp, "ORIGIN\n");

    for (indi = 0, indj = 0, indk = 1; indi < data.seq_length; indi++) {
        if ((indk % 60) == 1)
            fprintf(fp, "   %6d ", indk);
        fprintf(fp, "%c", data.sequence[indi]);
        indj++;

        /* blank space follows every 10 bases, but not before '\n' */
        if ((indk % 60) == 0) {
            fprintf(fp, "\n");
            indj = 0;
        }
        else if (indj == 10 && indi != (data.seq_length - 1)) {
            fprintf(fp, " ");
            indj = 0;
        }
        indk++;
    }

    if ((indk % 60) != 1)
        fprintf(fp, "\n");
    fprintf(fp, "//\n");
}

/* -----------------------------------------------------------
 *   Function genbank_to_genbank().
 *       Convert from genbank to genbank.
 */
void genbank_to_genbank(const char *inf, const char *outf) {
    FILE        *IFP = open_input_or_die(inf);
    FILE_BUFFER  ifp = create_FILE_BUFFER(inf, IFP);
    FILE        *ofp = open_output_or_die(outf);

    reinit_genbank();
    while (genbank_in(ifp) != EOF) {
        data.numofseq++;
        genbank_out(ofp);
        reinit_genbank();
    }

    log_processed(data.numofseq);
    destroy_FILE_BUFFER(ifp);
    fclose(ofp);
}

/* -----------------------------------------------------------
 *   Function CRAP_init_reference().
 *       Init. new reference record(init. value is "\n").
 */

void cleanup_reference(Reference *ref) {
    freenull(ref->ref);
    freenull(ref->author);
    freenull(ref->journal);
    freenull(ref->title);
    freenull(ref->standard);
}

void init_reference(Reference *ref) {
    ref->ref      = strdup("\n");
    ref->author   = strdup("\n");
    ref->journal  = strdup("\n");
    ref->title    = strdup("\n");
    ref->standard = strdup("\n");
}

void reinit_reference(Reference *ref) {
    cleanup_reference(ref);
    init_reference(ref);
}

