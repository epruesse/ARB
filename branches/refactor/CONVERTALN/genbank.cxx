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

static int genbank_comment_subkey_word(const char *line, int index, char *key, int length) {
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

static int genbank_check_blanks(const char *line, int numb) {
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

static void genbank_continue_line(char*& Str, int numb, Reader& reader) {
    // if following line(s) are continued line(s), append them to 'Str'.
    // if 'Str' is NULL, lines only get skipped.
    // 'numb' = number of blanks needed at BOL to defined continued lines

    // check continue lines
    for (++reader;
         reader.line() && (genbank_check_blanks(reader.line(), numb) || reader.line()[0] == '\n');
         ++reader)
    {
        if (reader.line()[0] != '\n') { // empty line is allowed
            if (Str) {
                // remove end-of-line, if there is any
                int  ind = Skip_white_space(reader.line(), 0);
                char temp[LINESIZE];
                strcpy(temp, (reader.line() + ind));
                skip_eolnl_and_append_spaced(Str, temp);
            }
        }
    }
}

void genbank_one_entry_in(char*& datastring, Reader& reader) { 
    freedup(datastring, reader.line()+Skip_white_space(reader.line(), GBINDENT));
    return genbank_continue_line(datastring, GBINDENT, reader);
}

static void genbank_one_comment_entry(char*& datastring, int start_index, Reader& reader) {
    // Read in one genbank sub-entry in comments lines.
    freedup(datastring, reader.line() + Skip_white_space(reader.line(), start_index));
    genbank_continue_line(datastring, 20, reader);
}

void genbank_source(GenBank& gbk, Reader& reader) {
    // Read in genbank SOURCE lines and also ORGANISM lines.
    genbank_one_entry_in(gbk.source, reader);
    char  key[TOKENSIZE];
    genbank_key_word(reader.line(), 2, key, TOKENSIZE);
    if (str_equal(key, "ORGANISM")) {
        int indent = Skip_white_space(reader.line(), GBINDENT);
        freedup(gbk.organism, reader.line() + indent);

        char *skip_em = NULL;
        genbank_continue_line(skip_em, GBINDENT, reader);
    }
}

class startsWithBlanks : Noncopyable {
    int blanks;

public:
    startsWithBlanks(int blanks_) : blanks(blanks_) {}
    bool operator()(const char *line) const { return genbank_check_blanks(line, blanks); }
};


void genbank_skip_unidentified(Reader& reader, int blank_num) {
    // Skip the lines of unidentified keyword.
    ++reader;
    reader.skipOverLinesThat(startsWithBlanks(blank_num));
}

void genbank_reference(GenBank& gbk, Reader& reader) {
    // Read in genbank REFERENCE lines.
    int  refnum;
    ASSERT_RESULT(int, 1, sscanf(reader.line() + GBINDENT, "%d", &refnum));
    if (refnum <= gbk.get_refcount()) {
        warningf(17, "Might redefine reference %d", refnum);
        genbank_skip_unidentified(reader, GBINDENT);
    }
    else {
        gbk.resize_refs(refnum);
        genbank_one_entry_in(gbk.get_latest_ref().ref, reader);
    }

    GenbankRef& ref = gbk.get_latest_ref();
    
    for (; reader.line() && reader.line()[0] == ' ' && reader.line()[1] == ' ';) {
        char key[TOKENSIZE];
        genbank_key_word(reader.line(), 2, key, TOKENSIZE);
        if (str_equal(key, "AUTHORS")) {
            if (has_content(ref.author)) warningf(10, "AUTHORS of REFERENCE %d is redefined", refnum);
            genbank_one_entry_in(ref.author, reader);
            terminate_with(ref.author, '.'); // add '.' if missing at the end
        }
        else if (str_equal(key, "TITLE")) {
            if (has_content(ref.title)) warningf(11, "TITLE of REFERENCE %d is redefined", refnum);
            genbank_one_entry_in(ref.title, reader);
        }
        else if (str_equal(key, "JOURNAL")) {
            if (has_content(ref.journal)) warningf(12, "JOURNAL of REFERENCE %d is redefined", refnum);
            genbank_one_entry_in(ref.journal, reader);
        }
        else if (str_equal(key, "STANDARD")) {
            if (has_content(ref.standard)) warningf(13, "STANDARD of REFERENCE %d is redefined", refnum);
            genbank_one_entry_in(ref.standard, reader);
        }
        else {
            warningf(18, "Unidentified REFERENCE subkeyword: %s#", key);
            genbank_skip_unidentified(reader, GBINDENT);
        }
    }
}

static void genbank_comments(GenBank& gbk, Reader& reader) {
    // Read in genbank COMMENTS lines.
    char key[TOKENSIZE];

    if (str0len(reader.line()) <= GBINDENT) {
        ++reader;
        if (!reader.line()) return;
    }
    // make up data to match the logic reasoning for next statement
    for (int indi = 0; indi < GBINDENT; reader.line_WRITABLE()[indi++] = ' ') {}


    for (; reader.line() && (genbank_check_blanks(reader.line(), GBINDENT) || reader.line()[0] == '\n');) {
        if (reader.line()[0] == '\n') {  // skip empty line
            ++reader;
            continue;
        }

        int index = Skip_white_space(reader.line(), GBINDENT);
        ca_assert(index<TOKENSIZE); // buffer overflow ?
        
        index = genbank_comment_subkey_word(reader.line(), index, key, TOKENSIZE);
        ca_assert(index<TOKENSIZE); // buffer overflow ?

        RDP_comment_parser one_comment_entry = genbank_one_comment_entry;
        RDP_comments&      comments          = gbk.comments;

        if (!parse_RDP_comment(comments, one_comment_entry, key, index, reader)) {
            // other comments
            Append(comments.others, reader.line() + GBINDENT);
            ++reader;
        }
    }
}

static void genbank_verify_accession(GenBank& gbk) {
    // Verify accession information.
    int indi, len, count, remainder;

    if (str_equal(gbk.accession, "No information\n"))
        return;
    len = str0len(gbk.accession);
    if ((len % 7) != 0) {
        if (warning_out) fprintf(stderr, "\nACCESSION: %s", gbk.accession);
        warning(136, "Each accession number should be a six-character identifier.");
    }
    for (indi = count = 0; indi < len - 1; indi++) {
        remainder = indi % 7;
        switch (remainder) {
            case 0:
                count++;
                if (count > 9) {
                    if (warning_out) fprintf(stderr, "\nACCESSION: %s", gbk.accession);
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
                    if (warning_out) fprintf(stderr, "\nACCESSION: %s", gbk.accession);
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
void genbank_in(GenBank& gbk, Seq& seq, Reader& reader) {
    // Read in one genbank entry.
    char       key[TOKENSIZE];
    EntryState state = ENTRY_NONE;

    for (; reader.line() && state != ENTRY_COMPLETED; ) {
        if (!has_content(reader.line())) { ++reader; continue; } // empty line, skip

        genbank_key_word(reader.line(), 0, key, TOKENSIZE);
        state = ENTRY_STARTED;

        if (str_equal(key, "LOCUS")) {
            genbank_one_entry_in(gbk.locus, reader);
            if (!gbk.locus_contains_date())
                warning(14, "LOCUS data might be incomplete");
        }
        else if (str_equal(key, "DEFINITION")) {
            genbank_one_entry_in(gbk.definition, reader);

            // correct missing '.' at the end
            terminate_with(gbk.definition, '.');
        }
        else if (str_equal(key, "ACCESSION")) {
            genbank_one_entry_in(gbk.accession, reader);
            genbank_verify_accession(gbk);
        }
        else if (str_equal(key, "KEYWORDS")) {
            genbank_one_entry_in(gbk.keywords, reader);
            genbank_verify_keywords(gbk);
        }
        else if (str_equal(key, "SOURCE")) {
            genbank_source(gbk, reader);
            // correct missing '.' at the end
            terminate_with(gbk.source, '.');
            terminate_with(gbk.organism, '.');
        }
        else if (str_equal(key, "REFERENCE")) {
            genbank_reference(gbk, reader);
        }
        else if (str_equal(key, "COMMENTS")) {
            genbank_comments(gbk, reader);
        }
        else if (str_equal(key, "COMMENT")) {
            genbank_comments(gbk, reader);
        }
        else if (str_equal(key, "ORIGIN")) {
            genbank_origin(seq, reader);
            state = ENTRY_COMPLETED;
        }
        else {                  // unidentified key word
            genbank_skip_unidentified(reader, 2);
        }
        /* except "ORIGIN", at the end of all the other cases,
         * a new line has already read in, so no further read
         * is necessary */
    }

    if (state == ENTRY_STARTED) throw_incomplete_entry();

    ++reader;
}

void genbank_origin(Seq& seq, Reader& reader) {
    // Read in genbank sequence data.
    ca_assert(seq.is_empty());

    // read in whole sequence data
    for (++reader; reader.line() && !is_sequence_terminator(reader.line()); ++reader) {
        if (has_content(reader.line())) {
            for (int index = 9; reader.line()[index] != '\n' && reader.line()[index] != '\0'; index++) {
                if (reader.line()[index] != ' ')
                    seq.add(reader.line()[index]);
            }
        }
    }
}

void genbank_in_simple(GenBank& gbk, Seq& seq, Reader& reader) {
    // Read in next genbank locus and sequence only.
    // For use of converting to simple format(read in only simple
    // information instead of whole records).
    char        key[TOKENSIZE];
    EntryState  state = ENTRY_NONE;

    for (; reader.line() && state != ENTRY_COMPLETED;) {
        if (!has_content(reader.line())) { ++reader; continue; } // empty line, skip @@@ enable later
        genbank_key_word(reader.line(), 0, key, TOKENSIZE);
        state = ENTRY_STARTED;
        if (str_equal(key, "ORIGIN")) {
            genbank_origin(seq, reader);
            state = ENTRY_COMPLETED;
        }
        else if (str_equal(key, "LOCUS")) {
            genbank_one_entry_in(gbk.locus, reader);
        }
        else {
            ++reader;
        }
    }

    if (state == ENTRY_STARTED) throw_incomplete_entry();
    ++reader;
}

static void genbank_print_lines(Writer& write, const char *key, const char *content, const WrapMode& wrapMode) { // @@@ WRAPPER
    // Print one genbank line, wrap around if over column GBMAXLINE

    ca_assert(strlen(key) == GBINDENT);
    ca_assert(content[strlen(content)-1] == '\n');

    wrapMode.print(write, key, "            ", content, GBMAXLINE);
}

static void genbank_out_one_entry(Writer& write, const char *key, const char *content, const WrapMode& wrapMode, int period) {
    /* Print out key and content if content length > 1
     * otherwise print key and "No information" w/wo
     * period at the end depending on flag period.
     */

    if (!has_content(content)) {
        content = period ? "No information.\n" : "No information\n";
    }
    genbank_print_lines(write, key, content, wrapMode);
}

static void genbank_out_one_reference(Writer& write, const GenbankRef& gbk_ref, int gbk_ref_num, bool SIMULATE_BUG) {
    WrapMode wrapWords(true);

    {
        const char *r = gbk_ref.ref;
        char        refnum[TOKENSIZE];

        if (!has_content(r)) {
            sprintf(refnum, "%d\n", gbk_ref_num);
            r = refnum;
        }
        genbank_out_one_entry(write, "REFERENCE   ", r, wrapWords, NOPERIOD);
    }

    genbank_out_one_entry(write, "  AUTHORS   ", gbk_ref.author, WrapMode(" "), NOPERIOD);

    if (SIMULATE_BUG) { // @@@
        // skip title completely, if there is no information
        if (has_content(gbk_ref.title)) {
            genbank_out_one_entry(write, "  TITLE     ", gbk_ref.title, wrapWords, NOPERIOD);
        }
        genbank_out_one_entry(write, "  JOURNAL   ", gbk_ref.journal, wrapWords, NOPERIOD);
    }
    else {
        // print title also, if there is no information (like other fields here)
        genbank_out_one_entry(write, "  JOURNAL   ", gbk_ref.journal, wrapWords, NOPERIOD);
        genbank_out_one_entry(write, "  TITLE     ", gbk_ref.title, wrapWords, NOPERIOD);
    }

    genbank_out_one_entry(write, "  STANDARD  ", gbk_ref.standard, wrapWords, NOPERIOD);
}

static void genbank_print_comment_if_content(Writer& write, const char *key, const char *content) { // @@@ WRAPPER
    // Print one genbank line, wrap around if over column GBMAXLINE

    if (!has_content(content)) return;

    char first[LINESIZE]; sprintf(first, "%*s%s", GBINDENT+RDP_SUBKEY_INDENT, "", key);
    char other[LINESIZE]; sprintf(other, "%*s", GBINDENT+RDP_SUBKEY_INDENT+RDP_CONTINUED_INDENT, "");
    WrapMode(true).print(write, first, other, content, GBMAXLINE);
}

static void genbank_out_origin(const Seq& seq, Writer& write) {
    // Output sequence data in genbank format.
    int indi, indj, indk;

    write.out("ORIGIN\n");

    const char *sequence = seq.get_seq();
    for (indi = 0, indj = 0, indk = 1; indi < seq.get_len(); indi++) {
        if ((indk % 60) == 1)
            write.outf("   %6d ", indk);
        write.out(sequence[indi]);
        indj++;

        // blank space follows every 10 bases, but not before '\n'
        if ((indk % 60) == 0) {
            write.out('\n');
            indj = 0;
        }
        else if (indj == 10 && indi != (seq.get_len() - 1)) {
            write.out(' ');
            indj = 0;
        }
        indk++;
    }

    if ((indk % 60) != 1)
        write.out('\n');
    write.out("//\n");
}

void genbank_out(const GenBank& gbk, const Seq& seq, Writer& write) {
    // Output in a genbank format

    int      indi;
    WrapMode wrapWords(true);

    genbank_out_one_entry(write, "LOCUS       ", gbk.locus,      wrapWords,     NOPERIOD);
    genbank_out_one_entry(write, "DEFINITION  ", gbk.definition, wrapWords,     PERIOD);
    genbank_out_one_entry(write, "ACCESSION   ", gbk.accession,  wrapWords,     NOPERIOD);
    genbank_out_one_entry(write, "KEYWORDS    ", gbk.keywords,   WrapMode(";"), PERIOD);
    genbank_out_one_entry(write, "SOURCE      ", gbk.source,     wrapWords,     PERIOD);
    genbank_out_one_entry(write, "  ORGANISM  ", gbk.organism,   wrapWords,     PERIOD);

    if (gbk.has_refs()) {
        for (indi = 0; indi < gbk.get_refcount(); indi++) {
            genbank_out_one_reference(write, gbk.get_ref(indi), indi+1, true);
        }
    }
    else {
        genbank_out_one_reference(write, GenbankRef(), 1, false);
    }

    const OrgInfo& orginf = gbk.comments.orginf;
    const SeqInfo& seqinf = gbk.comments.seqinf;

    if (orginf.exists ||
        seqinf.exists ||
        has_content(gbk.comments.others))
    {
        // @@@ code below is broken
        // see ../UNIT_TESTER/run/impexp/conv.EMBL_2_GENBANK.expected@Next
        write.out("COMMENTS    ");

        if (orginf.exists) {
            write.out("Organism information\n");

            genbank_print_comment_if_content(write, "Source of strain: ",   orginf.source);
            genbank_print_comment_if_content(write, "Culture collection: ", orginf.cultcoll); // this field is used in ../lib/import/.rdp_old.ift
            genbank_print_comment_if_content(write, "Former name: ",        orginf.formname); // other fields occur in no .ift
            genbank_print_comment_if_content(write, "Alternate name: ",     orginf.nickname);
            genbank_print_comment_if_content(write, "Common name: ",        orginf.commname);
            genbank_print_comment_if_content(write, "Host organism: ",      orginf.hostorg);

            if (seqinf.exists || str0len(gbk.comments.others) > 0)
                write.out("            ");
        }

        if (seqinf.exists) {
            write.outf("Sequence information (bases 1 to %d)\n", seq.get_len());

            genbank_print_comment_if_content(write, "RDP ID: ",                      seqinf.RDPid);
            genbank_print_comment_if_content(write, "Corresponding GenBank entry: ", seqinf.gbkentry); // this field is used in ../lib/import/.rdp_old.ift
            genbank_print_comment_if_content(write, "Sequencing methods: ",          seqinf.methods);
        }

        // @@@ DRY (when bug was removed):
        if      (seqinf.comp5 == 'n') write.out("              5' end complete: No\n");
        else if (seqinf.comp5 == 'y') write.out("              5' end complete: Yes\n");

        if      (seqinf.comp3 == 'n') write.out("              3' end complete: No\n");
        else if (seqinf.comp3 == 'y') write.out("             3' end complete: Yes\n"); // @@@ now you see the 'bug'

        // @@@ use wrapper for code below ?
        // print GBINDENT spaces of the first line
        if (str0len(gbk.comments.others) > 0) {
            write.repeated(' ', GBINDENT);
        }

        if (str0len(gbk.comments.others) > 0) {
            int length = str0len(gbk.comments.others);
            for (indi = 0; indi < length; indi++) {
                write.out(gbk.comments.others[indi]);

                // if another line, print GBINDENT spaces first
                if (gbk.comments.others[indi] == '\n' && gbk.comments.others[indi + 1] != '\0') {
                    write.repeated(' ', GBINDENT);
                }
            }
        }
    }

    {
        BaseCounts bases;
        seq.count(bases);
        write.outf("BASE COUNT  %6d a %6d c %6d g %6d t", bases.a, bases.c, bases.g, bases.t);
        if (bases.other) { // don't write 0 others
            write.outf(" %6d others", bases.other);
        }
        write.out('\n');
    }
    genbank_out_origin(seq, write);
}

void genbank_to_genbank(const char *inf, const char *outf) {
    // Convert from genbank to genbank.
    Reader reader(inf);
    Writer write(outf);

    while (1) {
        GenBank gbk;
        Seq     seq;
        genbank_in(gbk, seq, reader);
        if (reader.failed()) break;

        genbank_out(gbk, seq, write);
        write.seq_done();
    }
}

