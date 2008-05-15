// =============================================================== //
//                                                                 //
//   File      : AWT_translate.cxx                                 //
//   Purpose   :                                                   //
//   Time-stamp: <Tue Jun/20/2006 15:37 MET Coder@ReallySoft.de>   //
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

GB_ERROR AWT_findTranslationTable(GBDATA *gb_species, int& arb_transl_table) {
    // looks for a sub-entry 'transl_table' of 'gb_species'
    // if found -> test for validity and translate from EMBL to ARB table number
    // returns: an error in case of problems
    // 'arb_transl_table' is set to -1 if not found, otherwise it contains the arb table number

    arb_transl_table          = -1; // not found yet
    GB_ERROR  error           = 0;
    GBDATA   *gb_transl_table = GB_find(gb_species, "transl_table", 0, down_level);

    if (gb_transl_table) {
        int embl_table   = atoi(GB_read_char_pntr(gb_transl_table));
        arb_transl_table = AWT_embl_transl_table_2_arb_code_nr(embl_table);
        if (arb_transl_table == -1) { // ill. table
            const char *name    = "<unnamed>";
            GBDATA     *gb_name = GB_find(gb_species, "name", 0, down_level);
            if (gb_name) name = GB_read_char_pntr(gb_name);

            error = GBS_global_string("Illegal (or unsupported) value (%i) in 'transl_table' (species=%s)", embl_table, name);
        }
    }

    return error;
}

int AWT_pro_a_nucs_convert(char *data, long size, int pos, bool translate_all)
{
    // if translate_all == true -> 'pos' > 1 produces a leading 'X' in protein data
    //                             (otherwise nucleotides in front of the starting pos are simply ignored)
    // returns the translated protein sequence in 'data'
    
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
    
    for (char *p = data+pos; i+2<size; p+=3,i+=3) {
        buffer[0] = p[0];
        buffer[1] = p[1];
        buffer[2] = p[2];
        int spro  = (int)GBS_read_hash(awt_pro_a_nucs->t2i_hash,buffer);
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
    dest[0] = 0;
    return stops;
}





