// =============================================================== //
//                                                                 //
//   File      : adtools.cxx                                       //
//   Purpose   : misc functions                                    //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "gb_data.h"
#include "gb_main.h"

#include <arb_sort.h>
#include <arb_file.h>
#include <arb_sleep.h>
#include <arb_str.h>
#include <arb_strarray.h>
#include <ad_cb.h>

#include <algorithm>
#include "ad_remote.h"

using namespace std;

GBDATA *GBT_create(GBDATA *father, const char *key, long delete_level) {
    GBDATA *gbd = GB_create_container(father, key);
    if (gbd) {
        GB_ERROR error = GB_write_security_delete(gbd, delete_level);
        if (error) {
            GB_export_error(error);
            gbd = NULL; // assuming caller will abort the transaction
        }
    }
    return gbd;
}

GBDATA *GBT_find_or_create(GBDATA *father, const char *key, long delete_level) {
    GBDATA *gbd = GB_entry(father, key);
    if (!gbd) gbd = GBT_create(father, key, delete_level);
    return gbd;
}


/* the following functions were meant to use user defined values.
 *
 * Especially for 'ECOLI' there is already a possibility to
 * specify a different reference in edit4, but there's no
 * data model in the DB for it. Consider whether it makes sense,
 * if secedit uses it as well.
 *
 * Note: Don't change the result type to 'const char *' even if the functions always
 *       return the same atm. That may change.
 */

char *GBT_get_default_helix   (GBDATA *) { return strdup("HELIX"); }
char *GBT_get_default_helix_nr(GBDATA *) { return strdup("HELIX_NR"); }
char *GBT_get_default_ref     (GBDATA *) { return strdup("ECOLI"); }


// ----------------
//      scan DB

#define GBT_SUM_LEN 4096                            // maximum length of path

struct GB_DbScanner : virtual Noncopyable {
    GB_HASH   *hash_table;
    StrArray&  result; // not owned!
    char      *buffer;

    GB_DbScanner(StrArray& result_)
        : result(result_)
    {
        hash_table = GBS_create_hash(1024, GB_MIND_CASE);
        buffer     = (char*)ARB_alloc(GBT_SUM_LEN);
        buffer[0]  = 0;
    }

    ~GB_DbScanner() {
        GBS_free_hash(hash_table);
        free(buffer);
    }
};

static void gbt_scan_db_rek(GBDATA *gbd, char *prefix, int deep, GB_DbScanner *scanner) {
    GB_TYPES type = GB_read_type(gbd);
    if (type == GB_DB) {
        int len_of_prefix = strlen(prefix);
        for (GBDATA *gb2 = GB_child(gbd); gb2; gb2 = GB_nextChild(gb2)) {  // find everything
            if (deep) {
                const char *key = GB_read_key_pntr(gb2);
                if (key[0] != '@') { // skip internal containers
                    sprintf(&prefix[len_of_prefix], "/%s", key);
                    gbt_scan_db_rek(gb2, prefix, 1, scanner);
                }
            }
            else {
                prefix[len_of_prefix] = 0;
                gbt_scan_db_rek(gb2, prefix, 1, scanner);
            }
        }
        prefix[len_of_prefix] = 0;
    }
    else {
        gb_assert(!prefix[0] || GB_check_hkey(prefix) == NULL);
        LocallyModify<char> firstCharWithType(prefix[0], char(type)); // first char always is '/' -> use to store type in hash
        GBS_incr_hash(scanner->hash_table, prefix);
    }
}

struct scan_db_insert {
    GB_DbScanner *scanner;
    const char   *datapath;
};

static long gbs_scan_db_insert(const char *key, long val, void *cd_insert_data) {
    scan_db_insert *insert    = (scan_db_insert *)cd_insert_data;
    char           *to_insert = 0;

    if (!insert->datapath) {
        to_insert = strdup(key);
    }
    else {
        bool do_insert = ARB_strBeginsWith(key+1, insert->datapath);
        gb_assert(implicated(!do_insert, !ARB_strBeginsWith(insert->datapath, key+1))); // oops - previously inserted also in this case. inspect!

        if (do_insert) {                                         // datapath matches
            to_insert    = strdup(key+strlen(insert->datapath)); // cut off prefix
            to_insert[0] = key[0]; // copy type
        }
    }

    if (to_insert) {
        GB_DbScanner *scanner = insert->scanner;
        scanner->result.put(to_insert);
    }

    return val;
}

static int gbs_scan_db_compare(const void *left, const void *right, void *) {
    return strcmp((GB_CSTR)left+1, (GB_CSTR)right+1);
}


void GBT_scan_db(StrArray& fieldNames, GBDATA *gbd, const char *datapath) {
    /*! scan CONTAINER for existing sub-keys
     *
     * recurses completely downwards the DB tree
     *
     * @param fieldNames gets filled with result strings
     * - each string is the path to a node beyond gbd
     * - every string exists only once
     * - the first character of a string is the type of the entry
     * - the strings are sorted alphabetically
     * @param gbd node where search starts
     * @param datapath if != NULL, only keys with prefix datapath are scanned and
     * the prefix is removed from the resulting key_names.
     *
     * Note: this function is incredibly slow when called from clients
     */

    {
        GB_DbScanner scanner(fieldNames);
        gbt_scan_db_rek(gbd, scanner.buffer, 0, &scanner);
        scan_db_insert insert = { &scanner, datapath, };
        GBS_hash_do_loop(scanner.hash_table, gbs_scan_db_insert, &insert);
    }
    fieldNames.sort(gbs_scan_db_compare, 0);
}

// --------------------------
//      message injection

