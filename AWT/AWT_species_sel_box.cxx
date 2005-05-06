#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
// #include <malloc.h>
#include <memory.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include "awt.hxx"
#include "awtlocal.hxx"

void awt_create_selection_list_on_scandb_cb(GBDATA *dummy, struct adawcbstruct *cbs)
{
    GBDATA *gb_key_data;
    gb_key_data = GB_search(cbs->gb_main, cbs->selector->change_key_path, GB_CREATE_CONTAINER);
    AWUSE(dummy);

    cbs->aws->clear_selection_list(cbs->id);

    if (cbs->add_all_fields_pseudo_field) {
        cbs->aws->insert_selection(cbs->id, ALL_FIELDS_PSEUDO_FIELD, ALL_FIELDS_PSEUDO_FIELD);
    }

    GBDATA *gb_key;
    GBDATA *gb_key_name;
    for (	gb_key = GB_find(gb_key_data,CHANGEKEY,0,down_level);
            gb_key;
            gb_key = GB_find(gb_key,CHANGEKEY,0,this_level|search_next))
    {
        GBDATA *key_type = GB_find(gb_key,CHANGEKEY_TYPE,0,down_level);
        if ( !( ((long)cbs->def_filter) & (1<<GB_read_int(key_type)))) continue; // type does not match filter

        gb_key_name = GB_find(gb_key,CHANGEKEY_NAME,0,down_level);
        if (!gb_key_name) continue; // key w/o name -> don't show
        char *name  = GB_read_char_pntr(gb_key_name);

        GBDATA *gb_hidden = GB_find(gb_key, CHANGEKEY_HIDDEN, 0, down_level);
        if (!gb_hidden) { // it's an older db version w/o hidden flag -> add it
            gb_hidden = GB_create(gb_key, CHANGEKEY_HIDDEN, GB_INT);
            GB_write_int(gb_hidden, 0); // default is non-hidden
        }

        if (GB_read_int(gb_hidden)) {
            if (!cbs->include_hidden_fields) continue; // don't show hidden fields

            cbs->aws->insert_selection( cbs->id, GBS_global_string("[hidden] %s", name), name );
        }
        else { // normal field
            cbs->aws->insert_selection( cbs->id, name, name );
        }
    }

    cbs->aws->insert_default_selection( cbs->id, "????", "----" );
    cbs->aws->update_selection_list( cbs->id );
}

