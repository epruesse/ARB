#ifndef __ADMAP_H
#define __ADMAP_H

#ifndef GBL_INCLUDED
#include "adlocal.h"
#endif

#define ADMAP_ID      "ARBDB Mapfile"
#define ADMAP_ID_LEN  13
#define ADMAP_VERSION 5

struct gb_map_header
{
    char        mapfileID[ADMAP_ID_LEN+1];
    long        version;
    long        byte_order;
    GB_MAIN_IDX main_idx; /* main_idx used in GBDATA and GBCONTAINER */
    long        main_data_offset;  /* offset of Main->data in MAP-File */
};

#endif
