#include "input_format.h"
#include "reader.h"
#include "ali.h"

#define PRTLENGTH   62

static void printable_print_line(const char *id, const char *sequence, int start, int base_count, Writer& write) {
    // print one printable line.
    int indi, index, count, bnum, seq_length;

    write.out(' ');
    if ((bnum = str0len(id)) > 10) {
        // truncate if length of id is greater than 10
        for (indi = 0; indi < 10; indi++) write.out(id[indi]);
        bnum = 1;
    }
    else {
        write.out(id);
        bnum = 10 - bnum + 1;
    }
    // fill in the blanks to make up 10 chars id spaces
    seq_length = str0len(sequence);
    if (start < seq_length)
        for (indi = 0; indi < bnum; indi++) write.out(' ');
    else {
        write.out('\n');
        return;
    }
    write.outf("%4d ", base_count);
    for (index = start, count = 0; count < PRTLENGTH && index < seq_length; index++) {
        write.out(sequence[index]);
        count++;
    }
    write.out('\n');
}

void to_printable(const FormattedFile& in, const char *outf) {
    // Convert from some format to PRINTABLE format.
    if (!is_input_format(in.type())) {
        throw_conversion_not_supported(in.type(), PRINTABLE);
    }

    FileWriter write(outf);

    Alignment ali;
    read_alignment(ali, in);

    int total_seq = ali.get_count();
    int maxsize   = ali.get_max_len();

    ca_assert(implicated(total_seq == 0, maxsize == -1));

    if (total_seq>0) {
        int base_nums[total_seq];
        for (int i = 0; i<total_seq; ++i) base_nums[i] = 0;

        int current = 0;
        while (maxsize > current) {
            for (int indi = 0; indi < total_seq; indi++) {
                const Seq&  seq        = ali.get(indi);
                int         length     = seq.get_len();
                const char *sequence   = seq.get_seq();
                int         base_count = 0;
                for (int index = 0; index < PRTLENGTH && (current + index) < length; index++)
                    if (!is_gapchar(sequence[index + current]))
                        base_count++;

                // check if the first char is base or not
                int   start;
                if (current < length && !is_gapchar(sequence[current]))
                    start = base_nums[indi] + 1;
                else
                    start = base_nums[indi];

                printable_print_line(seq.get_id(), seq.get_seq(), current, start, write);
                base_nums[indi] += base_count;
            }
            current += PRTLENGTH;
            if (maxsize > current)
                write.out("\n\n");
        }
    }

    write.seq_done(ali.get_count());
    write.expect_written();
}

