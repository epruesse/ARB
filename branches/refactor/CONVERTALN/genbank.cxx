/* -------------- genbank related subroutines ----------------- */

#include <stdio.h>
#include <ctype.h>
#include "global.h"

#define NOPERIOD    0
#define PERIOD      1

extern int warning_out;

static void cleanup_reference(GenbankRef& ref) {
    freenull(ref.ref);
    freenull(ref.author);
    freenull(ref.journal);
    freenull(ref.title);
    freenull(ref.standard);
}

void init_reference(GenbankRef& ref) {
    ref.ref      = no_content();
    ref.author   = no_content();
    ref.journal  = no_content();
    ref.title    = no_content();
    ref.standard = no_content();
}
void reinit_reference(GenbankRef& ref) {
    cleanup_reference(ref);
    init_reference(ref);
}

void init_comments(Comments& comments) {
    OrgInfo& orginf = comments.orginf;
    orginf.exists = false;
    orginf.source   = no_content();
    orginf.cultcoll = no_content();
    orginf.formname = no_content();
    orginf.nickname = no_content();
    orginf.commname = no_content();
    orginf.hostorg  = no_content();

    SeqInfo& seqinf = comments.seqinf;
    seqinf.exists = false;
    seqinf.RDPid    = no_content();
    seqinf.gbkentry = no_content();
    seqinf.methods  = no_content();
    seqinf.comp5    = ' ';
    seqinf.comp3    = ' ';

    comments.others = NULL;
}

void cleanup_comments(Comments& comments) {
    OrgInfo& orginf = comments.orginf;
    freenull(orginf.source);
    freenull(orginf.cultcoll);
    freenull(orginf.formname);
    freenull(orginf.nickname);
    freenull(orginf.commname);
    freenull(orginf.hostorg);

    SeqInfo& seqinf = comments.seqinf;
    freenull(seqinf.RDPid);
    freenull(seqinf.gbkentry);
    freenull(seqinf.methods);

    freenull(comments.others);
}

void cleanup_genbank() {
    GenBank& gbk = data.gbk;

    freenull(gbk.locus);
    freenull(gbk.definition);
    freenull(gbk.accession);
    freenull(gbk.keywords);
    freenull(gbk.source);
    freenull(gbk.organism);
    for (int indi = 0; indi < gbk.numofref; indi++) {
        cleanup_reference(gbk.reference[indi]);
    }
    freenull(gbk.reference);
    cleanup_comments(gbk.comments);
}

void reinit_genbank() {
    /* initialize genbank format */
    cleanup_genbank();

    GenBank& gbk = data.gbk;
    
    gbk.locus      = no_content();
    gbk.definition = no_content();
    gbk.accession  = no_content();
    gbk.keywords   = no_content();
    gbk.source     = no_content();
    gbk.organism   = no_content();
    gbk.numofref   = 0;

    init_comments(gbk.comments);
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
            if (!data.gbk.locus_contains_date())
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

    for (indi = index, indj = 0; (index - indi) < length && line[indi] != ' ' && line[indi] != '\t' && line[indi] != '\n' && line[indi] != '\0' && indi < GBINDENT; indi++, indj++)
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
    freedup(datastring, line+Skip_white_space(line, GBINDENT));
    return genbank_continue_line(datastring, line, GBINDENT, fp);
}

/* ------------------------------------------------------------
 *  Function genbank_one_comment_entry().
 *      Read in one genbank sub-entry in comments lines.
 */
