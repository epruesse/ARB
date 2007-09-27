
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
// #include <malloc.h>
#include <memory.h>

#include <inline.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <AW_rename.hxx>

#include <awt.hxx>
#include <awtlocal.hxx>
#include <awt_changekey.hxx>

#include <db_scanner.hxx>




/**************************************************************************
	create a database scanner
		the scanned database is displayed in a selection list
		the gbdata pointer can be read
***************************************************************************/


/* return the selected GBDATA pntr the should be no !!! running transaction and
	this function will begin a transaction */

static GBDATA *awt_get_arbdb_scanner_gbd_and_begin_trans(AW_CL arbdb_scanid)
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



static GB_BOOL awt_check_scanner_key_data(struct adawcbstruct *cbs,GBDATA *gbd)
{
    GBDATA *gb_key_data;
    gb_key_data = GB_search(cbs->gb_main,cbs->selector->change_key_path,GB_CREATE_CONTAINER);
    return GB_check_father(gbd,gb_key_data);
}

static void awt_arbdb_scanner_delete(void *dummy, struct adawcbstruct *cbs)
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
static void awt_edit_changed_cb(GBDATA *dummy, struct adawcbstruct *cbs, GB_CB_TYPE gbtype)
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


static void awt_arbdb_scanner_value_change(void *, struct adawcbstruct *cbs)
{
    char *value = cbs->aws->get_root()->awar(cbs->def_dest)->read_string();
    int   vlen  = strlen(value);
    
    while (vlen>0 && value[vlen-1] == '\n') vlen--; // remove trailing newlines
    value[vlen]     = 0;

    // read the value from the window
    GBDATA   *gbd         = awt_get_arbdb_scanner_gbd_and_begin_trans((AW_CL)cbs);
    GB_ERROR  error       = 0;
    bool      update_self = false;

    if (!gbd) {
        error = "Sorry, cannot perform your operation, please redo it\n(Hint: No item or fields selected or 'enable edit' is unchecked)";
        if (!cbs->aws->get_root()->awar(cbs->def_filter)->read_int()) {	// edit disabled
            cbs->aws->get_root()->awar(cbs->def_dest)->write_string("");
        }
    }
    else {
        awt_assert(cbs->aws->get_root()->awar(cbs->def_filter)->read_int() != 0); // edit is enabled (disabled causes gdb to be 0)

        GBDATA *gb_key_name;
        char   *key_name = 0;
        if (awt_check_scanner_key_data(cbs,gbd)) { // not exist, create new element
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
                cbs->aws->get_root()->awar(cbs->def_gbd)->write_int((long)gb_new); // remap arbdb
            }
        }
        else { // change old element
            key_name = GB_read_key(gbd);
            if (GB_get_father(gbd) == cbs->gb_user && strcmp(key_name, "name") == 0) { // This is a real rename !!!
                const struct ad_item_selector *selector = cbs->selector;

                if (selector->type == AWT_QUERY_ITEM_SPECIES) { // species
                    GBDATA *gb_name = GB_find(cbs->gb_user, "name", 0, down_level);
                    if (gb_name) {
                        char *name = GB_read_string(gb_name);
                        aw_openstatus("Renaming species");

                        if (strlen(value)) {
                            GBT_begin_rename_session(cbs->gb_main,0);
                            
                            error = GBT_rename_species(name, value, GB_FALSE);

                            if (error) GBT_abort_rename_session();
                            else GBT_commit_rename_session(aw_status, aw_status);
                        }
                        else {
                            error = AWTC_recreate_name(cbs->gb_user, true);
                        }

                        aw_closestatus();
                        free(name);
                    }
                    else {
                        error = GB_export_error("harmless internal error: Please try again");
                    }
                }
                else { // non-species (gene, experiment, etc.)
                    if (strlen(value)) {
                        GBDATA *gb_exists    = 0;
                        GBDATA *gb_item_data = GB_get_father(cbs->gb_user);

                        for (gb_exists = selector->get_first_item(gb_item_data);
                             gb_exists;
                             gb_exists = selector->get_next_item(gb_exists))
                        {
                            GBDATA *gb_name = GB_find(gb_exists, "name", 0, down_level);
                            if (gb_name) {
                                const char *name = GB_read_char_pntr(gb_name);
                                if (ARB_stricmp(name, value) == 0) break;
                            }
                        }

                        if (gb_exists) error = GBS_global_string("There is already a %s named '%s'", selector->item_name, value);
                        else error           = GB_write_as_string(gbd, value);
                    }
                    else {
                        error = "The 'name' field can't be empty.";
                    }
                }

                if (!error) update_self = true;
            }
            else {
                if (strlen(value)) {
                    error = GB_write_as_string(gbd, value);
                }
                else {
                    GBDATA *gb_key = awt_get_key(cbs->gb_main, key_name, cbs->selector->change_key_path);
                    
                    error = GB_delete(gbd);
                    if (!error) {
                        cbs->aws->get_root()->awar(cbs->def_gbd)->write_int( (long) gb_key);
                    }
                }
            }
            
            // if (error) awt_edit_changed_cb(0, cbs, GB_CB_CHANGED); // refresh old value
        }
        free(key_name);
    }
    
    awt_edit_changed_cb(0, cbs, GB_CB_CHANGED); // refresh edit field

    if (error){
        aw_message(error);
        GB_abort_transaction(cbs->gb_main);
    }
    else {
        GB_touch(cbs->gb_user);	// change of linked object does not change source of link, so do it by hand
        GB_commit_transaction(cbs->gb_main);
    }

    if (update_self) { // if the name changed -> rewrite awars AFTER transaction was closed
        GB_transaction ta(cbs->gb_main);
        
        char *my_id = cbs->selector->generate_item_id(cbs->gb_main, cbs->gb_user);
        cbs->selector->update_item_awars(cbs->gb_main, cbs->awr, my_id); // update awars (e.g. AWAR_SPECIES_NAME)
        free(my_id);
    }

    free(value);
}

/***************** change the flag in cbs->gb_user *****************************/

static void awt_mark_changed_cb(AW_window *aws, struct adawcbstruct *cbs, char *awar_name)
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

static void awt_map_arbdb_edit_box(GBDATA *dummy, struct adawcbstruct *cbs)
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



static void awt_scanner_changed_cb(GBDATA *dummy, struct adawcbstruct *cbs, GB_CB_TYPE gbtype);
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
    struct adawcbstruct *cbs        = new adawcbstruct;
    memset(cbs, 0, sizeof(*cbs));

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
    cbs->awr         = aw_root;
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

static void awt_scanner_scan_rek(GBDATA *gbd,struct adawcbstruct *cbs,int deep, AW_selection_list *id)
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

static void awt_scanner_scan_list(GBDATA *dummy, struct adawcbstruct *cbs)
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

static void awt_scanner_changed_cb(GBDATA *dummy, struct adawcbstruct *cbs, GB_CB_TYPE gbtype)
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
static void awt_scanner_changed_cb2(GBDATA *dummy, struct adawcbstruct *cbs, GB_CB_TYPE gbtype)
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

