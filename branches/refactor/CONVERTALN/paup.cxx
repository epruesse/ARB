#include <stdio.h>
#include "convert.h"
#include "global.h"

/* ------------------------------------------------------------
 *   Function reinit_paup().
 *       Init. paup data.
 */
void reinit_paup() {
    free_sequence_data(data.paup.ntax);
    data.paup.ntax = 0;
    data.paup.nchar = 0;
}

/* -------------------------------------------------------------
 *   Function to_paup()
 *       Convert from some format to NEXUS format.
 */
void to_paup(const char *inf, const char *outf, int informat) {
    if (informat != GENBANK && informat != EMBL && informat != SWISSPROT && informat != MACKE) {
        throw_conversion_not_supported(informat, NEXUS);
    }

    int maxsize, current, total_seq, first_line;
    int out_of_memory, indi;
    char temp[TOKENSIZE], eof;
    char *name;

    FILE        *IFP = open_input_or_die(inf);
    FILE_BUFFER  ifp = create_FILE_BUFFER(inf, IFP);
    FILE        *ofp = open_output_or_die(outf);

    maxsize       = 1;
    out_of_memory = 0;
    name          = NULL;

    // NOOP_global_data_was_previously_initialized_here();
    reinit_paup();

    paup_print_header(ofp);
    total_seq = 0;

    do {
        if (informat == GENBANK) {
            reinit_genbank();
            eof = genbank_in_locus(ifp);
            if (eof == EOF) break;
            genbank_key_word(data.gbk.locus, 0, temp, TOKENSIZE); 
        }
        else if (informat == EMBL || informat == SWISSPROT) {
            reinit_embl();
            eof = embl_in_id(ifp);
            if (eof == EOF) break;
            embl_key_word(data.embl.id, 0, temp, TOKENSIZE);
        }
        else if (informat == MACKE) {
            reinit_macke();
            eof = macke_in_name(ifp);
            if (eof == EOF) break;
            strcpy(temp, data.macke.seqabbr); 
        }
        else ca_assert(0);

        total_seq++;

        if ((name = str0dup(temp)) == NULL && temp != NULL) {
            out_of_memory = 1;
            break;
        }
        paup_verify_name(name);

        if (data.seq_length > maxsize)
            maxsize = data.seq_length;
        if (!realloc_sequence_data(total_seq)) {
            out_of_memory = 1;
            break;
        }

        data.ids[total_seq - 1] = name;
        data.seqs[total_seq - 1] = str0dup(data.sequence);
        data.lengths[total_seq - 1] = str0len(data.sequence);
    } while (!out_of_memory);

    if (out_of_memory) {
        /* cannot hold all seqs into mem. */
        fprintf(stderr, "Out of memory: Rerun the conversion sequence by sequence.\n");

        destroy_FILE_BUFFER(ifp);
        fclose(ofp);
        to_paup_1x1(inf, outf, informat);
        return;
    }
    current = 0;
    while (maxsize > current) {
        first_line = 0;
        for (indi = 0; indi < total_seq; indi++) {
            if (current < data.lengths[indi])
                first_line++;
            paup_print_line(data.ids[indi], data.seqs[indi], data.lengths[indi], current, (first_line == 1), ofp);

            /* Avoid repeating */
            if (first_line == 1)
                first_line++;
        }
        current += (SEQLINE - 10);
        if (maxsize > current)
            fprintf(ofp, "\n");
    }

    fprintf(ofp, "      ;\nENDBLOCK;\n");
    /* rewrite output header */
    rewind(ofp);
    fprintf(ofp, "#NEXUS\n");

    fprintf(ofp, "[! RDP - the Ribosomal Database Project, (%s).]\n", today_date());

    fprintf(ofp, "[! To get started, send HELP to rdp@info.mcs.anl.gov ]\n");

    fprintf(ofp, "BEGIN DATA;\n   DIMENSIONS\n");
    fprintf(ofp, "      NTAX = %6d\n      NCHAR = %6d\n      ;\n", total_seq, maxsize);

    log_processed(total_seq);
    free_sequence_data(total_seq);
    destroy_FILE_BUFFER(ifp);
    fclose(ofp);
}

/* ---------------------------------------------------------------
 *   Function to_paup_1x1()
 *       Convert from ALMA format to NEXUS format,
 *           one seq by one seq.
 */
