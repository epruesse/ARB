// =============================================================== //
//                                                                 //
//   File      : gb_cb.h                                           //
//   Purpose   : gb_callback_list                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GB_CB_H
#define GB_CB_H

#ifndef ARBDB_H
#include <arbdb.h>
#endif

struct gb_transaction_save;

struct gb_callback_list {
    gb_callback_list    *next;
    gb_cb_spec           spec;
    gb_transaction_save *old;
    GBDATA              *gbd;
};

#else
#error gb_cb.h included twice
#endif // GB_CB_H
