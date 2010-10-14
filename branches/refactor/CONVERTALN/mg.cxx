// genbank and Macke converting program 

#include "genbank.h"
#include "macke.h"

extern int warning_out;

/* ----------------------------------------------------------
 *   Function genbank_to_macke().
 *   Convert from Genbank format to Macke format.
 */

void genbank_to_macke(const char *inf, const char *outf) {
    FILE        *IFP = open_input_or_die(inf);
    FILE_BUFFER  ifp = create_FILE_BUFFER(inf, IFP);
    FILE        *ofp = open_output_or_die(outf);

    int indi, total_num;

    // sequence irrelevant header
    macke_out_header(ofp);



    for (indi = 0; indi < 3; indi++) {
        FILE_BUFFER_rewind(ifp);

        int numofseq = 0;
        while (1) {
            GenBank gbk;
            Seq     seq;
            if (genbank_in(gbk, seq, ifp) == EOF) break;

            numofseq++;
            Macke macke;
            if (!gtom(gbk, macke)) throw_conversion_failure(GENBANK, MACKE);
            
            switch (indi) {
                case 0: macke_seq_display_out(macke, ofp, GENBANK, numofseq == 1); break;
                case 1: macke_seq_info_out(macke, ofp); break;
                case 2: macke_seq_data_out(seq, macke, ofp); break;
                default:;
            }
        }
        total_num = numofseq;
        if (indi == 0) {
            fputs("#-\n", ofp);
            // no warning message for next loop 
            warning_out = 0;
        }
    }

    warning_out = 1; // resume warning messages 
    log_processed(total_num);
    fclose(ofp);
    destroy_FILE_BUFFER(ifp);
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
            scan_token_or_die(gbk.accession, buffer, NULL);
            strcat(buffer, "\n");
        }
        else
            strcpy(buffer, "\n");
        freedup(macke.acs, buffer);
    }

    // copy the first reference from GenBank to Macke 
    if (gbk.get_refcount() > 0) {
        freedup_if_content(macke.author,  gbk.get_ref(0).author);
        freedup_if_content(macke.journal, gbk.get_ref(0).journal);
        freedup_if_content(macke.title,   gbk.get_ref(0).title);
    }
    // the rest of references are put into remarks, rem:..... 

    gtom_remarks(gbk, macke);

    // adjust the strain, subspecies, and atcc information 
    freeset(macke.strain,     genbank_get_strain(gbk));
    freeset(macke.subspecies, genbank_get_subspecies(gbk));
    if (str0len(macke.atcc) <= 1) {
        freeset(macke.atcc, genbank_get_atcc(gbk, macke));
    }

    return (1);
}

void add_35end_remark(Macke& macke, char end35, char yn) {
    if (yn == ' ') return;

    char *content = strf("%c' end complete:  %s\n", end35, yn == 'y' ? "Yes" : "No");
    macke.add_remark(content);
    free(content);
}

