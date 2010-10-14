#include "input_format.h"
#include "ali.h"

#include <cerrno>

/* ---------------------------------------------------------------
 *  Function to_phylip()
 *      Convert from some format to PHYLIP format.
 */
void to_phylip(const char *inf, const char *outf, int informat, int readstdin) {
    if (!InputFormat::is_known(informat)) {
        throw_conversion_not_supported(informat, PHYLIP);
    }

    FILE        *IFP = open_input_or_die(inf);
    FILE_BUFFER  ifp = create_FILE_BUFFER(inf, IFP);
    FILE        *ofp = open_output_or_die(outf);

    if (ofp == stdout) {
        ca_assert(0); // can't use stdout (because rewind is used below)
        throw_error(140, "Cannot write to standard output");
    }

    Alignment ali;
    while(1) {
        SmartPtr<InputFormat> in       = InputFormat::create(informat);
        SeqPtr       seq = in->get_data(ifp);
        if (seq.isNull()) break;
        ali.add(seq);
    }

    int maxsize     = ali.get_max_len();
    int total_seq   = ali.get_count();
    int current     = 0;
    int headersize1 = fprintf(ofp, "%8d %8d", maxsize, current);

    if (readstdin) {
        int spaced = 0;
        while (1) {
            int c = getchar();
            if (c == EOF) break; // read all from stdin now (not only one line) 
            if (!spaced) {
                fputc(' ', ofp);
                spaced = 1;
            }
            fputc(c, ofp);
        }
    }
    fputc('\n', ofp);

    while (maxsize > current) {
        for (int indi = 0; indi < total_seq; indi++) {
            phylip_print_line(ali.get_id(indi), ali.get_seq(indi), ali.get_len(indi), current, ofp);
        }
        if (current == 0)
            current += (SEQLINE - 10);
        else
            current += SEQLINE;
        if (maxsize > current)
            fputc('\n', ofp);
    }
    // rewrite output header 
    errno = 0;
    rewind(ofp);
    ca_assert(errno == 0);
    if (errno) {
        perror("rewind error");
        throw_errorf(141, "Failed to rewind file (errno=%i)", errno);
    }

    int headersize2 = fprintf(ofp, "%8d %8d", total_seq, maxsize);

    destroy_FILE_BUFFER(ifp);
    fclose(ofp);

    if (headersize1 != headersize2) {
        ca_assert(0);
        throw_errorf(142, "Failed to rewrite header (headersize differs: %i != %i)", headersize1, headersize2);
    }

    log_processed(total_seq);
}

/* --------------------------------------------------------------
 *  Function phylip_print_line().
 *      Print phylip line.
 */
void phylip_print_line(const char *name, const char *sequence, int seq_length, int index, FILE * fp) {
    ca_assert(seq_length>0);

    int length;
    if (index == 0) {
        int bnum;
        int nlen = str0len(name);
        if (nlen > 10) {
            // truncate id length of sequence ID is greater than 10
            for (int indi = 0; indi < 10; indi++) fputc(name[indi], fp);
            bnum = 1;
        }
        else {
            fputs(name, fp);
            bnum = 10 - nlen + 1;
        }
        // fill in blanks to make up 10 chars for ID. 
        for (int indi = 0; indi < bnum; indi++) fputc(' ', fp);
        length = SEQLINE - 10;
    }
    else if (index >= seq_length) {
        length = 0;
    }
    else {
        length = SEQLINE;
    }

    for (int indi = 0, indj = 0; indi < length; indi++) {
        if ((index + indi) < seq_length) {
            char c = sequence[index + indi];

            if (c == '.')
                c = '?';
            fputc(c, fp);
            indj++;
            if (indj == 10 && (index + indi) < (seq_length - 1) && indi < (length - 1)) {
                fputc(' ', fp);
                indj = 0;
            }
        }
        else
            break;
    }
    fputc('\n', fp);
}
