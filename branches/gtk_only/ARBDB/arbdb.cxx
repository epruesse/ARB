// =============================================================== //
//                                                                 //
//   File      : arbdb.cxx                                         //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#if defined(DARWIN)
#include <cstdio>
#endif // DARWIN

#include <rpc/types.h>
#include <rpc/xdr.h>

#include "ad_hcb.h"
#include "gb_comm.h"
#include "gb_compress.h"
#include "gb_localdata.h"
#include "gb_ta.h"
#include "gb_ts.h"
#include "gb_index.h"
#include <arb_strarray.h>

#include <glib.h>
#include <glib/gprintf.h>

gb_local_data *gb_local = 0;

#define INIT_TYPE_NAME(t) GB_TYPES_name[t] = #t

static const char *GB_TYPES_2_name(GB_TYPES type) {
    static const char *GB_TYPES_name[GB_TYPE_MAX];
    static bool        initialized = false;

    if (!initialized) {
        memset(GB_TYPES_name, 0, sizeof(GB_TYPES_name));
        INIT_TYPE_NAME(GB_NONE);
        INIT_TYPE_NAME(GB_BIT);
        INIT_TYPE_NAME(GB_BYTE);
        INIT_TYPE_NAME(GB_INT);
        INIT_TYPE_NAME(GB_FLOAT);
        INIT_TYPE_NAME(GB_POINTER);
        INIT_TYPE_NAME(GB_BITS);
        INIT_TYPE_NAME(GB_BYTES);
        INIT_TYPE_NAME(GB_INTS);
        INIT_TYPE_NAME(GB_FLOATS);
        INIT_TYPE_NAME(GB_LINK);
        INIT_TYPE_NAME(GB_STRING);
        INIT_TYPE_NAME(GB_STRING_SHRT);
        INIT_TYPE_NAME(GB_DB);
        initialized = true;
    }

    const char *name = NULL;
    if (type >= 0 && type<GB_TYPE_MAX) name = GB_TYPES_name[type];
    if (!name) name = GBS_global_string("invalid-type-%i", type);
    return name;
}

inline GB_ERROR gb_transactable_type(GB_TYPES type, GBDATA *gbd) {
    GB_ERROR error = NULL;
    if (GB_MAIN(gbd)->get_transaction_level() == 0) {
        error = "No transaction running";
    }
    else if (GB_ARRAY_FLAGS(gbd).changed == GB_DELETED) {
        error = "Entry has been deleted";
    }
    else {
        GB_TYPES gb_type = gbd->type();
        if (gb_type != type && (type != GB_STRING || gb_type != GB_LINK)) {
            char *rtype    = strdup(GB_TYPES_2_name(type));
            char *rgb_type = strdup(GB_TYPES_2_name(gb_type));
            
            error = GBS_global_string("type mismatch (want='%s', got='%s') in '%s'", rtype, rgb_type, GB_get_db_path(gbd));

            free(rgb_type);
            free(rtype);
        }
    }
    if (error) {
        GBK_dump_backtrace(stderr, error); // it's a bug: none of the above errors should ever happen
        gb_assert(0);
    }
    return error;
}

__ATTR__USERESULT static GB_ERROR gb_security_error(GBDATA *gbd) {
    GB_MAIN_TYPE *Main  = GB_MAIN(gbd);
    const char   *error = GBS_global_string("Protection: Attempt to change a level-%i-'%s'-entry,\n"
                                            "but your current security level is only %i",
                                            GB_GET_SECURITY_WRITE(gbd),
                                            GB_read_key_pntr(gbd),
                                            Main->security_level);
#if defined(DEBUG)
    fprintf(stderr, "%s\n", error);
#endif // DEBUG
    return error;
}

inline GB_ERROR gb_type_writeable_to(GB_TYPES type, GBDATA *gbd) {
    GB_ERROR error = gb_transactable_type(type, gbd);
    if (!error) {
        if (GB_GET_SECURITY_WRITE(gbd) > GB_MAIN(gbd)->security_level) {
            error = gb_security_error(gbd);
        }
    }
    return error;
}
inline GB_ERROR gb_type_readable_from(GB_TYPES type, GBDATA *gbd) {
    return gb_transactable_type(type, gbd);
}

inline GB_ERROR error_with_dbentry(const char *action, GBDATA *gbd, GB_ERROR error) {
    const char *path = GB_get_db_path(gbd);
    return GBS_global_string("Can't %s '%s':\n%s", action, path, error);
}


#define RETURN_ERROR_IF_NOT_WRITEABLE_AS_TYPE(gbd, type)        \
    do {                                                        \
        GB_ERROR error = gb_type_writeable_to(type, gbd);       \
        if (error) {                                            \
            return error_with_dbentry("write", gbd, error);     \
        }                                                       \
    } while(0)

#define EXPORT_ERROR_AND_RETURN_0_IF_NOT_READABLE_AS_TYPE(gbd, type)    \
    do {                                                                \
        GB_ERROR error = gb_type_readable_from(type, gbd);              \
        if (error) {                                                    \
            error = error_with_dbentry("read", gbd, error);             \
            GB_export_error(error);                                     \
            return 0;                                                   \
        }                                                               \
    } while(0)                                                          \


#if defined(WARN_TODO)
#warning replace GB_TEST_READ / GB_TEST_READ by new names later
#endif

#define GB_TEST_READ(gbd, type, ignored) EXPORT_ERROR_AND_RETURN_0_IF_NOT_READABLE_AS_TYPE(gbd, type)
#define GB_TEST_WRITE(gbd, type, ignored) RETURN_ERROR_IF_NOT_WRITEABLE_AS_TYPE(gbd, type)

#define GB_TEST_NON_BUFFER(x, gerror)                                   \
    do {                                                                \
        if (GB_is_in_buffer(x)) {                                       \
            GBK_terminatef("%s: you are not allowed to write any data, which you get by pntr", gerror); \
        }                                                               \
    } while (0)


static GB_ERROR GB_safe_atof(const char *str, double *res) {
    GB_ERROR  error = NULL;
    char     *end;
    *res            = strtod(str, &end);
    if (end == str || end[0] != 0) {
        if (!str[0]) {
            *res = 0.0;
        }
        else {
            error = GBS_global_string("cannot convert '%s' to double", str);
        }
    }
    return error;
}

double GB_atof(const char *str) {
    // convert ASCII to double
    double   res = 0;
    GB_ERROR err = GB_safe_atof(str, &res);
    if (err) {
        // expected double in 'str'- better use GB_safe_atof()
        GBK_terminatef("GB_safe_atof(\"%s\", ..) returns error: %s", str, err);
    }
    return res;
}

// ---------------------------
//      compression tables

const int gb_convert_type_2_compression_flags[] = {
    GB_COMPRESSION_NONE,                                                                 // GB_NONE  0
    GB_COMPRESSION_NONE,                                                                 // GB_BIT   1
    GB_COMPRESSION_NONE,                                                                 // GB_BYTE  2
    GB_COMPRESSION_NONE,                                                                 // GB_INT   3
    GB_COMPRESSION_NONE,                                                                 // GB_FLOAT 4
    GB_COMPRESSION_NONE,                                                                 // GB_??    5
    GB_COMPRESSION_BITS,                                                                 // GB_BITS  6
    GB_COMPRESSION_NONE,                                                                 // GB_??    7
    GB_COMPRESSION_RUNLENGTH | GB_COMPRESSION_HUFFMANN,                                  // GB_BYTES 8
    GB_COMPRESSION_RUNLENGTH | GB_COMPRESSION_HUFFMANN | GB_COMPRESSION_SORTBYTES,       // GB_INTS  9
    GB_COMPRESSION_RUNLENGTH | GB_COMPRESSION_HUFFMANN | GB_COMPRESSION_SORTBYTES,       // GB_FLTS 10
    GB_COMPRESSION_NONE,                                                                 // GB_LINK 11
    GB_COMPRESSION_RUNLENGTH | GB_COMPRESSION_HUFFMANN | GB_COMPRESSION_DICTIONARY,      // GB_STR  12
    GB_COMPRESSION_NONE,                                                                 // GB_STRS 13
    GB_COMPRESSION_NONE,                                                                 // GB??    14
    GB_COMPRESSION_NONE                                                                  // GB_DB   15
};

int gb_convert_type_2_sizeof[] = { /* contains the unit-size of data stored in DB,
                                    * i.e. realsize = unit_size * size()
                                    */
    0,                                              // GB_NONE  0
    0,                                              // GB_BIT   1
    sizeof(char),                                   // GB_BYTE  2
    sizeof(int),                                    // GB_INT   3
    sizeof(float),                                  // GB_FLOAT 4
    0,                                              // GB_??    5
    0,                                              // GB_BITS  6
    0,                                              // GB_??    7
    sizeof(char),                                   // GB_BYTES 8
    sizeof(int),                                    // GB_INTS  9
    sizeof(float),                                  // GB_FLTS 10
    sizeof(char),                                   // GB_LINK 11
    sizeof(char),                                   // GB_STR  12
    sizeof(char),                                   // GB_STRS 13
    0,                                              // GB_??   14
    0,                                              // GB_DB   15
};

int gb_convert_type_2_appendix_size[] = { /* contains the size of the suffix (aka terminator element)
                                           * size is in bytes
                                           */

    0,                                              // GB_NONE  0
    0,                                              // GB_BIT   1
    0,                                              // GB_BYTE  2
    0,                                              // GB_INT   3
    0,                                              // GB_FLOAT 4
    0,                                              // GB_??    5
    0,                                              // GB_BITS  6
    0,                                              // GB_??    7
    0,                                              // GB_BYTES 8
    0,                                              // GB_INTS  9
    0,                                              // GB_FLTS 10
    1,                                              // GB_LINK 11 (zero terminated)
    1,                                              // GB_STR  12 (zero terminated)
    1,                                              // GB_STRS 13 (zero terminated)
    0,                                              // GB_??   14
    0,                                              // GB_DB   15
};


// ---------------------------------
//      local buffer management

static void init_buffer(gb_buffer *buf, size_t initial_size) {
    buf->size = initial_size;
    buf->mem  = buf->size ? (char*)malloc(buf->size) : NULL;
}

static char *check_out_buffer(gb_buffer *buf) {
    char *checkOut = buf->mem;

    buf->mem  = 0;
    buf->size = 0;

    return checkOut;
}

static void alloc_buffer(gb_buffer *buf, size_t size) {
    free(buf->mem);
    buf->size = size;
#if (MEMORY_TEST==1)
    buf->mem  = (char *)malloc(buf->size);
#else
    buf->mem  = (char *)GB_calloc(buf->size, 1);
#endif
}

static GB_BUFFER give_buffer(gb_buffer *buf, size_t size) {
#if (MEMORY_TEST==1)
    alloc_buffer(buf, size); // do NOT reuse buffer if testing memory
#else
    if (size >= buf->size) {
        alloc_buffer(buf, size);
    }
#endif
    return buf->mem;
}

static int is_in_buffer(gb_buffer *buf, GB_CBUFFER ptr) {
    return ptr >= buf->mem && ptr < buf->mem+buf->size;
}

// ------------------------------

GB_BUFFER GB_give_buffer(size_t size) {
    // return a pointer to a static piece of memory at least size bytes long
    return give_buffer(&gb_local->buf1, size);
}

GB_BUFFER GB_increase_buffer(size_t size) {
    if (size < gb_local->buf1.size) {
        char   *old_buffer = gb_local->buf1.mem;
        size_t  old_size   = gb_local->buf1.size;

        gb_local->buf1.mem = NULL;
        alloc_buffer(&gb_local->buf1, size);
        memcpy(gb_local->buf1.mem, old_buffer, old_size);

        free(old_buffer);
    }
    return gb_local->buf1.mem;
}

NOT4PERL int GB_give_buffer_size() {
    return gb_local->buf1.size;
}

GB_BUFFER GB_give_buffer2(long size) {
    return give_buffer(&gb_local->buf2, size);
}

static int GB_is_in_buffer(GB_CBUFFER ptr) {
    /* returns 1 or 2 if 'ptr' points to gb_local->buf1/buf2
     * returns 0 otherwise
     */
    int buffer = 0;

    if (is_in_buffer(&gb_local->buf1, ptr)) buffer = 1;
    else if (is_in_buffer(&gb_local->buf2, ptr)) buffer = 2;

    return buffer;
}

char *GB_check_out_buffer(GB_CBUFFER buffer) {
    /* Check a piece of memory out of the buffer management
     * after it is checked out, the user has the full control to use and free it
     * Returns a pointer to the start of the buffer (even if 'buffer' points inside the buffer!)
     */
    char *old = 0;

    if (is_in_buffer(&gb_local->buf1, buffer)) old = check_out_buffer(&gb_local->buf1);
    else if (is_in_buffer(&gb_local->buf2, buffer)) old = check_out_buffer(&gb_local->buf2);

    return old;
}

GB_BUFFER GB_give_other_buffer(GB_CBUFFER buffer, long size) {
    return is_in_buffer(&gb_local->buf1, buffer)
        ? GB_give_buffer2(size)
        : GB_give_buffer(size);
}

static unsigned char GB_BIT_compress_data[] = {
    0x1d, GB_CS_OK,  0, 0,
    0x04, GB_CS_OK,  0, 1,
    0x0a, GB_CS_OK,  0, 2,
    0x0b, GB_CS_OK,  0, 3,
    0x0c, GB_CS_OK,  0, 4,
    0x1a, GB_CS_OK,  0, 5,
    0x1b, GB_CS_OK,  0, 6,
    0x1c, GB_CS_OK,  0, 7,
    0xf0, GB_CS_OK,  0, 8,
    0xf1, GB_CS_OK,  0, 9,
    0xf2, GB_CS_OK,  0, 10,
    0xf3, GB_CS_OK,  0, 11,
    0xf4, GB_CS_OK,  0, 12,
    0xf5, GB_CS_OK,  0, 13,
    0xf6, GB_CS_OK,  0, 14,
    0xf7, GB_CS_OK,  0, 15,
    0xf8, GB_CS_SUB, 0, 16,
    0xf9, GB_CS_SUB, 0, 32,
    0xfa, GB_CS_SUB, 0, 48,
    0xfb, GB_CS_SUB, 0, 64,
    0xfc, GB_CS_SUB, 0, 128,
    0xfd, GB_CS_SUB, 1, 0,
    0xfe, GB_CS_SUB, 2, 0,
    0xff, GB_CS_SUB, 4, 0,
    0
};

struct gb_exitfun {
    void (*exitfun)();
    gb_exitfun *next;
};

void GB_atexit(void (*exitfun)()) {
    // called when GB_shell is destroyed (use similar to atexit()) 
    //
    // Since the program does not neccessarily terminate, your code calling
    // GB_atexit() may run multiple times. Make sure everything is completely reset by your 'exitfun'

    gb_exitfun *fun = new gb_exitfun;
    fun->exitfun    = exitfun;

    fun->next          = gb_local->atgbexit;
    gb_local->atgbexit = fun;
}

static void run_and_destroy_exit_functions(gb_exitfun *fun) {
    if (fun) {
        fun->exitfun();
        run_and_destroy_exit_functions(fun->next);
        delete fun;
    }
}

static void GB_exit_gb() {
    GB_shell::ensure_inside();

    if (gb_local) {
        gb_local->~gb_local_data(); // inplace-dtor
        gbm_free_mem(gb_local, sizeof(*gb_local), 0);
        gb_local = NULL;
        gbm_flush_mem();
    }
}

gb_local_data::~gb_local_data() {
    gb_assert(openedDBs == closedDBs);

    run_and_destroy_exit_functions(atgbexit);

    free(bitcompress);
    gb_free_compress_tree(bituncompress);
    free(write_buffer);

    free(check_out_buffer(&buf2));
    free(check_out_buffer(&buf1));
    free(open_gb_mains);
}

// -----------------
//      GB_shell


static GB_shell *inside_shell = NULL;

GB_shell::GB_shell() {
    if (inside_shell) GBK_terminate("only one GB_shell allowed");
    inside_shell = this;
}
GB_shell::~GB_shell() {
    gb_assert(inside_shell == this);
    GB_exit_gb();
    inside_shell = NULL;
}
void GB_shell::ensure_inside()  { if (!inside_shell) GBK_terminate("Not inside GB_shell"); }

bool GB_shell::in_shell() { // used by code based on ARBDB (Kai IIRC)
    return inside_shell;
}

struct GB_test_shell_closed {
    ~GB_test_shell_closed() {
        if (GB_shell::in_shell()) { // leave that call
            inside_shell->~GB_shell(); // call dtor
        }
    }
};
static GB_test_shell_closed test;

#if defined(UNIT_TESTS)
static bool closed_open_shell_for_unit_tests() {
    bool was_open = inside_shell;
    if (was_open) {
        if (gb_local) gb_local->fake_closed_DBs();
        inside_shell->~GB_shell(); // just call dtor (not delete)
    }
    return was_open;
}
#endif

void GB_init_gb() {
    GB_shell::ensure_inside();
    if (!gb_local) {
        GBK_install_SIGSEGV_handler(true);          // never uninstalled
        gbm_init_mem();
        gb_local = (gb_local_data *)gbm_get_mem(sizeof(gb_local_data), 0);
        ::new(gb_local) gb_local_data(); // inplace-ctor
    }
}

int GB_open_DBs() { return gb_local ? gb_local->open_dbs() : 0; }

gb_local_data::gb_local_data()
{
    init_buffer(&buf1, 4000);
    init_buffer(&buf2, 4000);

    write_bufsize = GBCM_BUFFER;
    write_buffer  = (char *)malloc((size_t)write_bufsize);
    write_ptr     = write_buffer;
    write_free    = write_bufsize;

    bituncompress = gb_build_uncompress_tree(GB_BIT_compress_data, 1, 0);
    bitcompress   = gb_build_compress_list(GB_BIT_compress_data, 1, &(bc_size));

    openedDBs = 0;
    closedDBs = 0;

    open_gb_mains = NULL;
    open_gb_alloc = 0;

    atgbexit = NULL;

    iamclient                  = false;
    search_system_folder       = false;
    running_client_transaction = ARB_NO_TRANS;
}