char *genbank_one_comment_entry(char*& datastring, char *line, int start_index, FILE_BUFFER fp) {
    freedup(datastring, line + Skip_white_space(line, start_index));
    return genbank_continue_line(datastring, line, 20, fp);
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
        index = Skip_white_space(line, GBINDENT);
        data.gbk.organism = nulldup(line + index);
        dummy = no_content();
        eof = genbank_continue_line(dummy, line, GBINDENT, fp);
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

    ASSERT_RESULT(int, 1, sscanf(line + GBINDENT, "%d", &refnum));
    if (refnum <= data.gbk.numofref) {
        warningf(17, "Might redefine reference %d", refnum);
        eof = genbank_skip_unidentified(line, fp, GBINDENT);
    }
    else {
        data.gbk.numofref = refnum;
        data.gbk.reference = (GenbankRef *) Reallocspace((char *)data.gbk.reference, (unsigned)(sizeof(GenbankRef) * (data.gbk.numofref)));
        /* initialize the buffer */
        init_reference(data.gbk.reference[refnum - 1]);
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
            eof = genbank_skip_unidentified(line, fp, GBINDENT);
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

    if (str0len(line) <= GBINDENT) {
        if ((eof = Fgetline(line, LINESIZE, fp)) == NULL)
            return (eof);
    }
    /* make up data to match the logic reasoning for next statement */
    for (indi = 0; indi < GBINDENT; line[indi++] = ' ') {}
    eof = "NONNULL";

    OrgInfo& orginf = data.gbk.comments.orginf;
    SeqInfo& seqinf = data.gbk.comments.seqinf;

    for (; eof != NULL && (genbank_check_blanks(line, GBINDENT) || line[0] == '\n');) {
        if (line[0] == '\n') {  /* skip empty line */
            eof = Fgetline(line, LINESIZE, fp);
            continue;
        }

        ptr = index = GBINDENT;

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
            eof = genbank_one_comment_entry(orginf.source, line, index, fp);
        }
        else if (str_equal(key, "Culture collection:")) {
            eof = genbank_one_comment_entry(orginf.cultcoll, line, index, fp);
        }
        else if (str_equal(key, "Former name:")) {
            eof = genbank_one_comment_entry(orginf.formname, line, index, fp);
        }
        else if (str_equal(key, "Alternate name:")) {
            eof = genbank_one_comment_entry(orginf.nickname, line, index, fp);
        }
        else if (str_equal(key, "Common name:")) {
            eof = genbank_one_comment_entry(orginf.commname, line, index, fp);
        }
        else if (str_equal(key, "Host organism:")) {
            eof = genbank_one_comment_entry(orginf.hostorg, line, index, fp);
        }
        else if (str_equal(key, "RDP ID:")) {
            eof = genbank_one_comment_entry(seqinf.RDPid, line, index, fp);
        }
        else if (str_equal(key, "Corresponding GenBank entry:")) {
            eof = genbank_one_comment_entry(seqinf.gbkentry, line, index, fp);
        }
        else if (str_equal(key, "Sequencing methods:")) {
            eof = genbank_one_comment_entry(seqinf.methods, line, index, fp);
        }
        else if (str_equal(key, "5' end complete:")) {
            scan_token_or_die(line + index, key, &fp);
            if (key[0] == 'Y')
                seqinf.comp5 = 'y';
            else
                seqinf.comp5 = 'n';
            eof = Fgetline(line, LINESIZE, fp);
        }
        else if (str_equal(key, "3' end complete:")) {
            scan_token_or_die(line + index, key, &fp);
            if (key[0] == 'Y')
                seqinf.comp3 = 'y';
            else
                seqinf.comp3 = 'n';
            eof = Fgetline(line, LINESIZE, fp);
        }
        else if (str_equal(key, "Sequence information ")) {
            /* do nothing */
            seqinf.exists = true;
            eof = Fgetline(line, LINESIZE, fp);
        }
        else if (str_equal(key, "Organism information")) {
            /* do nothing */
            orginf.exists = true;
            eof = Fgetline(line, LINESIZE, fp);
        }
        else {                  /* other comments */
            ca_assert(ptr == GBINDENT);
            if (data.gbk.comments.others == NULL) {
                data.gbk.comments.others = nulldup(line + ptr);
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
    for (eof = Fgetline(line, LINESIZE, fp); eof != NULL && genbank_check_blanks(line, blank_num); eof = Fgetline(line, LINESIZE, fp)) {}
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

void genbank_out_one_reference(FILE * fp, const GenbankRef& gbk_ref, int gbk_ref_num, bool SIMULATE_BUG) {
    WrapMode wrapWords(true);

    {
        const char *r = gbk_ref.ref;
        char        refnum[TOKENSIZE];

        if (!has_content(r)) {
            sprintf(refnum, "%d\n", gbk_ref_num);
            r = refnum;
        }
        genbank_out_one_entry(fp, "REFERENCE   ", r, wrapWords, NOPERIOD);
    }

    genbank_out_one_entry(fp, "  AUTHORS   ", gbk_ref.author, WrapMode(" "), NOPERIOD);

    if (SIMULATE_BUG) { // @@@
        // skip title completely, if there is no information
        if (has_content(gbk_ref.title)) {
            genbank_out_one_entry(fp, "  TITLE     ", gbk_ref.title, wrapWords, NOPERIOD);
        }
        genbank_out_one_entry(fp, "  JOURNAL   ", gbk_ref.journal, wrapWords, NOPERIOD);
    }
    else {
        // print title also, if there is no information (like other fields here)
        genbank_out_one_entry(fp, "  JOURNAL   ", gbk_ref.journal, wrapWords, NOPERIOD);
        genbank_out_one_entry(fp, "  TITLE     ", gbk_ref.title, wrapWords, NOPERIOD);
    }

    genbank_out_one_entry(fp, "  STANDARD  ", gbk_ref.standard, wrapWords, NOPERIOD);
}

void genbank_out(FILE * fp) {
    // Output in a genbank format

    int indi, length;
    int base_a, base_t, base_g, base_c, base_other;

    WrapMode wrapWords(true);

    genbank_out_one_entry(fp, "LOCUS       ", data.gbk.locus,      wrapWords,     NOPERIOD);
    genbank_out_one_entry(fp, "DEFINITION  ", data.gbk.definition, wrapWords,     PERIOD);
    genbank_out_one_entry(fp, "ACCESSION   ", data.gbk.accession,  wrapWords,     NOPERIOD);
    genbank_out_one_entry(fp, "KEYWORDS    ", data.gbk.keywords,   WrapMode(";"), PERIOD);
    genbank_out_one_entry(fp, "SOURCE      ", data.gbk.source,     wrapWords,     PERIOD);
    genbank_out_one_entry(fp, "  ORGANISM  ", data.gbk.organism,   wrapWords,     PERIOD);

    if (data.gbk.numofref > 0) {
        for (indi = 0; indi < data.gbk.numofref; indi++) {
            genbank_out_one_reference(fp, data.gbk.reference[indi], indi+1, true);
        }
    }
    else {
        GenbankRef ref;
        init_reference(ref);
        genbank_out_one_reference(fp, ref, 1, false);
        cleanup_reference(ref);
    }

    OrgInfo& orginf = data.gbk.comments.orginf;
    SeqInfo& seqinf = data.gbk.comments.seqinf;

    if (orginf.exists ||
        seqinf.exists ||
        str0len(data.gbk.comments.others) > 0)
    {
        fputs("COMMENTS    ", fp);

        if (orginf.exists) {
            fputs("Organism information\n", fp);

            genbank_print_comment_if_content(fp, "Source of strain: ",   orginf.source);
            genbank_print_comment_if_content(fp, "Culture collection: ", orginf.cultcoll); // this field is used in ../lib/import/.rdp_old.ift
            genbank_print_comment_if_content(fp, "Former name: ",        orginf.formname); // other fields occur in no .ift
            genbank_print_comment_if_content(fp, "Alternate name: ",     orginf.nickname);
            genbank_print_comment_if_content(fp, "Common name: ",        orginf.commname);
            genbank_print_comment_if_content(fp, "Host organism: ",      orginf.hostorg);

            if (seqinf.exists || str0len(data.gbk.comments.others) > 0)
                fputs("            ", fp);
        }                       /* organism information */

        if (seqinf.exists) {
            fprintf(fp, "Sequence information (bases 1 to %d)\n", data.seq_length);
        }

        genbank_print_comment_if_content(fp, "RDP ID: ",                      seqinf.RDPid);
        genbank_print_comment_if_content(fp, "Corresponding GenBank entry: ", seqinf.gbkentry); // this field is used in ../lib/import/.rdp_old.ift
        genbank_print_comment_if_content(fp, "Sequencing methods: ",          seqinf.methods);

        // @@@ DRY (when bug was removed):
        if      (seqinf.comp5 == 'n') fputs("              5' end complete: No\n", fp);
        else if (seqinf.comp5 == 'y') fputs("              5' end complete: Yes\n", fp);

        if      (seqinf.comp3 == 'n') fputs("              3' end complete: No\n", fp);
        else if (seqinf.comp3 == 'y') fputs("             3' end complete: Yes\n", fp); // @@@ now you see the 'bug'

        // @@@ use wrapper for code below ? 
        /* print GBINDENT spaces of the first line */
        if (str0len(data.gbk.comments.others) > 0) {
            fputc_rep(' ', GBINDENT, fp);
        }

        if (str0len(data.gbk.comments.others) > 0) {
            length = str0len(data.gbk.comments.others);
            for (indi = 0; indi < length; indi++) {
                fputc(data.gbk.comments.others[indi], fp);

                /* if another line, print GBINDENT spaces first */
                if (data.gbk.comments.others[indi] == '\n' && data.gbk.comments.others[indi + 1] != '\0') {
                    fputc_rep(' ', GBINDENT, fp);
                }
            }
        }                       /* other comments */
    }                           /* comment */

    count_bases(&base_a, &base_t, &base_g, &base_c, &base_other);

    /* don't write 0 others in this base line */
    if (base_other > 0)
        fprintf(fp, "BASE COUNT  %6d a %6d c %6d g %6d t %6d others\n", base_a, base_c, base_g, base_t, base_other);
    else
        fprintf(fp, "BASE COUNT  %6d a %6d c %6d g %6d t\n", base_a, base_c, base_g, base_t);

    genbank_out_origin(fp);
}

void genbank_out_one_entry(FILE * fp, const char *key, const char *content, const WrapMode& wrapMode, int period) {
    /* Print out key and content if content length > 1
     * otherwise print key and "No information" w/wo
     * period at the end depending on flag period.
     */

    if (!has_content(content)) {
        content = period ? "No information.\n" : "No information\n";
    }
    genbank_print_lines(fp, key, content, wrapMode);
}

void genbank_print_lines(FILE * fp, const char *key, const char *content, const WrapMode& wrapMode) { // @@@ WRAPPER
    // Print one genbank line, wrap around if over column GBMAXLINE

    ca_assert(strlen(key) == GBINDENT);
    ca_assert(content[strlen(content)-1] == '\n');

    print_wrapped(fp, key, "            ", content, wrapMode, GBMAXLINE, WRAP_CORRECTLY);
}

void genbank_print_comment_if_content(FILE * fp, const char *key, const char *content) { // @@@ WRAPPER
    // Print one genbank line, wrap around if over column GBMAXLINE
    
    if (!has_content(content)) return;

    char first[LINESIZE]; sprintf(first, "%*s%s", GBINDENT+RDP_SUBKEY_INDENT, "", key);
    char other[LINESIZE]; sprintf(other, "%*s", GBINDENT+RDP_SUBKEY_INDENT+RDP_CONTINUED_INDENT, "");
    print_wrapped(fp, first, other, content, WrapMode(true), GBMAXLINE, WRAP_CORRECTLY);
}

/* ---------------------------------------------------------------
 *   Function genbank_out_origin().
 *       Output sequence data in genbank format.
 */
void genbank_out_origin(FILE * fp) {
    int indi, indj, indk;

    fputs("ORIGIN\n", fp);

    for (indi = 0, indj = 0, indk = 1; indi < data.seq_length; indi++) {
        if ((indk % 60) == 1)
            fprintf(fp, "   %6d ", indk);
        fputc(data.sequence[indi], fp);
        indj++;

        /* blank space follows every 10 bases, but not before '\n' */
        if ((indk % 60) == 0) {
            fputc('\n', fp);
            indj = 0;
        }
        else if (indj == 10 && indi != (data.seq_length - 1)) {
            fputc(' ', fp);
            indj = 0;
        }
        indk++;
    }

    if ((indk % 60) != 1)
        fputc('\n', fp);
    fputs("//\n", fp);
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

