// =============================================================== //
//                                                                 //
//   File      : adChangeKey.cxx                                   //
//   Purpose   : Changekey management                              //
//                                                                 //
//   Coded by Elmar Pruesse and Ralf Westram in May 2009           //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <arbdbt.h>
#include "gb_local.h"

GBDATA *GBT_get_changekey(GBDATA *gb_main, const char *key, const char *change_key_path) {
    // get the container of an item key description
#if defined(WARN_TODO)
#warning check if search for CHANGEKEY_NAME should be case-sensitive!
#endif
    GBDATA *gb_key      = 0;
    GBDATA *gb_key_data = GB_search(gb_main, change_key_path,
                                    GB_CREATE_CONTAINER);

    if (gb_key_data) {
        GBDATA *gb_key_name = GB_find_string(gb_key_data, CHANGEKEY_NAME, key, GB_IGNORE_CASE, SEARCH_GRANDCHILD);
        if (gb_key_name) {
            gb_key = GB_get_father(gb_key_name);
        }
    }
    return gb_key;
}

GB_TYPES GBT_get_type_of_changekey(GBDATA *gb_main, const char *field_name, const char *change_key_path) {
    // get the type of an item key
    GB_TYPES  type = GB_NONE;
    GBDATA   *gbd  = GBT_get_changekey(gb_main, field_name, change_key_path);

    if (gbd) {
        long *typePtr     = GBT_read_int(gbd, CHANGEKEY_TYPE);
        if (typePtr) {
            type = (GB_TYPES)*typePtr;
        }
    }

    return type;
}

static GB_ERROR gbt_set_type_of_changekey(GBDATA *gb_main, const char *field_name, GB_TYPES type, const char *change_key_path) {
    GB_ERROR  error = NULL;
    GBDATA   *gbd   = GBT_get_changekey(gb_main, field_name, change_key_path);

    if (!gbd) {
        error = GBS_global_string("Can't set type of nonexistent changekey \"%s\"", field_name);
    }
    else {
        error = GBT_write_int(gbd, CHANGEKEY_TYPE, type);
    }
    return error;
}

GBDATA *GBT_searchOrCreate_itemfield_according_to_changekey(GBDATA *gb_item, const char *field_name, const char *change_key_path) {
    /*! search or create an item entry.
     * If the entry exists, the result is identical to GB_search(gb_item, field_name, GB_FIND).
     * If the entry does not exist, an entry with the type stored in the changekey-table will be created.
     * @return created itemfield or NULL in case of error (which is exported in that case)
     */

    gb_assert(!GB_have_error());
    GBDATA *gb_entry = GB_search(gb_item, field_name, GB_FIND);
    if (!gb_entry) {
        GB_clear_error();

        GB_TYPES type = GBT_get_type_of_changekey(GB_get_root(gb_item), field_name, change_key_path);
        if (type == GB_NONE) {
            GB_export_errorf("Unknown changekey '%s' (cannot create)", field_name);
        }
        else {
            gb_entry = GB_search(gb_item, field_name, type);
        }
    }
    gb_assert(gb_entry || GB_have_error());
    return gb_entry;
}


GB_ERROR GBT_add_new_changekey_to_keypath(GBDATA *gb_main, const char *name, int type, const char *keypath) {
    GB_ERROR    error  = NULL;
    GBDATA     *gb_key = GBT_get_changekey(gb_main, name, keypath);
    const char *c      = GB_first_non_key_char(name);

    if (c) {
        char *new_name = strdup(name);

        *(char*)GB_first_non_key_char(new_name) = 0;

        if      (*c == '/') error = GBT_add_new_changekey(gb_main, new_name, GB_DB);
        else if (*c == '-') error = GBT_add_new_changekey(gb_main, new_name, GB_LINK);
        else               error = GBS_global_string("Cannot add '%s' to your key list (illegal character '%c')", name, *c);

        free(new_name);
    }

    if (!error) {
        if (!gb_key) {          // create new key
            GBDATA *gb_key_data = GB_search(gb_main, keypath, GB_CREATE_CONTAINER);
            gb_key              = gb_key_data ? GB_create_container(gb_key_data, CHANGEKEY) : 0;

            if (!gb_key) error = GB_await_error();
            else {
                error             = GBT_write_string(gb_key, CHANGEKEY_NAME, name);
                if (!error) error = GBT_write_int(gb_key, CHANGEKEY_TYPE, type);
            }
        }
        else {                  // check type of existing key
            long *elem_type = GBT_read_int(gb_key, CHANGEKEY_TYPE);

            if (!elem_type)              error = GB_await_error();
            else if (*elem_type != type) error = GBS_global_string("Key '%s' exists, but has different type", name);
        }
    }

    gb_assert(gb_key || error);

    return error;
}

