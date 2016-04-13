// =============================================================== //
//                                                                 //
//   File      : db_scanner.cxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <db_scanner.hxx>

#include <AW_rename.hxx>

#include <aw_awar.hxx>
#include <aw_select.hxx>
#include <aw_msg.hxx>
#include <arb_progress.h>
#include <aw_root.hxx>

#include <arb_str.h>
#include <arb_strbuf.h>
#include <ad_cb.h>

#include <unistd.h>
#include <ctime>

/*!************************************************************************
    create a database scanner
        the scanned database is displayed in a selection list
        the gbdata pointer can be read
***************************************************************************/

#if defined(WARN_TODO)
#warning derive DbScanner from AW_DB_selection
#endif

struct DbScanner {
    ItemSelector& selector;

    AW_window *aws;
    AW_root   *awr;
    GBDATA    *gb_main;

    AW_selection_list *fields;

    GBDATA         *gb_user;
    GBDATA         *gb_edit;
    bool            may_be_an_error;
    DB_SCANNERMODE  scannermode;

    char *awarname_editfield;
    char *awarname_current_item; // awar contains pointer to mapped item (GBDATA*)
    char *awarname_edit_enabled;
    char *awarname_mark;

    DbScanner(ItemSelector& selector_)
        : selector(selector_),
          aws(NULL),
          awr(NULL),
          gb_main(NULL),
          fields(NULL),
          gb_user(NULL),
          gb_edit(NULL),
          may_be_an_error(false),
          scannermode(DB_SCANNER),
          awarname_editfield(NULL), 
          awarname_current_item(NULL), 
          awarname_edit_enabled(NULL), 
          awarname_mark(NULL) 
    {
        arb_assert(&selector);
    }

    char *get_mapped_item_id() const {
        char *id = NULL;
        if (gb_user) {
            GB_transaction ta(gb_main);
            id = selector.generate_item_id(gb_main, gb_user);
        }
        return id;
    }
};

static GBDATA *get_mapped_item_and_begin_trans(DbScanner *cbs) {
    /* return the selected GBDATA pntr
     * there should be no running transaction; this function will begin a transaction */
    cbs->may_be_an_error = false;
    GB_push_transaction(cbs->gb_main);

    GBDATA *gbd = cbs->awr->awar(cbs->awarname_current_item)->read_pointer();

    if (!cbs->gb_user || !gbd || cbs->may_be_an_error) { // something changed in the database
        return NULL;
    }
    return gbd;
}



static bool inside_scanner_keydata(DbScanner *cbs, GBDATA *gbd) {
    // return true if 'gbd' is below keydata of scanner
    GBDATA *gb_key_data;
    gb_key_data = GB_search(cbs->gb_main, cbs->selector.change_key_path, GB_CREATE_CONTAINER);
    return GB_check_father(gbd, gb_key_data);
}

static void scanner_delete_selected_field(AW_window*, DbScanner *cbs) {
    GBDATA *gbd = get_mapped_item_and_begin_trans(cbs);
    if (!gbd) {
        aw_message("Sorry, cannot perform your operation, please redo it");
    }
    else if (inside_scanner_keydata(cbs, gbd)) {       // already deleted
        ;
    }
    else {
        GB_ERROR error = GB_delete(gbd);
        if (error) aw_message((char *)error);
    }
    GB_commit_transaction(cbs->gb_main);
}

static void selected_field_changed_cb(GBDATA *, DbScanner *cbs, GB_CB_TYPE gbtype) {
    cbs->may_be_an_error = true;

    if (gbtype == GB_CB_DELETE) {
        cbs->gb_edit = 0;
    }
    if (cbs->gb_edit) {
        if (inside_scanner_keydata(cbs, cbs->gb_edit)) {    // doesn't exist
            cbs->awr->awar(cbs->awarname_editfield)->write_string("");
        }
        else {
            char *data = GB_read_as_string(cbs->gb_edit);
            if (!data) data = strdup("<YOU CANNOT EDIT THIS TYPE>");
            cbs->awr->awar(cbs->awarname_editfield)->write_string(data);
            free(data);
        }
    }
    else {
        cbs->awr->awar(cbs->awarname_editfield)->write_string("");
    }
}


