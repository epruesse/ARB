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
#ifndef GB_CB_H
#include "gb_cb.h"
#endif

// ------------------------------
//      forward declare types

struct g_b_undo_mgr;
struct gb_close_callback_list;
struct gb_user;
struct gb_project;
struct gb_Key;
struct gb_server_data;
struct gb_hierarchy_callback_list;
class  gb_hierarchy_location;

// --------------------------------------------------------------------------------

enum gb_open_types {
    gb_open_all             = 0,
    gb_open_read_only_all   = 16,
    gb_open_read_only_small = 17
};

struct gb_quick_save {
    char *quick_save_disabled;                      // if set, quick save is not possible and text describes reason why
    int   last_index;

    gb_quick_save()
        : quick_save_disabled(NULL),
          last_index(0)
    {}
};

// --------------------------------------------------------------------------------

#if defined(DEBUG)
// #define GEN_CACHE_STATS // unit tests will fail if enabled
#endif // DEBUG

typedef uint16_t gb_cache_idx;

struct gb_cache_entry;
struct gb_cache : virtual Noncopyable {
    gb_cache_entry *entries;

    gb_cache_idx firstfree_entry;
    gb_cache_idx newest_entry;
    gb_cache_idx oldest_entry;
    
    size_t sum_data_size;
    size_t max_data_size;
    size_t big_data_min_size;

#if defined(GEN_CACHE_STATS)
    GB_HASH *not_reused; // key = DB_path, value = number of cache entries not reused
    GB_HASH *reused;     // key = DB_path, value = number of cache entries reused
    GB_HASH *reuse_sum;  // key = DB_path, value = how often entries were reused
#endif

    gb_cache();
    ~gb_cache();
};

// --------------------------------------------------------------------------------
//      root structure (one for each database)

#define ALLOWED_KEYS  15000
#define ALLOWED_DATES 256

class GB_MAIN_TYPE : virtual Noncopyable {
    inline GB_ERROR start_transaction() __ATTR__USERESULT;
    GB_ERROR check_quick_save() const;
    GB_ERROR initial_client_transaction() __ATTR__USERESULT;

    int transaction_level;
    int aborted_transaction;

    bool i_am_server;

    struct callback_group : virtual Noncopyable {
        gb_hierarchy_callback_list *hierarchy_cbs; // defined hierarchy callbacks
        gb_pending_callbacks        pending;       // collect triggered callbacks (will be called by commit; discarded by abort)

        callback_group() : hierarchy_cbs(NULL) {}

        inline void add_hcb(const gb_hierarchy_location& loc, const TypedDatabaseCallback& dbcb);
        inline void remove_hcb(const gb_hierarchy_location& loc, const TypedDatabaseCallback& dbcb);
        inline void forget_hcbs();

        void trigger(GBDATA *gbd, GB_CB_TYPE type, gb_callback_list *dataCBs);
    };

    callback_group changeCBs; // all but GB_CB_DELETE
    callback_group deleteCBs; // GB_CB_DELETE

public:

    gbcmc_comm     *c_link;
    gb_server_data *server_data;
    GBCONTAINER    *dummy_father;
    GBCONTAINER    *root_container;
    GBCONTAINER    *gb_key_data;
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
    bool     mapped;                                // true -> loaded via mapfile

    unsigned int last_updated;
    long         last_saved_time;
    long         last_saved_transaction;
    long         last_main_saved_transaction;

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

    gb_user *users[GB_MAX_USERS];                   // user 0 is server
    gb_user *this_user;

    // --------------------

private:
    GBCONTAINER*& gb_main_ref() { return root_container; }

    GB_ERROR check_saveable(const char *new_path, const char *flags) const;
    GB_ERROR check_quick_saveable(const char *new_path, const char *flags) const {
        GB_ERROR error = check_quick_save();
        return error ? error : check_saveable(new_path, flags);
    }
public:

    GB_MAIN_TYPE(const char *db_path);
    ~GB_MAIN_TYPE();

    void free_all_keys();
    void release_main_idx();

    int get_transaction_level() const { return transaction_level; }

    GBDATA *gb_main() const { return (GBDATA*)root_container; }

    GB_ERROR login_remote(const char *db_path, const char *opent) __ATTR__USERESULT;

    inline GB_ERROR begin_transaction() __ATTR__USERESULT;
    inline GB_ERROR commit_transaction() __ATTR__USERESULT;
    inline GB_ERROR abort_transaction() __ATTR__USERESULT;

    inline GB_ERROR push_transaction() __ATTR__USERESULT;
    inline GB_ERROR pop_transaction() __ATTR__USERESULT;

    inline GB_ERROR no_transaction();

    __ATTR__USERESULT GB_ERROR send_update_to_server(GBDATA *gbd) __ATTR__USERESULT;

    GB_ERROR save_quick(const char *refpath);

    GB_ERROR save_as(const char *as_path, const char *savetype);
    GB_ERROR save_quick_as(const char *as_path);

    GB_ERROR panic_save(const char *db_panic);

    void mark_as_server() { i_am_server = true; }

    bool is_server() const { return i_am_server; }
    bool is_client() const { return !is_server(); }

    void call_pending_callbacks();

    bool has_pending_change_callback() const { return changeCBs.pending.pending(); }
    bool has_pending_delete_callback() const { return deleteCBs.pending.pending(); }

    GB_ERROR add_hierarchy_cb(const gb_hierarchy_location& loc, const TypedDatabaseCallback& dbcb);
    GB_ERROR remove_hierarchy_cb(const gb_hierarchy_location& loc, const TypedDatabaseCallback& dbcb);
    void forget_hierarchy_cbs();

    inline void trigger_change_callbacks(GBDATA *gbd, GB_CB_TYPE type);
    void trigger_delete_callbacks(GBDATA *gbd);
};

#else
#error gb_main.h included twice
#endif // GB_MAIN_H

