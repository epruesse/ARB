// =============================================================== //
//                                                                 //
//   File      : gb_header.h                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GB_HEADER_H
#define GB_HEADER_H

#ifndef GB_DATA_H
#include "gb_data.h"
#endif


struct gb_header_flags {
    unsigned int flags : GB_MAX_USERS;              // public
    unsigned int key_quark : 24;                    // == 0 -> invalid
    unsigned int changed : 3;
    unsigned int ever_changed : 1;                  // is this element ever changed

    void set_change(GB_CHANGE val) {
        changed      = val;
        ever_changed = 1;
    }
    void inc_change(GB_CHANGE val) {
        if (changed<unsigned(val)) set_change(val);
    }
};

struct gb_header_list {                             // public fast flags
    gb_header_flags flags;
    GB_REL_GBDATA   rel_hl_gbd;
    /* pointer to data
       if 0 & !key_index -> free data
       if 0 & key_index -> data only in server */
};

inline GBDATA *GB_HEADER_LIST_GBD(gb_header_list& hl) {
    return GB_RESOLVE(GBDATA*, (&(hl)), rel_hl_gbd);
}
inline void SET_GB_HEADER_LIST_GBD(gb_header_list& hl, GBDATA *gbd) {
    GB_SETREL(&hl, rel_hl_gbd, gbd);
}

inline gb_header_flags& GB_ARRAY_FLAGS(GBDATA *gbd) { return GB_DATA_LIST_HEADER(GB_FATHER(gbd)->d)[gbd->index].flags; }

// ---------------------------------
//      container element access

inline GBDATA *EXISTING_GBCONTAINER_ELEM(GBCONTAINER *gbc, int idx) {
    return GB_HEADER_LIST_GBD(GB_DATA_LIST_HEADER((gbc)->d)[idx]);
}
inline GBDATA *GBCONTAINER_ELEM(GBCONTAINER *gbc, int idx) {
    return (idx<gbc->d.nheader) ? EXISTING_GBCONTAINER_ELEM(gbc, idx) : (GBDATA*)NULL;
}
inline void SET_GBCONTAINER_ELEM(GBCONTAINER *gbc, int idx, GBDATA *gbd) {
    SET_GB_HEADER_LIST_GBD(GB_DATA_LIST_HEADER(gbc->d)[idx], gbd);
}


#else
#error gb_header.h included twice
#endif // GB_HEADER_H
