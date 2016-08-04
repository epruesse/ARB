// =============================================================== //
//                                                                 //
//   File      : adsystem.cxx                                      //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <sys/types.h>
#include <netinet/in.h>

#include <arbdbt.h>
#include <ad_cb.h>

#include "gb_key.h"
#include "gb_dict.h"

static GB_CSTR gb_read_dict_data(GBDATA *gb_dict, long *size) {
    GB_CSTR data = 0;
    if (gb_dict->flags.compressed_data) {
        GB_internal_error("Dictionary is compressed");
        data = GB_read_bytes(gb_dict);
    }
    else {
        data = GB_read_bytes_pntr(gb_dict);
    }
    *size = GB_read_bytes_count(gb_dict);
    return data;
}

GB_ERROR gb_load_dictionary_data(GBDATA *gb_main, const char *key, char **dict_data, long *size) {
    /* returns dictionary data (like saved in DB)
     * in a block allocated by gbm_get_mem(.., GBM_DICT_INDEX)
     */
    GB_MAIN_TYPE *Main  = GB_MAIN(gb_main);
    GB_ERROR      error = 0;

    *dict_data = 0;
    *size      = -1;
    gb_main    = Main->gb_main();

    if (key[0] == '@') {
        error = GB_export_error("No dictionaries for system fields");
    }
    else {
        GBDATA *gb_key_data = Main->gb_key_data;
        GBDATA *gb_name;

        GB_push_my_security(gb_main);
        gb_name = GB_find_string(gb_key_data, "@name", key, GB_MIND_CASE, SEARCH_GRANDCHILD);

        if (gb_name) {
            GBDATA *gb_key  = GB_get_father(gb_name);
            GBDATA *gb_dict = GB_entry(gb_key, "@dictionary");
            if (gb_dict) {
                const char *data = gb_read_dict_data(gb_dict, size);
                char       *copy = (char*)gbm_get_mem(*size, GBM_DICT_INDEX);
                memcpy(copy, data, *size);
                *dict_data       = copy;
            }
        }
        GB_pop_my_security(gb_main);
    }
    return error;
}

static GB_DICTIONARY *gb_create_dict(GBDATA *gb_dict) {
    GB_DICTIONARY *dict = (GB_DICTIONARY *)ARB_calloc(sizeof(GB_DICTIONARY), 1);

    long size;
    const char *data = gb_read_dict_data(gb_dict, &size);
    GB_write_security_write(gb_dict, 7);

    GB_NINT *idata = (GB_NINT *)data;
    dict->words = ntohl(*idata++);
    dict->textlen = (int)(size - sizeof(GB_NINT)*(1+dict->words*2));

    dict->offsets = idata;
    dict->resort =  idata+dict->words;
    dict->text = (unsigned char*)(idata+2*dict->words);

    return dict;
}

static void delete_gb_dictionary(GB_DICTIONARY *dict) {
    free(dict);
}

static void gb_system_key_changed_cb(GBDATA *gbd, GBQUARK q, GB_CB_TYPE type) {
    if (type == GB_CB_DELETE) {
        GB_MAIN_TYPE *Main = gb_get_main_during_cb();

        delete_gb_dictionary(Main->keys[q].dictionary);
        Main->keys[q].dictionary = 0;
        Main->keys[q].gb_key     = 0;
    }
    else {
        gb_load_single_key_data(gbd, q);
    }
}

static void gb_system_master_changed_cb(GBDATA *gbd, GBQUARK q, GB_CB_TYPE type) {
    if (type == GB_CB_DELETE) {
        GB_MAIN_TYPE *Main = gb_get_main_during_cb();
        Main->keys[q].gb_master_ali = 0;
    }
    else {
        gb_load_single_key_data(gbd, q);
    }
}

void gb_load_single_key_data(GBDATA *gb_main, GBQUARK q) {
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    gb_Key       *ks   = &Main->keys[q];
    const char   *key  = ks->key;

    if (!Main->gb_key_data) {
        ks->compression_mask = -1;
        return;
    }

    gb_main = Main->gb_main();
    if (key[0] == '@') {
        ks->compression_mask = 0;
        ks->dictionary       = 0;
        ks->gb_key_disabled  = 1;
        ks->gb_master_ali    = 0;
    }
    else {
        GBCONTAINER *gb_key_data = Main->gb_key_data;
        GB_push_my_security(gb_main);

        GBDATA      *gb_name = GB_find_string(gb_key_data, "@name", key, GB_MIND_CASE, SEARCH_GRANDCHILD);
        GBCONTAINER *gb_key;
        if (gb_name) {
            gb_key = GB_FATHER(gb_name);
        }
        else {
            gb_key  = gb_create_container(gb_key_data, "@key");
            gb_name = gb_create(gb_key, "@name", GB_STRING);
            GB_write_string(gb_name, key);
        }

        GB_ensure_callback(gb_key, GB_CB_CHANGED_OR_DELETED, makeDatabaseCallback(gb_system_key_changed_cb, q));

        if (ks->dictionary) {
            delete_gb_dictionary(ks->dictionary);
            ks->dictionary = 0;
        }

        ks->compression_mask = *GBT_readOrCreate_int(gb_key, "compression_mask", -1);
        GBDATA *gb_dict      = GB_entry(gb_key, "@dictionary");
        ks->dictionary       = gb_dict ? gb_create_dict(gb_dict) : 0;
        ks->gb_key           = gb_key;

        {
            char buffer[256];
            sprintf(buffer, "%s/@master_data/@%s", GB_SYSTEM_FOLDER, key);

            ks->gb_master_ali = GBDATA::as_container(GB_search(gb_main, buffer, GB_FIND));
            if (ks->gb_master_ali) {
                GB_ensure_callback(ks->gb_master_ali, GB_CB_CHANGED_OR_DELETED, makeDatabaseCallback(gb_system_master_changed_cb, q));
            }
        }
        GB_pop_my_security(gb_main);
    }
}

