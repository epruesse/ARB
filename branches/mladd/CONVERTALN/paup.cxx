#include "input_format.h"
#include "reader.h"
#include "paup.h"
#include "ali.h"

static void paup_verify_name(char*& Str) {
    // Verify short_id in NEXUS format.
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

static void paup_print_line(const Seq& seq, int offset, int first_line, Writer& write) {
    // print paup file.
    int length = SEQLINE - 10;
    write.out("      ");

    int indi;

    const char *id = seq.get_id();
    for (indi = 0; indi < 10 && id[indi]; indi++) // truncate id to 10 characters
        write.out(id[indi]);

    if (offset < seq.get_len()) {
        for (; indi < 11; indi++) write.out(' ');

        const char *sequence = seq.get_seq();

        int indj = 0;
        for (indi = indj = 0; indi < length; indi++) {
            if ((offset + indi) < seq.get_len()) {
                write.out(sequence[offset + indi]);
                indj++;
                if (indj == 10 && indi < (length - 1) && (indi + offset) < (seq.get_len() - 1)) {
                    write.out(' ');
                    indj = 0;
                }
            }
            else
                break;
        }
    }

    if (first_line)
        write.outf(" [%d - %d]", offset + 1, (offset + indi));

    write.out('\n');
}

static void paup_print_headerstart(Writer& write) {
    write.out("#NEXUS\n");
    write.outf("[! RDP - the Ribosomal Database Project, (%s).]\n", today_date());
    write.out("[! To get started, send HELP to rdp@info.mcs.anl.gov ]\n");
    write.out("BEGIN DATA;\n   DIMENSIONS\n");
}

static void paup_print_header_counters(Writer& write) {
    write.outf("      NTAX = %6s\n      NCHAR = %6s\n      ;\n", "", "");
}
static void paup_print_header_counters(Writer& write, int total_seq, int maxsize) {
    write.outf("      NTAX = %6d\n      NCHAR = %6d\n      ;\n", total_seq, maxsize);
}

static void paup_print_header(const Paup& paup, Writer& write) {
    // Print out the header of each paup format.
    paup_print_headerstart(write);
    paup_print_header_counters(write);

    write.out("   FORMAT\n      LABELPOS = LEFT\n");
    write.outf("      MISSING = .\n      EQUATE = \"%s\"\n", paup.equate);
    write.outf("      INTERLEAVE\n      DATATYPE = RNA\n      GAP = %c\n      ;\n", paup.gap);
    write.out("   OPTIONS\n      GAPMODE = MISSING\n      ;\n   MATRIX\n");
}

void to_paup(const FormattedFile& in, const char *outf) {
    // Convert from some format to NEXUS format.
    if (!is_input_format(in.type())) {
        throw_conversion_not_supported(in.type(), NEXUS);
    }

    FileWriter write(outf);
    Paup       paup;

    paup_print_header(paup, write);

    Alignment ali;
    read_alignment(ali, in);

    for (int i = 0; i<ali.get_count(); ++i) {
        SeqPtr  seq  = ali.getSeqPtr(i);
        char   *name = ARB_strdup(seq->get_id());
        paup_verify_name(name);
        seq->replace_id(name);
        ca_assert(seq->get_id());
        free(name);
    }

    int maxsize   = ali.get_max_len();
    int total_seq = ali.get_count();
    int current   = 0;

    while (maxsize > current) {
        int first_line = 0;
        for (int indi = 0; indi < total_seq; indi++) {
            if (current < ali.get_len(indi))
                first_line++;
            paup_print_line(ali.get(indi), current, (first_line == 1), write);

            // Avoid repeating
            if (first_line == 1)
                first_line++;
        }
        current += (SEQLINE - 10);
        if (maxsize > current) write.out('\n');
    }

    write.out("      ;\nENDBLOCK;\n");

    // rewrite output header
    rewind(write.get_FILE());
    paup_print_headerstart(write);
    paup_print_header_counters(write, total_seq, maxsize);

    write.seq_done(ali.get_count());
    write.expect_written();
}
