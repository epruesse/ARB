// ============================================================= //
//                                                               //
//   File      : SQ_ambiguities.cxx                              //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "SQ_ambiguities.h"
#include <arbdb.h>

using namespace std;

void SQ_ambiguities::SQ_count_ambiguities(const char *iupac, int length,
        GBDATA * gb_quality) {
    char c;

    for (int i = 0; i < length; i++) {
        c = iupac[i];
        switch (c) {
        case 'R':
            number++;
            iupac_value = iupac_value + 2;
            break;
        case 'Y':
            number++;
            iupac_value = iupac_value + 3;
            break;
        case 'M':
            number++;
            iupac_value = iupac_value + 2;
            break;
        case 'K':
            number++;
            iupac_value = iupac_value + 3;
            break;
        case 'W':
            number++;
            iupac_value = iupac_value + 3;
            break;
        case 'S':
            number++;
            iupac_value = iupac_value + 2;
            break;
        case 'B':
            number++;
            iupac_value = iupac_value + 4;
            break;
        case 'D':
            number++;
            iupac_value = iupac_value + 4;
            break;
        case 'H':
            number++;
            iupac_value = iupac_value + 4;
            break;
        case 'V':
            number++;
            iupac_value = iupac_value + 3;
            break;
        case 'N':
            number++;
            iupac_value = iupac_value + 5;
            break;
        }
    }
    percent = (100 * number) / length;

    GBDATA *gb_result1 = GB_search(gb_quality, "iupac_value", GB_INT);
    seq_assert(gb_result1);
    GB_write_int(gb_result1, iupac_value);
}

