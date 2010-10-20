// genbank and Macke converting program

#include "genbank.h"
#include "macke.h"

extern int warning_out;

void genbank_to_macke(const char *inf, const char *outf) {
    // Convert from Genbank format to Macke format.
    Format     inType = GENBANK;
    Reader     reader(inf);
    FileWriter write(outf);

    int indi;

    // sequence irrelevant header
    macke_out_header(write);

    for (indi = 0; indi < 3; indi++) {
        reader.rewind();

        int numofseq = 0;
        while (1) {
            GenBank gbk;
            Seq     seq;
            genbank_in(gbk, seq, reader);
            if (reader.failed()) break;

            numofseq++;
            Macke macke;
            if (!gtom(gbk, macke)) throw_conversion_failure(inType, MACKE);

            switch (indi) {
                case 0: macke_seq_display_out(macke, write, inType, numofseq == 1); break;
                case 1: macke_seq_info_out(macke, write); break;
                case 2:
                    macke_seq_data_out(seq, macke, write);
                    write.seq_done();
                    break;
                default:;
            }
        }
        if (indi == 0) {
            write.out("#-\n");
            warning_out = 0; // no warning message for next loop
        }
    }

    warning_out = 1; // resume warning messages
}

static int paren_string(char *line, char *pstring, int index) {
    int len       = str0len(line);
    int paren_num = 0;
    int indk;

    for (indk = 0; index < len; index++) {
        if (paren_num >= 1)
            pstring[indk++] = line[index];
        if (line[index] == '(')
            paren_num++;
        if (line[index] == ')')
            paren_num--;
    }
    if (indk == 0)
        return (-1);
    pstring[--indk] = '\0';
    return (index);
}

static void get_atcc_string(const char *line, char *temp, int index) {
    // Get the rest of the line until reaching certain terminators, such as ';', ',', '.',...

    int len       = str0len(line);
    int paren_num = 0;
    int indk;

    for (indk = 0; index < len; index++, indk++) {
        temp[indk] = line[index];
        if (temp[indk] == '(')
            paren_num++;
        if (temp[indk] == ')')
            if (paren_num == 0)
                break;
            else
                paren_num--;
        else if (paren_num == 0 && (temp[indk] == ';' || temp[indk] == '.' || temp[indk] == ',' || temp[indk] == '/' || temp[indk] == '\n'))
            break;
    }
    temp[indk] = '\0';
}

static char *get_atcc(const Macke& macke, char *source) {
    static int cc_num = 16;
    static const char *CC[16] = {
        "ATCC", "CCM", "CDC", "CIP", "CNCTC",
        "DSM", "EPA", "JCM", "NADC", "NCDO", "NCTC", "NRCC",
        "NRRL", "PCC", "USDA", "VPI"
    };

    int  indi, indj, index;
    int  length;
    char buffer[LONGTEXT], temp[LONGTEXT], pstring[LONGTEXT];
    char atcc[LONGTEXT];

    atcc[0] = '\0';
    for (indi = 0; indi < cc_num; indi++) {
        index = 0;
        while ((index = paren_string(source, pstring, index)) > 0) {
            if ((indj = find_pattern(pstring, CC[indi])) >= 0) {
                // skip the key word
                indj += str0len(CC[indi]);
                // skip blank spaces
                indj = Skip_white_space(pstring, indj);
                // get strain
                get_atcc_string(pstring, buffer, indj);
                sprintf(temp, "%s %s", CC[indi], buffer);
                length = str0len(atcc);
                if (length > 0) {
                    atcc[length] = '\0';
                    strcat(atcc, ", ");
                }
                strcat(atcc, temp);
            }
        }
    }
    // append eoln to the atcc string
    length = str0len(atcc);
    if (macke.atcc) {
        macke.atcc[length] = '\0';
    }
    strcat(atcc, "\n");
    return (nulldup(atcc));
}

static char *genbank_get_atcc(const GenBank& gbk, const Macke& macke) {
    // Get atcc from SOURCE line in Genbank data file.
    char  temp[LONGTEXT];
    char *atcc;

    atcc = NULL;
    // get culture collection #
    if (has_content(gbk.source)) {
        atcc = get_atcc(macke, gbk.source);
    }
    if (!has_content(atcc) && has_content(macke.strain)) {
        // add () to macke strain to be processed correctly
        sprintf(temp, "(%s)", macke.strain);
        atcc = get_atcc(macke, temp);
    }
    return atcc;
}