static void editfield_value_changed(AW_window*, DbScanner *cbs)
{
    char *value = cbs->awr->awar(cbs->awarname_editfield)->read_string();
    int   vlen  = strlen(value);

    while (vlen>0 && value[vlen-1] == '\n') vlen--; // remove trailing newlines
    value[vlen]     = 0;

    // read the value from the window
    GBDATA   *gbd         = get_mapped_item_and_begin_trans(cbs);
    GB_ERROR  error       = 0;
    bool      update_self = false;

    if (!gbd) {
        error = "No item or fields selected";
    }
    else if (!cbs->awr->awar(cbs->awarname_edit_enabled)->read_int()) { // edit disabled
        cbs->awr->awar(cbs->awarname_editfield)->write_string("");
        error = "Edit is disabled";
    }
    else {
        char *key_name = 0;

        if (inside_scanner_keydata(cbs, gbd)) { // not exist, create new element
            GBDATA *gb_key_name = GB_entry(gbd, CHANGEKEY_NAME);
            key_name            = GB_read_string(gb_key_name);
            GBDATA *gb_key_type = GB_entry(gbd, CHANGEKEY_TYPE);

            if (strlen(value)) {
                GBDATA *gb_new     = GB_search(cbs->gb_user, key_name, (GB_TYPES)GB_read_int(gb_key_type));
                if (!gb_new) error = GB_await_error();
                else    error      = GB_write_autoconv_string(gb_new, value);

                cbs->awr->awar(cbs->awarname_current_item)->write_pointer(gb_new); // remap arbdb
            }
        }
        else { // change old element
            key_name = GB_read_key(gbd);
            if (GB_get_father(gbd) == cbs->gb_user && strcmp(key_name, "name") == 0) { // This is a real rename !!!
                ItemSelector& selector = cbs->selector;

                if (selector.type == QUERY_ITEM_SPECIES) { // species
                    arb_progress  progress("Manual change of species ID");
                    char         *name = nulldup(GBT_read_name(cbs->gb_user));

                    if (strlen(value)) {
                        GBT_begin_rename_session(cbs->gb_main, 0);

                        error = GBT_rename_species(name, value, false);

                        if (error) GBT_abort_rename_session();
                        else error = GBT_commit_rename_session();
                    }
                    else {
                        error = AWTC_recreate_name(cbs->gb_user);
                    }

                    free(name);
                }
                else { // non-species (gene, experiment, etc.)
                    if (strlen(value)) {
                        GBDATA *gb_exists    = 0;
                        GBDATA *gb_item_data = GB_get_father(cbs->gb_user);

                        for (gb_exists = selector.get_first_item(gb_item_data, QUERY_ALL_ITEMS);
                             gb_exists;
                             gb_exists = selector.get_next_item(gb_exists, QUERY_ALL_ITEMS))
                        {
                            if (ARB_stricmp(GBT_read_name(gb_exists), value) == 0) break;
                        }

                        if (gb_exists) error = GBS_global_string("There is already a %s named '%s'", selector.item_name, value);
                        else error           = GB_write_autoconv_string(gbd, value);
                    }
                    else {
                        error = "The 'name' field can't be empty.";
                    }
                }

                if (!error) update_self = true;
            }
            else {
                if (strlen(value)) {
                    error = GB_write_autoconv_string(gbd, value);
                }
                else {
                    GBDATA *gb_key = GBT_get_changekey(cbs->gb_main, key_name, cbs->selector.change_key_path);
                    if (GB_child(gbd)) {
                        error = "Sorry, cannot perform a deletion.\n(The selected entry has child entries. Delete them first.)";
                    }
                    else {
                        error = GB_delete(gbd);
                        if (!error) {
                            cbs->awr->awar(cbs->awarname_current_item)->write_pointer(gb_key);
                        }
                    }
                }
            }

        }
        free(key_name);
    }

    selected_field_changed_cb(0, cbs, GB_CB_CHANGED); // refresh edit field

    if (error) {
        aw_message(error);
        GB_abort_transaction(cbs->gb_main);
    }
    else {
        GB_touch(cbs->gb_user); // change of linked object does not change source of link, so do it by hand
        GB_commit_transaction(cbs->gb_main);
    }

    if (update_self) { // if the name changed -> rewrite awars AFTER transaction was closed
        GB_transaction ta(cbs->gb_main);

        char *my_id = cbs->selector.generate_item_id(cbs->gb_main, cbs->gb_user);
        cbs->selector.update_item_awars(cbs->gb_main, cbs->awr, my_id); // update awars (e.g. AWAR_SPECIES_NAME)
        free(my_id);
    }

    free(value);
}

