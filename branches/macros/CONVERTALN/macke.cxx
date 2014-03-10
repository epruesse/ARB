// -------------------- Macke related subroutines --------------

#include "macke.h"
#include "wrap.h"
#include "parser.h"
#include "rdp_info.h"


#define MACKELIMIT 10000

static int macke_abbrev(const char *line, char *key, int index) {
    // Get the key from a macke line.
    // returns index behind delimiting ':'
    index   = Skip_white_space(line, index);
    int len = parse_key_word(line+index, key, " :\t\n");
    return index+len+1;
}

static void macke_continue_line(const char *key, char *oldname, char*& var, Reader& reader) {
    // Append macke continue line.

    for (++reader; reader.line(); ++reader) {
        if (has_content(reader.line())) {
            char name[TOKENSIZE];
            int  index = macke_abbrev(reader.line(), name, 0);

            if (!str_equal(name, oldname)) break;

            char newkey[TOKENSIZE];
            index = macke_abbrev(reader.line(), newkey, index);
            if (!str_equal(newkey, key)) break;

            skip_eolnl_and_append_spaced(var, reader.line() + index);
        }
    }
}
static void macke_one_entry_in(Reader& reader, const char *key, char *oldname, char*& var, int index) {
    // Get one Macke entry.
    if (has_content(var))
        skip_eolnl_and_append_spaced(var, reader.line() + index);
    else
        freedup(var, reader.line() + index);

    macke_continue_line(key, oldname, var, reader);
}

static void macke_read_seq(Seq& seq, char*& seqabbr, Reader& reader) {
    ca_assert(seq.is_empty());
    for (; reader.line(); ++reader) { // read in sequence data line by line
        if (!has_content(reader.line())) continue;

        char name[TOKENSIZE];
        int  index = macke_abbrev(reader.line(), name, 0);

        if (seqabbr) {
            if (!str_equal(seqabbr, name)) break; // stop if reached different abbrev
        }
        else {
            seqabbr = nulldup(name);
        }

        int  seqnum;
        char data[LINESIZE];
        int scanned = sscanf(reader.line() + index, "%d%s", &seqnum, data);
        if (scanned != 2) throw_errorf(80, "Failed to parse '%s'", reader.line());

        for (int indj = seq.get_len(); indj < seqnum; indj++) seq.add('.');
        for (int indj = 0; data[indj] != '\n' && data[indj] != '\0'; indj++) seq.add(data[indj]);
    }
}

void macke_origin(Seq& seq, char*& seqabbr, Reader& reader) {
    // Read in sequence data in macke file.
    ca_assert(seqabbr);                   // = macke.seqabbr
    macke_read_seq(seq, seqabbr, reader);
}

void macke_out_header(Writer& write) {
    // Output the Macke format header.
    write.out("#-\n#-\n#-\teditor\n");
    const char *date = today_date();
    write.outf("#-\t%s\n#-\n#-\n", date);
}

void macke_seq_display_out(const Macke& macke, Writer& write, Format inType, bool first_sequence) {
    // Output the Macke format each sequence format (wot?)
    if (first_sequence) {
        write.outf("#-\tReference sequence:  %s\n", macke.seqabbr);
        write.out("#-\tAttributes:\n");
    }

    write.out("#=\t\t");
    if (write.out(macke.seqabbr)<8) write.out('\t');
    write.out("\tin  out  vis  prt   ord  ");
    if (inType == SWISSPROT) {
        write.out("pro  lin  n>c");
    }
    else {
        write.out(macke.rna_or_dna == 'r' ? "rna" : "dna");
        write.out("  lin  5>3");
    }
    write.out("  func");
    if (first_sequence) write.out(" ref");
    write.out('\n');
}

static void macke_print_line(Writer& write, const char *prefix, const char *content) {
    // print a macke line and wrap around line after MACKEMAXLINE column.
    WrapMode(true).print(write, prefix, prefix, content, MACKEMAXLINE);
}

static void macke_print_prefixed_line(const Macke& macke, Writer& write, const char *tag, const char *content) {
    ca_assert(has_content(content));

    char prefix[LINESIZE];
    sprintf(prefix, "#:%s:%s:", macke.seqabbr, tag);

    macke_print_line(write, prefix, content);
}