static void add_35end_remark(Macke& macke, char end35, char yn) {
    if (yn == ' ') return;

    char *content = strf("%c' end complete:  %s\n", end35, yn == 'y' ? "Yes" : "No");
    macke.add_remark(content);
    free(content);
}

static void gtom_remarks(const GenBank& gbk, Macke& macke) {
    // Create Macke remarks.

    // REFERENCE the first reference
    if (gbk.has_refs())
        macke.add_remark_if_content("ref:", gbk.get_ref(0).ref);

    // The rest of the REFERENCES
    for (int indi = 1; indi < gbk.get_refcount(); indi++) {
        macke.add_remark_if_content("ref:",      gbk.get_ref(indi).ref);
        macke.add_remark_if_content("auth:",     gbk.get_ref(indi).author);
        macke.add_remark_if_content("jour:",     gbk.get_ref(indi).journal);
        macke.add_remark_if_content("title:",    gbk.get_ref(indi).title);
        macke.add_remark_if_content("standard:", gbk.get_ref(indi).standard);
    }

    const OrgInfo& orginf = gbk.comments.orginf;
    const SeqInfo& seqinf = gbk.comments.seqinf;

    macke.add_remark_if_content("KEYWORDS:",           gbk.keywords);        // copy keywords as remark
    macke.add_remark_if_content("GenBank ACCESSION:",  gbk.accession);       // copy accession as remark when genbank entry also exists.
    macke.add_remark_if_content("Source of strain:",   orginf.source);       // copy source of strain
    macke.add_remark_if_content("Former name:",        orginf.formname);     // copy former name
    macke.add_remark_if_content("Alternate name:",     orginf.nickname);     // copy alternate name
    macke.add_remark_if_content("Common name:",        orginf.commname);     // copy common name
    macke.add_remark_if_content("Host organism:",      orginf.hostorg);      // copy host organism
    macke.add_remark_if_content("RDP ID:",             seqinf.RDPid);        // copy RDP ID
    macke.add_remark_if_content("Sequencing methods:", seqinf.methods);      // copy methods

    add_35end_remark(macke, '3', seqinf.comp3);
    add_35end_remark(macke, '5', seqinf.comp5);

    // other comments, not RDP DataBase specially defined
    int len = str0len(gbk.comments.others);
    if (len > 0) {
        for (int indi = 0, indj = 0; indi < len; indi++) {
            char temp[LONGTEXT];
            temp[indj++] = gbk.comments.others[indi];
            if (gbk.comments.others[indi] == '\n' || gbk.comments.others[indi] == '\0') {
                temp[indj]                 = '\0';
                macke.add_remark(temp);
                indj = 0;
            }
        }
    }
}

static void correct_subspecies(char *subspecies) {
    // Remove the strain information in subspecies which is sometime mistakenly written into it.
    int indj;

    if ((indj = find_pattern(subspecies, "str\n")) >= 0 || (indj = find_strain(subspecies, 0)) >= 0) {
        ca_assert(subspecies[indj-1] == ' '); // assume to overwrite a space
        subspecies[indj - 1] = '\n';
        subspecies[indj]     = '\0';
    }
}

static void check_consistency(const char *what, char* const& var, const char *New) {
    if (has_content(var)) {
        if (!str_equal(var, New)) {
            warningf(20, "Inconsistent %s definitions detected:\n"
                     "    %s"
                     "and %s", what, var, New);
        }
    }
    else {
        strcpy(var, New);
    }
}

static void get_string(char *temp, const char *line, int index) {
    // Get the rest of the line until reaching certain terminators,
    // such as ';', ',', '.',...
    // Always append "\n" at the end of the result.

    index = Skip_white_space(line, index);

    int len       = str0len(line);
    int paren_num = 0;
    int indk;

    for (indk = 0; index < len; index++, indk++) {
        temp[indk] = line[index];
        if (temp[indk] == '(')
            paren_num++;
        if (temp[indk] == ')')
            if (paren_num == 0)
                break;
            else
                paren_num--;
        else if (temp[indk] == '\n' || (paren_num == 0 && temp[indk] == ';'))
            break;
    }
    if (indk > 1 && is_end_mark(temp[indk - 1]))
        indk--;
    temp[indk++] = '\n';
    temp[indk] = '\0';
}