static void toggle_marked_cb(AW_window *aws, DbScanner *cbs) {
    cbs->may_be_an_error = false;

    long flag = aws->get_root()->awar(cbs->awarname_mark)->read_int();
    GB_push_transaction(cbs->gb_main);
    if ((!cbs->gb_user) || cbs->may_be_an_error) {      // something changed in the database
    }
    else {
        GB_write_flag(cbs->gb_user, flag);
    }
    GB_pop_transaction(cbs->gb_main);
}

static void remap_edit_box(UNFIXED, DbScanner *cbs) {
    cbs->may_be_an_error = false;
    GB_push_transaction(cbs->gb_main);
    if (cbs->may_be_an_error) {     // sorry
        cbs->awr->awar(cbs->awarname_current_item)->write_pointer(NULL);
    }
    GBDATA *gbd = cbs->awr->awar(cbs->awarname_current_item)->read_pointer();

    if (cbs->gb_edit) {
        GB_remove_callback(cbs->gb_edit, GB_CB_CHANGED_OR_DELETED, makeDatabaseCallback(selected_field_changed_cb, cbs));
    }

    if (cbs->awr->awar(cbs->awarname_edit_enabled)->read_int()) {      // edit enabled
        cbs->gb_edit = gbd;
    }
    else {
        cbs->gb_edit = 0;       // disable map
    }
    if (cbs->gb_edit) {
        GB_add_callback(cbs->gb_edit, GB_CB_CHANGED_OR_DELETED, makeDatabaseCallback(selected_field_changed_cb, cbs));
    }
    selected_field_changed_cb(gbd, cbs, GB_CB_CHANGED);

    GB_pop_transaction(cbs->gb_main);
}



static void scanner_changed_cb(UNFIXED, DbScanner *cbs, GB_CB_TYPE gbtype);

DbScanner *create_db_scanner(GBDATA         *gb_main,
                             AW_window      *aws,
                             const char     *box_pos_fig,            // the position for the box in the xfig file
                             const char     *delete_pos_fig,
                             const char     *edit_pos_fig,
                             const char     *edit_enable_pos_fig,
                             DB_SCANNERMODE  scannermode,
                             const char     *mark_pos_fig,
                             ItemSelector&   selector)
{
    /* create an unmapped scanner box and optional some buttons,
       the return value is the id to further scanner functions */

    static int scanner_id = 0;

    DbScanner *cbs = new DbScanner(selector);

    char     buffer[256];
    AW_root *aw_root = aws->get_root();

    GB_push_transaction(gb_main);
    //!************* Create local AWARS ******************
    sprintf(buffer, "tmp/arbdb_scanner_%i/list", scanner_id);
    cbs->awarname_current_item = strdup(buffer);
    aw_root->awar_pointer(cbs->awarname_current_item, 0, AW_ROOT_DEFAULT);

    sprintf(buffer, "tmp/arbdb_scanner_%i/edit_enable", scanner_id);
    cbs->awarname_edit_enabled = strdup(buffer);
    aw_root->awar_int(cbs->awarname_edit_enabled, true, AW_ROOT_DEFAULT);

    sprintf(buffer, "tmp/arbdb_scanner_%i/mark", scanner_id);
    cbs->awarname_mark = strdup(buffer);
    aw_root->awar_int(cbs->awarname_mark, true, AW_ROOT_DEFAULT);

    aws->at(box_pos_fig);

    cbs->fields      = aws->create_selection_list(cbs->awarname_current_item, 20, 10, true);
    cbs->aws         = aws;
    cbs->awr         = aw_root;
    cbs->gb_main     = gb_main;
    cbs->gb_user     = 0;
    cbs->gb_edit     = 0;
    cbs->scannermode = scannermode;

    //!************* Create the delete button ***************
    if (delete_pos_fig) {
        aws->at(delete_pos_fig);
        aws->callback(makeWindowCallback(scanner_delete_selected_field, cbs));
        aws->create_button("DELETE_DB_FIELD", "DELETE", "D");
    }

    //!************* Create the enable edit selector ***************
    if (edit_enable_pos_fig) {
        aws->at(edit_enable_pos_fig);
        aws->callback(makeWindowCallback(remap_edit_box, cbs));
        aws->create_toggle(cbs->awarname_edit_enabled);
    }

    if (mark_pos_fig) {
        aws->at(mark_pos_fig);
        aws->callback(makeWindowCallback(toggle_marked_cb, cbs));
        aws->create_toggle(cbs->awarname_mark);
    }

    cbs->awarname_editfield = 0;
    if (edit_pos_fig) {
        RootCallback remap_cb = makeRootCallback(remap_edit_box, cbs);
        aw_root->awar(cbs->awarname_current_item)->add_callback(remap_cb);
        if (edit_enable_pos_fig) {
            aw_root->awar(cbs->awarname_edit_enabled)->add_callback(remap_cb);
        }
        sprintf(buffer, "tmp/arbdb_scanner_%i/edit", scanner_id);
        cbs->awarname_editfield = strdup(buffer);
        aw_root->awar_string(cbs->awarname_editfield, "", AW_ROOT_DEFAULT);

        aws->at(edit_pos_fig);
        aws->callback(makeWindowCallback(editfield_value_changed, cbs));
        aws->create_text_field(cbs->awarname_editfield, 20, 10);
    }

    scanner_id++;
    GB_pop_transaction(gb_main);
    aws->set_popup_callback(makeWindowCallback(scanner_changed_cb, cbs, GB_CB_CHANGED));
    return cbs;
}