static void new_gbt_message_created_cb(GBDATA *gb_pending_messages) {
    static int avoid_deadlock = 0;

    if (!avoid_deadlock) {
        GBDATA *gb_msg;

        avoid_deadlock++;
        GB_push_transaction(gb_pending_messages);

        for (gb_msg = GB_entry(gb_pending_messages, "msg"); gb_msg;) {
            {
                const char *msg = GB_read_char_pntr(gb_msg);
                GB_warning(msg);
            }
            {
                GBDATA *gb_next_msg = GB_nextEntry(gb_msg);
                GB_delete(gb_msg);
                gb_msg              = gb_next_msg;
            }
        }

        GB_pop_transaction(gb_pending_messages);
        avoid_deadlock--;
    }
}

inline GBDATA *find_or_create_error_container(GBDATA *gb_main) {
    return GB_search(gb_main, ERROR_CONTAINER_PATH, GB_CREATE_CONTAINER);
}

void GBT_install_message_handler(GBDATA *gb_main) {
    GBDATA *gb_pending_messages;

    GB_push_transaction(gb_main);
    gb_pending_messages = find_or_create_error_container(gb_main);
    gb_assert(gb_pending_messages);
    GB_add_callback(gb_pending_messages, GB_CB_SON_CREATED, makeDatabaseCallback(new_gbt_message_created_cb));
    GB_pop_transaction(gb_main);

#if defined(DEBUG) && 0
    GBT_message(GB_get_root(gb_pending_messages), GBS_global_string("GBT_install_message_handler installed for gb_main=%p", gb_main));
#endif // DEBUG
}


void GBT_message(GBDATA *gb_main, const char *msg) {
    /*! When called in client(or server) this causes the DB server to show the message.
     * Message is shown via GB_warning (which uses aw_message in GUIs)
     *
     * Note: The message is not shown before the transaction ends.
     * If the transaction is aborted, the message is never shown!
     *
     * see also : GB_warning()
     */

    GB_ERROR error = GB_push_transaction(gb_main);

    if (!error) {
        GBDATA *gb_pending_messages = find_or_create_error_container(gb_main);
        GBDATA *gb_msg              = gb_pending_messages ? GB_create(gb_pending_messages, "msg", GB_STRING) : 0;

        if (!gb_msg) error = GB_await_error();
        else {
            gb_assert(msg);
            error = GB_write_string(gb_msg, msg);
        }
    }
    error = GB_end_transaction(gb_main, error);

    if (error) {
        fprintf(stderr, "GBT_message: Failed to write message '%s'\n(Reason: %s)\n", msg, error);
    }
}

char *GBT_read_string(GBDATA *gb_container, const char *fieldpath) {
    /*! Read value from database field (of type GB_STRING)
     *
     * @param gb_container where to start search for field
     * @param fieldpath relative path from gb_container
     *
     * @return
     * NULL in case of error (use GB_await_error()) or when field does not exist.
     * otherwise returns a heap copy.
     *
     * other functions return a pointer to a temporary variable (invalidated by next call)
     */

    GBDATA *gbd;
    char   *result = NULL;

    GB_push_transaction(gb_container);
    gbd = GB_search(gb_container, fieldpath, GB_FIND);
    if (gbd) result = GB_read_string(gbd);
    GB_pop_transaction(gb_container);
    return result;
}

char *GBT_read_as_string(GBDATA *gb_container, const char *fieldpath) {
    /*! like GBT_read_string()
     *
     * but
     * - field may be of any type (result gets converted to reasonable char*)
     */

    GBDATA *gbd;
    char   *result = NULL;

    GB_push_transaction(gb_container);
    gbd = GB_search(gb_container, fieldpath, GB_FIND);
    if (gbd) result = GB_read_as_string(gbd);
    GB_pop_transaction(gb_container);
    return result;
}

const char *GBT_read_char_pntr(GBDATA *gb_container, const char *fieldpath) {
    /*! like GBT_read_string()
     *
     * but
     * - result is not a heap-copy and may be invalidated by further DB access
     *   (especially by overwriting that fields)
     *
     * Note: Under no circumstances you may modify the result!
     */

    GBDATA     *gbd;
    const char *result = NULL;

    GB_push_transaction(gb_container);
    gbd = GB_search(gb_container, fieldpath, GB_FIND);
    if (gbd) result = GB_read_char_pntr(gbd);
    GB_pop_transaction(gb_container);
    return result;
}

NOT4PERL long *GBT_read_int(GBDATA *gb_container, const char *fieldpath) {
    /*! similar to GBT_read_string()
     *
     * but
     * - for fields of type GB_INT
     * - result gets invalidated by next call
     */

    GBDATA *gbd;
    long   *result = NULL;

    GB_push_transaction(gb_container);
    gbd = GB_search(gb_container, fieldpath, GB_FIND);
    if (gbd) {
        static long result_var;
        result_var = GB_read_int(gbd);
        result     = &result_var;
    }
    GB_pop_transaction(gb_container);
    return result;
}

NOT4PERL float *GBT_read_float(GBDATA *gb_container, const char *fieldpath) {
    /*! similar to GBT_read_string()
     *
     * but
     * - for fields of type GB_FLOAT
     * - result gets invalidated by next call
     */

    GBDATA *gbd;
    float  *result = NULL;

    GB_push_transaction(gb_container);
    gbd = GB_search(gb_container, fieldpath, GB_FIND);
    if (gbd) {
        static float result_var;
        result_var = GB_read_float(gbd);
        result     = &result_var;
    }
    GB_pop_transaction(gb_container);
    return result;
}

char *GBT_readOrCreate_string(GBDATA *gb_container, const char *fieldpath, const char *default_value) {
    /*! like GBT_read_string(),
     *
     * but if field does not exist, it will be created and initialized with 'default_value'
     */
    GBDATA *gb_string;
    char   *result = NULL;

    GB_push_transaction(gb_container);
    gb_string             = GB_searchOrCreate_string(gb_container, fieldpath, default_value);
    if (gb_string) result = GB_read_string(gb_string);
    GB_pop_transaction(gb_container);
    return result;
}