static void copy_subspecies_and_check_consistency(char* const& subspecies, const char *from, int indj) {
    char temp[LONGTEXT];
    get_string(temp, from, indj);
    correct_subspecies(temp);
    check_consistency("subspecies", subspecies, temp);
}
static void copy_strain_and_check_consistency(char* const& strain, const char *from, int indj) {
    char temp[LONGTEXT];
    get_string(temp, from, indj);
    check_consistency("strain", strain, temp);
}

static void check_strain_from(char* const& strain, const char *from) {
    if (has_content(from)) {
        int indj = skip_strain(from, ' ');
        if (indj >= 0) copy_strain_and_check_consistency(strain, from, indj);
    }
}

static char *genbank_get_strain(const GenBank& gbk) {
    // Get strain from DEFINITION, COMMENT or SOURCE line in Genbank data file.
    char strain[LONGTEXT];

    strain[0] = '\0';

    if (has_content(gbk.comments.others)) {
        int indj = find_pattern(gbk.comments.others, "*source:");
        if (indj >= 0) {
            int indk = skip_pattern(gbk.comments.others + indj, "strain=");
            if (indk >= 0) copy_strain_and_check_consistency(strain, gbk.comments.others, indj+indk);
        }
    }

    check_strain_from(strain, gbk.definition);
    check_strain_from(strain, gbk.source);

    return nulldup(strain);
}

static char *genbank_get_subspecies(const GenBank& gbk) {
    // Get subspecies information from SOURCE, DEFINITION, or COMMENT line of Genbank data file.
    int  indj;
    char subspecies[LONGTEXT];

    subspecies[0] = '\0';

    if (has_content(gbk.definition)) {
        if ((indj = skip_pattern(gbk.definition, "subsp. ")) >= 0) {
            copy_subspecies_and_check_consistency(subspecies, gbk.definition, indj);
        }
    }
    if (has_content(gbk.comments.others)) {
        if ((indj = find_pattern(gbk.comments.others, "*source:")) >= 0) {
            int indk = skip_subspecies(gbk.comments.others + indj, '=');
            if (indk >= 0) {
                copy_subspecies_and_check_consistency(subspecies, gbk.comments.others, indj+indk);
            }
        }
    }

    if (has_content(gbk.source)) {
        if ((indj = skip_subspecies(gbk.source, ' ')) >= 0) {
            copy_subspecies_and_check_consistency(subspecies, gbk.source, indj);
        }
    }

    return nulldup(subspecies);
}

void macke_to_genbank(const char *inf, const char *outf) {
    // Convert from macke format to genbank format.
    MackeReader mackeReader(inf);
    FileWriter  write(outf);

    while (1) {
        Macke   macke;
        Seq     seq;

        if (!mackeReader.read_one_entry(macke, seq)) break;

        GenBank gbk;
        if (!mtog(macke, gbk, seq)) throw_conversion_failure(MACKE, GENBANK);
        genbank_out(gbk, seq, write);
        write.seq_done();
    }
}

