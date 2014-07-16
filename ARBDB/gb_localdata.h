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
#ifndef ARBTOOLS_H
#include <arbtools.h>
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
    ARB_TRANS,
    ARB_NO_TRANS
};

struct gb_exitfun;

class gb_local_data : virtual Noncopyable {
    // global data structure used for all open databases
    // @@@ could be moved into GB_shell

    GB_MAIN_TYPE **open_gb_mains;
    int            open_gb_alloc;

    int openedDBs;
    int closedDBs;

public:
    gb_buffer  buf1, buf2;
    char      *write_buffer;
    char      *write_ptr;
    long       write_bufsize;
    long       write_free;
    bool       iamclient;
    bool       search_system_folder;

    gb_compress_tree *bituncompress;
    gb_compress_list *bitcompress;

    long bc_size;

    ARB_TRANS_TYPE  running_client_transaction;
    struct gbl_ {
        GBDATA *gb_main;
        gbl_() : gb_main(0) {}
    } gbl;

    gb_exitfun *atgbexit;

    gb_local_data();
    ~gb_local_data();

    int open_dbs() const { return openedDBs-closedDBs; }
    
    void announce_db_open(GB_MAIN_TYPE *Main);
    void announce_db_close(GB_MAIN_TYPE *Main);
    GB_MAIN_TYPE *get_any_open_db() { int idx = open_dbs(); return idx ? open_gb_mains[idx-1] : NULL; }

#if defined(UNIT_TESTS) // UT_DIFF
    void fake_closed_DBs() {
        closedDBs = openedDBs;
    }
#endif
};

extern gb_local_data *gb_local;

#else
#error gb_localdata.h included twice
#endif // GB_LOCALDATA_H