void gtom_remarks(const GenBank& gbk, Macke& macke) {
    // Create Macke remarks.
    int  len;
    int  indi, indj;
    char temp[LONGTEXT];

    // REFERENCE the first reference 
    if (gbk.get_refcount() > 0)
        macke.add_remark_if_content("ref:", gbk.get_ref(0).ref);

    // The rest of the REFERENCES 
    for (indi = 1; indi < gbk.get_refcount(); indi++) {
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
    if (str0len(gbk.comments.others) > 0) {
        len = str0len(gbk.comments.others);
        for (indi = 0, indj = 0; indi < len; indi++) {
            temp[indj++] = gbk.comments.others[indi];
            if (gbk.comments.others[indi] == '\n' || gbk.comments.others[indi] == '\0') {
                temp[indj]                 = '\0';
                macke.add_remark(temp);
                indj = 0;
            }
        }
    }
}

/* --------------------------------------------------------------------
 *   Function genbank_get_strain().
 *       Get strain from DEFINITION, COMMENT or SOURCE line in
 *       Genbank data file.
 */
char *genbank_get_strain(const GenBank& gbk) {
    int      indj, indk;
    char     strain[LONGTEXT], temp[LONGTEXT];

    strain[0] = '\0';
    // get strain
    if (has_content(gbk.comments.others)) {
        if ((indj = find_pattern(gbk.comments.others, "*source:")) >= 0) {
            if ((indk = find_pattern((gbk.comments.others + indj), "strain=")) >= 0) {
                // skip blank spaces 
                indj = Skip_white_space(gbk.comments.others, (indj + indk + 7));
                // get strain 
                get_string(gbk.comments.others, temp, indj);
                strcpy(strain, temp);   // copy new strain 
            }
        }
    }

    if (has_content(gbk.definition)) {
        if ((indj = find_pattern(gbk.definition, "str. ")) >= 0 || (indj = find_pattern(gbk.definition, "strain ")) >= 0) {
            // skip the key word 
            indj = Reach_white_space(gbk.definition, indj);
            // skip blank spaces 
            indj = Skip_white_space(gbk.definition, indj);
            // get strain 
            get_string(gbk.definition, temp, indj);
            if (has_content(strain)) {
                if (!str_equal(temp, strain)) {
                    warningf(91, "Inconsistent strain definition in DEFINITION: %s and %s", temp, strain);
                }
            }
            else
                strcpy(strain, temp);   // get strain 
        }
    }

    if (has_content(gbk.source)) {
        if ((indj = find_pattern(gbk.source, "str. ")) >= 0 || (indj = find_pattern(gbk.source, "strain ")) >= 0) {
            // skip the key word 
            indj = Reach_white_space(gbk.source, indj);
            // skip blank spaces 
            indj = Skip_white_space(gbk.source, indj);
            // get strain 
            get_string(gbk.source, temp, indj);
            if (has_content(strain)) {
                if (!str_equal(temp, strain)) {
                    warningf(92, "Inconsistent strain definition in SOURCE: %s and %s", temp, strain);
                }
            }
            else
                strcpy(strain, temp);
            // check consistency of duplicated def 
        }
    }

    return (nulldup(strain));
}

/* --------------------------------------------------------------------
 *   Function genbank_get_subspecies().
 *       Get subspecies information from SOURCE, DEFINITION, or
 *       COMMENT line of Genbank data file.
 */
char *genbank_get_subspecies(const GenBank& gbk) {
    int      indj, indk;
    char     subspecies[LONGTEXT], temp[LONGTEXT];

    subspecies[0] = '\0';
    // get subspecies
    if (has_content(gbk.definition)) {
        if ((indj = find_pattern(gbk.definition, "subsp. ")) >= 0) {
            // skip the key word 
            indj = Reach_white_space(gbk.definition, indj);
            // skip blank spaces 
            indj = Skip_white_space(gbk.definition, indj);
            // get subspecies 
            get_string(gbk.definition, temp, indj);
            correct_subspecies(temp);
            strcpy(subspecies, temp);
        }
    }
    if (has_content(gbk.comments.others)) {
        if ((indj = find_pattern(gbk.comments.others, "*source:")) >= 0) {
            if ((indk = find_pattern((gbk.comments.others + indj),
                                     "sub-species=")) >= 0
                || (indk = find_pattern((gbk.comments.others + indj),
                                        "subspecies=")) >= 0 || (indk = find_pattern((gbk.comments.others + indj), "subsp.=")) >= 0) {
                // skip the key word 
                for (indj += indk; gbk.comments.others[indj] != '='; indj++) {}
                indj++;
                // skip blank spaces 
                indj = Skip_white_space(gbk.comments.others, indj);
                // get subspecies 
                get_string(gbk.comments.others, temp, indj);
                if (has_content(subspecies)) {
                    if (!str_equal(temp, subspecies)) {
                        warningf(20, "Inconsistent subspecies definition in COMMENTS *source: %s and %s", temp, subspecies);
                    }
                }
                else
                    strcpy(subspecies, temp);
            }
        }
    }

    if (has_content(gbk.source)) {
        if ((indj = find_pattern(gbk.source, "subsp. ")) >= 0
            || (indj = find_pattern(gbk.source, "subspecies ")) >= 0 || (indj = find_pattern(gbk.source, "sub-species ")) >= 0) {
            // skip the key word 
            indj = Reach_white_space(gbk.source, indj);
            // skip blank spaces 
            indj = Skip_white_space(gbk.source, indj);
            // get subspecies 
            get_string(gbk.source, temp, indj);
            correct_subspecies(temp);
            if (has_content(subspecies)) {
                if (!str_equal(temp, subspecies)) {
                    warningf(21, "Inconsistent subspecies definition in SOURCE: %s and %s", temp, subspecies);
                }
            }
            else
                strcpy(subspecies, temp);
            // check consistency of duplicated def 
        }
    }

    return (nulldup(subspecies));
}

/* ---------------------------------------------------------------
 *   Function correct_subspecies().
 *       Remove the strain information in subspecies which is
 *       sometime mistakenly written into it.
 */
void correct_subspecies(char *subspecies) {
    int indj;

    if ((indj = find_pattern(subspecies, "str\n")) >= 0 || (indj = find_pattern(subspecies, "str.")) >= 0 || (indj = find_pattern(subspecies, "strain")) >= 0) {
        subspecies[indj - 1] = '\n';
        subspecies[indj] = '\0';
    }
}

char *genbank_get_atcc(const GenBank& gbk, const Macke& macke) {
    // Get atcc from SOURCE line in Genbank data file.
    char  temp[LONGTEXT];
    char *atcc;

    atcc = NULL;
    // get culture collection #
    if (has_content(gbk.source))
        atcc = get_atcc(macke, gbk.source);
    if (has_content(atcc) <= 1 && str0len(macke.strain)) {
        // add () to macke strain to be processed correctly 
        sprintf(temp, "(%s)", macke.strain);
        atcc = get_atcc(macke, temp);
    }
    return (atcc);
}

// ------------------------------------------------------------------- 
/*  Function get_atcc().
 */
char *get_atcc(const Macke& macke, char *source) {
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

// ----------------------------------------------------------------- 
/*  Function paren_string()
 */
int paren_string(char *Str, char *pstring, int index) {
    int pcount = 0, len, indi;

    for (indi = 0, len = str0len(Str); index < len; index++) {
        if (pcount >= 1)
            pstring[indi++] = Str[index];
        if (Str[index] == '(')
            pcount++;
        if (Str[index] == ')')
            pcount--;
    }
    if (indi == 0)
        return (-1);
    pstring[--indi] = '\0';
    return (index);
}

/* -----------------------------------------------------------------
 *   Function macke_to_genbank().
 *       Convert from macke format to genbank format.
 */
void macke_to_genbank(const char *inf, const char *outf) {
    MackeReader  mackeReader(inf);
    FILE        *ofp      = open_output_or_die(outf);
    int          numofseq = 0;

    while (1) {
        Macke macke;
        if (!mackeReader.mackeIn(macke)) break;

        SeqPtr seq = mackeReader.read_sequence_data();
        if (seq.isNull()) break;

        GenBank gbk;
        if (!mtog(macke, gbk, *seq)) throw_conversion_failure(MACKE, GENBANK);
        genbank_out(gbk, *seq, ofp);
        numofseq++;
    }

    log_processed(numofseq);
    fclose(ofp);
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
    if (has_content(gbk.comments.orginf.cultcoll) <= 1 && str0len(macke.atcc)) {
        freedup(gbk.comments.orginf.cultcoll, macke.atcc);
    }

    // define GenBank DEFINITION, after GenBank KEYWORD is defined. 
    mtog_genbank_def_and_source(macke, gbk);

    return (1);
}

/* ---------------------------------------------------------------
 *   Function mtog_decode_remarks().
 *       Decode remarks of Macke to GenBank format.
 */
void mtog_decode_ref_and_remarks(const Macke& macke, GenBank& gbk) {
    int acount, tcount, jcount, rcount, scount;

    ca_assert(gbk.get_refcount() == 0);
    acount = tcount = jcount = rcount = scount = 0;


    if (has_content(macke.author)) {
        ca_assert(acount == gbk.get_refcount());
        gbk.resize_refs(acount+1);
        freedup(gbk.get_ref(acount++).author, macke.author);
    }
    if (has_content(macke.journal)) {
        ca_assert(jcount <= gbk.get_refcount());
        jcount = gbk.get_refcount() - 1;
        freedup(gbk.get_ref(jcount++).journal, macke.journal);
    }
    if (has_content(macke.title)) {
        ca_assert(tcount <= gbk.get_refcount());
        tcount = gbk.get_refcount() - 1;
        freedup(gbk.get_ref(tcount++).title, macke.title);
    }

    for (int ridx = 0; ridx < macke.get_rem_count(); ridx++) {
        char key[TOKENSIZE];
        int offset = macke_key_word(macke.get_rem(ridx), 0, key, TOKENSIZE);

        if (str_equal(key, "ref")) {
            ca_assert(rcount == gbk.get_refcount() || (rcount == 0 && gbk.get_refcount() == 1)); // 2nd is "delayed" ref in comment

            if (rcount == gbk.get_refcount()) gbk.resize_refs(rcount+1);
            else rcount = gbk.get_refcount() - 1;

            freeset(gbk.get_ref(rcount++).ref, macke.copy_multi_rem(ridx, offset));
        }
        else if (str_equal(key, "auth")) {
            ca_assert(acount <= gbk.get_refcount());
            acount = gbk.get_refcount() - 1;
            freeset(gbk.get_ref(acount++).author, macke.copy_multi_rem(ridx, offset));
        }
        else if (str_equal(key, "title")) {
            ca_assert(tcount <= gbk.get_refcount());
            tcount = gbk.get_refcount() - 1;
            freeset(gbk.get_ref(tcount++).title, macke.copy_multi_rem(ridx, offset));
        }
        else if (str_equal(key, "jour")) {
            ca_assert(jcount <= gbk.get_refcount());
            jcount = gbk.get_refcount() - 1;
            freeset(gbk.get_ref(jcount++).journal, macke.copy_multi_rem(ridx, offset));
        }
        else if (str_equal(key, "standard")) {
            ca_assert(scount <= gbk.get_refcount());
            scount = gbk.get_refcount() - 1;
            freeset(gbk.get_ref(scount++).standard, macke.copy_multi_rem(ridx, offset));
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
            scan_token_or_die(macke.get_rem(ridx) + offset, key, NULL);
            gbk.comments.seqinf.comp3 = str_equal(key, "Yes") ? 'y' : 'n';
        }
        else if (str_equal(key, "5' end complete")) {
            gbk.comments.seqinf.exists = true;
            scan_token_or_die(macke.get_rem(ridx) + offset, key, NULL);
            gbk.comments.seqinf.comp5 = str_equal(key, "Yes") ? 'y' : 'n';
        }
        else { // other (non-interpreted) comments
            Append(gbk.comments.others, macke.get_rem(ridx));
        }
    }
}

/* ------------------------------------------------------------------
 *   Function mtog_genbank_def_and_source().
 *       Define GenBank DEFINITION and SOURCE lines the way RDP
 *           group likes.
 */
void mtog_genbank_def_and_source(const Macke& macke, GenBank& gbk) {
    freedup_if_content(gbk.definition, macke.name);
    if (has_content(macke.subspecies)) {
        if (str0len(gbk.definition) <= 1) {
            warning(22, "Genus and Species not defined");
            skip_eolnl_and_append(gbk.definition, "subsp. ");
        }
        else
            skip_eolnl_and_append(gbk.definition, " subsp. ");

        Append(gbk.definition, macke.subspecies);
    }

    if (has_content(macke.strain)) {
        if (str0len(gbk.definition) <= 1) {
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

/* -------------------------------------------------------------------
 *   Function get_string().
 *       Get the rest of the Str until reaching certain
 *           terminators, such as ';', ',', '.',...
 *       Always append "\n" at the end of the returned Str.
 */
void get_string(char *line, char *temp, int index) {
    int indk, len, paren_num;

    len = str0len(line);
    paren_num = 0;
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

/* -------------------------------------------------------------------
 *   Function get_atcc_string().
 *       Get the rest of the Str until reaching certain
 *           terminators, such as ';', ',', '.',...
 */
void get_atcc_string(char *line, char *temp, int index) {
    int indk, len, paren_num;

    len = str0len(line);
    paren_num = 0;
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
