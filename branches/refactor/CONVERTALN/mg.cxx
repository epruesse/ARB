/* genbank and Macke converting program */

#include <stdio.h>
#include <stdlib.h>
#include "convert.h"
#include "global.h"
#include "macke.h"

extern int warning_out;

/* ----------------------------------------------------------
 *   Function genbank_to_macke().
 *   Convert from Genbank format to Macke format.
 */

inline void reinit_macke_genbank() { reinit_macke(); reinit_genbank(); }
 
void genbank_to_macke(const char *inf, const char *outf) {
    FILE        *IFP = open_input_or_die(inf);
    FILE_BUFFER  ifp = create_FILE_BUFFER(inf, IFP);
    FILE        *ofp = open_output_or_die(outf);

    int indi, total_num;

    /* seq irrelevant header */
    macke_out_header(ofp);

    for (indi = 0; indi < 3; indi++) {
        FILE_BUFFER_rewind(ifp);
        reset_seq_data();
        while (genbank_in(ifp) != EOF) {
            data.numofseq++;
            if (gtom()) {
                /* convert from genbank form to macke form */
                switch (indi) {
                    case 0:
                        /* output seq display format */
                        macke_out0(ofp, GENBANK);
                        break;
                    case 1:
                        /* output seq information */
                        macke_out1(ofp);
                        break;
                    case 2:
                        /* output seq data */
                        macke_out2(ofp);
                        break;
                    default:;
                }
            }
            else throw_error(7, "Conversion from genbank to macke fails");

            reinit_macke_genbank();
        }
        total_num = data.numofseq;
        if (indi == 0) {
            fputs("#-\n", ofp);
            /* no warning message for next loop */
            warning_out = 0;
        }
    }

    warning_out = 1; /* resume warning messages */
    log_processed(total_num);
    fclose(ofp);
    destroy_FILE_BUFFER(ifp);
}

/* --------------------------------------------------------------
 *   Function gtom().
 *       Convert from Genbank format to Macke format.
 */
int gtom() {
    // Responsibility note: caller has to cleanup genbank and macke data
    
    char temp[LONGTEXT], buffer[TOKENSIZE];
    char genus[TOKENSIZE], species[TOKENSIZE];

    /* copy seq abbr, assume every entry in gbk must end with \n\0 */
    /* no '\n' at the end of the string */
    genbank_key_word(data.gbk.locus, 0, temp, TOKENSIZE);
    freedup(data.macke.seqabbr, temp);

    /* copy name and definition */
    if (has_content(data.gbk.organism)) {
        freedup(data.macke.name, data.gbk.organism);
    }
    else if (has_content(data.gbk.definition)) {
        ASSERT_RESULT(int, 2, sscanf(data.gbk.definition, "%s %s", genus, species));
        if (species[str0len(species) - 1] == ';')
            species[str0len(species) - 1] = '\0';
        sprintf(temp, "%s %s\n", genus, species);
        freedup(data.macke.name, temp);
    }

    freedup_if_content(data.macke.atcc, data.gbk.comments.orginf.cc); /* copy cc name and number */
    freedup_if_content(data.macke.rna,  data.gbk.comments.seqinf.methods); /* copy rna(methods) */
    if (str0len(data.gbk.locus) < 61) {
        freedup(data.macke.date, genbank_date(today_date())); /* copy date---DD-MMM-YYYY\n\0  */
        Append(data.macke.date, "\n");
    }
    else
        freedup(data.macke.date, data.gbk.locus + 50);

    /* copy genbank entry  (gbkentry has higher priority than gbk.accession) */
    if (has_content(data.gbk.comments.seqinf.gbkentry))
        freedup(data.macke.acs, data.gbk.comments.seqinf.gbkentry);
    else {
        if (has_content(data.gbk.accession) && !str_equal(data.gbk.accession, "No information\n")) {
            scan_token_or_die(data.gbk.accession, buffer, NULL);
            strcat(buffer, "\n");
        }
        else
            strcpy(buffer, "\n");
        freedup(data.macke.acs, buffer);
    }

    /* copy the first reference from GenBank to Macke */
    if (data.gbk.numofref > 0) {
        freedup_if_content(data.macke.author,  data.gbk.reference[0].author);
        freedup_if_content(data.macke.journal, data.gbk.reference[0].journal);
        freedup_if_content(data.macke.title,   data.gbk.reference[0].title);
    }                           /* the rest of references are put into remarks, rem:..... */

    gtom_remarks();

    /* adjust the strain, subspecies, and atcc information */
    freeset(data.macke.strain,     genbank_get_strain());
    freeset(data.macke.subspecies, genbank_get_subspecies());
    if (str0len(data.macke.atcc) <= 1) {
        freeset(data.macke.atcc, genbank_get_atcc());
    }

    return (1);
}

