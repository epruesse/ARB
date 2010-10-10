/* ================================================================ */
/*                                                                  */
/*   File      : convert.c                                          */
/*   Purpose   : some helpers for global data handling              */
/*                                                                  */
/*   Coded by Ralf Westram (coder@reallysoft.de) in December 2006   */
/*   Institute of Microbiology (Technical University Munich)        */
/*   http://www.arb-home.de/                                        */
/*                                                                  */
/* ================================================================ */

#include "global.h"

struct global_data data;

int realloc_sequence_data(int total_seqs) {
    if (total_seqs > data.allocated) {
        data.allocated = (int)(data.allocated * 1.5) + 5;

        data.ids = (char **)Reallocspace((char *)data.ids, sizeof(char *) * data.allocated);
        data.seqs = (char **)Reallocspace((char *)data.seqs, sizeof(char *) * data.allocated);
        data.lengths = (int *)Reallocspace((char *)data.lengths, sizeof(int) * data.allocated);

        if (!data.ids || !data.seqs || !data.lengths) {
            return 0;           // failed
        }
    }
    return 1;                   // success
}

void free_sequence_data(int used_entries) {
    int indi;

    for (indi = 0; indi < used_entries; indi++) {
        freenull(data.ids[indi]);
        freenull(data.seqs[indi]);
    }
    freenull(data.ids);
    freenull(data.seqs);
    freenull(data.lengths);

    data.allocated = 0;
}