const char *GBT_readOrCreate_char_pntr(GBDATA *gb_container, const char *fieldpath, const char *default_value) {
    /*! like GBT_read_char_pntr(),
     *
     * but if field does not exist, it will be created and initialized with 'default_value'
     */

    GBDATA     *gb_string;
    const char *result = NULL;

    GB_push_transaction(gb_container);
    gb_string             = GB_searchOrCreate_string(gb_container, fieldpath, default_value);
    if (gb_string) result = GB_read_char_pntr(gb_string);
    GB_pop_transaction(gb_container);
    return result;
}

NOT4PERL long *GBT_readOrCreate_int(GBDATA *gb_container, const char *fieldpath, long default_value) {
    /*! like GBT_read_int(),
     *
     * but if field does not exist, it will be created and initialized with 'default_value'
     */

    GBDATA *gb_int;
    long   *result = NULL;

    GB_push_transaction(gb_container);
    gb_int = GB_searchOrCreate_int(gb_container, fieldpath, default_value);
    if (gb_int) {
        static long result_var;
        result_var = GB_read_int(gb_int);
        result     = &result_var;
    }
    GB_pop_transaction(gb_container);
    return result;
}

NOT4PERL float *GBT_readOrCreate_float(GBDATA *gb_container, const char *fieldpath, float default_value) {
    /*! like GBT_read_float(),
     *
     * but if field does not exist, it will be created and initialized with 'default_value'
     */

    gb_assert(default_value == default_value); // !nan

    GBDATA *gb_float;
    float  *result = NULL;

    GB_push_transaction(gb_container);
    gb_float = GB_searchOrCreate_float(gb_container, fieldpath, default_value);
    if (gb_float) {
        static float result_var;
        result_var = GB_read_float(gb_float);
        result     = &result_var;
    }
    else {
        gb_assert(0);
    }
    GB_pop_transaction(gb_container);
    return result;
}

// -------------------------------------------------------------------
//      overwrite existing or create new database field
//      (field must not exist twice or more - it has to be unique!!)

GB_ERROR GBT_write_string(GBDATA *gb_container, const char *fieldpath, const char *content) {
    /*! Write content to database field of type GB_STRING
     *
     * if field exists, it will be overwritten.
     * if field does not exist, it will be created.
     *
     * The field should be unique, i.e. it should not exist twice or more in it's parent container
     *
     * @return GB_ERROR on failure
     */
    GB_ERROR  error = GB_push_transaction(gb_container); // @@@ result unused
    GBDATA   *gbd   = GB_search(gb_container, fieldpath, GB_STRING);
    if (!gbd) error = GB_await_error();
    else {
        error = GB_write_string(gbd, content);
        gb_assert(GB_nextEntry(gbd) == 0); // only one entry should exist (sure you want to use this function?)
    }
    return GB_end_transaction(gb_container, error);
}

GB_ERROR GBT_write_int(GBDATA *gb_container, const char *fieldpath, long content) {
    /*! like GBT_write_string(),
     *
     * but for fields of type GB_INT
     */
    GB_ERROR  error = GB_push_transaction(gb_container); // @@@ result unused
    GBDATA   *gbd   = GB_search(gb_container, fieldpath, GB_INT);
    if (!gbd) error = GB_await_error();
    else {
        error = GB_write_int(gbd, content);
        gb_assert(GB_nextEntry(gbd) == 0); // only one entry should exist (sure you want to use this function?)
    }
    return GB_end_transaction(gb_container, error);
}

GB_ERROR GBT_write_byte(GBDATA *gb_container, const char *fieldpath, unsigned char content) {
    /*! like GBT_write_string(),
     *
     * but for fields of type GB_BYTE
     */
    GB_ERROR  error = GB_push_transaction(gb_container); // @@@ result unused
    GBDATA   *gbd   = GB_search(gb_container, fieldpath, GB_BYTE);
    if (!gbd) error = GB_await_error();
    else {
        error = GB_write_byte(gbd, content);
        gb_assert(GB_nextEntry(gbd) == 0); // only one entry should exist (sure you want to use this function?)
    }
    return GB_end_transaction(gb_container, error);
}


GB_ERROR GBT_write_float(GBDATA *gb_container, const char *fieldpath, float content) {
    /*! like GBT_write_string(),
     *
     * but for fields of type GB_FLOAT
     */

    gb_assert(content == content); // !nan

    GB_ERROR  error = GB_push_transaction(gb_container); // @@@ result unused
    GBDATA   *gbd   = GB_search(gb_container, fieldpath, GB_FLOAT);
    if (!gbd) error = GB_await_error();
    else {
        error = GB_write_float(gbd, content);
        gb_assert(GB_nextEntry(gbd) == 0); // only one entry should exist (sure you want to use this function?)
    }
    return GB_end_transaction(gb_container, error);
}


static GBDATA *GB_test_link_follower(GBDATA *gb_main, GBDATA */*gb_link*/, const char *link) {
    GBDATA *linktarget = GB_search(gb_main, "tmp/link/string", GB_STRING);
    GB_write_string(linktarget, GBS_global_string("Link is '%s'", link));
    return GB_get_father(linktarget);
}

// --------------------
//      save & load

