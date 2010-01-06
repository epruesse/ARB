// =============================================================== //
//                                                                 //
//   File      : gb_map.h                                          //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GB_MAP_H
#define GB_MAP_H

#ifndef GB_LOCAL_H
#include "gb_local.h"
#endif

#define ADMAP_ID      "ARBDB Mapfile"
#define ADMAP_ID_LEN  13
#define ADMAP_VERSION 5

struct gb_map_header
{
    char        mapfileID[ADMAP_ID_LEN+1];
    long        version;
    long        byte_order;
    GB_MAIN_IDX main_idx;                           // main_idx used in GBDATA and GBCONTAINER
    long        main_data_offset;                   // offset of Main->data in MAP-File
};

#else
#error gb_map.h included twice
#endif // GB_MAP_H