static void mtog_decode_ref_and_remarks(const Macke& macke, GenBank& gbk) {
    // Decode remarks of Macke to GenBank format.
    ca_assert(gbk.get_refcount() == 0);

    if (has_content(macke.author))  freedup(gbk.get_new_ref().author, macke.author);
    if (has_content(macke.journal)) freedup(gbk.get_latest_ref().journal, macke.journal);
    if (has_content(macke.title))   freedup(gbk.get_latest_ref().title, macke.title);

    bool first_ref = true;
    for (int ridx = 0; ridx < macke.get_rem_count(); ridx++) {
        char key[TOKENSIZE];
        int offset = macke_key_word(macke.get_rem(ridx), 0, key, TOKENSIZE);

        if (str_equal(key, "ref")) {
            GenbankRef& ref = first_ref ? gbk.get_latest_ref() : gbk.get_new_ref();
            freeset(ref.ref, macke.copy_multi_rem(ridx, offset));
            first_ref = false;
        }
        else if (str_equal(key, "auth")) {
            freeset(gbk.get_latest_ref().author, macke.copy_multi_rem(ridx, offset));
        }
        else if (str_equal(key, "title")) {
            freeset(gbk.get_latest_ref().title, macke.copy_multi_rem(ridx, offset));
        }
        else if (str_equal(key, "jour")) {
            freeset(gbk.get_latest_ref().journal, macke.copy_multi_rem(ridx, offset));
        }
        else if (str_equal(key, "standard")) {
            freeset(gbk.get_latest_ref().standard, macke.copy_multi_rem(ridx, offset));
        }
        else if (str_equal(key, "KEYWORDS")) {
            freeset(gbk.keywords, macke.copy_multi_rem(ridx, offset));
            terminate_with(gbk.keywords, '.');
        }
        else if (str_equal(key, "GenBank ACCESSION")) {
            freeset(gbk.accession, macke.copy_multi_rem(ridx, offset));
        }
        else if (str_equal(key, "Source of strain")) {
            gbk.comments.orginf.exists = true;
            freeset(gbk.comments.orginf.source, macke.copy_multi_rem(ridx, offset));
        }
        else if (str_equal(key, "Former name")) {
            gbk.comments.orginf.exists = true;
            freeset(gbk.comments.orginf.formname, macke.copy_multi_rem(ridx, offset));
        }
        else if (str_equal(key, "Alternate name")) {
            gbk.comments.orginf.exists = true;
            freeset(gbk.comments.orginf.nickname, macke.copy_multi_rem(ridx, offset));
        }
        else if (str_equal(key, "Common name")) {
            gbk.comments.orginf.exists = true;
            freeset(gbk.comments.orginf.commname, macke.copy_multi_rem(ridx, offset));
        }
        else if (str_equal(key, "Host organism")) {
            gbk.comments.orginf.exists = true;
            freeset(gbk.comments.orginf.hostorg, macke.copy_multi_rem(ridx, offset));
        }
        else if (str_equal(key, "RDP ID")) {
            gbk.comments.seqinf.exists = true;
            freeset(gbk.comments.seqinf.RDPid, macke.copy_multi_rem(ridx, offset));
        }
        else if (str_equal(key, "Sequencing methods")) {
            gbk.comments.seqinf.exists = true;
            freeset(gbk.comments.seqinf.methods, macke.copy_multi_rem(ridx, offset));
        }
        else if (str_equal(key, "3' end complete")) {
            gbk.comments.seqinf.exists = true;
            scan_token_or_die(key, macke.get_rem(ridx) + offset);
            gbk.comments.seqinf.comp3 = str_equal(key, "Yes") ? 'y' : 'n';
        }
        else if (str_equal(key, "5' end complete")) {
            gbk.comments.seqinf.exists = true;
            scan_token_or_die(key, macke.get_rem(ridx) + offset);
            gbk.comments.seqinf.comp5 = str_equal(key, "Yes") ? 'y' : 'n';
        }
        else { // other (non-interpreted) comments
            Append(gbk.comments.others, macke.get_rem(ridx));
        }
    }
}

static void mtog_genbank_def_and_source(const Macke& macke, GenBank& gbk) {
    // Define GenBank DEFINITION and SOURCE lines the way RDP group likes.
    freedup_if_content(gbk.definition, macke.name);
    if (has_content(macke.subspecies)) {
        if (!has_content(gbk.definition)) {
            warning(22, "Genus and Species not defined");
            skip_eolnl_and_append(gbk.definition, "subsp. ");
        }
        else
            skip_eolnl_and_append(gbk.definition, " subsp. ");

        Append(gbk.definition, macke.subspecies);
    }

    if (has_content(macke.strain)) {
        if (!has_content(gbk.definition)) {
            warning(23, "Genus and Species and Subspecies not defined");
            skip_eolnl_and_append(gbk.definition, "str. ");
        }
        else
            skip_eolnl_and_append(gbk.definition, " str. ");

        Append(gbk.definition, macke.strain);
    }

    // create SOURCE line, temp.
    if (has_content(gbk.definition)) {
        freedup(gbk.source, gbk.definition);
        terminate_with(gbk.source, '.');
    }

    // append keyword to definition, if there is keyword.
    if (has_content(gbk.keywords)) {
        if (has_content(gbk.definition))
            skip_eolnl_and_append(gbk.definition, "; \n");

        // Here keywords must be ended by a '.' already
        skip_eolnl_and_append(gbk.definition, gbk.keywords);
    }
    else
        skip_eolnl_and_append(gbk.definition, ".\n");
}

