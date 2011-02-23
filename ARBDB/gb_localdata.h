// =============================================================== //
//                                                                 //
//   File      : gb_localdata.h                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GB_LOCALDATA_H
#define GB_LOCALDATA_H

#ifndef _GLIBCXX_CSTDDEF
#include <cstddef>
#endif

struct GBDATA;
struct GB_MAIN_TYPE;
struct gb_compress_tree;
struct gb_compress_list;

struct gb_buffer {
    char   *mem;
    size_t  size;
};

enum ARB_TRANS_TYPE {
    ARB_COMMIT,
    ARB_ABORT,
    ARB_TRANS
};

struct gb_exitfun;

struct gb_local_data {
    // global data structure used for all open databases
    // @@@ could be moved into GB_shell

    gb_buffer  buf1, buf2;
    char      *write_buffer;
    char      *write_ptr;
    long       write_bufsize;
    long       write_free;
    int        iamclient;
    int        search_system_folder;

    gb_compress_tree *bituncompress;
    gb_compress_list *bitcompress;

    long bc_size;
    long gb_compress_keys_count;
    long gb_compress_keys_level;

    GB_MAIN_TYPE   *gb_compress_keys_main;
    ARB_TRANS_TYPE  running_client_transaction;
    struct {
        GBDATA *gb_main;
    } gbl;

    int openedDBs; 
    int closedDBs;

    gb_exitfun *atgbexit;
};

extern gb_local_data *gb_local;

#else
#error gb_localdata.h included twice
#endif // GB_LOCALDATA_H
