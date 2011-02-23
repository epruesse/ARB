// =============================================================== //
//                                                                 //
//   File      : seq.cxx                                           //
//   Purpose   :                                                   //
//                                                                 //
// =============================================================== //

#include "seq.h"
#include "ali.h"
#include "reader.h"

void Seq::out(Writer& write, Format outType) const {
    ca_assert(outType == EMBL || outType == GENBANK); // others not implemented yet

    const char *sequence = get_seq();
    int         indk     = 1;

    for (int indi = 0, indj = 0; indi < get_len(); indi++) {
        if ((indk % 60) == 1) {
            switch (outType) {
                case EMBL: write.out("     "); break;
                case GENBANK: write.outf("   %6d ", indk); break;
                default: ca_assert(0); break;
            }
        }
        write.out(sequence[indi]);
        indj++;

        // blank space follows every 10 bases, but not before '\n'
        if ((indk % 60) == 0) {
            write.out('\n');
            indj = 0;
        }
        else if (indj == 10 && indi != (get_len() - 1)) {
            write.out(' ');
            indj = 0;
        }
        indk++;
    }

    if ((indk % 60) != 1)
        write.out('\n');
    write.out("//\n");
}

void read_alignment(Alignment& ali, const FormattedFile& in) {
    FormatReaderPtr reader = FormatReader::create(in);
    while (1) {
        SeqPtr seq = new Seq;
        if (!reader->read_one_entry(*seq)) break;
        ali.add(seq);
    }
}


