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

#ifndef GB_DATA_H
#include "gb_data.h"
#endif

inline bool store_inside_entry(int size, int memsize) {
    // returns true if data can be stored inside GBENTRY
    return size<256 && memsize<SIZOFINTERN;
}

inline char *GBENTRY::alloc_data(long Size, long Memsize) {
    char *mem;
    gb_assert(!flags2.is_indexed);
    gb_assert(implicated(stored_external(), !data())); // would leak memory

    if (store_inside_entry(Size, Memsize)) {
        mark_as_intern();

        info.istr.size    = (unsigned char)Size;
        info.istr.memsize = (unsigned char)Memsize;

        mem = data();
    }
    else {
        mark_as_extern();

        info.ex.size    = Size;
        info.ex.memsize = Memsize;

        mem = (char*)gbm_get_mem((size_t)Memsize, GB_GBM_INDEX(this));
        info.ex.set_data(mem);
    }
    return mem;
}

class GBENTRY_memory : virtual Noncopyable {
    GBENTRY *gbe;
    char    *mem;
public:
    GBENTRY_memory(GBENTRY *gb_entry, long size, long memsize)
        : gbe(gb_entry),
          mem(gbe->alloc_data(size, memsize))
    {}
    ~GBENTRY_memory() { gbe->index_re_check_in(); }
    operator char*() { return mem; }
};

inline void GBENTRY::insert_data(const char *Data, long Size, long Memsize) {
    gb_assert(Data);
    memcpy(GBENTRY_memory(this, Size, Memsize), Data, Memsize);
}

#else
#error gb_storage.h included twice
#endif // GB_STORAGE_H
