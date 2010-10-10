/* -------------------- Macke related subroutines -------------- */

#include <stdio.h>
#include "global.h"
#include "macke.h"

#define MACKELIMIT 10000

/* -------------------------------------------------------------
 *   Function init_mache().
 *   Initialize macke entry.
 */
void cleanup_macke() {
    freenull(data.macke.seqabbr);
    freenull(data.macke.name);
    freenull(data.macke.atcc);
    freenull(data.macke.rna);
    freenull(data.macke.date);
    freenull(data.macke.nbk);
    freenull(data.macke.acs);
    freenull(data.macke.who);
    for (int indi = 0; indi < data.macke.numofrem; indi++) {
        freenull(data.macke.remarks[indi]);
    }
    freenull(data.macke.remarks);
    freenull(data.macke.journal);
    freenull(data.macke.title);
    freenull(data.macke.author);
    freenull(data.macke.strain);
    freenull(data.macke.subspecies);
}

void reinit_macke() {
    /* initialize macke format */
    cleanup_macke();

    data.macke.seqabbr = strdup("");
    data.macke.name = no_content();
    data.macke.atcc = no_content();
    data.macke.rna = no_content();
    data.macke.date = no_content();
    data.macke.nbk = no_content();
    data.macke.acs = no_content();
    data.macke.who = no_content();
    data.macke.numofrem = 0;
    data.macke.rna_or_dna = 'd';
    data.macke.journal = no_content();
    data.macke.title = no_content();
    data.macke.author = no_content();
    data.macke.strain = no_content();
    data.macke.subspecies = no_content();
}

/* -------------------------------------------------------------
 *  Function macke_in().
 *      Read in one sequence data from Macke file.
 */

void MackeReader::start_reading() {
    IFP1 = open_input_or_die(inName);
    IFP2 = open_input_or_die(inName);
    IFP3 = open_input_or_die(inName);
    fp1 = create_FILE_BUFFER(inName, IFP1);
    fp2 = create_FILE_BUFFER(inName, IFP2);
    fp3 = create_FILE_BUFFER(inName, IFP3);
}

void MackeReader::stop_reading() {
    destroy_FILE_BUFFER(fp3);
    destroy_FILE_BUFFER(fp2);
    destroy_FILE_BUFFER(fp1);
}

