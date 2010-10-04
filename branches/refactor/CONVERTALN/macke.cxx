/* -------------------- Macke related subroutines -------------- */

#include <stdio.h>
#include "convert.h"
#include "global.h"
#include "macke.h"

#define MACKELIMIT 10000

/* -------------------------------------------------------------
 *   Function init_mache().
 *   Initialize macke entry.
 */
void cleanup_macke() {
    Freespace(&(data.macke.seqabbr));
    Freespace(&(data.macke.name));
    Freespace(&(data.macke.atcc));
    Freespace(&(data.macke.rna));
    Freespace(&(data.macke.date));
    Freespace(&(data.macke.nbk));
    Freespace(&(data.macke.acs));
    Freespace(&(data.macke.who));
    for (int indi = 0; indi < data.macke.numofrem; indi++) {
        Freespace(&(data.macke.remarks[indi]));
    }
    Freespace((char **)&(data.macke.remarks));
    Freespace(&(data.macke.journal));
    Freespace(&(data.macke.title));
    Freespace(&(data.macke.author));
    Freespace(&(data.macke.strain));
    Freespace(&(data.macke.subspecies));
}

void reinit_macke() {
    /* initialize macke format */
    cleanup_macke();

    data.macke.seqabbr = strdup("");
    data.macke.name = strdup("\n");
    data.macke.atcc = strdup("\n");
    data.macke.rna = strdup("\n");
    data.macke.date = strdup("\n");
    data.macke.nbk = strdup("\n");
    data.macke.acs = strdup("\n");
    data.macke.who = strdup("\n");
    data.macke.numofrem = 0;
    data.macke.rna_or_dna = 'd';
    data.macke.journal = strdup("\n");
    data.macke.title = strdup("\n");
    data.macke.author = strdup("\n");
    data.macke.strain = strdup("\n");
    data.macke.subspecies = strdup("\n");
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
    Freespace(&data.macke.seqabbr);
    data.macke.seqabbr = str0dup(oldname);

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
            data.macke.remarks[numofrem++] = str0dup(line1 + index);
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

    if (str0len(var) > 1)
        skip_eolnl_and_append_spaced(var, line + index);
    else
        replace_entry(var, line + index);

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
 */
bool macke_is_continued_remark(const char *str) {
    return strcmp(str, ":  ") == 0;
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
    Freespace(&(data.macke.seqabbr));
    for (index = macke_abbrev(line, name, 0), data.macke.seqabbr = str0dup(name); line[0] != EOF && str_equal(data.macke.seqabbr, name);) {
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
    fprintf(fp, "#-\n#-\n#-\teditor\n");
    const char *date = today_date();
    fprintf(fp, "#-\t%s\n#-\n#-\n", date);
#warning next line crashes, but compiles w/o warning
    // Freespace(&date);
}

/* ------------------------------------------------------------
 *   Function macke_out0().
 *       Output the Macke format each sequence format.
 */
void macke_out0(FILE * fp, int format) {
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
        fprintf(fp, "#-\tAttributes:\n");

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

/* ---------------------------------------------------------------
 *   Function macke_out1().
 *       Output sequences information.
 */
void macke_out1(FILE * fp) {
    char temp[LINESIZE];
    int  indi;

    if (str0len(data.macke.name) > 1) {
        sprintf(temp, "#:%s:name:", data.macke.seqabbr);
        macke_print_line_78(fp, temp, data.macke.name);
    }
    if (str0len(data.macke.strain) > 1) {
        sprintf(temp, "#:%s:strain:", data.macke.seqabbr);
        macke_print_line_78(fp, temp, data.macke.strain);
    }
    if (str0len(data.macke.subspecies) > 1) {
        sprintf(temp, "#:%s:subsp:", data.macke.seqabbr);
        macke_print_line_78(fp, temp, data.macke.subspecies);
    }
    if (str0len(data.macke.atcc) > 1) {
        sprintf(temp, "#:%s:atcc:", data.macke.seqabbr);
        macke_print_line_78(fp, temp, data.macke.atcc);
    }
    if (str0len(data.macke.rna) > 1) {
        /* old version entry */
        sprintf(temp, "#:%s:rna:", data.macke.seqabbr);
        macke_print_line_78(fp, temp, data.macke.rna);
    }
    if (str0len(data.macke.date) > 1) {
        sprintf(temp, "#:%s:date:", data.macke.seqabbr);
        macke_print_line_78(fp, temp, data.macke.date);
    }
    if (str0len(data.macke.acs) > 1) {
        sprintf(temp, "#:%s:acs:", data.macke.seqabbr);
        macke_print_line_78(fp, temp, data.macke.acs);
    }
    else if (str0len(data.macke.nbk) > 1) {
        /* old version entry */
        sprintf(temp, "#:%s:acs:", data.macke.seqabbr);
        macke_print_line_78(fp, temp, data.macke.nbk);
    }
    if (str0len(data.macke.author) > 1) {
        sprintf(temp, "#:%s:auth:", data.macke.seqabbr);
        macke_print_line_78(fp, temp, data.macke.author);
    }
    if (str0len(data.macke.journal) > 1) {
        sprintf(temp, "#:%s:jour:", data.macke.seqabbr);
        macke_print_line_78(fp, temp, data.macke.journal);
    }
    if (str0len(data.macke.title) > 1) {
        sprintf(temp, "#:%s:title:", data.macke.seqabbr);
        macke_print_line_78(fp, temp, data.macke.title);
    }
    if (str0len(data.macke.who) > 1) {
        sprintf(temp, "#:%s:who:", data.macke.seqabbr);
        macke_print_line_78(fp, temp, data.macke.who);
    }

    /* print out remarks, wrap around if more than 78 columns */
    for (indi = 0; indi < data.macke.numofrem; indi++) {
        /* Check if it is general comment or GenBank entry */
        /* if general comment, macke_in_one_line return(1). */
        if (macke_in_one_line(data.macke.remarks[indi])) {
            sprintf(temp, "#:%s:rem:", data.macke.seqabbr);
            macke_print_line_78(fp, temp, data.macke.remarks[indi]);
            continue;
        }

        /* if GenBank entry comments */
        macke_print_keyword_rem(indi, fp);
    }                           /* for each remark */
}

/* ----------------------------------------------------------------
 *   Function macke_print_keyword_rem().
 *       Print out keyworded remark line in Macke file with
 *           wrap around functionality.
 *       (Those keywords are defined in GenBank COMMENTS by
 *           RDP group)
 */
void macke_print_keyword_rem(int index, FILE * fp) {
    int indj, indk, indl, lineno, len, maxc;

    lineno = 0;
    len = str0len(data.macke.remarks[index]) - 1;
    for (indj = 0; indj < len; indj += (indk + 1)) {
        indk = maxc = MACKEMAXCHAR - 7 - str0len(data.macke.seqabbr);
        if (lineno != 0)
            indk = maxc = maxc - 3;
        if ((str0len(data.macke.remarks[index] + indj) - 1)
            > maxc) {
            /* Search the last word */
            for (indk = maxc - 1; indk >= 0 && is_word_char(data.macke.remarks[index][indk + indj]); indk--) ;

            if (lineno == 0) {
                fprintf(fp, "#:%s:rem:", data.macke.seqabbr);
                lineno++;
            }
            else
                fprintf(fp, "#:%s:rem::  ", data.macke.seqabbr);

            for (indl = 0; indl < indk; indl++)
                fprintf(fp, "%c", data.macke.remarks[index][indj + indl]);

            if (data.macke.remarks[index][indj + indk] != ' ')
                fprintf(fp, "%c", data.macke.remarks[index][indj + indk]);
            fprintf(fp, "\n");
        }
        else if (lineno == 0)
            fprintf(fp, "#:%s:rem:%s", data.macke.seqabbr, data.macke.remarks[index] + indj);
        else
            fprintf(fp, "#:%s:rem::  %s", data.macke.seqabbr, data.macke.remarks[index] + indj);
    }                           /* for every MACKEMAXCHAR columns */
}

/* ---------------------------------------------------------------
 *   Function macke_print_line_78().
 *       print a macke line and wrap around line after
 *           78(MACKEMAXCHAR) column.
 */
void macke_print_line_78(FILE * fp, char *line1, char *line2) {
    int len, indi, indj, indk, ibuf;

    for (indi = 0, len = str0len(line2); indi < len; indi += indj) {
        indj = 78 - str0len(line1);

        if ((str0len(line2 + indi)) > indj) {
            ibuf = indj;

            for (; indj > 0 && is_word_char(line2[indi + indj]); indj--) ;

            if (indj == 0)
                indj = ibuf;
            else if (line2[indi + indj + 1] == ' ')
                indj++;

            fprintf(fp, "%s", line1);

            for (indk = 0; indk < indj; indk++)
                fprintf(fp, "%c", line2[indi + indk]);

            /* print out the last char if it is not blank */
            if (line2[indi + indj] == ' ')
                indj++;

            fprintf(fp, "\n");
        }
        else
            fprintf(fp, "%s%s", line1, line2 + indi);
    }                           /* for loop */
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

/* --------------------------------------------------------------
 *   Function macke_out2().
 *       Output Macke format sequences data.
 */
void macke_out2(FILE * fp) {
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
            fprintf(fp, "\n");
        }
    }                           /* every line */

    if (indj != 0)
        fprintf(fp, "\n");
    /* every sequence */
}