void gb_local_data::announce_db_open(GB_MAIN_TYPE *Main) {
    gb_assert(Main);
    int idx = open_dbs();
    if (idx >= open_gb_alloc) {
        int            new_alloc = open_gb_alloc+10;
        GB_MAIN_TYPE **new_mains = (GB_MAIN_TYPE**)realloc(open_gb_mains, new_alloc*sizeof(*new_mains));
        memset(new_mains+open_gb_alloc, 0, 10*sizeof(*new_mains));
        open_gb_alloc            = new_alloc;
        open_gb_mains            = new_mains;
    }
    open_gb_mains[idx] = Main;
    openedDBs++;
}

void gb_local_data::announce_db_close(GB_MAIN_TYPE *Main) {
    gb_assert(Main);
    int open = open_dbs();
    int idx;
    for (idx = 0; idx<open; ++idx) if (open_gb_mains[idx] == Main) break;

    gb_assert(idx<open); // passed gb_main is unknown
    if (idx<open) {
        if (idx<(open-1)) { // not last
            open_gb_mains[idx] = open_gb_mains[open-1];
        }
        closedDBs++;
    }
    if (closedDBs == openedDBs) {
        GB_exit_gb(); // free most memory allocated by ARBDB library
        // Caution: calling GB_exit_gb() frees 'this'!
    }
}

static GBDATA *gb_remembered_db() {
    GB_MAIN_TYPE *Main = gb_local ? gb_local->get_any_open_db() : NULL;
    return Main ? Main->gb_main() : NULL;
}

GB_ERROR gb_unfold(GBCONTAINER *gbc, long deep, int index_pos) {
    /*! get data from server.
     *
     * @param gbc container to unfold
     * @param deep if != 0, then get subitems too.
     * @param index_pos
     * - >= 0, get indexed item from server
     * - <0, get all items
     *
     * @return error on failure
     */

    GB_ERROR        error;
    gb_header_list *header = GB_DATA_LIST_HEADER(gbc->d);

    if (!gbc->flags2.folded_container) return 0;
    if (index_pos> gbc->d.nheader) gb_create_header_array(gbc, index_pos + 1);
    if (index_pos >= 0  && GB_HEADER_LIST_GBD(header[index_pos])) return 0;

    if (GBCONTAINER_MAIN(gbc)->is_server()) {
        GB_internal_error("Cannot unfold in server");
        return 0;
    }

    do {
        if (index_pos<0) break;
        if (index_pos >= gbc->d.nheader) break;
        if (header[index_pos].flags.changed >= GB_DELETED) {
            GB_internal_error("Tried to unfold a deleted item");
            return 0;
        }
        if (GB_HEADER_LIST_GBD(header[index_pos])) return 0;            // already unfolded
    } while (0);

    error = gbcm_unfold_client(gbc, deep, index_pos);
    if (error) {
        GB_print_error();
        return error;
    }

    if (index_pos<0) {
        gb_untouch_children(gbc);
        gbc->flags2.folded_container = 0;
    }
    else {
        GBDATA *gb2 = GBCONTAINER_ELEM(gbc, index_pos);
        if (gb2) {
            if (gb2->is_container()) {
                gb_untouch_children_and_me(gb2->as_container());
            }
            else {
                gb_untouch_me(gb2->as_entry());
            }
        }
    }
    return 0;
}

// -----------------------
//      close database

typedef void (*gb_close_callback)(GBDATA *gb_main, void *client_data);

struct gb_close_callback_list {
    gb_close_callback_list *next;
    gb_close_callback       cb;
    void                   *client_data;
};

#if defined(ASSERTION_USED)
static bool atclose_cb_exists(gb_close_callback_list *gccs, gb_close_callback cb) {
    return gccs && (gccs->cb == cb || atclose_cb_exists(gccs->next, cb));
}
#endif // ASSERTION_USED

void GB_atclose(GBDATA *gbd, void (*fun)(GBDATA *gb_main, void *client_data), void *client_data) {
    /*! Add a callback, which gets called directly before GB_close destroys all data.
     * This is the recommended way to remove all callbacks from DB elements.
     */

    GB_MAIN_TYPE *Main = GB_MAIN(gbd);

    gb_assert(!atclose_cb_exists(Main->close_callbacks, fun)); // each close callback should only exist once

    gb_close_callback_list *gccs = (gb_close_callback_list *)malloc(sizeof(*gccs));

    gccs->next        = Main->close_callbacks;
    gccs->cb          = fun;
    gccs->client_data = client_data;

    Main->close_callbacks = gccs;
}

static void run_close_callbacks(GBDATA *gb_main, gb_close_callback_list *gccs) {
    while (gccs) {
        gccs->cb(gb_main, gccs->client_data);
        gb_close_callback_list *next = gccs->next;
        free(gccs);
        gccs = next;
    }
}

static gb_triggered_callback *currently_called_back = NULL; // points to callback during callback; NULL otherwise
static GB_MAIN_TYPE          *inside_callback_main  = NULL; // points to DB root during callback; NULL otherwise

void gb_pending_callbacks::call_and_forget(GB_CB_TYPE allowedTypes) {
#if defined(ASSERTION_USED)
    const gb_triggered_callback *tail = get_tail();
#endif

    for (itertype cb = callbacks.begin(); cb != callbacks.end(); ++cb) {
        currently_called_back = &*cb;
        gb_assert(currently_called_back);
        currently_called_back->spec(cb->gbd, allowedTypes);
        currently_called_back = NULL;
    }

    gb_assert(tail == get_tail());

    callbacks.clear();
}

void GB_MAIN_TYPE::call_pending_callbacks() {
    inside_callback_main = this;

    deleteCBs.pending.call_and_forget(GB_CB_DELETE);         // first all delete callbacks:
    changeCBs.pending.call_and_forget(GB_CB_ALL_BUT_DELETE); // then all change callbacks:

    inside_callback_main = NULL;
}

inline void GB_MAIN_TYPE::callback_group::forget_hcbs() {
    delete hierarchy_cbs;
    hierarchy_cbs = NULL;
}

void GB_MAIN_TYPE::forget_hierarchy_cbs() {
    changeCBs.forget_hcbs();
    deleteCBs.forget_hcbs();
}

void GB_close(GBDATA *gbd) {
    GB_ERROR      error = NULL;
    GB_MAIN_TYPE *Main  = GB_MAIN(gbd);

    gb_assert(Main->get_transaction_level() <= 0); // transaction running - you can't close DB yet!

    Main->forget_hierarchy_cbs();

    gb_assert(Main->gb_main() == gbd);
    run_close_callbacks(gbd, Main->close_callbacks);
    Main->close_callbacks = 0;

    if (Main->is_client()) {
        long result            = gbcmc_close(Main->c_link);
        if (result != 0) error = GBS_global_string("gbcmc_close returns %li", result);
    }

    gbcm_logout(Main, NULL);                        // logout default user
    
    if (!error) {
        gb_assert(Main->close_callbacks == 0);

        gb_delete_dummy_father(Main->dummy_father);
        Main->root_container = NULL;

        /* ARBDB applications using awars easily crash in call_pending_callbacks(),
         * if AWARs are still bound to elements in the closed database.
         *
         * To unlink awars call AW_root::unlink_awars_from_DB().
         * If that doesn't help, test Main->data (often aka as GLOBAL_gb_main)
         */
        Main->call_pending_callbacks();                  // do all callbacks
        delete Main;
    }

    if (error) {
        GB_warningf("Error in GB_close: %s", error);
    }
}

void gb_close_unclosed_DBs() {
    GBDATA *gb_main;
    while ((gb_main = gb_remembered_db())) {
        GB_close(gb_main);
    }
}

// ------------------
//      read data

long GB_read_int(GBDATA *gbd)
{
    GB_TEST_READ(gbd, GB_INT, "GB_read_int");
    return gbd->as_entry()->info.i;
}

int GB_read_byte(GBDATA *gbd)
{
    GB_TEST_READ(gbd, GB_BYTE, "GB_read_byte");
    return gbd->as_entry()->info.i;
}

GBDATA *GB_read_pointer(GBDATA *gbd) {
    GB_TEST_READ(gbd, GB_POINTER, "GB_read_pointer");
    return gbd->as_entry()->info.ptr;
}

double GB_read_float(GBDATA *gbd) // @@@ change return type to float (that's what's stored in DB)
{
    XDR          xdrs;
    static float f; // @@@ why static?
    
    GB_TEST_READ(gbd, GB_FLOAT, "GB_read_float");
    xdrmem_create(&xdrs, &gbd->as_entry()->info.in.data[0], SIZOFINTERN, XDR_DECODE);
    xdr_float(&xdrs, &f);
    xdr_destroy(&xdrs);

    gb_assert(f == f); // !nan

    return (double)f;
}

long GB_read_count(GBDATA *gbd) {
    return gbd->as_entry()->size();
}

long GB_read_memuse(GBDATA *gbd) {
    return gbd->as_entry()->memsize();
}

#if defined(DEBUG)

#define STD_LIST_NODE_NAME _List_node

inline long calc_size(gb_callback_list *gbcbl) {
    return gbcbl
        ? sizeof(*gbcbl) + gbcbl->callbacks.size()* sizeof(std::STD_LIST_NODE_NAME<gb_callback_list::cbtype>)
        : 0;
}
inline long calc_size(gb_transaction_save *gbts) {
    return gbts
        ? sizeof(*gbts)
        : 0;
}
inline long calc_size(gb_if_entries *gbie) {
    return gbie
        ? sizeof(*gbie) + calc_size(GB_IF_ENTRIES_NEXT(gbie))
        : 0;
}
inline long calc_size(GB_REL_IFES *gbri, int table_size) {
    long size = 0;

    gb_if_entries *ifes;
    for (int idx = 0; idx<table_size; ++idx) {
        for (ifes = GB_ENTRIES_ENTRY(gbri, idx);
             ifes;
             ifes = GB_IF_ENTRIES_NEXT(ifes))
        {
            size += calc_size(ifes);
        }
    }
    return size;
}
inline long calc_size(gb_index_files *gbif) {
    return gbif
        ? sizeof(*gbif) + calc_size(GB_INDEX_FILES_NEXT(gbif)) + calc_size(GB_INDEX_FILES_ENTRIES(gbif), gbif->hash_table_size)
        : 0;
}
inline long calc_size(gb_db_extended *gbe) {
    return gbe
        ? sizeof(*gbe) + calc_size(gbe->callback) + calc_size(gbe->old)
        : 0;
}
inline long calc_size(GBENTRY *gbe) {
    return gbe
        ? sizeof(*gbe) + calc_size(gbe->ext)
        : 0;
}
inline long calc_size(GBCONTAINER *gbc) {
    return gbc
        ? sizeof(*gbc) + calc_size(gbc->ext) + calc_size(GBCONTAINER_IFS(gbc))
        : 0;
}

long GB_calc_structure_size(GBDATA *gbd) {
    long size = 0;
    if (gbd->is_container()) {
        size = calc_size(gbd->as_container());
    }
    else {
        size = calc_size(gbd->as_entry());
    }
    return size;
}

void GB_SizeInfo::collect(GBDATA *gbd) {
    if (gbd->is_container()) {
        ++containers;
        for (GBDATA *gb_child = GB_child(gbd); gb_child; gb_child = GB_nextChild(gb_child)) {
            collect(gb_child);
        }
    }
    else {
        ++terminals;
        mem += GB_read_memuse(gbd);

        long size;
        switch (gbd->type()) {
            case GB_INT:    size = sizeof(int); break;
            case GB_FLOAT:  size = sizeof(float); break;
            case GB_BYTE:   size = sizeof(char); break;
            case GB_STRING: size = GB_read_count(gbd); break; // accept 0 sized data for strings

            default:
                size = GB_read_count(gbd);
                gb_assert(size>0);                            // terminal w/o data - really?
                break;
        }
        data += size;
    }
    structure += GB_calc_structure_size(gbd);
}
#endif

GB_CSTR GB_read_pntr(GBDATA *gbd) {
    GBENTRY    *gbe  = gbd->as_entry();
    const char *data = gbe->data();

    if (data) {
        if (gbe->flags.compressed_data) {   // uncompressed data return pntr to database entry
            char *ca = gb_read_cache(gbe);

            if (!ca) {
                size_t      size = gbe->uncompressed_size();
                const char *da   = gb_uncompress_data(gbe, data, size);

                if (da) {
                    ca = gb_alloc_cache_index(gbe, size);
                    memcpy(ca, da, size);
                }
            }
            data = ca;
        }
    }
    return data;
}

int gb_read_nr(GBDATA *gbd) {
    return gbd->index;
}

GB_CSTR GB_read_char_pntr(GBDATA *gbd) {
    GB_TEST_READ(gbd, GB_STRING, "GB_read_char_pntr");
    return GB_read_pntr(gbd);
}

char *GB_read_string(GBDATA *gbd) {
    GB_TEST_READ(gbd, GB_STRING, "GB_read_string");
    const char *d = GB_read_pntr(gbd);
    if (!d) return NULL;
    return GB_memdup(d, gbd->as_entry()->size()+1);
}

size_t GB_read_string_count(GBDATA *gbd) {
    GB_TEST_READ(gbd, GB_STRING, "GB_read_string_count");
    return gbd->as_entry()->size();
}

GB_CSTR GB_read_link_pntr(GBDATA *gbd) {
    GB_TEST_READ(gbd, GB_LINK, "GB_read_link_pntr");
    return GB_read_pntr(gbd);
}

static char *GB_read_link(GBDATA *gbd) {
    const char *d;
    GB_TEST_READ(gbd, GB_LINK, "GB_read_link_pntr");
    d = GB_read_pntr(gbd);
    if (!d) return NULL;
    return GB_memdup(d, gbd->as_entry()->size()+1);
}

long GB_read_bits_count(GBDATA *gbd) {
    GB_TEST_READ(gbd, GB_BITS, "GB_read_bits_count");
    return gbd->as_entry()->size();
}

GB_CSTR GB_read_bits_pntr(GBDATA *gbd, char c_0, char c_1) {
    GB_TEST_READ(gbd, GB_BITS, "GB_read_bits_pntr");
    GBENTRY *gbe  = gbd->as_entry();
    long     size = gbe->size();
    if (size) {
        char *ca = gb_read_cache(gbe);
        if (ca) return ca;

        ca               = gb_alloc_cache_index(gbe, size+1);
        const char *data = gbe->data();
        char       *da   = gb_uncompress_bits(data, size, c_0, c_1);
        if (ca) {
            memcpy(ca, da, size+1);
            return ca;
        }
        return da;
    }
    return 0;
}

char *GB_read_bits(GBDATA *gbd, char c_0, char c_1) {
    GB_CSTR d = GB_read_bits_pntr(gbd, c_0, c_1);
    return d ? GB_memdup(d, gbd->as_entry()->size()+1) : 0;
}


GB_CSTR GB_read_bytes_pntr(GBDATA *gbd)
{
    GB_TEST_READ(gbd, GB_BYTES, "GB_read_bytes_pntr");
    return GB_read_pntr(gbd);
}

long GB_read_bytes_count(GBDATA *gbd)
{
    GB_TEST_READ(gbd, GB_BYTES, "GB_read_bytes_count");
    return gbd->as_entry()->size();
}

char *GB_read_bytes(GBDATA *gbd) {
    GB_CSTR d = GB_read_bytes_pntr(gbd);
    return d ? GB_memdup(d, gbd->as_entry()->size()) : 0;
}

GB_CUINT4 *GB_read_ints_pntr(GBDATA *gbd)
{
    GB_TEST_READ(gbd, GB_INTS, "GB_read_ints_pntr");
    GBENTRY *gbe = gbd->as_entry();

    GB_UINT4 *res;
    if (gbe->flags.compressed_data) {
        res = (GB_UINT4 *)GB_read_pntr(gbe);
    }
    else {
        res = (GB_UINT4 *)gbe->data();
    }
    if (!res) return NULL;

    if (0x01020304U == htonl(0x01020304U)) {
        return res;
    }
    else {
        int       size = gbe->size();
        char     *buf2 = GB_give_other_buffer((char *)res, size<<2);
        GB_UINT4 *s    = (GB_UINT4 *)res;
        GB_UINT4 *d    = (GB_UINT4 *)buf2;

        for (long i=size; i; i--) {
            *(d++) = htonl(*(s++));
        }
        return (GB_UINT4 *)buf2;
    }
}

long GB_read_ints_count(GBDATA *gbd) { // used by ../PERL_SCRIPTS/SAI/SAI.pm@read_ints_count
    GB_TEST_READ(gbd, GB_INTS, "GB_read_ints_count");
    return gbd->as_entry()->size();
}

GB_UINT4 *GB_read_ints(GBDATA *gbd)
{
    GB_CUINT4 *i = GB_read_ints_pntr(gbd);
    if (!i) return NULL;
    return  (GB_UINT4 *)GB_memdup((char *)i, gbd->as_entry()->size()*sizeof(GB_UINT4));
}

GB_CFLOAT *GB_read_floats_pntr(GBDATA *gbd)
{
    GB_TEST_READ(gbd, GB_FLOATS, "GB_read_floats_pntr");
    GBENTRY *gbe = gbd->as_entry();
    char    *res;
    if (gbe->flags.compressed_data) {
        res = (char *)GB_read_pntr(gbe);
    }
    else {
        res = (char *)gbe->data();
    }
    if (res) {
        long size      = gbe->size();
        long full_size = size*sizeof(float);

        XDR xdrs;
        xdrmem_create(&xdrs, res, (int)(full_size), XDR_DECODE);

        char  *buf2 = GB_give_other_buffer(res, full_size);
        float *d    = (float *)(void*)buf2;
        for (long i=size; i; i--) {
            xdr_float(&xdrs, d);
            d++;
        }
        xdr_destroy(&xdrs);
        return (float *)(void*)buf2;
    }
    return NULL;
}

static long GB_read_floats_count(GBDATA *gbd)
{
    GB_TEST_READ(gbd, GB_FLOATS, "GB_read_floats_count");
    return gbd->as_entry()->size();
}

float *GB_read_floats(GBDATA *gbd) { // @@@ only used in unittest - check usage of floats
    GB_CFLOAT *f;
    f = GB_read_floats_pntr(gbd);
    if (!f) return NULL;
    return  (float *)GB_memdup((char *)f, gbd->as_entry()->size()*sizeof(float));
}