int mtog(const Macke& macke, GenBank& gbk, const Seq& seq) { // __ATTR__USERESULT
    // Convert Macke format to Genbank format.
    int  indi;
    char temp[LONGTEXT];

    strcpy(temp, macke.seqabbr);

    for (indi = str0len(temp); indi < 13; temp[indi++] = ' ') {}

    if (has_content(macke.date))
        sprintf((temp + 10), "%7d bp    RNA             RNA       %s\n", seq.get_len(), genbank_date(macke.date));
    else
        sprintf((temp + 10), "%7d bp    RNA             RNA       %s\n", seq.get_len(), genbank_date(today_date()));

    freedup(gbk.locus, temp);

    // GenBank ORGANISM
    if (has_content(macke.name)) {
        freedup(gbk.organism, macke.name);

        // append a '.' at the end
        terminate_with(gbk.organism, '.');
    }

    if (has_content(macke.rna)) {
        gbk.comments.seqinf.exists = true;
        freedup(gbk.comments.seqinf.methods, macke.rna);
    }
    if (has_content(macke.acs)) {
        /* #### not converted to accession but to comment gbkentry only, temporarily
           freenull(gbk.accession);
           gbk.accession = nulldup(macke.acs);
         */
        gbk.comments.seqinf.exists = true;
        freedup(gbk.comments.seqinf.gbkentry, macke.acs);
    }
    else if (has_content(macke.nbk)) {
        /* #### not converted to accession but to comment gbkentry only, temp
           freenull(gbk.accession);
           gbk.accession = nulldup(macke.nbk);
         */
        gbk.comments.seqinf.exists = true;
        freedup(gbk.comments.seqinf.gbkentry, macke.nbk);
    }
    if (has_content(macke.atcc)) {
        gbk.comments.orginf.exists = true;
        freedup(gbk.comments.orginf.cultcoll, macke.atcc);
    }
    mtog_decode_ref_and_remarks(macke, gbk);
    // final conversion of cultcoll
    if (!has_content(gbk.comments.orginf.cultcoll) && has_content(macke.atcc)) {
        freedup(gbk.comments.orginf.cultcoll, macke.atcc);
    }

    // define GenBank DEFINITION, after GenBank KEYWORD is defined.
    mtog_genbank_def_and_source(macke, gbk);

    return (1);
}

int gtom(const GenBank& gbk, Macke& macke) { // __ATTR__USERESULT
    // Convert from Genbank format to Macke format.
    char temp[LONGTEXT], buffer[TOKENSIZE];
    char genus[TOKENSIZE], species[TOKENSIZE];

    // copy sequence abbr, assume every entry in gbk must end with \n\0
    // no '\n' at the end of the string
    genbank_key_word(gbk.locus, 0, temp, TOKENSIZE);
    freedup(macke.seqabbr, temp);

    // copy name and definition
    if (has_content(gbk.organism)) {
        freedup(macke.name, gbk.organism);
    }
    else if (has_content(gbk.definition)) {
        ASSERT_RESULT(int, 2, sscanf(gbk.definition, "%s %s", genus, species));

        int last = str0len(species)-1;
        if (species[last] == ';') species[last] = '\0';

        freeset(macke.name, strf("%s %s\n", genus, species));

    }

    freedup_if_content(macke.atcc, gbk.comments.orginf.cultcoll); // copy cultcoll name and number
    freedup_if_content(macke.rna,  gbk.comments.seqinf.methods); // copy rna(methods)

    freeset(macke.date, gbk.get_date()); Append(macke.date, "\n");

    // copy genbank entry  (gbkentry has higher priority than gbk.accession)
    if (has_content(gbk.comments.seqinf.gbkentry))
        freedup(macke.acs, gbk.comments.seqinf.gbkentry);
    else {
        if (has_content(gbk.accession) && !str_equal(gbk.accession, "No information\n")) {
            scan_token_or_die(buffer, gbk.accession);
            strcat(buffer, "\n");
        }
        else
            strcpy(buffer, "\n");
        freedup(macke.acs, buffer);
    }

    // copy the first reference from GenBank to Macke
    if (gbk.has_refs()) {
        freedup_if_content(macke.author,  gbk.get_ref(0).author);
        freedup_if_content(macke.journal, gbk.get_ref(0).journal);
        freedup_if_content(macke.title,   gbk.get_ref(0).title);
    }
    // the rest of references are put into remarks, rem:.....

    gtom_remarks(gbk, macke);

    // adjust the strain, subspecies, and atcc information
    freeset(macke.strain,     genbank_get_strain(gbk));
    freeset(macke.subspecies, genbank_get_subspecies(gbk));
    if (!has_content(macke.atcc)) {
        freeset(macke.atcc, genbank_get_atcc(gbk, macke));
    }

    return (1);
}
