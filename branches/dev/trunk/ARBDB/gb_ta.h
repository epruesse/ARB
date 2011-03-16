// =============================================================== //
//                                                                 //
//   File      : gb_ta.h                                           //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GB_TA_H
#define GB_TA_H

#ifndef GB_MAIN_H
#include "gb_main.h"
#endif
#ifndef GB_DATA_H
#include "gb_data.h"
#endif

inline void GB_test_transaction(GBDATA *gbd) {
    if (!GB_MAIN(gbd)->transaction) {
        GBK_terminate("No running transaction");
    }
}

#else
#error gb_ta.h included twice
#endif // GB_TA_H