static void scan_fields_recursive(GBDATA *gbd, DbScanner *cbs, int deep, AW_selection_list *fields)
{
    GB_TYPES  type = GB_read_type(gbd);
    char     *key  = GB_read_key(gbd);

    GBS_strstruct *out = GBS_stropen(1000);
    for (int i = 0; i < deep; i++) GBS_strcat(out, ": ");
    GBS_strnprintf(out, 30, "%-12s", key);

    switch (type) {
        case GB_DB: {
            GBS_strcat(out, "<CONTAINER>:");
            cbs->fields->insert(GBS_mempntr(out), gbd);
            GBS_strforget(out);

            for (GBDATA *gb2 = GB_child(gbd); gb2; gb2 = GB_nextChild(gb2)) {
                scan_fields_recursive(gb2, cbs, deep + 1, fields);
            }
            break;
        }
        case GB_LINK: {
            GBS_strnprintf(out, 100, "LINK TO '%s'", GB_read_link_pntr(gbd));
            cbs->fields->insert(GBS_mempntr(out), gbd);
            GBS_strforget(out);

            GBDATA *gb_al = GB_follow_link(gbd);
            if (gb_al) {
                for (GBDATA *gb2 = GB_child(gb_al); gb2; gb2 = GB_nextChild(gb2)) {
                    scan_fields_recursive(gb2, cbs, deep + 1, fields);
                }
            }
            break;
        }
        default: {
            char *data = GB_read_as_string(gbd);

            if (data) {
                GBS_strcat(out, data);
                free(data);
            }
            else {
                GBS_strcat(out, "<unprintable>");
            }
            cbs->fields->insert(GBS_mempntr(out), gbd);
            GBS_strforget(out);
            break;
        }
    }
    free(key);
}

