// -------------------- Macke related subroutines --------------

#include "macke.h"
#include "wrap.h"


#define MACKELIMIT 10000

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
void macke_origin(Seq& seq, const char *key, Reader& reader) { 
    // Read in sequence data in macke file.
    ca_assert(seq.is_empty());

    for (; reader.line();) { // read in sequence data line by line
        char name[TOKENSIZE];
        int  index = macke_abbrev(reader.line(), name, 0);

        if (!str_equal(key, name)) break;

        int  seqnum;
        char data[LINESIZE];
        ASSERT_RESULT(int, 2, sscanf(reader.line() + index, "%d%s", &seqnum, data));
        for (int indj = seq.get_len(); indj < seqnum; indj++) seq.add('.');
        for (int indj = 0; data[indj] != '\n' && data[indj] != '\0'; indj++) seq.add(data[indj]);

        ++reader;
    }
}

int macke_abbrev(const char *line, char *key, int index) {
    // Find the key in Macke line.
    int indi;

    // skip white space
    index = Skip_white_space(line, index);

    for (indi = index; line[indi] != ' ' && line[indi] != ':' && line[indi] != '\t' && line[indi] != '\n' && line[indi] != '\0'; indi++)
        key[indi - index] = line[indi];

    key[indi - index] = '\0';
    return (indi + 1);
}

bool macke_is_continued_remark(const char *str) {
    /* If there is 3 blanks at the beginning of the line, it is continued line.
     *
     * The comment above is lying:
     *      The function always only tested for 2 spaces
     *      and the converter only produced 2 spaces.
     */
    return strncmp(str, ":  ", 3) == 0;
}

void macke_in_simple(Macke& macke, Seq& seq, Reader& reader) {
    // Read in next sequence name and data only.
    // (sequence name is stored in 'macke.seqabbr')

    ca_assert(seq.is_empty());
    freenull(macke.seqabbr);
    
    if (reader.line()) reader.skipOverLinesThat(isMackeHeader); // skip header

    // read in sequence data line by line
    for (; reader.line() ;) {
        int index;
        {
            char name[TOKENSIZE];
            index = macke_abbrev(reader.line(), name, 0);
            if (macke.seqabbr) {
                if (!str_equal(macke.seqabbr, name)) break; // stop if reached different abbrev
            }
            else {
                macke.seqabbr = nulldup(name);
            }
        }
        {
            int  seqnum;
            char data[LINESIZE];
            ASSERT_RESULT(int, 2, sscanf(reader.line() + index, "%d%s", &seqnum, data));
            for (int indj = seq.get_len(); indj < seqnum; indj++) seq.add('.');
            for (int indj = 0; data[indj] != '\n' && data[indj] != '\0'; indj++) seq.add(data[indj]);
        }

        ++reader;
    }
}
void macke_out_header(FILE * fp) {
    // Output the Macke format header.
    fputs("#-\n#-\n#-\teditor\n", fp);
    const char *date = today_date();
    fprintf(fp, "#-\t%s\n#-\n#-\n", date);
}

void macke_seq_display_out(const Macke& macke, FILE * fp, int format, bool first_sequence) {
    // Output the Macke format each sequence format (wot?)

    char   token[TOKENSIZE], direction[TOKENSIZE];

    if (format == SWISSPROT) {
        strcpy(token, "pro");
        strcpy(direction, "n>c");
    }
    else {
        strcpy(direction, "5>3");
        if (macke.rna_or_dna == 'r')
            strcpy(token, "rna");
        else
            strcpy(token, "dna");
    }
    if (first_sequence) {
        fprintf(fp, "#-\tReference sequence:  %s\n", macke.seqabbr);
        fputs("#-\tAttributes:\n", fp);

        if (str0len(macke.seqabbr) < 8)
            fprintf(fp, "#=\t\t%s\t \tin  out  vis  prt   ord  %s  lin  %s  func ref\n", macke.seqabbr, token, direction);
        else
            fprintf(fp, "#=\t\t%s\tin  out  vis  prt   ord  %s  lin  %s  func ref\n", macke.seqabbr, token, direction);
    }
    else if (str0len(macke.seqabbr) < 8)
        fprintf(fp, "#=\t\t%s\t\tin  out  vis  prt   ord  %s  lin  %s  func\n", macke.seqabbr, token, direction);
    else
        fprintf(fp, "#=\t\t%s\tin  out  vis  prt   ord  %s  lin  %s  func\n", macke.seqabbr, token, direction);
}

static void macke_print_line(FILE * fp, const char *prefix, const char *content) { // @@@ WRAPPER
    // print a macke line and wrap around line after MACKEMAXLINE column.
    // WrapMode(true).print(fp, prefix, prefix, content, MACKEMAXLINE, false); // @@@ wanted
    WrapMode(true).print(fp, prefix, prefix, content, MACKEMAXLINE, WRAPBUG_WRAP_BEFORE_SEP);
}

static void macke_print_prefixed_line(const Macke& macke, FILE *fp, const char *tag, const char *content) {
    ca_assert(has_content(content));

    char prefix[LINESIZE];
    sprintf(prefix, "#:%s:%s:", macke.seqabbr, tag);

    macke_print_line(fp, prefix, content);
}

