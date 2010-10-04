#include <stdio.h>
#include "convert.h"
#include "global.h"

#include <errno.h>

/* ---------------------------------------------------------------
 *   Function reinit_phylip().
 *       Initialize genbank entry.
 */
void reinit_phylip() {
    // @@@ useless
}


/* ---------------------------------------------------------------
 *  Function to_phylip()
 *      Convert from some format to PHYLIP format.
 */
void to_phylip(const char *inf, const char *outf, int informat, int readstdin) {
    if (informat != GENBANK && informat != EMBL && informat != SWISSPROT && informat != MACKE) {
        throw_conversion_not_supported(informat, PHYLIP);
    }

    int maxsize, current, total_seq;
    int out_of_memory, indi;
    char temp[TOKENSIZE], eof;
    char *name;

    FILE        *IFP = open_input_or_die(inf);
    FILE_BUFFER  ifp = create_FILE_BUFFER(inf, IFP);
    FILE        *ofp = open_output_or_die(outf);

    if (ofp == stdout) {
        ca_assert(0); // can't use stdout (because rewind is used below)
        throw_error(140, "Cannot write to standard output");
    }

    maxsize       = 1;
    out_of_memory = 0;
    name          = NULL;
    // NOOP_global_data_was_previously_initialized_here();
    reinit_phylip();
    total_seq     = 0;
    do {
        if (informat == GENBANK) {
            reinit_genbank();
            eof = genbank_in_locus(ifp);
            if (eof == EOF) break;
            genbank_key_word(data.gbk.locus, 0, temp, TOKENSIZE);
        }
        else if (informat == EMBL || informat == SWISSPROT) {
            reinit_embl();
            eof = embl_in_id(ifp);
            if (eof == EOF) break;
            embl_key_word(data.embl.id, 0, temp, TOKENSIZE);
        }
        else if (informat == MACKE) {
            reinit_macke();
            eof = macke_in_name(ifp);
            if (eof == EOF) break;
            strcpy(temp, data.macke.seqabbr);
        }
        else ca_assert(0);

        total_seq++;

        if ((name = str0dup(temp)) == NULL && temp != NULL) {
            out_of_memory = 1;
            break;
        }
        if (data.seq_length > maxsize)
            maxsize = data.seq_length;

        if (!realloc_sequence_data(total_seq)) {
            out_of_memory = 1;
            break;
        }

        data.ids[total_seq - 1] = name;
        data.seqs[total_seq - 1] = str0dup(data.sequence);
        data.lengths[total_seq - 1] = str0len(data.sequence);
    } while (!out_of_memory);

    if (out_of_memory) {        /* cannot hold all seqs into mem. */
        fprintf(stderr, "Out of memory: Rerun the conversion sequence by sequence.\n");
        destroy_FILE_BUFFER(ifp);
        fclose(ofp);
        to_phylip_1x1(inf, outf, informat);
        return;
    }
    current = 0;
    int headersize1 = fprintf(ofp, "%8d %8d", maxsize, current);

    if (readstdin) {
        int c;

        int spaced = 0;

        while (1) {
            c = getchar();
            if (c == EOF) break; /* read all from stdin now (not only one line) */
            if (!spaced) {
                fputc(' ', ofp);
                spaced = 1;
            }
            fputc(c, ofp);
        }
    }
    fprintf(ofp, "\n");

    while (maxsize > current) {
        for (indi = 0; indi < total_seq; indi++) {
            phylip_print_line(data.ids[indi], data.seqs[indi], data.lengths[indi], current, ofp);
        }
        if (current == 0)
            current += (SEQLINE - 10);
        else
            current += SEQLINE;
        if (maxsize > current)
            fprintf(ofp, "\n");
    }
    /* rewrite output header */
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
    free_sequence_data(total_seq);
}

/* ---------------------------------------------------------------
 *   Function to_phylip_1x1()
 *       Convert from one format to PHYLIP format, one seq by one seq.
 */
void to_phylip_1x1(const char *inf, const char *outf, int informat) {
    int maxsize, current, total_seq;
    char temp[TOKENSIZE], eof;
    char *name;

    FILE        *IFP = open_input_or_die(inf);
    FILE_BUFFER  ifp = create_FILE_BUFFER(inf, IFP);
    FILE        *ofp = open_output_or_die(outf);
    
    maxsize = 1;
    current = 0;
    name    = NULL;
    fprintf(ofp, "%4d %4d\n", maxsize, current);
    while (maxsize > current) {
        NOOP_global_data_was_previously_initialized_here();
        FILE_BUFFER_rewind(ifp);
        total_seq = 0;
        do {                    /* read in one sequence */
            reinit_phylip();
            if (informat == GENBANK) {
                reinit_genbank();
                eof = genbank_in_locus(ifp);
            }
            else if (informat == EMBL || informat == SWISSPROT) {
                reinit_embl();
                eof = embl_in_id(ifp);
            }
            else if (informat == MACKE) {
                reinit_macke();
                eof = macke_in_name(ifp);
            }
            else throw_error(128, "UNKNOWN input format when converting to PHYLIP format");

            if (eof == EOF) break;

            if (informat == GENBANK) {
                genbank_key_word(data.gbk.locus, 0, temp, TOKENSIZE);
            }
            else if (informat == EMBL || informat == SWISSPROT) {
                embl_key_word(data.embl.id, 0, temp, TOKENSIZE);
            }
            else if (informat == MACKE) {
                macke_key_word(data.macke.name, 0, temp, TOKENSIZE);
            }
            else
                throw_error(130, "UNKNOWN input format when converting to PHYLIP format");
            freenull(name);
            name = str0dup(temp);
            if (data.seq_length > maxsize)
                maxsize = data.seq_length;
            phylip_print_line(name, data.sequence, 0, current, ofp);
            total_seq++;
        } while (1);
        if (current == 0)
            current += (SEQLINE - 10);
        else
            current += SEQLINE;
        if (maxsize > current)
            fprintf(ofp, "\n");
    }                           /* print block by block */

    rewind(ofp);
    fprintf(ofp, "%4d %4d", total_seq, maxsize);

    log_processed(total_seq);
    destroy_FILE_BUFFER(ifp);
    fclose(ofp);
}

/* --------------------------------------------------------------
 *  Function phylip_print_line().
 *      Print phylip line.
 */
void phylip_print_line(char *name, char *sequence, int seq_length, int index, FILE * fp) {
    int indi, indj, length, bnum;

    if (index == 0) {
        if (str0len(name) > 10) {
            /* truncate id length of seq ID is greater than 10 */
            for (indi = 0; indi < 10; indi++)
                fputc(name[indi], fp);
            bnum = 1;
        }
        else {
            fprintf(fp, "%s", name);
            bnum = 10 - str0len(name) + 1;
        }
        /* fill in blanks to make up 10 chars for ID. */
        for (indi = 0; indi < bnum; indi++)
            fputc(' ', fp);
        length = SEQLINE - 10;
    }
    else if (index >= data.seq_length)
        length = 0;
    else
        length = SEQLINE;

    if (seq_length == 0)
        seq_length = str0len(sequence);
    for (indi = indj = 0; indi < length; indi++) {
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
    fprintf(fp, "\n");
}