static bool macke_print_prefixed_line_if_content(const Macke& macke, Writer& write, const char *tag, const char *content) {
    if (!has_content(content)) return false;
    macke_print_prefixed_line(macke, write, tag, content);
    return true;
}

static const char *genbankEntryComments[] = {
    "KEYWORDS",
    "GenBank ACCESSION",
    "auth",
    "title",
    "jour",
    "standard",
    "Source of strain",
    "Former name",
    "Alternate name",
    "Common name",
    "Host organism",
    "RDP ID",
    "Sequencing methods",
    "3' end complete",
    "5' end complete",
};

static bool macke_is_genbank_entry_comment(const char *Str) {
    char keyword[TOKENSIZE];
    macke_key_word(Str, 0, keyword);

    return lookup_keyword(keyword, genbankEntryComments) >= 0;
}

static void macke_print_keyword_rem(const Macke& macke, int index, Writer& write) {
    // Print out keyworded remark line in Macke file with wrap around functionality.
    // (Those keywords are defined in GenBank COMMENTS by RDP group)
    char first[LINESIZE]; sprintf(first, "#:%s:rem:", macke.seqabbr);
    char other[LINESIZE]; sprintf(other, "%s:%*s", first, RDP_SUBKEY_INDENT, "");
    const char *remark = macke.get_rem(index);

    WrapMode(true).print(write, first, other, remark, MACKEMAXLINE);
}

void macke_seq_info_out(const Macke& macke, Writer& write) {
    // Output sequence information

    macke_print_prefixed_line_if_content(macke, write, "name",   macke.name);
    macke_print_prefixed_line_if_content(macke, write, "strain", macke.strain);
    macke_print_prefixed_line_if_content(macke, write, "subsp",  macke.subspecies);
    macke_print_prefixed_line_if_content(macke, write, "atcc",   macke.atcc);
    macke_print_prefixed_line_if_content(macke, write, "rna",    macke.rna);        // old version entry
    macke_print_prefixed_line_if_content(macke, write, "date",   macke.date);
    macke_print_prefixed_line_if_content(macke, write, "acs",    macke.acs)
        || macke_print_prefixed_line_if_content(macke, write, "acs", macke.nbk);    // old version entry
    macke_print_prefixed_line_if_content(macke, write, "auth",   macke.author);
    macke_print_prefixed_line_if_content(macke, write, "jour",   macke.journal);
    macke_print_prefixed_line_if_content(macke, write, "title",  macke.title);
    macke_print_prefixed_line_if_content(macke, write, "who",    macke.who);

    // print out remarks, wrap around if more than MACKEMAXLINE columns
    for (int indi = 0; indi < macke.get_rem_count(); indi++) {
        if (macke_is_genbank_entry_comment(macke.get_rem(indi))) {
            macke_print_keyword_rem(macke, indi, write);
        }
        else { // general comment
            macke_print_prefixed_line(macke, write, "rem", macke.get_rem(indi));
        }
    }
}

int macke_key_word(const char *line, int index, char *key) {
    // Find the key in Macke line.
    // return position behind ':' delimiter
    int len = parse_key_word(line+index, key, ":\n");
    return len ? index+len+1 : index;
}

void macke_seq_data_out(const Seq& seq, const Macke& macke, Writer& write) {
    // Output Macke format sequence data
    int indj, indk;

    if (seq.get_len() > MACKELIMIT) {
        warningf(145, "Length of sequence data is %d over AE2's limit %d.", seq.get_len(), MACKELIMIT);
    }

    const char *sequence = seq.get_seq();
    for (indk = indj = 0; indk < seq.get_len(); indk++) {
        if (indj == 0)
            write.outf("%s%6d ", macke.seqabbr, indk);

        write.out(sequence[indk]);

        indj++;
        if (indj == 50) {
            indj = 0;
            write.out('\n');
        }
    }

    if (indj != 0)
        write.out('\n');
    // every sequence
}

void MackeReader::read_to_start() {
    r1->skipOverLinesThat(Not(isMackeSeqInfo));   // skip to #:; where the sequence information is
    r2->skipOverLinesThat(isMackeNonSeq);         // skip to where sequence data starts
    r3->skipOverLinesThat(Not(isMackeSeqHeader)); // skip to #=; where sequence first appears
}