GB_ERROR gb_save_dictionary_data(GBDATA *gb_main, const char *key, const char *dict, int size) {
    // if 'dict' is NULL, an existing directory gets deleted
    GB_MAIN_TYPE *Main  = GB_MAIN(gb_main);
    GB_ERROR      error = 0;
    gb_main             = Main->gb_main();
    if (key[0] == '@') {
        error = GB_export_error("No dictionaries for system fields");
    }
    else {
        GBCONTAINER *gb_key_data = Main->gb_key_data;
        GB_push_my_security(gb_main);

        GBDATA      *gb_name = GB_find_string(gb_key_data, "@name", key, GB_MIND_CASE, SEARCH_GRANDCHILD);
        GBCONTAINER *gb_key;
        if (gb_name) {
            gb_key = GB_FATHER(gb_name);
        }
        else {
            gb_key = gb_create_container(gb_key_data, "@key");
            gb_name = gb_create(gb_key, "@name", GB_STRING);
            GB_write_string(gb_name, key);
        }
        GBDATA *gb_dict;
        if (dict) {
            gb_dict = gb_search(gb_key, "@dictionary", GB_BYTES, 1);
            error   = GB_write_bytes(gb_dict, dict, size);
        }
        else {
            gb_dict = GB_entry(gb_key, "@dictionary");
            if (gb_dict) {
                GB_delete(gb_dict); // delete existing dictionary
            }
        }
        GB_pop_my_security(gb_main);
    }
    if (!error) {
        GBQUARK q = gb_find_or_create_quark(Main, key);
        gb_load_single_key_data(gb_main, q);
    }
    return error;
}

GB_ERROR gb_load_key_data_and_dictionaries(GB_MAIN_TYPE *Main) { // goes to header: __ATTR__USERESULT
    GBCONTAINER *gb_main = Main->root_container;
    GB_ERROR     error   = NULL;

    GBCONTAINER *gb_key_data = gb_search(gb_main, GB_SYSTEM_FOLDER "/" GB_SYSTEM_KEY_DATA, GB_CREATE_CONTAINER, 1)->as_container();
    if (!gb_key_data) {
        error = GB_await_error();
    }
    else {
        Main->gb_key_data = gb_key_data;
        if (Main->is_server()) { // do not create anything at the client side
            GB_push_my_security(gb_main);

            // search unused keys and delete them
            for (GBDATA *gb_key = GB_entry(gb_key_data, "@key"); gb_key && !error;) {
                GBDATA *gb_next_key = GB_nextEntry(gb_key);
                GBDATA *gb_name     = GB_entry(gb_key, "@name");
                if (!gb_name) error = GB_await_error();
                else {
                    const char *name = GB_read_char_pntr(gb_name);
                    if (!name) error = GB_await_error();
                    else {
                        if (!name[0]) {
                            error = GB_delete(gb_key);  // delete invalid empty key
                        }
                        else {
                            GBQUARK quark = gb_find_or_create_quark(Main, name);
                            if (quark<=0 || quark >= Main->sizeofkeys || !quark2key(Main, quark)) {
                                error = GB_delete(gb_key);  // delete unused key
                            }
                        }
                    }
                }
                gb_key = gb_next_key;
            }

            if (!error) error = GB_create_index(gb_key_data, "@name", GB_MIND_CASE, Main->sizeofkeys*2); // create key index
            if (!error) {
                ASSERT_RESULT_PREDICATE(isAbove<int>(0), gb_find_or_create_quark(Main, "@name"));
                ASSERT_RESULT_PREDICATE(isAbove<int>(0), gb_find_or_create_quark(Main, "@key"));
                ASSERT_RESULT_PREDICATE(isAbove<int>(0), gb_find_or_create_quark(Main, "@dictionary"));
                ASSERT_RESULT_PREDICATE(isAbove<int>(0), gb_find_or_create_quark(Main, "compression_mask"));

                for (int key=1; key<Main->sizeofkeys; key++) {
                    if (quark2key(Main, key)) {
                        gb_load_single_key_data(gb_main, key);
                    }
                }
            }
            GB_pop_my_security(gb_main);
        }
    }
    RETURN_ERROR(error);
}

/* gain access to allow repair of broken compression
 * (see NT_fix_dict_compress)
 */

struct DictData {
    char *data;
    long  size;
};

static void GB_free_dictionary(DictData *dd) {
    if (dd) {
        if (dd->data) gbm_free_mem(dd->data, dd->size, GBM_DICT_INDEX);
        free(dd);
    }
}

DictData *GB_get_dictionary(GBDATA *gb_main, const char *key) {
    /* return DictData or
     * NULL if no dictionary or error occurred
     */
    DictData *dd    = (DictData*)ARB_calloc(1, sizeof(*dd));
    GB_ERROR  error = gb_load_dictionary_data(gb_main, key, &dd->data, &dd->size);

    if (error || !dd->data) {
        GB_free_dictionary(dd);
        dd = NULL;
        if (error) GB_export_error(error);
    }

    return dd;
}

GB_ERROR GB_set_dictionary(GBDATA *gb_main, const char *key, const DictData *dd) {
    // if 'dd' == NULL -> delete dictionary
    GB_ERROR error;
    if (dd) {
        error = gb_save_dictionary_data(gb_main, key, dd->data, dd->size);
    }
    else {
        error = gb_save_dictionary_data(gb_main, key, NULL, 0);
    }
    return error;
}

