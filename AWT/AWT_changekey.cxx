// ==================================================================== //
//                                                                      //
//   File      : AWT_changekey.cxx                                      //
//   Purpose   : changekey management                                   //
//   Time-stamp: <Mon May/23/2005 20:04 MET Coder@ReallySoft.de>        //
//                                                                      //
//                                                                      //
// Coded by Ralf Westram (coder@reallysoft.de) in May 2005              //
// Copyright Department of Microbiology (Technical University Munich)   //
//                                                                      //
// Visit our web site at: http://www.arb-home.de/                       //
//                                                                      //
// ==================================================================== //

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <aw_awars.hxx>
#include <arbdb.h>
#include <arbdbt.h>

#include "awt.hxx"
#include "awt_changekey.hxx"

GBDATA *awt_get_key(GBDATA *gb_main, const char *key, const char *change_key_path)
// get the container of a species key description
{
    GBDATA *gb_key_data = GB_search(gb_main, change_key_path, GB_CREATE_CONTAINER);
    GBDATA *gb_key_name = GB_find(gb_key_data, CHANGEKEY_NAME, key, down_2_level);
    GBDATA *gb_key = 0;
    if (gb_key_name) gb_key = GB_get_father(gb_key_name);
    return gb_key;
}

GB_TYPES awt_get_type_of_changekey(GBDATA *gb_main,const char *field_name, const char *change_key_path) {
    GBDATA *gbd = awt_get_key(gb_main,field_name, change_key_path);
    if(!gbd) return GB_NONE;
    GBDATA *gb_key_type = GB_find(gbd,CHANGEKEY_TYPE,0,down_level);
    if (!gb_key_type) return GB_NONE;
    return (GB_TYPES) GB_read_int(gb_key_type);
}


GB_ERROR awt_add_new_changekey_to_keypath(GBDATA *gb_main,const char *name, int type, const char *keypath)
{
    GBDATA *gb_key;
    GBDATA *key_name;
    GB_ERROR error = 0;
    GBDATA *gb_key_data;
    gb_key_data = GB_search(gb_main,keypath,GB_CREATE_CONTAINER);

    for (gb_key = GB_find(gb_key_data,CHANGEKEY,0,down_level);
         gb_key;
         gb_key = GB_find(gb_key,CHANGEKEY,0,this_level|search_next))
    {
        key_name = GB_search(gb_key,CHANGEKEY_NAME,GB_STRING);
        if (!strcmp(GB_read_char_pntr(key_name),name)) break;
    }
    char *c = GB_first_non_key_char(name);
    if (c) {
        char *new_name = strdup(name);
        *GB_first_non_key_char(new_name) = 0;
        if (*c =='/'){
            error = awt_add_new_changekey(gb_main,new_name,GB_DB);
        }else if (*c =='-'){
            error = awt_add_new_changekey(gb_main,new_name,GB_LINK);
        }else{
            aw_message(GBS_global_string("Cannot add '%s' to your key list",name));
        }
        free(new_name);
        if (error && !strncmp(error, "Fatal",5) ) return error;
    }

    if (!gb_key) {
        GBDATA *key = GB_create_container(gb_key_data,CHANGEKEY);
        key_name = GB_create(key,CHANGEKEY_NAME,GB_STRING);
        GB_write_string(key_name,name);
        GBDATA *key_type = GB_create(key,CHANGEKEY_TYPE,GB_INT);
        GB_write_int(key_type,type);
    }else{
        GBDATA *key_type = GB_search(gb_key,CHANGEKEY_TYPE, GB_INT);
        int elem_type = GB_read_int(key_type);
        if ( elem_type != type && (type != GB_DB || elem_type != GB_LINK)){
            return ("Fatal Error: Key exists and type is incompatible");
        }
        return ("Warning: Key '%s' exists");
    }
    return 0;
}

GB_ERROR awt_add_new_changekey(GBDATA *gb_main,const char *name, int type)              { return awt_add_new_changekey_to_keypath(gb_main, name, type, CHANGE_KEY_PATH); }
GB_ERROR awt_add_new_gene_changekey(GBDATA *gb_main,const char *name, int type)         { return awt_add_new_changekey_to_keypath(gb_main, name, type, CHANGE_KEY_PATH_GENES); }
GB_ERROR awt_add_new_experiment_changekey(GBDATA *gb_main,const char *name, int type)   { return awt_add_new_changekey_to_keypath(gb_main, name, type, CHANGE_KEY_PATH_EXPERIMENTS); }

// --------------------------------------------------------------------------------

static const char GENE_DATA_PATH[]       = "gene_data/gene/";
static const char EXPERIMENT_DATA_PATH[] = "experiment_data/experiment/";

#define GENE_DATA_PATH_LEN       (sizeof(GENE_DATA_PATH)-1)
#define EXPERIMENT_DATA_PATH_LEN (sizeof(EXPERIMENT_DATA_PATH)-1)

