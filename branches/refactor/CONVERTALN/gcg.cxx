#include <stdio.h>
#include "convert.h"
#include "global.h"
#include "macke.h"
#include "types.h"

/* ------------------------------------------------------------------
 *   Function to_gcg().
 *       Convert from whatever to GCG format.
 */

class GcgWriter {
    FILE *ofp;
    char *species_name;
    bool  seq_written; // if true, any further sequences are ignored

public:
    GcgWriter(const char *outf)
        : ofp(open_output_or_die(outf)),
          seq_written(false)
    {}
    ~GcgWriter() {
        if (!seq_written) throw_errorf(110, "No data written for species '%s'", species_name);
        free(species_name);
        fclose(ofp);
        log_processed(1);
    }

    void set_species_name(const char *next_name) {
        if (!seq_written) species_name = Dupstr(next_name);
        else warningf(111, "Species '%s' dropped (GCG allows only 1 seq per file)", next_name);
    }

    void add_comment(const char *comment) {
        if (!seq_written) gcg_doc_out(comment, ofp);
    }

    void write_seq_data() { // uses global data
        if (!seq_written) {
            gcg_seq_out(ofp, species_name);
            seq_written = true;
        }
        reset_seq_data();
    }
};




static void macke_to_gcg(char *inf, char *outf) {
    // @@@ fix outfile handling - use GcgWriter!
    // @@@ use MackeReader here? 
    
    FILE        *IFP1 = open_input_or_die(inf);
    FILE        *IFP2 = open_input_or_die(inf);
    FILE        *IFP3 = open_input_or_die(inf);
    FILE_BUFFER  ifp1 = create_FILE_BUFFER(inf, IFP1);
    FILE_BUFFER  ifp2 = create_FILE_BUFFER(inf, IFP2);
    FILE_BUFFER  ifp3 = create_FILE_BUFFER(inf, IFP3);

    FILE *ofp = NULL;

    char line1[LINESIZE], line2[LINESIZE], line3[LINESIZE];
    char *eof1 = skipOverLinesThat(line1, LINESIZE, ifp1, Not(isMackeSeqHeader)); // skip to #=; where seq. first appears
    char *eof2 = skipOverLinesThat(line2, LINESIZE, ifp2, Not(isMackeSeqInfo));   // skip to #:; where the seq. information is
    char *eof3 = skipOverLinesThat(line3, LINESIZE, ifp3, isMackeHeader);         // skip to where seq. data starts

    for (; eof1 != NULL && isMackeSeqHeader(line1); eof1 = Fgetline(line1, LINESIZE, ifp1)) {
        char key[TOKENSIZE];
        macke_abbrev(line1, key, 2);

        char temp[TOKENSIZE];
        Cpystr(temp, key);
        ofp = open_output_or_die(outf);        // @@@ always overwrites same outfile

        char name[LINESIZE];
        for (macke_abbrev(line2, name, 2);
             eof2 != NULL && isMackeSeqInfo(line2) && str_equal(name, key);
             eof2  = Fgetline(line2, LINESIZE, ifp2), macke_abbrev(line2, name, 2))
        {
            gcg_doc_out(line2, ofp);
        }
        eof3 = macke_origin(key, line3, ifp3);
        gcg_seq_out(ofp, key);
        fclose(ofp);
        log_processed(1);
        ofp  = NULL;
        reset_seq_data();
        break; // can only handle 1 sequence!
    }
    destroy_FILE_BUFFER(ifp3);
    destroy_FILE_BUFFER(ifp2);
    destroy_FILE_BUFFER(ifp1);
}

static void genbank_to_gcg(char *inf, char *outf) {
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
            inp.read_sequence_data();
            out.write_seq_data();
        }
        else {
            out.add_comment(inp.line());
        }
    }
}

static void embl_to_gcg(char *inf, char *outf) {
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
            inp.read_sequence_data();
            out.write_seq_data();
            break;
        }
        else {
            out.add_comment(inp.line());
        }
    }
}

void to_gcg(char *inf, char *outf, int intype) {
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

/* ----------------------------------------------------------------
 *   Function gcg_seq_out().
 *       Output sequence data in gcg format.
 */
void gcg_seq_out(FILE * ofp, const char *key) {
    fprintf(ofp, "\n%s  Length: %d  %s  Type: N  Check: %d  ..\n\n",
            key,
            gcg_seq_length(),
            gcg_date(today_date()),
            checksum(data.sequence, data.seq_length));
    gcg_out_origin(ofp);
}

/* --------------------------------------------------------------------
 *   Function gcg_doc_out().
 *       Output non-sequence data(document) of gcg format.
 */
void gcg_doc_out(const char *line, FILE * ofp) {
    int indi, len;
    int previous_is_dot;

    ca_assert(ofp);

    for (indi = 0, len = Lenstr(line), previous_is_dot = 0; indi < len; indi++) {
        if (previous_is_dot) {
            if (line[indi] == '.')
                fprintf(ofp, " ");
            else
                previous_is_dot = 0;
        }
        fprintf(ofp, "%c", line[indi]);
        if (line[indi] == '.')
            previous_is_dot = 1;
    }
}

/* -----------------------------------------------------------------
 *   Function checksum().
 *       Calculate checksum for GCG format.
 */
int checksum(char *string, int numofstr)
{
    int cksum = 0, indi, count = 0, charnum;

    for (indi = 0; indi < numofstr; indi++) {
        if (string[indi] == '.' || string[indi] == '-' || string[indi] == '~')
            continue;
        count++;
        if (string[indi] >= 'a' && string[indi] <= 'z')
            charnum = string[indi] - 'a' + 'A';
        else
            charnum = string[indi];
        cksum = ((cksum + count * charnum) % 10000);
        if (count == 57)
            count = 0;
    }
    return (cksum);
}

/* --------------------------------------------------------------------
 *   Function gcg_out_origin().
 *       Output sequence data in gcg format.
 */
void gcg_out_origin(FILE * fp)
{

    int indi, indj, indk;

    for (indi = 0, indj = 0, indk = 1; indi < data.seq_length; indi++) {
        if (data.sequence[indi] == '.' || data.sequence[indi] == '~' || data.sequence[indi] == '-')
            continue;
        if ((indk % 50) == 1)
            fprintf(fp, "%8d  ", indk);
        fprintf(fp, "%c", data.sequence[indi]);
        indj++;
        if (indj == 10) {
            fprintf(fp, " ");
            indj = 0;
        }
        if ((indk % 50) == 0)
            fprintf(fp, "\n\n");
        indk++;
    }
    if ((indk % 50) != 1)
        fprintf(fp, " \n");
}

/* --------------------------------------------------------------
 *   Function gcg_output_filename().
 *       Get gcg output filename, convert all '.' to '_' and
 *           append ".RDP" as suffix.
 */
void gcg_output_filename(char *prefix, char *name)
{
    int indi, len;

    for (indi = 0, len = Lenstr(prefix); indi < len; indi++)
        if (prefix[indi] == '.')
            prefix[indi] = '_';
    sprintf(name, "%s.RDP", prefix);
}

/* ------------------------------------------------------------------
 *   Function gcg_seq_length().
 *       Calculate sequence length without gap.
 */
int gcg_seq_length()
{

    int indi, len;

    for (indi = 0, len = data.seq_length; indi < data.seq_length; indi++)
        if (data.sequence[indi] == '.' || data.sequence[indi] == '-' || data.sequence[indi] == '~')
            len--;

    return (len);
}
