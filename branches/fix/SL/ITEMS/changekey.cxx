// ==================================================================== //
//                                                                      //
//   File      : changekey.cxx                                          //
//   Purpose   : changekey management                                   //
//                                                                      //
//                                                                      //
// Coded by Ralf Westram (coder@reallysoft.de) in May 2005              //
// Copyright Department of Microbiology (Technical University Munich)   //
//                                                                      //
// Visit our web site at: http://www.arb-home.de/                       //
//                                                                      //
// ==================================================================== //

#include "item_sel_list.h"

#include <arbdbt.h>
#include <arb_strarray.h>

static const char GENE_DATA_PATH[]       = "gene_data/gene/";
static const char EXPERIMENT_DATA_PATH[] = "experiment_data/experiment/";

#define GENE_DATA_PATH_LEN       (sizeof(GENE_DATA_PATH)-1)
#define EXPERIMENT_DATA_PATH_LEN (sizeof(EXPERIMENT_DATA_PATH)-1)

inline bool is_in_GENE_path(const char *fieldpath)          { return strncmp(fieldpath, GENE_DATA_PATH,         GENE_DATA_PATH_LEN) == 0; }
inline bool is_in_EXPERIMENT_path(const char *fieldpath)    { return strncmp(fieldpath, EXPERIMENT_DATA_PATH,   EXPERIMENT_DATA_PATH_LEN) == 0; }

inline bool is_in_reserved_path(const char *fieldpath) {
    return
        is_in_GENE_path(fieldpath) ||
        is_in_EXPERIMENT_path(fieldpath);
}

// --------------------------------------------------------------------------------


static void delete_unused_changekeys(GBDATA *gb_main, const CharPtrArray& names, const char *change_key_path) {
    // deletes all keys from 'change_key_path' which are not listed in 'names'
    GBDATA *gb_key_data = GB_search(gb_main, change_key_path, GB_CREATE_CONTAINER);
    GBDATA *gb_key      = gb_key_data ? GB_entry(gb_key_data, CHANGEKEY) : 0;

    while (gb_key) {
        bool        found    = false;
        int         key_type = *GBT_read_int(gb_key, CHANGEKEY_TYPE);
        const char *key_name = GBT_read_char_pntr(gb_key, CHANGEKEY_NAME);

        for (int i = 0; names[i]; ++i) {
            const char *name = names[i];
            if (strcmp(key_name, name+1) == 0) { // key with this name exists
                if (key_type == name[0]) {
                    found = true;
                }
                // otherwise key exists, but type mismatches = > delete this key
                break;
            }
        }

        it_assert(GB_has_key(gb_key, CHANGEKEY));
        GBDATA *gb_next_key = GB_nextEntry(gb_key);

        if (!found) {
            if (key_type == GB_DB) { // it's a container
                // test if any subkey is used
                int keylen = strlen(key_name);
                for (int i = 0; names[i]; ++i) {
                    const char *n = names[i]+1;

                    if (strncmp(key_name, n, keylen) == 0 && n[keylen] == '/') { // found a subkey -> do not delete
                        found = true;
                        break;
                    }
                }
            }

            if (!found) {       // key no longer exists = > delete from key list
                GB_delete(gb_key);
            }
        }

        gb_key = gb_next_key;
    }
}

static void show_all_changekeys(GBDATA *gb_main, const char *change_key_path) {
    GBDATA *gb_key_data = GB_search(gb_main, change_key_path, GB_CREATE_CONTAINER);
    for (GBDATA *gb_key = gb_key_data ? GB_entry(gb_key_data, CHANGEKEY) : 0;
         gb_key;
         gb_key = GB_nextEntry(gb_key))
    {
        GBDATA *gb_key_hidden = GB_entry(gb_key, CHANGEKEY_HIDDEN);
        if (gb_key_hidden) {
            if (GB_read_int(gb_key_hidden)) GB_write_int(gb_key_hidden, 0); // unhide
        }
    }
}

void species_field_selection_list_rescan(GBDATA *gb_main, long bitfilter, RescanMode mode) {
    GB_push_transaction(gb_main);
    GBDATA *gb_species_data = GBT_get_species_data(gb_main);

    StrArray names;
    GBT_scan_db(names, gb_species_data, 0);

    if (mode & RESCAN_REFRESH)  delete_unused_changekeys(gb_main, names, CHANGE_KEY_PATH);
    if (mode & RESCAN_SHOW_ALL) show_all_changekeys(gb_main, CHANGE_KEY_PATH);

    if (mode & RESCAN_REFRESH) {
        GBT_add_new_changekey(gb_main, "name", GB_STRING);
        GBT_add_new_changekey(gb_main, "acc", GB_STRING);
        GBT_add_new_changekey(gb_main, "full_name", GB_STRING);
        GBT_add_new_changekey(gb_main, "tmp", GB_STRING);

        for (int i = 0; names[i]; ++i) {
            const char *name = names[i];
            if ((1<<name[0]) & bitfilter) {         // look if already exists
                if (!is_in_reserved_path(name+1)) { // ignore gene, experiment, ... entries
                    GBT_add_new_changekey(gb_main, name+1, (int)(name[0]));
                }
            }
        }
    }

    GB_pop_transaction(gb_main);
}



