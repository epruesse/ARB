// =============================================================== //
//                                                                 //
//   File      : AWT_translate.cxx                                 //
//   Purpose   :                                                   //
//   Time-stamp: <Mon Nov/17/2008 15:50 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2006      //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <cstdio>
#include <cstdlib>
#include <arbdb.h>
#include <arbdbt.h>

#include <awt_pro_a_nucs.hxx>
#include <awt_codon_table.hxx>

#include "awt_translate.hxx"

GB_ERROR AWT_getTranslationInfo(GBDATA *gb_species, int& arb_transl_table, int& codon_start) {
    // looks for sub-entries 'transl_table' and 'codon_start' of species (works for genes as well)
    // if found -> test for validity and translate 'transl_table' from EMBL to ARB table number
    // 
    // returns: an error in case of problems
    // 
    // 'arb_transl_table' is set to -1 if not found, otherwise it contains the arb table number
    // 'codon_start'      is set to -1 if not found, otherwise it contains the codon_start (0..2)

    arb_transl_table = -1;          // not found yet
    codon_start      = -1;          // not found yet
    
    GB_ERROR  error           = 0;
    GBDATA   *gb_transl_table = GB_entry(gb_species, "transl_table");

    if (gb_transl_table) {
        int embl_table   = atoi(GB_read_char_pntr(gb_transl_table));
        arb_transl_table = AWT_embl_transl_table_2_arb_code_nr(embl_table);
        if (arb_transl_table == -1) { // ill. table
            error = GBS_global_string("Illegal (or unsupported) value (%i) in 'transl_table'", embl_table);
        }
    }

    if (!error) {
        GBDATA *gb_codon_start = GB_entry(gb_species, "codon_start");
        if (gb_codon_start) {
            int codon_start_value = atoi(GB_read_char_pntr(gb_codon_start));

            if (codon_start_value<1 || codon_start_value>3) {
                error = GBS_global_string("Illegal value (%i) in 'codon_start' (allowed: 1..3)", codon_start_value);
            }
            else {
                codon_start = codon_start_value-1; // internal value is 0..2
            }
        }
    }

    if (arb_transl_table != codon_start) {
        if (arb_transl_table == -1) error = "Found 'codon_start', but 'transl_table' is missing";
        else if (codon_start == -1) error = "Found 'transl_table', but 'codon_start' is missing";
    }

    if (error) { // append species name to error message
        const char *name    = "<unknown>";
        GBDATA     *gb_name = GB_entry(gb_species, "name");

        if (gb_name) name = GB_read_char_pntr(gb_name);
        error             = GBS_global_string("%s (species='%s')", error, name);
    }

    return error;
}

int AWT_pro_a_nucs_convert(int code_nr, char *data, long size, int pos, bool translate_all, bool create_start_codon, bool append_stop_codon) {
    // if translate_all == true -> 'pos' > 1 produces a leading 'X' in protein data
    //                             (otherwise nucleotides in front of the starting pos are simply ignored)
    // 
    // if 'create_start_codon' is true and the first generated codon is a start codon of the used
    //                                 code, a 'M' is inserted instead of the codon
    // if 'append_stop_codon' is true, the stop codon is appended as '*'. This is only done, if the last
    //                                 character not already is a stop codon. (Note: provide data with correct size)
    // 
    // returns the translated protein sequence in 'data'
    // return value = number of stop-codons in translated sequence

    gb_assert(pos >= 0 && pos <= 2);

    for (char *p = data; *p ; p++) {
        char c = *p;
        if ((c>='a') && (c<='z')) c = c+'A'-'a';
        if (c=='U') c = 'T';
        *p = c;
    }

    char buffer[4];
    buffer[3] = 0;

    char *dest  = data;

    if (pos && translate_all) {
        for (char *p = data; p<data+pos; ++p) {
            char c = *p;
            if (c!='.' && c!='-') { // found a nucleotide
                *dest++ = 'X';
                break;
            }
        }
    }

    int  stops = 0;
    long i     = pos;

    const GB_HASH *t2i_hash = AWT_get_translator(code_nr)->T2iHash();

    for (char *p = data+pos; i+2<size; p+=3,i+=3) {
        buffer[0] = p[0];
        buffer[1] = p[1];
        buffer[2] = p[2];
        int spro  = (int)GBS_read_hash(t2i_hash,buffer);
        int C;
        if (!spro) {
            C = 'X';
        }
        else {
            if (spro == '*') stops++;
            C = spro;
            if (spro == 's') C = 'S';
        }
        *(dest++) = (char)C;
    }

    if (dest>data) {            // at least 1 amino written
        if (create_start_codon) {
            buffer[0] = data[pos];
            buffer[1] = data[pos+1];
            buffer[2] = data[pos+2];

            char start_codon = AWT_is_start_codon(buffer, code_nr);
            if (start_codon) data[0] = start_codon;
        }

        if (append_stop_codon && dest[-1] != '*') {
            *dest++ = '*';
        }
    }
    dest[0] = 0;

    return stops;
}