static void scan_list(GBDATA *, DbScanner *cbs) {
#define INFO_WIDTH 1000
 refresh_again :
    char buffer[INFO_WIDTH+1];
    memset(buffer, 0, INFO_WIDTH+1);

    static int last_max_name_width;
    int        max_name_width = 0;

    if (last_max_name_width == 0) last_max_name_width = 15;

    cbs->fields->clear();

    GBDATA *gb_key_data = GB_search(cbs->gb_main, cbs->selector.change_key_path, GB_CREATE_CONTAINER);

    for (int existing = 1; existing >= 0; --existing) {
        for (GBDATA *gb_key = GB_entry(gb_key_data, CHANGEKEY); gb_key; gb_key = GB_nextEntry(gb_key)) {
            GBDATA *gb_key_hidden = GB_entry(gb_key, CHANGEKEY_HIDDEN);
            if (gb_key_hidden && GB_read_int(gb_key_hidden)) continue; // don't show hidden fields in 'species information' window

            GBDATA *gb_key_name = GB_entry(gb_key, CHANGEKEY_NAME);
            if (!gb_key_name) continue;

            GBDATA *gb_key_type = GB_entry(gb_key, CHANGEKEY_TYPE);

            const char *name = GB_read_char_pntr(gb_key_name);
            GBDATA     *gbd  = GB_search(cbs->gb_user, name, GB_FIND);

            if ((!existing) == (!gbd)) { // first print only existing; then non-existing entries
                char *p      = buffer;
                int   len    = sprintf(p, "%-*s %c", last_max_name_width, name, GB_type_2_char((GB_TYPES)GB_read_int(gb_key_type)));

                p += len;

                int name_width = strlen(name);
                if (name_width>max_name_width) max_name_width = name_width;

                if (gbd) {      // existing entry
                    *(p++) = GB_read_security_write(gbd)+'0';
                    *(p++) = ':';
                    *(p++) = ' ';
                    *p     = 0;

                    {
                        char *data = GB_read_as_string(gbd);
                        if (data) {
                            int rest  = INFO_WIDTH-(p-buffer);
                            int ssize = strlen(data);

                            if (ssize > rest) {
                                ssize = GBS_shorten_repeated_data(data);
                                if (ssize > rest) {
                                    if (ssize>5) strcpy(data+rest-5, "[...]");
                                    ssize = rest;
                                }
                            }

                            memcpy(p, data, ssize);
                            p[ssize] = 0;

                            free(data);
                        }
                    }
                    cbs->fields->insert(buffer, gbd);
                }
                else { // non-existing entry
                    p[0] = ' '; p[1] = ':'; p[2] = 0;
                    cbs->fields->insert(buffer, gb_key);
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

static void scanner_changed_cb(UNFIXED, DbScanner *cbs, GB_CB_TYPE gbtype) { // called by DB-callback or WindowCallback
    cbs->may_be_an_error = true;

    if (gbtype == GB_CB_DELETE) {
        cbs->gb_user = 0;
    }
    if (cbs->gb_user && !cbs->aws->is_shown()) {
        // unmap invisible window
        // map_db_scanner(cbs,0);
        // recalls this function !!!!
        return;
    }
    cbs->fields->clear();
    if (cbs->gb_user) {
        GB_transaction ta(cbs->gb_main);

        switch (cbs->scannermode) {
            case DB_SCANNER:
                scan_fields_recursive(cbs->gb_user, cbs, 0, cbs->fields);
                break;
            case DB_VIEWER:
                scan_list(cbs->gb_user, cbs);
                break;
        }
    }
    cbs->fields->insert_default("", (GBDATA*)NULL);
    cbs->fields->update();
    if (cbs->gb_user) {
        GB_transaction ta(cbs->gb_main);

        long flag = GB_read_flag(cbs->gb_user);
        cbs->awr->awar(cbs->awarname_mark)->write_int(flag);
    }
}
/*!********** Unmap edit field if 'key_data' has been changed (maybe entries deleted)
 *********/
static void scanner_changed_dcb2(GBDATA*, DbScanner *cbs, GB_CB_TYPE gbtype) {
    cbs->awr->awar(cbs->awarname_current_item)->write_pointer(NULL);
    // unmap edit field
    scanner_changed_cb(NULL, cbs, gbtype);
}

void map_db_scanner(DbScanner *scanner, GBDATA *gb_pntr, const char *key_path) {
    GB_transaction ta(scanner->gb_main);

    GBDATA *gb_key_data = GB_search(scanner->gb_main, key_path, GB_CREATE_CONTAINER);

    if (scanner->gb_user) {
        GB_remove_callback(scanner->gb_user, GB_CB_CHANGED_OR_DELETED, makeDatabaseCallback(scanner_changed_cb, scanner));
    }
    if (scanner->scannermode == DB_VIEWER) {
        GB_remove_callback(gb_key_data, GB_CB_CHANGED, makeDatabaseCallback(scanner_changed_dcb2, scanner));
    }

    scanner->gb_user = gb_pntr;

    if (gb_pntr) {
        GB_add_callback(gb_pntr, GB_CB_CHANGED_OR_DELETED, makeDatabaseCallback(scanner_changed_cb, scanner));
        if (scanner->scannermode == DB_VIEWER) {
            GB_add_callback(gb_key_data, GB_CB_CHANGED, makeDatabaseCallback(scanner_changed_dcb2, scanner));
        }
    }

    scanner->awr->awar(scanner->awarname_current_item)->write_pointer(NULL);
    scanner_changed_cb(NULL, scanner, GB_CB_CHANGED);
}

GBDATA *get_db_scanner_main(const DbScanner *scanner) {
    return scanner->gb_main;
}

char *get_id_of_item_mapped_in(const DbScanner *scanner) {
    return scanner->get_mapped_item_id();
}

const ItemSelector& get_itemSelector_of(const DbScanner *scanner) {
    return scanner->selector;
}