GBDATA *GBT_open(const char *path, const char *opent) {
    /*! Open a database (as GB_open does)
     *  Additionally:
     *  - disable saving in the PT_SERVER directory,
     *  - create an index for species and extended names (server only!),
     *  - install table link followers (maybe obsolete)
     *
     * @param path filename of the DB
     * @param opent see GB_login()
     * @return database handle (or NULL in which case an error is exported)
     * @see GB_open()
     */

    GBDATA *gbd = GB_open(path, opent);
    if (gbd) {
        GB_disable_path(gbd, GB_path_in_ARBLIB("pts/*"));
        GB_ERROR error = NULL;
        {
            GB_transaction ta(gbd);

            if (!strchr(path, ':')) {
                GBDATA *species_data = GB_search(gbd, "species_data", GB_FIND); // do NOT create species_data here!
                if (species_data) {
                    long hash_size = max(GB_number_of_subentries(species_data), GBT_SPECIES_INDEX_SIZE);
                    error          = GB_create_index(species_data, "name", GB_IGNORE_CASE, hash_size);

                    if (!error) {
                        GBDATA *extended_data = GBT_get_SAI_data(gbd);
                        hash_size             = max(GB_number_of_subentries(extended_data), GBT_SAI_INDEX_SIZE);
                        error                 = GB_create_index(extended_data, "name", GB_IGNORE_CASE, hash_size);
                    }
                }
            }
            if (!error) {
                {
                    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
                    Main->table_hash = GBS_create_hash(256, GB_MIND_CASE);
                    GB_install_link_follower(gbd, "REF", GB_test_link_follower);
                }
                GBT_install_table_link_follower(gbd);
            }
        }
        if (error) {
            GB_close(gbd);
            gbd = NULL;
            GB_export_error(error);
        }
    }
    gb_assert(contradicted(gbd, GB_have_error()));
    return gbd;
}

/* --------------------------------------------------------------------------------
 * Remote commands
 *
 * Note: These commands may seem to be unused from inside ARB.
 * They get used, but only indirectly via the macro-function.
 *
 * Search for
 * - BIO::remote_action         (use of GBT_remote_action)
 * - BIO::remote_awar           (use of GBT_remote_awar)
 * - BIO::remote_read_awar      (use of GBT_remote_read_awar)
 */

static GBDATA *wait_for_dbentry(GBDATA *gb_main, const char *entry, const ARB_timeout *timeout, bool verbose) {
    // if 'timeout' != NULL -> abort and return NULL if timeout has passed (otherwise wait forever)

    if (verbose) GB_warningf("[waiting for DBENTRY '%s']", entry);
    GBDATA *gbd;
    {
        ARB_timestamp  start;
        MacroTalkSleep increasing;

        while (1) {
            GB_begin_transaction(gb_main);
            gbd = GB_search(gb_main, entry, GB_FIND);
            GB_commit_transaction(gb_main);
            if (gbd) break;
            if (timeout && timeout->passed()) break;
            increasing.sleep();
        }
    }
    if (gbd && verbose) GB_warningf("[found DBENTRY '%s']", entry);
    return gbd;
}

static GB_ERROR gbt_wait_for_remote_action(GBDATA *gb_main, GBDATA *gb_action, const char *awar_read) {
    // waits until remote action has finished

    MacroTalkSleep increasing;
    GB_ERROR error = 0;
    while (!error) {
        increasing.sleep();
        error = GB_begin_transaction(gb_main);
        if (!error) {
            char *ac = GB_read_string(gb_action);
            if (ac[0] == 0) { // action has been cleared from remote side
                GBDATA *gb_result = GB_search(gb_main, awar_read, GB_STRING);
                error             = GB_read_char_pntr(gb_result); // check for errors
            }
            free(ac);
        }
        error = GB_end_transaction(gb_main, error);
    }

    if (error && !error[0]) return NULL; // empty error means ok
    return error; // may be error or result
}

static void mark_as_macro_executor(GBDATA *gb_main, bool mark) {
    // call this with 'mark' = true to mark yourself as "client running a macro"
    // -> GB_close will automatically announce termination of this client to main DB (MACRO_TRIGGER_TERMINATED)
    // do NOT call with 'mark' = false

    static bool client_is_macro_executor = false;
    if (mark) {
        if (!client_is_macro_executor) {
            GB_atclose(gb_main, (void (*)(GBDATA*, void*))mark_as_macro_executor, 0);
            client_is_macro_executor = true;
        }
    }
    else {
        // called via GB_atclose callback (i.e. at end of currently executed macro file)
        GB_ERROR error = NULL;
        gb_assert(client_is_macro_executor);
        if (client_is_macro_executor) {
            GB_transaction ta(gb_main);

            GBDATA *gb_terminated = GB_search(gb_main, MACRO_TRIGGER_TERMINATED, GB_FIND);
            gb_assert(gb_terminated); // should have been created by macro caller
            if (gb_terminated) {
                error = GB_write_int(gb_terminated, GB_read_int(gb_terminated)+1); // notify macro caller
            }

            error = ta.close(error);
        }
        if (error) GBT_message(gb_main, error);
    }
}

inline GB_ERROR set_intEntry_to(GBDATA *gb_main, const char *path, int value) {
    GBDATA *gbd = GB_searchOrCreate_int(gb_main, path, value);
    return gbd ? GB_write_int(gbd, value) : GB_await_error();
}

#if defined(DEBUG)
// # define DUMP_AUTH_HANDSHAKE // see also ../SL/MACROS/dbserver.cxx@DUMP_AUTHORIZATION
#endif

#if defined(DUMP_AUTH_HANDSHAKE)
# define IF_DUMP_HANDSHAKE(cmd) cmd
#else
# define IF_DUMP_HANDSHAKE(cmd)
#endif