static bool macke_print_prefixed_line_if_content(const Macke& macke, FILE *fp, const char *tag, const char *content) {
    if (!has_content(content)) return false;
    macke_print_prefixed_line(macke, fp, tag, content);
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
    macke_key_word(Str, 0, keyword, TOKENSIZE);

    return lookup_keyword(keyword, genbankEntryComments) >= 0;
}

static void macke_print_keyword_rem(const Macke& macke, int index, FILE * fp) { // @@@ WRAPPER
    // Print out keyworded remark line in Macke file with wrap around functionality.
    // (Those keywords are defined in GenBank COMMENTS by RDP group)
    char first[LINESIZE]; sprintf(first, "#:%s:rem:", macke.seqabbr);
    char other[LINESIZE]; sprintf(other, "%s:%*s", first, RDP_SUBKEY_INDENT, "");
    const char *remark = macke.get_rem(index);

    // WrapMode(true).print(fp, first, other, remark, MACKEMAXLINE, false); // @@@ wanted correct behavior
    WrapMode(true).print(fp, first, other, remark, MACKEMAXLINE-1, WRAPBUG_WRAP_AT_SPACE); // works with testdata
}

void macke_seq_info_out(const Macke& macke, FILE * fp) {
    // Output sequence information

    macke_print_prefixed_line_if_content(macke, fp, "name",   macke.name);
    macke_print_prefixed_line_if_content(macke, fp, "strain", macke.strain);
    macke_print_prefixed_line_if_content(macke, fp, "subsp",  macke.subspecies);
    macke_print_prefixed_line_if_content(macke, fp, "atcc",   macke.atcc);
    macke_print_prefixed_line_if_content(macke, fp, "rna",    macke.rna);        // old version entry
    macke_print_prefixed_line_if_content(macke, fp, "date",   macke.date);
    macke_print_prefixed_line_if_content(macke, fp, "acs",    macke.acs)
        || macke_print_prefixed_line_if_content(macke, fp, "acs", macke.nbk);    // old version entry
    macke_print_prefixed_line_if_content(macke, fp, "auth",   macke.author);
    macke_print_prefixed_line_if_content(macke, fp, "jour",   macke.journal);
    macke_print_prefixed_line_if_content(macke, fp, "title",  macke.title);
    macke_print_prefixed_line_if_content(macke, fp, "who",    macke.who);

    // print out remarks, wrap around if more than MACKEMAXLINE columns
    for (int indi = 0; indi < macke.get_rem_count(); indi++) {
        if (macke_is_genbank_entry_comment(macke.get_rem(indi))) {
            macke_print_keyword_rem(macke, indi, fp);
        }
        else { // general comment
            macke_print_prefixed_line(macke, fp, "rem", macke.get_rem(indi));
        }
    }
}

int macke_key_word(const char *line, int index, char *key, int length) {
    // Find the key in Macke line.
    int indi;

    if (line == NULL) {
        key[0] = '\0';
        return (index);
    }

    // skip white space
    index = Skip_white_space(line, index);

    for (indi = index; (indi - index) < (length - 1) && line[indi] != ':' && line[indi] != '\n' && line[indi] != '\0'; indi++)
        key[indi - index] = line[indi];

    key[indi - index] = '\0';

    return (indi + 1);
}

void macke_seq_data_out(const Seq& seq, const Macke& macke, FILE * fp) {
    // Output Macke format sequence data
    int indj, indk;

    if (seq.get_len() > MACKELIMIT) {
        warningf(145, "Length of sequence data is %d over AE2's limit %d.", seq.get_len(), MACKELIMIT);
    }

    const char *sequence = seq.get_seq();
    for (indk = indj = 0; indk < seq.get_len(); indk++) {
        if (indj == 0)
            fprintf(fp, "%s%6d ", macke.seqabbr, indk);

        fputc(sequence[indk], fp);

        indj++;
        if (indj == 50) {
            indj = 0;
            fputc('\n', fp);
        }
    }

    if (indj != 0)
        fputc('\n', fp);
    // every sequence
}

void MackeReader::start_reading() {
    r1 = new Reader(inName);
    r2 = new Reader(inName);
    r3 = new Reader(inName);
}

void MackeReader::stop_reading() {
    delete r3; r3 = NULL;
    delete r2; r2 = NULL;
    delete r1; r1 = NULL;
}

bool MackeReader::mackeIn(Macke& macke) {
    // Read in one sequence data from Macke file.
    char oldname[TOKENSIZE], name[TOKENSIZE];
    char key[TOKENSIZE];
    int  index;

    // r1 points to sequence information
    // r2 points to sequence data
    // r3 points to sequence names
    if (firstRead) {
        start_reading();

        r1->skipOverLinesThat(Not(isMackeSeqInfo));   // skip to #:; where the sequence information is
        r2->skipOverLinesThat(isMackeNonSeq);         // skip to where sequence data starts
        r3->skipOverLinesThat(Not(isMackeSeqHeader)); // skip to #=; where sequence first appears

        firstRead = false;
    }
    else {
        ++(*r3);
    }

    if (!isMackeSeqHeader(r3->line())) return false;

    // skip to next "#:" line or end of file
    r1->skipOverLinesThat(Not(isMackeSeqInfo));

    // read in sequence name
    index   = macke_abbrev(r3->line(), oldname, 2);
    freedup(macke.seqabbr, oldname);
    seqabbr = macke.seqabbr;

    // read sequence information
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

    return true;
}