inline bool is_in_GENE_path(const char *fieldpath)          { return strncmp(fieldpath, GENE_DATA_PATH,         GENE_DATA_PATH_LEN) == 0; }
inline bool is_in_EXPERIMENT_path(const char *fieldpath)    { return strncmp(fieldpath, EXPERIMENT_DATA_PATH,   EXPERIMENT_DATA_PATH_LEN) == 0; }

inline bool is_in_reserved_path(const char *fieldpath)  {
    return
        is_in_GENE_path(fieldpath) ||
        is_in_EXPERIMENT_path(fieldpath);
}

// --------------------------------------------------------------------------------


static void awt_delete_unused_changekeys(GBDATA *gb_main, const char **names, const char *change_key_path) {
    // deletes all keys from 'change_key_path' which are not listed in 'names'
    GBDATA *gb_key_data = GB_search(gb_main, change_key_path, GB_CREATE_CONTAINER);
    GBDATA *gb_key      = GB_find(gb_key_data, CHANGEKEY, 0, down_level);

    while (gb_key) {
        bool        found = false;
        int         key_type;
        const char *key_name;

        {
            GBDATA *gb_key_type = GB_find(gb_key, CHANGEKEY_TYPE, 0, down_level);
            key_type            = GB_read_int(gb_key_type);

            GBDATA *gb_key_name = GB_find(gb_key, CHANGEKEY_NAME, 0, down_level);
            key_name            = GB_read_char_pntr(gb_key_name);
        }

        for (const char **name = names; *name; ++name) {
            if (strcmp(key_name, (*name)+1) == 0) { // key with this name exists
                if (key_type == (*name)[0]) {
                    found = true;
                }
                // otherwise key exists, byt type mismatches = > delete this key
                break;
            }
        }

        GBDATA *gb_next_key = GB_find(gb_key, CHANGEKEY, 0, this_level|search_next);

        if (!found) {
            if (key_type == GB_DB) { // it's a container
                // test if any subkey is used
                int keylen = strlen(key_name);
                for (const char **name = names; *name; ++name) {
                    const char *n = (*name)+1;

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

static void awt_show_all_changekeys(GBDATA *gb_main, const char *change_key_path) {
    GBDATA *gb_key_data = GB_search(gb_main, change_key_path, GB_CREATE_CONTAINER);
    for (GBDATA *gb_key = GB_find(gb_key_data, CHANGEKEY, 0, down_level);
         gb_key;
         gb_key = GB_find(gb_key, CHANGEKEY, 0, this_level|search_next))
    {
        GBDATA *gb_key_hidden = GB_find(gb_key, CHANGEKEY_HIDDEN, 0, down_level);
        if (gb_key_hidden) {
            if (GB_read_int(gb_key_hidden)) GB_write_int(gb_key_hidden, 0); // unhide
        }
    }
}

void awt_selection_list_rescan(GBDATA *gb_main, long bitfilter, awt_rescan_mode mode) {
    GB_push_transaction(gb_main);
    char   **names;
    char   **name;
    GBDATA 	*gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);

    names = GBT_scan_db(gb_species_data, 0);

    if (mode & AWT_RS_DELETE_UNUSED_FIELDS) awt_delete_unused_changekeys(gb_main, const_cast<const char **>(names), CHANGE_KEY_PATH);
    if (mode & AWT_RS_SHOW_ALL) awt_show_all_changekeys(gb_main, CHANGE_KEY_PATH);

    if (mode & AWT_RS_SCAN_UNKNOWN_FIELDS) {
        awt_add_new_changekey(gb_main,"name",GB_STRING);
        awt_add_new_changekey(gb_main,"acc",GB_STRING);
        awt_add_new_changekey(gb_main,"full_name",GB_STRING);
        awt_add_new_changekey(gb_main,"group_name",GB_STRING);
        awt_add_new_changekey(gb_main,"tmp",GB_STRING);

        for (name = names; *name; name++) {
            if ( (1<<(**name)) & bitfilter ) {	// look if already exists
                if (!is_in_reserved_path((*name)+1)) { // ignore gene, experiment, ... entries
                    awt_add_new_changekey(gb_main,(*name)+1,(int)*name[0]);
                }
            }
        }
    }

    GBT_free_names(names);
    GB_pop_transaction(gb_main);
}



void awt_gene_field_selection_list_rescan(GBDATA *gb_main, long bitfilter, awt_rescan_mode mode) {
    GB_push_transaction(gb_main);
    char   **names;
    char   **name;
    GBDATA  *gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);

    names = GBT_scan_db(gb_species_data, GENE_DATA_PATH);

    if (mode & AWT_RS_DELETE_UNUSED_FIELDS) awt_delete_unused_changekeys(gb_main, const_cast<const char **>(names), CHANGE_KEY_PATH_GENES);
    if (mode & AWT_RS_SHOW_ALL) awt_show_all_changekeys(gb_main, CHANGE_KEY_PATH_GENES);

    if (mode & AWT_RS_SCAN_UNKNOWN_FIELDS) {
        awt_add_new_gene_changekey(gb_main,"name", GB_STRING);

        awt_add_new_gene_changekey(gb_main,"pos_begin", GB_INT);
        awt_add_new_gene_changekey(gb_main,"pos_end", GB_INT);
        awt_add_new_gene_changekey(gb_main,"pos_uncertain", GB_STRING);

        awt_add_new_gene_changekey(gb_main,"pos_begin2", GB_INT);
        awt_add_new_gene_changekey(gb_main,"pos_end2", GB_INT);
        awt_add_new_gene_changekey(gb_main,"pos_uncertain2", GB_STRING);

        awt_add_new_gene_changekey(gb_main,"pos_joined", GB_INT);
        awt_add_new_gene_changekey(gb_main,"complement", GB_BYTE);

        for (name = names; *name; name++) {
            if ( (1<<(**name)) & bitfilter ) {		// look if already exists
		awt_add_new_gene_changekey(gb_main,(*name)+1,(int)*name[0]);
            }
        }
    }

    GBT_free_names(names);
    GB_pop_transaction(gb_main);
}

void awt_experiment_field_selection_list_rescan(GBDATA *gb_main, long bitfilter, awt_rescan_mode mode) {
    GB_push_transaction(gb_main);
    char   **names;
    char   **name;
    GBDATA  *gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);

    names = GBT_scan_db(gb_species_data, EXPERIMENT_DATA_PATH);

    if (mode & AWT_RS_DELETE_UNUSED_FIELDS) awt_delete_unused_changekeys(gb_main, const_cast<const char **>(names), CHANGE_KEY_PATH_EXPERIMENTS);
    if (mode & AWT_RS_SHOW_ALL) awt_show_all_changekeys(gb_main, CHANGE_KEY_PATH_EXPERIMENTS);

    if (mode & AWT_RS_SCAN_UNKNOWN_FIELDS) {
        awt_add_new_experiment_changekey(gb_main,"name", GB_STRING);

        for (name = names; *name; name++) {
            if ( (1<<(**name)) & bitfilter ) { // look if already exists
                if (is_in_EXPERIMENT_path((*name)+1)) {
                    awt_add_new_experiment_changekey(gb_main,(*name)+1+EXPERIMENT_DATA_PATH_LEN,(int)*name[0]);
                }
            }
        }
    }

    GBT_free_names(names);
    GB_pop_transaction(gb_main);
}

void awt_selection_list_scan_unknown_cb(AW_window *,GBDATA *gb_main, long bitfilter)    { awt_selection_list_rescan(gb_main,bitfilter, AWT_RS_SCAN_UNKNOWN_FIELDS); }
void awt_selection_list_delete_unused_cb(AW_window *,GBDATA *gb_main, long bitfilter)   { awt_selection_list_rescan(gb_main,bitfilter, AWT_RS_DELETE_UNUSED_FIELDS); }
void awt_selection_list_unhide_all_cb(AW_window *,GBDATA *gb_main, long bitfilter)      { awt_selection_list_rescan(gb_main,bitfilter, AWT_RS_SHOW_ALL); }
void awt_selection_list_update_cb(AW_window *,GBDATA *gb_main, long bitfilter)          { awt_selection_list_rescan(gb_main,bitfilter, AWT_RS_UPDATE_FIELDS); }

void awt_gene_field_selection_list_scan_unknown_cb(AW_window *,GBDATA *gb_main, long bitfilter)     { awt_gene_field_selection_list_rescan(gb_main,bitfilter, AWT_RS_SCAN_UNKNOWN_FIELDS); }
void awt_gene_field_selection_list_delete_unused_cb(AW_window *,GBDATA *gb_main, long bitfilter)    { awt_gene_field_selection_list_rescan(gb_main,bitfilter, AWT_RS_DELETE_UNUSED_FIELDS); }
void awt_gene_field_selection_list_unhide_all_cb(AW_window *,GBDATA *gb_main, long bitfilter)    { awt_gene_field_selection_list_rescan(gb_main,bitfilter, AWT_RS_SHOW_ALL); }
void awt_gene_field_selection_list_update_cb(AW_window *,GBDATA *gb_main, long bitfilter)           { awt_gene_field_selection_list_rescan(gb_main,bitfilter, AWT_RS_UPDATE_FIELDS); }

void awt_experiment_field_selection_list_scan_unknown_cb(AW_window *,GBDATA *gb_main, long bitfilter)   { awt_experiment_field_selection_list_rescan(gb_main,bitfilter, AWT_RS_SCAN_UNKNOWN_FIELDS); }
void awt_experiment_field_selection_list_delete_unused_cb(AW_window *,GBDATA *gb_main, long bitfilter)  { awt_experiment_field_selection_list_rescan(gb_main,bitfilter, AWT_RS_DELETE_UNUSED_FIELDS); }
void awt_experiment_field_selection_list_unhide_all_cb(AW_window *,GBDATA *gb_main, long bitfilter)  { awt_experiment_field_selection_list_rescan(gb_main,bitfilter, AWT_RS_SHOW_ALL); }
void awt_experiment_field_selection_list_update_cb(AW_window *,GBDATA *gb_main, long bitfilter)         { awt_experiment_field_selection_list_rescan(gb_main,bitfilter, AWT_RS_UPDATE_FIELDS); }