GB_ERROR GB_set_macro_error(GBDATA *gb_main, const char *curr_error) {
    GB_ERROR        error          = NULL;
    GB_transaction  ta(gb_main);
    GBDATA         *gb_macro_error = GB_searchOrCreate_string(gb_main, MACRO_TRIGGER_ERROR, curr_error);
    if (gb_macro_error) {
        const char *prev_error = GB_read_char_pntr(gb_macro_error);
        if (prev_error && prev_error[0]) { // already have an error
            if (strstr(prev_error, curr_error) == 0) { // do not add message twice
                error = GB_write_string(gb_macro_error, GBS_global_string("%s\n%s", prev_error, curr_error));
            }
        }
        else {
            error = GB_write_string(gb_macro_error, curr_error);
        }
    }
    return error;
}
GB_ERROR GB_get_macro_error(GBDATA *gb_main) {
    GB_ERROR error = NULL;

    GB_transaction  ta(gb_main);
    GBDATA         *gb_macro_error = GB_search(gb_main, MACRO_TRIGGER_ERROR, GB_FIND);
    if (gb_macro_error) {
        const char *macro_error       = GB_read_char_pntr(gb_macro_error);
        if (!macro_error) macro_error = GBS_global_string("failed to retrieve error message (Reason: %s)", GB_await_error());
        if (macro_error[0]) error     = GBS_global_string("macro-error: %s", macro_error);
    }
    return error;
}
GB_ERROR GB_clear_macro_error(GBDATA *gb_main) {
    GB_transaction  ta(gb_main);
    GB_ERROR        error          = NULL;
    GBDATA         *gb_macro_error = GB_search(gb_main, MACRO_TRIGGER_ERROR, GB_FIND);
    if (gb_macro_error) error      = GB_write_string(gb_macro_error, "");
    return error;
}

static GB_ERROR start_remote_command_for_application(GBDATA *gb_main, const remote_awars& remote, const ARB_timeout *timeout, bool verbose) {
    // Called before any remote command will be written to DB.
    // Application specific initialization is done here.
    //
    // if 'timeout' != NULL -> abort with error if timeout has passed (otherwise wait forever)

    ARB_timestamp start;
    bool          wait_for_app = false;

    GB_ERROR error    = GB_begin_transaction(gb_main);
    if (!error) error = GB_get_macro_error(gb_main);
    if (!error) {
        GBDATA *gb_granted     = GB_searchOrCreate_int(gb_main, remote.granted(), 0);
        if (!gb_granted) error = GB_await_error();
        else {
            if (GB_read_int(gb_granted)) {
                // ok - authorization already granted
            }
            else {
                error        = set_intEntry_to(gb_main, remote.authReq(), 1); // ask client to ack execution
                wait_for_app = !error;

                if (!error) {
                    IF_DUMP_HANDSHAKE(fprintf(stderr, "AUTH_HANDSHAKE [set %s to 1]\n", remote.authReq()));
                }
            }
        }
    }
    error = GB_end_transaction(gb_main, error);

    MacroTalkSleep increasing;
    while (wait_for_app) {
        gb_assert(!error);

        IF_DUMP_HANDSHAKE(fprintf(stderr, "AUTH_HANDSHAKE [waiting for %s]\n", remote.authAck()));

        GBDATA *gb_authAck = wait_for_dbentry(gb_main, remote.authAck(), timeout, verbose);
        if (gb_authAck) {
            error = GB_begin_transaction(gb_main);
            if (!error) {
                long ack_pid = GB_read_int(gb_authAck);
                if (ack_pid) {
                    IF_DUMP_HANDSHAKE(fprintf(stderr, "AUTH_HANDSHAKE [got authAck %li]\n", ack_pid));
                    
                    GBDATA *gb_granted = GB_searchOrCreate_int(gb_main, remote.granted(), ack_pid);
                    long    old_pid    = GB_read_int(gb_granted);

                    if (old_pid != ack_pid) { // we have two applications with same id that acknowledged the execution request
                        if (old_pid == 0) {
                            error             = GB_write_int(gb_granted, ack_pid); // grant rights to execute remote-command to ack_pid
                            if (!error) error = GB_write_int(gb_authAck, 0);       // allow a second application to acknowledge the request
                            if (!error) {
                                wait_for_app = false;
                                IF_DUMP_HANDSHAKE(fprintf(stderr, "AUTH_HANDSHAKE [granted permission to execute macros to pid %li]\n", ack_pid));
                            }
                        }
                    }
                    else {
                        error = GB_write_int(gb_authAck, 0); // allow a second application to acknowledge the request
                    }
                }
                else {
                    // old entry with value 0 -> wait until client acknowledges authReq
                    IF_DUMP_HANDSHAKE(fprintf(stderr, "AUTH_HANDSHAKE [no %s yet]\n", remote.authAck()));
                }
            }
            error = GB_end_transaction(gb_main, error);
        }
        else {
            // happens after timeout (handled by next if-clause)
        }

        if (timeout && timeout->passed()) {
            wait_for_app = false;
            error        = "remote application did not answer (within timeout)";
        }

        if (wait_for_app) {
            increasing.sleep();
        }
    }

    return error;
}

NOT4PERL GB_ERROR GBT_remote_action_with_timeout(GBDATA *gb_main, const char *application, const char *action_name, const class ARB_timeout *timeout, bool verbose) {
    // if timeout_ms > 0 -> abort with error if application does not answer (e.g. is not running)
    // Note: opposed to GBT_remote_action, this function may NOT be called directly by perl-macros

    remote_awars remote(application);
    GB_ERROR     error = start_remote_command_for_application(gb_main, remote, timeout, verbose);

    if (!error) {
        GBDATA *gb_action = wait_for_dbentry(gb_main, remote.action(), NULL, false);
        error             = GB_begin_transaction(gb_main);
        if (!error) error = GB_write_string(gb_action, action_name); // write command
        error             = GB_end_transaction(gb_main, error);
        if (!error) error = gbt_wait_for_remote_action(gb_main, gb_action, remote.result());
    }
    return error;
}

