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
    GB_CS_OK   = 0,
    GB_CS_SUB  = 1,
    GB_CS_ID   = 2,
    GB_CS_END  = 3,
    GB_CD_NODE = 4
};

struct gb_compress_tree {
    char              leaf;
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

inline size_t GBENTRY::uncompressed_size() const {
    return size() * gb_convert_type_2_sizeof[type()] + gb_convert_type_2_appendix_size[type()];
}

#else
#error gb_compress.h included twice
#endif // GB_COMPRESS_H
