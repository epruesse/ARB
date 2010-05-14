// =============================================================== //
//                                                                 //
//   File      : gb_main.h                                         //
//   Purpose   : GB_MAIN_TYPE                                      //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GB_MAIN_H
#define GB_MAIN_H

#ifndef GB_LOCAL_H
#include "gb_local.h"
#endif

// ------------------
//      constants

#define GB_MAX_PROJECTS 256

// ------------------------------
//      forward declare types

struct g_b_undo_mgr;
struct gb_close_callback_list;
struct gb_callback_list;
struct gb_user;
struct gb_project;
struct gb_Key;
struct gb_server_data;

// --------------------------------------------------------------------------------

enum gb_open_types {
    gb_open_all             = 0,
    gb_open_read_only_all   = 16,
    gb_open_read_only_small = 17
};

struct gb_quick_save {
    char *quick_save_disabled;                      // if set, quick save is not possible and text describes reason why
    int   last_index;
};

// --------------------------------------------------------------------------------

#if defined(DEBUG)
// #define GEN_CACHE_STATS
#endif // DEBUG

typedef uint16_t gb_cache_idx;

struct gb_cache_entry;
struct gb_cache {
    gb_cache_entry *entries;

    gb_cache_idx firstfree_entry;
    gb_cache_idx newest_entry;
    gb_cache_idx oldest_entry;
    
    size_t sum_data_size;
    size_t max_data_size;
    size_t big_data_min_size;

    size_t max_entries;

#if defined(GEN_CACHE_STATS)
    GB_HASH *not_reused;                            // key = DB_path, value = number of cache entries not reused
    GB_HASH *reused;                                // key = DB_path, value = number of cache entries reused
    GB_HASH *reuse_sum;                             // key = DB_path, value = how often entries were reused
#endif
};

// --------------------------------------------------------------------------------
//      root structure (one for each database)

#define ALLOWED_KEYS  15000
#define ALLOWED_DATES 256

struct GB_MAIN_TYPE {
    int transaction;
    int aborted_transaction;
    int local_mode;                                 // 1 = server, 0 = client
    int client_transaction_socket;

    gbcmc_comm     *c_link;
    gb_server_data *server_data;
    GBCONTAINER    *dummy_father;
    GBCONTAINER    *data;
    GBDATA         *gb_key_data;
    char           *path;
    gb_open_types   opentype;
    char           *disabled_path;
    int             allow_corrupt_file_recovery;

    gb_quick_save qs;
    gb_cache      cache;
    int           compression_mask;

    int      keycnt;                                // first non used key
    long     sizeofkeys;                            // malloc size
    long     first_free_key;                        // index of first gap
    gb_Key  *keys;
    GB_HASH *key_2_index_hash;
    long     key_clock;                             // trans. nr. of last change

    char         *keys_new[256];
    unsigned int  last_updated;
    long          last_saved_time;
    long          last_saved_transaction;
    long          last_main_saved_transaction;

    GB_UNDO_TYPE requested_undo_type;
    GB_UNDO_TYPE undo_type;

    g_b_undo_mgr *undo;

    char         *dates[ALLOWED_DATES];           // @@@ saved to DB, but never used
    unsigned int  security_level;
    int           old_security_level;
    int           pushed_security_level;
    long          clock;
    GB_NUMHASH   *remote_hash;

    GB_HASH *command_hash;
    GB_HASH *resolve_link_hash;
    GB_HASH *table_hash;

    gb_close_callback_list *close_callbacks;

    gb_callback_list *cbl;                          // contains change-callbacks (after change, until callbacks are done)
    gb_callback_list *cbl_last;

    gb_callback_list *cbld;                         // contains delete-callbacks (after delete, until callbacks are done)
    gb_callback_list *cbld_last;

    gb_user    *users[GB_MAX_USERS];                // user 0 is server
    gb_project *projects[GB_MAX_PROJECTS];          // projects

    gb_user    *this_user;
    gb_project *this_project;
};


#else
#error gb_main.h included twice
#endif // GB_MAIN_H