GB_ERROR GBT_remote_action(GBDATA *gb_main, const char *application, const char *action_name) {
    // needs to be public (used exclusively(!) by perl-macros)
    mark_as_macro_executor(gb_main, true);
    return GBT_remote_action_with_timeout(gb_main, application, action_name, 0, true);
}

GB_ERROR GBT_remote_awar(GBDATA *gb_main, const char *application, const char *awar_name, const char *value) {
    // needs to be public (used exclusively(!) by perl-macros)

    mark_as_macro_executor(gb_main, true);

    remote_awars remote(application);
    GB_ERROR     error = start_remote_command_for_application(gb_main, remote, 0, true);

    if (!error) {
        GBDATA *gb_awar   = wait_for_dbentry(gb_main, remote.awar(), 0, false);
        error             = GB_begin_transaction(gb_main);
        if (!error) error = GB_write_string(gb_awar, awar_name);
        if (!error) error = GBT_write_string(gb_main, remote.value(), value);
        error             = GB_end_transaction(gb_main, error);
        if (!error) error = gbt_wait_for_remote_action(gb_main, gb_awar, remote.result());
    }
    return error;
}

GB_ERROR GBT_remote_read_awar(GBDATA *gb_main, const char *application, const char *awar_name) {
    // needs to be public (used exclusively(!) by perl-macros)

    mark_as_macro_executor(gb_main, true);

    remote_awars remote(application);
    GB_ERROR     error = start_remote_command_for_application(gb_main, remote, 0, true);

    if (!error) {
        GBDATA *gb_awar   = wait_for_dbentry(gb_main, remote.awar(), 0, false);
        error             = GB_begin_transaction(gb_main);
        if (!error) error = GB_write_string(gb_awar, awar_name);
        if (!error) error = GBT_write_string(gb_main, remote.action(), "AWAR_REMOTE_READ");
        error             = GB_end_transaction(gb_main, error);
        if (!error) error = gbt_wait_for_remote_action(gb_main, gb_awar, remote.value());
    }
    return error;
}

inline char *find_macro_in(const char *dir, const char *macroname) {
    char *full = GBS_global_string_copy("%s/%s", dir, macroname);
    if (!GB_is_readablefile(full)) {
        freeset(full, GBS_global_string_copy("%s.amc", full));
        if (!GB_is_readablefile(full)) freenull(full);
    }
    return full;
}

static char *fullMacroname(const char *macro_name) {
    /*! detect full path of 'macro_name'
     * @param macro_name full path or path relative to ARBMACRO or ARBMACROHOME
     * @return full path or NULL (in which case an error is exported)
     */

    gb_assert(!GB_have_error());

    if (GB_is_readablefile(macro_name)) return strdup(macro_name);

    char *in_ARBMACROHOME = find_macro_in(GB_getenvARBMACROHOME(), macro_name);
    char *in_ARBMACRO     = find_macro_in(GB_getenvARBMACRO(),     macro_name);
    char *result          = NULL;

    if (in_ARBMACROHOME) {
        if (in_ARBMACRO) {
            GB_export_errorf("ambiguous macro name '%s'\n"
                             "('%s' and\n"
                             " '%s' exist both.\n"
                             " You have to rename or delete one of them!)",
                             macro_name, in_ARBMACROHOME, in_ARBMACRO);
        }
        else reassign(result, in_ARBMACROHOME);
    }
    else {
        if (in_ARBMACRO) reassign(result, in_ARBMACRO);
        else GB_export_errorf("Failed to detect macro '%s'", macro_name);
    }

    free(in_ARBMACRO);
    free(in_ARBMACROHOME);

    gb_assert(contradicted(result, GB_have_error()));

    return result;
}

inline const char *relative_inside(const char *dir, const char *fullpath) {
    if (ARB_strBeginsWith(fullpath, dir)) {
        const char *result = fullpath+strlen(dir);
        if (result[0] == '/') return result+1;
    }
    return NULL;
}

const char *GBT_relativeMacroname(const char *macro_name) {
    /*! make macro_name relative if it is located in or below ARBMACROHOME or ARBMACRO.
     * Inverse function of fullMacroname()
     * @return pointer into macro_name (relative part) or macro_name itself (if macro is located somewhere else)
     */

    const char *result  = relative_inside(GB_getenvARBMACROHOME(), macro_name);
    if (!result) result = relative_inside(GB_getenvARBMACRO(), macro_name);
    if (!result) result = macro_name;
    return result;
}

GB_ERROR GBT_macro_execute(const char *macro_name, bool loop_marked, bool run_async) {
    /*! simply execute a macro
     *
     * In doubt: do not use this function, instead use ../SL/MACROS/macros.hxx@execute_macro
     *
     * @param macro_name contains the macro filename (either with full path or relative to directories pointed to by ARBMACRO or ARBMACROHOME)
     * @param loop_marked if true -> call macro once for each marked species
     * @param run_async if true -> run perl asynchronously (will not detect perl failure in that case!)
     */

    GB_ERROR  error     = NULL;
    char     *fullMacro = fullMacroname(macro_name);
    if (!fullMacro) {
        error = GB_await_error();
    }
    else {
        char *perl_args = NULL;
        freeset(fullMacro, GBK_singlequote(fullMacro));
        if (loop_marked) {
            const char *with_all_marked = GB_path_in_ARBHOME("PERL_SCRIPTS/MACROS/with_all_marked.pl");
            perl_args = GBS_global_string_copy("'%s' %s", with_all_marked, fullMacro);
        }
        else {
            perl_args = GBS_global_string_copy("%s", fullMacro);
        }

        char *cmd = GBS_global_string_copy("perl %s %s", perl_args, run_async ? "&" : "");

        error = GBK_system(cmd);

        free(cmd);
        free(perl_args);
        free(fullMacro);
    }
    return error;
}