char *GB_read_as_string(GBDATA *gbd) {
    switch (gbd->type()) {
        case GB_STRING: return GB_read_string(gbd);
        case GB_LINK:   return GB_read_link(gbd);
        case GB_BYTE:   return GBS_global_string_copy("%i", GB_read_byte(gbd));
        case GB_INT:    return GBS_global_string_copy("%li", GB_read_int(gbd));
        case GB_FLOAT:  return GBS_global_string_copy("%g", GB_read_float(gbd));
        case GB_BITS:   return GB_read_bits(gbd, '0', '1');
            /* Be careful : When adding new types here, you have to make sure that
             * GB_write_as_string is able to write them back and that this makes sense.
             */
        default:    return NULL;
    }
}

// ------------------------------------------------------------
//      array type access functions (intended for perl use)

long GB_read_from_ints(GBDATA *gbd, long index) { // used by ../PERL_SCRIPTS/SAI/SAI.pm@read_from_ints
    static GBDATA    *last_gbd = 0;
    static long       count    = 0;
    static GB_CUINT4 *i        = 0;

    if (gbd != last_gbd) {
        count    = GB_read_ints_count(gbd);
        i        = GB_read_ints_pntr(gbd);
        last_gbd = gbd;
    }

    if (index >= 0 && index < count) {
        return i[index];
    }
    return -1;
}

double GB_read_from_floats(GBDATA *gbd, long index) { // @@@ unused
    static GBDATA    *last_gbd = 0;
    static long       count    = 0;
    static GB_CFLOAT *f        = 0;

    if (gbd != last_gbd) {
        count    = GB_read_floats_count(gbd);
        f        = GB_read_floats_pntr(gbd);
        last_gbd = gbd;
    }

    if (index >= 0 && index < count) {
        return f[index];
    }
    return -1;
}

// -------------------
//      write data

static void gb_remove_callbacks_marked_for_deletion(GBDATA *gbd);

static void gb_do_callbacks(GBDATA *gbd) {
    gb_assert(GB_MAIN(gbd)->get_transaction_level() < 0); // only use in NO_TRANSACTION_MODE!

    while (gbd) {
        GBDATA *gbdn = GB_get_father(gbd);
        gb_callback_list *cbl = gbd->get_callbacks();
        if (cbl && cbl->call(gbd, GB_CB_CHANGED)) {
            gb_remove_callbacks_marked_for_deletion(gbd);
        }
        gbd = gbdn;
    }
}

#define GB_DO_CALLBACKS(gbd) do { if (GB_MAIN(gbd)->get_transaction_level() < 0) gb_do_callbacks(gbd); } while (0)

GB_ERROR GB_write_byte(GBDATA *gbd, int i)
{
    GB_TEST_WRITE(gbd, GB_BYTE, "GB_write_byte");
    GBENTRY *gbe = gbd->as_entry();
    if (gbe->info.i != i) {
        gb_save_extern_data_in_ts(gbe);
        gbe->info.i = i & 0xff;
        gb_touch_entry(gbe, GB_NORMAL_CHANGE);
        GB_DO_CALLBACKS(gbe);
    }
    return 0;
}

GB_ERROR GB_write_int(GBDATA *gbd, long i) {
#if defined(ARB_64)
#if defined(WARN_TODO)
#warning GB_write_int should be GB_ERROR GB_write_int(GBDATA *gbd,int32_t i)
#endif
#endif

    GB_TEST_WRITE(gbd, GB_INT, "GB_write_int");
    if ((long)((int32_t)i) != i) {
        gb_assert(0);
        GB_warningf("Warning: 64bit incompatibility detected\nNo data written to '%s'\n", GB_get_db_path(gbd));
        return "GB_INT out of range (signed, 32bit)";
    }
    GBENTRY *gbe = gbd->as_entry();
    if (gbe->info.i != (int32_t)i) {
        gb_save_extern_data_in_ts(gbe);
        gbe->info.i = i;
        gb_touch_entry(gbe, GB_NORMAL_CHANGE);
        GB_DO_CALLBACKS(gbe);
    }
    return 0;
}

GB_ERROR GB_write_pointer(GBDATA *gbd, GBDATA *pointer) {
    GB_TEST_WRITE(gbd, GB_POINTER, "GB_write_pointer");
    GBENTRY *gbe = gbd->as_entry();
    if (gbe->info.ptr != pointer) {
        gb_save_extern_data_in_ts(gbe);
        gbe->info.ptr = pointer;
        gb_touch_entry(gbe, GB_NORMAL_CHANGE);
        GB_DO_CALLBACKS(gbe);
    }
    return 0;
}

GB_ERROR GB_write_float(GBDATA *gbd, double f)
{
    gb_assert(f == f); // !nan

    XDR          xdrs;
    static float f2;

    GB_TEST_WRITE(gbd, GB_FLOAT, "GB_write_float");

#if defined(WARN_TODO)
#warning call GB_read_float here!
#endif
    GB_TEST_READ(gbd, GB_FLOAT, "GB_read_float");

    GBENTRY *gbe = gbd->as_entry();
    xdrmem_create(&xdrs, &gbe->info.in.data[0], SIZOFINTERN, XDR_DECODE);
    xdr_float(&xdrs, &f2);
    xdr_destroy(&xdrs);

    if (f2 != f) {
        f2 = f;
        gb_save_extern_data_in_ts(gbe);
        xdrmem_create(&xdrs, &gbe->info.in.data[0], SIZOFINTERN, XDR_ENCODE);
        xdr_float(&xdrs, &f2);
        xdr_destroy(&xdrs);
        gb_touch_entry(gbe, GB_NORMAL_CHANGE);
        GB_DO_CALLBACKS(gbe);
    }
    xdr_destroy(&xdrs);
    return 0;
}

GB_ERROR gb_write_compressed_pntr(GBENTRY *gbe, const char *s, long memsize, long stored_size) {
    gb_uncache(gbe);
    gb_save_extern_data_in_ts(gbe);
    gbe->flags.compressed_data = 1;
    gbe->insert_data((char *)s, stored_size, (size_t)memsize);
    gb_touch_entry(gbe, GB_NORMAL_CHANGE);

    return 0;
}

int gb_get_compression_mask(GB_MAIN_TYPE *Main, GBQUARK key, int gb_type) {
    gb_Key *ks = &Main->keys[key];
    int     compression_mask;

    if (ks->gb_key_disabled) {
        compression_mask = 0;
    }
    else {
        if (!ks->gb_key) gb_load_single_key_data(Main->gb_main(), key);
        compression_mask = gb_convert_type_2_compression_flags[gb_type] & ks->compression_mask;
    }

    return compression_mask;
}

GB_ERROR GB_write_pntr(GBDATA *gbd, const char *s, size_t bytes_size, size_t stored_size)
{
    // 'bytes_size' is the size of what 's' points to.
    // 'stored_size' is the size-information written into the DB
    //
    // e.g. for strings : stored_size = bytes_size-1, cause stored_size is string len,
    //                    but bytes_size includes zero byte.

    GBENTRY      *gbe  = gbd->as_entry();
    GB_MAIN_TYPE *Main = GB_MAIN(gbe);
    GBQUARK       key  = GB_KEY_QUARK(gbe);
    GB_TYPES      type = gbe->type();

    gb_assert(implicated(type == GB_STRING, stored_size == bytes_size-1)); // size constraint for strings not fulfilled!

    gb_uncache(gbe);
    gb_save_extern_data_in_ts(gbe);

    int compression_mask = gb_get_compression_mask(Main, key, type);

    const char *d;
    size_t      memsize;
    if (compression_mask) {
        d = gb_compress_data(gbe, key, s, bytes_size, &memsize, compression_mask, false);
    }
    else {
        d = NULL;
    }
    if (d) {
        gbe->flags.compressed_data = 1;
    }
    else {
        d = s;
        gbe->flags.compressed_data = 0;
        memsize = bytes_size;
    }

    gbe->insert_data(d, stored_size, memsize);
    gb_touch_entry(gbe, GB_NORMAL_CHANGE);
    GB_DO_CALLBACKS(gbe);

    return 0;
}

GB_ERROR GB_write_string(GBDATA *gbd, const char *s)
{
    GBENTRY *gbe = gbd->as_entry();
    GB_TEST_WRITE(gbe, GB_STRING, "GB_write_string");
    GB_TEST_NON_BUFFER(s, "GB_write_string");        // compress would destroy the other buffer

    if (!s) s = "";
    size_t size = strlen(s);

    // no zero len strings allowed
    if (gbe->memsize() && (size == gbe->size()))
    {
        if (!strcmp(s, GB_read_pntr(gbe)))
            return 0;
    }
#if defined(DEBUG) && 0
    // check for error (in compression)
    {
        GB_ERROR error = GB_write_pntr(gbe, s, size+1, size);
        if (!error) {
            char *check = GB_read_string(gbe);

            gb_assert(check);
            gb_assert(strcmp(check, s) == 0);

            free(check);
        }
        return error;
    }
#else
    return GB_write_pntr(gbe, s, size+1, size);
#endif // DEBUG
}

GB_ERROR GB_write_link(GBDATA *gbd, const char *s)
{
    GBENTRY *gbe = gbd->as_entry();
    GB_TEST_WRITE(gbe, GB_STRING, "GB_write_link");
    GB_TEST_NON_BUFFER(s, "GB_write_link");          // compress would destroy the other buffer

    if (!s) s = "";
    size_t size = strlen(s);

    // no zero len strings allowed
    if (gbe->memsize()  && (size == gbe->size()))
    {
        if (!strcmp(s, GB_read_pntr(gbe)))
            return 0;
    }
    return GB_write_pntr(gbe, s, size+1, size);
}


GB_ERROR GB_write_bits(GBDATA *gbd, const char *bits, long size, const char *c_0)
{
    GBENTRY *gbe = gbd->as_entry();
    GB_TEST_WRITE(gbe, GB_BITS, "GB_write_bits");
    GB_TEST_NON_BUFFER(bits, "GB_write_bits");       // compress would destroy the other buffer
    gb_save_extern_data_in_ts(gbe);

    long  memsize;
    char *d = gb_compress_bits(bits, size, (const unsigned char *)c_0, &memsize);

    gbe->flags.compressed_data = 1;
    gbe->insert_data(d, size, memsize);
    gb_touch_entry(gbe, GB_NORMAL_CHANGE);
    GB_DO_CALLBACKS(gbe);
    return 0;
}

GB_ERROR GB_write_bytes(GBDATA *gbd, const char *s, long size)
{
    GB_TEST_WRITE(gbd, GB_BYTES, "GB_write_bytes");
    return GB_write_pntr(gbd, s, size, size);
}

GB_ERROR GB_write_ints(GBDATA *gbd, const GB_UINT4 *i, long size)
{

    GB_TEST_WRITE(gbd, GB_INTS, "GB_write_ints");
    GB_TEST_NON_BUFFER((char *)i, "GB_write_ints");  // compress would destroy the other buffer

    if (0x01020304 != htonl((GB_UINT4)0x01020304)) {
        long      j;
        char     *buf2 = GB_give_other_buffer((char *)i, size<<2);
        GB_UINT4 *s    = (GB_UINT4 *)i;
        GB_UINT4 *d    = (GB_UINT4 *)buf2;

        for (j=size; j; j--) {
            *(d++) = htonl(*(s++));
        }
        i = (GB_UINT4 *)buf2;
    }
    return GB_write_pntr(gbd, (char *)i, size* 4 /* sizeof(long4) */, size);
}

GB_ERROR GB_write_floats(GBDATA *gbd, const float *f, long size)
{
    long fullsize = size * sizeof(float);
    GB_TEST_WRITE(gbd, GB_FLOATS, "GB_write_floats");
    GB_TEST_NON_BUFFER((char *)f, "GB_write_floats"); // compress would destroy the other buffer

    {
        XDR    xdrs;
        long   i;
        char  *buf2 = GB_give_other_buffer((char *)f, fullsize);
        float *s    = (float *)f;

        xdrmem_create(&xdrs, buf2, (int)fullsize, XDR_ENCODE);
        for (i=size; i; i--) {
            xdr_float(&xdrs, s);
            s++;
        }
        xdr_destroy (&xdrs);
        f = (float*)(void*)buf2;
    }
    return GB_write_pntr(gbd, (char *)f, size*sizeof(float), size);
}

GB_ERROR GB_write_as_string(GBDATA *gbd, const char *val) {
    switch (gbd->type()) {
        case GB_STRING: return GB_write_string(gbd, val);
        case GB_LINK:   return GB_write_link(gbd, val);
        case GB_BYTE:   return GB_write_byte(gbd, atoi(val));
        case GB_INT:    return GB_write_int(gbd, atoi(val));
        case GB_FLOAT:  return GB_write_float(gbd, GB_atof(val));
        case GB_BITS:   return GB_write_bits(gbd, val, strlen(val), "0");
        default:    return GB_export_errorf("Error: You cannot use GB_write_as_string on this type of entry (%s)", GB_read_key_pntr(gbd));
    }
}

// ---------------------------
//      security functions

int GB_read_security_write(GBDATA *gbd) {
    GB_test_transaction(gbd);
    return GB_GET_SECURITY_WRITE(gbd);
}
int GB_read_security_read(GBDATA *gbd) {
    GB_test_transaction(gbd);
    return GB_GET_SECURITY_READ(gbd);
}
int GB_read_security_delete(GBDATA *gbd) {
    GB_test_transaction(gbd);
    return GB_GET_SECURITY_DELETE(gbd);
}
GB_ERROR GB_write_security_write(GBDATA *gbd, unsigned long level)
{
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    GB_test_transaction(Main);

    if (GB_GET_SECURITY_WRITE(gbd)>Main->security_level)
        return gb_security_error(gbd);
    if (GB_GET_SECURITY_WRITE(gbd) == level) return 0;
    GB_PUT_SECURITY_WRITE(gbd, level);
    gb_touch_entry(gbd, GB_NORMAL_CHANGE);
    GB_DO_CALLBACKS(gbd);
    return 0;
}
GB_ERROR GB_write_security_read(GBDATA *gbd, unsigned long level) // @@@ unused 
{
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    GB_test_transaction(Main);
    if (GB_GET_SECURITY_WRITE(gbd)>Main->security_level)
        return gb_security_error(gbd);
    if (GB_GET_SECURITY_READ(gbd) == level) return 0;
    GB_PUT_SECURITY_READ(gbd, level);
    gb_touch_entry(gbd, GB_NORMAL_CHANGE);
    GB_DO_CALLBACKS(gbd);
    return 0;
}
GB_ERROR GB_write_security_delete(GBDATA *gbd, unsigned long level)
{
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    GB_test_transaction(Main);
    if (GB_GET_SECURITY_WRITE(gbd)>Main->security_level)
        return gb_security_error(gbd);
    if (GB_GET_SECURITY_DELETE(gbd) == level) return 0;
    GB_PUT_SECURITY_DELETE(gbd, level);
    gb_touch_entry(gbd, GB_NORMAL_CHANGE);
    GB_DO_CALLBACKS(gbd);
    return 0;
}
GB_ERROR GB_write_security_levels(GBDATA *gbd, unsigned long readlevel, unsigned long writelevel, unsigned long deletelevel)
{
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    GB_test_transaction(Main);
    if (GB_GET_SECURITY_WRITE(gbd)>Main->security_level)
        return gb_security_error(gbd);
    GB_PUT_SECURITY_WRITE(gbd, writelevel);
    GB_PUT_SECURITY_READ(gbd, readlevel);
    GB_PUT_SECURITY_DELETE(gbd, deletelevel);
    gb_touch_entry(gbd, GB_NORMAL_CHANGE);
    GB_DO_CALLBACKS(gbd);
    return 0;
}

void GB_change_my_security(GBDATA *gbd, int level) {
    GB_MAIN(gbd)->security_level = level<0 ? 0 : (level>7 ? 7 : level);
}

// For internal use only
void GB_push_my_security(GBDATA *gbd)
{
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    Main->pushed_security_level++;
    if (Main->pushed_security_level <= 1) {
        Main->old_security_level = Main->security_level;
        Main->security_level = 7;
    }
}

void GB_pop_my_security(GBDATA *gbd) {
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    Main->pushed_security_level--;
    if (Main->pushed_security_level <= 0) {
        Main->security_level = Main->old_security_level;
    }
}


// ------------------------
//      Key information

GB_TYPES GB_read_type(GBDATA *gbd) {
    GB_test_transaction(gbd);
    return gbd->type();
}

bool GB_is_container(GBDATA *gbd) {
    return gbd && gbd->is_container();
}

char *GB_read_key(GBDATA *gbd) {
    return strdup(GB_read_key_pntr(gbd));
}

GB_CSTR GB_read_key_pntr(GBDATA *gbd) {
    GB_CSTR k;
    GB_test_transaction(gbd);
    k         = GB_KEY(gbd);
    if (!k) k = GBS_global_string("<invalid key (quark=%i)>", GB_KEY_QUARK(gbd));
    return k;
}

GB_CSTR gb_read_key_pntr(GBDATA *gbd) {
    return GB_KEY(gbd);
}


GBQUARK gb_find_existing_quark(GB_MAIN_TYPE *Main, const char *key) {
    //! @return existing quark for 'key' (-1 if key == NULL, 0 if key is unknown)
    return key ? GBS_read_hash(Main->key_2_index_hash, key) : -1;
}

GBQUARK gb_find_or_create_quark(GB_MAIN_TYPE *Main, const char *key) {
    //! @return existing or newly created quark for 'key'
    GBQUARK quark     = gb_find_existing_quark(Main, key);
    if (!quark) quark = gb_create_key(Main, key, true);
    return quark;
}

GBQUARK gb_find_or_create_NULL_quark(GB_MAIN_TYPE *Main, const char *key) {
    // similar to gb_find_or_create_quark,
    // but if 'key' is NULL, quark 0 will be returned.
    //
    // Use this function with care.
    //
    // Known good use:
    // - create main entry and its dummy father via gb_make_container()

    return key ? gb_find_or_create_quark(Main, key) : 0;
}

GBQUARK GB_find_existing_quark(GBDATA *gbd, const char *key) {
    //! @return existing quark for 'key' (-1 if key == NULL, 0 if key is unknown)
    return gb_find_existing_quark(GB_MAIN(gbd), key);
}

GBQUARK GB_find_or_create_quark(GBDATA *gbd, const char *key) {
    //! @return existing or newly created quark for 'key'
    return gb_find_or_create_quark(GB_MAIN(gbd), key);
}


