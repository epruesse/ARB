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


// ------------------------
//      indexed keys

inline void GB_INDEX_CHECK_IN(GBENTRY *gbe) { if (gbe->flags2.tisa_index) gb_index_check_in(gbe); }
inline void GB_INDEX_CHECK_OUT(GBENTRY *gbe) { if ((gbe)->flags2.is_indexed) gb_index_check_out(gbe); }

#define GB_GBM_INDEX(gbd) ((gbd)->flags2.gbm_index)

// ---------------------------------------------------------
//      extern data storage (not directly inside GBDATA)

inline bool GB_CHECKINTERN(int size, int memsize) { return size<256 && memsize<SIZOFINTERN; }

inline char *GB_EXTERN_DATA_DATA(gb_extern_data& ex) { return GB_RESOLVE(char*, (&(ex)), rel_data); }
inline void SET_GB_EXTERN_DATA_DATA(gb_extern_data& ex, char *data) { GB_SETREL(&ex, rel_data, data); }

inline void GB_CREATE_EXT(GBDATA *gbd) { if (!gbd->ext) gb_create_extended(gbd); }
inline void GB_DELETE_EXT(GBDATA *gbd, long gbm_index) {
    if (gbd->ext) {
        gbm_free_mem(gbd->ext, sizeof(gb_db_extended), gbm_index);
        gbd->ext = 0;
    }
}

inline long GB_GET_EXT_CREATION_DATE(GBDATA *gbd) { return gbd->ext ? gbd->ext->creation_date : 0; }
inline long GB_GET_EXT_UPDATE_DATE(GBDATA *gbd) { return gbd->ext ? gbd->ext->update_date : 0; }

inline gb_callback *GB_GET_EXT_CALLBACKS(GBDATA *gbd) { return gbd->ext ? gbd->ext->callback : 0; }
inline gb_transaction_save *GB_GET_EXT_OLD_DATA(GBDATA *gbd) { return gbd->ext ? gbd->ext->old : 0; }

// --------------------------
//      data and datasize

inline char *GB_GETDATA(GBENTRY *gbe) { return gbe->stored_external() ? GB_EXTERN_DATA_DATA(gbe->info.ex)  : &((gbe)->info.istr.data[0]); } // @@@ move into GBENTRY

inline void GB_FREEDATA(GBENTRY *gbe) { // @@@ move into GBENTRY
    GB_INDEX_CHECK_OUT(gbe);
    if (gbe->stored_external() && GB_EXTERN_DATA_DATA(gbe->info.ex)) {
        gbm_free_mem(GB_EXTERN_DATA_DATA(gbe->info.ex), (size_t)(gbe->info.ex.memsize), GB_GBM_INDEX(gbe));
        SET_GB_EXTERN_DATA_DATA(gbe->info.ex, 0);
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
        SET_GB_EXTERN_DATA_DATA(gbe->info.ex, dat);
    }
    else {
        gbe->info.istr.size = (unsigned char)siz;
        gbe->info.istr.memsize = (unsigned char)memsiz;
    }
    GB_INDEX_CHECK_IN(gbe);
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
        SET_GB_EXTERN_DATA_DATA(gbe->info.ex, exData);
        if (dat) memcpy(exData, (char *)dat, (size_t)(memsiz));
    }
    GB_INDEX_CHECK_IN(gbe);
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
        SET_GB_EXTERN_DATA_DATA(gbe->info.ex, exData);
    }
    GB_INDEX_CHECK_IN(gbe);
}

#else
#error gb_storage.h included twice
#endif // GB_STORAGE_H
