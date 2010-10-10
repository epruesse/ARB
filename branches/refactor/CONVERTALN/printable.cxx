#include <stdio.h>
#include "global.h"

#define PRTLENGTH   62

/* ---------------------------------------------------------------
 *   Function to_printable()
 *       Convert from some format to PRINTABLE format.
 */
void to_printable(const char *inf, const char *outf, int informat) {
    if (informat != GENBANK && informat != EMBL && informat != SWISSPROT && informat != MACKE) {
        throw_conversion_not_supported(informat, PRINTABLE);
    }
    
    int   maxsize, current, total_seq, length;
    int   out_of_memory, indi, index, *base_nums, base_count, start;
    char  temp[TOKENSIZE], eof;
    char *name;

    FILE        *IFP = open_input_or_die(inf);
    FILE_BUFFER  ifp = create_FILE_BUFFER(inf, IFP);
    FILE        *ofp = open_output_or_die(outf);
    
    maxsize       = 1;
    out_of_memory = 0;
    name          = NULL;
    // NOOP_global_data_was_previously_initialized_here();
    total_seq     = 0;
    base_nums     = NULL;
    do {
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
        else throw_error(48, "UNKNOWN input format when converting to PRINTABLE format");

        if (eof == EOF) break;
        
        if (informat == GENBANK) {
            genbank_key_word(data.gbk.locus, 0, temp, TOKENSIZE);
        }
        else if (informat == EMBL || informat == SWISSPROT) {
            embl_key_word(data.embl.id, 0, temp, TOKENSIZE);
        }
        else if (informat == MACKE) {
            strcpy(temp, data.macke.seqabbr);
        }
        else throw_error(120, "UNKNOWN input format when converting to PRINTABLE format");
        
        total_seq++;

        if ((name = nulldup(temp)) == NULL && temp != NULL) {
            out_of_memory = 1;
            break;
        }
        if (data.seq_length > maxsize)
            maxsize = data.seq_length;

        if (!realloc_sequence_data(total_seq)) {
            out_of_memory = 1;
            break;
        }

        base_nums = (int *)Reallocspace((char *)base_nums, sizeof(int) * total_seq);
        if (base_nums == NULL) {
            out_of_memory = 1;
            break;
        }

        data.ids[total_seq - 1] = name;
        data.seqs[total_seq - 1] = nulldup(data.sequence);
        data.lengths[total_seq - 1] = str0len(data.sequence);
        base_nums[total_seq - 1] = 0;
    } while (!out_of_memory);

    if (out_of_memory) {        // cannot hold all seqs into mem. 
        fputs("Out of memory: Rerun the conversion sequence by sequence.\n", stderr);
        destroy_FILE_BUFFER(ifp);
        fclose(ofp);
        to_printable_1x1(inf, outf, informat);
        return;
    }
    current = 0;
    while (maxsize > current) {
        for (indi = 0; indi < total_seq; indi++) {
            length = str0len(data.seqs[indi]);
            for (index = base_count = 0; index < PRTLENGTH && (current + index) < length; index++)
                if (data.seqs[indi][index + current] != '~' && data.seqs[indi][index + current] != '-' && data.seqs[indi][index + current] != '.')
                    base_count++;

            // check if the first char is base or not 
            if (current < length && data.seqs[indi][current] != '~' && data.seqs[indi][current] != '-' && data.seqs[indi][current] != '.')
                start = base_nums[indi] + 1;
            else
                start = base_nums[indi];

            printable_print_line(data.ids[indi], data.seqs[indi], current, start, ofp);
            base_nums[indi] += base_count;
        }
        current += PRTLENGTH;
        if (maxsize > current)
            fputs("\n\n", ofp);
    }

    freenull(base_nums);

    log_processed(total_seq);
    free_sequence_data(total_seq); 
    destroy_FILE_BUFFER(ifp);
    fclose(ofp);
}

/* ---------------------------------------------------------------
 *   Function to_printable_1x1()
 *       Convert from one foramt to PRINTABLE format, one seq by one seq.
 */
void to_printable_1x1(const char *inf, const char *outf, int informat) {
    int maxsize, current, total_seq;
    int base_count, index;
    char temp[TOKENSIZE], eof;
    char *name;

    FILE        *IFP = open_input_or_die(inf);
    FILE_BUFFER  ifp = create_FILE_BUFFER(inf, IFP);
    FILE        *ofp = open_output_or_die(outf);
    
    maxsize = 1;
    current = 0;
    name = NULL;
    while (maxsize > current) {
        NOOP_global_data_was_previously_initialized_here();
        FILE_BUFFER_rewind(ifp);
        total_seq = 0;
        do {                    // read in one sequence 
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
            else throw_error(129, "UNKNOWN input format when converting to PRINTABLE format");

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
                throw_error(131, "UNKNOWN input format when converting to PRINTABLE format");
            freedup(name, temp);
            if (data.seq_length > maxsize)
                maxsize = data.seq_length;
            for (index = base_count = 0; index < current && index < data.seq_length; index++) {
                if (data.sequence[index] != '~' && data.sequence[index] != '.' && data.sequence[index] != '-')
                    base_count++;
            }
            // check if the first char is a base or not 
            if (current < data.seq_length && data.sequence[current] != '~' && data.sequence[current] != '.' && data.sequence[current] != '-')
                base_count++;

            printable_print_line(name, data.sequence, current, base_count, ofp);
            total_seq++;
        } while (1);
        current += PRTLENGTH;
        if (maxsize > current)
            fputs("\n\n", ofp);
    }                           // print block by block 

    log_processed(total_seq);
    destroy_FILE_BUFFER(ifp);
    fclose(ofp);
}

/* ------------------------------------------------------------
 *   Function printable_print_line().
 *       print one printable line.
 */
void printable_print_line(char *id, char *sequence, int start, int base_count, FILE * fp) {
    int indi, index, count, bnum, seq_length;

    fputc(' ', fp);
    if ((bnum = str0len(id)) > 10) {
        // truncate if length of id is greater than 10 
        for (indi = 0; indi < 10; indi++) fputc(id[indi], fp);
        bnum = 1;
    }
    else {
        fputs(id, fp);
        bnum = 10 - bnum + 1;
    }
    // fill in the blanks to make up 10 chars id spaces 
    seq_length = str0len(sequence);
    if (start < seq_length)
        for (indi = 0; indi < bnum; indi++) fputc(' ', fp);
    else {
        fputc('\n', fp);
        return;
    }
    fprintf(fp, "%4d ", base_count);
    for (index = start, count = 0; count < PRTLENGTH && index < seq_length; index++) {
        fputc(sequence[index], fp);
        count++;
    }                           // printout sequence data 
    fputc('\n', fp);
}