// ---------------------------------------------

GBQUARK GB_get_quark(GBDATA *gbd) {
    return GB_KEY_QUARK(gbd);
}

bool GB_has_key(GBDATA *gbd, const char *key) {
    GBQUARK quark = GB_find_existing_quark(gbd, key); 
    return quark && (quark == GB_get_quark(gbd));
}

// ---------------------------------------------

long GB_read_clock(GBDATA *gbd) {
    if (GB_ARRAY_FLAGS(gbd).changed) return GB_MAIN(gbd)->clock;
    return gbd->update_date();
}

// ---------------------------------------------
//      Get and check the database hierarchy

GBDATA *GB_get_father(GBDATA *gbd) {
    // Get the father of an entry
    GB_test_transaction(gbd);
    return gbd->get_father();
}

GBDATA *GB_get_grandfather(GBDATA *gbd) {
    GB_test_transaction(gbd);

    GBDATA *gb_grandpa = GB_FATHER(gbd);
    if (gb_grandpa) {
        gb_grandpa = GB_FATHER(gb_grandpa);
        if (gb_grandpa && !GB_FATHER(gb_grandpa)) gb_grandpa = NULL; // never return dummy_father of root container
    }
    return gb_grandpa;
}

// Get the root entry (gb_main)
GBDATA *GB_get_root(GBDATA *gbd) { return GB_MAIN(gbd)->gb_main(); }
GBCONTAINER *gb_get_root(GBENTRY *gbe) { return GB_MAIN(gbe)->root_container; }
GBCONTAINER *gb_get_root(GBCONTAINER *gbc) { return GB_MAIN(gbc)->root_container; }

bool GB_check_father(GBDATA *gbd, GBDATA *gb_maybefather) {
    // Test whether an entry is a subentry of another
    GBDATA *gbfather;
    for (gbfather = GB_get_father(gbd);
         gbfather;
         gbfather = GB_get_father(gbfather))
    {
        if (gbfather == gb_maybefather) return true;
    }
    return false;
}

// --------------------------
//      create and rename

GBENTRY *gb_create(GBCONTAINER *father, const char *key, GB_TYPES type) {
    GBENTRY *gbe = gb_make_entry(father, key, -1, 0, type);
    gb_touch_header(GB_FATHER(gbe));
    gb_touch_entry(gbe, GB_CREATED);

    gb_assert(GB_ARRAY_FLAGS(gbe).changed < GB_DELETED); // happens sometimes -> needs debugging

    return gbe;
}

GBCONTAINER *gb_create_container(GBCONTAINER *father, const char *key) {
    // Create a container, do not check anything
    GBCONTAINER *gbc = gb_make_container(father, key, -1, 0);
    gb_touch_header(GB_FATHER(gbc));
    gb_touch_entry(gbc, GB_CREATED);
    return gbc;
}

GBDATA *GB_create(GBDATA *father, const char *key, GB_TYPES type) {
    /*! Create a DB entry
     *
     * @param father container to create DB field in
     * @param key name of field
     * @param type field type
     *
     * @return
     * - created DB entry
     * - NULL on failure (error is exported then)
     *
     * @see GB_create_container()
     */

    if (GB_check_key(key)) {
        GB_print_error();
        return NULL;
    }

    if (type == GB_DB) {
        gb_assert(type != GB_DB); // you like to use GB_create_container!
        GB_export_error("GB_create error: can't create containers");
        return NULL;
    }

    if (!father) {
        GB_internal_errorf("GB_create error in GB_create:\nno father (key = '%s')", key);
        return NULL;
    }
    GB_test_transaction(father);
    if (father->is_entry()) {
        GB_export_errorf("GB_create: father (%s) is not of GB_DB type (%i) (creating '%s')",
                         GB_read_key_pntr(father), father->type(), key);
        return NULL;
    }

    if (type == GB_POINTER) {
        if (!GB_in_temporary_branch(father)) {
            GB_export_error("GB_create: pointers only allowed in temporary branches");
            return NULL;
        }
    }

    return gb_create(father->expect_container(), key, type);
}

GBDATA *GB_create_container(GBDATA *father, const char *key) {
    /*! Create a new DB container
     *
     * @param father parent container
     * @param key name of created container
     *
     * @return
     * - created container
     * - NULL on failure (error is exported then)
     *
     * @see GB_create()
     */

    if (GB_check_key(key)) {
        GB_print_error();
        return NULL;
    }

    if ((*key == '\0')) {
        GB_export_error("GB_create error: empty key");
        return NULL;
    }
    if (!father) {
        GB_internal_errorf("GB_create error in GB_create:\nno father (key = '%s')", key);
        return NULL;
    }

    GB_test_transaction(father);
    return gb_create_container(father->expect_container(), key);
}

// ----------------------
//      recompression

#if defined(WARN_TODO)
#warning rename gb_set_compression into gb_recompress (misleading name)
#endif

static GB_ERROR gb_set_compression(GBDATA *source) {
    GB_ERROR error = 0;
    GB_test_transaction(source);

    switch (source->type()) {
        case GB_STRING: {
            char *str = GB_read_string(source);
            GB_write_string(source, "");
            GB_write_string(source, str);
            free(str);
            break;
        }
        case GB_BITS:
        case GB_BYTES:
        case GB_INTS:
        case GB_FLOATS:
            break;
        case GB_DB:
            for (GBDATA *gb_p = GB_child(source); gb_p && !error; gb_p = GB_nextChild(gb_p)) {
                error = gb_set_compression(gb_p);
            }
            break;
        default:
            break;
    }
    return error;
}

bool GB_allow_compression(GBDATA *gb_main, bool allow_compression) {
    GB_MAIN_TYPE *Main      = GB_MAIN(gb_main);
    int           prev_mask = Main->compression_mask;
    Main->compression_mask  = allow_compression ? -1 : 0;

    return prev_mask == 0 ? false : true;
}


GB_ERROR GB_delete(GBDATA*& source) {
    GBDATA *gb_main;

    GB_test_transaction(source);
    if (GB_GET_SECURITY_DELETE(source)>GB_MAIN(source)->security_level) {
        return GBS_global_string("Security error: deleting entry '%s' not permitted", GB_read_key_pntr(source));
    }

    gb_main = GB_get_root(source);

    if (source->flags.compressed_data) {
        bool was_allowed = GB_allow_compression(gb_main, false);
        gb_set_compression(source); // write data w/o compression (otherwise GB_read_old_value... won't work)
        GB_allow_compression(gb_main, was_allowed);
    }

    {
        GB_MAIN_TYPE *Main = GB_MAIN(source);
        if (Main->get_transaction_level() < 0) { // no transaction mode
            gb_delete_entry(source);
            Main->call_pending_callbacks();
        }
        else {
            gb_touch_entry(source, GB_DELETED);
        }
    }
    return 0;
}

GB_ERROR gb_delete_force(GBDATA *source)    // delete always
{
    gb_touch_entry(source, GB_DELETED);
    return 0;
}


// ------------------
//      Copy data

#if defined(WARN_TODO)
#warning replace GB_copy with GB_copy_with_protection after release
#endif

GB_ERROR GB_copy(GBDATA *dest, GBDATA *source) {
    return GB_copy_with_protection(dest, source, false);
}

GB_ERROR GB_copy_with_protection(GBDATA *dest, GBDATA *source, bool copy_all_protections) {
    GB_ERROR error = 0;
    GB_test_transaction(source);

    GB_TYPES type = source->type();
    if (dest->type() != type) {
        return GB_export_errorf("incompatible types in GB_copy (source %s:%u != %s:%u",
                                GB_read_key_pntr(source), type, GB_read_key_pntr(dest), dest->type());
    }

    switch (type) {
        case GB_INT:
            error = GB_write_int(dest, GB_read_int(source));
            break;
        case GB_FLOAT:
            error = GB_write_float(dest, GB_read_float(source));
            break;
        case GB_BYTE:
            error = GB_write_byte(dest, GB_read_byte(source));
            break;
        case GB_STRING:     // No local compression
            error = GB_write_string(dest, GB_read_char_pntr(source));
            break;
        case GB_LINK:       // No local compression
            error = GB_write_link(dest, GB_read_link_pntr(source));
            break;
        case GB_BITS:       // only local compressions for the following types
        case GB_BYTES:
        case GB_INTS:
        case GB_FLOATS: {
            GBENTRY *source_entry = source->as_entry();
            GBENTRY *dest_entry   = dest->as_entry();

            gb_save_extern_data_in_ts(dest_entry);
            dest_entry->insert_data(source_entry->data(), source_entry->size(), source_entry->memsize());

            dest->flags.compressed_data = source->flags.compressed_data;
            break;
        }
        case GB_DB: {
            if (!dest->is_container()) {
                GB_ERROR err = GB_export_errorf("GB_COPY Type conflict %s:%i != %s:%i",
                                                GB_read_key_pntr(dest), dest->type(), GB_read_key_pntr(source), GB_DB);
                GB_internal_error(err);
                return err;
            }

            GBCONTAINER *destc   = dest->as_container();
            GBCONTAINER *sourcec = source->as_container();

            if (sourcec->flags2.folded_container) gb_unfold(sourcec, -1, -1);
            if (destc->flags2.folded_container)   gb_unfold(destc, 0, -1);

            for (GBDATA *gb_p = GB_child(sourcec); gb_p; gb_p = GB_nextChild(gb_p)) {
                const char *key = GB_read_key_pntr(gb_p);
                GBDATA     *gb_d;

                if (gb_p->is_container()) {
                    gb_d = GB_create_container(destc, key);
                    gb_create_header_array(gb_d->as_container(), gb_p->as_container()->d.size);
                }
                else {
                    gb_d = GB_create(destc, key, gb_p->type());
                }

                if (!gb_d) error = GB_await_error();
                else error       = GB_copy_with_protection(gb_d, gb_p, copy_all_protections);

                if (error) break;
            }

            destc->flags3 = sourcec->flags3;
            break;
        }
        default:
            error = GB_export_error("GB_copy-error: unhandled type");
    }
    if (error) return error;

    gb_touch_entry(dest, GB_NORMAL_CHANGE);

    dest->flags.security_read = source->flags.security_read;
    if (copy_all_protections) {
        dest->flags.security_write  = source->flags.security_write;
        dest->flags.security_delete = source->flags.security_delete;
    }

    return 0;
}


static char *gb_stpcpy(char *dest, const char *source)
{
    while ((*dest++=*source++)) ;
    return dest-1; // return pointer to last copied character (which is \0)
}

char* GB_get_subfields(GBDATA *gbd) {
    /*! Get all subfield names
     *
     * @return all subfields of 'gbd' as ';'-separated heap-copy
     * (first and last char of result is a ';')
     */
    GB_test_transaction(gbd);

    char *result = 0;
    if (gbd->is_container()) {
        GBCONTAINER *gbc           = gbd->as_container();
        int          result_length = 0;

        if (gbc->flags2.folded_container) {
            gb_unfold(gbc, -1, -1);
        }

        for (GBDATA *gbp = GB_child(gbd); gbp; gbp = GB_nextChild(gbp)) {
            const char *key = GB_read_key_pntr(gbp);
            int keylen = strlen(key);

            if (result) {
                char *neu_result = (char*)malloc(result_length+keylen+1+1);

                if (neu_result) {
                    char *p = gb_stpcpy(neu_result, result);
                    p = gb_stpcpy(p, key);
                    *p++ = ';';
                    p[0] = 0;

                    freeset(result, neu_result);
                    result_length += keylen+1;
                }
                else {
                    gb_assert(0);
                }
            }
            else {
                result = (char*)malloc(1+keylen+1+1);
                result[0] = ';';
                strcpy(result+1, key);
                result[keylen+1] = ';';
                result[keylen+2] = 0;
                result_length = keylen+2;
            }
        }
    }
    else {
        result = strdup(";");
    }

    return result;
}

// --------------------------
//      temporary entries

GB_ERROR GB_set_temporary(GBDATA *gbd) { // goes to header: __ATTR__USERESULT
    /*! if the temporary flag is set, then that entry (including all subentries) will not be saved
     * @see GB_clear_temporary() and GB_is_temporary()
     */

    GB_ERROR error = NULL;
    GB_test_transaction(gbd);

    if (GB_GET_SECURITY_DELETE(gbd)>GB_MAIN(gbd)->security_level) {
        error = GBS_global_string("Security error in GB_set_temporary: %s", GB_read_key_pntr(gbd));
    }
    else {
        gbd->flags.temporary = 1;
        gb_touch_entry(gbd, GB_NORMAL_CHANGE);
    }
    RETURN_ERROR(error);
}

GB_ERROR GB_clear_temporary(GBDATA *gbd) { // @@@ used in ptpan branch - do not remove
    //! undo effect of GB_set_temporary()

    GB_test_transaction(gbd);
    gbd->flags.temporary = 0;
    gb_touch_entry(gbd, GB_NORMAL_CHANGE);
    return 0;
}

bool GB_is_temporary(GBDATA *gbd) {
    //! @see GB_set_temporary() and GB_in_temporary_branch()
    GB_test_transaction(gbd);
    return (long)gbd->flags.temporary;
}

bool GB_in_temporary_branch(GBDATA *gbd) {
    /*! @return true, if 'gbd' is member of a temporary subtree,
     * i.e. if GB_is_temporary(itself or any parent)
     */

    if (GB_is_temporary(gbd)) return true;

    GBDATA *gb_parent = GB_get_father(gbd);
    if (!gb_parent) return false;

    return GB_in_temporary_branch(gb_parent);
}

// ---------------------
//      transactions

GB_ERROR GB_MAIN_TYPE::initial_client_transaction() {
    // the first client transaction ever
    transaction_level = 1;
    GB_ERROR error    = gbcmc_init_transaction(root_container);
    if (!error) ++clock;
    return error;
}

inline GB_ERROR GB_MAIN_TYPE::start_transaction() {
    gb_assert(transaction_level == 0);

    transaction_level   = 1;
    aborted_transaction = 0;

    GB_ERROR error = NULL;
    if (is_client()) {
        error = gbcmc_begin_transaction(gb_main());
        if (!error) {
            error = gb_commit_transaction_local_rek(gb_main_ref(), 0, 0); // init structures
            gb_untouch_children_and_me(root_container);
        }
    }

    if (!error) {
        /* do all callbacks
         * cb that change the db are no problem, because it's the beginning of a ta
         */
        call_pending_callbacks();
        ++clock;
    }
    return error;
}

inline GB_ERROR GB_MAIN_TYPE::begin_transaction() {
    if (transaction_level>0) return GBS_global_string("attempt to start a NEW transaction (at transaction level %i)", transaction_level);
    if (transaction_level == 0) return start_transaction();
    return NULL; // NO_TRANSACTION_MODE
}

inline GB_ERROR GB_MAIN_TYPE::abort_transaction() {
    if (transaction_level<=0) {
        if (transaction_level<0) return "GB_abort_transaction: Attempt to abort transaction in no-transaction-mode";
        return "GB_abort_transaction: No transaction running";
    }
    if (transaction_level>1) {
        aborted_transaction = 1;
        return pop_transaction();
    }

    gb_abort_transaction_local_rek(gb_main_ref());
    if (is_client()) {
        GB_ERROR error = gbcmc_abort_transaction(gb_main());
        if (error) return error;
    }
    clock--;
    call_pending_callbacks();
    transaction_level = 0;
    gb_untouch_children_and_me(root_container);
    return 0;
}

inline GB_ERROR GB_MAIN_TYPE::commit_transaction() {
    GB_ERROR      error = 0;
    GB_CHANGE     flag;

    if (!transaction_level) {
        return "commit_transaction: No transaction running";
    }
    if (transaction_level>1) {
        return GBS_global_string("attempt to commit at transaction level %i", transaction_level);
    }
    if (aborted_transaction) {
        aborted_transaction = 0;
        return abort_transaction();
    }
    if (is_server()) {
        char *error1 = gb_set_undo_sync(gb_main());
        while (1) {
            flag = (GB_CHANGE)GB_ARRAY_FLAGS(gb_main()).changed;
            if (!flag) break;           // nothing to do
            error = gb_commit_transaction_local_rek(gb_main_ref(), 0, 0);
            gb_untouch_children_and_me(root_container);
            if (error) break;
            call_pending_callbacks();
        }
        gb_disable_undo(gb_main());
        if (error1) {
            transaction_level = 0;
            gb_assert(error); // maybe return error1?
            return error; // @@@ huh? why not return error1
        }
    }
    else {
        gb_disable_undo(gb_main());
        while (1) {
            flag = (GB_CHANGE)GB_ARRAY_FLAGS(gb_main()).changed;
            if (!flag) break;           // nothing to do

            error = gbcmc_begin_sendupdate(gb_main());                    if (error) break;
            error = gb_commit_transaction_local_rek(gb_main_ref(), 1, 0); if (error) break;
            error = gbcmc_end_sendupdate(gb_main());                      if (error) break;

            gb_untouch_children_and_me(root_container);
            call_pending_callbacks();
        }
        if (!error) error = gbcmc_commit_transaction(gb_main());

    }
    transaction_level = 0;
    return error;
}

inline GB_ERROR GB_MAIN_TYPE::push_transaction() {
    if (transaction_level == 0) return start_transaction();
    if (transaction_level>0) ++transaction_level;
    // transaction<0 is NO_TRANSACTION_MODE
    return NULL;
}

inline GB_ERROR GB_MAIN_TYPE::pop_transaction() {
    if (transaction_level==0) return "attempt to pop nested transaction while none running";
    if (transaction_level<0)  return 0;  // NO_TRANSACTION_MODE
    if (transaction_level==1) return commit_transaction();
    transaction_level--;
    return NULL;
}

inline GB_ERROR GB_MAIN_TYPE::no_transaction() {
    if (is_client()) return "Tried to disable transactions in a client";
    transaction_level = -1;
    return NULL;
}

