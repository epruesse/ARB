/* ============================================================ */
/*                                                              */
/*   File      : adtools.c                                      */
/*   Purpose   : misc function                                  */
/*                                                              */
/*   Institute of Microbiology (Technical University Munich)    */
/*   www.arb-home.de                                            */
/*                                                              */
/* ============================================================ */

#include <string.h>
#include <stdlib.h>

#include "adlocal.h"
#include "arbdbt.h"

GBDATA *GBT_find_or_create(GBDATA *Main,const char *key,long delete_level)
{
    GBDATA *gbd;
    gbd = GB_entry(Main,key);
    if (gbd) return gbd;
    gbd = GB_create_container(Main,key);
    GB_write_security_delete(gbd,delete_level);
    return gbd;
}


/* the following function were meant to use user defined values.
 *
 * Especially for 'ECOLI' there is already a possibility to
 * specify a different reference in edit4, but there's no
 * data model in the DB for it. Consider whether it makes sense,
 * if secedit uses it as well.
 *
 * Note: Don't change the result type to 'const char *' even if the functions always
 *       return the same atm. That may change.
 */

char *GBT_get_default_helix(GBDATA *gb_main) {
    GBUSE(gb_main);
    return strdup("HELIX");
}

char *GBT_get_default_helix_nr(GBDATA *gb_main) {
    GBUSE(gb_main);
    return strdup("HELIX_NR");
}

char *GBT_get_default_ref(GBDATA *gb_main) {
    GBUSE(gb_main);
    return strdup("ECOLI");
}


/********************************************************************************************
                    check routines
********************************************************************************************/
GB_ERROR GBT_check_arb_file(const char *name)
     /* Checks whether the name of a file seemed to be an arb file */
     /* if == 0 it was an arb file */
{
    FILE *in;
    long i;
    char buffer[100];
    if (strchr(name,':')) return 0;
    in = fopen(name,"r");
    if (!in) return GB_export_errorf("Cannot find file '%s'",name);
    i = gb_read_in_long(in, 0);
    if ( (i== 0x56430176) || (i == GBTUM_MAGIC_NUMBER) || (i == GBTUM_MAGIC_REVERSED)) {
        fclose(in);
        return 0;
    }
    rewind(in);
    fgets(buffer,50,in);
    fclose(in);
    if (!strncmp(buffer,"/*ARBDB AS",10)) {
        return 0;
    };


    return GB_export_errorf("'%s' is not an arb file",name);
}

/********************************************************************************************
                    analyse the database
********************************************************************************************/
#define GBT_SUM_LEN 4096
/* maximum length of path */

struct DbScanner {
    GB_HASH   *hash_table;
    int        count;
    char     **result;
    GB_TYPES   type;
    char      *buffer;
};

static void gbt_scan_db_rek(GBDATA *gbd,char *prefix, int deep, struct DbScanner *scanner) {
    GB_TYPES type = GB_read_type(gbd);
    GBDATA *gb2;
    const char *key;
    int len_of_prefix;
    if (type == GB_DB) {
        len_of_prefix = strlen(prefix);
        for (gb2 = GB_child(gbd); gb2; gb2 = GB_nextChild(gb2)) {  /* find everything */
            if (deep){
                key = GB_read_key_pntr(gb2);
                sprintf(&prefix[len_of_prefix],"/%s",key);
                gbt_scan_db_rek(gb2, prefix, 1, scanner);
            }
            else {
                prefix[len_of_prefix] = 0;
                gbt_scan_db_rek(gb2, prefix, 1, scanner);
            }
        }
        prefix[len_of_prefix] = 0;
    }
    else {
        if (GB_check_hkey(prefix+1)) {
            prefix = prefix;        /* for debugging purpose */
        }
        else {
            prefix[0] = (char)type;
            GBS_incr_hash(scanner->hash_table, prefix);
        }
    }
}

static long gbs_scan_db_count(const char *key, long val, void *cd_scanner) {
    struct DbScanner *scanner = (struct DbScanner*)cd_scanner;

    scanner->count++;
    key = key;

    return val;
}

