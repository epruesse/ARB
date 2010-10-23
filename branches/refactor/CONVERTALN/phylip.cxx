#include "input_format.h"
#include "reader.h"
#include "ali.h"

#include <cerrno>

static void phylip_print_line(const Seq& seq, int index, Writer& write) {
    // Print phylip line.
    ca_assert(seq.get_len()>0);

    int length;
    if (index == 0) {
        int         bnum;
        const char *name = seq.get_id();
        int         nlen = str0len(name);
        if (nlen > 10) {
            // truncate id length of sequence ID is greater than 10
            for (int indi = 0; indi < 10; indi++) write.out(name[indi]);
            bnum = 1;
        }
        else {
            write.out(name);
            bnum = 10 - nlen + 1;
        }
        // fill in blanks to make up 10 chars for ID.
        for (int indi = 0; indi < bnum; indi++) write.out(' ');
        length = SEQLINE - 10;
    }
    else if (index >= seq.get_len()) {
        length = 0;
    }
    else {
        length = SEQLINE;
    }

    const char *sequence = seq.get_seq();
    for (int indi = 0, indj = 0; indi < length; indi++) {
        if ((index + indi) < seq.get_len()) {
            char c = sequence[index + indi];

            if (c == '.')
                c = '?';
            write.out(c);
            indj++;
            if (indj == 10 && (index + indi) < (seq.get_len() - 1) && indi < (length - 1)) {
                write.out(' ');
                indj = 0;
            }
        }
        else
            break;
    }
    write.out('\n');
}

void to_phylip(const char *inf, const char *outf, Format inType, int readstdin) {
    // Convert from some format to PHYLIP format.
    if (!is_input_format(inType)) {
        throw_conversion_not_supported(inType, PHYLIP);
    }

    FileWriter write(outf);

    if (write.get_FILE() == stdout) {
        ca_assert(0); // can't use stdout (because rewind is used below)
        throw_error(140, "Cannot write to standard output");
    }

    Alignment ali;
    read_alignment(ali, inType, inf);

    int maxsize     = ali.get_max_len();
    int total_seq   = ali.get_count();
    int current     = 0;
    int headersize1 = write.outf("%8d %8d", maxsize, current);

    if (readstdin) {
        int spaced = 0;
        while (1) {
            int c = getchar();
            if (c == EOF) break; // read all from stdin now (not only one line)
            if (!spaced) {
                write.out(' ');
                spaced = 1;
            }
            write.out(c);
        }
    }
    write.out('\n');

    while (maxsize > current) {
        for (int indi = 0; indi < total_seq; indi++) {
            phylip_print_line(ali.get(indi), current, write);
        }
        if (current == 0)
            current += (SEQLINE - 10);
        else
            current += SEQLINE;
        if (maxsize > current)
            write.out('\n');
    }
    // rewrite output header
    errno = 0;
    rewind(write.get_FILE());
    ca_assert(errno == 0);
    if (errno) {
        perror("rewind error");
        throw_errorf(141, "Failed to rewind file (errno=%i)", errno);
    }

    int headersize2 = write.outf("%8d %8d", total_seq, maxsize);

    if (headersize1 != headersize2) {
        ca_assert(0);
        throw_errorf(142, "Failed to rewrite header (headersize differs: %i != %i)", headersize1, headersize2);
    }

    write.seq_done(ali.get_count());
}