GB_ERROR GBT_add_new_changekey(GBDATA *gb_main, const char *name, int type) {
    return GBT_add_new_changekey_to_keypath(gb_main, name, type, CHANGE_KEY_PATH);
}

GB_ERROR GBT_add_new_gene_changekey(GBDATA *gb_main, const char *name, int type) {
    return GBT_add_new_changekey_to_keypath(gb_main, name, type, CHANGE_KEY_PATH_GENES);
}

GB_ERROR GBT_add_new_experiment_changekey(GBDATA *gb_main, const char *name, int type) {
    return GBT_add_new_changekey_to_keypath(gb_main, name, type, CHANGE_KEY_PATH_EXPERIMENTS);
}

static GB_ERROR write_as_int(GBDATA *gbfield, const char *data, bool trimmed, size_t *rounded) {
    char          *end   = 0;
    unsigned long  i     = strtoul(data, &end, 10);
    GB_ERROR       error = NULL;

    if (end == data || end[0] != 0) {
        if (trimmed) {
            // fallback: convert to double and round

            double d = strtod(data, &end);
            if (end == data || end[0] != 0) {
                error = GBS_global_string("cannot convert '%s' to rounded numeric value", data);
            }
            else {
                (*rounded)++;
                i                = d>0 ? (int)(d+0.5) : (int)(d-0.5);
                error            = GB_write_int(gbfield, i);
                if (error) error = GBS_global_string("write error (%s)", error);
            }
        }
        else {
            char *trimmed_data = GBS_trim(data);
            error              = write_as_int(gbfield, trimmed_data, true, rounded);
            free(trimmed_data);
        }
    }
    else {
        error = GB_write_int(gbfield, i);
        if (error) error = GBS_global_string("write error (%s)", error);
    }

    return error;
}

static GB_ERROR write_as_float(GBDATA *gbfield, const char *data, bool trimmed) {
    char     *end   = 0;
    float     f     = strtof(data, &end);
    GB_ERROR  error = NULL;

    if (end == data || end[0] != 0) {
        if (trimmed) {
            error = GBS_global_string("cannot convert '%s' to numeric value", data);
        }
        else {
            char *trimmed_data = GBS_trim(data);
            error              = write_as_float(gbfield, trimmed_data, true);
            free(trimmed_data);
        }
    }
    else {
        error = GB_write_float(gbfield, f);
        if (error) error = GBS_global_string("write error (%s)", error);
    }

    return error;
}


GB_ERROR GBT_convert_changekey(GBDATA *gb_main, const char *name, GB_TYPES target_type) {
    GB_ERROR error        = GB_push_transaction(gb_main);
    bool     need_convert = true;

    if (!error) {
        GBDATA *gbkey = GBT_get_changekey(gb_main, name, CHANGE_KEY_PATH);
        if (gbkey) {
            GB_TYPES source_type = (GB_TYPES)*GBT_read_int(gbkey, CHANGEKEY_TYPE);
            if (source_type == target_type) need_convert = false;
        }
        else {
            error = GBS_global_string("Unknown changekey '%s'", name);
        }
    }

    if (!error && need_convert) {
        GBDATA *gbspec  = GBT_first_species(gb_main);
        size_t  rounded = 0;

        for (; gbspec; gbspec = GBT_next_species(gbspec)) {
            GBDATA *gbfield = GB_entry(gbspec, name);

            // If entry does not exist, no need to convert (sparse population is valid => 'NULL' value)
            if (gbfield) {
                char *data = GB_read_as_string(gbfield);
                if (!data) {
                    error = GBS_global_string("read error (%s)", GB_await_error());
                }
                else {
                    error = GB_delete(gbfield);
                    if (!error) {
                        gbfield = GB_create(gbspec, name, target_type);
                        if (!gbfield) {
                            error = GBS_global_string("create error (%s)", GB_await_error());
                        }
                        else {
                            switch (target_type) {
                                case GB_INT:
                                    error = write_as_int(gbfield, data, false, &rounded);
                                    break;

                                case GB_FLOAT:
                                    error = write_as_float(gbfield, data, false);
                                    break;

                                case GB_STRING:
                                    error = GB_write_string(gbfield, data);
                                    if (error) error = GBS_global_string("write error (%s)", error);
                                    break;

                                default:
                                    error = "Conversion is not possible";
                                    break;
                            }
                        }
                    }
                    free(data);
                }
            }
            if (error) break;
        }

        if (error && gbspec) {
            const char *spname = GBT_read_name(gbspec);
            error              = GBS_global_string("%s for species '%s'", error, spname);
        }

        if (!error) error = gbt_set_type_of_changekey(gb_main, name, target_type, CHANGE_KEY_PATH);
        if (!error && rounded>0) {
            GB_warningf("%zi values were rounded (loss of precision)", rounded);
        }
    }

    if (error) error  = GBS_global_string("GBT_convert: %s", error);

    return GB_end_transaction(gb_main, error);
}

