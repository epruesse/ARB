// =============================================================== //
//                                                                 //
//   File      : gb_compress.h                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GB_COMPRESS_H
#define GB_COMPRESS_H

#ifndef GB_STORAGE_H
#include "gb_storage.h"
#endif

#define GB_COMPRESSION_TAGS_SIZE_MAX 100

enum gb_compress_list_commands {
    gb_cs_ok   = 0,
    gb_cs_sub  = 1,
    gb_cs_id   = 2,
    gb_cs_end  = 3,
    gb_cd_node = 4
};

struct gb_compress_tree {
    char              leave;
    gb_compress_tree *son[2];
};

struct gb_compress_list {
    gb_compress_list_commands command;

    int  value;
    int  bitcnt;
    int  bits;
    int  mask;
    long count;

    gb_compress_list *son[2];
};

extern int gb_convert_type_2_sizeof[];
extern int gb_convert_type_2_appendix_size[];

#define GB_UNCOMPRESSED_SIZE(gbd, type) (GB_GETSIZE(gbd) * gb_convert_type_2_sizeof[type] + gb_convert_type_2_appendix_size[type])

#else
#error gb_compress.h included twice
#endif // GB_COMPRESS_H
