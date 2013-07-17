// =============================================================== //
//                                                                 //
//   File      : gb_ts.h                                           //
//   Purpose   : gb_transaction_save                               //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GB_TS_H
#define GB_TS_H

#ifndef GB_DATA_H
#include "gb_data.h"
#endif


struct GB_INTern_strings2 {
    char          data[SIZOFINTERN];
    unsigned char memsize;
    unsigned char size;
};

struct GB_INTern2 {
    char data[SIZOFINTERN];
};

struct gb_extern_data2 {
    char *data;
    long  memsize;
    long  size;
};

union gb_data_base_type_union2 {
    GB_INTern_strings2 istr;
    GB_INTern2         in;
    gb_extern_data2    ex;
};

struct gb_transaction_save {
    gb_flag_types            flags;
    gb_flag_types2           flags2;
    gb_data_base_type_union2 info;
    short                    refcount;              // number of references to this object

    bool stored_external() const { return flags2.extern_data; }
};

inline GB_TYPES GB_TYPE_TS(gb_transaction_save *ts)   { return GB_TYPES(ts->flags.type); }
inline long GB_GETSIZE_TS(gb_transaction_save *ts)    { return ts->stored_external() ? ts->info.ex.size     : ts->info.istr.size; }
inline long GB_GETMEMSIZE_TS(gb_transaction_save *ts) { return ts->stored_external() ? ts->info.ex.memsize  : ts->info.istr.memsize; }
inline char *GB_GETDATA_TS(gb_transaction_save *ts)   { return ts->stored_external() ? ts->info.ex.data     : &(ts->info.istr.data[0]); }

inline void GB_FREE_TRANSACTION_SAVE(GBDATA *gbd) {
    if (gbd->ext && gbd->ext->old) {
        gb_del_ref_gb_transaction_save(gbd->ext->old);
        gbd->ext->old = NULL;
    }
}

#else
#error gb_ts.h included twice
#endif // GB_TS_H
