#include "genbank.h"
#include "embl.h"
#include "macke.h"

static void gcg_doc_out(const char *line, FILE * ofp) {
    // Output non-sequence data(document) of gcg format.
    int indi, len;
    int previous_is_dot;

    ca_assert(ofp);

    for (indi = 0, len = str0len(line), previous_is_dot = 0; indi < len; indi++) {
        if (previous_is_dot) {
            if (line[indi] == '.')
                fputc(' ', ofp);
            else
                previous_is_dot = 0;
        }
        fputc(line[indi], ofp);
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

static void gcg_out_origin(const Seq& seq, FILE * fp) {
    // Output sequence data in gcg format.
    int         indi, indj, indk;
    const char *sequence = seq.get_seq();

    for (indi = 0, indj = 0, indk = 1; indi < seq.get_len(); indi++) {
        if (!is_gapchar(sequence[indi])) {
            if ((indk % 50) == 1) fprintf(fp, "%8d  ", indk);
            fputc(sequence[indi], fp);
            indj++;
            if (indj == 10) {
                fputc(' ', fp);
                indj = 0;
            }
            if ((indk % 50) == 0) fputs("\n\n", fp);
            indk++;
        }
    }
    if ((indk % 50) != 1) fputs(" \n", fp);
}

static void gcg_seq_out(const Seq& seq, FILE * ofp, const char *key) {
    // Output sequence data in gcg format.
    fprintf(ofp, "\n%s  Length: %d  %s  Type: N  Check: %d  ..\n\n",
            key,
            seq.get_len()-seq.count_gaps(),
            gcg_date(today_date()),
            gcg_checksum(seq.get_seq(), seq.get_len()));
    gcg_out_origin(seq, ofp);
}

class GcgWriter {
    FILE *ofp;
    char *species_name;
    bool  seq_written; // if true, any further sequences are ignored

public:
    GcgWriter(const char *outf)
        : ofp(open_output_or_die(outf)),
          species_name(NULL),
          seq_written(false)
    {}
    ~GcgWriter() {
        if (!seq_written) throw_errorf(110, "No data written for species '%s'", species_name);
        free(species_name);
        fclose(ofp);
        log_processed(1);
    }

    void set_species_name(const char *next_name) {
        if (!seq_written) species_name = nulldup(next_name);
        else warningf(111, "Species '%s' dropped (GCG allows only 1 sequence per file)", next_name);
    }

    void add_comment(const char *comment) {
        if (!seq_written) gcg_doc_out(comment, ofp);
    }

    void write_seq_data(const Seq& seq) {
        if (!seq_written) {
            gcg_seq_out(seq, ofp, species_name);
            seq_written = true;
        }
    }
};

static void macke_to_gcg(const char *inf, const char *outf) {
    MackeReader inp(inf);
    Macke       macke;
    Seq         seq;

    if (inp.read_one_entry(macke, seq)) {
        FILE *ofp = open_output_or_die(outf);
        macke_seq_info_out(macke, ofp);
        gcg_seq_out(seq, ofp, macke.get_id());
        log_processed(1);
        fclose(ofp);
    }
}

static void genbank_to_gcg(const char *inf, const char *outf) {
    GenbankReader inp(inf);
    GcgWriter     out(outf);

    for (; inp.line(); ++inp) {
        const char *key = inp.get_key_word(0);

        if (str_equal(key, "LOCUS")) {
            out.set_species_name(inp.get_key_word(12));
            out.add_comment(inp.line());
        }
        else if (str_equal(key, "ORIGIN")) {
            out.add_comment(inp.line());
            Seq seq;
            inp.read_seq_data(seq);
            out.write_seq_data(seq);
        }
        else {
            out.add_comment(inp.line());
        }
    }
}

static void embl_to_gcg(const char *inf, const char *outf) {
    EmblSwissprotReader inp(inf);
    GcgWriter           out(outf);

    for (; inp.line(); ++inp) {
        const char *key = inp.get_key_word(0);

        if (str_equal(key, "ID")) {
            out.set_species_name(inp.get_key_word(5));
            out.add_comment(inp.line());
        }
        else if (str_equal(key, "SQ")) {
            out.add_comment(inp.line());
            Seq seq;
            inp.read_seq_data(seq);
            out.write_seq_data(seq);
            break;
        }
        else {
            out.add_comment(inp.line());
        }
    }
}

void to_gcg(const char *inf, const char *outf, int intype) {
    // Convert from whatever to GCG format
    // @@@ use InputFormat ?

    if (intype == MACKE) {
        macke_to_gcg(inf, outf);
    }
    else if (intype == GENBANK) {
        genbank_to_gcg(inf, outf);
    }
    else if (intype == EMBL || intype == SWISSPROT) {
        embl_to_gcg(inf, outf);
    }
    else {
        throw_conversion_not_supported(intype, GCG);
    }
}