GB_ERROR GB_MAIN_TYPE::send_update_to_server(GBDATA *gbd) {
    GB_ERROR error = NULL;

    if (!transaction_level) error = "send_update_to_server: no transaction running";
    else if (is_server()) error   = "send_update_to_server: only possible from clients (not from server itself)";
    else {
        const gb_triggered_callback *chg_cbl_old = changeCBs.pending.get_tail();
        const gb_triggered_callback *del_cbl_old = deleteCBs.pending.get_tail();

        error             = gbcmc_begin_sendupdate(gb_main());
        if (!error) error = gb_commit_transaction_local_rek(gbd, 2, 0);
        if (!error) error = gbcmc_end_sendupdate(gb_main());

        if (!error &&
            (chg_cbl_old != changeCBs.pending.get_tail() ||
             del_cbl_old != deleteCBs.pending.get_tail()))
        {
            error = "send_update_to_server triggered a callback (this is not allowed)";
        }
    }
    return error;
}

// --------------------------------------
//      client transaction interface

GB_ERROR GB_push_transaction(GBDATA *gbd) {
    /*! start a transaction if no transaction is running.
     * (otherwise only trace nested transactions)
     *
     * recommended transaction usage:
     *
     * \code
     * GB_ERROR myFunc() {
     *     GB_ERROR error = GB_push_transaction(gbd);
     *     if (!error) {
     *         error = ...;
     *     }
     *     return GB_end_transaction(gbd, error);
     * }
     *
     * void myFunc() {
     *     GB_ERROR error = GB_push_transaction(gbd);
     *     if (!error) {
     *         error = ...;
     *     }
     *     GB_end_transaction_show_error(gbd, error, aw_message);
     * }
     * \endcode
     *
     * @see GB_pop_transaction(), GB_end_transaction(), GB_begin_transaction()
     */

    return GB_MAIN(gbd)->push_transaction();
}

GB_ERROR GB_pop_transaction(GBDATA *gbd) {
    //! commit a transaction started with GB_push_transaction()
    return GB_MAIN(gbd)->pop_transaction();
}
GB_ERROR GB_begin_transaction(GBDATA *gbd) {
    /*! like GB_push_transaction(),
     * but fails if there is already an transaction running.
     * @see GB_commit_transaction() and GB_abort_transaction()
     */
    return GB_MAIN(gbd)->begin_transaction();
}
GB_ERROR GB_no_transaction(GBDATA *gbd) { // goes to header: __ATTR__USERESULT
    return GB_MAIN(gbd)->no_transaction();
}

GB_ERROR GB_abort_transaction(GBDATA *gbd) {
    /*! abort a running transaction,
     * i.e. forget all changes made to DB inside the current transaction.
     *
     * May be called instead of GB_pop_transaction() or GB_commit_transaction()
     *
     * If a nested transactions got aborted,
     * committing a surrounding transaction will silently abort it as well.
     */
    return GB_MAIN(gbd)->abort_transaction();
}

GB_ERROR GB_commit_transaction(GBDATA *gbd) {
    /*! commit a transaction started with GB_begin_transaction()
     *
     * commit changes made to DB.
     *
     * in case of nested transactions, this is equal to GB_pop_transaction()
     */
    return GB_MAIN(gbd)->commit_transaction();
}

GB_ERROR GB_end_transaction(GBDATA *gbd, GB_ERROR error) {
    /*! abort or commit transaction
     *
     * @ param error
     * - if NULL commit transaction
     * - else abort transaction
     *
     * always commits in no-transaction-mode
     *
     * @return error or transaction error
     * @see GB_push_transaction() for example
     */

    if (GB_get_transaction_level(gbd)<0) {
        ASSERT_RESULT(GB_ERROR, NULL, GB_pop_transaction(gbd));
    }
    else {
        if (error) GB_abort_transaction(gbd);
        else error = GB_pop_transaction(gbd);
    }
    return error;
}

void GB_end_transaction_show_error(GBDATA *gbd, GB_ERROR error, void (*error_handler)(GB_ERROR)) {
    //! like GB_end_transaction(), but show error using 'error_handler'
    error = GB_end_transaction(gbd, error);
    if (error) error_handler(error);
}

int GB_get_transaction_level(GBDATA *gbd) {
    /*! @return transaction level
     * <0 -> in no-transaction-mode (abort is impossible)
     *  0 -> not in transaction
     *  1 -> one single transaction
     *  2, ... -> nested transactions
     */
    return GB_MAIN(gbd)->get_transaction_level();
}

// ------------------
//      callbacks

static void dummy_db_cb(GBDATA*, GB_CB_TYPE) { gb_assert(0); } // used as marker for deleted callbacks
DatabaseCallback TypedDatabaseCallback::MARKED_DELETED = makeDatabaseCallback(dummy_db_cb);

GB_MAIN_TYPE *gb_get_main_during_cb() {
    /* if inside a callback, return the DB root of the DB element, the callback was called for.
     * if not inside a callback, return NULL.
     */
    return inside_callback_main;
}

NOT4PERL bool GB_inside_callback(GBDATA *of_gbd, GB_CB_TYPE cbtype) {
    GB_MAIN_TYPE *Main   = gb_get_main_during_cb();
    bool          inside = false;

    if (Main) {                 // inside a callback
        gb_assert(currently_called_back);
        if (currently_called_back->gbd == of_gbd) {
            GB_CB_TYPE curr_cbtype;
            if (Main->has_pending_delete_callback()) { // delete callbacks were not all performed yet
                                                       // => current callback is a delete callback
                curr_cbtype = GB_CB_TYPE(currently_called_back->spec.get_type() & GB_CB_DELETE);
            }
            else {
                gb_assert(Main->has_pending_change_callback());
                curr_cbtype = GB_CB_TYPE(currently_called_back->spec.get_type() & (GB_CB_ALL-GB_CB_DELETE));
            }
            gb_assert(curr_cbtype != GB_CB_NONE); // wtf!? are we inside callback or not?

            if ((cbtype&curr_cbtype) != GB_CB_NONE) {
                inside = true;
            }
        }
    }

    return inside;
}

GBDATA *GB_get_gb_main_during_cb() {
    GBDATA       *gb_main = NULL;
    GB_MAIN_TYPE *Main    = gb_get_main_during_cb();

    if (Main) {                 // inside callback
        if (!GB_inside_callback(Main->gb_main(), GB_CB_DELETE)) { // main is not deleted
            gb_main = Main->gb_main();
        }
    }
    return gb_main;
}



static GB_CSTR gb_read_pntr_ts(GBDATA *gbd, gb_transaction_save *ts) {
    int         type = GB_TYPE_TS(ts);
    const char *data = GB_GETDATA_TS(ts);
    if (data) {
        if (ts->flags.compressed_data) {    // uncompressed data return pntr to database entry
            long size = GB_GETSIZE_TS(ts) * gb_convert_type_2_sizeof[type] + gb_convert_type_2_appendix_size[type];
            data = gb_uncompress_data(gbd, data, size);
        }
    }
    return data;
}

// get last array value in callbacks
NOT4PERL const void *GB_read_old_value() {
    char *data;

    if (!currently_called_back) {
        GB_export_error("You cannot call GB_read_old_value outside a ARBDB callback");
        return NULL;
    }
    if (!currently_called_back->old) {
        GB_export_error("No old value available in GB_read_old_value");
        return NULL;
    }
    data = GB_GETDATA_TS(currently_called_back->old);
    if (!data) return NULL;

    return gb_read_pntr_ts(currently_called_back->gbd, currently_called_back->old);
}
// same for size
long GB_read_old_size() {
    if (!currently_called_back) {
        GB_export_error("You cannot call GB_read_old_size outside a ARBDB callback");
        return -1;
    }
    if (!currently_called_back->old) {
        GB_export_error("No old value available in GB_read_old_size");
        return -1;
    }
    return GB_GETSIZE_TS(currently_called_back->old);
}

inline char *cbtype2readable(GB_CB_TYPE type) {
    ConstStrArray septype;

#define appendcbtype(cbt) do {                  \
        if (type&cbt) {                         \
            type = GB_CB_TYPE(type-cbt);        \
            septype.put(#cbt);                  \
        }                                       \
    } while(0)

    appendcbtype(GB_CB_DELETE);
    appendcbtype(GB_CB_CHANGED);
    appendcbtype(GB_CB_SON_CREATED);

    gb_assert(type == GB_CB_NONE);

    return GBT_join_names(septype, '|');
}

char *TypedDatabaseCallback::get_info() const {
    const char *readable_fun    = GBS_funptr2readable((void*)dbcb.callee(), true);
    char       *readable_cbtype = cbtype2readable((GB_CB_TYPE)dbcb.inspect_CD2());
    char       *result          = GBS_global_string_copy("func='%s' type=%s clientdata=%p",
                                                         readable_fun, readable_cbtype, (void*)dbcb.inspect_CD1());

    free(readable_cbtype);

    return result;
}

char *GB_get_callback_info(GBDATA *gbd) {
    // returns human-readable information about callbacks of 'gbd' or 0
    char *result = 0;
    if (gbd->ext) {
        gb_callback_list *cbl = gbd->get_callbacks();
        if (cbl) {
            for (gb_callback_list::itertype cb = cbl->callbacks.begin(); cb != cbl->callbacks.end(); ++cb) {
                char *cb_info = cb->spec.get_info();
                if (result) {
                    char *new_result = GBS_global_string_copy("%s\n%s", result, cb_info);
                    free(result);
                    free(cb_info);
                    result = new_result;
                }
                else {
                    result = cb_info;
                }
            }
        }
    }

    return result;
}

#if defined(ASSERTION_USED)
template<typename CB>
bool CallbackList<CB>::contains_unremoved_callback(const CB& like) const {
    for (const_itertype cb = callbacks.begin(); cb != callbacks.end(); ++cb) {
        if (cb->spec.is_equal_to(like.spec) && !cb->spec.is_marked_for_removal()) {
            return true;
        }
    }
    return false;
}
#endif

inline void add_to_callback_chain(gb_callback_list*& head, const TypedDatabaseCallback& cbs) {
    if (!head) head = new gb_callback_list;
    head->add(gb_callback(cbs));
}
inline void add_to_callback_chain(gb_hierarchy_callback_list*& head, const TypedDatabaseCallback& cbs, GBDATA *gb_representative) {
    if (!head) head = new gb_hierarchy_callback_list;
    head->add(gb_hierarchy_callback(cbs, gb_representative));
}

inline GB_ERROR gb_add_callback(GBDATA *gbd, const TypedDatabaseCallback& cbs) {
    /* Adds a callback to a DB entry.
     *
     * Be careful when writing GB_CB_DELETE callbacks, there is a severe restriction:
     *
     * - the DB element may already be freed. The pointer is still pointing to the original
     *   location, so you can use it to identify the DB element, but you cannot dereference
     *   it under all circumstances.
     *
     * ARBDB internal delete-callbacks may use gb_get_main_during_cb() to access the DB root.
     * See also: GB_get_gb_main_during_cb()
     */

#if defined(DEBUG)
    if (GB_inside_callback(gbd, GB_CB_DELETE)) {
        printf("Warning: add_priority_callback called inside delete-callback of gbd (gbd may already be freed)\n");
#if defined(DEVEL_RALF)
        gb_assert(0); // fix callback-handling (never modify callbacks from inside delete callbacks)
#endif // DEVEL_RALF
    }
#endif // DEBUG

    GB_test_transaction(gbd); // may return error
    gbd->create_extended();
    add_to_callback_chain(gbd->ext->callback, cbs);
    return 0;
}

GB_ERROR GB_add_callback(GBDATA *gbd, GB_CB_TYPE type, const DatabaseCallback& dbcb) {
    return gb_add_callback(gbd, TypedDatabaseCallback(dbcb, type));
}

inline void GB_MAIN_TYPE::callback_group::add_hcb(GBDATA *gb_representative, const TypedDatabaseCallback& dbcb) {
    add_to_callback_chain(hierarchy_cbs, dbcb, gb_representative);
}

GB_ERROR GB_MAIN_TYPE::add_hierarchy_cb(GBDATA *gb_representative, const TypedDatabaseCallback& dbcb) {
    GB_CB_TYPE type = dbcb.get_type();
    if (type & GB_CB_DELETE) {
        deleteCBs.add_hcb(gb_representative, dbcb.with_type_changed_to(GB_CB_DELETE));
    }
    if (type & GB_CB_ALL_BUT_DELETE) {
        changeCBs.add_hcb(gb_representative, dbcb.with_type_changed_to(GB_CB_TYPE(type&GB_CB_ALL_BUT_DELETE)));
    }
    return NULL;
}

GB_ERROR GB_add_hierarchy_callback(GBDATA *gbd, GB_CB_TYPE type, const DatabaseCallback& dbcb) {
    /*! bind callback to ALL entries which are at the same DB-hierarchy as 'gbd'.
     *
     * Hierarchy callbacks are triggered before normal callbacks (added by GB_add_callback or GB_ensure_callback).
     * Nevertheless delete callbacks take precedence over change callbacks
     * (i.e. a normal delete callback is triggered before a hierarchical change callback).
     *
     * Hierarchy callbacks are cannot be installed and will NOT be triggered in NO_TRANSACTION_MODE
     * (i.e. it will not work in ARBs property DBs)
     */
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    gb_assert(Main->get_transaction_level()>=0); // hierarchical callbacks are not supported in NO_TRANSACTION_MODE
    return Main->add_hierarchy_cb(gbd, TypedDatabaseCallback(dbcb, type));
}

template <typename PRED>
inline void gb_remove_callbacks_that(GBDATA *gbd, PRED shallRemove) {
#if defined(ASSERTION_USED)
    if (GB_inside_callback(gbd, GB_CB_DELETE)) {
        printf("Warning: gb_remove_callback called inside delete-callback of gbd (gbd may already be freed)\n");
        gb_assert(0); // fix callback-handling (never modify callbacks from inside delete callbacks)
        return;
    }
#endif // DEBUG

    if (gbd->ext) {
        gb_callback_list *cbl = gbd->get_callbacks();
        if (cbl) {
            bool prev_running = false;

            for (gb_callback_list::itertype cb = cbl->callbacks.begin(); cb != cbl->callbacks.end(); ) {
                bool this_running = cb->running;

                if (shallRemove(*cb)) {
                    if (prev_running || this_running) {
                        cb->spec.mark_for_removal();
                        ++cb;
                    }
                    else {
                        cb = cbl->callbacks.erase(cb);
                    }
                }
                else {
                    ++cb;
                }
                prev_running = this_running;
            }
        }
    }
}

struct ShallBeDeleted {
    bool operator()(const gb_callback& cb) const { return cb.spec.is_marked_for_removal(); }
};
static void gb_remove_callbacks_marked_for_deletion(GBDATA *gbd) {
    gb_remove_callbacks_that(gbd, ShallBeDeleted());
}

struct IsCallback : private TypedDatabaseCallback {
    IsCallback(GB_CB func_, GB_CB_TYPE type_) : TypedDatabaseCallback(makeDatabaseCallback((GB_CB)func_, (int*)NULL), type_) {}
    bool operator()(const gb_callback& cb) const { return sig_is_equal_to(cb.spec); }
};
struct IsSpecificCallback : private TypedDatabaseCallback {
    IsSpecificCallback(const TypedDatabaseCallback& cb) : TypedDatabaseCallback(cb) {}
    bool operator()(const gb_callback& cb) const { return is_equal_to(cb.spec); }
};

void GB_remove_callback(GBDATA *gbd, GB_CB_TYPE type, const DatabaseCallback& dbcb) {
    // remove specific callback; 'type' and 'dbcb' have to match
    gb_remove_callbacks_that(gbd, IsSpecificCallback(TypedDatabaseCallback(dbcb, type)));
}
void GB_remove_all_callbacks_to(GBDATA *gbd, GB_CB_TYPE type, GB_CB func) {
    // removes all callbacks 'func' bound to 'gbd' with 'type'
    gb_remove_callbacks_that(gbd, IsCallback(func, type));
}

GB_ERROR GB_ensure_callback(GBDATA *gbd, GB_CB_TYPE type, const DatabaseCallback& dbcb) {
    TypedDatabaseCallback newcb(dbcb, type);
    gb_callback_list *cbl = gbd->get_callbacks();
    if (cbl) {
        for (gb_callback_list::itertype cb = cbl->callbacks.begin(); cb != cbl->callbacks.end(); ++cb) {
            if (cb->spec.is_equal_to(newcb) && !cb->spec.is_marked_for_removal()) {
                return NULL; // already in cb list
            }
        }
    }
    return gb_add_callback(gbd, newcb);
}

int GB_nsons(GBDATA *gbd) {
    /*! return number of child entries
     *
     * @@@ does this work in clients ?
     */

    return gbd->is_container()
        ? gbd->as_container()->d.size
        : 0;
}

void GB_disable_quicksave(GBDATA *gbd, const char *reason) {
    /*! Disable quicksaving database
     * @param gbd any DB node
     * @param reason why quicksaving is not allowed
     */
    freedup(GB_MAIN(gbd)->qs.quick_save_disabled, reason);
}

GB_ERROR GB_resort_data_base(GBDATA *gb_main, GBDATA **new_order_list, long listsize) {
    {
        long client_count = GB_read_clients(gb_main);
        if (client_count<0)
            return "Sorry: this program is not the arbdb server, you cannot resort your data";

        if (client_count>0)
            return GBS_global_string("There are %li clients (editors, tree programs) connected to this server.\n"
                                     "You need to these close clients before you can run this operation.",
                                     client_count);
    }

    if (listsize <= 0) return 0;

    GBCONTAINER *father = GB_FATHER(new_order_list[0]);
    GB_disable_quicksave(gb_main, "some entries in the database got a new order");

    gb_header_list *hl = GB_DATA_LIST_HEADER(father->d);
    for (long new_index = 0; new_index< listsize; new_index++) {
        long old_index = new_order_list[new_index]->index;

        if (old_index < new_index) {
            GB_warningf("Warning at resort database: entry exists twice: %li and %li",
                        old_index, new_index);
        }
        else {
            GBDATA *ogb = GB_HEADER_LIST_GBD(hl[old_index]);
            GBDATA *ngb = GB_HEADER_LIST_GBD(hl[new_index]);

            gb_header_list h = hl[new_index];
            hl[new_index] = hl[old_index];
            hl[old_index] = h;              // Warning: Relative Pointers are incorrect !!!

            SET_GB_HEADER_LIST_GBD(hl[old_index], ngb);
            SET_GB_HEADER_LIST_GBD(hl[new_index], ogb);

            if (ngb) ngb->index = old_index;
            if (ogb) ogb->index = new_index;
        }
    }

    gb_touch_entry(father, GB_NORMAL_CHANGE);
    return 0;
}