char MackeReader::mackeIn() {
    char  oldname[TOKENSIZE], name[TOKENSIZE];
    char  key[TOKENSIZE];
    int   numofrem = 0;
    int   index;

    /* file 1 points to seq. information */
    /* file 2 points to seq. data */
    /* file 3 points to seq. names */
    if (firstRead) {
        start_reading();

        eof1 = skipOverLinesThat(line1, LINESIZE, fp1, Not(isMackeSeqInfo));   // skip to #:; where the seq. information is
        eof2 = skipOverLinesThat(line2, LINESIZE, fp2, isMackeNonSeq);         // skip to where seq. data starts
        eof3 = skipOverLinesThat(line3, LINESIZE, fp3, Not(isMackeSeqHeader)); // skip to #=; where seq. first appears

        firstRead = false;
    }
    else
        eof3 = Fgetline(line3, LINESIZE, fp3);

    /* end of reading seq names */
    if (line3[0] != '#' || line3[1] != '=')
        return (EOF);

    /* skip to next "#:" line or end of file */
    if (!isMackeSeqInfo(line1)) {
        eof1 = skipOverLinesThat(line1, LINESIZE, fp1, Not(isMackeSeqInfo));
    }

    /* read in seq name */
    index = macke_abbrev(line3, oldname, 2);
    freedup(data.macke.seqabbr, oldname);

    /* read seq. information */
    for (index = macke_abbrev(line1, name, 2); eof1 != NULL && isMackeSeqInfo(line1) && str_equal(name, oldname); ) {
        index = macke_abbrev(line1, key, index);
        if (str_equal(key, "name")) {
            eof1 = macke_one_entry_in(fp1, "name", oldname, data.macke.name, line1, index);
        }
        else if (str_equal(key, "atcc")) {
            eof1 = macke_one_entry_in(fp1, "atcc", oldname, data.macke.atcc, line1, index);
        }
        else if (str_equal(key, "rna")) {
            /* old version entry */
            eof1 = macke_one_entry_in(fp1, "rna", oldname, data.macke.rna, line1, index);
        }
        else if (str_equal(key, "date")) {
            eof1 = macke_one_entry_in(fp1, "date", oldname, data.macke.date, line1, index);
        }
        else if (str_equal(key, "nbk")) {
            /* old version entry */
            eof1 = macke_one_entry_in(fp1, "nbk", oldname, data.macke.nbk, line1, index);
        }
        else if (str_equal(key, "acs")) {
            eof1 = macke_one_entry_in(fp1, "acs", oldname, data.macke.acs, line1, index);
        }
        else if (str_equal(key, "subsp")) {
            eof1 = macke_one_entry_in(fp1, "subsp", oldname, data.macke.subspecies, line1, index);
        }
        else if (str_equal(key, "strain")) {
            eof1 = macke_one_entry_in(fp1, "strain", oldname, data.macke.strain, line1, index);
        }
        else if (str_equal(key, "auth")) {
            eof1 = macke_one_entry_in(fp1, "auth", oldname, data.macke.author, line1, index);
        }
        else if (str_equal(key, "title")) {
            eof1 = macke_one_entry_in(fp1, "title", oldname, data.macke.title, line1, index);
        }
        else if (str_equal(key, "jour")) {
            eof1 = macke_one_entry_in(fp1, "jour", oldname, data.macke.journal, line1, index);
        }
        else if (str_equal(key, "who")) {
            eof1 = macke_one_entry_in(fp1, "who", oldname, data.macke.who, line1, index);
        }
        else if (str_equal(key, "rem")) {
            data.macke.remarks = (char **)Reallocspace((char *)data.macke.remarks, (unsigned)(sizeof(char *) * (numofrem + 1)));
            data.macke.remarks[numofrem++] = nulldup(line1 + index);
            eof1 = Fgetline(line1, LINESIZE, fp1);
        }
        else {
            warningf(144, "Unidentified AE2 key word #%s#", key);
            eof1 = Fgetline(line1, LINESIZE, fp1);
        }
        if (eof1 != NULL && isMackeSeqInfo(line1))
            index = macke_abbrev(line1, name, 2);
        else
            index = 0;
    }
    data.macke.numofrem = numofrem;

    /* read in sequence data */
    eof2 = macke_origin(oldname, line2, fp2);

    return (EOF + 1);
}

/* -----------------------------------------------------------------
 *  Function macke_one_entry_in().
 *      Get one Macke entry.
 */
char *macke_one_entry_in(FILE_BUFFER fp, const char *key, char *oldname, char*& var, char *line, int index) {
    char *eof;

    if (has_content(var))
        skip_eolnl_and_append_spaced(var, line + index);
    else
        freedup(var, line + index);

    eof = macke_continue_line(key, oldname, var, line, fp);

    return (eof);
}

/* --------------------------------------------------------------
 *  Function macke_continue_line().
 *      Append macke continue line.
 */
char *macke_continue_line(const char *key, char *oldname, char*& var, char *line, FILE_BUFFER fp) {
    char *eof, name[TOKENSIZE], newkey[TOKENSIZE];
    int   index;

    for (eof = Fgetline(line, LINESIZE, fp); eof != NULL; eof = Fgetline(line, LINESIZE, fp)) {
        if (str0len(line) <= 1)
            continue;

        index = macke_abbrev(line, name, 0);
        if (!str_equal(name, oldname))
            break;

        index = macke_abbrev(line, newkey, index);
        if (!str_equal(newkey, key))
            break;

        skip_eolnl_and_append_spaced(var, line + index);
    }

    return (eof);
}

/* ----------------------------------------------------------------
 *  Function macke_origin().
 *      Read in sequence data in macke file.
 */