void gene_field_selection_list_rescan(GBDATA *gb_main, long bitfilter, RescanMode mode) {
    GB_push_transaction(gb_main);
    GBDATA *gb_species_data = GBT_get_species_data(gb_main);

    StrArray names;
    GBT_scan_db(names, gb_species_data, GENE_DATA_PATH);

    if (mode & RESCAN_REFRESH)  delete_unused_changekeys(gb_main, names, CHANGE_KEY_PATH_GENES);
    if (mode & RESCAN_SHOW_ALL) show_all_changekeys(gb_main, CHANGE_KEY_PATH_GENES);

    if (mode & RESCAN_REFRESH) {
        GBT_add_new_gene_changekey(gb_main, "name", GB_STRING);

        GBT_add_new_gene_changekey(gb_main, "pos_start", GB_STRING);
        GBT_add_new_gene_changekey(gb_main, "pos_stop", GB_STRING);
        GBT_add_new_gene_changekey(gb_main, "pos_complement", GB_STRING);

        GBT_add_new_gene_changekey(gb_main, "pos_joined", GB_INT);
        GBT_add_new_gene_changekey(gb_main, "pos_certain", GB_STRING);

        for (int i = 0; names[i]; ++i) {
            const char *name = names[i];
            if ((1<<name[0]) & bitfilter) {            // look if already exists
                GBT_add_new_gene_changekey(gb_main, name+1, (int)(name[0]));
            }
        }
    }

    GB_pop_transaction(gb_main);
}

static void experiment_field_selection_list_rescan(GBDATA *gb_main, long bitfilter, RescanMode mode) {
    GB_push_transaction(gb_main);
    GBDATA *gb_species_data = GBT_get_species_data(gb_main);

    StrArray names;
    GBT_scan_db(names, gb_species_data, EXPERIMENT_DATA_PATH);

    if (mode & RESCAN_REFRESH)  delete_unused_changekeys(gb_main, names, CHANGE_KEY_PATH_EXPERIMENTS);
    if (mode & RESCAN_SHOW_ALL) show_all_changekeys(gb_main, CHANGE_KEY_PATH_EXPERIMENTS);

    if (mode & RESCAN_REFRESH) {
        GBT_add_new_experiment_changekey(gb_main, "name", GB_STRING);

        for (int i = 0; names[i]; ++i) {
            const char *name = names[i];
            if ((1<<name[0]) & bitfilter) {   // look if already exists
                if (is_in_EXPERIMENT_path(name+1)) {
                    GBT_add_new_experiment_changekey(gb_main, name+1+EXPERIMENT_DATA_PATH_LEN, (int)(name[0]));
                }
            }
        }
    }

    GB_pop_transaction(gb_main);
}

void species_field_selection_list_unhide_all_cb(AW_window *, GBDATA *gb_main, long bitfilter) { species_field_selection_list_rescan(gb_main, bitfilter, RESCAN_SHOW_ALL); }
void species_field_selection_list_update_cb    (AW_window *, GBDATA *gb_main, long bitfilter) { species_field_selection_list_rescan(gb_main, bitfilter, RESCAN_REFRESH);  }

void gene_field_selection_list_unhide_all_cb(AW_window *, GBDATA *gb_main, long bitfilter) { gene_field_selection_list_rescan(gb_main, bitfilter, RESCAN_SHOW_ALL); }
void gene_field_selection_list_update_cb    (AW_window *, GBDATA *gb_main, long bitfilter) { gene_field_selection_list_rescan(gb_main, bitfilter, RESCAN_REFRESH);  }

void experiment_field_selection_list_unhide_all_cb(AW_window *, GBDATA *gb_main, long bitfilter) { experiment_field_selection_list_rescan(gb_main, bitfilter, RESCAN_SHOW_ALL); }
void experiment_field_selection_list_update_cb    (AW_window *, GBDATA *gb_main, long bitfilter) { experiment_field_selection_list_rescan(gb_main, bitfilter, RESCAN_REFRESH);  }
