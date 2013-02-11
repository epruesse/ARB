#include "genbank.h"
#include "embl.h"
#include "macke.h"

static void gcg_doc_out(const char *line, Writer& writer) {
    // Output non-sequence data(document) of gcg format.
    int indi, len;
    int previous_is_dot;

    ca_assert(writer.ok());

    for (indi = 0, len = str0len(line), previous_is_dot = 0; indi < len; indi++) {
        if (previous_is_dot) {
            if (line[indi] == '.')
                writer.out(' ');
            else
                previous_is_dot = 0;
        }
        writer.out(line[indi]);
        if (line[indi] == '.')
            previous_is_dot = 1;
    }
}

static int gcg_checksum(const char *Str, int numofstr) {
    // Calculate gcg_checksum for GCG format.
    int cksum = 0;
    int count = 0;
    for (int indi = 0; indi < numofstr; indi++) {
        if (!is_gapchar(Str[indi])) {
            count++;
            cksum = ((cksum + count * toupper(Str[indi])) % 10000);
            if (count == 57) count = 0;
        }
    }
    return cksum;
}

static void gcg_out_origin(const Seq& seq, Writer& write) {
    // Output sequence data in gcg format.
    int         indi, indj, indk;
    const char *sequence = seq.get_seq();

    for (indi = 0, indj = 0, indk = 1; indi < seq.get_len(); indi++) {
        if (!is_gapchar(sequence[indi])) {
            if ((indk % 50) == 1) write.outf("%8d  ", indk);
            write.out(sequence[indi]);
            indj++;
            if (indj == 10) {
                write.out(' ');
                indj = 0;
            }
            if ((indk % 50) == 0) write.out("\n\n");
            indk++;
        }
    }
    if ((indk % 50) != 1) write.out(" \n");
}

static void gcg_seq_out(const Seq& seq, Writer& write, const char *key) {
    // Output sequence data in gcg format.
    write.outf("\n%s  Length: %d  %s  Type: N  Check: %d  ..\n\n",
            key,
            seq.get_len()-seq.count_gaps(),
            gcg_date(today_date()),
            gcg_checksum(seq.get_seq(), seq.get_len()));
    gcg_out_origin(seq, write);
}

class GcgWriter;

class GcgCommentWriter : public Writer {
    GcgWriter& gcg_writer;

    char linebuf[LINESIZE];
    int  used;
public:
    GcgCommentWriter(GcgWriter& gcg_writer_)
        : gcg_writer(gcg_writer_),
          used(0)
    {}
    ~GcgCommentWriter() OVERRIDE {
        ca_assert(used == 0); // trailing \n has not been written
    }

    bool ok() const OVERRIDE { return true; }
    void throw_write_error() const OVERRIDE { ca_assert(0); }
    void out(char ch) OVERRIDE;
    const char *name() const OVERRIDE { return "comment-writer"; }
};

class GcgWriter : public FileWriter { // derived from a Noncopyable
    char *species_name;
    bool  seq_written;             // if true, any further sequences are ignored

    GcgCommentWriter writer;

public:
    GcgWriter(const char *outname)
        : FileWriter(outname),
          species_name(NULL),
          seq_written(false),
          writer(*this)
    {}
    ~GcgWriter() OVERRIDE { free(species_name); }

    void set_species_name(const char *next_name) {
        if (!seq_written) species_name = nulldup(next_name);
        else warningf(111, "Species '%s' dropped (GCG allows only 1 sequence per file)", next_name);
    }

    void add_comment(const char *comment) {
        if (!seq_written) gcg_doc_out(comment, *this);
    }

    Writer& comment_writer() {
        ca_assert(!seq_written);
        return writer;
    }

    void write_seq_data(const Seq& seq) {
        if (!seq_written) {
            ca_assert(species_name); // you have to call set_species_name() before!
            gcg_seq_out(seq, *this, species_name);
            seq_written = true;
        }
    }

    void expect_written() {
        FileWriter::seq_done(seq_written);
        seq_written = false;
        FileWriter::expect_written();
    }
};

void GcgCommentWriter::out(char ch) {
    linebuf[used++] = ch;
    ca_assert(used<LINESIZE);
    if (ch == '\n') {
        linebuf[used] = 0;
        gcg_writer.add_comment(linebuf);
        used = 0;
    }
}

static void macke_to_gcg(const char *inf, const char *outf) {
    MackeReader reader(inf);
    GcgWriter   out(outf);

    Seq seq;
    if (reader.read_one_entry(seq)) {
        Macke& macke = dynamic_cast<Macke&>(reader.get_data());
        out.set_species_name(macke.get_id());
        macke_seq_info_out(macke, out);
        out.write_seq_data(seq);

        reader.ignore_rest_of_file();
    }
    out.expect_written();
}

static void genbank_to_gcg(const char *inf, const char *outf) {
    FormatReaderPtr reader = FormatReader::create(FormattedFile(inf, GENBANK));
    GcgWriter       write(outf);

    GenBank gbk;
    Seq     seq;

    GenbankReader& greader = dynamic_cast<GenbankReader&>(*reader);
    if (GenbankParser(gbk, seq, greader).parse_entry()) {
        genbank_out_header(gbk, seq, write.comment_writer());
        genbank_out_base_count(seq, write.comment_writer());
        write.out("ORIGIN\n");
        write.set_species_name(gbk.get_id());
        write.write_seq_data(seq);

        reader->ignore_rest_of_file();
    }
    write.expect_written();
}

static void embl_to_gcg(const char *inf, const char *outf) {
    EmblSwissprotReader reader(inf);
    GcgWriter           write(outf);

    Embl embl;
    Seq  seq;

    if (EmblParser(embl, seq, reader).parse_entry()) {
        embl_out_header(embl, seq, write);
        write.set_species_name(embl.get_id());
        write.write_seq_data(seq);

        reader.ignore_rest_of_file();
    }
    write.expect_written();
}

void to_gcg(const FormattedFile& in, const char *outf) {
    // Convert from whatever to GCG format
    // @@@ use InputFormat ?

    switch (in.type()) {
        case MACKE:     macke_to_gcg(in.name(), outf); break;
        case GENBANK:   genbank_to_gcg(in.name(), outf); break;
        case EMBL:
        case SWISSPROT: embl_to_gcg(in.name(), outf); break;
        default:
            throw_conversion_not_supported(in.type(), GCG);
            break;
    }
}


