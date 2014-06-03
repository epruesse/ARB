// -------------- genbank related subroutines -----------------

#include "genbank.h"
#include "wrap.h"

#define NOPERIOD    0
#define PERIOD      1

void genbank_key_word(const char *line, int index, char *key) {
    ca_assert((GBINDENT-index) >= 0);
    int len = parse_key_word(line+index, key, " \t\n");
    if ((index+len) >= GBINDENT) {
        key[GBINDENT-index] = 0;
    }
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

static void genbank_one_entry_in(char*& datastring, Reader& reader) {
    freedup(datastring, reader.line()+Skip_white_space(reader.line(), GBINDENT));
    return genbank_continue_line(datastring, GBINDENT, reader);
}

static void genbank_one_comment_entry(char*& datastring, int start_index, Reader& reader) {
    // Read in one genbank sub-entry in comments lines.
    freedup(datastring, reader.line() + Skip_white_space(reader.line(), start_index));
    genbank_continue_line(datastring, 20, reader);
}

static void genbank_source(GenBank& gbk, Reader& reader) {
    // Read in genbank SOURCE lines and also ORGANISM lines.
    genbank_one_entry_in(gbk.source, reader);
    char  key[TOKENSIZE];
    genbank_key_word(reader.line(), 2, key);
    if (str_equal(key, "ORGANISM")) {
        int indent = Skip_white_space(reader.line(), GBINDENT);
        freedup(gbk.organism, reader.line() + indent);

        char *skip_em = NULL;
        genbank_continue_line(skip_em, GBINDENT, reader);
    }
}

class startsWithBlanks : virtual Noncopyable {
    int blanks;

public:
    startsWithBlanks(int blanks_) : blanks(blanks_) {}
    bool operator()(const char *line) const { return genbank_check_blanks(line, blanks); }
};


static void genbank_skip_unidentified(Reader& reader, int blank_num) {
    // Skip the lines of unidentified keyword.
    ++reader;
    startsWithBlanks num_blanks(blank_num);
    reader.skipOverLinesThat(num_blanks);
}

static void genbank_reference(GenBank& gbk, Reader& reader) {
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
        genbank_key_word(reader.line(), 2, key);
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

    // replace keyword with spaces
    // => identical format for 1st and following lines.
    {
        char *line = strdup(reader.line());
        for (int indi = 0; indi < GBINDENT; line[indi++] = ' ') {}
        reader.set_line(line);
        free(line);
    }


    for (; reader.line() && (genbank_check_blanks(reader.line(), GBINDENT) || reader.line()[0] == '\n');) {
        if (reader.line()[0] == '\n') {  // skip empty line
            ++reader;
            continue;
        }

        int index = Skip_white_space(reader.line(), GBINDENT);
        ca_assert(index<TOKENSIZE); // buffer overflow ?

        index += comment_subkey(reader.line()+index, key);
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

inline bool valid_acc_char(char ch) { return isalnum(ch) || ch == '_'; }

static void genbank_verify_accession(GenBank& gbk) {
    // Verify accession information.
    if (str_equal(gbk.accession, "No information\n")) return; // @@@ really allow this ?

    char         *new_acc = NULL;
    const char   *sep     = " \t\n;";
    SmartCharPtr  req_fail;
    SmartCharPtr  copy    = strdup(gbk.accession);
    int           count   = 0;

    for (char *acc = strtok(&*copy, sep); acc && req_fail.isNull(); acc = strtok(NULL, sep)) {
        count++;
        if (!isalpha(acc[0])) req_fail = strdup("has to start with a letter");
        else {
            for (int i = 0; acc[i]; ++i) {
                if (!valid_acc_char(acc[i])) {
                    req_fail = strf("invalid char '%c'", acc[i]);
                    break;
                }
            }
        }

        if (new_acc) Append(new_acc, ' ');
        Append(new_acc, acc);
    }

    if (req_fail.isNull() && count>9) {
        req_fail = strf("No more than 9 accession number allowed (found %i)", count);
    }

    if (!req_fail.isNull()) {
        skip_eolnl_and_append(gbk.accession, "");
        throw_errorf(15, "Invalid accession number '%s' (%s)", gbk.accession, &*req_fail);
    }

    Append(new_acc, '\n');
    freeset(gbk.accession, new_acc);
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
        // @@@ raise error here ?
        if (Warnings::shown())
            fprintf(stderr, "\nKEYWORDS: %s", gbk.keywords);
        warning(141, "No more than one period is allowed in KEYWORDS line.");
    }
}
void GenbankParser::parse_section() {
    char key[TOKENSIZE];
    genbank_key_word(reader.line(), 0, key);
    state = ENTRY_STARTED;
    parse_keyed_section(key);
}

static void genbank_origin(Seq& seq, Reader& reader) {
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

void GenbankParser::parse_keyed_section(const char *key) {
    if (str_equal(key, "LOCUS")) {
        genbank_one_entry_in(gbk.locus, reader);
        if (!gbk.locus_contains_date()) {
            static bool alreadyWarned = false;
            if (!alreadyWarned) {
                warning(14, "LOCUS data might be incomplete (no date seen)");
                alreadyWarned = true;
            }
        }
    }
    else if (str_equal(key, "DEFINITION")) {
        genbank_one_entry_in(gbk.definition, reader);
        terminate_with(gbk.definition, '.'); // correct missing '.' at the end
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
        terminate_with(gbk.source, '.'); // correct missing '.' at the end
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
    else {
        genbank_skip_unidentified(reader, 2);
    }
}

static void genbank_print_lines(Writer& write, const char *key, const char *content, const WrapMode& wrapMode) {
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

static void genbank_out_one_reference(Writer& write, const GenbankRef& gbk_ref, int gbk_ref_num) {
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
    genbank_out_one_entry(write, "  JOURNAL   ", gbk_ref.journal, wrapWords, NOPERIOD);
    genbank_out_one_entry(write, "  TITLE     ", gbk_ref.title, wrapWords, NOPERIOD);
    genbank_out_one_entry(write, "  STANDARD  ", gbk_ref.standard, wrapWords, NOPERIOD);
}

static void genbank_print_comment_if_content(Writer& write, const char *key, const char *content) {
    // Print one genbank line, wrap around if over column GBMAXLINE

    if (!has_content(content)) return;

    char first[LINESIZE]; sprintf(first, "%*s%s", GBINDENT+RDP_SUBKEY_INDENT, "", key);
    char other[LINESIZE]; sprintf(other, "%*s", GBINDENT+RDP_SUBKEY_INDENT+RDP_CONTINUED_INDENT, "");
    WrapMode(true).print(write, first, other, content, GBMAXLINE);
}

static void genbank_out_origin(const Seq& seq, Writer& write) { // @@@ inline method
    // Output sequence data in genbank format.
    seq.out(write, GENBANK);
}

inline void genbank_print_completeness(Writer& write, char compX, char X) {
    if (compX == ' ') return;
    ca_assert(compX == 'y' || compX == 'n');
    write.outf("              %c' end complete: %s\n", X, compX == 'y' ? "Yes" : "No");
}

void genbank_out_header(const GenBank& gbk, const Seq& seq, Writer& write) {
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
            genbank_out_one_reference(write, gbk.get_ref(indi), indi+1);
        }
    }
    else {
        genbank_out_one_reference(write, GenbankRef(), 1);
    }

    const RDP_comments& comments = gbk.comments;
    const OrgInfo&      orginf   = comments.orginf;
    const SeqInfo&      seqinf   = comments.seqinf;

    if (comments.exists()) {
        write.out("COMMENTS    ");

        if (orginf.exists()) {
            write.out("Organism information\n");

            genbank_print_comment_if_content(write, "Source of strain: ",   orginf.source);
            genbank_print_comment_if_content(write, "Culture collection: ", orginf.cultcoll); // this field is used in ../lib/import/.rdp_old.ift
            genbank_print_comment_if_content(write, "Former name: ",        orginf.formname); // other fields occur in no .ift
            genbank_print_comment_if_content(write, "Alternate name: ",     orginf.nickname);
            genbank_print_comment_if_content(write, "Common name: ",        orginf.commname);
            genbank_print_comment_if_content(write, "Host organism: ",      orginf.hostorg);

            if (seqinf.exists() || str0len(comments.others) > 0)
                write.out("            ");
        }

        if (seqinf.exists()) {
            write.outf("Sequence information (bases 1 to %d)\n", seq.get_len());

            genbank_print_comment_if_content(write, "RDP ID: ",                      seqinf.RDPid);
            genbank_print_comment_if_content(write, "Corresponding GenBank entry: ", seqinf.gbkentry); // this field is used in ../lib/import/.rdp_old.ift
            genbank_print_comment_if_content(write, "Sequencing methods: ",          seqinf.methods);

            genbank_print_completeness(write, seqinf.comp5, '5');
            genbank_print_completeness(write, seqinf.comp3, '3');
        }

        // @@@ use wrapper for code below ?
        // print GBINDENT spaces of the first line
        if (str0len(comments.others) > 0) {
            write.repeated(' ', GBINDENT);
        }

        if (str0len(comments.others) > 0) {
            int length = str0len(comments.others);
            for (indi = 0; indi < length; indi++) {
                write.out(comments.others[indi]);

                // if another line, print GBINDENT spaces first
                if (comments.others[indi] == '\n' && comments.others[indi + 1] != '\0') {
                    write.repeated(' ', GBINDENT);
                }
            }
        }
    }
}

void genbank_out_base_count(const Seq& seq, Writer& write) {
    BaseCounts bases;
    seq.count(bases);
    write.outf("BASE COUNT  %6d a %6d c %6d g %6d t", bases.a, bases.c, bases.g, bases.t);
    if (bases.other) { // don't write 0 others
        write.outf(" %6d others", bases.other);
    }
    write.out('\n');
}

void genbank_out(const GenBank& gbk, const Seq& seq, Writer& write) {
    // Output in a genbank format

    genbank_out_header(gbk, seq, write);
    genbank_out_base_count(seq, write);
    write.out("ORIGIN\n");
    genbank_out_origin(seq, write);
}

bool GenbankReader::read_one_entry(Seq& seq) {
    data.reinit();
    if (!GenbankParser(data, seq, *this).parse_entry()) abort();
    return ok();
}