GB_ERROR gb_resort_system_folder_to_top(GBCONTAINER *gb_main) {
    GBDATA *gb_system = GB_entry(gb_main, GB_SYSTEM_FOLDER);
    GBDATA *gb_first = GB_child(gb_main);
    GBDATA **new_order_list;
    GB_ERROR error = 0;
    int i, len;
    if (GB_read_clients(gb_main)<0) return 0; // we are not server
    if (!gb_system) {
        return GB_export_error("System databaseentry does not exist");
    }
    if (gb_first == gb_system) return 0;
    len = GB_number_of_subentries(gb_main);
    new_order_list = (GBDATA **)GB_calloc(sizeof(GBDATA *), len);
    new_order_list[0] = gb_system;
    for (i=1; i<len; i++) {
        new_order_list[i] = gb_first;
        do {
            gb_first = GB_nextChild(gb_first);
        } while (gb_first == gb_system);
    }
    error = GB_resort_data_base(gb_main, new_order_list, len);
    free(new_order_list);
    return error;
}

// ------------------------------
//      private(?) user flags

STATIC_ASSERT_ANNOTATED(((GB_USERFLAG_ANY+1)&GB_USERFLAG_ANY) == 0, "not all bits set in GB_USERFLAG_ANY");

#if defined(ASSERTION_USED)
inline bool legal_user_bitmask(unsigned char bitmask) {
    return bitmask>0 && bitmask<=GB_USERFLAG_ANY;
}
#endif

inline gb_flag_types2& get_user_flags(GBDATA *gbd) {
    return gbd->expect_container()->flags2;
}

bool GB_user_flag(GBDATA *gbd, unsigned char user_bit) {
    gb_assert(legal_user_bitmask(user_bit));
    return get_user_flags(gbd).user_bits & user_bit;
}
void GB_set_user_flag(GBDATA *gbd, unsigned char user_bit) {
    gb_assert(legal_user_bitmask(user_bit));
    gb_flag_types2& flags  = get_user_flags(gbd);
    flags.user_bits       |= user_bit;
}

void GB_clear_user_flag(GBDATA *gbd, unsigned char user_bit) {
    gb_assert(legal_user_bitmask(user_bit));
    gb_flag_types2& flags  = get_user_flags(gbd);
    flags.user_bits       &= (user_bit^GB_USERFLAG_ANY);
}

// ------------------------
//      mark DB entries

void GB_write_flag(GBDATA *gbd, long flag) {
    GBCONTAINER  *gbc  = gbd->expect_container();
    GB_MAIN_TYPE *Main = GB_MAIN(gbc);

    GB_test_transaction(Main);

    int ubit = Main->users[0]->userbit;
    int prev = GB_ARRAY_FLAGS(gbc).flags;
    gbc->flags.saved_flags = prev;

    if (flag) {
        GB_ARRAY_FLAGS(gbc).flags |= ubit;
    }
    else {
        GB_ARRAY_FLAGS(gbc).flags &= ~ubit;
    }
    if (prev != (int)GB_ARRAY_FLAGS(gbc).flags) {
        gb_touch_entry(gbc, GB_NORMAL_CHANGE);
        gb_touch_header(GB_FATHER(gbc));
        GB_DO_CALLBACKS(gbc);
    }
}

int GB_read_flag(GBDATA *gbd) {
    GB_test_transaction(gbd);
    if (GB_ARRAY_FLAGS(gbd).flags & GB_MAIN(gbd)->users[0]->userbit) return 1;
    else return 0;
}

void GB_touch(GBDATA *gbd) {
    GB_test_transaction(gbd);
    gb_touch_entry(gbd, GB_NORMAL_CHANGE);
    GB_DO_CALLBACKS(gbd);
}


char GB_type_2_char(GB_TYPES type) {
    const char *type2char = "-bcif-B-CIFlSS-%";
    return type2char[type];
}

GB_ERROR GB_print_debug_information(void */*dummy_AW_root*/, GBDATA *gb_main) {
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    GB_push_transaction(gb_main);
    for (int i=0; i<Main->keycnt; i++) {
        if (Main->keys[i].key) {
            printf("%3i %20s    nref %i\n", i, Main->keys[i].key, (int)Main->keys[i].nref);
        }
        else {
            printf("    %3i unused key, next free key = %li\n", i, Main->keys[i].next_free_key);
        }
    }
    gbm_debug_mem();
    GB_pop_transaction(gb_main);
    return 0;
}

static int GB_info_deep = 15;


static int gb_info(GBDATA *gbd, int deep) {
    if (gbd==NULL) { printf("NULL\n"); return -1; }
    GB_push_transaction(gbd);

    GB_TYPES type = gbd->type();

    if (deep) {
        printf("    ");
    }

    printf("(GBDATA*)0x%lx (GBCONTAINER*)0x%lx ", (long)gbd, (long)gbd);

    if (gbd->rel_father==0) { printf("father=NULL\n"); return -1; }

    GBCONTAINER  *gbc;
    GB_MAIN_TYPE *Main;
    if (type==GB_DB) { gbc = gbd->as_container(); Main = GBCONTAINER_MAIN(gbc); }
    else             { gbc = NULL;                Main = GB_MAIN(gbd); }

    if (!Main) { printf("Oops - I have no main entry!!!\n"); return -1; }
    if (gbd==Main->dummy_father) { printf("dummy_father!\n"); return -1; }

    printf("%10s Type '%c'  ", GB_read_key_pntr(gbd), GB_type_2_char(type));

    switch (type) {
        case GB_DB: {
            int size = gbc->d.size;
            printf("Size %i nheader %i hmemsize %i", gbc->d.size, gbc->d.nheader, gbc->d.headermemsize);
            printf(" father=(GBDATA*)0x%lx\n", (long)GB_FATHER(gbd));
            if (size < GB_info_deep) {
                int             index;
                gb_header_list *header;

                header = GB_DATA_LIST_HEADER(gbc->d);
                for (index = 0; index < gbc->d.nheader; index++) {
                    GBDATA *gb_sub = GB_HEADER_LIST_GBD(header[index]);
                    printf("\t\t%10s (GBDATA*)0x%lx (GBCONTAINER*)0x%lx\n", Main->keys[header[index].flags.key_quark].key, (long)gb_sub, (long)gb_sub);
                }
            }
            break;
        }
        default: {
            char *data = GB_read_as_string(gbd);
            if (data) { printf("%s", data); free(data); }
            printf(" father=(GBDATA*)0x%lx\n", (long)GB_FATHER(gbd));
        }
    }


    GB_pop_transaction(gbd);

    return 0;
}

int GB_info(GBDATA *gbd) { // unused - intended to be used in debugger
    return gb_info(gbd, 0);
}