GB_ERROR awt_add_new_changekey_to_keypath(GBDATA *gb_main,const char *name, int type, const char *keypath)
{
    GBDATA *gb_key;
    GBDATA *key_name;
    GB_ERROR error = 0;
    GBDATA *gb_key_data;
    gb_key_data = GB_search(gb_main,keypath,GB_CREATE_CONTAINER);

    for (	gb_key = GB_find(gb_key_data,CHANGEKEY,0,down_level);
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
// --------------------------------------------------------------------------------------------------------------------
//      static void awt_delete_unused_changekeys(GBDATA *gb_main, const char **names, const char *change_key_path)
// --------------------------------------------------------------------------------------------------------------------
// deletes all keys from 'change_key_path' which are not listed in 'names'

static void awt_delete_unused_changekeys(GBDATA *gb_main, const char **names, const char *change_key_path) {
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
// -------------------------------------------------------------------------------------------
//      static void awt_show_all_changekeys(GBDATA *gb_main, const char *change_key_path)
// -------------------------------------------------------------------------------------------
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
            if ( (1<<(**name)) & bitfilter ) {		// look if already exists
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

AW_window *awt_existing_window(AW_window *, AW_CL cl1, AW_CL)
{
    return (AW_window*)cl1;
}

AW_CL awt_create_selection_list_on_scandb(GBDATA                 *gb_main,
                                          AW_window              *aws,
                                          const char             *varname,
                                          long                    type_filter,
                                          const char             *scan_xfig_label,
                                          const char             *rescan_xfig_label,
                                          const ad_item_selector *selector,
                                          size_t                  columns,
                                          size_t                  visible_rows,
                                          AW_BOOL                 popup_list_in_window,
                                          AW_BOOL                 add_all_fields_pseudo_field,
                                          AW_BOOL                 include_hidden_fields)
{
    AW_selection_list *id              = 0;
    GBDATA            *gb_key_data;
    AW_window         *win_for_sellist = aws;

    GB_push_transaction(gb_main);

    if (scan_xfig_label) aws->at(scan_xfig_label);

    if (popup_list_in_window) {

        // create HIDDEN popup window containing the selection list
        {
            AW_window_simple *aw_popup = new AW_window_simple;
            aw_popup->init(aws->get_root(), "SELECT_LIST_ENTRY", "SELECT AN ENTRY");
            //         aw_popup->load_xfig(0, AW_TRUE);

            aw_popup->auto_space(10, 10);
            aw_popup->at_newline();

            aw_popup->callback((AW_CB0)AW_POPDOWN);
            id = aw_popup->create_selection_list(varname, 0, "", columns, visible_rows);

            aw_popup->at_newline();
            aw_popup->callback((AW_CB0)AW_POPDOWN);
            aw_popup->create_button("CLOSE", "CLOSE", "C");

            aw_popup->window_fit();

            win_for_sellist = aw_popup;
        }

        aws->button_length(columns);
        aws->callback((AW_CB2)AW_POPUP,(AW_CL)awt_existing_window, (AW_CL)win_for_sellist);
        aws->create_button("SELECTED_ITEM", varname);

    }
    else { // otherwise we build a normal selection list
        id = aws->create_selection_list(varname,0,"",columns,visible_rows); // 20,10);
    }
//     else { // otherwise we build an option menu
//         aws->create_option_menu(varname, 0, "");
//     }

    struct adawcbstruct *cbs;
    cbs                              = new adawcbstruct;
    cbs->aws                         = win_for_sellist;
    cbs->gb_main                     = gb_main;
    cbs->id                          = id;
    cbs->def_filter                  = (char *)type_filter;
    cbs->selector                    = selector;
    cbs->add_all_fields_pseudo_field = add_all_fields_pseudo_field;
    cbs->include_hidden_fields       = include_hidden_fields;

    if (rescan_xfig_label) {
        int x, y;
        aws->get_at_position(&x, &y);

        aws->at(rescan_xfig_label);
        aws->callback(selector->selection_list_rescan_cb, (AW_CL)cbs->gb_main,(AW_CL)-1);
        aws->create_button("RESCAN_DB", "RESCAN","R");

        if (popup_list_in_window) {
            aws->at(x, y);          // restore 'at' position if popup_list_in_window
        }
    }

    awt_create_selection_list_on_scandb_cb(0,cbs);
//     win_for_sellist->update_selection_list( id );

    gb_key_data = GB_search(gb_main, cbs->selector->change_key_path, GB_CREATE_CONTAINER);

    GB_add_callback(gb_key_data, GB_CB_CHANGED, (GB_CB)awt_create_selection_list_on_scandb_cb, (int *)cbs);

    GB_pop_transaction(gb_main);
    return (AW_CL)cbs;
}


/**************************************************************************
	create a database scanner
		the scanned database is displayed in a selection list
		the gbdata pointer can be read
***************************************************************************/


/* return the selected GBDATA pntr the should be no !!! running transaction and
	this function will begin a transaction */

GBDATA *awt_get_arbdb_scanner_gbd_and_begin_trans(AW_CL arbdb_scanid)
{
    struct adawcbstruct *cbs = (struct adawcbstruct *)arbdb_scanid;
    AW_root *aw_root = cbs->aws->get_root();
    cbs->may_be_an_error = 0;
    GB_push_transaction(cbs->gb_main);
    GBDATA *gbd = (GBDATA *)aw_root->awar(cbs->def_gbd)->read_int();
    if (	!cbs->gb_user ||
            !gbd ||
            cbs->may_be_an_error) {		// something changed in the database
        return 0;
    }
    return gbd;
}



GB_BOOL awt_check_scanner_key_data(struct adawcbstruct *cbs,GBDATA *gbd)
{
    GBDATA *gb_key_data;
    gb_key_data = GB_search(cbs->gb_main,cbs->selector->change_key_path,GB_CREATE_CONTAINER);
    return GB_check_father(gbd,gb_key_data);
}

void awt_arbdb_scanner_delete(void *dummy, struct adawcbstruct *cbs)
{
    AWUSE(dummy);
    GBDATA *gbd = awt_get_arbdb_scanner_gbd_and_begin_trans((AW_CL)cbs);
    if (!gbd) {
        aw_message("Sorry, cannot perform your operation, please redo it");
    }else if (awt_check_scanner_key_data(cbs,gbd)) {		// already deleted
        ;
    }else{
        GB_ERROR error = GB_delete(gbd);
        if (error) aw_message((char *)error);
    }
    GB_commit_transaction(cbs->gb_main);
}
void awt_edit_changed_cb(GBDATA *dummy, struct adawcbstruct *cbs, GB_CB_TYPE gbtype)
{
    AWUSE(dummy);
    AW_window *aws = cbs->aws;
    cbs->may_be_an_error = 1;

    if (gbtype == GB_CB_DELETE) {
        cbs->gb_edit = 0;
    }
    if (cbs->gb_edit) {
        if (awt_check_scanner_key_data(cbs,cbs->gb_edit)) {		// dont exist
            aws->get_root()->awar(cbs->def_dest)->write_string("");
        }else{
            char *data;
            data = GB_read_as_string(cbs->gb_edit);
            if (!data) {
                data = strdup("<YOU CANNOT EDIT THIS TYPE>");
            }
            cbs->aws->get_root()->awar(cbs->def_dest)->write_string(data);
            free(data);
        }
    }else{
        aws->get_root()->awar(cbs->def_dest)->write_string("");
    }
}


void awt_arbdb_scanner_value_change(void *dummy, struct adawcbstruct *cbs)
{
    AWUSE(dummy);
    char *value = cbs->aws->get_root()->awar(cbs->def_dest)->read_string();
    int vlen = strlen(value);
    while (vlen>0 && value[vlen-1] == '\n') vlen--;		// remove trailing newlines
    value[vlen] = 0;
    // read the value from the window
    GBDATA *gbd = awt_get_arbdb_scanner_gbd_and_begin_trans((AW_CL)cbs);
    GB_ERROR error = 0;
    if (!gbd) {
        error = "Sorry, cannot perform your operation, please redo it\n(Hint: No item or fields selected or 'enable edit' is unchecked)";
        if (!cbs->aws->get_root()->awar(cbs->def_filter)->read_int()) {	// edit disabled
            cbs->aws->get_root()->awar(cbs->def_dest)->write_string("");
        }
    }
//     else if (!cbs->aws->get_root()->awar(cbs->def_filter)->read_int()) {	// edit disabled
//         if (strlen(value)){
//             error = "Write disabled";
//             cbs->aws->get_root()->awar(cbs->def_dest)->write_string("");
//         }
//     }
    else {
        awt_assert(cbs->aws->get_root()->awar(cbs->def_filter)->read_int() != 0); // edit is enabled (disabled causes gdb to be 0)

        GBDATA *gb_key_name;
        char   *key_name = 0;
        if (awt_check_scanner_key_data(cbs,gbd)) {		// not exist, create new element
            gb_key_name = GB_find(gbd,CHANGEKEY_NAME,0,down_level);
            key_name = GB_read_string(gb_key_name);
            GBDATA *gb_key_type = GB_find(gbd,CHANGEKEY_TYPE,0,down_level);
            if (strlen(value)) {
                GBDATA *gb_new = GB_search(cbs->gb_user, key_name,GB_read_int(gb_key_type));
                if (!gb_new) {
                    error = GB_get_error();
                }
                if (!error){
                    error = GB_write_as_string(gb_new,value);
                }
                cbs->aws->get_root()->awar(cbs->def_gbd)->write_int((long)gb_new);
                // remap arbdb
            }
        } else {		// change old element
            key_name = GB_read_key(gbd);
            if (GB_get_father(gbd) == cbs->gb_user &&!strcmp(key_name, "name")) { // This is a real rename !!!
                int answer = aw_message("Changing the 'name' field will harm your database! Really continue?", "Yes,No", true);
                if (answer == 0) {
                    GBT_begin_rename_session(cbs->gb_main,0);
                    GBDATA *gb_name = GB_find(cbs->gb_user,"name",0,down_level);
                    if (gb_name) {
                        char *name = GB_read_string(gb_name);
                        error = GBT_rename_species( name,value);
                        free(name);
                    }else{
                        error = GB_export_error("harmless internal error: Please try again");
                    }
                    if (error) {
                        GBT_abort_rename_session();
                    }else{
                        GBT_commit_rename_session(0);
                    }
                }
            }else{
                if (strlen(value)) {
                    error = GB_write_as_string(gbd, value);
                    if (error) {
                        awt_edit_changed_cb(0, cbs, GB_CB_CHANGED);
                    }
                } else {
                    GBDATA         *gb_key = awt_get_key(cbs->gb_main, key_name, cbs->selector->change_key_path);
                    error = GB_delete(gbd);
                    if (!error) {
                        cbs->aws->get_root()->awar(cbs->def_gbd)->write_int( (long) gb_key);
                    }
                }
            }
        }
        free(key_name);
    }
    if (error){
        aw_message(error);
        GB_abort_transaction(cbs->gb_main);
    }else{
        GB_touch(cbs->gb_user);	// change of linked object does not change source of link, so do it by hand
        GB_commit_transaction(cbs->gb_main);
    }
    free(value);
}
/********************* get the container of a species key description ***************/

GBDATA *awt_get_key(GBDATA *gb_main, const char *key, const char *change_key_path)
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

/***************** change the flag in cbs->gb_user *****************************/

void awt_mark_changed_cb(AW_window *aws, struct adawcbstruct *cbs, char *awar_name)
{
    cbs->may_be_an_error = 0;
    long flag = aws->get_root()->awar(awar_name)->read_int();
    GB_push_transaction(cbs->gb_main);
    if ( (!cbs->gb_user) || cbs->may_be_an_error) {		// something changed in the database
    }
    else{
        GB_write_flag(cbs->gb_user,flag);
    }
    GB_pop_transaction(cbs->gb_main);
}

void awt_scanner_changed_cb(GBDATA *dummy, struct adawcbstruct *cbs, GB_CB_TYPE gbtype);
/* create an unmapped scanner box and optional some buttons,
   the return value is the id to further scanner functions */


AW_CL awt_create_arbdb_scanner(GBDATA                 *gb_main, AW_window *aws,
                               const char             *box_pos_fig, /* the position for the box in the xfig file */
                               const char             *delete_pos_fig,
                               const char             *edit_pos_fig,
                               const char             *edit_enable_pos_fig,
                               AWT_SCANNERMODE         scannermode,
                               const char             *rescan_pos_fig, // AWT_VIEWER
                               const char             *mark_pos_fig,
                               long                    type_filter,
                               const ad_item_selector *selector)
{
    static int           scanner_id = 0;
    struct adawcbstruct *cbs;
    cbs                             = new adawcbstruct;
    char                 buffer[256];
    AW_root             *aw_root    = aws->get_root();

    GB_push_transaction(gb_main);
    /*************** Create local AWARS *******************/
    sprintf(buffer,"tmp/arbdb_scanner_%i/list",scanner_id);
    cbs->def_gbd = strdup(buffer);
    aw_root->awar_int( cbs->def_gbd, 0, AW_ROOT_DEFAULT);

    sprintf(buffer,"tmp/arbdb_scanner_%i/find",scanner_id);
    cbs->def_source = strdup(buffer);
    aw_root->awar_string( cbs->def_source,"", AW_ROOT_DEFAULT);

    sprintf(buffer,"tmp/arbdb_scanner_%i/edit_enable",scanner_id);
    cbs->def_filter = strdup(buffer);
    aw_root->awar_int( cbs->def_filter,GB_TRUE, AW_ROOT_DEFAULT);

    sprintf(buffer,"tmp/arbdb_scanner_%i/mark",scanner_id);
    cbs->def_dir = strdup(buffer);
    aw_root->awar_int( cbs->def_dir,GB_TRUE, AW_ROOT_DEFAULT);

    aws->at(box_pos_fig);

    cbs->id          = aws->create_selection_list(cbs->def_gbd,0,"",20,10);
    cbs->aws         = aws;
    cbs->gb_main     = gb_main;
    cbs->gb_user     = 0;
    cbs->gb_edit     = 0;
    cbs->scannermode = (char) scannermode;
    cbs->selector    = selector;

    /*************** Create the delete button ****************/
    if (delete_pos_fig) {
        aws->at(delete_pos_fig);
        aws->callback((AW_CB)awt_arbdb_scanner_delete,(AW_CL)cbs,0);
        aws->create_button("DELETE_DB_FIELD", "DELETE","D");
    }

    /*************** Create the enable edit selector ****************/
    if (edit_enable_pos_fig) {
        aws->at(edit_enable_pos_fig);
        aws->callback((AW_CB1)awt_map_arbdb_edit_box,(AW_CL)cbs);
        aws->create_toggle(cbs->def_filter);
    }

    if (mark_pos_fig) {
        aws->at(mark_pos_fig);
        aws->callback((AW_CB)awt_mark_changed_cb,(AW_CL)cbs,(AW_CL)cbs->def_dir);
        aws->create_toggle(cbs->def_dir);
    }

    cbs->def_dest = 0;
    if (edit_pos_fig) {
        aw_root->awar(cbs->def_gbd)->add_callback((AW_RCB1)awt_map_arbdb_edit_box,(AW_CL)cbs);
        if (edit_enable_pos_fig) {
            aw_root->awar(cbs->def_filter)->add_callback((AW_RCB1)awt_map_arbdb_edit_box,(AW_CL)cbs);
        }
        sprintf(buffer,"tmp/arbdb_scanner_%i/edit",scanner_id);
        cbs->def_dest = strdup(buffer);
        aw_root->awar_string( cbs->def_dest,"", AW_ROOT_DEFAULT);

        aws->at(edit_pos_fig);
        aws->callback((AW_CB1)awt_arbdb_scanner_value_change,(AW_CL)cbs);
        aws->create_text_field(cbs->def_dest,20,10);
    }

    /*************** Create the rescan button ****************/
    if (rescan_pos_fig) {
        aws->at(rescan_pos_fig);
        aws->callback(cbs->selector->selection_list_rescan_cb, (AW_CL)cbs->gb_main,(AW_CL)type_filter);
        aws->create_button("RESCAN_DB", "RESCAN","R");
    }

    scanner_id++;
    GB_pop_transaction(gb_main);
    aws->set_popup_callback((AW_CB)awt_scanner_changed_cb,(AW_CL)cbs, GB_CB_CHANGED);
    return (AW_CL)cbs;
}

void awt_map_arbdb_edit_box(GBDATA *dummy, struct adawcbstruct *cbs)
{
    AWUSE(dummy);
    GBDATA *gbd;
    cbs->may_be_an_error = 0;
    GB_push_transaction(cbs->gb_main);
    if (cbs->may_be_an_error) {		// sorry
        cbs->aws->get_root()->awar(cbs->def_gbd)->write_int((long)0);
    }
    gbd = (GBDATA *)cbs->aws->get_root()->awar(cbs->def_gbd)->read_int();

    if (cbs->gb_edit) {
        GB_remove_callback(cbs->gb_edit,(GB_CB_TYPE)(GB_CB_CHANGED|GB_CB_DELETE),
                           (GB_CB)awt_edit_changed_cb, (int *)cbs);
    }

    if (cbs->aws->get_root()->awar(cbs->def_filter)->read_int()) {		// edit enabled
        cbs->gb_edit = gbd;
    }else{
        cbs->gb_edit = 0;		// disable map
    }
    if (cbs->gb_edit) {
        GB_add_callback(cbs->gb_edit,(GB_CB_TYPE)(GB_CB_CHANGED|GB_CB_DELETE),
                        (GB_CB)awt_edit_changed_cb, (int *)cbs);
    }
    awt_edit_changed_cb(gbd,cbs,GB_CB_CHANGED);

    GB_pop_transaction(cbs->gb_main);
}

void awt_scanner_scan_rek(GBDATA *gbd,struct adawcbstruct *cbs,int deep, AW_selection_list *id)
{
    char buffer[256];
    int rest = 255;
    char *p = &buffer[0];
    GB_TYPES type = GB_read_type(gbd);
    GBDATA *gb2;
    char *key = GB_read_key(gbd);
    char *data;
    int ssize;
    for (int i=0; i<deep; i++){
        *(p++) = ':';
        *(p++) = ' ';
        rest -=2;
    }
    sprintf(p,"%-12s",key);
    rest -= strlen(p); p+= strlen(p);

    switch (type) {
        case GB_DB:
            sprintf(p,"<CONTAINER>:");
            cbs->aws->insert_selection( cbs->id, buffer, (long)gbd );
            for (	gb2 = GB_find(gbd,0,0,down_level);
                    gb2;
                    gb2 = GB_find(gb2,0,0,this_level|search_next)){
                awt_scanner_scan_rek(gb2,cbs, deep+1,id);
            }
            break;
        case GB_LINK:{
            sprintf(p,GBS_global_string("LINK TO '%s'",GB_read_link_pntr(gbd)));
            cbs->aws->insert_selection( cbs->id, buffer, (long)gbd );
            GBDATA *gb_al = GB_follow_link(gbd);
            if (gb_al){
                for (	gb2 = GB_find(gbd,0,0,down_level);
                        gb2;
                        gb2 = GB_find(gb2,0,0,this_level|search_next)){
                    awt_scanner_scan_rek(gb2,cbs, deep+1,id);
                }
            }
            break;
        }
        default:
            data = GB_read_as_string(gbd);
            if (data) {
                ssize = strlen(data);
                if (ssize > rest) ssize = rest;
                memcpy(p,data,ssize);
                p[ssize] = 0;
                free(data);
            }else{
                strcpy(p,"<unprintable>");
            }
            break;
    }
    if (type != GB_DB) {
        cbs->aws->insert_selection( cbs->id, buffer, (long)gbd );
    }
    free(key);
}

void awt_scanner_scan_list(GBDATA *dummy, struct adawcbstruct *cbs)
{
#define INFO_WIDTH 1000
refresh_again:
    char buffer[INFO_WIDTH+1];
    memset(buffer,0,INFO_WIDTH+1);

    static int last_max_name_width;
    int        max_name_width = 0;

    if (last_max_name_width == 0) last_max_name_width = 15;

    AWUSE(dummy);
    cbs->aws->clear_selection_list(cbs->id);

    GBDATA *gb_key_data = GB_search(cbs->gb_main, cbs->selector->change_key_path, GB_CREATE_CONTAINER);

    for (int existing = 1; existing >= 0; --existing) {
        for (GBDATA *gb_key = GB_find(gb_key_data,CHANGEKEY,0,down_level);
             gb_key;
             gb_key = GB_find(gb_key,CHANGEKEY,0,this_level|search_next))
        {
            GBDATA *gb_key_hidden = GB_find(gb_key,CHANGEKEY_HIDDEN,0,down_level);
            if (gb_key_hidden && GB_read_int(gb_key_hidden)) continue; // dont show hidden fields in 'species information' window

            GBDATA *gb_key_name = GB_find(gb_key,CHANGEKEY_NAME,0,down_level);
            if (!gb_key_name) continue;

            GBDATA *gb_key_type = GB_find(gb_key,CHANGEKEY_TYPE,0,down_level);

            const char *name = GB_read_char_pntr(gb_key_name);
            GBDATA     *gbd  = GB_search(cbs->gb_user,name,GB_FIND);

            if ((!existing) == (!gbd)) { // first print only existing; then non-existing entries
                char *p      = buffer;
                int   len    = sprintf(p,"%-*s %c", last_max_name_width, name, GB_TYPE_2_CHAR[GB_read_int(gb_key_type)]);

                p += len;

                int name_width = strlen(name);
                if (name_width>max_name_width) max_name_width = name_width;

                if (gbd) {      // existing entry
                    *(p++) = GB_read_security_write(gbd)+'0';
                    *(p++) = ':';
                    *(p++) = ' ';
                    *p     = 0;

                    char *data = GB_read_as_string(gbd);
                    int   ssize;

                    if (data){
                        int rest = INFO_WIDTH-(p-buffer);
                        ssize    = strlen(data);

                        if (ssize > rest) ssize = rest;
                        memcpy(p,data,ssize);
                        p[ssize] = 0;

                        free(data);
                    }
                    cbs->aws->insert_selection( cbs->id, buffer, (long)gbd );
                }
                else { // non-existing entry
                    p[0]=' '; p[1] = ':'; p[2] = 0;
                    cbs->aws->insert_selection( cbs->id, buffer, (long)gb_key );
                }
            }
        }
    }

    if (last_max_name_width < max_name_width) {
        last_max_name_width = max_name_width;
        goto refresh_again;
    }
#undef INFO_WIDTH
}

void awt_scanner_changed_cb(GBDATA *dummy, struct adawcbstruct *cbs, GB_CB_TYPE gbtype)
{
    AWUSE(dummy);
    AW_window *aws = cbs->aws;
    GB_transaction dummy2(cbs->gb_main);

    cbs->may_be_an_error = 1;

    if (gbtype == GB_CB_DELETE) {
        cbs->gb_user = 0;
    }
    if (cbs->gb_user && (cbs->aws->get_show() == AW_FALSE)) {
        // unmap invisible window
        //awt_map_arbdb_scanner((AW_CL)cbs,0,cbs->show_only_marked);
        // recalls this function !!!!
        return;
    }
    aws->clear_selection_list(cbs->id);
    if (cbs->gb_user) {
        switch (cbs->scannermode) {
            case AWT_SCANNER:
                awt_scanner_scan_rek(cbs->gb_user,cbs,0,cbs->id);
                break;
            case AWT_VIEWER:
                awt_scanner_scan_list(cbs->gb_user,cbs);
                break;
        }
    }
    aws->insert_default_selection( cbs->id, "", (long)0 );
    aws->update_selection_list( cbs->id );
    if (cbs->gb_user) {
        long flag = GB_read_flag(cbs->gb_user);
        aws->get_root()->awar(cbs->def_dir)->write_int(flag);
    }
}
/************ Unmap edit field if 'key_data' has been changed (maybe entries deleted)
	*********/
void awt_scanner_changed_cb2(GBDATA *dummy, struct adawcbstruct *cbs, GB_CB_TYPE gbtype)
{
    cbs->aws->get_root()->awar(cbs->def_gbd)->write_int((long)0);
    // unmap edit field
    awt_scanner_changed_cb(dummy,cbs,gbtype);
}

void awt_map_arbdb_scanner(AW_CL arbdb_scanid, GBDATA *gb_pntr, int show_only_marked_flag, const char *key_path)
{
    struct adawcbstruct *cbs         = (struct adawcbstruct *)arbdb_scanid;
    GB_push_transaction(cbs->gb_main);
    GBDATA              *gb_key_data = GB_search(cbs->gb_main,key_path,GB_CREATE_CONTAINER);

    if (cbs->gb_user) {
        GB_remove_callback(cbs->gb_user,(GB_CB_TYPE)(GB_CB_CHANGED|GB_CB_DELETE), (GB_CB)awt_scanner_changed_cb, (int *)cbs);
        if (cbs->scannermode == AWT_VIEWER) {
            GB_remove_callback(gb_key_data,(GB_CB_TYPE)(GB_CB_CHANGED), (GB_CB)awt_scanner_changed_cb2, (int *)cbs);
        }
    }

    cbs->show_only_marked = show_only_marked_flag;
    cbs->gb_user          = gb_pntr;

    if (gb_pntr) {
        GB_add_callback(gb_pntr,(GB_CB_TYPE)(GB_CB_CHANGED|GB_CB_DELETE), (GB_CB)awt_scanner_changed_cb, (int *)cbs);
        if (cbs->scannermode == AWT_VIEWER) {
            GB_add_callback(gb_key_data,(GB_CB_TYPE)(GB_CB_CHANGED), (GB_CB)awt_scanner_changed_cb2, (int *)cbs);
        }
    }

    cbs->aws->get_root()->awar(cbs->def_gbd)->write_int((long)0);
    awt_scanner_changed_cb(gb_pntr,cbs,GB_CB_CHANGED);

    GB_pop_transaction(cbs->gb_main);
}