/* --------------------------------------------------------------
 *   Function gtom_remarks().
 *       Create Macke remarks.
 */
void gtom_remarks() {
    int  remnum, len;
    int  indi, indj;
    char temp[LONGTEXT];

    /* remarks in Macke format */
    remnum = num_of_remark();
    data.macke.remarks = (char **)calloc(1, (unsigned)(sizeof(char *) * remnum));
    remnum = 0;

    /* REFERENCE the first reference */
    if (data.gbk.numofref > 0)
        gtom_copy_remark(data.gbk.reference[0].ref, "ref:", &remnum);

    /* The rest of the REFERENCES */
    for (indi = 1; indi < data.gbk.numofref; indi++) {
        gtom_copy_remark(data.gbk.reference[indi].ref, "ref:", &remnum);
        gtom_copy_remark(data.gbk.reference[indi].author, "auth:", &remnum);
        gtom_copy_remark(data.gbk.reference[indi].journal, "jour:", &remnum);
        gtom_copy_remark(data.gbk.reference[indi].title, "title:", &remnum);
        gtom_copy_remark(data.gbk.reference[indi].standard, "standard:", &remnum);
    }                           /* loop for copying other reference */

    /* copy keywords as remark */
    gtom_copy_remark(data.gbk.keywords, "KEYWORDS:", &remnum);

    /* copy accession as remark when genbank entry also exists. */
    gtom_copy_remark(data.gbk.accession, "GenBank ACCESSION:", &remnum);

    /* copy source of strain */
    gtom_copy_remark(data.gbk.comments.orginf.source, "Source of strain:", &remnum);

    /* copy former name */
    gtom_copy_remark(data.gbk.comments.orginf.formname, "Former name:", &remnum);

    /* copy alternate name */
    gtom_copy_remark(data.gbk.comments.orginf.nickname, "Alternate name:", &remnum);

    /* copy common name */
    gtom_copy_remark(data.gbk.comments.orginf.commname, "Common name:", &remnum);

    /* copy host organism */
    gtom_copy_remark(data.gbk.comments.orginf.hostorg, "Host organism:", &remnum);

    /* copy RDP ID */
    gtom_copy_remark(data.gbk.comments.seqinf.RDPid, "RDP ID:", &remnum);

    /* copy methods */
    gtom_copy_remark(data.gbk.comments.seqinf.methods, "Sequencing methods:", &remnum);

    /* copy 3' end complete */
    if (data.gbk.comments.seqinf.comp3 != ' ') {
        if (data.gbk.comments.seqinf.comp3 == 'y')
            data.macke.remarks[remnum++] = strdup("3' end complete:  Yes\n");
        else
            data.macke.remarks[remnum++] = strdup("3' end complete:  No\n");
    }

    /* copy 5' end complete */
    if (data.gbk.comments.seqinf.comp5 != ' ') {
        if (data.gbk.comments.seqinf.comp5 == 'y')
            data.macke.remarks[remnum++] = strdup("5' end complete:  Yes\n");
        else
            data.macke.remarks[remnum++] = strdup("5' end complete:  No\n");
    }

    /* other comments, not RDP DataBase specially defined */
    if (str0len(data.gbk.comments.others) > 0) {
        len = str0len(data.gbk.comments.others);
        for (indi = 0, indj = 0; indi < len; indi++) {
            temp[indj++] = data.gbk.comments.others[indi];
            if (data.gbk.comments.others[indi] == '\n' || data.gbk.comments.others[indi] == '\0') {
                temp[indj] = '\0';
                data.macke.remarks[remnum++] = nulldup(temp);

                indj = 0;
            }                   /* new remark line */
        }                       /* for loop to find other remarks */
    }                           /* other comments */

    /* done with the remarks copying */

    data.macke.numofrem = remnum;
}

