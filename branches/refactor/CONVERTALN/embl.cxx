#include "embl.h"
#include "genbank.h"
#include "macke.h"
#include "wrap.h"

extern int warning_out;

static void embl_continue_line(char *pattern, char*& Str, Reader& reader) {
    // if there are (numb) blanks at the beginning of line,
    // it is a continue line of the current command.
    int  ind;
    char key[TOKENSIZE], temp[LINESIZE];

    // check continue lines
    for (++reader; reader.line(); ++reader) {
        if (has_content(reader.line())) {
            embl_key_word(reader.line(), 0, key, TOKENSIZE);
            if (!str_equal(pattern, key)) break;

            // remove end-of-line, if there is any
            ind = Skip_white_space(reader.line(), p_nonkey_start);
            strcpy(temp, reader.line() + ind);
            skip_eolnl_and_append_spaced(Str, temp);
        }
    }
}

static void embl_one_entry(Reader& reader, char*& entry, char *key) {
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
    embl_key_word(reader.line(), 0, key, TOKENSIZE);
    if (str_equal(key, "DT")) {
        index = Skip_white_space(reader.line(), p_nonkey_start);
        freedup(embl.datec, reader.line() + index);
        // skip the rest of DT lines
        do {
            ++reader;
            if (!reader.line()) break;
            embl_key_word(reader.line(), 0, key, TOKENSIZE);
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

static int embl_comment_key(const char *line, char *key) {
    // Get the subkey_word in comment lines beginning at index.
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
    OrgInfo& orginf = embl.comments.orginf;
    SeqInfo& seqinf = embl.comments.seqinf;

    for (; is_embl_comment(reader.line()) ;) {
        char key[TOKENSIZE];
        int  index  = Skip_white_space(reader.line(), 5);
        int  offset = embl_comment_key(reader.line() + index, key);
        index       = Skip_white_space(reader.line(), index + offset);

        if      (str_equal(key, "Source of strain:"))            embl_one_comment_entry(orginf.source, index, reader);
        else if (str_equal(key, "Culture collection:"))          embl_one_comment_entry(orginf.cultcoll, index, reader);
        else if (str_equal(key, "Former name:"))                 embl_one_comment_entry(orginf.formname, index, reader);
        else if (str_equal(key, "Alternate name:"))              embl_one_comment_entry(orginf.nickname, index, reader);
        else if (str_equal(key, "Common name:"))                 embl_one_comment_entry(orginf.commname, index, reader);
        else if (str_equal(key, "Host organism:"))               embl_one_comment_entry(orginf.hostorg, index, reader);
        else if (str_equal(key, "RDP ID:"))                      embl_one_comment_entry(seqinf.RDPid, index, reader);
        else if (str_equal(key, "Corresponding GenBank entry:")) embl_one_comment_entry(seqinf.gbkentry, index, reader);
        else if (str_equal(key, "Sequencing methods:"))          embl_one_comment_entry(seqinf.methods, index, reader);
        else if (str_equal(key, "5' end complete:")) {
            scan_token_or_die(key, reader, index);
            if (key[0] == 'Y') seqinf.comp5 = 'y';
            else               seqinf.comp5 = 'n';
            ++reader;
        }
        else if (str_equal(key, "3' end complete:")) {
            scan_token_or_die(key, reader, index);
            if (key[0] == 'Y') seqinf.comp3 = 'y';
            else               seqinf.comp3 = 'n';
            ++reader;
        }
        else if (str_equal(key, "Sequence information ")) {
            seqinf.exists = true;
            ++reader;
        }
        else if (str_equal(key, "Organism information")) {
            orginf.exists = true;
            ++reader;
        }
        else {                  // other comments
            Append(embl.comments.others, reader.line() + 5);
            ++reader;
        }
    }
}

static void embl_skip_unidentified(char *pattern, Reader& reader) {
    // if there are (numb) blanks at the beginning of line,
    // it is a continue line of the current command.

    for (++reader; reader.line(); ++reader) {
        char  key[TOKENSIZE];
        embl_key_word(reader.line(), 0, key, TOKENSIZE);
        if (!str_equal(key, pattern)) break;
    }
}

void embl_in(Embl& embl, Seq& seq, Reader& reader) {
    // Read in one embl entry.

    EntryState state = ENTRY_NONE;

    for (; reader.line() && state != ENTRY_COMPLETED;) {
        if (!has_content(reader.line())) {
            ++reader;
            ca_assert(reader.ok());
            continue;           // empty line, skip
        }

        char key[TOKENSIZE];
        embl_key_word(reader.line(), 0, key, TOKENSIZE);
        state = ENTRY_STARTED;

        if (str_equal(key, "ID")) {
            embl_one_entry(reader, embl.id, key);
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
        else {                  // unidentified key word
            embl_skip_unidentified(key, reader);
        }
        /* except "ORIGIN", at the end of all the other cases,
         * a new line has already read in, so no further read
         * is necessary */

        ca_assert(reader.ok());
    }
    ca_assert(reader.ok());
    if (state == ENTRY_STARTED) throw_incomplete_entry();
    ++reader;
}

void embl_in_simple(Embl& embl, Seq& seq, Reader& reader) {
    // Read in one embl entry.
    char       key[TOKENSIZE];
    EntryState state = ENTRY_NONE;

    for (; reader.line() && state != ENTRY_COMPLETED;) {
        if (!has_content(reader.line())) { ++reader; continue; } // empty line, skip

        embl_key_word(reader.line(), 0, key, TOKENSIZE);
        state = ENTRY_STARTED;

        if (str_equal(key, "ID")) {
            embl_one_entry(reader, embl.id, key);
        }
        else if (str_equal(key, "SQ")) {
            embl_origin(seq, reader);
            state = ENTRY_COMPLETED;
        }
        else {                  // unidentified key word
            embl_skip_unidentified(key, reader);
        }
        /* except "ORIGIN", at the end of all the other cases,
           a new line has already read in, so no further read
           is necessary */
    }

    if (state == ENTRY_STARTED) throw_incomplete_entry();
    ++reader;
}
void embl_key_word(const char *line, int index, char *key, int length) { // @@@ similar to genbank_key_word and macke_key_word
    // Get the key_word from line beginning at index.
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

void embl_origin(Seq& seq, Reader& reader) {
    // Read in embl sequence data.
    ca_assert(seq.is_empty());

    // read in whole sequence data
    for (++reader;
         reader.line() && !is_sequence_terminator(reader.line());
         ++reader
         )
    {
        const char *line = reader.line();
        char ch;
        for (int idx = 5; (ch = line[idx]); ++idx) {
            if (ch == ' ' || ch == '\n') continue;
            seq.add(ch);
        }
    }

}
static void embl_print_lines(FILE * fp, const char *key, const char *content, const WrapMode& wrapMode) { // @@@ WRAPPER
    // Print EMBL entry and wrap around if line over EMBLMAXLINE.
    ca_assert(strlen(key) == 2);

    char prefix[TOKENSIZE];
    sprintf(prefix, "%-*s", EMBLINDENT, key);

    // wrapMode.print(fp, prefix, prefix, content, EMBLMAXLINE, true); // @@@ wanted
    wrapMode.print(fp, prefix, prefix, content, EMBLMAXLINE-1, WRAPBUG_WRAP_AT_SPACE); // @@@ 2 BUGs
}

static void embl_print_lines_if_content(FILE *fp, const char *key, const char *content, const WrapMode& wrapMode, bool followed_by_spacer) {
    if (has_content(content)) {
        embl_print_lines(fp, key, content, wrapMode);
        followed_by_spacer && fputs("XX\n", fp);
    }
}

static void embl_print_comment_if_content(FILE * fp, const char *key, const char *content) { // @@@ WRAPPER
    // Print one embl comment line, wrap around

    if (!has_content(content)) return;

    char first[LINESIZE]; sprintf(first, "CC%*s%s", (EMBLINDENT-2)+RDP_SUBKEY_INDENT, "", key);
    char other[LINESIZE]; sprintf(other, "CC%*s", (EMBLINDENT-2)+RDP_SUBKEY_INDENT+RDP_CONTINUED_INDENT, "");
    // WrapMode(true).print(fp, first, other, content, EMBLMAXLINE, false); // @@@ wanted
    WrapMode(true).print(fp, first, other, content, EMBLMAXLINE-2, WRAP_CORRECTLY); // @@@ 1 BUG
}

inline void embl_print_completeness(FILE *fp, char compX, char X) {
    if (compX == ' ') return;
    ca_assert(compX == 'y' || compX == 'n');
    fprintf(fp, "CC     %c' end complete: %s\n", X, compX == 'y' ? "Yes" : "No");
}

static void embl_out_comments(const Embl& embl, const Seq& seq, FILE * fp) {
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

static void embl_out_origin(const Seq& seq, FILE *fp) {
    // Print out the sequence data of EMBL format.
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
static void embl_out(const Embl& embl, const Seq& seq, FILE * fp) {
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
static void etog_convert_comments(const Embl& embl, GenBank& gbk) {
    // Convert comment part from EMBL to GenBank.

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
STATIC_ATTRIBUTED(__ATTR__USERESULT, int etog(const Embl& embl, GenBank& gbk, const Seq& seq)) {
    // Convert from embl to genbank format.
    int  indi;
    char key[TOKENSIZE], temp[LONGTEXT];
    char t1[TOKENSIZE], t2[TOKENSIZE], t3[TOKENSIZE];

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

STATIC_ATTRIBUTED(__ATTR__USERESULT, int etom(const Embl& embl, Macke& macke, const Seq& seq)) {
    // Convert from embl format to Macke format.
    GenBank gbk;
    return etog(embl, gbk, seq) && gtom(gbk, macke);
}

void embl_to_macke(const char *inf, const char *outf, Format inType) {
    // Convert from Embl format to Macke format.
    Reader reader(inf);
    FILE *ofp = open_output_or_die(outf);

    macke_out_header(ofp); // macke format sequence irrelevant header

    int total_num;
    for (int indi = 0; indi < 3; indi++) {
        reader.rewind();
        int numofseq = 0;

        while (1) {
            Embl embl;
            Seq  seq;
            embl_in(embl, seq, reader);
            if (reader.failed()) break;

            numofseq++;
            Macke macke;
            if (!etom(embl, macke, seq)) throw_conversion_failure(EMBL, MACKE);

            switch (indi) {
                case 0: macke_seq_display_out(macke, ofp, inType, numofseq == 1); break;
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
}

void embl_to_embl(const char *inf, const char *outf) {
    // Print out EMBL data.
    Reader reader(inf);
    FILE *ofp      = open_output_or_die(outf);
    int   numofseq = 0;

    while (1) {
        Embl embl;
        Seq  seq;
        embl_in(embl, seq, reader);
        if (reader.failed()) break;

        numofseq++;
        embl_out(embl, seq, ofp);
    }

    log_processed(numofseq);
    fclose(ofp);
}

void embl_to_genbank(const char *inf, const char *outf) {
    // Convert from EMBL format to genbank format.
    Reader reader(inf);
    FILE        *ofp      = open_output_or_die(outf);
    int          numofseq = 0;

    while (1) {
        Embl embl;
        Seq  seq;
        embl_in(embl, seq, reader);
        if (reader.failed()) break;

        numofseq++;
        GenBank gbk;
        if (!etog(embl, gbk, seq)) throw_conversion_failure(MACKE, GENBANK);
        genbank_out(gbk, seq, ofp);
    }

    log_processed(numofseq);
    fclose(ofp);
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

static void gtoe_comments(const GenBank& gbk, Embl& embl) {
    // Convert comment part from GenBank to EMBL.

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
STATIC_ATTRIBUTED(__ATTR__USERESULT, int gtoe(const GenBank& gbk, Embl& embl, const Seq& seq)) {
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
    gtoe_comments(gbk, embl);

    return (1);
}

void genbank_to_embl(const char *inf, const char *outf) {
    // Convert from genbank to EMBL.
    Reader  reader(inf);
    FILE   *ofp      = open_output_or_die(outf);
    int     numofseq = 0;

    while (1) {
        GenBank gbk;
        Seq     seq;

        genbank_in(gbk, seq, reader);
        if (reader.failed()) break;

        numofseq++;

        Embl embl;
        if (!gtoe(gbk, embl, seq)) throw_conversion_failure(GENBANK, EMBL);
        embl_out(embl, seq, ofp);

    }

    log_processed(numofseq);
    fclose(ofp);
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

STATIC_ATTRIBUTED(__ATTR__USERESULT, int mtoe(const Macke& macke, Embl& embl, const Seq& seq)) {
    GenBank gbk;
    return mtog(macke, gbk, seq) && gtoe(gbk, embl, seq) && partial_mtoe(macke, embl);
}

void macke_to_embl(const char *inf, const char *outf) {
    // Convert from macke to EMBL.
    FILE        *ofp      = open_output_or_die(outf);
    MackeReader  mackeReader(inf);
    int          numofseq = 0;

    while (1) {
        Macke  macke;
        Seq    seq;

        if (!mackeReader.read_one_entry(macke, seq)) break;
        
        Embl embl;
        numofseq++;

        /* partial_mtoe() is particularly handling
         * subspecies information, not converting whole
         * macke format to embl */

        if (!mtoe(macke, embl, seq)) throw_conversion_failure(MACKE, EMBL);
        embl_out(embl, seq, ofp);
    }

    log_processed(numofseq);
    fclose(ofp);
}

