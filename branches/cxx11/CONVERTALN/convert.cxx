// ================================================================
//
// File      : convert.cxx
// Purpose   : some helpers for global data handling
//
// ================================================================


#include "embl.h"
#include "genbank.h"
#include "macke.h"

FormatReaderPtr FormatReader::create(const FormattedFile& in) {
    switch (in.type()) {
        case GENBANK:   return new GenbankReader(in.name());
        case SWISSPROT:
        case EMBL:      return new EmblSwissprotReader(in.name());
        case MACKE:     return new MackeReader(in.name());
        default: throw_unsupported_input_format(in.type());
    }
    return NULL;
}

class ConvertibleData {
    const InputFormat& in;

    mutable OutputFormatPtr embl;
    mutable OutputFormatPtr gbk;
    mutable OutputFormatPtr macke;

public:
    ConvertibleData(const InputFormat& in_) : in(in_) {}

    Format format() const { return in.format(); }

    const Embl&    to_embl(const Seq& seq) const;
    const GenBank& to_genbank(const Seq& seq) const;
    const Macke&   to_macke(const Seq& seq) const;
};

#if defined(WARN_TODO)
#warning refactor below
#endif

const Embl& ConvertibleData::to_embl(const Seq& seq) const {
    Format inType = in.format();

    if (inType == EMBL) return dynamic_cast<const Embl&>(in);

    embl = new Embl;

    bool ok = true;
    switch (inType) {
        case GENBANK: ok = gtoe(dynamic_cast<const GenBank&>(in), dynamic_cast<Embl&>(*embl), seq); break;
        case MACKE:   ok = mtoe(dynamic_cast<const Macke&>(in), dynamic_cast<Embl&>(*embl), seq); break;
        default: throw_conversion_not_implemented(inType, EMBL);
    }
    if (!ok) throw_conversion_failure(inType, EMBL);

    ca_assert(!embl.isNull());
    return dynamic_cast<const Embl&>(*embl);
}

const GenBank& ConvertibleData::to_genbank(const Seq& seq) const {
    Format inType = in.format();

    if (inType == GENBANK) return dynamic_cast<const GenBank&>(in);

    gbk = new GenBank;

    bool ok = true;
    switch (inType) {
        case EMBL:  ok = etog(dynamic_cast<const Embl&>(in), dynamic_cast<GenBank&>(*gbk), seq); break;
        case MACKE: ok = mtog(dynamic_cast<const Macke&>(in), dynamic_cast<GenBank&>(*gbk), seq); break;
        default: throw_conversion_not_implemented(inType, GENBANK);
    }
    if (!ok) throw_conversion_failure(inType, GENBANK);

    ca_assert(!gbk.isNull());
    return dynamic_cast<const GenBank&>(*gbk);
}

const Macke& ConvertibleData::to_macke(const Seq& seq) const {
    Format inType = in.format();

    if (inType == MACKE) return dynamic_cast<const Macke&>(in);

    macke = new Macke;

    bool ok = true;
    switch (inType) {
        case EMBL:    ok = etom(dynamic_cast<const Embl&>(in), dynamic_cast<Macke&>(*macke), seq); break;
        case GENBANK: ok = gtom(dynamic_cast<const GenBank&>(in), dynamic_cast<Macke&>(*macke)); break;
        default: throw_conversion_not_implemented(inType, MACKE);
    }
    if (!ok) throw_conversion_failure(inType, MACKE);

    ca_assert(!macke.isNull());
    return dynamic_cast<const Macke&>(*macke);
}

static void write_to_embl(FileWriter& write, const ConvertibleData& data, const Seq& seq) {
    const Embl& embl = data.to_embl(seq);
    embl_out(embl, seq, write);
    write.seq_done();
}
static void write_to_genbank(FileWriter& write, const ConvertibleData& data, const Seq& seq) {
    const GenBank& gbk = data.to_genbank(seq);
    genbank_out(gbk, seq, write);
    write.seq_done();
}
static void write_to_macke(FileWriter& write, const ConvertibleData& data, const Seq& seq, int phase, bool first) {
    Format       inType = data.format();
    const Macke& macke  = data.to_macke(seq);
    switch (phase) {
        case 0:
            macke_seq_display_out(macke, write, inType, first);
            break;
        case 1:
            macke_seq_info_out(macke, write);
            break;
        case 2:
            macke_seq_data_out(seq, macke, write);
            write.seq_done(); // count only in one and last phase!
            break;
    }
}

static void to_macke(const FormattedFile& in, const char *outf) {
    FormatReaderPtr    reader = FormatReader::create(in);
    FileWriter         write(outf);
    SmartPtr<Warnings> suppress;

    macke_out_header(write);

    for (int phase = 0; phase<3; ++phase) {
        bool first = true;
        while (1) {
            Seq seq;
            if (!reader->read_one_entry(seq)) break;
            write_to_macke(write, reader->get_data(), seq, phase, first);

            first = false;
        }
        if (phase<2) {
            if (phase == 0) {
                write.out("#-\n");
                suppress = new Warnings;  // no warning messages for phase 1+2
            }
            reader->rewind();
        }
    }
    write.expect_written();
}

void convert(const FormattedFile& in, const FormattedFile& out) {
    if (str_equal(in.name(), out.name()))
        throw_error(30, "Input file and output file must be different file");

    bool converted = true;
    switch (out.type()) {
        case EMBL:
        case SWISSPROT:
        case GENBANK: {
            Format inType = in.type();
            Format ouType = out.type();
            if ((inType == GENBANK   && ouType == SWISSPROT) ||
                (inType == SWISSPROT && ouType == GENBANK)   ||
                (inType == EMBL      && ouType == SWISSPROT) ||
                (inType == SWISSPROT && ouType == EMBL))
            {
                converted = false;
                break;
            }

            FormatReaderPtr reader = FormatReader::create(in);
            FileWriter      write(out.name());

            while (1) {
                Seq seq;
                if (!reader->read_one_entry(seq)) break;
                if (ouType == GENBANK) {
                    write_to_genbank(write, reader->get_data(), seq);
                }
                else {
                    write_to_embl(write, reader->get_data(), seq);
                }
            }
            write.expect_written();
            break;
        }
        case MACKE:     to_macke(in, out.name()); break;
        case GCG:       to_gcg(in, out.name()); break;
        case NEXUS:     to_paup(in, out.name()); break;
        case PHYLIP:    to_phylip(in, out.name(), false); break;
        case FASTDNAML: to_phylip(in, out.name(), true); break;
        case PRINTABLE: to_printable(in, out.name()); break;

        default: converted = false; break;
    }
    if (!converted) {
        throw_conversion_not_supported(in.type(), out.type());
    }
}

