#include "embl.h"
#include "genbank.h"
#include "macke.h"
#include "wrap.h"

static void embl_continue_line(const char *pattern, char*& Str, Reader& reader) {
    // if there are (numb) blanks at the beginning of line,
    // it is a continue line of the current command.
    int  ind;
    char key[TOKENSIZE], temp[LINESIZE];

    // check continue lines
    for (++reader; reader.line(); ++reader) {
        if (has_content(reader.line())) {
            embl_key_word(reader.line(), 0, key);
            if (!str_equal(pattern, key)) break;

            // remove end-of-line, if there is any
            ind = Skip_white_space(reader.line(), p_nonkey_start);
            strcpy(temp, reader.line() + ind);
            skip_eolnl_and_append_spaced(Str, temp);
        }
    }
}

static void embl_one_entry(Reader& reader, char*& entry, const char *key) {
    // Read in one embl entry lines.
    int index = Skip_white_space(reader.line(), p_nonkey_start);
    freedup(entry, reader.line() + index);
    embl_continue_line(key, entry, reader);
}

static void embl_date(Embl& embl, Reader& reader) {
    // Read in embl DATE lines.
    int index = Skip_white_space(reader.line(), p_nonkey_start);
    freedup(embl.dateu, reader.line() + index);

    ++reader;

    char key[TOKENSIZE];
    embl_key_word(reader.line(), 0, key);
    if (str_equal(key, "DT")) {
        index = Skip_white_space(reader.line(), p_nonkey_start);
        freedup(embl.datec, reader.line() + index);
        // skip the rest of DT lines
        do {
            ++reader;
            if (!reader.line()) break;
            embl_key_word(reader.line(), 0, key);
        }
        while (str_equal(key, "DT"));
    }
    else {
        // always expect more than two DT lines
        warning(33, "one DT line is missing");
    }
}

