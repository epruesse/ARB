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

inline bool store_inside_entry(int size, int memsize) {
    // returns true if data can be stored inside GBENTRY
    return size<256 && memsize<SIZOFINTERN;
}

// --------------------------
//      data and datasize

// wtf means SMD ??? (stored memory data)

inline void GB_SETSMD(GBENTRY *gbe, long siz, long memsiz, char *dat) {
    /* insert data into any db. field
     * Warning: this function has a lot of side effects:
     * 1. extern_data must be set by the user before calling this
     * 2. if !extern_data the data is not set (only size is set)
     *
     * -> better use GB_SETSMDMALLOC
     */

    gb_assert(gbe->stored_internal() == store_inside_entry(siz, memsiz));

    if (gbe->stored_external()) {
        // COVERED();
        gbe->info.ex.size = siz;
        gbe->info.ex.memsize = memsiz;
        gbe->info.ex.set_data(dat);
    }
    else {
        // COVERED();
        gbe->info.istr.size = (unsigned char)siz;
        gbe->info.istr.memsize = (unsigned char)memsiz;
    }
    gbe->index_re_check_in();
}

inline void GB_SETSMDMALLOC(GBENTRY *gbe, long siz, long memsiz, const char *dat) {
    gb_assert(dat);

    if (store_inside_entry(siz, memsiz)) {
        gbe->mark_as_intern();
        gbe->info.istr.size    = (unsigned char)siz;
        gbe->info.istr.memsize = (unsigned char)memsiz;

        if (dat) memcpy(&(gbe->info.istr.data[0]), (char *)dat, (size_t)(memsiz));
    }
    else {
        gbe->mark_as_extern();
        gbe->info.ex.size    = siz;
        gbe->info.ex.memsize = memsiz;

        char *exData = (char*)gbm_get_mem((size_t)memsiz, GB_GBM_INDEX(gbe));
        gbe->info.ex.set_data(exData);

        if (dat) memcpy(exData, (char *)dat, (size_t)(memsiz));
    }
    gbe->index_re_check_in();
}

inline void GB_SETSMDMALLOC_UNINITIALIZED(GBENTRY *gbe, long siz, long memsiz) {
    if (store_inside_entry(siz, memsiz)) {
        gbe->mark_as_intern();
        gbe->info.istr.size = (unsigned char)siz;
        gbe->info.istr.memsize = (unsigned char)memsiz;
    }
    else {
        gbe->mark_as_extern();
        gbe->info.ex.size    = siz;
        gbe->info.ex.memsize = memsiz;

        char *exData = (char*)gbm_get_mem((size_t)memsiz, GB_GBM_INDEX(gbe));
        gbe->info.ex.set_data(exData);
    }
    gbe->index_re_check_in();
}

#else
#error gb_storage.h included twice
#endif // GB_STORAGE_H