// ---------------------------
//      self-notification
// ---------------------------
// provides a mechanism to notify ARB after some external tool finishes

#define ARB_NOTIFICATIONS "tmp/notify"

/* DB structure for notifications :
 *
 * ARB_NOTIFICATIONS/counter        GB_INT      counts used ids
 * ARB_NOTIFICATIONS/notify/id      GB_INT      id of notification
 * ARB_NOTIFICATIONS/notify/message GB_STRING   message of notification (set by callback)
 */

typedef void (*notify_cb_type)(const char *message, void *client_data);

struct NotifyCb {
    notify_cb_type  cb;
    void           *client_data;
};

static void notify_cb(GBDATA *gb_message, NotifyCb *pending, GB_CB_TYPE cb_type) {
    if (cb_type != GB_CB_DELETE) {
        GB_remove_callback(gb_message, GB_CB_CHANGED_OR_DELETED, makeDatabaseCallback(notify_cb, pending));
    }

    if (cb_type == GB_CB_CHANGED) {
        int         cb_done = 0;
        GB_ERROR    error   = 0;
        const char *message = GB_read_char_pntr(gb_message);
        if (!message) error = GB_await_error();
        else {
            pending->cb(message, pending->client_data);
            cb_done = 1;
        }

        if (!cb_done) {
            gb_assert(error);
            GB_warningf("Notification failed (Reason: %s)\n", error);
            gb_assert(0);
        }
    }
    else { // called from GB_remove_last_notification
        gb_assert(cb_type == GB_CB_DELETE);
    }

    free(pending);
}

