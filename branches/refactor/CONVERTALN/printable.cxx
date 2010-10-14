#include "input_format.h"
#include "ali.h"

#define PRTLENGTH   62

/* ---------------------------------------------------------------
 *   Function to_printable()
 *       Convert from some format to PRINTABLE format.
 */
void to_printable(const char *inf, const char *outf, int informat) {
    if (!InputFormat::is_known(informat)) {
        throw_conversion_not_supported(informat, PRINTABLE);
    }

    FILE        *IFP = open_input_or_die(inf);
    FILE_BUFFER  ifp = create_FILE_BUFFER(inf, IFP);
    FILE        *ofp = open_output_or_die(outf);

    Alignment ali;
    while (1) {
        SmartPtr<InputFormat> in       = InputFormat::create(informat);
        SeqPtr       seq = in->get_data(ifp);
        if (seq.isNull()) break;
        ali.add(seq);
    }

    int total_seq = ali.get_count();
    int maxsize   = ali.get_max_len();
    int base_nums[total_seq];
    for (int i = 0; i<total_seq; ++i) base_nums[i] = 0;

    int current = 0;
    while (maxsize > current) {
        for (int indi = 0; indi < total_seq; indi++) {
            int length     = str0len(ali.get_seq(indi));
            int base_count = 0;
            for (int index = 0; index < PRTLENGTH && (current + index) < length; index++)
                if (ali.get_seq(indi)[index + current] != '~' && ali.get_seq(indi)[index + current] != '-' && ali.get_seq(indi)[index + current] != '.')
                    base_count++;

            // check if the first char is base or not
            int   start;
            if (current < length && ali.get_seq(indi)[current] != '~' && ali.get_seq(indi)[current] != '-' && ali.get_seq(indi)[current] != '.')
                start = base_nums[indi] + 1;
            else
                start = base_nums[indi];

            printable_print_line(ali.get_id(indi), ali.get_seq(indi), current, start, ofp);
            base_nums[indi] += base_count;
        }
        current += PRTLENGTH;
        if (maxsize > current)
            fputs("\n\n", ofp);
    }

    log_processed(total_seq);
    destroy_FILE_BUFFER(ifp);
    fclose(ofp);
}

/* ------------------------------------------------------------
 *   Function printable_print_line().
 *       print one printable line.
 */
void printable_print_line(const char *id, const char *sequence, int start, int base_count, FILE * fp) {
    int indi, index, count, bnum, seq_length;

    fputc(' ', fp);
    if ((bnum = str0len(id)) > 10) {
        // truncate if length of id is greater than 10 
        for (indi = 0; indi < 10; indi++) fputc(id[indi], fp);
        bnum = 1;
    }
    else {
        fputs(id, fp);
        bnum = 10 - bnum + 1;
    }
    // fill in the blanks to make up 10 chars id spaces 
    seq_length = str0len(sequence);
    if (start < seq_length)
        for (indi = 0; indi < bnum; indi++) fputc(' ', fp);
    else {
        fputc('\n', fp);
        return;
    }
    fprintf(fp, "%4d ", base_count);
    for (index = start, count = 0; count < PRTLENGTH && index < seq_length; index++) {
        fputc(sequence[index], fp);
        count++;
    }
    fputc('\n', fp);
}
