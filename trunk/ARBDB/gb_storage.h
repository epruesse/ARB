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

inline void GB_INDEX_CHECK_IN(GBDATA *gbd) { if (gbd->flags2.tisa_index) gb_index_check_in(gbd); }
inline void GB_INDEX_CHECK_OUT(GBDATA *gbd) { if ((gbd)->flags2.is_indexed) gb_index_check_out(gbd); }

#define GB_GBM_INDEX(gbd) ((gbd)->flags2.gbm_index)

// ---------------------------------------------------------
//      extern data storage (not directly inside GBDATA)

inline GB_BOOL GB_CHECKINTERN(int size, int memsize) { return size<256 && memsize<SIZOFINTERN; }

inline void GB_SETINTERN(GBDATA *gbd) { gbd->flags2.extern_data = 0; }
inline void GB_SETEXTERN(GBDATA *gbd) { gbd->flags2.extern_data = 1; }

inline char *GB_EXTERN_DATA_DATA(struct gb_extern_data& ex) { return GB_RESOLVE(char*,(&(ex)),rel_data); }
inline void SET_GB_EXTERN_DATA_DATA(gb_extern_data& ex, char *data) { GB_SETREL(&ex, rel_data, data); }

inline void GB_CREATE_EXT(GBDATA *gbd) { if (!gbd->ext) gb_create_extended(gbd); }
inline void _GB_DELETE_EXT(GBDATA *gbd, long gbm_index) {
    if (gbd->ext) {
        gbm_free_mem((char *) gbd->ext, sizeof(struct gb_db_extended), gbm_index);
        gbd->ext = 0;
    }
}

inline long GB_GET_EXT_CREATION_DATE(GBDATA *gbd) { return gbd->ext ? gbd->ext->creation_date : 0; }
inline long GB_GET_EXT_UPDATE_DATE(GBDATA *gbd) { return gbd->ext ? gbd->ext->update_date : 0; }

inline gb_callback *GB_GET_EXT_CALLBACKS(GBDATA *gbd) { return gbd->ext ? gbd->ext->callback : 0; }
inline gb_transaction_save *GB_GET_EXT_OLD_DATA(GBDATA *gbd) { return gbd->ext ? gbd->ext->old : 0; }

// --------------------------
//      data and datasize

inline long GB_GETSIZE(const GBDATA *gbd)       { return gbd->flags2.extern_data ? gbd->info.ex.size     : gbd->info.istr.size; }
inline long GB_GETMEMSIZE(const GBDATA *gbd)    { return gbd->flags2.extern_data ? gbd->info.ex.memsize  : gbd->info.istr.memsize; }
inline char *GB_GETDATA(GBDATA *gbd)            { return gbd->flags2.extern_data ? GB_EXTERN_DATA_DATA(gbd->info.ex)  : &((gbd)->info.istr.data[0]); }

inline void GB_FREEDATA(GBDATA *gbd) {
    GB_INDEX_CHECK_OUT(gbd);
    if (gbd->flags2.extern_data && GB_EXTERN_DATA_DATA(gbd->info.ex)) {
        gbm_free_mem(GB_EXTERN_DATA_DATA(gbd->info.ex), (size_t)(gbd->info.ex.memsize), GB_GBM_INDEX(gbd));
        SET_GB_EXTERN_DATA_DATA(gbd->info.ex,0);
    }
}

// wtf means SMD ??? 

inline void GB_SETSMD(GBDATA *gbd, long siz, long memsiz, char *dat) {
    /** insert external data into any db. field
        Warning: this function has a lot of side effects:
        1. extern_data must be set by the user before calling this
        2. if !extern_data the data is not set
        -> better use GB_SETSMDMALLOC
    */

    if (gbd->flags2.extern_data) {
        gbd->info.ex.size = siz;
        gbd->info.ex.memsize = memsiz;
        SET_GB_EXTERN_DATA_DATA(gbd->info.ex,dat);
    }
    else {
        gbd->info.istr.size = (unsigned char)siz;
        gbd->info.istr.memsize = (unsigned char)memsiz;
    }
    GB_INDEX_CHECK_IN(gbd);
}

inline void GB_SETSMDMALLOC(GBDATA *gbd, long siz, long memsiz, const char *dat) {
    gb_assert(dat);
    
    if (GB_CHECKINTERN(siz,memsiz)) {
        GB_SETINTERN(gbd);
        gbd->info.istr.size = (unsigned char)siz;
        gbd->info.istr.memsize = (unsigned char)memsiz;
        if (dat) memcpy(&(gbd->info.istr.data[0]), (char *)dat, (size_t)(memsiz));
    }
    else {
        char *exData;
        GB_SETEXTERN(gbd);
        gbd->info.ex.size = siz;
        gbd->info.ex.memsize = memsiz;
        exData = gbm_get_mem((size_t)memsiz,GB_GBM_INDEX(gbd));
        SET_GB_EXTERN_DATA_DATA(gbd->info.ex,exData);
        if (dat) memcpy(exData, (char *)dat, (size_t)(memsiz));
    }
    GB_INDEX_CHECK_IN(gbd);
}

inline void GB_SETSMDMALLOC_UNINITIALIZED(GBDATA *gbd, long siz, long memsiz) {
    if (GB_CHECKINTERN(siz,memsiz)) {
        GB_SETINTERN(gbd);
        gbd->info.istr.size = (unsigned char)siz;
        gbd->info.istr.memsize = (unsigned char)memsiz;
    }
    else {
        char *exData;
        GB_SETEXTERN(gbd);
        gbd->info.ex.size = siz;
        gbd->info.ex.memsize = memsiz;
        exData = gbm_get_mem((size_t)memsiz,GB_GBM_INDEX(gbd));
        SET_GB_EXTERN_DATA_DATA(gbd->info.ex,exData);
    }
    GB_INDEX_CHECK_IN(gbd);
}

#else
#error gb_storage.h included twice
#endif // GB_STORAGE_H