long GB_number_of_subentries(GBDATA *gbd) {
    GBCONTAINER    *gbc        = gbd->expect_container();
    gb_header_list *header     = GB_DATA_LIST_HEADER(gbc->d);

    long subentries = 0;
    int  end        = gbc->d.nheader;

    for (int index = 0; index<end; index++) {
        if (header[index].flags.changed < GB_DELETED) subentries++;
    }
    return subentries;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS

#include <test_unit.h>
#include <locale.h>

void TEST_GB_atof() {
    // startup of ARB (gtk_only@11651) is failing on ubuntu 13.10 (in GBT_read_tree)
    // (failed with "Error: 'GB_safe_atof("0.0810811", ..) returns error: cannot convert '0.0810811' to double'")
    // Reason: LANG[UAGE] or LC_NUMERIC set to "de_DE..."
    //
    // Notes:
    // * gtk apparently calls 'setlocale(LC_ALL, "");', motif doesnt

    TEST_EXPECT_SIMILAR(GB_atof("0.031"), 0.031, 0.0001); // @@@ make this fail, then fix it
}

void TEST_999_strtod_replacement() {
    // caution: if it fails -> locale is not reset (therefore call with low priority 999)
    const char *old = setlocale(LC_NUMERIC, "de_DE.UTF-8");
    {
        // TEST_EXPECT_SIMILAR__BROKEN(strtod("0.031", NULL), 0.031, 0.0001);
        TEST_EXPECT_SIMILAR(g_ascii_strtod("0.031", NULL), 0.031, 0.0001);
    }
    setlocale(LC_NUMERIC, old);
}

static void test_another_shell() { delete new GB_shell; }
static void test_opendb() { GB_close(GB_open("no.arb", "c")); }

void TEST_GB_shell() {
    {
        GB_shell *shell = new GB_shell;
        TEST_EXPECT_SEGFAULT(test_another_shell);
        test_opendb(); // no SEGV here
        delete shell;
    }

    TEST_EXPECT_SEGFAULT(test_opendb); // should be impossible to open db w/o shell
}

void TEST_GB_number_of_subentries() {
    GB_shell  shell;
    GBDATA   *gb_main = GB_open("no.arb", "c");

    {
        GB_transaction ta(gb_main);

        GBDATA   *gb_cont = GB_create_container(gb_main, "container");
        TEST_EXPECT_EQUAL(GB_number_of_subentries(gb_cont), 0);

        TEST_EXPECT_RESULT__NOERROREXPORTED(GB_create(gb_cont, "entry", GB_STRING));
        TEST_EXPECT_EQUAL(GB_number_of_subentries(gb_cont), 1);

        {
            GBDATA *gb_entry;
            TEST_EXPECT_RESULT__NOERROREXPORTED(gb_entry = GB_create(gb_cont, "entry", GB_STRING));
            TEST_EXPECT_EQUAL(GB_number_of_subentries(gb_cont), 2);

            TEST_EXPECT_NO_ERROR(GB_delete(gb_entry));
            TEST_EXPECT_EQUAL(GB_number_of_subentries(gb_cont), 1);
        }

        TEST_EXPECT_RESULT__NOERROREXPORTED(GB_create(gb_cont, "entry", GB_STRING));
        TEST_EXPECT_EQUAL(GB_number_of_subentries(gb_cont), 2);
    }

    // TEST_REJECT(true); // @@@ fail while GB_shell open

    GB_close(gb_main);
}

static void test_count_cb(GBDATA *, int *counter) {
    fprintf(stderr, "test_count_cb: var.add=%p old.val=%i ", counter, *counter);
    (*counter)++;
    fprintf(stderr, "new.val=%i\n", *counter);
    fflush(stderr);
}

static void remove_self_cb(GBDATA *gbe, GB_CB_TYPE cbtype) {
    GB_remove_callback(gbe, cbtype, makeDatabaseCallback(remove_self_cb));
}

static void re_add_self_cb(GBDATA *gbe, int *calledCounter, GB_CB_TYPE cbtype) {
    ++(*calledCounter);

    DatabaseCallback dbcb = makeDatabaseCallback(re_add_self_cb, calledCounter);
    GB_remove_callback(gbe, cbtype, dbcb);

    GB_ERROR error = GB_add_callback(gbe, cbtype, dbcb);
    gb_assert(!error);
}

void TEST_db_callbacks_ta_nota() {
    GB_shell shell;

    enum TAmode {
        NO_TA   = 1, // no transaction mode
        WITH_TA = 2, // transaction mode

        BOTH_TA_MODES = (NO_TA|WITH_TA)
    };

    for (TAmode ta_mode = NO_TA; ta_mode <= WITH_TA; ta_mode = TAmode(ta_mode+1)) {
        GBDATA   *gb_main = GB_open("no.arb", "c");
        GB_ERROR  error;

        TEST_ANNOTATE(ta_mode == NO_TA ? "NO_TA" : "WITH_TA");
        if (ta_mode == NO_TA) {
            error = GB_no_transaction(gb_main); TEST_EXPECT_NO_ERROR(error);
        }

        // create some DB entries
        GBDATA *gbc;
        GBDATA *gbe1;
        GBDATA *gbe2;
        GBDATA *gbe3;
        {
            GB_transaction ta(gb_main);

            gbc  = GB_create_container(gb_main, "cont");
            gbe1 = GB_create(gbc, "entry", GB_STRING);
            gbe2 = GB_create(gb_main, "entry", GB_INT);
        }

        // counters to detect called callbacks
        int e1_changed    = 0;
        int e2_changed    = 0;
        int c_changed     = 0;
        int c_son_created = 0;

        int e1_deleted = 0;
        int e2_deleted = 0;
        int e3_deleted = 0;
        int c_deleted  = 0;

#define CHCB_COUNTERS_EXPECTATION(e1c,e2c,cc,csc)       \
        that(e1_changed).is_equal_to(e1c),              \
            that(e2_changed).is_equal_to(e2c),          \
            that(c_changed).is_equal_to(cc),            \
            that(c_son_created).is_equal_to(csc)

#define DLCB_COUNTERS_EXPECTATION(e1d,e2d,e3d,cd)       \
        that(e1_deleted).is_equal_to(e1d),              \
            that(e2_deleted).is_equal_to(e2d),          \
            that(e3_deleted).is_equal_to(e3d),          \
            that(c_deleted).is_equal_to(cd)

#define TEST_EXPECT_CHCB_COUNTERS(e1c,e2c,cc,csc,tam) do{ if (ta_mode & (tam)) TEST_EXPECTATION(all().of(CHCB_COUNTERS_EXPECTATION(e1c,e2c,cc,csc))); }while(0)
#define TEST_EXPECT_CHCB___WANTED(e1c,e2c,cc,csc,tam) do{ if (ta_mode & (tam)) TEST_EXPECTATION__WANTED(all().of(CHCB_COUNTERS_EXPECTATION(e1c,e2c,cc,csc))); }while(0)

#define TEST_EXPECT_DLCB_COUNTERS(e1d,e2d,e3d,cd,tam) do{ if (ta_mode & (tam)) TEST_EXPECTATION(all().of(DLCB_COUNTERS_EXPECTATION(e1d,e2d,e3d,cd))); }while(0)
#define TEST_EXPECT_DLCB___WANTED(e1d,e2d,e3d,cd,tam) do{ if (ta_mode & (tam)) TEST_EXPECTATION__WANTED(all().of(DLCB_COUNTERS_EXPECTATION(e1d,e2d,e3d,cd))); }while(0)

#define TEST_EXPECT_COUNTER(tam,cnt,expected)             do{ if (ta_mode & (tam)) TEST_EXPECT_EQUAL(cnt, expected); }while(0)
#define TEST_EXPECT_COUNTER__BROKEN(tam,cnt,expected,got) do{ if (ta_mode & (tam)) TEST_EXPECT_EQUAL__BROKEN(cnt, expected, got); }while(0)

#define RESET_CHCB_COUNTERS()   do{ e1_changed = e2_changed = c_changed = c_son_created = 0; }while(0)
#define RESET_DLCB_COUNTERS()   do{ e1_deleted = e2_deleted = e3_deleted = c_deleted = 0; }while(0)
#define RESET_ALL_CB_COUNTERS() do{ RESET_CHCB_COUNTERS(); RESET_DLCB_COUNTERS(); }while(0)

        // install some DB callbacks
        {
            GB_transaction ta(gb_main);
            GB_add_callback(gbe1, GB_CB_CHANGED,     makeDatabaseCallback(test_count_cb, &e1_changed));
            GB_add_callback(gbe2, GB_CB_CHANGED,     makeDatabaseCallback(test_count_cb, &e2_changed));
            GB_add_callback(gbc,  GB_CB_CHANGED,     makeDatabaseCallback(test_count_cb, &c_changed));
            GB_add_callback(gbc,  GB_CB_SON_CREATED, makeDatabaseCallback(test_count_cb, &c_son_created));
        }

        // check callbacks were not called yet
        TEST_EXPECT_CHCB_COUNTERS(0, 0, 0, 0, BOTH_TA_MODES);

        // trigger callbacks
        {
            GB_transaction ta(gb_main);

            error = GB_write_string(gbe1, "hi"); TEST_EXPECT_NO_ERROR(error);
            error = GB_write_int(gbe2, 666);     TEST_EXPECT_NO_ERROR(error);

            TEST_EXPECT_CHCB_COUNTERS(1, 1, 1, 0, NO_TA);   // callbacks triggered instantly in NO_TA mode
            TEST_EXPECT_CHCB_COUNTERS(0, 0, 0, 0, WITH_TA); // callbacks delayed until transaction is committed

        } // [Note: callbacks happen here in ta_mode]

        // test that GB_CB_SON_CREATED is not triggered here:
        TEST_EXPECT_CHCB_COUNTERS(1, 1, 1, 0, NO_TA);
        TEST_EXPECT_CHCB_COUNTERS(1, 1, 1, 0, WITH_TA);

        // really create a son
        RESET_CHCB_COUNTERS();
        {
            GB_transaction ta(gb_main);
            gbe3 = GB_create(gbc, "e3", GB_STRING);
        }
        TEST_EXPECT_CHCB_COUNTERS(0, 0, 0, 0, NO_TA); // broken
        TEST_EXPECT_CHCB___WANTED(0, 0, 1, 1, NO_TA);
        TEST_EXPECT_CHCB_COUNTERS(0, 0, 1, 1, WITH_TA);

        // change that son
        RESET_CHCB_COUNTERS();
        {
            GB_transaction ta(gb_main);
            error = GB_write_string(gbe3, "bla"); TEST_EXPECT_NO_ERROR(error);
        }
        TEST_EXPECT_CHCB_COUNTERS(0, 0, 1, 0, BOTH_TA_MODES);


        // test delete callbacks
        RESET_CHCB_COUNTERS();
        {
            GB_transaction ta(gb_main);

            GB_add_callback(gbe1, GB_CB_DELETE, makeDatabaseCallback(test_count_cb, &e1_deleted));
            GB_add_callback(gbe2, GB_CB_DELETE, makeDatabaseCallback(test_count_cb, &e2_deleted));
            GB_add_callback(gbe3, GB_CB_DELETE, makeDatabaseCallback(test_count_cb, &e3_deleted));
            GB_add_callback(gbc,  GB_CB_DELETE, makeDatabaseCallback(test_count_cb, &c_deleted));
        }
        TEST_EXPECT_CHCB_COUNTERS(0, 0, 0, 0, BOTH_TA_MODES); // adding callbacks does not trigger existing change-callbacks
        {
            GB_transaction ta(gb_main);

            error = GB_delete(gbe3); TEST_EXPECT_NO_ERROR(error);
            error = GB_delete(gbe2); TEST_EXPECT_NO_ERROR(error);

            TEST_EXPECT_DLCB_COUNTERS(0, 1, 1, 0, NO_TA);
            TEST_EXPECT_DLCB_COUNTERS(0, 0, 0, 0, WITH_TA);
        }

        TEST_EXPECT_CHCB_COUNTERS(0, 0, 1, 0, WITH_TA); // container changed by deleting a son (gbe3); no longer triggers via GB_SON_CHANGED
        TEST_EXPECT_CHCB_COUNTERS(0, 0, 0, 0, NO_TA);   // change is not triggered in NO_TA mode (error?)
        TEST_EXPECT_CHCB___WANTED(0, 0, 1, 1, NO_TA);

        TEST_EXPECT_DLCB_COUNTERS(0, 1, 1, 0, BOTH_TA_MODES);

        RESET_ALL_CB_COUNTERS();
        {
            GB_transaction ta(gb_main);
            error = GB_delete(gbc);  TEST_EXPECT_NO_ERROR(error); // delete the container containing gbe1 and gbe3 (gbe3 alreay deleted)
        }
        TEST_EXPECT_CHCB_COUNTERS(0, 0, 0, 0, BOTH_TA_MODES); // deleting the container does not trigger any change callbacks
        TEST_EXPECT_DLCB_COUNTERS(1, 0, 0, 1, BOTH_TA_MODES); // deleting the container does also trigger the delete callback for gbe1

        // --------------------------------------------------------------------------------
        // document that a callback now can be removed while it is running
        // (in NO_TA mode; always worked in WITH_TA mode)
        {
            GBDATA *gbe;
            {
                GB_transaction ta(gb_main);
                gbe = GB_create(gb_main, "new_e1", GB_INT); // recreate
                GB_add_callback(gbe, GB_CB_CHANGED, makeDatabaseCallback(remove_self_cb));
            }
            { GB_transaction ta(gb_main); GB_touch(gbe); }
        }

        // test that a callback may remove and re-add itself
        {
            GBDATA *gbn1;
            GBDATA *gbn2;

            int counter1 = 0;
            int counter2 = 0;

            {
                GB_transaction ta(gb_main);
                gbn1 = GB_create(gb_main, "new_e1", GB_INT);
                gbn2 = GB_create(gb_main, "new_e2", GB_INT);
                GB_add_callback(gbn1, GB_CB_CHANGED, makeDatabaseCallback(re_add_self_cb, &counter1));
            }

            TEST_EXPECT_COUNTER(NO_TA,         counter1, 0); // no callback triggered (trigger happens BEFORE call to GB_add_callback in NO_TA mode!)
            TEST_EXPECT_COUNTER(WITH_TA,       counter1, 1); // callback gets triggered by GB_create
            TEST_EXPECT_COUNTER(BOTH_TA_MODES, counter2, 0);

            counter1 = 0; counter2 = 0;

            // test no callback is triggered by just adding a callback
            {
                GB_transaction ta(gb_main);
                GB_add_callback(gbn2, GB_CB_CHANGED, makeDatabaseCallback(re_add_self_cb, &counter2));
            }
            TEST_EXPECT_COUNTER(BOTH_TA_MODES, counter1, 0);
            TEST_EXPECT_COUNTER(BOTH_TA_MODES, counter2, 0);

            { GB_transaction ta(gb_main); GB_touch(gbn1); }
            TEST_EXPECT_COUNTER(BOTH_TA_MODES, counter1, 1);
            TEST_EXPECT_COUNTER(BOTH_TA_MODES, counter2, 0);

            { GB_transaction ta(gb_main); GB_touch(gbn2); }
            TEST_EXPECT_COUNTER(BOTH_TA_MODES, counter1, 1);
            TEST_EXPECT_COUNTER(BOTH_TA_MODES, counter2, 1);

            { GB_transaction ta(gb_main);  }
            TEST_EXPECT_COUNTER(BOTH_TA_MODES, counter1, 1);
            TEST_EXPECT_COUNTER(BOTH_TA_MODES, counter2, 1);
        }

        GB_close(gb_main);
    }
}

// -----------------------
//      test callbacks

struct calledWith {
    GBDATA     *gbd;
    GB_CB_TYPE  type;
    int         time_called;

    static int timer;

    calledWith(GBDATA *gbd_, GB_CB_TYPE type_) : gbd(gbd_), type(type_), time_called(++timer) {}
    calledWith(const calledWith& other) : gbd(other.gbd), type(other.type), time_called(other.time_called) {}
    DECLARE_ASSIGNMENT_OPERATOR(calledWith);
};

int calledWith::timer = 0;

class callback_trace {
    typedef std::list<calledWith> calledList;
    typedef calledList::iterator  calledIter;

    calledList called;

    calledIter find(GBDATA *gbd) {
        calledIter c = called.begin();
        while (c != called.end()) {
            if (c->gbd == gbd) break;
            ++c;
        }
        return c;
    }
    calledIter find(GB_CB_TYPE exp_type) {
        calledIter c = called.begin();
        while (c != called.end()) {
            if (c->type&exp_type) break;
            ++c;
        }
        return c;
    }
    calledIter find(GBDATA *gbd, GB_CB_TYPE exp_type) {
        calledIter c = called.begin();
        while (c != called.end()) {
            if (c->gbd == gbd && (c->type&exp_type)) break;
            ++c;
        }
        return c;
    }

    bool removed(calledIter c) {
        if (c == called.end()) return false;
        called.erase(c);
        return true;
    }

public:
    callback_trace() { called.clear(); }

    void set_called_by(GBDATA *gbd, GB_CB_TYPE type) { called.push_back(calledWith(gbd, type)); }

    bool was_called_by(GBDATA *gbd) { return removed(find(gbd)); }
    bool was_called_by(GB_CB_TYPE exp_type) { return removed(find(exp_type)); }
    bool was_called_by(GBDATA *gbd, GB_CB_TYPE exp_type) { return removed(find(gbd, exp_type)); }

    int call_time(GBDATA *gbd, GB_CB_TYPE exp_type) {
        calledIter found = find(gbd, exp_type);
        if (found == called.end()) return -1;

        int t = found->time_called;
        removed(found);
        return t;
    }

    bool was_not_called() const { return called.empty(); }
    bool was_called() const { return !was_not_called(); }
};

static void some_cb(GBDATA *gbd, callback_trace *trace, GB_CB_TYPE cbtype) {
    trace->set_called_by(gbd, cbtype);
}

#define TRACESTRUCT(ELEM,FLAVOR)           trace_##ELEM##_##FLAVOR
#define HIERARCHY_TRACESTRUCT(ELEM,FLAVOR) traceHier_##ELEM##_##FLAVOR

#define ADD_CHANGED_CALLBACK(elem) TEST_EXPECT_NO_ERROR(GB_add_callback(elem, GB_CB_CHANGED,     makeDatabaseCallback(some_cb, &TRACESTRUCT(elem,changed))))
#define ADD_DELETED_CALLBACK(elem) TEST_EXPECT_NO_ERROR(GB_add_callback(elem, GB_CB_DELETE,      makeDatabaseCallback(some_cb, &TRACESTRUCT(elem,deleted))))
#define ADD_NWCHILD_CALLBACK(elem) TEST_EXPECT_NO_ERROR(GB_add_callback(elem, GB_CB_SON_CREATED, makeDatabaseCallback(some_cb, &TRACESTRUCT(elem,newchild))))

#define ADD_CHANGED_HIERARCHY_CALLBACK(elem) TEST_EXPECT_NO_ERROR(GB_add_hierarchy_callback(elem, GB_CB_CHANGED,     makeDatabaseCallback(some_cb, &HIERARCHY_TRACESTRUCT(elem,changed))))
#define ADD_DELETED_HIERARCHY_CALLBACK(elem) TEST_EXPECT_NO_ERROR(GB_add_hierarchy_callback(elem, GB_CB_DELETE,      makeDatabaseCallback(some_cb, &HIERARCHY_TRACESTRUCT(elem,deleted))))
#define ADD_NWCHILD_HIERARCHY_CALLBACK(elem) TEST_EXPECT_NO_ERROR(GB_add_hierarchy_callback(elem, GB_CB_SON_CREATED, makeDatabaseCallback(some_cb, &HIERARCHY_TRACESTRUCT(elem,newchild))))

#define ENSURE_CHANGED_CALLBACK(elem) TEST_EXPECT_NO_ERROR(GB_ensure_callback(elem, GB_CB_CHANGED,     makeDatabaseCallback(some_cb, &TRACESTRUCT(elem,changed))))
#define ENSURE_DELETED_CALLBACK(elem) TEST_EXPECT_NO_ERROR(GB_ensure_callback(elem, GB_CB_DELETE,      makeDatabaseCallback(some_cb, &TRACESTRUCT(elem,deleted))))
#define ENSURE_NWCHILD_CALLBACK(elem) TEST_EXPECT_NO_ERROR(GB_ensure_callback(elem, GB_CB_SON_CREATED, makeDatabaseCallback(some_cb, &TRACESTRUCT(elem,newchild))))

#define REMOVE_CHANGED_CALLBACK(elem) GB_remove_callback(elem, GB_CB_CHANGED,     makeDatabaseCallback(some_cb, &TRACESTRUCT(elem,changed)))
#define REMOVE_DELETED_CALLBACK(elem) GB_remove_callback(elem, GB_CB_DELETE,      makeDatabaseCallback(some_cb, &TRACESTRUCT(elem,deleted)))
#define REMOVE_NWCHILD_CALLBACK(elem) GB_remove_callback(elem, GB_CB_SON_CREATED, makeDatabaseCallback(some_cb, &TRACESTRUCT(elem,newchild)))

#define INIT_CHANGED_CALLBACK(elem) callback_trace TRACESTRUCT(elem,changed);  ADD_CHANGED_CALLBACK(elem)
#define INIT_DELETED_CALLBACK(elem) callback_trace TRACESTRUCT(elem,deleted);  ADD_DELETED_CALLBACK(elem)
#define INIT_NWCHILD_CALLBACK(elem) callback_trace TRACESTRUCT(elem,newchild); ADD_NWCHILD_CALLBACK(elem)

#define INIT_CHANGED_HIERARCHY_CALLBACK(elem) callback_trace HIERARCHY_TRACESTRUCT(elem,changed);  ADD_CHANGED_HIERARCHY_CALLBACK(elem)
#define INIT_DELETED_HIERARCHY_CALLBACK(elem) callback_trace HIERARCHY_TRACESTRUCT(elem,deleted);  ADD_DELETED_HIERARCHY_CALLBACK(elem)
#define INIT_NWCHILD_HIERARCHY_CALLBACK(elem) callback_trace HIERARCHY_TRACESTRUCT(elem,newchild); ADD_NWCHILD_HIERARCHY_CALLBACK(elem)

#define ENSURE_ENTRY_CALLBACKS(entry)    ENSURE_CHANGED_CALLBACK(entry); ENSURE_DELETED_CALLBACK(entry)
#define ENSURE_CONTAINER_CALLBACKS(cont) ENSURE_CHANGED_CALLBACK(cont);  ENSURE_NWCHILD_CALLBACK(cont); ENSURE_DELETED_CALLBACK(cont)

#define REMOVE_ENTRY_CALLBACKS(entry)    REMOVE_CHANGED_CALLBACK(entry); REMOVE_DELETED_CALLBACK(entry)
#define REMOVE_CONTAINER_CALLBACKS(cont) REMOVE_CHANGED_CALLBACK(cont);  REMOVE_NWCHILD_CALLBACK(cont); REMOVE_DELETED_CALLBACK(cont)

#define INIT_ENTRY_CALLBACKS(entry)    INIT_CHANGED_CALLBACK(entry); INIT_DELETED_CALLBACK(entry)
#define INIT_CONTAINER_CALLBACKS(cont) INIT_CHANGED_CALLBACK(cont);  INIT_NWCHILD_CALLBACK(cont); INIT_DELETED_CALLBACK(cont)

#define TRIGGER_CHANGE(gbd) do {                \
        GB_initial_transaction ta(gb_main);     \
        if (ta.ok()) GB_touch(gbd);             \
        TEST_EXPECT_NO_ERROR(ta.close(NULL));   \
    } while(0)

#define TRIGGER_2_CHANGES(gbd1, gbd2) do {      \
        GB_initial_transaction ta(gb_main);     \
        if (ta.ok()) {                          \
            GB_touch(gbd1);                     \
            GB_touch(gbd2);                     \
        }                                       \
        TEST_EXPECT_NO_ERROR(ta.close(NULL));   \
    } while(0)

#define TRIGGER_DELETE(gbd) do {                \
        GB_initial_transaction ta(gb_main);     \
        GB_ERROR error = NULL;                  \
        if (ta.ok()) error = GB_delete(gbd);    \
        TEST_EXPECT_NO_ERROR(ta.close(error));  \
    } while(0)

#define TEST_EXPECT_NO_CALLBACK_TRIGGERED()                     \
    TEST_EXPECT(trace_top_deleted.was_not_called());            \
    TEST_EXPECT(trace_son_deleted.was_not_called());            \
    TEST_EXPECT(trace_grandson_deleted.was_not_called());       \
    TEST_EXPECT(trace_ograndson_deleted.was_not_called());      \
    TEST_EXPECT(trace_cont_top_deleted.was_not_called());       \
    TEST_EXPECT(trace_cont_son_deleted.was_not_called());       \
    TEST_EXPECT(trace_top_changed.was_not_called());            \
    TEST_EXPECT(trace_son_changed.was_not_called());            \
    TEST_EXPECT(trace_grandson_changed.was_not_called());       \
    TEST_EXPECT(trace_ograndson_changed.was_not_called());      \
    TEST_EXPECT(trace_cont_top_changed.was_not_called());       \
    TEST_EXPECT(trace_cont_son_changed.was_not_called());       \
    TEST_EXPECT(trace_cont_top_newchild.was_not_called());      \
    TEST_EXPECT(trace_cont_son_newchild.was_not_called());      \


#define TEST_EXPECT_CB_TRIGGERED(TRACE,GBD,TYPE)         TEST_EXPECT(TRACE.was_called_by(GBD, TYPE))
#define TEST_EXPECT_CB_TRIGGERED_AT(TRACE,GBD,TYPE,TIME) TEST_EXPECT_EQUAL(TRACE.call_time(GBD, TYPE), TIME)

#define TEST_EXPECT_CHANGE_TRIGGERED(TRACE,GBD) TEST_EXPECT_CB_TRIGGERED(TRACE, GBD, GB_CB_CHANGED)
#define TEST_EXPECT_DELETE_TRIGGERED(TRACE,GBD) TEST_EXPECT_CB_TRIGGERED(TRACE, GBD, GB_CB_DELETE)
#define TEST_EXPECT_NCHILD_TRIGGERED(TRACE,GBD) TEST_EXPECT_CB_TRIGGERED(TRACE, GBD, GB_CB_SON_CREATED)

#define TEST_EXPECT_CHANGE_TRIGGERED_AT(TRACE,GBD,TIME) TEST_EXPECT_CB_TRIGGERED_AT(TRACE, GBD, GB_CB_CHANGED, TIME)
#define TEST_EXPECT_DELETE_TRIGGERED_AT(TRACE,GBD,TIME) TEST_EXPECT_CB_TRIGGERED_AT(TRACE, GBD, GB_CB_DELETE, TIME)