MackeReader::MackeReader(const char *inName_)
    : inName(strdup(inName_)),
      seqabbr(dummy),
      dummy(NULL),
      r1(new Reader(inName)),
      r2(new Reader(inName)),
      r3(new Reader(inName)),
      using_reader(&r1)
{
    read_to_start();
}

MackeReader::~MackeReader() {
    char                    *msg = NULL;
    const Convaln_exception *exc     = Convaln_exception::exception_thrown();

    ca_assert(using_reader);
    delete *using_reader; *using_reader = NULL;

    // avoid that all 3 readers decorate the error
    if (exc) msg = strdup(exc->get_msg());
    delete r3; r3 = NULL;
    delete r2; r2 = NULL;
    delete r1; r1 = NULL;
    if (exc) { exc->replace_msg(msg); free(msg); }

    free(inName);
}

class MackeParser : public Parser {
    Macke& macke;

public:
    MackeParser(Macke& macke_, Seq& seq_, Reader& reader_) : Parser(seq_, reader_), macke(macke_) {}

    void parse_section() OVERRIDE {
        ca_assert(0); // @@@ unused yet
    }
};


bool MackeReader::macke_in(Macke& macke) {
    // Read in one sequence data from Macke file.
    char oldname[TOKENSIZE], name[TOKENSIZE];
    char key[TOKENSIZE];
    int  index;

    // r1 points to sequence information
    // r2 points to sequence data
    // r3 points to sequence names

    usingReader(r3);
    if (!r3->line() || !isMackeSeqHeader(r3->line())) return false;

    // skip to next "#:" line or end of file
    usingReader(r1);
    r1->skipOverLinesThat(Not(isMackeSeqInfo));

    // read in sequence name
    usingReader(r3);
    index   = macke_abbrev(r3->line(), oldname, 2);
    freedup(macke.seqabbr, oldname);
    seqabbr = macke.seqabbr;

    // read sequence information
    usingReader(r1);
    for (index = macke_abbrev(r1->line(), name, 2);
         r1->line() && isMackeSeqInfo(r1->line()) && str_equal(name, oldname);
         )
    {
        index = macke_abbrev(r1->line(), key, index);
        if (str_equal(key, "name")) {
            macke_one_entry_in(*r1, "name", oldname, macke.name, index);
        }
        else if (str_equal(key, "atcc")) {
            macke_one_entry_in(*r1, "atcc", oldname, macke.atcc, index);
        }
        else if (str_equal(key, "rna")) {
            // old version entry
            macke_one_entry_in(*r1, "rna", oldname, macke.rna, index);
        }
        else if (str_equal(key, "date")) {
            macke_one_entry_in(*r1, "date", oldname, macke.date, index);
        }
        else if (str_equal(key, "nbk")) {
            // old version entry
            macke_one_entry_in(*r1, "nbk", oldname, macke.nbk, index);
        }
        else if (str_equal(key, "acs")) {
            macke_one_entry_in(*r1, "acs", oldname, macke.acs, index);
        }
        else if (str_equal(key, "subsp")) {
            macke_one_entry_in(*r1, "subsp", oldname, macke.subspecies, index);
        }
        else if (str_equal(key, "strain")) {
            macke_one_entry_in(*r1, "strain", oldname, macke.strain, index);
        }
        else if (str_equal(key, "auth")) {
            macke_one_entry_in(*r1, "auth", oldname, macke.author, index);
        }
        else if (str_equal(key, "title")) {
            macke_one_entry_in(*r1, "title", oldname, macke.title, index);
        }
        else if (str_equal(key, "jour")) {
            macke_one_entry_in(*r1, "jour", oldname, macke.journal, index);
        }
        else if (str_equal(key, "who")) {
            macke_one_entry_in(*r1, "who", oldname, macke.who, index);
        }
        else if (str_equal(key, "rem")) {
            macke.add_remark(r1->line()+index);
            ++(*r1);
        }
        else {
            warningf(144, "Unidentified AE2 key word #%s#", key);
            ++(*r1);
        }
        if (r1->line() && isMackeSeqInfo(r1->line())) index = macke_abbrev(r1->line(), name, 2);
        else index = 0;
    }

    ++(*r3);

    return true;
}

bool MackeReader::read_one_entry(Seq& seq) {
    data.reinit();
    if (!macke_in(data) || !read_seq_data(seq)) abort();
    seq.set_id(data.get_id());
    return ok();
}