/* --------------------------------------------------------------------
 *  Function gtom_copy_remark().
 *      If Str length > 1 then copy Str with key to remark.
 */
void gtom_copy_remark(char *Str, const char *key, int *remnum) { // @@@ remnum -> ref
    /* copy host organism */
    if (has_content(Str)) {
        data.macke.remarks[(*remnum)] = nulldup(key);
        Append(data.macke.remarks[(*remnum)], Str);
        (*remnum)++;
    }
}

/* --------------------------------------------------------------------
 *   Function genbank_get_strain().
 *       Get strain from DEFINITION, COMMENT or SOURCE line in
 *       Genbank data file.
 */
char *genbank_get_strain() {
    int  indj, indk;
    char strain[LONGTEXT], temp[LONGTEXT];

    strain[0] = '\0';
    /* get strain */
    if (has_content(data.gbk.comments.others)) {
        if ((indj = find_pattern(data.gbk.comments.others, "*source:")) >= 0) {
            if ((indk = find_pattern((data.gbk.comments.others + indj), "strain=")) >= 0) {
                /* skip blank spaces */
                indj = Skip_white_space(data.gbk.comments.others, (indj + indk + 7));
                /* get strain */
                get_string(data.gbk.comments.others, temp, indj);
                strcpy(strain, temp);   /* copy new strain */
            }                   /* get strain */
        }                       /* find source: line in comment */
    }                           /* look for strain on comments */

    if (has_content(data.gbk.definition)) {
        if ((indj = find_pattern(data.gbk.definition, "str. ")) >= 0 || (indj = find_pattern(data.gbk.definition, "strain ")) >= 0) {
            /* skip the key word */
            indj = Reach_white_space(data.gbk.definition, indj);
            /* skip blank spaces */
            indj = Skip_white_space(data.gbk.definition, indj);
            /* get strain */
            get_string(data.gbk.definition, temp, indj);
            if (has_content(strain)) {
                if (!str_equal(temp, strain)) {
                    warningf(91, "Inconsistent strain definition in DEFINITION: %s and %s", temp, strain);
                }               /* check consistency of duplicated def */
            }
            else
                strcpy(strain, temp);   /* get strain */
        }                       /* find strain in definition */
    }                           /* if there is definition line */

    if (has_content(data.gbk.source)) {
        if ((indj = find_pattern(data.gbk.source, "str. ")) >= 0 || (indj = find_pattern(data.gbk.source, "strain ")) >= 0) {
            /* skip the key word */
            indj = Reach_white_space(data.gbk.source, indj);
            /* skip blank spaces */
            indj = Skip_white_space(data.gbk.source, indj);
            /* get strain */
            get_string(data.gbk.source, temp, indj);
            if (has_content(strain)) {
                if (!str_equal(temp, strain)) {
                    warningf(92, "Inconsistent strain definition in SOURCE: %s and %s", temp, strain);
                }
            }
            else
                strcpy(strain, temp);
            /* check consistency of duplicated def */
        }                       /* find strain */
    }                           /* look for strain in SOURCE line */

    return (nulldup(strain));
}

/* --------------------------------------------------------------------
 *   Function genbank_get_subspecies().
 *       Get subspecies information from SOURCE, DEFINITION, or
 *       COMMENT line of Genbank data file.
 */