char *macke_origin(char *key, char *line, FILE_BUFFER fp) {
    int index, indj, seqnum;

    char *eof, name[TOKENSIZE], seq[LINESIZE];

    /* read in seq. data */
    data.seq_length = 0;
    /* read in line by line */

    index = macke_abbrev(line, name, 0);
    eof = line;
    for (; eof != NULL && str_equal(key, name);) {
        ASSERT_RESULT(int, 2, sscanf(line + index, "%d%s", &seqnum, seq));
        for (indj = data.seq_length; indj < seqnum; indj++)
            if (indj < data.max)
                data.sequence[data.seq_length++]
                    = '.';
            else {
                data.max += 100;
                data.sequence = Reallocspace(data.sequence, (unsigned) (sizeof(char) * data.max));
                data.sequence[data.seq_length++] = '.';
            }
        for (indj = 0; seq[indj] != '\n' && seq[indj] != '\0'; indj++)
            if (data.seq_length < data.max)
                data.sequence[data.seq_length++] = seq[indj];
            else {
                data.max += 100;
                data.sequence = Reallocspace(data.sequence, (unsigned) (sizeof(char) * data.max));
                data.sequence[data.seq_length++] = seq[indj];
            }
        data.sequence[data.seq_length] = '\0';
        eof = Fgetline(line, LINESIZE, fp);
        if (eof != NULL)
            index = macke_abbrev(line, name, 0);
    }                           /* reading seq loop */

    return (eof);
}

/* --------------------------------------------------------------
 *   Function macke_abbrev().
 *       Find the key in Macke line.
 */
int macke_abbrev(char *line, char *key, int index) {
    int indi;

    /* skip white space */
    index = Skip_white_space(line, index);

    for (indi = index; line[indi] != ' ' && line[indi] != ':' && line[indi] != '\t' && line[indi] != '\n' && line[indi] != '\0'; indi++)
        key[indi - index] = line[indi];

    key[indi - index] = '\0';
    return (indi + 1);
}

/* --------------------------------------------------------------
 *   Function macke_is_continued_remark().
 *       If there is 3 blanks at the beginning of the line,
 *           it is continued line.
 *
 * The comment above is lying:
 *      The function always only tested for 2 spaces
 *      and the converter only produced 2 spaces.
 */
bool macke_is_continued_remark(const char *str) {
    return strncmp(str, ":  ", 3) == 0;
}

/* -------------------------------------------------------------
 *   Function macke_in_name().
 *       Read in next sequence name and data only.
 */
char macke_in_name(FILE_BUFFER fp) {
    static char line[LINESIZE];
#warning another function remembering it has been called (seems to be the last)
    static int  first_time = 1;
    char        name[TOKENSIZE];
    char        seq[LINESIZE];
    char       *eof;
    int         numofrem   = 0, seqnum;
    int         index, indj;

    /* skip other information, file index points to seq. data */
    if (first_time) {
        eof = skipOverLinesThat(line, LINESIZE, fp, isMackeHeader);        // skip all "#" lines to where seq. data is

        first_time = 0;
    }
    else if (line[0] == EOF) {
        line[0] = EOF + 1;
        first_time = 1;
        return (EOF);
    }

    /* read in seq. data */
    data.macke.numofrem = numofrem;
    data.seq_length = 0;

    /* read in line by line */
    freenull(data.macke.seqabbr);
    for (index = macke_abbrev(line, name, 0), data.macke.seqabbr = nulldup(name); line[0] != EOF && str_equal(data.macke.seqabbr, name);) {
        ASSERT_RESULT(int, 2, sscanf(line + index, "%d%s", &seqnum, seq));
        for (indj = data.seq_length; indj < seqnum; indj++)
            if (data.seq_length < data.max)
                data.sequence[data.seq_length++] = '.';
            else {
                data.max += 100;
                data.sequence = Reallocspace(data.sequence, (unsigned) (sizeof(char) * data.max));
                data.sequence[data.seq_length++] = '.';
            }
        for (indj = 0; seq[indj] != '\n' && seq[indj] != '\0'; indj++) {
            if (data.seq_length < data.max)
                data.sequence[data.seq_length++] = seq[indj];
            else {
                data.max += 100;
                data.sequence = Reallocspace(data.sequence, (unsigned) (sizeof(char) * data.max));
                data.sequence[data.seq_length++] = seq[indj];
            }
        }
        data.sequence[data.seq_length] = '\0';

        if ((eof = Fgetline(line, LINESIZE, fp)) != NULL)
            index = macke_abbrev(line, name, 0);
        else
            line[0] = EOF;
    }                           /* reading seq loop */

    return (EOF + 1);
}