static void embl_correct_title(Emblref& ref) {
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

int comment_subkey(const char *line, char *key) {
    // Get the subkey-word (including delimiting ':') from a comment line
    int len = parse_key_word(line, key, ":\t\n(");
    if (!len) return 0;

    if (line[len] == ':') {
        key[len]   = ':';
        key[len+1] = 0;
    }
    return len+1;
}

inline bool is_embl_comment(const char *line) { return line && line[0] == 'C' && line[1] == 'C'; }

static void embl_one_comment_entry(char*& datastring, int start_index, Reader& reader) {
    // Read in one embl sub-entry in comments lines.
    // If it's not a RDP defined comment, you should not call this function.

    int index = Skip_white_space(reader.line(), start_index);
    freedup(datastring, reader.line() + index);

    const int expectedIndent = RDP_CONTINUED_INDENT+RDP_SUBKEY_INDENT;

    for (++reader;
         is_embl_comment(reader.line()) && count_spaces(reader.line() + 2) >= expectedIndent;
         ++reader)
    {
        index = Skip_white_space(reader.line(), p_nonkey_start + expectedIndent);

        char temp[LINESIZE];
        strcpy(temp, reader.line() + index);
        skip_eolnl_and_append_spaced(datastring, temp);
    }
}

static void embl_comments(Embl& embl, Reader& reader) {
    // Read in embl comment lines.

    for (; is_embl_comment(reader.line());) {
        char key[TOKENSIZE];
        int  index  = Skip_white_space(reader.line(), 5);
        int  offset = comment_subkey(reader.line() + index, key);
        index       = Skip_white_space(reader.line(), index + offset);

        RDP_comment_parser one_comment_entry = embl_one_comment_entry;
        RDP_comments&      comments          = embl.comments;

        if (!parse_RDP_comment(comments, one_comment_entry, key, index, reader)) {
            // other comments
            Append(comments.others, reader.line() + 5);
            ++reader;
        }
    }
}

static void embl_skip_unidentified(const char *pattern, Reader& reader) {
    // if there are (numb) blanks at the beginning of line,
    // it is a continue line of the current command.

    for (++reader; reader.line(); ++reader) {
        char  key[TOKENSIZE];
        embl_key_word(reader.line(), 0, key);
        if (!str_equal(key, pattern)) break;
    }
}

void EmblParser::parse_section() {
    char key[TOKENSIZE];
    embl_key_word(reader.line(), 0, key);
    state = ENTRY_STARTED;
    parse_keyed_section(key);
}

static void embl_origin(Seq& seq, Reader& reader) {
    // Read in embl sequence data.
    ca_assert(seq.is_empty());

    // read in whole sequence data
    for (++reader;
         reader.line() && !is_sequence_terminator(reader.line());
         ++reader
         )
    {
        const char *line = reader.line();
        for (int idx = 5; line[idx]; ++idx) {
            char ch = line[idx];
            if (ch == ' ' || ch == '\n') continue;
            if (idx>70) continue;
            seq.add(ch);
        }
    }
}

void EmblParser::parse_keyed_section(const char *key) {
    if (str_equal(key, "ID")) {
        embl_one_entry(reader, embl.ID, key);
    }
    else if (str_equal(key, "DT")) {
        embl_date(embl, reader);
    }
    else if (str_equal(key, "DE")) {
        embl_one_entry(reader, embl.description, key);
    }
    else if (str_equal(key, "OS")) {
        embl_one_entry(reader, embl.os, key);
    }
    else if (str_equal(key, "AC")) {
        embl_one_entry(reader, embl.accession, key);
    }
    else if (str_equal(key, "KW")) {
        embl_one_entry(reader, embl.keywords, key);

        // correct missing '.'
        if (!has_content(embl.keywords)) freedup(embl.keywords, ".\n");
        else terminate_with(embl.keywords, '.');
    }
    else if (str_equal(key, "DR")) {
        embl_one_entry(reader, embl.dr, key);
    }
    else if (str_equal(key, "RA")) {
        Emblref& ref = embl.get_latest_ref();
        embl_one_entry(reader, ref.author, key);
        terminate_with(ref.author, ';');
    }
    else if (str_equal(key, "RT")) {
        Emblref& ref = embl.get_latest_ref();
        embl_one_entry(reader, ref.title, key);
        embl_correct_title(ref);
    }
    else if (str_equal(key, "RL")) {
        Emblref& ref = embl.get_latest_ref();
        embl_one_entry(reader, ref.journal, key);
        terminate_with(ref.journal, '.');
    }
    else if (str_equal(key, "RP")) {
        Emblref& ref = embl.get_latest_ref();
        embl_one_entry(reader, ref.processing, key);
    }
    else if (str_equal(key, "RN")) {
        embl.resize_refs(embl.get_refcount()+1);
        ++reader;
    }
    else if (str_equal(key, "CC")) {
        embl_comments(embl, reader);
    }
    else if (str_equal(key, "SQ")) {
        embl_origin(seq, reader);
        state = ENTRY_COMPLETED;
    }
    else {
        embl_skip_unidentified(key, reader);
    }
}

void embl_key_word(const char *line, int index, char *key) {
    parse_key_word(line+index, key, " \t\n");
}

static void embl_print_lines(Writer& write, const char *key, const char *content, const WrapMode& wrapMode) {
    // Print EMBL entry and wrap around if line over EMBLMAXLINE.
    ca_assert(strlen(key) == 2);

    char prefix[TOKENSIZE];
    sprintf(prefix, "%-*s", EMBLINDENT, key);

    wrapMode.print(write, prefix, prefix, content, EMBLMAXLINE);
}

static bool embl_print_lines_if_content(Writer& write, const char *key, const char *content, const WrapMode& wrapMode, bool followed_by_spacer) {
    if (has_content(content)) {
        embl_print_lines(write, key, content, wrapMode);
        if (followed_by_spacer) write.out("XX\n");
        return true;
    }
    return false;
}

static void embl_print_comment_if_content(Writer& write, const char *key, const char *content) {
    // Print one embl comment line, wrap around

    if (!has_content(content)) return;

    char first[LINESIZE]; sprintf(first, "CC%*s%s", (EMBLINDENT-2)+RDP_SUBKEY_INDENT, "", key);
    char other[LINESIZE]; sprintf(other, "CC%*s", (EMBLINDENT-2)+RDP_SUBKEY_INDENT+RDP_CONTINUED_INDENT, "");
    WrapMode(true).print(write, first, other, content, EMBLMAXLINE);
}

inline void embl_print_completeness(Writer& write, char compX, char X) {
    if (compX == ' ') return;
    ca_assert(compX == 'y' || compX == 'n');
    write.outf("CC     %c' end complete: %s\n", X, compX == 'y' ? "Yes" : "No");
}

static void embl_out_comments(const Embl& embl, const Seq& seq, Writer& write) {
    // Print out the comments part of EMBL format.

    const OrgInfo& orginf = embl.comments.orginf;
    if (orginf.exists()) {
        write.out("CC   Organism information\n");

        embl_print_comment_if_content(write, "Source of strain: ",   orginf.source);
        embl_print_comment_if_content(write, "Culture collection: ", orginf.cultcoll);
        embl_print_comment_if_content(write, "Former name: ",        orginf.formname);
        embl_print_comment_if_content(write, "Alternate name: ",     orginf.nickname);
        embl_print_comment_if_content(write, "Common name: ",        orginf.commname);
        embl_print_comment_if_content(write, "Host organism: ",      orginf.hostorg);
    }

    const SeqInfo& seqinf = embl.comments.seqinf;
    if (seqinf.exists()) {
        write.outf("CC   Sequence information (bases 1 to %d)\n", seq.get_len());

        embl_print_comment_if_content(write, "RDP ID: ",                      seqinf.RDPid);
        embl_print_comment_if_content(write, "Corresponding GenBank entry: ", seqinf.gbkentry);
        embl_print_comment_if_content(write, "Sequencing methods: ",          seqinf.methods);

        embl_print_completeness(write, seqinf.comp5, '5');
        embl_print_completeness(write, seqinf.comp3, '3');
    }

    embl_print_lines_if_content(write, "CC", embl.comments.others, WrapMode("\n"), true);
}

static void embl_out_origin(const Seq& seq, Writer& write) {
    // Print out the sequence data of EMBL format.
    BaseCounts bases;
    seq.count(bases);
    write.outf("SQ   Sequence %d BP; %d A; %d C; %d G; %d T; %d other;\n",
               seq.get_len(), bases.a, bases.c, bases.g, bases.t, bases.other);

    seq.out(write, EMBL);
}

void embl_out_header(const Embl& embl, const Seq& seq, Writer& write) {
    WrapMode wrapWords(true);
    WrapMode neverWrap(false);

    embl_print_lines_if_content(write, "ID", embl.ID,        neverWrap, true);
    embl_print_lines_if_content(write, "AC", embl.accession, wrapWords, true);

    {
        bool dt1 = embl_print_lines_if_content(write, "DT", embl.dateu, neverWrap, false);
        bool dt2 = embl_print_lines_if_content(write, "DT", embl.datec, neverWrap, false);
        if (dt1 || dt2) write.out("XX\n");
    }

    embl_print_lines_if_content(write, "DE", embl.description, wrapWords,     true);
    embl_print_lines_if_content(write, "KW", embl.keywords,    WrapMode(";"), true);

    if (has_content(embl.os)) {
        embl_print_lines(write, "OS", embl.os, wrapWords);
        write.out("OC   No information.\n");
        write.out("XX\n");
    }

    // GenbankRef
    for (int indi = 0; indi < embl.get_refcount(); indi++) {
        const Emblref& ref = embl.get_ref(indi);

        write.outf("RN   [%d]\n", indi + 1);
        embl_print_lines_if_content(write, "RP", ref.processing, neverWrap, false);
        embl_print_lines_if_content(write, "RA", ref.author, WrapMode(","), false);

        if (has_content(ref.title)) embl_print_lines(write, "RT", ref.title, wrapWords);
        else write.out("RT   ;\n");

        embl_print_lines_if_content(write, "RL", ref.journal, wrapWords, false);
        write.out("XX\n");
    }

    if (has_content(embl.dr)) {
        embl_print_lines(write, "DR", embl.dr, wrapWords);
        write.out("XX\n");
    }

    embl_out_comments(embl, seq, write);
}

void embl_out(const Embl& embl, const Seq& seq, Writer& write) {
    // Output EMBL data.
    embl_out_header(embl, seq, write);
    embl_out_origin(seq, write);
}

static char *etog_author(char *Str) {
    // Convert EMBL author format to Genbank author format.
    int  indi, indk, len, index;
    char token[TOKENSIZE], *author;

    author = strdup("");
    for (indi = index = 0, len = str0len(Str) - 1; indi < len; indi++, index++) {
        if (Str[indi] == ',' || Str[indi] == ';') {
            token[index--] = '\0';
            if (has_content(author)) {
                Append(author, (Str[indi] == ',') ? "," : " and");
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
static char *etog_journal(const char *eJournal) {
    // Convert journal part from EMBL to GenBank format.
    char *new_journal = 0;
    char  token[TOKENSIZE];

    scan_token_or_die(token, eJournal);
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
static void etog_convert_references(const Embl& embl, GenBank& gbk) {
    // Convert reference from EMBL to GenBank format.
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

int etog(const Embl& embl, GenBank& gbk, const Seq& seq) { // __ATTR__USERESULT
    // Convert from embl to genbank format.
    int  indi;
    char key[TOKENSIZE], temp[LONGTEXT];
    char t1[TOKENSIZE], t2[TOKENSIZE], t3[TOKENSIZE];

    embl_key_word(embl.ID, 0, key);
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
    {
        const char *date = has_content(embl.dateu) ? embl.dateu : today_date();
        sprintf((temp + 10), "%7d bp    RNA             RNA       %s\n",
                seq.get_len(),
                genbank_date(date));
    }
    freedup(gbk.locus, temp);

    // DEFINITION
    if (copy_content(gbk.definition, embl.description)) terminate_with(gbk.definition, '.');

    // SOURCE and DEFINITION if not yet defined
    if (copy_content(gbk.source, embl.os)) {
        freedup(gbk.organism, embl.os);
        if (!has_content(embl.description)) {
            freedup(gbk.definition, embl.os);
        }
    }

    // COMMENT GenBank entry
    copy_content(gbk.accession, embl.accession);
    if (has_content(embl.keywords) && embl.keywords[0] != '.') {
        freedup(gbk.keywords, embl.keywords);
    }

    etog_convert_references(embl, gbk);
    gbk.comments.set_content_from(embl.comments);

    return (1);
}

int etom(const Embl& embl, Macke& macke, const Seq& seq) { // __ATTR__USERESULT
    // Convert from embl format to Macke format.
    GenBank gbk;
    return etog(embl, gbk, seq) && gtom(gbk, macke);
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>

#define TEST_EXPECT_ETOG_JOURNAL_PARSES(i,o)              \
    do {                                                  \
        char *dup = strdup(i);                            \
        char *res = etog_journal(dup);                    \
        TEST_EXPECT_EQUAL(res, o);                        \
        free(res);                                        \
        free(dup);                                        \
    } while (0)

void TEST_BASIC_etog_journal() {
    // behavior documented in r6943:
    TEST_EXPECT_ETOG_JOURNAL_PARSES("Gene 134:283-287(1993).\n",
                                    "Gene 134, 283-287 (1993)\n");
    TEST_EXPECT_ETOG_JOURNAL_PARSES("J. Exp. Med. 179:1809-1821(1994).\n",
                                    "J. Exp. Med. 179, 1809-1821 (1994)\n");
    TEST_EXPECT_ETOG_JOURNAL_PARSES("Unpublished whatever.\n",
                                    "Unpublished whatever\n");
    TEST_EXPECT_ETOG_JOURNAL_PARSES("bla bla bla.\n",
                                    "\n"); // skips if can't parse
    TEST_EXPECT_ETOG_JOURNAL_PARSES("bla bla bla\n",
                                    "\n");
}

#endif // UNIT_TESTS

static char *gtoe_author(char *author) {
    // Convert GenBank author to EMBL author.
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
static char *gtoe_journal(char *Str) {
    // Convert GenBank journal to EMBL journal.
    char token[TOKENSIZE], *journal;
    int  indi, indj, index, len;

    if (scan_token(token, Str)) {
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
static void gtoe_reference(const GenBank& gbk, Embl& embl) {
    // Convert references from GenBank to EMBL.
    if (gbk.has_refs()) {
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

int gtoe(const GenBank& gbk, Embl& embl, const Seq& seq) { // __ATTR__USERESULT
    // Genbank to EMBL.
    {
        char temp[LONGTEXT];
        strcpy(temp, gbk.get_id());

        upcase(temp); // Adjust short-id, EMBL short_id always upper case
        for (int indi = min(str0len(temp), 9); indi < 10; indi++)
            temp[indi] = ' ';

        sprintf(temp + 10, "preliminary; RNA; UNA; %d BP.\n", seq.get_len());
        freedup(embl.ID, temp);
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
    copy_content(embl.description, gbk.definition);
    // EMBL KW line
    if (copy_content(embl.keywords, gbk.keywords)) {
        terminate_with(embl.keywords, '.');
    }
    else {
        freedup(embl.keywords, ".\n");
    }

    copy_content(embl.os, gbk.organism); // EMBL OS line
    // reference
    gtoe_reference(gbk, embl);

    // EMBL DR line
    {
        char token[TOKENSIZE];
        char temp[LONGTEXT];

        scan_token_or_die(token, gbk.locus); // short_id
        if (has_content(gbk.comments.seqinf.RDPid)) {
            char rdpid[TOKENSIZE];
            scan_token_or_die(rdpid, gbk.comments.seqinf.RDPid);
            sprintf(temp, "RDP; %s; %s.\n", rdpid, token);
        }
        else {
            sprintf(temp, "RDP; %s.\n", token);
        }
        freedup(embl.dr, temp);
    }
    embl.comments.set_content_from(gbk.comments);

    return (1);
}

static int partial_mtoe(const Macke& macke, Embl& embl) {
    // Handle subspecies information when converting from Macke to EMBL.
    char*& others = embl.comments.others;

    if (has_content(macke.strain)) {
        int  ridx        = skip_pattern(others, "*source:");
        bool have_strain = ridx >= 0 && stristr(others+ridx, "strain=");

        if (!have_strain) {
            if (!has_content(others)) freenull(others);
            Append(others, "*source: strain=");
            Append(others, macke.strain);
            if (!is_end_mark(others[str0len(others) - 2])) skip_eolnl_and_append(others, ";\n");
        }
    }

    if (has_content(macke.subspecies)) {
        int  ridx       = skip_pattern(others, "*source:");
        bool have_subsp = ridx >= 0 && find_subspecies(others+ridx, '=') >= 0;

        if (!have_subsp) {
            if (!has_content(others)) freenull(others);
            Append(others, "*source: subspecies=");
            Append(others, macke.subspecies);
            if (!is_end_mark(others[str0len(others) - 2])) skip_eolnl_and_append(others, ";\n");
        }
    }

    return (1);
}

int mtoe(const Macke& macke, Embl& embl, const Seq& seq) { // __ATTR__USERESULT
    GenBank gbk;
    return mtog(macke, gbk, seq) && gtoe(gbk, embl, seq) && partial_mtoe(macke, embl);
}

bool EmblSwissprotReader::read_one_entry(Seq& seq) {
    data.reinit();
    if (!EmblParser(data, seq, *this).parse_entry()) abort();
    return ok();
}