char *genbank_get_subspecies() {
    int  indj, indk;
    char subspecies[LONGTEXT], temp[LONGTEXT];

    subspecies[0] = '\0';
    /* get subspecies */
    if (has_content(data.gbk.definition)) {
        if ((indj = find_pattern(data.gbk.definition, "subsp. ")) >= 0) {
            /* skip the key word */
            indj = Reach_white_space(data.gbk.definition, indj);
            /* skip blank spaces */
            indj = Skip_white_space(data.gbk.definition, indj);
            /* get subspecies */
            get_string(data.gbk.definition, temp, indj);
            correct_subspecies(temp);
            strcpy(subspecies, temp);
        }
    }
    if (has_content(data.gbk.comments.others)) {
        if ((indj = find_pattern(data.gbk.comments.others, "*source:")) >= 0) {
            if ((indk = find_pattern((data.gbk.comments.others + indj),
                                     "sub-species=")) >= 0
                || (indk = find_pattern((data.gbk.comments.others + indj),
                                        "subspecies=")) >= 0 || (indk = find_pattern((data.gbk.comments.others + indj), "subsp.=")) >= 0) {
                /* skip the key word */
                for (indj += indk; data.gbk.comments.others[indj] != '='; indj++) ;
                indj++;
                /* skip blank spaces */
                indj = Skip_white_space(data.gbk.comments.others, indj);
                /* get subspecies */
                get_string(data.gbk.comments.others, temp, indj);
                if (has_content(subspecies)) {
                    if (!str_equal(temp, subspecies)) {
                        warningf(20, "Inconsistent subspecies definition in COMMENTS *source: %s and %s", temp, subspecies);
                    }
                }
                else
                    strcpy(subspecies, temp);
            }                   /* get subspecies */
        }                       /* find *source: line in comment */
    }                           /* look for subspecies on comments */

    if (has_content(data.gbk.source)) {
        if ((indj = find_pattern(data.gbk.source, "subsp. ")) >= 0
            || (indj = find_pattern(data.gbk.source, "subspecies ")) >= 0 || (indj = find_pattern(data.gbk.source, "sub-species ")) >= 0) {
            /* skip the key word */
            indj = Reach_white_space(data.gbk.source, indj);
            /* skip blank spaces */
            indj = Skip_white_space(data.gbk.source, indj);
            /* get subspecies */
            get_string(data.gbk.source, temp, indj);
            correct_subspecies(temp);
            if (has_content(subspecies)) {
                if (!str_equal(temp, subspecies)) {
                    warningf(21, "Inconsistent subspecies definition in SOURCE: %s and %s", temp, subspecies);
                }
            }
            else
                strcpy(subspecies, temp);
            /* check consistency of duplicated def */
        }                       /* find subspecies */
    }                           /* look for subspecies in SOURCE line */

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

/* --------------------------------------------------------------------
 *   Function genbank_get_atcc().
 *       Get atcc from SOURCE line in Genbank data file.
 */
char *genbank_get_atcc() {
    char  temp[LONGTEXT];
    char *atcc;

    atcc = NULL;
    /* get culture collection # */
    if (has_content(data.gbk.source))
        atcc = get_atcc(data.gbk.source);
    if (has_content(atcc) <= 1 && str0len(data.macke.strain)) {
        /* add () to macke strain to be processed correctly */
        sprintf(temp, "(%s)", data.macke.strain);
        atcc = get_atcc(temp);
    }
    return (atcc);
}

/* ------------------------------------------------------------------- */
/*  Function get_atcc().
 */
char *get_atcc(char *source) {
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
                /* skip the key word */
                indj += str0len(CC[indi]);
                /* skip blank spaces */
                indj = Skip_white_space(pstring, indj);
                /* get strain */
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
    /* append eoln to the atcc string */
    length = str0len(atcc);
    if (data.macke.atcc) {
        data.macke.atcc[length] = '\0';
    }
    strcat(atcc, "\n");
    return (nulldup(atcc));
}

/* ----------------------------------------------------------------- */
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

/* ----------------------------------------------------------------
 *   Function num_of_remark().
 *       Count num of remarks needed in order to alloc spaces.
 */
int num_of_remark() {
    int remnum, indj, length;

    remnum = 0;
    /* count references to be put into remarks */
    if (data.gbk.numofref > 0 && has_content(data.gbk.reference[0].ref))
        remnum++;
    for (indj = 1; indj < data.gbk.numofref; indj++) {
        if (has_content(data.gbk.reference[indj].ref))
            remnum++;
        if (has_content(data.gbk.reference[indj].journal))
            remnum++;
        if (has_content(data.gbk.reference[indj].author))
            remnum++;
        if (has_content(data.gbk.reference[indj].title))
            remnum++;
        if (has_content(data.gbk.reference[indj].standard))
            remnum++;
    }                           /* loop for copying other reference */
    /* count the other keyword in GenBank format to be put into remarks */
    if (has_content(data.gbk.keywords))
        remnum++;
    if (has_content(data.gbk.accession))
        remnum++;
    if (has_content(data.gbk.comments.orginf.source))    /* Source of strain */
        remnum++;
    if (has_content(data.gbk.comments.orginf.formname))
        remnum++;
    if (has_content(data.gbk.comments.orginf.nickname))  /* Alternate name */
        remnum++;
    if (has_content(data.gbk.comments.orginf.commname))
        remnum++;
    if (has_content(data.gbk.comments.orginf.hostorg))   /* host organism */
        remnum++;
    if (has_content(data.gbk.comments.seqinf.RDPid))
        remnum++;
    if (has_content(data.gbk.comments.seqinf.methods))
        remnum++;
    if (data.gbk.comments.seqinf.comp3 != ' ')
        remnum++;
    if (data.gbk.comments.seqinf.comp5 != ' ')
        remnum++;
    /* counting other than specific keyword comments */
    if (str0len(data.gbk.comments.others) > 0) {
        length = str0len(data.gbk.comments.others);
        for (indj = 0; indj < length; indj++) {
            if (data.gbk.comments.others[indj] == '\n' || data.gbk.comments.others[indj] == '\0') {
                remnum++;
            }                   /* new remark line */
        }                       /* for loop to find other remarks */
    }                           /* other comments */
    return (remnum);
}

/* -----------------------------------------------------------------
 *   Function macke_to_genbank().
 *       Convert from macke format to genbank format.
 */
void macke_to_genbank(const char *inf, const char *outf) {
    reset_seq_data();

    MackeReader  macke(inf);
    FILE        *ofp = open_output_or_die(outf);
    
    while (macke.mackeIn() != EOF) {
        data.numofseq++;
        if (mtog())
            genbank_out(ofp);
        else
            throw_error(15, "Conversion from macke to genbank fails");
        reinit_macke_genbank();
    }

    log_processed(data.numofseq);
    fclose(ofp);
}

/* ----------------------------------------------------------------
 *   Function mtog().
 *       Convert Macke format to Genbank format.
 */
int mtog() {
    // Responsibility note: caller has to cleanup macke and genbank data
    
    int  indi;
    char temp[LONGTEXT];

    strcpy(temp, data.macke.seqabbr);

    for (indi = str0len(temp); indi < 13; temp[indi++] = ' ') ;

    if (has_content(data.macke.date))

        sprintf((temp + 10), "%7d bp    RNA             RNA       %s\n", data.seq_length, genbank_date(data.macke.date));

    else
        sprintf((temp + 10), "%7d bp    RNA             RNA       %s\n", data.seq_length, genbank_date(today_date()));

    freedup(data.gbk.locus, temp);

    /* GenBank ORGANISM */
    if (has_content(data.macke.name)) {
        freedup(data.gbk.organism, data.macke.name);

        /* append a '.' at the end */
        terminate_with(data.gbk.organism, '.');
    }

    if (has_content(data.macke.rna)) {
        data.gbk.comments.seqinf.exist = 1;
        freedup(data.gbk.comments.seqinf.methods, data.macke.rna);
    }
    if (has_content(data.macke.acs)) {
        /* #### not converted to accession but to comment gbkentry only, temporarily
           freenull(data.gbk.accession);
           data.gbk.accession = nulldup(data.macke.acs);
         */
        data.gbk.comments.seqinf.exist = 1;
        freedup(data.gbk.comments.seqinf.gbkentry, data.macke.acs);
    }
    else if (has_content(data.macke.nbk)) {
        /* #### not converted to accession but to comment gbkentry only, temp
           freenull(data.gbk.accession);
           data.gbk.accession = nulldup(data.macke.nbk);
         */
        data.gbk.comments.seqinf.exist = 1;
        freedup(data.gbk.comments.seqinf.gbkentry, data.macke.nbk);
    }
    if (has_content(data.macke.atcc)) {
        data.gbk.comments.orginf.exist = 1;
        freedup(data.gbk.comments.orginf.cc, data.macke.atcc);
    }
    mtog_decode_ref_and_remarks();
    /* final conversion of cc */
    if (has_content(data.gbk.comments.orginf.cc) <= 1 && str0len(data.macke.atcc)) {
        freedup(data.gbk.comments.orginf.cc, data.macke.atcc);
    }

    /* define GenBank DEFINITION, after GenBank KEYWORD is defined. */
    mtog_genbank_def_and_source();

    return (1);
}

inline void resize_references(GenbankRef*& ref, int& size, int new_size) {
    ca_assert(new_size>size);

    ref = (GenbankRef*)Reallocspace(ref, sizeof(GenbankRef)*new_size);
    for (int i = size; i<new_size; ++i) {
        init_reference(ref[i]);
    }
    size = new_size;
}

inline void resize_references(GenBank& gbk, int new_size) {
    if (new_size>gbk.numofref) {
        resize_references(gbk.reference, gbk.numofref, new_size);
    }
}

/* ---------------------------------------------------------------
 *   Function mtog_decode_remarks().
 *       Decode remarks of Macke to GenBank format.
 */
void mtog_decode_ref_and_remarks() {
    Macke&   macke = data.macke;
    GenBank& gbk   = data.gbk;
    int      acount, tcount, jcount, rcount, scount;
    gbk.numofref   = acount = tcount = jcount = rcount = scount = 0;


    if (has_content(macke.author)) {
        ca_assert(acount == gbk.numofref);
        resize_references(gbk, acount+1);
        freedup(gbk.reference[acount++].author, macke.author);
    }
    if (has_content(macke.journal)) {
        ca_assert(jcount <= gbk.numofref);
        jcount = gbk.numofref - 1;
        freedup(gbk.reference[jcount++].journal, macke.journal);
    }
    if (has_content(macke.title)) {
        ca_assert(tcount <= gbk.numofref);
        tcount = gbk.numofref - 1;
        freedup(gbk.reference[tcount++].title, macke.title);
    }

    for (int indi = 0; indi < macke.numofrem; indi++) {
        char key[TOKENSIZE];
        int indj = macke_key_word(macke.remarks[indi], 0, key, TOKENSIZE);

        if (str_equal(key, "ref")) {
            ca_assert(rcount == gbk.numofref || (rcount == 0 && gbk.numofref == 1)); // 2nd is "delayed" ref in comment

            if (rcount == gbk.numofref) resize_references(gbk, rcount+1);
            else rcount = gbk.numofref - 1;

            freeset(gbk.reference[rcount++].ref, macke_copyrem(macke.remarks, &indi, macke.numofrem, indj));
        }
        else if (str_equal(key, "auth")) {
            ca_assert(acount <= gbk.numofref);
            acount = gbk.numofref - 1;
            freeset(gbk.reference[acount++].author, macke_copyrem(macke.remarks, &indi, macke.numofrem, indj));
        }
        else if (str_equal(key, "title")) {
            ca_assert(tcount <= gbk.numofref);
            tcount = gbk.numofref - 1;
            freeset(gbk.reference[tcount++].title, macke_copyrem(macke.remarks, &indi, macke.numofrem, indj));
        }
        else if (str_equal(key, "jour")) {
            ca_assert(jcount <= gbk.numofref);
            jcount = gbk.numofref - 1;
            freeset(gbk.reference[jcount++].journal, macke_copyrem(macke.remarks, &indi, macke.numofrem, indj));
        }
        else if (str_equal(key, "standard")) {
            ca_assert(scount <= gbk.numofref);
            scount = gbk.numofref - 1;
            freeset(gbk.reference[scount++].standard, macke_copyrem(macke.remarks, &indi, macke.numofrem, indj));
        }
        else if (str_equal(key, "KEYWORDS")) {
            mtog_copy_remark(gbk.keywords, &indi, indj);
            terminate_with(gbk.keywords, '.');
        }
        else if (str_equal(key, "GenBank ACCESSION")) {
            mtog_copy_remark(gbk.accession, &indi, indj);
        }
        else if (str_equal(key, "Source of strain")) {
            gbk.comments.orginf.exist = 1;
            mtog_copy_remark(gbk.comments.orginf.source, &indi, indj);
        }
        else if (str_equal(key, "Former name")) {
            gbk.comments.orginf.exist = 1;
            mtog_copy_remark(gbk.comments.orginf.formname, &indi, indj);
        }
        else if (str_equal(key, "Alternate name")) {
            gbk.comments.orginf.exist = 1;
            mtog_copy_remark(gbk.comments.orginf.nickname, &indi, indj);
        }
        else if (str_equal(key, "Common name")) {
            gbk.comments.orginf.exist = 1;
            mtog_copy_remark(gbk.comments.orginf.commname, &indi, indj);
        }
        else if (str_equal(key, "Host organism")) {
            gbk.comments.orginf.exist = 1;
            mtog_copy_remark(gbk.comments.orginf.hostorg, &indi, indj);
        }
        else if (str_equal(key, "RDP ID")) {
            gbk.comments.seqinf.exist = 1;
            mtog_copy_remark(gbk.comments.seqinf.RDPid, &indi, indj);
        }
        else if (str_equal(key, "Sequencing methods")) {
            gbk.comments.seqinf.exist = 1;
            mtog_copy_remark(gbk.comments.seqinf.methods, &indi, indj);
        }
        else if (str_equal(key, "3' end complete")) {
            gbk.comments.seqinf.exist = 1;
            scan_token_or_die(macke.remarks[indi] + indj, key, NULL);
            gbk.comments.seqinf.comp3 = str_equal(key, "Yes") ? 'y' : 'n';
        }
        else if (str_equal(key, "5' end complete")) {
            gbk.comments.seqinf.exist = 1;
            scan_token_or_die(macke.remarks[indi] + indj, key, NULL);
            gbk.comments.seqinf.comp5 = str_equal(key, "Yes") ? 'y' : 'n';
        }
        else { // other (non-interpreted) comments
            Append(gbk.comments.others, macke.remarks[indi]);
        }
    }                           /* for each rem */
}

/* ---------------------------------------------------------------
 *   Function mtog_copy_remark().
 *       Convert one remark back to GenBank format.
 */
void mtog_copy_remark(char*& Str, int *indi, int indj) {
    freeset(Str, macke_copyrem(data.macke.remarks, indi, data.macke.numofrem, indj));
}

/* ------------------------------------------------------------------
 *  Function macke_copyrem().
 *      uncode rem lines.
 */
char *macke_copyrem(char **strings, int *index, int maxline, int pointer) {
    char *Str;
    int   indi;

    Str = nulldup(strings[(*index)] + pointer);
    for (indi = (*index) + 1; indi < maxline && macke_is_continued_remark(strings[indi]); indi++)
        skip_eolnl_and_append_spaced(Str, strings[indi] + 3);
    (*index) = indi - 1;
    return (Str);
}

/* ------------------------------------------------------------------
 *   Function mtog_genbank_def_and_source().
 *       Define GenBank DEFINITION and SOURCE lines the way RDP
 *           group likes.
 */
void mtog_genbank_def_and_source() {
    freedup_if_content(data.gbk.definition, data.macke.name);
    if (has_content(data.macke.subspecies)) {
        if (str0len(data.gbk.definition) <= 1) {
            warning(22, "Genus and Species not defined");
            skip_eolnl_and_append(data.gbk.definition, "subsp. ");
        }
        else
            skip_eolnl_and_append(data.gbk.definition, " subsp. ");

        Append(data.gbk.definition, data.macke.subspecies);
    }

    if (has_content(data.macke.strain)) {
        if (str0len(data.gbk.definition) <= 1) {
            warning(23, "Genus and Species and Subspecies not defined");
            skip_eolnl_and_append(data.gbk.definition, "str. ");
        }
        else
            skip_eolnl_and_append(data.gbk.definition, " str. ");

        Append(data.gbk.definition, data.macke.strain);
    }

    /* create SOURCE line, temp. */
    if (has_content(data.gbk.definition)) {
        freedup(data.gbk.source, data.gbk.definition);
        terminate_with(data.gbk.source, '.');
    }

    /* append keyword to definition, if there is keyword. */
    if (has_content(data.gbk.keywords)) {
        if (has_content(data.gbk.definition))
            skip_eolnl_and_append(data.gbk.definition, "; \n");

        /* Here keywords must be ended by a '.' already */
        skip_eolnl_and_append(data.gbk.definition, data.gbk.keywords);
    }
    else
        skip_eolnl_and_append(data.gbk.definition, ".\n");
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
