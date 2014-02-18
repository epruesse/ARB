// =============================================================== //
//                                                                 //
//   File      : gb_key.h                                          //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GB_KEY_H
#define GB_KEY_H

#ifndef GB_HEADER_H
#include "gb_header.h"
#endif
#ifndef GB_MAIN_H
#include "gb_main.h"
#endif
#ifndef GB_TUNE_H
#include "gb_tune.h"
#endif


#define GB_KEY_LEN_MAX 64                           // max. length of a key (a whole key path may be longer)
#define GB_KEY_LEN_MIN 2

struct gb_Key {
    char *key;

    long nref;
    long next_free_key;
    long nref_last_saved;

    GBDATA        *gb_key;                          // for fast access and dynamic loading
    GBCONTAINER   *gb_master_ali;                   // master container
    int            gb_key_disabled;                 // There will never be a gb_key
    int            compression_mask;                // maximum compression for this type
    GB_DICTIONARY *dictionary;                      // optional dictionary
};

inline GBQUARK GB_KEY_QUARK(GBDATA *gbd) { return GB_DATA_LIST_HEADER(GB_FATHER(gbd)->d)[gbd->index].flags.key_quark; }
inline char *GB_KEY(GBDATA *gbd) { return GB_MAIN(gbd)->keys[GB_KEY_QUARK(gbd)].key; }

inline long GB_QUARK_2_GBMINDEX(GB_MAIN_TYPE *Main, GBQUARK key_quark)  { return (Main->keys[key_quark].nref<GBM_MAX_UNINDEXED_ENTRIES) ? 0 : key_quark; }

#else
#error gb_key.h included twice
#endif // GB_KEY_H
