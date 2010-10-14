#include "embl.h"
#include "genbank.h"
#include "macke.h"
#include "wrap.h"

extern int warning_out;

/* ---------------------------------------------------------------
 *      Function embl_in().
 *              Read in one embl entry.
 */
char embl_in(Embl& embl, Seq& seq, FILE_BUFFER fp) { // @@@ use Reader
    char  line[LINESIZE], key[TOKENSIZE];
    char *eof, eoen;

    eoen = ' ';                 // end-of-entry, set to be 'y' after '//' is read

    for (eof = Fgetline(line, LINESIZE, fp); eof != NULL && eoen != 'y';) {
        if (str0len(line) <= 1) {
            eof = Fgetline(line, LINESIZE, fp);
            continue;           // empty line, skip 
        }

        embl_key_word(line, 0, key, TOKENSIZE);
        eoen = 'n';

        if (str_equal(key, "ID")) {
            eof = embl_one_entry(line, fp, embl.id, key);
        }
        else if (str_equal(key, "DT")) {
            eof = embl_date(embl, line, fp);
        }
        else if (str_equal(key, "DE")) {
            eof = embl_one_entry(line, fp, embl.description, key);
        }
        else if (str_equal(key, "OS")) {
            eof = embl_one_entry(line, fp, embl.os, key);
        }
        else if (str_equal(key, "AC")) {
            eof = embl_one_entry(line, fp, embl.accession, key);
        }
        else if (str_equal(key, "KW")) {
            eof = embl_one_entry(line, fp, embl.keywords, key);

            // correct missing '.' 
            if (str0len(embl.keywords) <= 1) freedup(embl.keywords, ".\n");
            else terminate_with(embl.keywords, '.');
        }
        else if (str_equal(key, "DR")) {
            eof = embl_one_entry(line, fp, embl.dr, key);
        }
        else if (str_equal(key, "RA")) {
            Emblref& ref = embl.get_latest_ref();
            eof          = embl_one_entry(line, fp, ref.author, key);
            terminate_with(ref.author, ';');
        }
        else if (str_equal(key, "RT")) {
            Emblref& ref = embl.get_latest_ref();
            eof = embl_one_entry(line, fp, ref.title, key);
            embl_correct_title(ref); 
        }
        else if (str_equal(key, "RL")) {
            Emblref& ref = embl.get_latest_ref();
            eof = embl_one_entry(line, fp, ref.journal, key);
            terminate_with(ref.journal, '.');
        }
        else if (str_equal(key, "RP")) {
            Emblref& ref = embl.get_latest_ref();
            eof = embl_one_entry(line, fp, ref.processing, key);
        }
        else if (str_equal(key, "RN")) {
            eof = embl_version(embl, line, fp);
        }
        else if (str_equal(key, "CC")) {
            eof = embl_comments(embl, line, fp);
        }
        else if (str_equal(key, "SQ")) {
            eof = embl_origin(seq, line, fp);
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
char embl_in_id(Embl& embl, Seq& seq, FILE_BUFFER fp) { // @@@ use reader
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
            eof = embl_one_entry(line, fp, embl.id, key);
        }
        else if (str_equal(key, "SQ")) {
            eof = embl_origin(seq, line, fp);
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
    }

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

void embl_correct_title(Emblref& ref) {
    // Check missing '"' at the both ends
    
    terminate_with(ref.title, ';');

    int len = str0len(ref.title);
    if (len > 2 && (ref.title[0] != '"' || ref.title[len - 3] != '"')) {
        char *temp = NULL;
        if (ref.title[0] != '"')
            temp = strdup("\"");
        else
            temp = strdup("");
        Append(temp, ref.title);
        if ((len > 2 && ref.title[len - 3]
             != '"')) {
            len = str0len(temp);
            temp[len - 2] = '"';
            terminate_with(temp, ';');
        }
        freedup(ref.title, temp);
        free(temp);
    }
}

/* --------------------------------------------------------------
 *      Function embl_date().
 *              Read in embl DATE lines.
 */
char *embl_date(Embl& embl, char *line, FILE_BUFFER fp) { // @@@ use Reader
    int   index;
    char *eof, key[TOKENSIZE];

    index = Skip_white_space(line, p_nonkey_start);
    freedup(embl.dateu, line + index);

    eof = Fgetline(line, LINESIZE, fp);
    embl_key_word(line, 0, key, TOKENSIZE);
    if (str_equal(key, "DT")) {
        index = Skip_white_space(line, p_nonkey_start);
        freedup(embl.datec, line + index);
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
char *embl_version(Embl& embl, char *line, FILE_BUFFER fp) { // @@@ use Reader
    int index;
    char *eof;

    index = Skip_white_space(line, p_nonkey_start);
    
    embl.resize_refs(embl.get_refcount()+1);

    eof = Fgetline(line, LINESIZE, fp);
    return (eof);
}

/* --------------------------------------------------------------
 *      Function embl_comments().
 *              Read in embl comment lines.
 */
char *embl_comments(Embl& embl, char *line, FILE_BUFFER fp) { // @@@ use Reader
    int   index, offset;
    char *eof;
    char  key[TOKENSIZE];

    OrgInfo& orginf = embl.comments.orginf;
    SeqInfo& seqinf = embl.comments.seqinf;

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
            if (embl.comments.others == NULL) {
                embl.comments.others = nulldup(line + 5);
            }
            else
                Append(embl.comments.others, line + 5);
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
    }
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
    }

    return eof;
}

/* --------------------------------------------------------------
 *      Function embl_origin().
 *              Read in embl sequence data.
 */
char *embl_origin(Seq& seq, char *line, FILE_BUFFER fp) { // @@@ use Reader
    char *eof;
    int index;

    ca_assert(seq.is_empty());
    
    // read in whole sequence data
    for (eof = Fgetline(line, LINESIZE, fp);
         eof != NULL && line[0] != '/' && line[1] != '/';
         eof = Fgetline(line, LINESIZE, fp))
    {
        for (index = 5; line[index] != '\n' && line[index] != '\0'; index++) {
            if (line[index] != ' ') {
                seq.add(line[index]);
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

void embl_out(const Embl& embl, const Seq& seq, FILE * fp) {
    // Output EMBL data.
    int indi;

    WrapMode wrapWords(true);
    WrapMode neverWrap(false);

    embl_print_lines_if_content(fp, "ID", embl.id,          neverWrap,     true);
    embl_print_lines_if_content(fp, "AC", embl.accession,   wrapWords,     true);
    embl_print_lines_if_content(fp, "DT", embl.dateu,       neverWrap,     false);
    embl_print_lines_if_content(fp, "DT", embl.datec,       neverWrap,     true);
    // @@@ change behavior ? (print XX if any of the two DTs has been written)
    embl_print_lines_if_content(fp, "DE", embl.description, wrapWords,     true);
    embl_print_lines_if_content(fp, "KW", embl.keywords,    WrapMode(";"), true);

    if (has_content(embl.os)) {
        embl_print_lines(fp, "OS", embl.os, wrapWords);
        fputs("OC   No information.\n", fp);
        fputs("XX\n", fp);
    }

    // GenbankRef 
    for (indi = 0; indi < embl.get_refcount(); indi++) {
        const Emblref& ref = embl.get_ref(indi);

        fprintf(fp, "RN   [%d]\n", indi + 1);
        embl_print_lines_if_content(fp, "RP", ref.processing, neverWrap, false);
        embl_print_lines_if_content(fp, "RA", ref.author, WrapMode(","), false);

        if (has_content(ref.title)) embl_print_lines(fp, "RT", ref.title, wrapWords);
        else fputs("RT   ;\n", fp);
        
        embl_print_lines_if_content(fp, "RL", ref.journal, wrapWords, false);
        fputs("XX\n", fp);
    }

    if (has_content(embl.dr)) {
        embl_print_lines(fp, "DR", embl.dr, wrapWords);
        fputs("XX\n", fp);
    }

    embl_out_comments(embl, seq, fp);
    embl_out_origin(seq, fp);
}

void embl_print_lines(FILE * fp, const char *key, const char *content, const WrapMode& wrapMode) { // @@@ WRAPPER
    // Print EMBL entry and wrap around if line over EMBLMAXLINE.
    ca_assert(strlen(key) == 2);
    
    char prefix[TOKENSIZE];
    sprintf(prefix, "%-*s", EMBLINDENT, key);

    // wrapMode.print(fp, prefix, prefix, content, EMBLMAXLINE, true); // @@@ wanted
    wrapMode.print(fp, prefix, prefix, content, EMBLMAXLINE-1, WRAPBUG_WRAP_AT_SPACE); // @@@ 2 BUGs
}

inline void embl_print_completeness(FILE *fp, char compX, char X) {
    if (compX == ' ') return;
    ca_assert(compX == 'y' || compX == 'n');
    fprintf(fp, "CC     %c' end complete: %s\n", X, compX == 'y' ? "Yes" : "No");
}

void embl_out_comments(const Embl& embl, const Seq& seq, FILE * fp) {
    // Print out the comments part of EMBL format.

    const OrgInfo& orginf = embl.comments.orginf;
    if (orginf.exists) {
        fputs("CC   Organism information\n", fp);

        embl_print_comment_if_content(fp, "Source of strain: ",   orginf.source);
        embl_print_comment_if_content(fp, "Culture collection: ", orginf.cultcoll);
        embl_print_comment_if_content(fp, "Former name: ",        orginf.formname);
        embl_print_comment_if_content(fp, "Alternate name: ",     orginf.nickname);
        embl_print_comment_if_content(fp, "Common name: ",        orginf.commname);
        embl_print_comment_if_content(fp, "Host organism: ",      orginf.hostorg);
    }

    const SeqInfo& seqinf = embl.comments.seqinf;
    if (seqinf.exists) {
        fprintf(fp, "CC   Sequence information (bases 1 to %d)\n", seq.get_len());

        embl_print_comment_if_content(fp, "RDP ID: ",                      seqinf.RDPid);
        embl_print_comment_if_content(fp, "Corresponding GenBank entry: ", seqinf.gbkentry);
        embl_print_comment_if_content(fp, "Sequencing methods: ",          seqinf.methods);

        embl_print_completeness(fp, seqinf.comp5, '5');
        embl_print_completeness(fp, seqinf.comp3, '3');
    }

    embl_print_lines_if_content(fp, "CC", embl.comments.others, WrapMode("\n"), true);
}

void embl_print_comment_if_content(FILE * fp, const char *key, const char *content) { // @@@ WRAPPER
    // Print one embl comment line, wrap around

    if (!has_content(content)) return;

    char first[LINESIZE]; sprintf(first, "CC%*s%s", (EMBLINDENT-2)+RDP_SUBKEY_INDENT, "", key);
    char other[LINESIZE]; sprintf(other, "CC%*s", (EMBLINDENT-2)+RDP_SUBKEY_INDENT+RDP_CONTINUED_INDENT, "");
    // WrapMode(true).print(fp, first, other, content, EMBLMAXLINE, false); // @@@ wanted
    WrapMode(true).print(fp, first, other, content, EMBLMAXLINE-2, WRAP_CORRECTLY); // @@@ 1 BUG
}

/* -----------------------------------------------------
 *      Function embl_out_origin().
 *              Print out the sequence data of EMBL format.
 */
void embl_out_origin(const Seq& seq, FILE *fp) {

    // print sequence data

    BaseCounts bases;
    seq.count(bases);
    fprintf(fp, "SQ   Sequence %d BP; %d A; %d C; %d G; %d T; %d other;\n",
            seq.get_len(), bases.a, bases.c, bases.g, bases.t, bases.other);

    const char *sequence = seq.get_seq();
    int indi, indj, indk;
    for (indi = 0, indj = 0, indk = 1; indi < seq.get_len(); indi++) {
        if ((indk % 60) == 1)
            fputs("     ", fp);
        fputc(sequence[indi], fp);
        indj++;
        if ((indk % 60) == 0) {
            fputc('\n', fp);
            indj = 0;
        }
        else if (indj == 10 && indi != (seq.get_len() - 1)) {
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

    macke_out_header(ofp); // macke format sequence irrelevant header

    int total_num;
    for (int indi = 0; indi < 3; indi++) {
        FILE_BUFFER_rewind(ifp);

        int numofseq = 0;

        while (1) {
            Embl embl;
            Seq  seq;
            if (embl_in(embl, seq, ifp) == EOF) break;

            numofseq++;
            Macke macke;
            if (!etom(embl, macke, seq)) throw_conversion_failure(EMBL, MACKE);
            
            switch (indi) {
                case 0: macke_seq_display_out(macke, ofp, format, numofseq == 1); break;
                case 1: macke_seq_info_out(macke, ofp); break;
                case 2: macke_seq_data_out(seq, macke, ofp); break;
                default:;
            }
        }
        total_num = numofseq;
        if (indi == 0) {
            fputs("#-\n", ofp);
            warning_out = 0; // no warning messages for next loop
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
int etom(const Embl& embl, Macke& macke, const Seq& seq) { // __ATTR__USERESULT
    GenBank gbk;
    return etog(embl, gbk, seq) && gtom(gbk, macke);
}

/* -------------------------------------------------------------
 *      Function embl_to_embl().
 *              Print out EMBL data.
 */
void embl_to_embl(const char *inf, const char *outf) {
    FILE        *IFP      = open_input_or_die(inf);
    FILE_BUFFER  ifp      = create_FILE_BUFFER(inf, IFP);
    FILE        *ofp      = open_output_or_die(outf);
    int          numofseq = 0;

    while (1) {
        Embl embl;
        Seq  seq;
        if (embl_in(embl, seq, ifp) == EOF) break;

        numofseq++;
        embl_out(embl, seq, ofp);
    }

    log_processed(numofseq);
    fclose(ofp);
    destroy_FILE_BUFFER(ifp);
}

/* -------------------------------------------------------------
 *      Function embl_to_genbank().
 *              Convert from EMBL format to genbank format.
 */
void embl_to_genbank(const char *inf, const char *outf) {
    FILE        *IFP      = open_input_or_die(inf);
    FILE_BUFFER  ifp      = create_FILE_BUFFER(inf, IFP);
    FILE        *ofp      = open_output_or_die(outf);
    int          numofseq = 0;

    while (1) {
        Embl embl;
        Seq  seq;
        if (embl_in(embl, seq, ifp) == EOF) break;

        numofseq++;
        GenBank gbk;
        if (!etog(embl, gbk, seq)) throw_conversion_failure(MACKE, GENBANK);
        genbank_out(gbk, seq, ofp);
    }

    log_processed(numofseq);
    fclose(ofp);
    destroy_FILE_BUFFER(ifp);
}

/* -------------------------------------------------------------
 *      Function etog()
 *              Convert from embl to genbank format.
 */
int etog(const Embl& embl, GenBank& gbk, const Seq& seq) {  // __ATTR__USERESULT
    int      indi;
    char     key[TOKENSIZE], temp[LONGTEXT];
    char     t1[TOKENSIZE], t2[TOKENSIZE], t3[TOKENSIZE];

    embl_key_word(embl.id, 0, key, TOKENSIZE);
    if (has_content(embl.dr)) {
        // get short_id from DR line if there is RDP def.
        strcpy(t3, "dummy");
        ASSERT_RESULT(int, 3, sscanf(embl.dr, "%s %s %s", t1, t2, t3));
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
    if (has_content(embl.dateu)) {
        sprintf((temp + 10), "%7d bp    RNA             RNA       %s\n", seq.get_len(), genbank_date(embl.dateu));
    }
    else {
        sprintf((temp + 10), "7%d bp    RNA             RNA       %s\n", seq.get_len(), genbank_date(today_date()));
    }
#else
    {
        const char *date = has_content(embl.dateu) ? embl.dateu : today_date();
        sprintf((temp + 10), "%7d bp    RNA             RNA       %s\n",
                seq.get_len(),
                genbank_date(date));
    }
#endif
    freedup(gbk.locus, temp);

    // DEFINITION 
    if (has_content(embl.description)) {
        freedup(gbk.definition, embl.description);

        // must have a period at the end 
        terminate_with(gbk.definition, '.');
    }

    // SOURCE and DEFINITION if not yet defined 
    if (has_content(embl.os)) {
        freedup(gbk.source, embl.os);
        freedup(gbk.organism, embl.os);
        if (!has_content(embl.description)) {
            freedup(gbk.definition, embl.os);
        }
    }

    // COMMENT GenBank entry 
    freedup_if_content(gbk.accession, embl.accession);
    if (has_content(embl.keywords) && embl.keywords[0] != '.') {
        freedup(gbk.keywords, embl.keywords);
    }

    etog_convert_references(embl, gbk);
    etog_convert_comments(embl, gbk);

    return (1);
}

/* ---------------------------------------------------------------------
 *      Function etog_convert_references().
 *              Convert reference from EMBL to GenBank format.
 */

void etog_convert_references(const Embl& embl, GenBank& gbk) {
    int  indi, len, start, end;
    char temp[LONGTEXT];

    gbk.resize_refs(embl.get_refcount());

    for (indi = 0; indi < embl.get_refcount(); indi++) {
        const Emblref& ref  = embl.get_ref(indi);
        GenbankRef&    gref = gbk.get_ref(indi);

        if (has_content(ref.processing) &&
            sscanf(ref.processing, "%d %d", &start, &end) == 2)
        {
            end *= -1; // will get negative from sscanf 
            sprintf(temp, "%d  (bases %d to %d)\n", (indi + 1), start, end);
        }
        else {
            sprintf(temp, "%d\n", (indi + 1));
        }

        freedup(gref.ref, temp);

        if (has_content(ref.title) && ref.title[0] != ';') {
            // remove '"' and ';', if there is any
            len = str0len(ref.title);
            if (len > 2 && ref.title[0] == '"' && ref.title[len - 2] == ';' && ref.title[len - 3] == '"') {
                ref.title[len - 3] = '\n';
                ref.title[len - 2] = '\0';
                freedup(gref.title, ref.title+1);
                ref.title[len - 3] = '"';
                ref.title[len - 2] = ';';
            }
            else {
                freedup(gref.title, ref.title);
            }
        }
        else {
            freeset(gref.title, no_content());
        }


        freeset(gref.author, has_content(ref.author) ? etog_author(ref.author) : no_content());
        freeset(gref.journal, has_content(ref.journal) ? etog_journal(ref.journal) : no_content());

        freeset(gref.standard, no_content());
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
void etog_convert_comments(const Embl& embl, GenBank& gbk) {
    // RDP defined Organism Information comments
    const OrgInfo& eorginf = embl.comments.orginf;
    OrgInfo&       gorginf = gbk.comments.orginf;

    gorginf.exists = eorginf.exists;

    freedup_if_content(gorginf.source,   eorginf.source);
    freedup_if_content(gorginf.cultcoll, eorginf.cultcoll);
    freedup_if_content(gorginf.formname, eorginf.formname);
    freedup_if_content(gorginf.nickname, eorginf.nickname);
    freedup_if_content(gorginf.commname, eorginf.commname);
    freedup_if_content(gorginf.hostorg,  eorginf.hostorg);

    // RDP defined Sequence Information comments
    const SeqInfo& eseqinf = embl.comments.seqinf;
    SeqInfo&       gseqinf = gbk.comments.seqinf;

    gseqinf.exists = eseqinf.exists;

    freedup_if_content(gseqinf.RDPid,    eseqinf.RDPid);
    freedup_if_content(gseqinf.gbkentry, eseqinf.gbkentry);
    freedup_if_content(gseqinf.methods,  eseqinf.methods);
    gseqinf.comp5 = eseqinf.comp5;
    gseqinf.comp3 = eseqinf.comp3;

    // other comments 
    freedup_if_content(gbk.comments.others, embl.comments.others);
}

/* ----------------------------------------------------------------
 *      Function genbank_to_embl().
 *              Convert from genbank to EMBL.
 */
void genbank_to_embl(const char *inf, const char *outf) {
    FILE        *IFP      = open_input_or_die(inf);
    FILE_BUFFER  ifp      = create_FILE_BUFFER(inf, IFP);
    FILE        *ofp      = open_output_or_die(outf);
    int          numofseq = 0;

    while (1) {
        GenBank gbk;
        Seq     seq;

        if (genbank_in(gbk, seq, ifp) == EOF) break;
        numofseq++;
        
        Embl embl;
        if (!gtoe(gbk, embl, seq)) throw_conversion_failure(GENBANK, EMBL);
        embl_out(embl, seq, ofp);
    }

    log_processed(numofseq);
    destroy_FILE_BUFFER(ifp);
    fclose(ofp);
}

int gtoe(const GenBank& gbk, Embl& embl, const Seq& seq) { // __ATTR__USERESULT
    // Genbank to EMBL.
    {
        char temp[LONGTEXT];
        genbank_key_word(gbk.locus, 0, temp, TOKENSIZE);
        // Adjust short-id, EMBL short_id always upper case 
        upcase(temp);

        int indi = min(str0len(temp), 9);
        for (; indi < 10; indi++) temp[indi] = ' ';

        sprintf(temp + 10, "preliminary; RNA; UNA; %d BP.\n", seq.get_len());
        freedup(embl.id, temp);
    }
    
    // accession number 
    if (has_content(gbk.accession))
        // take just the accession num, no version num. 
        freedup(embl.accession, gbk.accession);

    // date 
    {
        char *date = gbk.get_date();

        freeset(embl.dateu, strf("%s (Rel. 1, Last updated, Version 1)\n", date));
        freeset(embl.datec, strf("%s (Rel. 1, Created)\n", date));

        free(date);
    }

    // description 
    freedup_if_content(embl.description, gbk.definition);
    // EMBL KW line 
    if (has_content(gbk.keywords)) {
        freedup(embl.keywords, gbk.keywords);
        terminate_with(embl.keywords, '.');
    }
    else {
        freedup(embl.keywords, ".\n");
    }

    freedup_if_content(embl.os, gbk.organism); // EMBL OS line 
    // reference 
    gtoe_reference(gbk, embl);

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
        freedup(embl.dr, temp);
    }
    gtoe_comments(gbk, embl);

    return (1);
}

/* ------------------------------------------------------------------
 *      Function gtoe_reference().
 *              Convert references from GenBank to EMBL.
 */
void gtoe_reference(const GenBank& gbk, Embl& embl) {

    if (gbk.get_refcount() > 0) {
        embl.resize_refs(gbk.get_refcount());
    }

    for (int indi = 0; indi < gbk.get_refcount(); indi++) {
        Emblref&          ref  = embl.get_ref(indi);
        const GenbankRef& gref = gbk.get_ref(indi);

        freedup(ref.title, gref.title);
        embl_correct_title(ref);

        freeset(ref.journal, gtoe_journal(gref.journal));
        terminate_with(ref.journal, '.');
        
        freeset(ref.author, gtoe_author(gref.author));
        terminate_with(ref.author, ';');

        // create processing information
        int refnum, start = 0, end = 0;
        char t1[TOKENSIZE], t2[TOKENSIZE], t3[TOKENSIZE];

        if (!gref.ref || sscanf(gref.ref, "%d %s %d %s %d %s", &refnum, t1, &start, t2, &end, t3) != 6) {
            start = 0;
            end = 0;
        }

        freenull(ref.processing);
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
void gtoe_comments(const GenBank& gbk, Embl& embl) {
    // RDP defined Organism Information comments
    OrgInfo&       eorginf = embl.comments.orginf;
    const OrgInfo& gorginf = gbk.comments.orginf;

    eorginf.exists = gorginf.exists;

    freedup_if_content(eorginf.source,   gorginf.source);
    freedup_if_content(eorginf.cultcoll, gorginf.cultcoll);
    freedup_if_content(eorginf.formname, gorginf.formname);
    freedup_if_content(eorginf.nickname, gorginf.nickname);
    freedup_if_content(eorginf.commname, gorginf.commname);
    freedup_if_content(eorginf.hostorg,  gorginf.hostorg);

    // RDP defined Sequence Information comments
    SeqInfo&       eseqinf = embl.comments.seqinf;
    const SeqInfo& gseqinf = gbk.comments.seqinf;

    eseqinf.exists = gseqinf.exists;

    freedup_if_content(eseqinf.RDPid,    gseqinf.RDPid);
    freedup_if_content(eseqinf.gbkentry, gseqinf.gbkentry);
    freedup_if_content(eseqinf.methods,  gseqinf.methods);
    eseqinf.comp5 = gseqinf.comp5;
    eseqinf.comp3 = gseqinf.comp3;

    // other comments 
    freedup_if_content(embl.comments.others, gbk.comments.others);
}

int mtoe(const Macke& macke, Embl& embl, const Seq& seq) { // __ATTR__USERESULT
    GenBank gbk;
    return mtog(macke, gbk, seq) && gtoe(gbk, embl, seq) && partial_mtoe(macke, embl);
}

/* ---------------------------------------------------------------
 *      Function macke_to_embl().
 *              Convert from macke to EMBL.
 */
void macke_to_embl(const char *inf, const char *outf) {
    FILE        *ofp      = open_output_or_die(outf);
    MackeReader  mackeReader(inf);
    int          numofseq = 0;

    while (1) {
        Macke macke;
        if (!mackeReader.mackeIn(macke)) break;

        SeqPtr seq = mackeReader.read_sequence_data();
        if (seq.isNull()) break;

        Embl  embl;
        numofseq++;

        /* partial_mtoe() is particularly handling
         * subspecies information, not converting whole
         * macke format to embl */

        if (!mtoe(macke, embl, *seq)) throw_conversion_failure(MACKE, EMBL);
        embl_out(embl, *seq, ofp);
    }

    log_processed(numofseq);
    fclose(ofp);
}

/* --------------------------------------------------------------
 *      Function partial_mtoe().
 *              Handle subspecies information when converting from
 *                      Macke to EMBL.
 */
int partial_mtoe(const Macke& macke, Embl& embl) {
    int indj, indk;

    if (has_content(macke.strain)) {
        if ((indj = find_pattern(embl.comments.others, "*source:")) >= 0 && (indk = find_pattern(embl.comments.others + indj, "strain=")) >= 0) {
            ;                   // do nothing
        }
        else {
            if (str0len(embl.comments.others) <= 1)
                freenull(embl.comments.others);
            Append(embl.comments.others, "*source: strain=");
            Append(embl.comments.others, macke.strain);
            if (!is_end_mark(embl.comments.others[str0len(embl.comments.others) - 2])) {
                skip_eolnl_and_append(embl.comments.others, ";\n");
            }
        }
    }

    if (has_content(macke.subspecies)) {
        if ((indj = find_pattern(embl.comments.others, "*source:")) >= 0 &&
            ((indk = find_pattern(embl.comments.others + indj, "subspecies=")) >= 0 ||
             (indk = find_pattern(embl.comments.others + indj, "sub-species=")) >= 0 ||
             (indk = find_pattern(embl.comments.others + indj, "subsp.=")) >= 0)) {
            ;                   // do nothing 
        }
        else {
            if (str0len(embl.comments.others) <= 1)
                freenull(embl.comments.others);

            Append(embl.comments.others, "*source: subspecies=");
            Append(embl.comments.others, macke.subspecies);

            if (!is_end_mark(embl.comments.others[str0len(embl.comments.others) - 2])) {
                skip_eolnl_and_append(embl.comments.others, ";\n");
            }
        }
    }

    return (1);
}