/* ------------------------------------------------------------
 *   Function macke_out_header().
 *       Output the Macke format header.
 */
void macke_out_header(FILE * fp) {
    fputs("#-\n#-\n#-\teditor\n", fp);
    const char *date = today_date();
    fprintf(fp, "#-\t%s\n#-\n#-\n", date);
}

void macke_seq_display_out(FILE * fp, int format) {
    // Output the Macke format each sequence format (wot?)
    
    char token[TOKENSIZE], direction[TOKENSIZE];

    if (format == SWISSPROT) {
        strcpy(token, "pro");
        strcpy(direction, "n>c");
    }
    else {
        strcpy(direction, "5>3");
        if (data.macke.rna_or_dna == 'r')
            strcpy(token, "rna");
        else
            strcpy(token, "dna");
    }
    if (data.numofseq == 1) {
        fprintf(fp, "#-\tReference sequence:  %s\n", data.macke.seqabbr);
        fputs("#-\tAttributes:\n", fp);

        if (str0len(data.macke.seqabbr) < 8)
            fprintf(fp, "#=\t\t%s\t \tin  out  vis  prt   ord  %s  lin  %s  func ref\n", data.macke.seqabbr, token, direction);
        else
            fprintf(fp, "#=\t\t%s\tin  out  vis  prt   ord  %s  lin  %s  func ref\n", data.macke.seqabbr, token, direction);
    }
    else if (str0len(data.macke.seqabbr) < 8)
        fprintf(fp, "#=\t\t%s\t\tin  out  vis  prt   ord  %s  lin  %s  func\n", data.macke.seqabbr, token, direction);
    else
        fprintf(fp, "#=\t\t%s\tin  out  vis  prt   ord  %s  lin  %s  func\n", data.macke.seqabbr, token, direction);
}

static void macke_print_prefixed_line(FILE *fp, const char *tag, const char *content) {
    ca_assert(has_content(content));

    char prefix[LINESIZE];
    sprintf(prefix, "#:%s:%s:", data.macke.seqabbr, tag);

    macke_print_line(fp, prefix, content);
}

static bool macke_print_prefixed_line_if_content(FILE *fp, const char *tag, const char *content) {
    if (!has_content(content)) return false;
    macke_print_prefixed_line(fp, tag, content);
    return true;
}

void macke_seq_info_out(FILE * fp) {
    // Output sequence information
    Macke& macke = data.macke;

    macke_print_prefixed_line_if_content(fp, "name",   macke.name);
    macke_print_prefixed_line_if_content(fp, "strain", macke.strain);
    macke_print_prefixed_line_if_content(fp, "subsp",  macke.subspecies);
    macke_print_prefixed_line_if_content(fp, "atcc",   macke.atcc);
    macke_print_prefixed_line_if_content(fp, "rna",    macke.rna);        // old version entry
    macke_print_prefixed_line_if_content(fp, "date",   macke.date);
    macke_print_prefixed_line_if_content(fp, "acs",    macke.acs)
        || macke_print_prefixed_line_if_content(fp, "acs", macke.nbk);    // old version entry
    macke_print_prefixed_line_if_content(fp, "auth",   macke.author);
    macke_print_prefixed_line_if_content(fp, "jour",   macke.journal);
    macke_print_prefixed_line_if_content(fp, "title",  macke.title);
    macke_print_prefixed_line_if_content(fp, "who",    macke.who);

    /* print out remarks, wrap around if more than MACKEMAXLINE columns */
    for (int indi = 0; indi < macke.numofrem; indi++) {
        /* Check if it is general comment or GenBank entry */
        /* if general comment, macke_in_one_line return(1). */
        if (macke_in_one_line(macke.remarks[indi])) {
            macke_print_prefixed_line(fp, "rem", macke.remarks[indi]);
        }
        else {
            /* if GenBank entry comments */
            macke_print_keyword_rem(indi, fp);
        }
    } 
}

