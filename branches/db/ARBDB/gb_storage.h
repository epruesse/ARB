// =============================================================== //
//                                                                 //
//   File      : gb_storage.h                                      //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GB_STORAGE_H
#define GB_STORAGE_H

#ifndef GB_KEY_H
#include "gb_key.h"
#endif


// ---------------------------------------------------------
//      extern data storage (not directly inside GBDATA)

inline bool GB_CHECKINTERN(int size, int memsize) { return size<256 && memsize<SIZOFINTERN; }

inline gb_transaction_save *GB_GET_EXT_OLD_DATA(GBDATA *gbd) { return gbd->ext ? gbd->ext->old : 0; }

// --------------------------
//      data and datasize

inline void GB_FREEDATA(GBENTRY *gbe) { // @@@ move into GBENTRY
    gbe->index_check_out();
    if (gbe->stored_external()) {
        char *exdata = gbe->info.ex.get_data();
        if (exdata) {
            gbm_free_mem(exdata, (size_t)(gbe->info.ex.memsize), GB_GBM_INDEX(gbe));
            gbe->info.ex.set_data(0);
        }
    }
}

// wtf means SMD ???

inline void GB_SETSMD(GBENTRY *gbe, long siz, long memsiz, char *dat) {
    /* insert external data into any db. field
     * Warning: this function has a lot of side effects:
     * 1. extern_data must be set by the user before calling this
     * 2. if !extern_data the data is not set
     *
     * -> better use GB_SETSMDMALLOC
     */

    if (gbe->stored_external()) {
        gbe->info.ex.size = siz;
        gbe->info.ex.memsize = memsiz;
        gbe->info.ex.set_data(dat);
    }
    else {
        gbe->info.istr.size = (unsigned char)siz;
        gbe->info.istr.memsize = (unsigned char)memsiz;
    }
    gbe->index_re_check_in();
}

inline void GB_SETSMDMALLOC(GBENTRY *gbe, long siz, long memsiz, const char *dat) {
    gb_assert(dat);

    if (GB_CHECKINTERN(siz, memsiz)) {
        gbe->mark_as_intern();
        gbe->info.istr.size = (unsigned char)siz;
        gbe->info.istr.memsize = (unsigned char)memsiz;
        if (dat) memcpy(&(gbe->info.istr.data[0]), (char *)dat, (size_t)(memsiz));
    }
    else {
        char *exData;
        gbe->mark_as_extern();
        gbe->info.ex.size = siz;
        gbe->info.ex.memsize = memsiz;
        exData = (char*)gbm_get_mem((size_t)memsiz, GB_GBM_INDEX(gbe));
        gbe->info.ex.set_data(exData);
        if (dat) memcpy(exData, (char *)dat, (size_t)(memsiz));
    }
    gbe->index_re_check_in();
}

inline void GB_SETSMDMALLOC_UNINITIALIZED(GBENTRY *gbe, long siz, long memsiz) {
    if (GB_CHECKINTERN(siz, memsiz)) {
        gbe->mark_as_intern();
        gbe->info.istr.size = (unsigned char)siz;
        gbe->info.istr.memsize = (unsigned char)memsiz;
    }
    else {
        char *exData;
        gbe->mark_as_extern();
        gbe->info.ex.size = siz;
        gbe->info.ex.memsize = memsiz;
        exData = (char*)gbm_get_mem((size_t)memsiz, GB_GBM_INDEX(gbe));
        gbe->info.ex.set_data(exData);
    }
    gbe->index_re_check_in();
}

#else
#error gb_storage.h included twice
#endif // GB_STORAGE_H
