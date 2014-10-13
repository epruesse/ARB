// =============================================================== //
//                                                                 //
//   File      : gb_index.h                                        //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GB_INDEX_H
#define GB_INDEX_H

#ifndef GB_MEMORY_H
#include "gb_memory.h"
#endif
#ifndef GB_DATA_H
#include "gb_data.h"
#endif

// ----------------------
//      gb_if_entries

struct gb_if_entries {
    GB_REL_IFES   rel_ie_next;
    GB_REL_GBDATA rel_ie_gbd;
};

inline gb_if_entries *GB_IF_ENTRIES_NEXT(gb_if_entries *ie) {
    return GB_RESOLVE(gb_if_entries *, ie, rel_ie_next);
}
inline void SET_GB_IF_ENTRIES_NEXT(gb_if_entries *ie, gb_if_entries *next) {
    GB_SETREL(ie, rel_ie_next, next);
}

inline GBDATA *GB_IF_ENTRIES_GBD(gb_if_entries *ie) {
    return GB_RESOLVE(GBDATA*, ie, rel_ie_gbd);
}
inline void SET_GB_IF_ENTRIES_GBD(gb_if_entries *ie, GBDATA *gbd) {
    GB_SETREL(ie, rel_ie_gbd, gbd);
}


// ------------------------------
//      gb_index_files

struct gb_index_files {
    GB_REL_IFS   rel_if_next;
    GBQUARK      key;
    long         hash_table_size;
    long         nr_of_elements;
    GB_CASE      case_sens;
    GB_REL_PIFES rel_entries;
};


#if (MEMORY_TEST==1)

#define GB_ENTRIES_ENTRY(entries, idx)       (entries)[idx]
#define SET_GB_ENTRIES_ENTRY(entries, idx, ie) (entries)[idx] = (ie);

#else

#define GB_ENTRIES_ENTRY(entries, idx)                                  \
    ((gb_if_entries *) ((entries)[idx] ? ((char*)(entries))+((entries)[idx]) : NULL))

#define SET_GB_ENTRIES_ENTRY(entries, idx, ie)                  \
    do {                                                        \
        if (ie) {                                               \
            (entries)[idx] = (char*)(ie)-(char*)(entries);      \
        }                                                       \
        else {                                                  \
            (entries)[idx] = 0;                                 \
        }                                                       \
    } while (0)

#endif // MEMORY_TEST


inline GB_REL_IFES *GB_INDEX_FILES_ENTRIES(gb_index_files *ifs) {
    return GB_RESOLVE(GB_REL_IFES *, ifs, rel_entries);
}
inline void SET_GB_INDEX_FILES_ENTRIES(gb_index_files *ixf, gb_if_entries **entries) {
    GB_SETREL(ixf, rel_entries, entries);
}

inline gb_index_files *GB_INDEX_FILES_NEXT(gb_index_files *ixf) {
    return GB_RESOLVE(gb_index_files *, ixf, rel_if_next);
}
inline void SET_GB_INDEX_FILES_NEXT(gb_index_files *ixf, gb_index_files *next) {
    GB_SETREL(ixf, rel_if_next, next);
}

inline gb_index_files *GBCONTAINER_IFS(GBCONTAINER *gbc) {
    return GB_RESOLVE(gb_index_files *, gbc, rel_ifs);
}
inline void SET_GBCONTAINER_IFS(GBCONTAINER *gbc, gb_index_files *ifs) {
    GB_SETREL(gbc, rel_ifs, ifs);
}

#else
#error gb_index.h included twice
#endif // GB_INDEX_H
