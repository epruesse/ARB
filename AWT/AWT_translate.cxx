// =============================================================== //
//                                                                 //
//   File      : AWT_translate.cxx                                 //
//   Purpose   :                                                   //
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

GB_ERROR AWT_saveTranslationInfo(GBDATA *gb_species, int arb_transl_table, int codon_start) {
    GB_ERROR  error;
    int embl_transl_table = AWT_arb_code_nr_2_embl_transl_table(arb_transl_table);

    awt_assert(codon_start >= 0 && codon_start<3); // codon_start has to be 0..2
    awt_assert(embl_transl_table >= 0);


    GBDATA   *gb_transl_table   = GB_search(gb_species, "transl_table", GB_STRING);
    if (!gb_transl_table) error = GB_get_error();
    else      error             = GB_write_string(gb_transl_table, GBS_global_string("%i", embl_transl_table));

    if (!error) {
        GBDATA *gb_start_pos     = GB_search(gb_species, "codon_start", GB_STRING);
        if (!gb_start_pos) error = GB_get_error();
        else    error            = GB_write_string(gb_start_pos, GBS_global_string("%i", codon_start+1));
    }

    return error;
}

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
        error = GBS_global_string("%s (species='%s')", error, GBT_read_name(gb_species));
    }

    return error;
}

int AWT_pro_a_nucs_convert(int arb_code_nr, char *data, int size, int pos, bool translate_all, bool create_start_codon, bool append_stop_codon, int *translatedSize) {
    // if translate_all == true -> 'pos' > 1 produces a leading 'X' in protein data
    //                             (otherwise nucleotides in front of the starting pos are simply ignored)
    // 
    // if 'create_start_codon' is true and the first generated codon is a start codon of the used
    //                                 code, a 'M' is inserted instead of the codon
    // if 'append_stop_codon' is true, the stop codon is appended as '*'. This is only done, if the last
    //                                 character not already is a stop codon. (Note: provide data with correct size)
    // 
    // returns:
    // - the translated protein sequence in 'data'
    // - the length of the translated protein sequence in 'translatedSize' (if != 0)
    // - number of stop-codons in translated sequence as result

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

    const GB_HASH *t2i_hash = AWT_get_translator(arb_code_nr)->T2iHash();

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

    int tsize = dest-data;

    if (tsize>0) {            // at least 1 amino written
        if (create_start_codon) {
            buffer[0] = data[pos];
            buffer[1] = data[pos+1];
            buffer[2] = data[pos+2];

            char start_codon = AWT_is_start_codon(buffer, arb_code_nr);
            if (start_codon) data[0] = start_codon;
        }

        if (append_stop_codon && dest[-1] != '*') {
            *dest++ = '*';
            tsize++;
        }
    }
    dest[0] = 0;

    if (translatedSize) *translatedSize = tsize;
    
    return stops;
}