void TEST_db_callbacks() {
    GB_shell  shell;
    GBDATA   *gb_main = GB_open("new.arb", "c");

    // create some data
    GB_begin_transaction(gb_main);

    GBDATA *cont_top = GB_create_container(gb_main,  "cont_top"); TEST_REJECT_NULL(cont_top);
    GBDATA *cont_son = GB_create_container(cont_top, "cont_son"); TEST_REJECT_NULL(cont_son);

    GBDATA *top       = GB_create(gb_main,  "top",       GB_STRING); TEST_REJECT_NULL(top);
    GBDATA *son       = GB_create(cont_top, "son",       GB_INT);    TEST_REJECT_NULL(son);
    GBDATA *grandson  = GB_create(cont_son, "grandson",  GB_STRING); TEST_REJECT_NULL(grandson);
    GBDATA *ograndson = GB_create(cont_son, "ograndson", GB_STRING); TEST_REJECT_NULL(ograndson);

    GB_commit_transaction(gb_main);

    // install callbacks
    GB_begin_transaction(gb_main);
    INIT_CONTAINER_CALLBACKS(cont_top);
    INIT_CONTAINER_CALLBACKS(cont_son);
    INIT_ENTRY_CALLBACKS(top);
    INIT_ENTRY_CALLBACKS(son);
    INIT_ENTRY_CALLBACKS(grandson);
    INIT_ENTRY_CALLBACKS(ograndson);
    GB_commit_transaction(gb_main);

    TEST_EXPECT_NO_CALLBACK_TRIGGERED();

    // trigger change callbacks via change
    GB_begin_transaction(gb_main);
    GB_write_string(top, "hello world");
    GB_commit_transaction(gb_main);
    TEST_EXPECT_CHANGE_TRIGGERED(trace_top_changed, top);
    TEST_EXPECT_NO_CALLBACK_TRIGGERED();

    GB_begin_transaction(gb_main);
    GB_write_string(top, "hello world"); // no change
    GB_commit_transaction(gb_main);
    TEST_EXPECT_NO_CALLBACK_TRIGGERED();

#if 0
    // code is wrong (cannot set terminal entry to "marked")
    GB_begin_transaction(gb_main);
    GB_write_flag(son, 1);                                  // only change "mark"
    GB_commit_transaction(gb_main);
    TEST_EXPECT_CHANGE_TRIGGERED(trace_son_changed, son);
    TEST_EXPECT_CHANGE_TRIGGERED(trace_cont_top_changed, cont_top);
    TEST_EXPECT_TRIGGER__UNWANTED(trace_cont_top_newchild); // @@@ modifying son should not trigger newchild callback
    TEST_EXPECT_NO_CALLBACK_TRIGGERED();
#else
    // @@@ add test code similar to wrong section above
#endif

    GB_begin_transaction(gb_main);
    GB_touch(grandson);
    GB_commit_transaction(gb_main);
    TEST_EXPECT_CHANGE_TRIGGERED(trace_grandson_changed, grandson);
    TEST_EXPECT_CHANGE_TRIGGERED(trace_cont_son_changed, cont_son);
    TEST_EXPECT_CHANGE_TRIGGERED(trace_cont_top_changed, cont_top);
    TEST_EXPECT_NO_CALLBACK_TRIGGERED();

    // trigger change- and soncreate-callbacks via create

    GB_begin_transaction(gb_main);
    GBDATA *son2 = GB_create(cont_top, "son2", GB_INT); TEST_REJECT_NULL(son2);
    GB_commit_transaction(gb_main);
    TEST_EXPECT_CHANGE_TRIGGERED(trace_cont_top_changed, cont_top);
    TEST_EXPECT_NCHILD_TRIGGERED(trace_cont_top_newchild, cont_top);
    TEST_EXPECT_NO_CALLBACK_TRIGGERED();

    GB_begin_transaction(gb_main);
    GBDATA *grandson2 = GB_create(cont_son, "grandson2", GB_STRING); TEST_REJECT_NULL(grandson2);
    GB_commit_transaction(gb_main);
    TEST_EXPECT_CHANGE_TRIGGERED(trace_cont_son_changed, cont_son);
    TEST_EXPECT_NCHILD_TRIGGERED(trace_cont_son_newchild, cont_son);
    TEST_EXPECT_CHANGE_TRIGGERED(trace_cont_top_changed, cont_top);
    TEST_EXPECT_NO_CALLBACK_TRIGGERED();

    // trigger callbacks via delete

    TRIGGER_DELETE(son2);
    TEST_EXPECT_CHANGE_TRIGGERED(trace_cont_top_changed, cont_top);
    TEST_EXPECT_NO_CALLBACK_TRIGGERED();

    TRIGGER_DELETE(grandson2);
    TEST_EXPECT_CHANGE_TRIGGERED(trace_cont_top_changed, cont_top);
    TEST_EXPECT_CHANGE_TRIGGERED(trace_cont_son_changed, cont_son);
    TEST_EXPECT_NO_CALLBACK_TRIGGERED();

    TEST_EXPECT_NO_ERROR(GB_request_undo_type(gb_main, GB_UNDO_UNDO));

    TRIGGER_DELETE(top);
    TEST_EXPECT_DELETE_TRIGGERED(trace_top_deleted, top);
    TEST_EXPECT_NO_CALLBACK_TRIGGERED();

    TRIGGER_DELETE(grandson);
    TEST_EXPECT_DELETE_TRIGGERED(trace_grandson_deleted, grandson);
    TEST_EXPECT_CHANGE_TRIGGERED(trace_cont_top_changed, cont_top);
    TEST_EXPECT_CHANGE_TRIGGERED(trace_cont_son_changed, cont_son);
    TEST_EXPECT_NO_CALLBACK_TRIGGERED();

    TRIGGER_DELETE(cont_son);
    TEST_EXPECT_DELETE_TRIGGERED(trace_ograndson_deleted, ograndson);
    TEST_EXPECT_DELETE_TRIGGERED(trace_cont_son_deleted, cont_son);
    TEST_EXPECT_CHANGE_TRIGGERED(trace_cont_top_changed, cont_top);
    TEST_EXPECT_NO_CALLBACK_TRIGGERED();

    // trigger callbacks by undoing last 3 delete transactions

    TEST_EXPECT_NO_ERROR(GB_undo(gb_main, GB_UNDO_UNDO)); // undo delete of cont_son
    TEST_EXPECT_CHANGE_TRIGGERED(trace_cont_top_changed, cont_top);
    TEST_EXPECT_NCHILD_TRIGGERED(trace_cont_top_newchild, cont_top);
    TEST_EXPECT_NO_CALLBACK_TRIGGERED();

    TEST_EXPECT_NO_ERROR(GB_undo(gb_main, GB_UNDO_UNDO)); // undo delete of grandson
    // cont_son callbacks are not triggered (they are not restored by undo)
    TEST_EXPECT_CHANGE_TRIGGERED(trace_cont_top_changed, cont_top);
    TEST_EXPECT_NO_CALLBACK_TRIGGERED();

    TEST_EXPECT_NO_ERROR(GB_undo(gb_main, GB_UNDO_UNDO)); // undo delete of top
    TEST_EXPECT_NO_CALLBACK_TRIGGERED();

    // reinstall callbacks that were removed by deletes

    GB_begin_transaction(gb_main);
    ENSURE_CONTAINER_CALLBACKS(cont_top);
    ENSURE_CONTAINER_CALLBACKS(cont_son);
    ENSURE_ENTRY_CALLBACKS(top);
    ENSURE_ENTRY_CALLBACKS(son);
    ENSURE_ENTRY_CALLBACKS(grandson);
    GB_commit_transaction(gb_main);
    TEST_EXPECT_NO_CALLBACK_TRIGGERED();

    // trigger callbacks which will be removed

    TRIGGER_CHANGE(son);
    TEST_EXPECT_CHANGE_TRIGGERED(trace_son_changed, son);
    TEST_EXPECT_CHANGE_TRIGGERED(trace_cont_top_changed, cont_top);
    TEST_EXPECT_NO_CALLBACK_TRIGGERED();

    GB_begin_transaction(gb_main);
    GBDATA *son3 = GB_create(cont_top, "son3", GB_INT); TEST_REJECT_NULL(son3);
    GB_commit_transaction(gb_main);
    TEST_EXPECT_CHANGE_TRIGGERED(trace_cont_top_changed, cont_top);
    TEST_EXPECT_NCHILD_TRIGGERED(trace_cont_top_newchild, cont_top);
    TEST_EXPECT_NO_CALLBACK_TRIGGERED();

    // test remove callback

    GB_begin_transaction(gb_main);
    REMOVE_ENTRY_CALLBACKS(son);
    REMOVE_CONTAINER_CALLBACKS(cont_top);
    GB_commit_transaction(gb_main);
    TEST_EXPECT_NO_CALLBACK_TRIGGERED();

    // "trigger" removed callbacks

    TRIGGER_CHANGE(son);
    TEST_EXPECT_NO_CALLBACK_TRIGGERED();

    GB_begin_transaction(gb_main);
    GBDATA *son4 = GB_create(cont_top, "son4", GB_INT); TEST_REJECT_NULL(son4);
    GB_commit_transaction(gb_main);
    TEST_EXPECT_NO_CALLBACK_TRIGGERED();

    GB_close(gb_main);
}

void TEST_POSTCOND_arbdb() {
    GB_ERROR error = NULL;
    if (GB_have_error()) {
        error = GB_await_error(); // clears the error (to make further tests succeed)
    }

    bool unclosed_GB_shell = closed_open_shell_for_unit_tests();

    TEST_REJECT(error);             // your test finished with an exported error
    TEST_REJECT(unclosed_GB_shell); // your test finished w/o destroying GB_shell
}

#define TEST_EXPECT_NO_HIERARCHY_CALLBACK_TRIGGERED()                   \
    TEST_EXPECT(traceHier_anyGrandson_changed.was_not_called());        \
    TEST_EXPECT(traceHier_anyGrandson_deleted.was_not_called());        \
    TEST_EXPECT(traceHier_anySonContainer_newchild.was_not_called())

#undef TEST_EXPECT_NO_CALLBACK_TRIGGERED

#define TEST_EXPECT_NO_CALLBACK_TRIGGERED()                             \
    TEST_EXPECT(trace_anotherGrandson_changed.was_not_called());        \
    TEST_EXPECT(trace_elimGrandson2_deleted.was_not_called())

void TEST_hierarchy_callbacks() {
    GB_shell  shell;
    GBDATA   *gb_main = GB_open("new.arb", "c");

    // create some data
    GB_begin_transaction(gb_main);

    GBDATA *cont_top1 = GB_create_container(gb_main, "cont_top"); TEST_REJECT_NULL(cont_top1);
    GBDATA *cont_top2 = GB_create_container(gb_main, "cont_top"); TEST_REJECT_NULL(cont_top2);

    GBDATA *cont_son11 = GB_create_container(cont_top1, "cont_son"); TEST_REJECT_NULL(cont_son11);
    GBDATA *cont_son21 = GB_create_container(cont_top2, "cont_son"); TEST_REJECT_NULL(cont_son21);
    GBDATA *cont_son22 = GB_create_container(cont_top2, "cont_son"); TEST_REJECT_NULL(cont_son22);

    GBDATA *top1 = GB_create(gb_main, "top", GB_STRING); TEST_REJECT_NULL(top1);
    GBDATA *top2 = GB_create(gb_main, "top", GB_STRING); TEST_REJECT_NULL(top2);

    GBDATA *son11 = GB_create(cont_top1, "son", GB_INT); TEST_REJECT_NULL(son11);
    GBDATA *son12 = GB_create(cont_top1, "son", GB_INT); TEST_REJECT_NULL(son12);
    GBDATA *son21 = GB_create(cont_top2, "son", GB_INT); TEST_REJECT_NULL(son21);

    GBDATA *grandson111 = GB_create(cont_son11, "grandson", GB_STRING); TEST_REJECT_NULL(grandson111);
    GBDATA *grandson112 = GB_create(cont_son11, "grandson", GB_STRING); TEST_REJECT_NULL(grandson112);
    GBDATA *grandson211 = GB_create(cont_son21, "grandson", GB_STRING); TEST_REJECT_NULL(grandson211);
    GBDATA *grandson221 = GB_create(cont_son22, "grandson", GB_STRING); TEST_REJECT_NULL(grandson221);
    GBDATA *grandson222 = GB_create(cont_son22, "grandson", GB_STRING); TEST_REJECT_NULL(grandson222);

    // create some entries at uncommon locations (compared to entries created above)
    GBDATA *ctop_top = GB_create          (cont_top2, "top", GB_STRING);      TEST_REJECT_NULL(ctop_top);
    GBDATA *top_son  = GB_create          (gb_main,   "son", GB_INT);         TEST_REJECT_NULL(top_son);
    GBDATA *cson     = GB_create_container(gb_main,   "cont_son");            TEST_REJECT_NULL(cson);
    GBDATA *cson_gs  = GB_create          (cson,      "grandson", GB_STRING); TEST_REJECT_NULL(cson_gs);

    GB_commit_transaction(gb_main);

    // test gb_hierarchy_location
    {
        gb_hierarchy_location loc_top(top1);
        gb_hierarchy_location loc_son(son11);
        gb_hierarchy_location loc_grandson(grandson222);

        TEST_EXPECT(loc_top.matches(top1));
        TEST_EXPECT(loc_top.matches(top2));
        TEST_EXPECT(!loc_top.matches(cont_top1));
        TEST_EXPECT(!loc_top.matches(son12));
        TEST_EXPECT(!loc_top.matches(cont_son22));
        TEST_EXPECT(!loc_top.matches(ctop_top));

        TEST_EXPECT(loc_son.matches(son11));
        TEST_EXPECT(loc_son.matches(son21));
        TEST_EXPECT(!loc_son.matches(top1));
        TEST_EXPECT(!loc_son.matches(grandson111));
        TEST_EXPECT(!loc_son.matches(cont_son22));
        TEST_EXPECT(!loc_son.matches(top_son));

        TEST_EXPECT(loc_grandson.matches(grandson222));
        TEST_EXPECT(loc_grandson.matches(grandson111));
        TEST_EXPECT(!loc_grandson.matches(son11));
        TEST_EXPECT(!loc_grandson.matches(top1));
        TEST_EXPECT(!loc_grandson.matches(cont_son22));
        TEST_EXPECT(!loc_grandson.matches(cson_gs));

        gb_hierarchy_location loc_ctop_top(ctop_top);
        TEST_EXPECT(loc_ctop_top.matches(ctop_top));
        TEST_EXPECT(!loc_ctop_top.matches(top1));

        gb_hierarchy_location loc_top_son(top_son);
        TEST_EXPECT(loc_top_son.matches(top_son));
        TEST_EXPECT(!loc_top_son.matches(son11));

        gb_hierarchy_location loc_gs(cson_gs);
        TEST_EXPECT(loc_gs.matches(cson_gs));
        TEST_EXPECT(!loc_gs.matches(grandson211));

        gb_hierarchy_location loc_root(gb_main);
        TEST_EXPECT(loc_root.matches(gb_main));
        TEST_EXPECT(!loc_root.matches(cont_top1));
        TEST_EXPECT(!loc_root.matches(cont_son11));
        TEST_EXPECT(!loc_root.matches(top1));
        TEST_EXPECT(!loc_root.matches(son11));
        TEST_EXPECT(!loc_root.matches(grandson211));
    }


    // instanciate callback_trace data and install hierarchy callbacks
    GBDATA *anySon = son11;

    GBDATA *anySonContainer     = cont_son11;
    GBDATA *anotherSonContainer = cont_son22;

    GBDATA *anyGrandson     = grandson221;
    GBDATA *anotherGrandson = grandson112;
    GBDATA *elimGrandson    = grandson222;
    GBDATA *elimGrandson2   = grandson111;
    GBDATA *newGrandson     = NULL;

    INIT_CHANGED_HIERARCHY_CALLBACK(anyGrandson);
    INIT_DELETED_HIERARCHY_CALLBACK(anyGrandson);
    INIT_NWCHILD_HIERARCHY_CALLBACK(anySonContainer);

    TEST_EXPECT_NO_HIERARCHY_CALLBACK_TRIGGERED();

    // trigger change-callback using same DB entry
    TRIGGER_CHANGE(anyGrandson);
    TEST_EXPECT_CHANGE_TRIGGERED(traceHier_anyGrandson_changed, anyGrandson);
    TEST_EXPECT_NO_HIERARCHY_CALLBACK_TRIGGERED();

    // trigger change-callback using another DB entry (same hierarchy)
    TRIGGER_CHANGE(anotherGrandson);
    TEST_EXPECT_CHANGE_TRIGGERED(traceHier_anyGrandson_changed, anotherGrandson);
    TEST_EXPECT_NO_HIERARCHY_CALLBACK_TRIGGERED();

    // check nothing is triggered by an element at different hierarchy
    TRIGGER_CHANGE(anySon);
    TEST_EXPECT_NO_HIERARCHY_CALLBACK_TRIGGERED();

    // trigger change-callback using both DB entries (in two TAs)
    TRIGGER_CHANGE(anyGrandson);
    TRIGGER_CHANGE(anotherGrandson);
    TEST_EXPECT_CHANGE_TRIGGERED(traceHier_anyGrandson_changed, anyGrandson);
    TEST_EXPECT_CHANGE_TRIGGERED(traceHier_anyGrandson_changed, anotherGrandson);
    TEST_EXPECT_NO_HIERARCHY_CALLBACK_TRIGGERED();

    // trigger change-callback using both DB entries (in one TA)
    TRIGGER_2_CHANGES(anyGrandson, anotherGrandson);
    TEST_EXPECT_CHANGE_TRIGGERED(traceHier_anyGrandson_changed, anyGrandson);
    TEST_EXPECT_CHANGE_TRIGGERED(traceHier_anyGrandson_changed, anotherGrandson);
    TEST_EXPECT_NO_HIERARCHY_CALLBACK_TRIGGERED();

    // trigger son-created-callback
    {
        GB_initial_transaction ta(gb_main);
        if (ta.ok()) {
            GBDATA *someson = GB_create(anySonContainer, "someson", GB_STRING); TEST_REJECT_NULL(someson);
        }
        TEST_EXPECT_NO_ERROR(ta.close(NULL));
    }
    TEST_EXPECT_NCHILD_TRIGGERED(traceHier_anySonContainer_newchild, anySonContainer);
    TEST_EXPECT_NO_HIERARCHY_CALLBACK_TRIGGERED();

    // trigger 2 son-created-callbacks (for 2 containers) and one change-callback (for a newly created son)
    {
        GB_initial_transaction ta(gb_main);
        if (ta.ok()) {
            newGrandson     = GB_create(anotherSonContainer, "grandson", GB_STRING); TEST_REJECT_NULL(newGrandson);
            GBDATA *someson = GB_create(anySonContainer,     "someson",  GB_STRING); TEST_REJECT_NULL(someson);
        }
        TEST_EXPECT_NO_ERROR(ta.close(NULL));
    }
    TEST_EXPECT_CHANGE_TRIGGERED(traceHier_anyGrandson_changed, newGrandson);
    TEST_EXPECT_NCHILD_TRIGGERED(traceHier_anySonContainer_newchild, anotherSonContainer);
    TEST_EXPECT_NCHILD_TRIGGERED(traceHier_anySonContainer_newchild, anySonContainer);
    TEST_EXPECT_NO_HIERARCHY_CALLBACK_TRIGGERED();

    // trigger delete-callback
    {
        GB_initial_transaction ta(gb_main);
        TEST_EXPECT_NO_ERROR(GB_delete(elimGrandson));
        TEST_EXPECT_NO_ERROR(ta.close(NULL));
    }
    TEST_EXPECT_DELETE_TRIGGERED(traceHier_anyGrandson_deleted, elimGrandson);
    TEST_EXPECT_NO_HIERARCHY_CALLBACK_TRIGGERED();

    // bind normal (non-hierarchical) callbacks to entries which trigger hierarchical callbacks and ..
    calledWith::timer = 0;
    GB_begin_transaction(gb_main);
    INIT_CHANGED_CALLBACK(anotherGrandson);
    INIT_DELETED_CALLBACK(elimGrandson2);
    GB_commit_transaction(gb_main);

    TEST_EXPECT_NO_CALLBACK_TRIGGERED();

    {
        GB_initial_transaction ta(gb_main);
        if (ta.ok()) {
            GB_touch(anotherGrandson);
            GB_touch(elimGrandson2);
            TEST_EXPECT_NO_ERROR(GB_delete(elimGrandson2));
        }
    }

    // .. test call-order (delete before change, hierarchical before normal):
    TEST_EXPECT_DELETE_TRIGGERED_AT(traceHier_anyGrandson_deleted, elimGrandson2,   1);
    TEST_EXPECT_DELETE_TRIGGERED_AT(trace_elimGrandson2_deleted,   elimGrandson2,   2);
    TEST_EXPECT_CHANGE_TRIGGERED_AT(traceHier_anyGrandson_changed, anotherGrandson, 3);
    TEST_EXPECT_CHANGE_TRIGGERED_AT(trace_anotherGrandson_changed, anotherGrandson, 4);

    TEST_EXPECT_NO_HIERARCHY_CALLBACK_TRIGGERED();
    TEST_EXPECT_NO_CALLBACK_TRIGGERED();

    // cleanup
    GB_close(gb_main);
}

#endif


