// -------------- genbank related subroutines -----------------

#include "genbank.h"
#include "wrap.h"

#define NOPERIOD    0
#define PERIOD      1

extern int warning_out;

void genbank_key_word(const char *line, int index, char *key, int length) { // @@@ similar to embl_key_word and macke_key_word
    // Get the key_word from line beginning at index.
    int indi, indj;

    if (line == NULL) {
        key[0] = '\0';
        return;
    }

    for (indi = index, indj = 0; (index - indi) < length && line[indi] != ' ' && line[indi] != '\t' && line[indi] != '\n' && line[indi] != '\0' && indi < GBINDENT; indi++, indj++)
        key[indj] = line[indi];
    key[indj] = '\0';
}

static int genbank_comment_subkey_word(char *line, int index, char *key, int length) {
    // Get the subkey_word in comment lines beginning at index.
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

static int genbank_check_blanks(char *line, int numb) {
    // Check if there is (numb) of blanks at beginning of line.
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

static char *genbank_continue_line(char*& Str, char *line, int numb, FILE_BUFFER fp) {
    // if following line(s) are continued line(s), append them to 'Str'.
    // if 'Str' is NULL, lines only get skipped.
    // 'numb' = number of blanks needed at BOL to defined continued lines

    char *eof;

    // check continue lines
    for (eof = Fgetline(line, LINESIZE, fp);
         eof != NULL && (genbank_check_blanks(line, numb) || line[0] == '\n');
         eof = Fgetline(line, LINESIZE, fp))
    {
        if (line[0] != '\n') { // empty line is allowed
            if (Str) {
                // remove end-of-line, if there is any
                int  ind = Skip_white_space(line, 0);
                char temp[LINESIZE];
                strcpy(temp, (line + ind));
                skip_eolnl_and_append_spaced(Str, temp);
            }
        }
    }

    return eof;
}

static char *genbank_one_entry_in(char*& datastring, char *line, FILE_BUFFER fp) {
    // Read in genbank one entry lines.
    freedup(datastring, line+Skip_white_space(line, GBINDENT));
    return genbank_continue_line(datastring, line, GBINDENT, fp);
}

static char *genbank_one_comment_entry(char*& datastring, char *line, int start_index, FILE_BUFFER fp) {
    // Read in one genbank sub-entry in comments lines.
    freedup(datastring, line + Skip_white_space(line, start_index));
    return genbank_continue_line(datastring, line, 20, fp);
}

static char *genbank_source(GenBank& gbk, char *line, FILE_BUFFER fp) {
    // Read in genbank SOURCE lines and also ORGANISM lines.
    char *eof = genbank_one_entry_in(gbk.source, line, fp);
    char  key[TOKENSIZE];
    genbank_key_word(line, 2, key, TOKENSIZE);
    if (str_equal(key, "ORGANISM")) {
        int indent = Skip_white_space(line, GBINDENT);
        freedup(gbk.organism, line + indent);

        char *skip_em = NULL;
        eof           = genbank_continue_line(skip_em, line, GBINDENT, fp);
    }
    return (eof);
}
static char *genbank_skip_unidentified(char *line, FILE_BUFFER fp, int blank_num) {
    // Skip the lines of unidentified keyword.
    char *eof;
    for (eof = Fgetline(line, LINESIZE, fp); eof != NULL && genbank_check_blanks(line, blank_num); eof = Fgetline(line, LINESIZE, fp)) {}
    return (eof);
}

static char *genbank_reference(GenBank& gbk, char *line, FILE_BUFFER fp) {
    // Read in genbank REFERENCE lines.
    char *eof, key[TOKENSIZE];
    int   refnum;
    int   acount = 0, tcount = 0, jcount = 0, scount = 0;

    ASSERT_RESULT(int, 1, sscanf(line + GBINDENT, "%d", &refnum));
    if (refnum <= gbk.get_refcount()) {
        warningf(17, "Might redefine reference %d", refnum);
        eof = genbank_skip_unidentified(line, fp, GBINDENT);
    }
    else {
        gbk.resize_refs(refnum);
        // initialize the buffer
        eof = genbank_one_entry_in(gbk.get_ref(refnum - 1).ref, line, fp);
    }
    // find the reference listings
    for (; eof != NULL && line[0] == ' ' && line[1] == ' ';) {
        // find the key word
        genbank_key_word(line, 2, key, TOKENSIZE);
        // skip white space
        if (str_equal(key, "AUTHORS")) {
            eof = genbank_one_entry_in(gbk.get_ref(refnum - 1).author, line, fp);

            // add '.' if missing at the end
            terminate_with(gbk.get_ref(refnum - 1).author, '.');

            if (acount == 0) acount = 1;
            else {
                warningf(10, "AUTHORS of REFERENCE %d is redefined", refnum);
            }
        }
        else if (str_equal(key, "TITLE")) {
            eof = genbank_one_entry_in(gbk.get_ref(refnum - 1).title, line, fp);
            if (tcount == 0) tcount = 1;
            else {
                warningf(11, "TITLE of REFERENCE %d is redefined", refnum);
            }
        }
        else if (str_equal(key, "JOURNAL")) {
            eof = genbank_one_entry_in(gbk.get_ref(refnum - 1).journal, line, fp);

            if (jcount == 0) jcount = 1;
            else {
                warningf(12, "JOURNAL of REFERENCE %d is redefined", refnum);
            }
        }
        else if (str_equal(key, "STANDARD")) {
            eof = genbank_one_entry_in(gbk.get_ref(refnum - 1).standard, line, fp);

            if (scount == 0) scount = 1;
            else {
                warningf(13, "STANDARD of REFERENCE %d is redefined", refnum);
            }
        }
        else {
            warningf(18, "Unidentified REFERENCE subkeyword: %s#", key);
            eof = genbank_skip_unidentified(line, fp, GBINDENT);
        }
    }
    return (eof);
}

static const char *genbank_comments(GenBank& gbk, char *line, FILE_BUFFER fp) {
    // Read in genbank COMMENTS lines.
    int         index, indi, ptr;
    const char *eof;
    char        key[TOKENSIZE];

    if (str0len(line) <= GBINDENT) {
        if ((eof = Fgetline(line, LINESIZE, fp)) == NULL)
            return (eof);
    }
    // make up data to match the logic reasoning for next statement
    for (indi = 0; indi < GBINDENT; line[indi++] = ' ') {}
    eof = "NONNULL";

    OrgInfo& orginf = gbk.comments.orginf;
    SeqInfo& seqinf = gbk.comments.seqinf;

    for (; eof != NULL && (genbank_check_blanks(line, GBINDENT) || line[0] == '\n');) {
        if (line[0] == '\n') {  // skip empty line
            eof = Fgetline(line, LINESIZE, fp);
            continue;
        }

        ptr = index = GBINDENT;

        index = Skip_white_space(line, index);
#if defined(DEBUG)
        if (index >= TOKENSIZE) {
            printf("big index %i after Skip_white_space\n", index);
        }
#endif // DEBUG
        index = genbank_comment_subkey_word(line, index, key, TOKENSIZE);
#if defined(DEBUG)
        if (index >= TOKENSIZE) {
            printf("big index %i after genbank_comment_subkey_word\n", index);
        }
#endif // DEBUG

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
            ca_assert(ptr == GBINDENT);
            if (gbk.comments.others == NULL) {
                gbk.comments.others = nulldup(line + ptr);
            }
            else {
                Append(gbk.comments.others, line + ptr);
            }

            eof = Fgetline(line, LINESIZE, fp);
        }
    }

    return (eof);
}
static void genbank_verify_accession(GenBank& gbk) {
    // Verify accession information.
    int indi, len, count, remainder;

    if (str_equal(gbk.accession, "No information\n"))
        return;
    len = str0len(gbk.accession);
    if ((len % 7) != 0) {
        if (warning_out)
            fprintf(stderr, "\nACCESSION: %s", gbk.accession);
        warning(136, "Each accession number should be a six-character identifier.");
    }
    for (indi = count = 0; indi < len - 1; indi++) {
        remainder = indi % 7;
        switch (remainder) {
            case 0:
                count++;
                if (count > 9) {
                    if (warning_out)
                        fprintf(stderr, "\nACCESSION: %s", gbk.accession);
                    warning(137, "No more than 9 accession numbers are allowed in ACCESSION line.");
                    gbk.accession[indi - 1] = '\n';
                    gbk.accession[indi] = '\0';
                    gbk.accession = Reallocspace(gbk.accession, (unsigned)(sizeof(char) * indi));
                    return;
                }
                if (!isalpha(gbk.accession[indi])) {
                    warningf(138, "The %d(th) accession number must start with a letter.", count);
                }
                break;
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
                if (!isdigit(gbk.accession[indi])) {
                    warningf(140, "The last 5 characters of the %d(th) accession number should be all digits.", count);
                }
                break;
            case 6:
                if ((indi != (len - 1) && gbk.accession[indi] != ' ')
                    || (indi == (len - 1) && gbk.accession[indi] != '\n')) {
                    if (warning_out)
                        fprintf(stderr, "\nACCESSION: %s", gbk.accession);
                    warning(139, "Accession numbers should be separated by a space.");
                    gbk.accession[indi] = ' ';
                }
                break;
            default:;
        }
    }
}
static void genbank_verify_keywords(GenBank& gbk) {
    // Verify keywords.
    int indi, count, len;

    // correct missing '.' at the end
    terminate_with(gbk.keywords, '.');

    for (indi = count = 0, len = str0len(gbk.keywords); indi < len; indi++)
        if (gbk.keywords[indi] == '.')
            count++;

    if (count != 1) {
        if (warning_out)
            fprintf(stderr, "\nKEYWORDS: %s", gbk.keywords);
        warning(141, "No more than one period is allowed in KEYWORDS line.");
    }
}
char genbank_in(GenBank& gbk, Seq& seq, FILE_BUFFER fp) {
    // Read in one genbank entry.
    char        line[LINESIZE], key[TOKENSIZE];
    const char *eof;
    char        eoen;

    eoen = ' ';
    // end-of-entry, set to be 'y' after '//' is read
    for (eof = Fgetline(line, LINESIZE, fp); eof != NULL && eoen != 'y';) {
        if (!has_content(line)) {
            eof = Fgetline(line, LINESIZE, fp);
            continue;           // empty line, skip
        }

        genbank_key_word(line, 0, key, TOKENSIZE);

        eoen = 'n';

        if (str_equal(key, "LOCUS")) {
            eof = genbank_one_entry_in(gbk.locus, line, fp);
            if (!gbk.locus_contains_date())
                warning(14, "LOCUS data might be incomplete");
        }
        else if (str_equal(key, "DEFINITION")) {
            eof = genbank_one_entry_in(gbk.definition, line, fp);

            // correct missing '.' at the end
            terminate_with(gbk.definition, '.');
        }
        else if (str_equal(key, "ACCESSION")) {
            eof = genbank_one_entry_in(gbk.accession, line, fp);
            genbank_verify_accession(gbk);
        }
        else if (str_equal(key, "KEYWORDS")) {
            eof = genbank_one_entry_in(gbk.keywords, line, fp);
            genbank_verify_keywords(gbk);
        }
        else if (str_equal(key, "SOURCE")) {
            eof = genbank_source(gbk, line, fp);
            // correct missing '.' at the end
            terminate_with(gbk.source, '.');
            terminate_with(gbk.organism, '.');
        }
        else if (str_equal(key, "REFERENCE")) {
            eof = genbank_reference(gbk, line, fp);
        }
        else if (str_equal(key, "COMMENTS")) {
            eof = genbank_comments(gbk, line, fp);
        }
        else if (str_equal(key, "COMMENT")) {
            eof = genbank_comments(gbk, line, fp);
        }
        else if (str_equal(key, "ORIGIN")) {
            eof = genbank_origin(seq, line, fp);
            eoen = 'y';
        }
        else {                  // unidentified key word
            eof = genbank_skip_unidentified(line, fp, 2);
        }
        /* except "ORIGIN", at the end of all the other cases,
         * a new line has already read in, so no further read
         * is necessary */
    }

    if (eoen == 'n') {
        warning(86, "Reach EOF before sequence data is read.");
        return (EOF);
    }
    if (eof == NULL)
        return (EOF);
    else
        return (EOF + 1);
}

char *genbank_origin(Seq& seq, char *line, FILE_BUFFER fp) {
    // Read in genbank sequence data.
    char *eof;
    int   index;

    ca_assert(seq.is_empty());

    // read in whole sequence data
    for (eof = Fgetline(line, LINESIZE, fp); eof != NULL && line[0] != '/' && line[1] != '/'; eof = Fgetline(line, LINESIZE, fp)) {
        if (has_content(line)) {
            for (index = 9; line[index] != '\n' && line[index] != '\0'; index++) {
                if (line[index] != ' ')
                    seq.add(line[index]);
            }
        }
    }

    return eof;
}

char genbank_in_locus(GenBank& gbk, Seq& seq, FILE_BUFFER fp) {
    // Read in next genbank locus and sequence only.
    // For use of converting to simple format(read in only simple
    // information instead of whole records).
    char  line[LINESIZE], key[TOKENSIZE];
    char *eof, eoen;

    eoen = ' ';                 // end-of-entry, set to be 'y' after '//' is read
    for (eof = Fgetline(line, LINESIZE, fp); eof != NULL && eoen != 'y';) {
        genbank_key_word(line, 0, key, TOKENSIZE);
        if (str_equal(key, "ORIGIN")) {
            eof = genbank_origin(seq, line, fp);
            eoen = 'y';
        }
        else if (str_equal(key, "LOCUS")) {
            eof = genbank_one_entry_in(gbk.locus, line, fp);
        }
        else
            eof = Fgetline(line, LINESIZE, fp);
    }

    if (eoen == 'n')
        throw_error(9, "Reach EOF before one entry is read");

    if (eof == NULL)
        return (EOF);
    else
        return (EOF + 1);
}

static void genbank_print_lines(FILE * fp, const char *key, const char *content, const WrapMode& wrapMode) { // @@@ WRAPPER
    // Print one genbank line, wrap around if over column GBMAXLINE

    ca_assert(strlen(key) == GBINDENT);
    ca_assert(content[strlen(content)-1] == '\n');

    wrapMode.print(fp, key, "            ", content, GBMAXLINE, WRAP_CORRECTLY);
}

static void genbank_out_one_entry(FILE * fp, const char *key, const char *content, const WrapMode& wrapMode, int period) {
    /* Print out key and content if content length > 1
     * otherwise print key and "No information" w/wo
     * period at the end depending on flag period.
     */

    if (!has_content(content)) {
        content = period ? "No information.\n" : "No information\n";
    }
    genbank_print_lines(fp, key, content, wrapMode);
}

static void genbank_out_one_reference(FILE * fp, const GenbankRef& gbk_ref, int gbk_ref_num, bool SIMULATE_BUG) {
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

static void genbank_print_comment_if_content(FILE * fp, const char *key, const char *content) { // @@@ WRAPPER
    // Print one genbank line, wrap around if over column GBMAXLINE

    if (!has_content(content)) return;

    char first[LINESIZE]; sprintf(first, "%*s%s", GBINDENT+RDP_SUBKEY_INDENT, "", key);
    char other[LINESIZE]; sprintf(other, "%*s", GBINDENT+RDP_SUBKEY_INDENT+RDP_CONTINUED_INDENT, "");
    WrapMode(true).print(fp, first, other, content, GBMAXLINE, WRAP_CORRECTLY);
}

static void genbank_out_origin(const Seq& seq, FILE * fp) {
    // Output sequence data in genbank format.
    int indi, indj, indk;

    fputs("ORIGIN\n", fp);

    const char *sequence = seq.get_seq();
    for (indi = 0, indj = 0, indk = 1; indi < seq.get_len(); indi++) {
        if ((indk % 60) == 1)
            fprintf(fp, "   %6d ", indk);
        fputc(sequence[indi], fp);
        indj++;

        // blank space follows every 10 bases, but not before '\n'
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

    if ((indk % 60) != 1)
        fputc('\n', fp);
    fputs("//\n", fp);
}

void genbank_out(const GenBank& gbk, const Seq& seq, FILE * fp) {
    // Output in a genbank format

    int      indi;
    WrapMode wrapWords(true);

    genbank_out_one_entry(fp, "LOCUS       ", gbk.locus,      wrapWords,     NOPERIOD);
    genbank_out_one_entry(fp, "DEFINITION  ", gbk.definition, wrapWords,     PERIOD);
    genbank_out_one_entry(fp, "ACCESSION   ", gbk.accession,  wrapWords,     NOPERIOD);
    genbank_out_one_entry(fp, "KEYWORDS    ", gbk.keywords,   WrapMode(";"), PERIOD);
    genbank_out_one_entry(fp, "SOURCE      ", gbk.source,     wrapWords,     PERIOD);
    genbank_out_one_entry(fp, "  ORGANISM  ", gbk.organism,   wrapWords,     PERIOD);

    if (gbk.has_refs()) {
        for (indi = 0; indi < gbk.get_refcount(); indi++) {
            genbank_out_one_reference(fp, gbk.get_ref(indi), indi+1, true);
        }
    }
    else {
        genbank_out_one_reference(fp, GenbankRef(), 1, false);
    }

    const OrgInfo& orginf = gbk.comments.orginf;
    const SeqInfo& seqinf = gbk.comments.seqinf;

    if (orginf.exists ||
        seqinf.exists ||
        has_content(gbk.comments.others))
    {
        // @@@ code below is broken
        // see ../UNIT_TESTER/run/impexp/conv.EMBL_2_GENBANK.expected@Next
        fputs("COMMENTS    ", fp);

        if (orginf.exists) {
            fputs("Organism information\n", fp);

            genbank_print_comment_if_content(fp, "Source of strain: ",   orginf.source);
            genbank_print_comment_if_content(fp, "Culture collection: ", orginf.cultcoll); // this field is used in ../lib/import/.rdp_old.ift
            genbank_print_comment_if_content(fp, "Former name: ",        orginf.formname); // other fields occur in no .ift
            genbank_print_comment_if_content(fp, "Alternate name: ",     orginf.nickname);
            genbank_print_comment_if_content(fp, "Common name: ",        orginf.commname);
            genbank_print_comment_if_content(fp, "Host organism: ",      orginf.hostorg);

            if (seqinf.exists || str0len(gbk.comments.others) > 0)
                fputs("            ", fp);
        }

        if (seqinf.exists) {
            fprintf(fp, "Sequence information (bases 1 to %d)\n", seq.get_len());

            genbank_print_comment_if_content(fp, "RDP ID: ",                      seqinf.RDPid);
            genbank_print_comment_if_content(fp, "Corresponding GenBank entry: ", seqinf.gbkentry); // this field is used in ../lib/import/.rdp_old.ift
            genbank_print_comment_if_content(fp, "Sequencing methods: ",          seqinf.methods);
        }

        // @@@ DRY (when bug was removed):
        if      (seqinf.comp5 == 'n') fputs("              5' end complete: No\n", fp);
        else if (seqinf.comp5 == 'y') fputs("              5' end complete: Yes\n", fp);

        if      (seqinf.comp3 == 'n') fputs("              3' end complete: No\n", fp);
        else if (seqinf.comp3 == 'y') fputs("             3' end complete: Yes\n", fp); // @@@ now you see the 'bug'

        // @@@ use wrapper for code below ?
        // print GBINDENT spaces of the first line
        if (str0len(gbk.comments.others) > 0) {
            fputc_rep(' ', GBINDENT, fp);
        }

        if (str0len(gbk.comments.others) > 0) {
            int length = str0len(gbk.comments.others);
            for (indi = 0; indi < length; indi++) {
                fputc(gbk.comments.others[indi], fp);

                // if another line, print GBINDENT spaces first
                if (gbk.comments.others[indi] == '\n' && gbk.comments.others[indi + 1] != '\0') {
                    fputc_rep(' ', GBINDENT, fp);
                }
            }
        }
    }

    {
        BaseCounts bases;
        seq.count(bases);
        fprintf(fp, "BASE COUNT  %6d a %6d c %6d g %6d t", bases.a, bases.c, bases.g, bases.t);
        if (bases.other) { // don't write 0 others
            fprintf(fp, " %6d others", bases.other);
        }
        fputc('\n', fp);
    }
    genbank_out_origin(seq, fp);
}

void genbank_to_genbank(const char *inf, const char *outf) {
    // Convert from genbank to genbank.
    FILE        *IFP      = open_input_or_die(inf);
    FILE_BUFFER  ifp      = create_FILE_BUFFER(inf, IFP);
    FILE        *ofp      = open_output_or_die(outf);
    int          numofseq = 0;

    while (1) {
        GenBank gbk;
        Seq     seq;
        if (genbank_in(gbk, seq, ifp) == EOF) break;

        numofseq++;
        genbank_out(gbk, seq, ofp);
    }

    log_processed(numofseq);
    destroy_FILE_BUFFER(ifp);
    fclose(ofp);
}

