#include <stdio.h>
#include "convert.h"
#include "global.h"

/* ------------------------------------------------------------------
 *   Function to_gcg().
 *       Convert from whatever to GCG format.
 */

#warning fix outfile handling in to_gcg() 
void to_gcg(char *inf, char *outf, int intype) {
    FILE *IFP1, *IFP2, *IFP3;
    FILE_BUFFER ifp1 = 0, ifp2 = 0, ifp3 = 0;
    char temp[TOKENSIZE], *eof, line[LINESIZE], key[TOKENSIZE];
    char line1[LINESIZE], line2[LINESIZE], line3[LINESIZE], name[LINESIZE];
    char *eof1, *eof2, *eof3;
    int seqdata;

    if (intype == MACKE) {
        IFP1 = open_input_or_die(inf);
        IFP2 = open_input_or_die(inf);
        IFP3 = open_input_or_die(inf);
        ifp1 = create_FILE_BUFFER(inf, IFP1);
        ifp2 = create_FILE_BUFFER(inf, IFP2);
        ifp3 = create_FILE_BUFFER(inf, IFP3);
    }
    else {
        IFP1 = open_input_or_die(inf);
        ifp1 = create_FILE_BUFFER(inf, IFP1);
    }
    FILE *ofp = NULL;
    if (intype == MACKE) {
        /* skip to #=; where seq. first appears */
        for (eof1 = Fgetline(line1, LINESIZE, ifp1); eof1 != NULL && (line1[0] != '#' || line1[1] != '='); eof1 = Fgetline(line1, LINESIZE, ifp1)) ;
        /* skip to #:; where the seq. information is */
        for (eof2 = Fgetline(line2, LINESIZE, ifp2); eof2 != NULL && (line2[0] != '#' || line2[1] != ':'); eof2 = Fgetline(line2, LINESIZE, ifp2)) ;
        /* skip to where seq. data starts */
        for (eof3 = Fgetline(line3, LINESIZE, ifp3); eof3 != NULL && line3[0] == '#'; eof3 = Fgetline(line3, LINESIZE, ifp3)) ;
        /* for each seq. print out one gcg file. */
        for (; eof1 != NULL && (line1[0] == '#' && line1[1] == '='); eof1 = Fgetline(line1, LINESIZE, ifp1)) {
            macke_abbrev(line1, key, 2);
            Cpystr(temp, key);
            ofp = open_output_or_die(outf); // @@@ always overwrites same outfile
            for (macke_abbrev(line2, name, 2);
                 eof2 != NULL && line2[0] == '#' && line2[1] == ':' && str_equal(name, key);
                 eof2 = Fgetline(line2, LINESIZE, ifp2), macke_abbrev(line2, name, 2))
            {
                gcg_doc_out(line2, ofp);
            }
            eof3 = macke_origin(key, line3, ifp3);
            gcg_seq_out(ofp, key);
            fclose(ofp);
            log_processed(1); 
            ofp = NULL;
            init_seq_data();
        }
    }
    else {
        seqdata = 0;            /* flag of doc or data */
        eof = Fgetline(line, LINESIZE, ifp1);
        while (eof != NULL) {
            if (intype == GENBANK) {
                genbank_key_word(line, 0, temp, TOKENSIZE);
                if (str_equal(temp, "LOCUS")) {
                    genbank_key_word(line + 12, 0, key, TOKENSIZE);
                    ofp = open_output_or_die(outf);
                }
                else if (str_equal(temp, "ORIGIN")) {
                    gcg_doc_out(line, ofp);
                    seqdata = 1;
                    eof = genbank_origin(line, ifp1);
                }
            }
            else { /* EMBL or SwissProt */
                if (intype != EMBL && intype != PROTEIN)
                    throw_conversion_not_supported(intype, GCG);

                if (Lenstr(line) > 2 && line[0] == 'I' && line[1] == 'D') {
                    embl_key_word(line, 5, key, TOKENSIZE);
                    ofp = open_output_or_die(outf);
                }
                else if (Lenstr(line) > 1 && line[0] == 'S' && line[1] == 'Q') {
                    gcg_doc_out(line, ofp);
                    seqdata = 1;
                    eof = embl_origin(line, ifp1);
                }
            }

            if (seqdata) {
                gcg_seq_out(ofp, key);
                init_seq_data();
                seqdata = 0;
                log_processed(1); 
                fclose(ofp);
                ofp = NULL;
            }
            else {
                gcg_doc_out(line, ofp);
            }
            eof = Fgetline(line, LINESIZE, ifp1);
        }
    }
}

/* ----------------------------------------------------------------
 *   Function gcg_seq_out().
 *       Output sequence data in gcg format.
 */
void gcg_seq_out(FILE * ofp, char *key) {
    fprintf(ofp, "\n%s  Length: %d  %s  Type: N  Check: %d  ..\n\n", key, gcg_seq_length(), gcg_date(today_date()), checksum(data.sequence, data.seq_length));
    gcg_out_origin(ofp);
}

/* --------------------------------------------------------------------
 *   Function gcg_doc_out().
 *       Output non-sequence data(document) of gcg format.
 */
void gcg_doc_out(char *line, FILE * ofp) {
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