/* ----------------------------------------------------------------
 *   Function macke_print_keyword_rem().
 *       Print out keyworded remark line in Macke file with
 *           wrap around functionality.
 *       (Those keywords are defined in GenBank COMMENTS by
 *           RDP group)
 */
void macke_print_keyword_rem(int index, FILE * fp) { // @@@ WRAPPER
    char first[LINESIZE]; sprintf(first, "#:%s:rem:", data.macke.seqabbr);
    char other[LINESIZE]; sprintf(other, "%s:%*s", first, RDP_SUBKEY_INDENT, "");
    const char *remark = data.macke.remarks[index];
    
    // print_wrapped(fp, first, other, remark, WrapMode(true), MACKEMAXLINE, false); // @@@ wanted correct behavior
    print_wrapped(fp, first, other, remark, WrapMode(true), MACKEMAXLINE-1, WRAPBUG_WRAP_AT_SPACE); // works with testdata
}

void macke_print_line(FILE * fp, const char *prefix, const char *content) { // @@@ WRAPPER
    // print a macke line and wrap around line after MACKEMAXLINE column.
    // print_wrapped(fp, prefix, prefix, content, WrapMode(true), MACKEMAXLINE, false); // @@@ wanted
    print_wrapped(fp, prefix, prefix, content, WrapMode(true), MACKEMAXLINE, WRAPBUG_WRAP_BEFORE_SEP); 
}

/* -----------------------------------------------------------
 *   Function macke_key_word().
 *       Find the key in Macke line.
 */
int macke_key_word(char *line, int index, char *key, int length) {
    int indi;

    if (line == NULL) {
        key[0] = '\0';
        return (index);
    }

    /* skip white space */
    index = Skip_white_space(line, index);

    for (indi = index; (indi - index) < (length - 1) && line[indi] != ':' && line[indi] != '\n' && line[indi] != '\0'; indi++)
        key[indi - index] = line[indi];

    key[indi - index] = '\0';

    return (indi + 1);
}

/* ------------------------------------------------------------
 *  Function macke_in_one_line().
 *      Check if Str should be in one line.
 */
int macke_in_one_line(char *Str) {
    char keyword[TOKENSIZE];

    int iskey;

    macke_key_word(Str, 0, keyword, TOKENSIZE);
    iskey = 0;
    if (str_equal(keyword, "KEYWORDS"))
        iskey = 1;
    else if (str_equal(keyword, "GenBank ACCESSION"))
        iskey = 1;
    else if (str_equal(keyword, "auth"))
        iskey = 1;
    else if (str_equal(keyword, "title"))
        iskey = 1;
    else if (str_equal(keyword, "jour"))
        iskey = 1;
    else if (str_equal(keyword, "standard"))
        iskey = 1;
    else if (str_equal(keyword, "Source of strain"))
        iskey = 1;
    else if (str_equal(keyword, "Former name"))
        iskey = 1;
    else if (str_equal(keyword, "Alternate name"))
        iskey = 1;
    else if (str_equal(keyword, "Common name"))
        iskey = 1;
    else if (str_equal(keyword, "Host organism"))
        iskey = 1;
    else if (str_equal(keyword, "RDP ID"))
        iskey = 1;
    else if (str_equal(keyword, "Sequencing methods"))
        iskey = 1;
    else if (str_equal(keyword, "3' end complete"))
        iskey = 1;
    else if (str_equal(keyword, "5' end complete"))
        iskey = 1;

    /* is-key then could be more than one line */
    /* otherwise, must be in one line */
    if (iskey)
        return (0);
    else
        return (1);
}

void macke_seq_data_out(FILE * fp) {
    // Output Macke format sequence data
    int indj, indk;

    if (data.seq_length > MACKELIMIT) {
        warningf(145, "Length of sequence data is %d over AE2's limit %d.", data.seq_length, MACKELIMIT);
    }

    for (indk = indj = 0; indk < data.seq_length; indk++) {
        if (indj == 0)
            fprintf(fp, "%s%6d ", data.macke.seqabbr, indk);

        fputc(data.sequence[indk], fp);

        indj++;
        if (indj == 50) {
            indj = 0;
            fputc('\n', fp);
        }
    }                           /* every line */

    if (indj != 0)
        fputc('\n', fp);
    /* every sequence */
}
