/* ================================================================ */
/*                                                                  */
/*   File      : convert.c                                          */
/*   Purpose   : some helpers for global data handling              */
/*   Time-stamp: <Thu Jan/25/2007 12:53 MET Coder@ReallySoft.de>    */
/*                                                                  */
/*   Coded by Ralf Westram (coder@reallysoft.de) in December 2006   */
/*   Institute of Microbiology (Technical University Munich)        */
/*   http://www.arb-home.de/                                        */
/*                                                                  */
/* ================================================================ */
#include "convert.h"
#include "global.h"

int realloc_sequence_data(int total_seqs) {
    if (total_seqs>data.allocated) {
        data.allocated = (int)(data.allocated*1.5)+5;

        data.ids     =(char**)Reallocspace((char *)data.ids,     sizeof(char*)*data.allocated);
        data.seqs    =(char**)Reallocspace((char *)data.seqs,    sizeof(char*)*data.allocated);
        data.lengths =(int*)  Reallocspace((char *)data.lengths, sizeof(int)  *data.allocated);

        if (!data.ids || !data.seqs || !data.lengths) {
            return 0; // failed
        }
    }
    return 1; // success
}

void free_sequence_data(int used_entries) {
    int indi;
    for(indi=0; indi<used_entries; indi++)  {
        Freespace(&(data.ids[indi]));
        Freespace(&(data.seqs[indi]));
    }
    Freespace(&(data.ids));
    Freespace(&(data.seqs));
    Freespace(&(data.lengths));

    data.allocated = 0;
}

