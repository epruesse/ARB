#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "macke.h"

extern int warning_out;

/* ----------------------------------------------------------
 *      Function init_pm_data().
 *              Init macke and embl data.
 */
void reinit_em_data() {
    reinit_macke();
    reinit_embl();
}

/* ------------------------------------------------------------
 *      Function reinit_embl().
 *              Initialize embl entry.
 */
void cleanup_embl() {
    Embl& embl = data.embl;

    freenull(embl.id);
    freenull(embl.dateu);
    freenull(embl.datec);
    freenull(embl.description);
    freenull(embl.os);
    freenull(embl.accession);
    freenull(embl.keywords);

    for (int indi = 0; indi < embl.numofref; indi++) {
        freenull(embl.reference[indi].author);
        freenull(embl.reference[indi].title);
        freenull(embl.reference[indi].journal);
        freenull(embl.reference[indi].processing);
    }
    freenull(embl.reference);
    freenull(embl.dr);

    cleanup_comments(embl.comments);
}

void reinit_embl() {
    // initialize embl format 
    cleanup_embl();

    Embl& embl = data.embl;
    
    embl.id          = no_content();
    embl.dateu       = no_content();
    embl.datec       = no_content();
    embl.description = no_content();
    embl.os          = no_content();
    embl.accession   = no_content();
    embl.keywords    = no_content();
    embl.dr          = no_content();
    embl.numofref    = 0;
    embl.reference   = NULL;

    init_comments(embl.comments);
}

/* ---------------------------------------------------------------
 *      Function embl_in().
 *              Read in one embl entry.
 */
char embl_in(FILE_BUFFER fp) {
    char  line[LINESIZE], key[TOKENSIZE];
    char *eof, eoen;
    int   refnum;

    eoen = ' ';                 // end-of-entry, set to be 'y' after '//' is read 

    for (eof = Fgetline(line, LINESIZE, fp); eof != NULL && eoen != 'y';) {
        if (str0len(line) <= 1) {
            eof = Fgetline(line, LINESIZE, fp);
            continue;           // empty line, skip 
        }

        embl_key_word(line, 0, key, TOKENSIZE);
        eoen = 'n';

        if (str_equal(key, "ID")) {
            eof = embl_one_entry(line, fp, data.embl.id, key);
        }
        else if (str_equal(key, "DT")) {
            eof = embl_date(line, fp);
        }
        else if (str_equal(key, "DE")) {
            eof = embl_one_entry(line, fp, data.embl.description, key);
        }
        else if (str_equal(key, "OS")) {
            eof = embl_one_entry(line, fp, data.embl.os, key);
        }
        else if (str_equal(key, "AC")) {
            eof = embl_one_entry(line, fp, data.embl.accession, key);
        }
        else if (str_equal(key, "KW")) {
            eof = embl_one_entry(line, fp, data.embl.keywords, key);

            // correct missing '.' 
            if (str0len(data.embl.keywords) <= 1) freedup(data.embl.keywords, ".\n");
            else terminate_with(data.embl.keywords, '.');
        }
        else if (str_equal(key, "DR")) {
            eof = embl_one_entry(line, fp, data.embl.dr, key);
        }
        else if (str_equal(key, "RA")) {
            refnum = data.embl.numofref - 1;
            eof = embl_one_entry(line, fp, data.embl.reference[refnum].author, key);
            terminate_with(data.embl.reference[refnum].author, ';');
        }
        else if (str_equal(key, "RT")) {
            refnum = data.embl.numofref - 1;
            eof = embl_one_entry(line, fp, data.embl.reference[refnum].title, key);

            // Check missing '"' at the both ends 
            embl_verify_title(refnum);
        }
        else if (str_equal(key, "RL")) {
            refnum = data.embl.numofref - 1;
            eof = embl_one_entry(line, fp, data.embl.reference[refnum].journal, key);
            terminate_with(data.embl.reference[refnum].journal, '.');
        }
        else if (str_equal(key, "RP")) {
            refnum = data.embl.numofref - 1;
            eof = embl_one_entry(line, fp, data.embl.reference[refnum].processing, key);
        }
        else if (str_equal(key, "RN")) {
            eof = embl_version(line, fp);
        }
        else if (str_equal(key, "CC")) {
            eof = embl_comments(line, fp);
        }
        else if (str_equal(key, "SQ")) {
            eof = embl_origin(line, fp);
            eoen = 'y';
        }
        else {                  // unidentified key word 
            eof = embl_skip_unidentified(key, line, fp);
        }
        /* except "ORIGIN", at the end of all the other cases,
         * a new line has already read in, so no further read
         * is necessary */
    }

    if (eoen == 'n') throw_error(83, "Reached EOF before one entry has been read");

    return eof ? EOF+1 : EOF;
}

/* ---------------------------------------------------------------
 *      Function embl_in_id().
 *              Read in one embl entry.
 */
char embl_in_id(FILE_BUFFER fp) {
    char line[LINESIZE], key[TOKENSIZE];
    char *eof, eoen;

    eoen = ' ';
    // end-of-entry, set to be 'y' after '//' is read 

    for (eof = Fgetline(line, LINESIZE, fp); eof != NULL && eoen != 'y';) {
        if (str0len(line) <= 1) {
            eof = Fgetline(line, LINESIZE, fp);
            continue;           // empty line, skip 
        }

        embl_key_word(line, 0, key, TOKENSIZE);
        eoen = 'n';

        if (str_equal(key, "ID")) {
            eof = embl_one_entry(line, fp, data.embl.id, key);
        }
        else if (str_equal(key, "SQ")) {
            eof = embl_origin(line, fp);
            eoen = 'y';
        }
        else {                  // unidentified key word 
            eof = embl_skip_unidentified(key, line, fp);
        }
        /* except "ORIGIN", at the end of all the other cases,
           a new line has already read in, so no further read
           is necessary */
    } 

    if (eoen == 'n')
        throw_error(84, "Reach EOF before one entry is read");

    if (eof == NULL)
        return (EOF);
    else
        return (EOF + 1);
}

