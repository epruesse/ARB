#include "input_format.h"
#include "paup.h"
#include "ali.h"

/* -------------------------------------------------------------
 *   Function to_paup()
 *       Convert from some format to NEXUS format.
 */
void to_paup(const char *inf, const char *outf, int informat) {
    if (!InputFormat::is_known(informat)) {
        throw_conversion_not_supported(informat, NEXUS);
    }

    FILE        *IFP = open_input_or_die(inf);
    FILE_BUFFER  ifp = create_FILE_BUFFER(inf, IFP);
    FILE        *ofp = open_output_or_die(outf);

    Paup paup;
    paup_print_header(paup, ofp);

    Alignment ali;
    while (1) {
        SmartPtr<InputFormat> in       = InputFormat::create(informat);
        SeqPtr       seq = in->get_data(ifp);
        if (seq.isNull()) break;

        {
            char *name = strdup(seq->get_id());
            paup_verify_name(name);
            seq->set_id(name);
            free(name);
        }
        ali.add(seq);
    }

    int maxsize   = ali.get_max_len();
    int total_seq = ali.get_count();
    int current   = 0;

    while (maxsize > current) {
        int first_line = 0;
        for (int indi = 0; indi < total_seq; indi++) {
            if (current < ali.get_len(indi))
                first_line++;
            paup_print_line(ali.get_id(indi), ali.get_seq(indi), ali.get_len(indi), current, (first_line == 1), ofp);

            // Avoid repeating 
            if (first_line == 1)
                first_line++;
        }
        current += (SEQLINE - 10);
        if (maxsize > current)
            fputc('\n', ofp);
    }

    fputs("      ;\nENDBLOCK;\n", ofp);
    // rewrite output header 
    rewind(ofp);
    fputs("#NEXUS\n", ofp);

    fprintf(ofp, "[! RDP - the Ribosomal Database Project, (%s).]\n", today_date());

    fputs("[! To get started, send HELP to rdp@info.mcs.anl.gov ]\n", ofp);

    fputs("BEGIN DATA;\n   DIMENSIONS\n", ofp);
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
        
        freedup(Str, temp);
    }
}

/* --------------------------------------------------------------
 *  Function paup_print_line().
 *      print paup file.
 */
void paup_print_line(const char *Str, const char *sequence, int seq_length, int index, int first_line, FILE * fp) {
    int indi, indj, length;

    length = SEQLINE - 10;

    fputs("      ", fp);
    // truncate if length of sequence ID is greater than 10 
    for (indi = 0; indi < 10 && Str[indi]; indi++)
        fputc(Str[indi], fp);

    if (seq_length == 0)
        seq_length = str0len(sequence);

    if (index < seq_length) {
        for (; indi < 11; indi++) fputc(' ', fp);

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
void paup_print_header(const Paup& paup, FILE * ofp) {
    fputs("#NEXUS\n", ofp);
    fprintf(ofp, "[! RDP - the Ribosomal Database Project, (%s).]\n", today_date());
    fputs("[! To get started, send HELP to rdp@info.mcs.anl.gov ]\n", ofp);
    fputs("BEGIN DATA;\n   DIMENSIONS\n", ofp);
    fputs("      NTAX =       \n      NCHAR =       \n      ;\n", ofp);
    fputs("   FORMAT\n      LABELPOS = LEFT\n", ofp);
    fprintf(ofp, "      MISSING = .\n      EQUATE = \"%s\"\n", paup.equate);
    fprintf(ofp, "      INTERLEAVE\n      DATATYPE = RNA\n      GAP = %c\n      ;\n", paup.gap);
    fputs("   OPTIONS\n      GAPMODE = MISSING\n      ;\n   MATRIX\n", ofp);
}