void to_paup_1x1(const char *inf, const char *outf, int informat) {
    int maxsize, current, total_seq, first_line;
    char temp[TOKENSIZE], eof;
    char *name;

    FILE        *IFP = open_input_or_die(inf);
    FILE_BUFFER  ifp = create_FILE_BUFFER(inf, IFP);
    FILE        *ofp = open_output_or_die(outf);
    
    maxsize = 1;
    current = 0;
    name    = NULL;
    paup_print_header(ofp);
    while (maxsize > current) {
        NOOP_global_data_was_previously_initialized_here();
        FILE_BUFFER_rewind(ifp);
        total_seq = 0;
        first_line = 0;
        do {                    /* read in one sequence */
            reinit_paup();
            if (informat == GENBANK) {
                reinit_genbank();
                eof = genbank_in_locus(ifp);
            }
            else if (informat == EMBL || informat == SWISSPROT) {
                reinit_embl();
                eof = embl_in_id(ifp);
            }
            else if (informat == MACKE) {
                reinit_macke();
                eof = macke_in_name(ifp);
            }
            else throw_error(127, "UNKNOWN input format when converting to NEXUS format");

            if (eof == EOF) break;
            
            if (informat == GENBANK) {
                genbank_key_word(data.gbk.locus, 0, temp, TOKENSIZE);
            }
            else if (informat == EMBL || informat == SWISSPROT) {
                embl_key_word(data.embl.id, 0, temp, TOKENSIZE);
            }
            else if (informat == MACKE) {
                macke_key_word(data.macke.name, 0, temp, TOKENSIZE);
            }
            else
                throw_error(70, "UNKNOWN input format when converting to NEXUS format");

            freenull(name);
            name = str0dup(temp);
            paup_verify_name(name);

            if (data.seq_length > maxsize)
                maxsize = data.seq_length;

            if (current < data.seq_length)
                first_line++;

            paup_print_line(name, data.sequence, data.seq_length, current, (first_line == 1), ofp);

            /* Avoid repeating */
            if (first_line == 1)
                first_line++;

            total_seq++;
        } while (1);
        current += (SEQLINE - 10);
        if (maxsize > current)
            fprintf(ofp, "\n");
    }                           /* print block by block */

    fprintf(ofp, "      ;\nENDBLOCK;\n");
    /* rewrite output header */
    rewind(ofp);
    fprintf(ofp, "#NEXUS\n");

    fprintf(ofp, "[! RDP - the Ribosomal Database Project, (%s).]\n", today_date());

    fprintf(ofp, "[! To get started, send HELP to rdp@info.mcs.anl.gov ]\n");

    fprintf(ofp, "BEGIN DATA;\n   DIMENSIONS\n");
    fprintf(ofp, "      NTAX = %6d\n      NCHAR = %6d\n      ;\n", total_seq, maxsize);

    log_processed(total_seq);
    destroy_FILE_BUFFER(ifp);
    fclose(ofp);
}

/* -----------------------------------------------------------
 *   Function paup_verify_name().
 *       Verify short_id in NEXUS format.
 */
void paup_verify_name(char*& Str) {
    if (strpbrk(Str, "*(){/,;_=:\\\'")) {
        char temp[TOKENSIZE];
        temp[0] = '\'';

        int len   = str0len(Str);
        int indi  = 0;
        int index = 1;
        for (; indi < len; indi++, index++) {
            temp[index] = Str[indi];
            if (Str[indi] == '\'') temp[++index] = '\'';
        }
        temp[index++] = '\'';
        temp[index]   = '\0';
        
        freenull(Str);
        Str = str0dup(temp);
    }
}

/* --------------------------------------------------------------
 *  Function paup_print_line().
 *      print paup file.
 */
void paup_print_line(char *Str, char *sequence, int seq_length, int index, int first_line, FILE * fp) {
    int indi, indj, length;

    length = SEQLINE - 10;

    fputs("      ", fp);
    /* truncate if length of seq ID is greater than 10 */
    for (indi = 0; indi < 10 && Str[indi]; indi++)
        fputc(Str[indi], fp);

    if (seq_length == 0)
        seq_length = str0len(sequence);

    if (index < seq_length) {
        for (; indi < 11; indi++)
            fputc(' ', fp);

        for (indi = indj = 0; indi < length; indi++) {
            if ((index + indi) < seq_length) {
                fputc(sequence[index + indi], fp);
                indj++;
                if (indj == 10 && indi < (length - 1) && (indi + index) < (seq_length - 1)) {
                    fputc(' ', fp);
                    indj = 0;
                }
            }
            else
                break;
        }
    }

    if (first_line)
        fprintf(fp, " [%d - %d]", index + 1, (index + indi));

    fputc('\n', fp);
}

/* ----------------------------------------------------------
 *   Function paup_print_header().
 *       Print out the header of each paup format.
 */
void paup_print_header(FILE * ofp) {
    fprintf(ofp, "#NEXUS\n");
    fprintf(ofp, "[! RDP - the Ribosomal Database Project, (%s).]\n", today_date());
    fprintf(ofp, "[! To get started, send HELP to rdp@info.mcs.anl.gov ]\n");
    fprintf(ofp, "BEGIN DATA;\n   DIMENSIONS\n");
    fprintf(ofp, "      NTAX =       \n      NCHAR =       \n      ;\n");
    fprintf(ofp, "   FORMAT\n      LABELPOS = LEFT\n");
    fprintf(ofp, "      MISSING = .\n      EQUATE = \"%s\"\n", data.paup.equate);
    fprintf(ofp, "      INTERLEAVE\n      DATATYPE = RNA\n      GAP = %c\n      ;\n", data.paup.gap);
    fprintf(ofp, "   OPTIONS\n      GAPMODE = MISSING\n      ;\n   MATRIX\n");
}