struct scan_db_insert {
    struct DbScanner *scanner;
    const char       *datapath;
};

static long gbs_scan_db_insert(const char *key, long val, void *cd_insert_data) {
    struct scan_db_insert *insert = (struct scan_db_insert *)cd_insert_data;
    char *to_insert = 0;

    if (!insert->datapath) {
        to_insert = strdup(key);
    }
    else {
        if (GBS_strscmp(insert->datapath, key+1) == 0) { // datapath matches
            to_insert    = strdup(key+strlen(insert->datapath)); // cut off prefix
            to_insert[0] = key[0]; // copy type
        }
    }

    if (to_insert) {
        struct DbScanner *scanner         = insert->scanner;
        scanner->result[scanner->count++] = to_insert;
    }

    return val;
}

static int gbs_scan_db_compare(const void *left, const void *right, void *unused){
    GBUSE(unused);
    return strcmp((GB_CSTR)left+1, (GB_CSTR)right+1);
}


char **GBT_scan_db(GBDATA *gbd, const char *datapath) {
    /* returns a NULL terminated array of 'strings':
     * - each string is the path to a node beyond gbd;
     * - every string exists only once
     * - the first character of a string is the type of the entry
     * - the strings are sorted alphabetically !!!
     *
     * if datapath != 0, only keys with prefix datapath are scanned and
     * the prefix is removed from the resulting key_names.
     *
     * @@@ this function is incredibly slow when called from clients
     */
    struct DbScanner scanner;

    scanner.hash_table = GBS_create_hash(1024, GB_MIND_CASE);
    scanner.buffer     = (char *)malloc(GBT_SUM_LEN);
    strcpy(scanner.buffer,"");
    
    gbt_scan_db_rek(gbd, scanner.buffer, 0, &scanner);

    scanner.count = 0;
    GBS_hash_do_loop(scanner.hash_table, gbs_scan_db_count, &scanner);

    scanner.result = (char **)GB_calloc(sizeof(char *),scanner.count+1);
    /* null terminated result */

    scanner.count = 0;
    struct scan_db_insert insert = { &scanner, datapath, };
    GBS_hash_do_loop(scanner.hash_table, gbs_scan_db_insert, &insert);

    GBS_free_hash(scanner.hash_table);

    GB_sort((void **)scanner.result, 0, scanner.count, gbs_scan_db_compare, 0);

    free(scanner.buffer);
    return scanner.result;
}

/********************************************************************************************
                send a message to the db server to AWAR_ERROR_MESSAGES
********************************************************************************************/