/* ----------------------------------------------------------------
 *      Function embl_key_word().
 *              Get the key_word from line beginning at index.
 */
void embl_key_word(const char *line, int index, char *key, int length) {
    // @@@ similar to genbank_key_word and macke_key_word
    // length = max size of key word 
    int indi, indj;

    if (line == NULL) {
        key[0] = '\0';
        return;
    }
    for (indi = index, indj = 0; (index - indi) < length && line[indi] != ' ' && line[indi] != '\t' && line[indi] != '\n' && line[indi] != '\0'; indi++, indj++)
        key[indj] = line[indi];
    key[indj] = '\0';
}

/* ------------------------------------------------------------
 *      Function embl_check_blanks().
 *              Check if there is (numb) blanks at beginning of line.
 */
int embl_check_blanks(char *line, int numb) {
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
 *      Function embl_continue_line().
 *              if there are (numb) blanks at the beginning of line,
 *                      it is a continue line of the current command.
 */
char *embl_continue_line(char *pattern, char*& Str, char *line, FILE_BUFFER fp) {
    int  ind;
    char key[TOKENSIZE], *eof, temp[LINESIZE];

    // check continue lines 
    for (eof = Fgetline(line, LINESIZE, fp); eof != NULL; eof = Fgetline(line, LINESIZE, fp)) {
        if (str0len(line) <= 1)
            continue;

        embl_key_word(line, 0, key, TOKENSIZE);

        if (!str_equal(pattern, key))
            break;

        // remove end-of-line, if there is any 
        ind = Skip_white_space(line, p_nonkey_start);
        strcpy(temp, (line + ind));
        skip_eolnl_and_append_spaced(Str, temp);
    }                           // end of continue line checking 

    return (eof);
}

/* --------------------------------------------------------------
 *      Function embl_one_entry().
 *              Read in one embl entry lines.
 */
char *embl_one_entry(char *line, FILE_BUFFER fp, char*& entry, char *key) {
    int   index;
    char *eof;

    index = Skip_white_space(line, p_nonkey_start);
    freedup(entry, line + index);
    eof = embl_continue_line(key, entry, line, fp);

    return (eof);
}

/* ---------------------------------------------------------------
 *      Function embl_verify_title().
 *              Verify title.
 */
void embl_verify_title(int refnum) {
    int   len;

    terminate_with(data.embl.reference[refnum].title, ';');

    len = str0len(data.embl.reference[refnum].title);
    if (len > 2 && (data.embl.reference[refnum].title[0] != '"' || data.embl.reference[refnum].title[len - 3] != '"')) {
        char *temp = NULL;
        if (data.embl.reference[refnum].title[0] != '"')
            temp = strdup("\"");
        else
            temp = strdup("");
        Append(temp, data.embl.reference[refnum].title);
        if ((len > 2 && data.embl.reference[refnum].title[len - 3]
             != '"')) {
            len = str0len(temp);
            temp[len - 2] = '"';
            terminate_with(temp, ';');
        }
        freedup(data.embl.reference[refnum].title, temp);
        free(temp);
    }
}

/* --------------------------------------------------------------
 *      Function embl_date().
 *              Read in embl DATE lines.
 */
char *embl_date(char *line, FILE_BUFFER fp) {
    int   index;
    char *eof, key[TOKENSIZE];

    index = Skip_white_space(line, p_nonkey_start);
    freedup(data.embl.dateu, line + index);

    eof = Fgetline(line, LINESIZE, fp);
    embl_key_word(line, 0, key, TOKENSIZE);
    if (str_equal(key, "DT")) {
        index = Skip_white_space(line, p_nonkey_start);
        freedup(data.embl.datec, line + index);
        // skip the rest of DT lines 
        do {
            eof = Fgetline(line, LINESIZE, fp);
            embl_key_word(line, 0, key, TOKENSIZE);
        } while (eof != NULL && str_equal(key, "DT"));
        return (eof);
    }
    else {
        // always expect more than two DT lines 
        warning(33, "one DT line is missing");
        return (eof);
    }
}

/* --------------------------------------------------------------
 *      Function embl_version().
 *              Read in embl RN lines.
 */
char *embl_version(char *line, FILE_BUFFER fp) {
    int index;

    char *eof;

    index = Skip_white_space(line, p_nonkey_start);
    if (data.embl.numofref == 0) {
        data.embl.numofref = 1;
        data.embl.reference = (Emblref *) calloc(1, sizeof(Emblref) * 1);
        data.embl.reference[0].author = strdup("");
        data.embl.reference[0].title = strdup("");
        data.embl.reference[0].journal = strdup("");
        data.embl.reference[0].processing = strdup("");
    }
    else {
        data.embl.numofref++;
        data.embl.reference = (Emblref *) Reallocspace(data.embl.reference, sizeof(Emblref) * (data.embl.numofref));
        data.embl.reference[data.embl.numofref - 1].author = strdup("");
        data.embl.reference[data.embl.numofref - 1].title = strdup("");
        data.embl.reference[data.embl.numofref - 1].journal = strdup("");
        data.embl.reference[data.embl.numofref - 1].processing = strdup("");
    }
    eof = Fgetline(line, LINESIZE, fp);
    return (eof);
}

/* --------------------------------------------------------------
 *      Function embl_comments().
 *              Read in embl comment lines.
 */
char *embl_comments(char *line, FILE_BUFFER fp) {
    int   index, offset;
    char *eof;
    char  key[TOKENSIZE];

    OrgInfo& orginf = data.embl.comments.orginf;
    SeqInfo& seqinf = data.embl.comments.seqinf;

    for (; line[0] == 'C' && line[1] == 'C';) {
        index = Skip_white_space(line, 5);
        offset = embl_comment_key(line + index, key);
        index = Skip_white_space(line, index + offset);
        if (str_equal(key, "Source of strain:")) {
            eof = embl_one_comment_entry(fp, orginf.source, line, index);
        }
        else if (str_equal(key, "Culture collection:")) {
            eof = embl_one_comment_entry(fp, orginf.cultcoll, line, index);
        }
        else if (str_equal(key, "Former name:")) {
            eof = embl_one_comment_entry(fp, orginf.formname, line, index);
        }
        else if (str_equal(key, "Alternate name:")) {
            eof = embl_one_comment_entry(fp, orginf.nickname, line, index);
        }
        else if (str_equal(key, "Common name:")) {
            eof = embl_one_comment_entry(fp, orginf.commname, line, index);
        }
        else if (str_equal(key, "Host organism:")) {
            eof = embl_one_comment_entry(fp, orginf.hostorg, line, index);
        }
        else if (str_equal(key, "RDP ID:")) {
            eof = embl_one_comment_entry(fp, seqinf.RDPid, line, index);
        }
        else if (str_equal(key, "Corresponding GenBank entry:")) {
            eof = embl_one_comment_entry(fp, seqinf.gbkentry, line, index);
        }
        else if (str_equal(key, "Sequencing methods:")) {
            eof = embl_one_comment_entry(fp, seqinf.methods, line, index);
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
            // do nothing 
            seqinf.exists = true;

            eof = Fgetline(line, LINESIZE, fp);
        }
        else if (str_equal(key, "Organism information")) {
            // do nothing 
            orginf.exists = true;

            eof = Fgetline(line, LINESIZE, fp);
        }
        else {                  // other comments 
            if (data.embl.comments.others == NULL) {
                data.embl.comments.others = nulldup(line + 5);
            }
            else
                Append(data.embl.comments.others, line + 5);
            eof = Fgetline(line, LINESIZE, fp);
        }
    }
    return (eof);
}

/* ----------------------------------------------------------------
 *      Function embl_skip_unidentified().
 *              if there are (numb) blanks at the beginning of line,
 *              it is a continue line of the current command.
 */
char *embl_skip_unidentified(char *pattern, char *line, FILE_BUFFER fp) {
    char *eof;
    char  key[TOKENSIZE];

    // check continue lines 
    for (eof = Fgetline(line, LINESIZE, fp); eof != NULL; eof = Fgetline(line, LINESIZE, fp)) {
        embl_key_word(line, 0, key, TOKENSIZE);
        if (!str_equal(key, pattern))
            break;
    }                           // end of continue line checking 
    return (eof);
}

/* -------------------------------------------------------------
 *      Function embl_comment_key().
 *              Get the subkey_word in comment lines beginning
 *                      at index.
 */
int embl_comment_key(char *line, char *key) {
    int indi, indj;

    if (line == NULL) {
        key[0] = '\0';
        return (0);
    }

    for (indi = indj = 0; line[indi] != ':' && line[indi] != '\t' && line[indi] != '\n' && line[indi] != '\0' && line[indi] != '('; indi++, indj++)
        key[indj] = line[indi];

    if (line[indi] == ':')
        key[indj++] = ':';

    key[indj] = '\0';

    return (indi + 1);
}

/* ------------------------------------------------------------
 *      Function embl_one_comment_entry().
 *              Read in one embl sub-entry in comments lines.
 *              If not RDP defined comment, should not call
 *                      this function.
 */
char *embl_one_comment_entry(FILE_BUFFER fp, char*& datastring, char *line, int start_index) {
    int   index;
    char *eof, temp[LINESIZE];

    index = Skip_white_space(line, start_index);
    freedup(datastring, line + index);

    // check continue lines 
    for (eof = Fgetline(line, LINESIZE, fp);
         eof != NULL && line[0] == 'C' && line[1] == 'C' && count_spaces(line + 2) >= RDP_CONTINUED_INDENT + RDP_SUBKEY_INDENT; eof = Fgetline(line, LINESIZE, fp)) {
        // remove end-of-line, if there is any 
        index = Skip_white_space(line, p_nonkey_start + RDP_SUBKEY_INDENT + RDP_CONTINUED_INDENT);
        strcpy(temp, (line + index));
        skip_eolnl_and_append_spaced(datastring, temp);
    }                           // end of continue line checking 

    return eof;
}

/* --------------------------------------------------------------
 *      Function embl_origin().
 *              Read in embl sequence data.
 */
char *embl_origin(char *line, FILE_BUFFER fp) {
    char *eof;
    int index;

    data.seq_length = 0;
    // read in whole sequence data 
    for (eof = Fgetline(line, LINESIZE, fp);
         eof != NULL && line[0] != '/' && line[1] != '/';
         eof = Fgetline(line, LINESIZE, fp))
    {
        for (index = 5; line[index] != '\n' && line[index] != '\0'; index++) {
            if (line[index] != ' ') {
                if (data.seq_length >= data.max) {
                    data.max += data.max/2+100;
                    data.sequence = Reallocspace(data.sequence, data.max);
                }
                data.sequence[data.seq_length++] = line[index];
                data.sequence[data.seq_length] = '\0';
            }
        }
    }

    return eof;
}

static void embl_print_lines_if_content(FILE *fp, const char *key, const char *content, const WrapMode& wrapMode, bool followed_by_spacer) {
    if (has_content(content)) {
        embl_print_lines(fp, key, content, wrapMode);
        followed_by_spacer && fputs("XX\n", fp);
    }
}

void embl_out(FILE * fp) {
    // Output EMBL data.
    int indi;

    WrapMode wrapWords(true);
    WrapMode neverWrap(false);

    embl_print_lines_if_content(fp, "ID", data.embl.id,          neverWrap,     true);
    embl_print_lines_if_content(fp, "AC", data.embl.accession,   wrapWords,     true);
    embl_print_lines_if_content(fp, "DT", data.embl.dateu,       neverWrap,     false);
    embl_print_lines_if_content(fp, "DT", data.embl.datec,       neverWrap,     true);
    // @@@ change behavior ? (print XX if any of the two DTs has been written)
    embl_print_lines_if_content(fp, "DE", data.embl.description, wrapWords,     true);
    embl_print_lines_if_content(fp, "KW", data.embl.keywords,    WrapMode(";"), true);

    if (has_content(data.embl.os)) {
        embl_print_lines(fp, "OS", data.embl.os, wrapWords);
        fputs("OC   No information.\n", fp);
        fputs("XX\n", fp);
    }

    // GenbankRef 
    for (indi = 0; indi < data.embl.numofref; indi++) {
        const Emblref& ref = data.embl.reference[indi];

        fprintf(fp, "RN   [%d]\n", indi + 1);
        embl_print_lines_if_content(fp, "RP", ref.processing, neverWrap, false);
        embl_print_lines_if_content(fp, "RA", ref.author, WrapMode(","), false);

        if (has_content(ref.title)) embl_print_lines(fp, "RT", ref.title, wrapWords);
        else fputs("RT   ;\n", fp);
        
        embl_print_lines_if_content(fp, "RL", ref.journal, wrapWords, false);
        fputs("XX\n", fp);
    }

    if (has_content(data.embl.dr)) {
        embl_print_lines(fp, "DR", data.embl.dr, wrapWords);
        fputs("XX\n", fp);
    }

    embl_out_comments(fp);
    embl_out_origin(fp);
}

int WrapMode::wrap_pos(const char *str, int wrapCol) const {
    // returns the first position lower equal 'wrapCol' after which splitting
    // is possible (according to separators of WrapMode)
    //
    // returns 'wrapCol' if no such position exists (fallback)
    //
    // throws if WrapMode is disabled

    if (!allowed_to_wrap()) throw_errorf(50, "Oversized content - no wrapping allowed here (content='%s')", str);

    ca_assert(wrapCol <= (int)strlen(str)); // to short to wrap

    const char *wrapAfter = get_seps();
    ca_assert(wrapAfter);
    int i = wrapCol;
    for (; i >= 0 && !occurs_in(str[i], wrapAfter); --i) {}
    return i >= 0 ? i : wrapCol;
}



void embl_print_lines(FILE * fp, const char *key, const char *content, const WrapMode& wrapMode) { // @@@ WRAPPER
    // Print EMBL entry and wrap around if line over EMBLMAXLINE.
    ca_assert(strlen(key) == 2);
    
    char prefix[TOKENSIZE];
    sprintf(prefix, "%-*s", EMBLINDENT, key);

    // print_wrapped(fp, prefix, prefix, content, wrapMode, EMBLMAXLINE, true); // @@@ wanted
    print_wrapped(fp, prefix, prefix, content, wrapMode, EMBLMAXLINE-1, WRAPBUG_WRAP_AT_SPACE); // @@@ 2 BUGs
}

inline void embl_print_completeness(FILE *fp, char compX, char X) {
    if (compX == ' ') return;
    ca_assert(compX == 'y' || compX == 'n');
    fprintf(fp, "CC     %c' end complete: %s\n", X, compX == 'y' ? "Yes" : "No");
}

void embl_out_comments(FILE * fp) {
    // Print out the comments part of EMBL format.

    OrgInfo& orginf = data.embl.comments.orginf;
    if (orginf.exists) {
        fputs("CC   Organism information\n", fp);

        embl_print_comment_if_content(fp, "Source of strain: ",   orginf.source);
        embl_print_comment_if_content(fp, "Culture collection: ", orginf.cultcoll);
        embl_print_comment_if_content(fp, "Former name: ",        orginf.formname);
        embl_print_comment_if_content(fp, "Alternate name: ",     orginf.nickname);
        embl_print_comment_if_content(fp, "Common name: ",        orginf.commname);
        embl_print_comment_if_content(fp, "Host organism: ",      orginf.hostorg);
    }                           // organism information 

    SeqInfo& seqinf = data.embl.comments.seqinf;
    if (seqinf.exists) {
        fprintf(fp, "CC   Sequence information (bases 1 to %d)\n", data.seq_length);

        embl_print_comment_if_content(fp, "RDP ID: ",                      seqinf.RDPid);
        embl_print_comment_if_content(fp, "Corresponding GenBank entry: ", seqinf.gbkentry);
        embl_print_comment_if_content(fp, "Sequencing methods: ",          seqinf.methods);

        embl_print_completeness(fp, seqinf.comp5, '5');
        embl_print_completeness(fp, seqinf.comp3, '3');
    }

    embl_print_lines_if_content(fp, "CC", data.embl.comments.others, WrapMode("\n"), true);
}

void embl_print_comment_if_content(FILE * fp, const char *key, const char *content) { // @@@ WRAPPER
    // Print one embl comment line, wrap around

    if (!has_content(content)) return;

    char first[LINESIZE]; sprintf(first, "CC%*s%s", (EMBLINDENT-2)+RDP_SUBKEY_INDENT, "", key);
    char other[LINESIZE]; sprintf(other, "CC%*s", (EMBLINDENT-2)+RDP_SUBKEY_INDENT+RDP_CONTINUED_INDENT, "");
    // print_wrapped(fp, first, other, content, WrapMode(true), EMBLMAXLINE, false); // @@@ wanted
    print_wrapped(fp, first, other, content, WrapMode(true), EMBLMAXLINE-2, WRAP_CORRECTLY); // @@@ 1 BUG
}

/* -----------------------------------------------------
 *      Function embl_out_origin().
 *              Print out the sequence data of EMBL format.
 */
void embl_out_origin(FILE * fp) {
    int base_a, base_c, base_t, base_g, base_other;
    int indi, indj, indk;

    // print seq data 
    count_bases(&base_a, &base_t, &base_g, &base_c, &base_other);

    fprintf(fp, "SQ   Sequence %d BP; %d A; %d C; %d G; %d T; %d other;\n", data.seq_length, base_a, base_c, base_g, base_t, base_other);

    for (indi = 0, indj = 0, indk = 1; indi < data.seq_length; indi++) {
        if ((indk % 60) == 1)
            fputs("     ", fp);
        fputc(data.sequence[indi], fp);
        indj++;
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
    if ((indk % 60) != 1) fputc('\n', fp);
    fputs("//\n", fp);
}

/* ----------------------------------------------------------
 *      Function embl_to_macke().
 *      Convert from Embl format to Macke format.
 */
void embl_to_macke(const char *inf, const char *outf, int format) {
    FILE        *IFP = open_input_or_die(inf);
    FILE_BUFFER  ifp = create_FILE_BUFFER(inf, IFP);
    FILE        *ofp = open_output_or_die(outf);
    
    int indi, total_num;

    // macke format seq irrelevant header 
    macke_out_header(ofp);
    for (indi = 0; indi < 3; indi++) {
        FILE_BUFFER_rewind(ifp);
        reset_seq_data();
        reinit_em_data();
        while (embl_in(ifp) != EOF) {
            data.numofseq++;
            if (etom()) {
                // convert from embl form to macke form 
                switch (indi) {
                    case 0: macke_seq_display_out(ofp, format); break;
                    case 1: macke_seq_info_out(ofp); break;
                    case 2: macke_seq_data_out(ofp); break;
                    default:;
                }
            }
            else throw_error(4, "Conversion from embl to macke fails");

            reinit_em_data();
        }
        total_num = data.numofseq;
        if (indi == 0) {
            fputs("#-\n", ofp);
            // no warning messages for next loop 
            warning_out = 0;
        }
    }

    warning_out = 1;

    log_processed(total_num);
    fclose(ofp);
    destroy_FILE_BUFFER(ifp);
}

/* ------------------------------------------------------------
 *      Function etom().
 *              Convert from embl format to Macke format.
 */
int etom() {
    // Responsibility note: caller has to cleanup embl and macke data

    int ok = etog() && gtom();
    reinit_genbank(); // cleanup afterwards!
    return ok;
}

/* -------------------------------------------------------------
 *      Function embl_to_embl().
 *              Print out EMBL data.
 */
void embl_to_embl(const char *inf, const char *outf) {
    FILE        *IFP = open_input_or_die(inf);
    FILE_BUFFER  ifp = create_FILE_BUFFER(inf, IFP);
    FILE        *ofp = open_output_or_die(outf);

    reset_seq_data();
    reinit_embl();

    while (embl_in(ifp) != EOF) {
        data.numofseq++;
        embl_out(ofp);
        reinit_embl();
    }

    log_processed(data.numofseq);
    fclose(ofp);
    destroy_FILE_BUFFER(ifp);
}

/* -------------------------------------------------------------
 *      Function embl_to_genbank().
 *              Convert from EMBL format to genbank format.
 */
void embl_to_genbank(const char *inf, const char *outf) {
    FILE        *IFP = open_input_or_die(inf);
    FILE_BUFFER  ifp = create_FILE_BUFFER(inf, IFP);
    FILE        *ofp = open_output_or_die(outf);

    reset_seq_data();
    reinit_genbank();
    reinit_embl();

    while (embl_in(ifp) != EOF) {
        data.numofseq++;
        if (etog())
            genbank_out(ofp);
        else
            throw_error(32, "Conversion from macke to genbank fails");

        reinit_genbank();
        reinit_embl();
    }

    log_processed(data.numofseq);
    fclose(ofp);
    destroy_FILE_BUFFER(ifp);
}

/* -------------------------------------------------------------
 *      Function etog()
 *              Convert from embl to genbank format.
 */
int etog() {
    // Responsibility note: caller has to cleanup embl and genbank data
    
    int  indi;
    char key[TOKENSIZE], temp[LONGTEXT];
    char t1[TOKENSIZE], t2[TOKENSIZE], t3[TOKENSIZE];

    embl_key_word(data.embl.id, 0, key, TOKENSIZE);
    if (has_content(data.embl.dr)) {
        // get short_id from DR line if there is RDP def. 
        strcpy(t3, "dummy");
        ASSERT_RESULT(int, 3, sscanf(data.embl.dr, "%s %s %s", t1, t2, t3));
        if (str_equal(t1, "RDP;")) {
            if (!str_equal(t3, "dummy")) {
                strcpy(key, t3);
            }
            else
                strcpy(key, t2);
            key[str0len(key) - 1] = '\0';        // remove '.' 
        }
    }
    strcpy(temp, key);

    // LOCUS 
    for (indi = str0len(temp); indi < 13; temp[indi++] = ' ') {}
#if 1
    // @@@ use else-version when done with refactoring
    // fixes two bugs in LOCUS line (id changed, wrong number of base positions)  
    if (has_content(data.embl.dateu)) {
        sprintf((temp + 10), "%7d bp    RNA             RNA       %s\n", data.seq_length, genbank_date(data.embl.dateu));
    }
    else {
        sprintf((temp + 10), "7%d bp    RNA             RNA       %s\n", data.seq_length, genbank_date(today_date()));
    }
#else
    {
        const char *date = has_content(data.embl.dateu) ? data.embl.dateu : today_date();
        sprintf((temp + 10), "%7d bp    RNA             RNA       %s\n",
                data.seq_length,
                genbank_date(date));
    }
#endif
    freedup(data.gbk.locus, temp);

    // DEFINITION 
    if (has_content(data.embl.description)) {
        freedup(data.gbk.definition, data.embl.description);

        // must have a period at the end 
        terminate_with(data.gbk.definition, '.');
    }

    // SOURCE and DEFINITION if not yet defined 
    if (has_content(data.embl.os)) {
        freedup(data.gbk.source, data.embl.os);
        freedup(data.gbk.organism, data.embl.os);
        if (!has_content(data.embl.description)) {
            freedup(data.gbk.definition, data.embl.os);
        }
    }

    // COMMENT GenBank entry 
    freedup_if_content(data.gbk.accession, data.embl.accession);
    if (has_content(data.embl.keywords) && data.embl.keywords[0] != '.') {
        freedup(data.gbk.keywords, data.embl.keywords);
    }

    etog_convert_references();
    etog_convert_comments();

    return (1);
}

/* ---------------------------------------------------------------------
 *      Function etog_convert_references().
 *              Convert reference from EMBL to GenBank format.
 */

void etog_convert_references() {
    int indi, len, start, end;
    char temp[LONGTEXT];

    data.gbk.numofref = data.embl.numofref;
    data.gbk.reference = (GenbankRef *) calloc(1, sizeof(GenbankRef) * data.embl.numofref);

    for (indi = 0; indi < data.embl.numofref; indi++) {
        const Emblref& ref = data.embl.reference[indi];
        

        if (has_content(ref.processing) &&
            sscanf(ref.processing, "%d %d", &start, &end) == 2)
        {
            end *= -1; // will get negative from sscanf 
            sprintf(temp, "%d  (bases %d to %d)\n", (indi + 1), start, end);
        }
        else {
            sprintf(temp, "%d\n", (indi + 1));
        }
        data.gbk.reference[indi].ref = nulldup(temp);

        if (has_content(ref.title) && ref.title[0] != ';') {
            // remove '"' and ';', if there is any 
            len = str0len(ref.title);
            if (len > 2 && ref.title[0] == '"'
                && ref.title[len - 2] == ';' && ref.title[len - 3] == '"') {
                ref.title[len - 3] = '\n';
                ref.title[len - 2] = '\0';
                data.gbk.reference[indi].title = nulldup(ref.title + 1);
                ref.title[len - 3] = '"';
                ref.title[len - 2] = ';';
            }
            else
                data.gbk.reference[indi].title = nulldup(ref.title);
        }
        else
            data.gbk.reference[indi].title = no_content();

        // GenBank AUTHOR 
        if (has_content(ref.author))
            data.gbk.reference[indi].author = etog_author(ref.author);
        else
            data.gbk.reference[indi].author = no_content();

        if (has_content(ref.journal))
            data.gbk.reference[indi].journal = etog_journal(ref.journal);
        else
            data.gbk.reference[indi].journal = no_content();

        data.gbk.reference[indi].standard = no_content();
    }
}

/* -----------------------------------------------------------------
 *      Function etog_author().
 *              Convert EMBL author format to Genbank author format.
 */
char *etog_author(char *Str) {
    int  indi, indk, len, index;
    char token[TOKENSIZE], *author;

    author = strdup("");
    for (indi = index = 0, len = str0len(Str) - 1; indi < len; indi++, index++) {
        if (Str[indi] == ',' || Str[indi] == ';') {
            token[index--] = '\0';
            if (Str[indi] == ',') {
                if (str0len(author) > 0)
                    Append(author, ",");
            }
            else if (str0len(author) > 0) {
                Append(author, " and");
            }
            // search backward to find the first blank and replace the blank by ',' 
            for (indk = 0; index > 0 && indk == 0; index--)
                if (token[index] == ' ') {
                    token[index] = ',';
                    indk = 1;
                }
            Append(author, token);
            index = (-1);
        }
        else
            token[index] = Str[indi];
    }
    Append(author, "\n");
    return (author);
}

/* ------------------------------------------------------------
 *      Function etog_journal().
 *              Convert jpurnal part from EMBL to GenBank format.
 */
char *etog_journal(const char *eJournal) {
    char *new_journal = 0;
    char  token[TOKENSIZE];

    scan_token_or_die(eJournal, token, NULL);
    if (str_equal(token, "(in)") == 1 || str_equal(token, "Submitted") || str_equal(token, "Unpublished")) {
        // remove trailing '.' 
        int len     = str0len(eJournal);
        ca_assert(eJournal[len-2] == '.');
        new_journal = strndup(eJournal, len-2);
        Append(new_journal, "\n");
    }
    else {
        const char *colon = strchr(eJournal, ':');

        if (colon) {
            const char *p1 = strchr(colon+1, '(');
            if (p1) {
                const char *p2 = strchr(p1+1, ')');
                if (p2 && strcmp(p2+1, ".\n") == 0) {
                    new_journal = Reallocspace(new_journal, str0len(eJournal)+1+1);

                    int l1 = colon-eJournal;
                    int l2 = p1-colon-1;
                    int l3 = p2-p1+1;

                    char *pos = new_journal;

                    memcpy(pos, eJournal, l1); pos += l1;
                    memcpy(pos, ", ",     2);  pos += 2;
                    memcpy(pos, colon+1,  l2); pos += l2;
                    memcpy(pos, " ",      1);  pos += 1;
                    memcpy(pos, p1,       l3); pos += l3;
                    memcpy(pos, "\n",     2);  
                }
            }
        }

        if (!new_journal) {
            warningf(148, "Removed unknown journal format: %s", eJournal);
            new_journal = no_content();
        }
    }

    return new_journal;
}

// --------------------------------------------------------------------------------

#if (UNIT_TESTS == 1)
#include <test_unit.h>

#define TEST_ASSERT_ETOG_JOURNAL_PARSES(i,o)              \
    do {                                                  \
        char *dup = strdup(i);                            \
        char *res = etog_journal(dup);                    \
        TEST_ASSERT_EQUAL(res, o);                        \
        free(res);                                        \
        free(dup);                                        \
    } while(0)

void TEST_BASIC_etog_journal() {
    // behavior documented in r6943:
    TEST_ASSERT_ETOG_JOURNAL_PARSES("Gene 134:283-287(1993).\n",
                                    "Gene 134, 283-287 (1993)\n");
    TEST_ASSERT_ETOG_JOURNAL_PARSES("J. Exp. Med. 179:1809-1821(1994).\n",
                                    "J. Exp. Med. 179, 1809-1821 (1994)\n");
    TEST_ASSERT_ETOG_JOURNAL_PARSES("Unpublished whatever.\n",
                                    "Unpublished whatever\n");
    TEST_ASSERT_ETOG_JOURNAL_PARSES("bla bla bla.\n",
                                    "\n"); // skips if can't parse
    TEST_ASSERT_ETOG_JOURNAL_PARSES("bla bla bla\n",
                                    "\n");
}

#endif // UNIT_TESTS


/* ---------------------------------------------------------------
 *      Function etog_convert_comments().
 *              Convert comment part from EMBL to GenBank.
 */
void etog_convert_comments() {
    // RDP defined Organism Information comments 
    OrgInfo& eorginf = data.embl.comments.orginf;
    OrgInfo& gorginf = data.gbk.comments.orginf;

    gorginf.exists = eorginf.exists;

    freedup_if_content(gorginf.source,   eorginf.source);
    freedup_if_content(gorginf.cultcoll, eorginf.cultcoll);
    freedup_if_content(gorginf.formname, eorginf.formname);
    freedup_if_content(gorginf.nickname, eorginf.nickname);
    freedup_if_content(gorginf.commname, eorginf.commname);
    freedup_if_content(gorginf.hostorg,  eorginf.hostorg);

     // RDP defined Sequence Information comments 
    SeqInfo& eseqinf = data.embl.comments.seqinf;
    SeqInfo& gseqinf = data.gbk.comments.seqinf;
    
    gseqinf.exists = eseqinf.exists;

    freedup_if_content(gseqinf.RDPid,    eseqinf.RDPid);
    freedup_if_content(gseqinf.gbkentry, eseqinf.gbkentry);
    freedup_if_content(gseqinf.methods,  eseqinf.methods);
    gseqinf.comp5 = eseqinf.comp5;
    gseqinf.comp3 = eseqinf.comp3;

    // other comments 
    freedup_if_content(data.gbk.comments.others, data.embl.comments.others);
}

/* ----------------------------------------------------------------
 *      Function genbank_to_embl().
 *              Convert from genbank to EMBL.
 */
void genbank_to_embl(const char *inf, const char *outf) {
    FILE        *IFP = open_input_or_die(inf);
    FILE_BUFFER  ifp = create_FILE_BUFFER(inf, IFP);
    FILE        *ofp = open_output_or_die(outf);
    
    reinit_genbank();
    reinit_embl();
    while (genbank_in(ifp) != EOF) {
        data.numofseq++;
        if (gtoe())
            embl_out(ofp);
        reinit_genbank();
        reinit_embl();
    }

    log_processed(data.numofseq);
    destroy_FILE_BUFFER(ifp);
    fclose(ofp);
}

int gtoe() {
    // Genbank to EMBL.
    // Responsibility note: caller has to cleanup genbank and embl data

    GenBank& gbk = data.gbk;

    {
        char temp[LONGTEXT];
        genbank_key_word(gbk.locus, 0, temp, TOKENSIZE);
        // Adjust short-id, EMBL short_id always upper case 
        upcase(temp);

        int indi = min(str0len(temp), 9);
        for (; indi < 10; indi++) temp[indi] = ' ';

        sprintf(temp + 10, "preliminary; RNA; UNA; %d BP.\n", data.seq_length);
        freedup(data.embl.id, temp);
    }
    
    // accession number 
    if (has_content(gbk.accession))
        // take just the accession num, no version num. 
        freedup(data.embl.accession, gbk.accession);

    // date 
    {
        char *date = gbk.get_date();

        freeset(data.embl.dateu, strf("%s (Rel. 1, Last updated, Version 1)\n", date));
        freeset(data.embl.datec, strf("%s (Rel. 1, Created)\n", date));

        free(date);
    }

    // description 
    freedup_if_content(data.embl.description, gbk.definition);
    // EMBL KW line 
    if (has_content(gbk.keywords)) {
        freedup(data.embl.keywords, gbk.keywords);
        terminate_with(data.embl.keywords, '.');
    }
    else {
        freedup(data.embl.keywords, ".\n");
    }

    freedup_if_content(data.embl.os, gbk.organism); // EMBL OS line 
    // reference 
    gtoe_reference();

    // EMBL DR line 
    {
        char token[TOKENSIZE];
        char temp[LONGTEXT];
        
        scan_token_or_die(gbk.locus, token, NULL); // short_id 
        if (has_content(gbk.comments.seqinf.RDPid)) {
            char rdpid[TOKENSIZE];
            scan_token_or_die(gbk.comments.seqinf.RDPid, rdpid, NULL);
            sprintf(temp, "RDP; %s; %s.\n", rdpid, token);
        }
        else {
            sprintf(temp, "RDP; %s.\n", token);
        }
        freedup(data.embl.dr, temp);
    }
    gtoe_comments();

    return (1);
}

/* ------------------------------------------------------------------
 *      Function gtoe_reference().
 *              Convert references from GenBank to EMBL.
 */
void gtoe_reference() {
    data.embl.numofref = data.gbk.numofref;

    if (data.gbk.numofref > 0) {
        data.embl.reference = (Emblref *) malloc(sizeof(Emblref) * data.gbk.numofref);
    }

    for (int indi = 0; indi < data.gbk.numofref; indi++) {
        Emblref& ref = data.embl.reference[indi];

        ref.title = nulldup(data.gbk.reference[indi].title);
        embl_verify_title(indi);
        ref.journal = gtoe_journal(data.gbk.reference[indi].journal);
        terminate_with(ref.journal, '.');
        ref.author = gtoe_author(data.gbk.reference[indi].author);
        terminate_with(ref.author, ';');

        // create processing information 
        int refnum, start = 0, end = 0;
        char t1[TOKENSIZE], t2[TOKENSIZE], t3[TOKENSIZE];

        if (!data.gbk.reference[indi].ref ||
            sscanf(data.gbk.reference[indi].ref, "%d %s %d %s %d %s", &refnum, t1, &start, t2, &end, t3) != 6)
        {
            start = 0;
            end = 0;
        }

        if (start || end) ref.processing = strf("%d-%d\n", start, end);
        else              ref.processing = no_content();

    }
}

/* ----------------------------------------------------------------
 *      Function gtoe_author().
 *              Convert GenBank author to EMBL author.
 */
char *gtoe_author(char *author) {
    int   indi, len, index, odd;
    char *auth, *Str;

    // replace " and " by ", " 
    auth = nulldup(author);
    if ((index = find_pattern(auth, " and ")) > 0) {
        auth[index] = '\0';
        Str = nulldup(auth);
        auth[index] = ' ';      // remove '\0' for free space later 
        Append(Str, ",");
        Append(Str, auth + index + 4);
    }
    else
        Str = nulldup(author);

    for (indi = 0, len = str0len(Str), odd = 1; indi < len; indi++) {
        if (Str[indi] == ',') {
            if (odd) {
                Str[indi] = ' ';
                odd = 0;
            }
            else {
                odd = 1;
            }
        }
    }

    freenull(auth);
    return (Str);
}

/* ------------------------------------------------------------
 *      Function gtoe_journal().
 *              Convert GenBank journal to EMBL journal.
 */
char *gtoe_journal(char *Str) {
    char token[TOKENSIZE], *journal;
    int  indi, indj, index, len;

    if (scan_token(Str, token)) {
        if (str_equal(token, "(in)") == 1 || str_equal(token, "Unpublished") || str_equal(token, "Submitted")) {
            journal = nulldup(Str);
            terminate_with(journal, '.');
            return (journal);
        }
    }

    journal = nulldup(Str);
    for (indi = indj = index = 0, len = str0len(journal); indi < len; indi++, indj++) {
        if (journal[indi] == ',') {
            journal[indi] = ':';
            indi++;             // skip blank after ',' 
            index = 1;
        }
        else if (journal[indi] == ' ' && index) {
            indj--;
        }
        else
            journal[indj] = journal[indi];
    }

    journal[indj] = '\0';
    terminate_with(journal, '.');
    return (journal);
}

/* ---------------------------------------------------------------
 *      Function gtoe_comments().
 *              Convert comment part from GenBank to EMBL.
 */
void gtoe_comments() {
    // RDP defined Organism Information comments 
    OrgInfo& eorginf = data.embl.comments.orginf;
    OrgInfo& gorginf = data.gbk.comments.orginf;

    eorginf.exists = gorginf.exists;

    freedup_if_content(eorginf.source,   gorginf.source);
    freedup_if_content(eorginf.cultcoll, gorginf.cultcoll);
    freedup_if_content(eorginf.formname, gorginf.formname);
    freedup_if_content(eorginf.nickname, gorginf.nickname);
    freedup_if_content(eorginf.commname, gorginf.commname);
    freedup_if_content(eorginf.hostorg,  gorginf.hostorg);

     // RDP defined Sequence Information comments 
    SeqInfo& eseqinf = data.embl.comments.seqinf;
    SeqInfo& gseqinf = data.gbk.comments.seqinf;
    
    eseqinf.exists = gseqinf.exists;

    freedup_if_content(eseqinf.RDPid,    gseqinf.RDPid);
    freedup_if_content(eseqinf.gbkentry, gseqinf.gbkentry);
    freedup_if_content(eseqinf.methods,  gseqinf.methods);
    eseqinf.comp5 = gseqinf.comp5;
    eseqinf.comp3 = gseqinf.comp3;

    // other comments 
    freedup_if_content(data.embl.comments.others, data.gbk.comments.others);
}

int mtoe() {
    // Responsibility note: caller has to cleanup macke and embl data
    int ok = mtog() && gtoe() && partial_mtoe();
    reinit_genbank();
    return ok;
}

/* ---------------------------------------------------------------
 *      Function macke_to_embl().
 *              Convert from macke to EMBL.
 */
void macke_to_embl(const char *inf, const char *outf) {
    FILE *ofp  = open_output_or_die(outf);

    MackeReader macke(inf);
    while (macke.mackeIn() != EOF) {
        data.numofseq++;

        /* partial_mtoe() is particularly handling
         * subspecies information, not converting whole
         * macke format to embl */

        if (mtoe())
            embl_out(ofp);
        else
            throw_error(14, "Conversion from macke to embl fails");

        reinit_embl();
        reinit_macke();
    }

    log_processed(data.numofseq);
    fclose(ofp);
}

/* --------------------------------------------------------------
 *      Function partial_mtoe().
 *              Handle subspecies information when converting from
 *                      Macke to EMBL.
 */
int partial_mtoe() {
    int indj, indk;

    if (has_content(data.macke.strain)) {
        if ((indj = find_pattern(data.embl.comments.others, "*source:")) >= 0 && (indk = find_pattern(data.embl.comments.others + indj, "strain=")) >= 0) {
            ;                   // do nothing 
        }
        else {
            if (str0len(data.embl.comments.others) <= 1)
                freenull(data.embl.comments.others);
            Append(data.embl.comments.others, "*source: strain=");
            Append(data.embl.comments.others, data.macke.strain);
            if (!is_end_mark(data.embl.comments.others[str0len(data.embl.comments.others) - 2])) {
                skip_eolnl_and_append(data.embl.comments.others, ";\n");
            }
        }
    }

    if (has_content(data.macke.subspecies)) {
        if ((indj = find_pattern(data.embl.comments.others, "*source:")) >= 0 &&
            ((indk = find_pattern(data.embl.comments.others + indj, "subspecies=")) >= 0 ||
             (indk = find_pattern(data.embl.comments.others + indj, "sub-species=")) >= 0 ||
             (indk = find_pattern(data.embl.comments.others + indj, "subsp.=")) >= 0)) {
            ;                   // do nothing 
        }
        else {
            if (str0len(data.embl.comments.others) <= 1)
                freenull(data.embl.comments.others);

            Append(data.embl.comments.others, "*source: subspecies=");
            Append(data.embl.comments.others, data.macke.subspecies);

            if (!is_end_mark(data.embl.comments.others[str0len(data.embl.comments.others) - 2])) {
                skip_eolnl_and_append(data.embl.comments.others, ";\n");
            }
        }
    }

    return (1);
}
