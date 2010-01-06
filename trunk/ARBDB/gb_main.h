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

struct g_b_undo_mgr_struct;
struct gb_close_callback_struct;
struct gb_callback_list;
struct gb_user_struct;
struct gb_project_struct;
struct gb_key_struct;

// --------------------------------------------------------------------------------

enum gb_open_types {
    gb_open_all             = 0,
    gb_open_read_only_all   = 16,
    gb_open_read_only_small = 17
};

struct gb_quick_save_struct {
    char *quick_save_disabled;                      // if set, quick save is not possible and text describes reason why
    int   last_index;
};

// --------------------------------------------------------------------------------

struct gb_cache_entry_struct {
    GBDATA *gbd;
    long    prev;
    long    next;
    char   *data;
    long    clock;
    int     sizeof_data;
};

struct gb_cache_struct {
    gb_cache_entry_struct *entries;
    
    long firstfree_entry;
    long newest_entry;
    long oldest_entry;
    long sum_data_size;
    long max_data_size;
    long max_entries;
};

// --------------------------------------------------------------------------------
//      root structure (one for each database)

struct GB_MAIN_TYPE {
    int transaction;
    int aborted_transaction;
    int local_mode;             // GB_TRUE = server, GB_FALSE = client
    int client_transaction_socket;

    gbcmc_comm    *c_link;
    void          *server_data;
    GBCONTAINER   *dummy_father;
    GBCONTAINER   *data;
    GBDATA        *gb_key_data;
    char          *path;
    gb_open_types  opentype;
    char          *disabled_path;
    int            allow_corrupt_file_recovery;

    gb_quick_save_struct qs;
    gb_cache_struct      cache;
    int                  compression_mask;

    int            keycnt;      /* first non used key */
    long           sizeofkeys;  /* malloc size */
    long           first_free_key; /* index of first gap */
    gb_key_struct *keys;
    GB_HASH       *key_2_index_hash;
    long           key_clock;   /* trans. nr. of last change */

    char         *keys_new[256];
    unsigned int  last_updated;
    long          last_saved_time;
    long          last_saved_transaction;
    long          last_main_saved_transaction;

    GB_UNDO_TYPE requested_undo_type;
    GB_UNDO_TYPE undo_type;

    g_b_undo_mgr_struct *undo;

    char         *dates[256];
    unsigned int  security_level;
    int           old_security_level;
    int           pushed_security_level;
    long          clock;
    GB_NUMHASH   *remote_hash;

    GB_HASH *command_hash;
    GB_HASH *resolve_link_hash;
    GB_HASH *table_hash;

    gb_close_callback_struct *close_callbacks;

    gb_callback_list *cbl;      /* contains change-callbacks (after change, until callbacks are done) */
    gb_callback_list *cbl_last;

    gb_callback_list *cbld;     /* contains delete-callbacks (after delete, until callbacks are done) */
    gb_callback_list *cbld_last;

    gb_user_struct    *users[GB_MAX_USERS]; /* user 0 is server */
    gb_project_struct *projects[GB_MAX_PROJECTS]; /* projects */

    gb_user_struct    *this_user;
    gb_project_struct *this_project;
};

// -------------------------
//      inline functions

inline gb_cache_struct& GB_GET_CACHE(GB_MAIN_TYPE *gbmain) {
    return gbmain->cache;
}

#else
#error gb_main.h included twice
#endif // GB_MAIN_H