static int allocateNotificationID(GBDATA *gb_main, NotifyCb *pending) {
    /* returns a unique notification ID
     * or 0 (use GB_get_error() in this case)
     */

    int      id    = 0;
    GB_ERROR error = GB_push_transaction(gb_main);

    if (!error) {
        GBDATA *gb_notify = GB_search(gb_main, ARB_NOTIFICATIONS, GB_CREATE_CONTAINER);

        if (gb_notify) {
            GBDATA *gb_counter = GB_searchOrCreate_int(gb_notify, "counter", 0);

            if (gb_counter) {
                int newid = GB_read_int(gb_counter) + 1; // increment counter
                error     = GB_write_int(gb_counter, newid);

                if (!error) {
                    // change transaction (to never use id twice!)
                    error             = GB_pop_transaction(gb_main);
                    if (!error) error = GB_push_transaction(gb_main);

                    if (!error) {
                        GBDATA *gb_notification = GB_create_container(gb_notify, "notify");

                        if (gb_notification) {
                            error = GBT_write_int(gb_notification, "id", newid);
                            if (!error) {
                                GBDATA *gb_message = GB_searchOrCreate_string(gb_notification, "message", "");

                                if (gb_message) {
                                    error = GB_add_callback(gb_message, GB_CB_CHANGED_OR_DELETED, makeDatabaseCallback(notify_cb, pending));
                                    if (!error) {
                                        id = newid; // success
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (!id) {
        if (!error) error = GB_await_error();
        error = GBS_global_string("Failed to allocate notification ID (%s)", error);
    }
    error = GB_end_transaction(gb_main, error);
    if (error) GB_export_error(error);

    return id;
}


char *GB_generate_notification(GBDATA *gb_main,
                               void (*cb)(const char *message, void *client_data),
                               const char *message, void *client_data)
{
    /* generates a call to 'arb_notify', meant to be inserted into some external system call.
     * When that call is executed, the callback instantiated here will be called.
     *
     * Tip : To return variable results from the shell script, use the name of an environment
     *       variable in 'message' (e.g. "$RESULT")
     */

    int       id;
    char     *arb_notify_call = 0;
    NotifyCb *pending         = (NotifyCb*)ARB_alloc(sizeof(*pending));

    pending->cb          = cb;
    pending->client_data = client_data;

    id = allocateNotificationID(gb_main, pending);
    if (id) {
        arb_notify_call = GBS_global_string_copy("arb_notify %i \"%s\"", id, message);
    }
    else {
        free(pending);
    }

    return arb_notify_call;
}

GB_ERROR GB_remove_last_notification(GBDATA *gb_main) {
    // aborts the last notification
    GB_ERROR error = GB_push_transaction(gb_main);

    if (!error) {
        GBDATA *gb_notify = GB_search(gb_main, ARB_NOTIFICATIONS, GB_CREATE_CONTAINER);
        if (gb_notify) {
            GBDATA *gb_counter = GB_entry(gb_notify, "counter");
            if (gb_counter) {
                int     id    = GB_read_int(gb_counter);
                GBDATA *gb_id = GB_find_int(gb_notify, "id", id, SEARCH_GRANDCHILD);

                if (!gb_id) {
                    error = GBS_global_string("No notification for ID %i", id);
                    gb_assert(0);           // GB_generate_notification() has not been called for 'id'!
                }
                else {
                    GBDATA *gb_message = GB_brother(gb_id, "message");

                    if (!gb_message) {
                        error = "Missing 'message' entry";
                    }
                    else {
                        error = GB_delete(gb_message); // calls notify_cb
                    }
                }
            }
            else {
                error = "No notification generated yet";
            }
        }
    }

    error = GB_end_transaction(gb_main, error);
    return error;
}

GB_ERROR GB_notify(GBDATA *gb_main, int id, const char *message) {
    /* called via 'arb_notify'
     * 'id' has to be generated by GB_generate_notification()
     * 'message' is passed to notification callback belonging to id
     */

    GB_ERROR  error     = 0;
    GBDATA   *gb_notify = GB_search(gb_main, ARB_NOTIFICATIONS, GB_FIND);

    if (!gb_notify) {
        error = "Missing notification data";
        gb_assert(0);           // GB_generate_notification() has not been called!
    }
    else {
        GBDATA *gb_id = GB_find_int(gb_notify, "id", id, SEARCH_GRANDCHILD);

        if (!gb_id) {
            error = GBS_global_string("No notification for ID %i", id);
        }
        else {
            GBDATA *gb_message = GB_brother(gb_id, "message");

            if (!gb_message) {
                error = "Missing 'message' entry";
            }
            else {
                // callback the instantiating DB client
                error = GB_write_string(gb_message, message);
            }
        }
    }

    return error;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>

#define TEST_EXPECT_SCANNED_EQUALS(path,expected) do {  \
        GB_transaction ta(gb_main);                     \
        StrArray fields;                                \
        GBT_scan_db(fields, gb_main, path);             \
        char  *joined = GBT_join_strings(fields, ',');  \
        TEST_EXPECT_EQUAL(joined, expected);            \
        free(joined);                                   \
    } while (0)

void TEST_scan_db() {
    GB_shell  shell;
    GBDATA   *gb_main = GB_open("TEST_loadsave.arb", "r");


    TEST_EXPECT_SCANNED_EQUALS(NULL,
                               "alignment/aligned,alignment/alignment_len,alignment/alignment_name,alignment/alignment_rem,alignment/alignment_type,alignment/alignment_write_security,alignment/auto_format,"
                               "byte,bytes,bytes,bytes,"
                               "extended/acc,extended/ali_16s/data,extended/aligned,extended/errors,extended/full_name,extended/name,"
                               "\nfloats,\tints,\tints_empty,"
                               "key_data/key/key_hidden,key_data/key/key_name,key_data/key/key_type,"
                               "species/AL,species/ARB_color,species/acc,species/ali_16s/data,species/bits_test,species/float_test,species/full_name,species/name,species/seqcheck,species/tax,"
                               "str,str_percent,use");

    TEST_EXPECT_SCANNED_EQUALS("species", "/AL,/ARB_color,/acc,/ali_16s/data,/bits_test,/float_test,/full_name,/name,/seqcheck,/tax");

    TEST_REJECT(GB_have_error());
    
    GB_close(gb_main);
}

static arb_test::match_expectation macroFoundAs(const char *shortName, const char *expectedFullName_tmp, const char *expectedRelName, const char *partOfError) {
    char *expectedFullName = nulldup(expectedFullName_tmp);

    using namespace   arb_test;
    expectation_group expected;

    {
        char *found = fullMacroname(shortName);
        expected.add(that(found).is_equal_to(expectedFullName));
        if (found) {
            expected.add(that(GBT_relativeMacroname(found)).is_equal_to(expectedRelName));
        }
        free(found);
    }

    GB_ERROR error = GB_have_error() ? GB_await_error() : NULL;
    if (partOfError) {
        if (error) {
            expected.add(that(error).does_contain(partOfError));
        }
        else {
            expected.add(that(error).does_differ_from_NULL());
        }
    }
    else {
        expected.add(that(error).is_equal_to_NULL());
    }

    free(expectedFullName);
    return all().ofgroup(expected);
}

#define TEST_FULLMACRO_EQUALS(shortName,fullName,relName) TEST_EXPECTATION(macroFoundAs(shortName, fullName, relName, NULL))
#define TEST_FULLMACRO_FAILS(shortName,expectedErrorPart) TEST_EXPECTATION(macroFoundAs(shortName, NULL, NULL, expectedErrorPart))

void TEST_find_macros() {
    gb_getenv_hook old = GB_install_getenv_hook(arb_test::fakeenv);

#define TEST         "test"
#define RESERVED     "reserved4ut"
#define TEST_AMC     TEST ".amc"
#define RESERVED_AMC RESERVED ".amc"

    // unlink test.amc in ARBMACROHOME (from previous run)
    // ../UNIT_TESTER/run/homefake/.arb_prop/macros
    char *test_amc = strdup(GB_concat_path(GB_getenvARBMACROHOME(), TEST_AMC));
    char *res_amc  = strdup(GB_concat_path(GB_getenvARBMACROHOME(), RESERVED_AMC));

    TEST_EXPECT_DIFFERENT(GB_unlink(test_amc), -1);
    TEST_EXPECT_DIFFERENT(GB_unlink(res_amc), -1);

    // check if it finds macros in ARBMACRO = ../lib/macros
    TEST_FULLMACRO_EQUALS(TEST, GB_path_in_ARBLIB("macros/" TEST_AMC), TEST_AMC);

    // searching reserved4ut.amc should fail now (macro should not exists)
    TEST_FULLMACRO_FAILS(RESERVED, "Failed to detect");

    // --------------------------------
    // create 2 macros in ARBMACROHOME
    TEST_EXPECT_NO_ERROR(GBK_system(GBS_global_string("touch '%s'; touch '%s'", test_amc, res_amc)));

    // searching test.amc should fail now (macro exists in ARBMACROHOME and ARBMACRO)
    TEST_FULLMACRO_FAILS(TEST, "ambiguous macro name");

    // check if it finds macros in ARBMACROHOME = ../UNIT_TESTER/run/homefake/.arb_prop/macros
    TEST_FULLMACRO_EQUALS(RESERVED, res_amc, RESERVED_AMC);

    free(res_amc);
    free(test_amc);

    TEST_EXPECT_EQUAL((void*)arb_test::fakeenv, (void*)GB_install_getenv_hook(old));
}
TEST_PUBLISH(TEST_find_macros);

#endif // UNIT_TESTS