static void new_gbt_message_created_cb(GBDATA *gb_pending_messages, int *cd, GB_CB_TYPE cbt) {
    static int avoid_deadlock = 0;

    GBUSE(cd);
    GBUSE(cbt);

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

void GBT_install_message_handler(GBDATA *gb_main) {
    GBDATA *gb_pending_messages;

    GB_push_transaction(gb_main);
    gb_pending_messages = GB_search(gb_main, AWAR_ERROR_CONTAINER, GB_CREATE_CONTAINER);
    GB_add_callback(gb_pending_messages, GB_CB_SON_CREATED, new_gbt_message_created_cb, 0);
    GB_pop_transaction(gb_main);

#if defined(DEBUG) && 0
    GBT_message(GB_get_root(gb_pending_messages), GBS_global_string("GBT_install_message_handler installed for gb_main=%p", gb_main));
#endif /* DEBUG */
}


void GBT_message(GBDATA *gb_main, const char *msg) {
    // When called in client(or server) this causes the DB server to show the message.
    // Message is showed via GB_warning (which uses aw_message in GUIs)
    //
    // Note: The message is not shown before the transaction ends.
    // If the transaction is aborted, the message is never shown!
    // 
    // see also : GB_warning

    GB_ERROR error = GB_push_transaction(gb_main);

    if (!error) {
        GBDATA *gb_pending_messages = GB_search(gb_main, AWAR_ERROR_CONTAINER, GB_CREATE_CONTAINER);
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

/* ---------------------------------------- 
 * conversion between
 *
 * - char ** (heap-allocated array of heap-allocated char*'s)
 * - one string containing several substrings separated by a separator
 *   (e.g. "name1,name2,name3")
 */

#if defined(DEVEL_RALF)
#warning search for code which is splitting strings and use GBT_split_string there
#warning rename to GBS_split_string and move to string functions
#endif /* DEVEL_RALF */

char **GBT_split_string(const char *namelist, char separator, int *countPtr) {
    // Split 'namelist' into an array of names at 'separator'.
    // Use GBT_free_names() to free it.
    // Sets 'countPtr' to the number of names found

    int         sepCount = 0;
    const char *sep      = namelist;
    while (sep) {
        sep = strchr(sep, separator);
        if (sep) {
            ++sep;
            ++sepCount;
        }
    }

    char **result = malloc((sepCount+2)*sizeof(*result)); // 3 separators -> 4 names (plus terminal NULL)
    int    count  = 0;

    for (; count < sepCount; ++count) {
        sep = strchr(namelist, separator);
        gb_assert(sep);
        
        result[count] = GB_strpartdup(namelist, sep-1);
        namelist      = sep+1;
    }

    result[count++] = strdup(namelist);
    result[count]   = NULL;

    if (countPtr) *countPtr = count;

    return result;
}

char *GBT_join_names(const char *const *names, char separator) {
    struct GBS_strstruct *out = GBS_stropen(1000);

    if (names[0]) {
        GBS_strcat(out, names[0]);
        gb_assert(strchr(names[0], separator) == 0); // otherwise you'll never be able to GBT_split_string
        int n;
        for (n = 1; names[n]; ++n) {
            GBS_chrcat(out, separator);
            GBS_strcat(out, names[n]);
            gb_assert(strchr(names[n], separator) == 0); // otherwise you'll never be able to GBT_split_string
        }
    }

    return GBS_strclose(out);
}

void GBT_free_names(char **names) {
    char **pn;
    for (pn = names; *pn;pn++) free(*pn);
    free((char *)names);
}

/* ---------------------------------------- */
/* read value from database field
 * returns 0 in case of error (use GB_await_error())
 * or when field does not exist
 *
 * otherwise GBT_read_string returns a heap copy
 * other functions return a pointer to a temporary variable (invalidated by next call)
 */

char *GBT_read_string(GBDATA *gb_container, const char *fieldpath){
    GBDATA *gbd;
    char   *result = NULL;
    
    GB_push_transaction(gb_container);
    gbd = GB_search(gb_container,fieldpath,GB_FIND);
    if (gbd) result = GB_read_string(gbd);
    GB_pop_transaction(gb_container);
    return result;
}

char *GBT_read_as_string(GBDATA *gb_container, const char *fieldpath){
    GBDATA *gbd;
    char   *result = NULL;
    
    GB_push_transaction(gb_container);
    gbd = GB_search(gb_container,fieldpath,GB_FIND);
    if (gbd) result = GB_read_as_string(gbd);
    GB_pop_transaction(gb_container);
    return result;
}

const char *GBT_read_char_pntr(GBDATA *gb_container, const char *fieldpath){
    GBDATA     *gbd;
    const char *result = NULL;

    GB_push_transaction(gb_container);
    gbd = GB_search(gb_container,fieldpath,GB_FIND);
    if (gbd) result = GB_read_char_pntr(gbd);
    GB_pop_transaction(gb_container);
    return result;
}

NOT4PERL long *GBT_read_int(GBDATA *gb_container, const char *fieldpath) {
    GBDATA *gbd;
    long   *result = NULL;
    
    GB_push_transaction(gb_container);
    gbd = GB_search(gb_container,fieldpath,GB_FIND);
    if (gbd) {
        static long result_var;
        result_var = GB_read_int(gbd);
        result     = &result_var;
    }
    GB_pop_transaction(gb_container);
    return result;
}

NOT4PERL double *GBT_read_float(GBDATA *gb_container, const char *fieldpath) {
    GBDATA *gbd;
    double *result = NULL;
    
    GB_push_transaction(gb_container);
    gbd = GB_search(gb_container,fieldpath,GB_FIND);
    if (gbd) {
        static double result_var;
        result_var = GB_read_float(gbd);
        result     = &result_var;
    }
    GB_pop_transaction(gb_container);
    return result;
}

/* -------------------------------------------------------------------------------------- */
/* read value from database field or create field with default_value if missing
 * (same usage as GBT_read_XXX above)
 */

char *GBT_readOrCreate_string(GBDATA *gb_container, const char *fieldpath, const char *default_value) {
    GBDATA *gb_string;
    char   *result = NULL;

    GB_push_transaction(gb_container);
    gb_string             = GB_searchOrCreate_string(gb_container, fieldpath, default_value);
    if (gb_string) result = GB_read_string(gb_string);
    GB_pop_transaction(gb_container);
    return result;
}

const char *GBT_readOrCreate_char_pntr(GBDATA *gb_container, const char *fieldpath, const char *default_value) {
    GBDATA     *gb_string;
    const char *result = NULL;

    GB_push_transaction(gb_container);
    gb_string             = GB_searchOrCreate_string(gb_container, fieldpath, default_value);
    if (gb_string) result = GB_read_char_pntr(gb_string);
    GB_pop_transaction(gb_container);
    return result;
}

NOT4PERL long *GBT_readOrCreate_int(GBDATA *gb_container, const char *fieldpath, long default_value) {
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

NOT4PERL double *GBT_readOrCreate_float(GBDATA *gb_container, const char *fieldpath, double default_value) {
    GBDATA *gb_float;
    double *result = NULL;

    GB_push_transaction(gb_container);
    gb_float = GB_searchOrCreate_float(gb_container, fieldpath, default_value);
    if (gb_float) {
        static double result_var;
        result_var = GB_read_float(gb_float);
        result     = &result_var;
    }
    GB_pop_transaction(gb_container);
    return result;
}

/* ------------------------------------------------------------------- */
/*      overwrite existing or create new database field                */
/*      (field must not exist twice or more - it has to be unique!!)   */

GB_ERROR GBT_write_string(GBDATA *gb_container, const char *fieldpath, const char *content) {
    GB_ERROR  error = GB_push_transaction(gb_container);
    GBDATA   *gbd   = GB_search(gb_container, fieldpath, GB_STRING);
    if (!gbd) error = GB_await_error();
    else {
        error = GB_write_string(gbd, content);
        gb_assert(GB_nextEntry(gbd) == 0); // only one entry should exist (sure you want to use this function?)
    }
    return GB_end_transaction(gb_container, error);
}

GB_ERROR GBT_write_int(GBDATA *gb_container, const char *fieldpath, long content) {
    GB_ERROR  error = GB_push_transaction(gb_container);
    GBDATA   *gbd   = GB_search(gb_container, fieldpath, GB_INT);
    if (!gbd) error = GB_await_error();
    else {
        error = GB_write_int(gbd, content);
        gb_assert(GB_nextEntry(gbd) == 0); // only one entry should exist (sure you want to use this function?)
    }
    return GB_end_transaction(gb_container, error);
}

GB_ERROR GBT_write_byte(GBDATA *gb_container, const char *fieldpath, unsigned char content) {
    GB_ERROR  error = GB_push_transaction(gb_container);
    GBDATA   *gbd   = GB_search(gb_container, fieldpath, GB_BYTE);
    if (!gbd) error = GB_await_error();
    else {
        error = GB_write_byte(gbd, content);
        gb_assert(GB_nextEntry(gbd) == 0); // only one entry should exist (sure you want to use this function?)
    }
    return GB_end_transaction(gb_container, error);
}


GB_ERROR GBT_write_float(GBDATA *gb_container, const char *fieldpath, double content) {
    GB_ERROR  error = GB_push_transaction(gb_container);
    GBDATA   *gbd   = GB_search(gb_container, fieldpath, GB_FLOAT);
    if (!gbd) error = GB_await_error();
    else {
        error = GB_write_float(gbd,content);
        gb_assert(GB_nextEntry(gbd) == 0); // only one entry should exist (sure you want to use this function?)
    }
    return GB_end_transaction(gb_container, error);
}


GBDATA *GB_test_link_follower(GBDATA *gb_main,GBDATA *gb_link,const char *link){
    GBDATA *linktarget = GB_search(gb_main,"tmp/link/string",GB_STRING);
    GBUSE(gb_link);
    GB_write_string(linktarget,GBS_global_string("Link is '%s'",link));
    return GB_get_father(linktarget);
}

/********************************************************************************************
                    SAVE & LOAD
********************************************************************************************/

/** Open a database, create an index for species and extended names,
    disable saving the database in the PT_SERVER directory */

GBDATA *GBT_open(const char *path,const char *opent,const char *disabled_path)
{
    GBDATA *gbd = GB_open(path,opent);
    GBDATA *species_data;
    GBDATA *extended_data;
    GBDATA *gb_tmp;
    long    hash_size;

    if (!gbd) return gbd;
    if (!disabled_path) disabled_path = "$(ARBHOME)/lib/pts/*";
    GB_disable_path(gbd,disabled_path);
    GB_begin_transaction(gbd);

    if (!strchr(path,':')){
        species_data = GB_search(gbd, "species_data", GB_FIND);
        if (species_data){
            hash_size = GB_number_of_subentries(species_data);
            if (hash_size < GBT_SPECIES_INDEX_SIZE) hash_size = GBT_SPECIES_INDEX_SIZE;
            GB_create_index(species_data,"name",GB_IGNORE_CASE,hash_size);

            extended_data = GBT_get_SAI_data(gbd);
            hash_size = GB_number_of_subentries(extended_data);
            if (hash_size < GBT_SAI_INDEX_SIZE) hash_size = GBT_SAI_INDEX_SIZE;
            GB_create_index(extended_data,"name",GB_IGNORE_CASE,hash_size);
        }
    }
    gb_tmp = GB_search(gbd,"tmp",GB_CREATE_CONTAINER);
    GB_set_temporary(gb_tmp);
    {               /* install link followers */
        GB_MAIN_TYPE *Main = GB_MAIN(gbd);
        Main->table_hash = GBS_create_hash(256, GB_MIND_CASE);
        GB_install_link_follower(gbd,"REF",GB_test_link_follower);
    }
    GBT_install_table_link_follower(gbd);
    GB_commit_transaction(gbd);
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
 * - BIO::remote_read_awar      (use of GBT_remote_read_awar - seems unused)
 * - BIO::remote_touch_awar     (use of GBT_remote_touch_awar - seems unused)
 */


#define AWAR_REMOTE_BASE_TPL            "tmp/remote/%s/"
#define MAX_REMOTE_APPLICATION_NAME_LEN 30
#define MAX_REMOTE_AWAR_STRING_LEN      (11+MAX_REMOTE_APPLICATION_NAME_LEN+1+6+1)

struct gbt_remote_awars {
    char awar_action[MAX_REMOTE_AWAR_STRING_LEN];
    char awar_result[MAX_REMOTE_AWAR_STRING_LEN];
    char awar_awar[MAX_REMOTE_AWAR_STRING_LEN];
    char awar_value[MAX_REMOTE_AWAR_STRING_LEN];
};

static void gbt_build_remote_awars(struct gbt_remote_awars *awars, const char *application) {
    int length;

    gb_assert(strlen(application) <= MAX_REMOTE_APPLICATION_NAME_LEN);

    length = sprintf(awars->awar_action, AWAR_REMOTE_BASE_TPL, application);
    gb_assert(length < (MAX_REMOTE_AWAR_STRING_LEN-6)); // Note :  6 is length of longest name appended below !

    strcpy(awars->awar_result, awars->awar_action);
    strcpy(awars->awar_awar, awars->awar_action);
    strcpy(awars->awar_value, awars->awar_action);

    strcpy(awars->awar_action+length, "action");
    strcpy(awars->awar_result+length, "result");
    strcpy(awars->awar_awar+length,   "awar");
    strcpy(awars->awar_value+length,  "value");
}

static GBDATA *gbt_remote_search_awar(GBDATA *gb_main, const char *awar_name) {
    GBDATA *gb_action;
    while (1) {
        GB_begin_transaction(gb_main);
        gb_action = GB_search(gb_main, awar_name, GB_FIND);
        GB_commit_transaction(gb_main);
        if (gb_action) break;
        GB_usleep(2000);
    }
    return gb_action;
}

static GB_ERROR gbt_wait_for_remote_action(GBDATA *gb_main, GBDATA *gb_action, const char *awar_read) {
    GB_ERROR error = 0;

    /* wait to end of action */

    while (!error) {
        GB_usleep(2000);
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

    return error; // may be error or result
}

GB_ERROR GBT_remote_action(GBDATA *gb_main, const char *application, const char *action_name){
    struct gbt_remote_awars  awars;
    GBDATA                  *gb_action;
    GB_ERROR                 error = NULL;

    gbt_build_remote_awars(&awars, application);
    gb_action = gbt_remote_search_awar(gb_main, awars.awar_action);

    error             = GB_begin_transaction(gb_main);
    if (!error) error = GB_write_string(gb_action, action_name); /* write command */
    error             = GB_end_transaction(gb_main, error);

    if (!error) error = gbt_wait_for_remote_action(gb_main, gb_action, awars.awar_result);
    return error;
}

GB_ERROR GBT_remote_awar(GBDATA *gb_main, const char *application, const char *awar_name, const char *value) {
    struct gbt_remote_awars  awars;
    GBDATA                  *gb_awar;
    GB_ERROR                 error = NULL;

    gbt_build_remote_awars(&awars, application);
    gb_awar = gbt_remote_search_awar(gb_main, awars.awar_awar);

    error = GB_begin_transaction(gb_main);
    if (!error) error = GB_write_string(gb_awar, awar_name);
    if (!error) error = GBT_write_string(gb_main, awars.awar_value, value);
    error = GB_end_transaction(gb_main, error);

    if (!error) error = gbt_wait_for_remote_action(gb_main, gb_awar, awars.awar_result);
    
    return error;
}

GB_ERROR GBT_remote_read_awar(GBDATA *gb_main, const char *application, const char *awar_name) {
    struct gbt_remote_awars  awars;
    GBDATA                  *gb_awar;
    GB_ERROR                 error = NULL;

    gbt_build_remote_awars(&awars, application);
    gb_awar = gbt_remote_search_awar(gb_main, awars.awar_awar);

    error             = GB_begin_transaction(gb_main);
    if (!error) error = GB_write_string(gb_awar, awar_name);
    if (!error) error = GBT_write_string(gb_main, awars.awar_action, "AWAR_REMOTE_READ");
    error             = GB_end_transaction(gb_main, error);

    if (!error) error = gbt_wait_for_remote_action(gb_main, gb_awar, awars.awar_value);
    return error;
}

const char *GBT_remote_touch_awar(GBDATA *gb_main, const char *application, const char *awar_name) {
    struct gbt_remote_awars  awars;
    GBDATA                  *gb_awar;
    GB_ERROR                 error = NULL;

    gbt_build_remote_awars(&awars, application);
    gb_awar = gbt_remote_search_awar(gb_main, awars.awar_awar);

    error             = GB_begin_transaction(gb_main);
    if (!error) error = GB_write_string(gb_awar, awar_name);
    if (!error) error = GBT_write_string(gb_main, awars.awar_action, "AWAR_REMOTE_TOUCH");
    error             = GB_end_transaction(gb_main, error);

    if (!error) error = gbt_wait_for_remote_action(gb_main, gb_awar, awars.awar_result);
    return error;
}


/* --------------------------- */
/*      self-notification      */
/* --------------------------- */
/* provides a mechanism to notify ARB after some external tool finishes */

#define ARB_NOTIFICATIONS "tmp/notify"

/* DB structure for notifications : 
 *
 * ARB_NOTIFICATIONS/counter        GB_INT      counts used ids
 * ARB_NOTIFICATIONS/notify/id      GB_INT      id of notification
 * ARB_NOTIFICATIONS/notify/message GB_STRING   message of notification (set by callback)
 */

typedef void (*notify_cb_type)(const char *message, void *client_data);

struct NCB {
    notify_cb_type  cb;
    void           *client_data;
};

static void notify_cb(GBDATA *gb_message, int *cb_info, GB_CB_TYPE cb_type) {
    GB_remove_callback(gb_message, GB_CB_CHANGED|GB_CB_DELETE, notify_cb, cb_info); // @@@ cbproblematic

    int         cb_done = 0;
    struct NCB *pending = (struct NCB*)cb_info;

    if (cb_type == GB_CB_CHANGED) {
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
    else { /* called from GB_remove_last_notification */
        gb_assert(cb_type == GB_CB_DELETE);
    }

    free(pending);
}

static int allocateNotificationID(GBDATA *gb_main, int *cb_info) {
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
                int newid = GB_read_int(gb_counter) + 1; /* increment counter */
                error     = GB_write_int(gb_counter, newid);

                if (!error) {
                    /* change transaction (to never use id twice!) */
                    error             = GB_pop_transaction(gb_main);
                    if (!error) error = GB_push_transaction(gb_main);

                    if (!error) {
                        GBDATA *gb_notification = GB_create_container(gb_notify, "notify");
                        
                        if (gb_notification) {
                            error = GBT_write_int(gb_notification, "id", newid);
                            if (!error) {
                                GBDATA *gb_message = GB_searchOrCreate_string(gb_notification, "message", "");
                                
                                if (gb_message) {
                                    error = GB_add_callback(gb_message, GB_CB_CHANGED|GB_CB_DELETE, notify_cb, cb_info);
                                    if (!error) {
                                        id = newid; /* success */
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

    int         id;
    char       *arb_notify_call = 0;
    struct NCB *pending         = malloc(sizeof(*pending));

    pending->cb          = cb;
    pending->client_data = client_data;

    id = allocateNotificationID(gb_main, (int*)pending);
    if (id) {
        arb_notify_call = GBS_global_string_copy("arb_notify %i \"%s\"", id, message);
    }
    else {
        free(pending);
    }

    return arb_notify_call;
}

GB_ERROR GB_remove_last_notification(GBDATA *gb_main) {
    /* aborts the last notification */
    GB_ERROR error = GB_push_transaction(gb_main);

    if (!error) {
        GBDATA *gb_notify = GB_search(gb_main, ARB_NOTIFICATIONS, GB_CREATE_CONTAINER);
        if (gb_notify) {
            GBDATA *gb_counter = GB_entry(gb_notify, "counter");
            if (gb_counter) {
                int     id    = GB_read_int(gb_counter);
                GBDATA *gb_id = GB_find_int(gb_notify, "id", id, down_2_level);

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
                        error = GB_delete(gb_message); /* calls notify_cb */
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
        GBDATA *gb_id = GB_find_int(gb_notify, "id", id, down_2_level);

        if (!gb_id) {
            error = GBS_global_string("No notification for ID %i", id);
        }
        else {
            GBDATA *gb_message = GB_brother(gb_id, "message");

            if (!gb_message) {
                error = "Missing 'message' entry";
            }
            else {
                /* callback the instantiating DB client */
                error = GB_write_string(gb_message, message);
            }
        }
    }

    return error;
}
