#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include "ad_spec.hxx"
#include <awt.hxx>
#include <awt_www.hxx>
#include <awt_tree.hxx>
#include <awt_canvas.hxx>
#include <awt_dtree.hxx>
#include <awtlocal.hxx>
#include <awt_changekey.hxx>
#include <awt_sel_boxes.hxx>
#include <db_scanner.hxx>
#include <awtc_next_neighbours.hxx>
#include <AW_rename.hxx>
#include <ntree.hxx>

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define nt_assert(bed) arb_assert(bed)

extern GBDATA *gb_main;
#define AD_F_ALL (AW_active)(-1)

void create_species_var(AW_root *aw_root, AW_default aw_def)
{
    aw_root->awar_string( AWAR_SPECIES_DEST, "" ,   aw_def);
    aw_root->awar_string( AWAR_SPECIES_INFO, "" ,   aw_def);
    aw_root->awar_string( AWAR_SPECIES_KEY, "" ,    aw_def);
    aw_root->awar_string( AWAR_FIELD_REORDER_SOURCE, "" ,   aw_def);
    aw_root->awar_string( AWAR_FIELD_REORDER_DEST, "" , aw_def);
    aw_root->awar_string( AWAR_FIELD_CREATE_NAME, "" ,  aw_def);
    aw_root->awar_int( AWAR_FIELD_CREATE_TYPE, GB_STRING,   aw_def );
    aw_root->awar_string( AWAR_FIELD_DELETE, "" ,   aw_def);

}

static void move_species_to_extended(AW_window *aww){
    GB_ERROR error = 0;
    char *source = aww->get_root()->awar(AWAR_SPECIES_NAME)->read_string();
    GB_begin_transaction(gb_main);

    GBDATA *gb_extended_data = GB_search(gb_main,"extended_data",GB_CREATE_CONTAINER);
    GBDATA *gb_species = GBT_find_species(gb_main,source);
    GBDATA *gb_dest = GBT_find_SAI_rel_exdata(gb_extended_data,source);
    if (gb_dest) {
        error = "Sorry: SAI already exists";
    }else   if (gb_species) {
        gb_dest = GB_create_container(gb_extended_data,"extended");
        error = GB_copy(gb_dest,gb_species);
        if (!error) {
            error = GB_delete(gb_species);
            if (!error) {
                aww->get_root()->awar(AWAR_SPECIES_NAME)->write_string("");
            }
        }
    }else{
        error = "Please select a species first";
    }

    if (!error) GB_commit_transaction(gb_main);
    else    GB_abort_transaction(gb_main);
    if (error) aw_message(error);
    free(source);
}


static void species_create_cb(AW_window *aww){
    GB_ERROR error = 0;
    char *dest = aww->get_root()->awar(AWAR_SPECIES_DEST)->read_string();
    GB_begin_transaction(gb_main);
    GBDATA *gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    GBDATA *gb_dest = GBT_find_species_rel_species_data(gb_species_data,dest);
    if (gb_dest) {
        error = "Sorry: species already exists";
    }else{
        gb_dest = GBT_create_species_rel_species_data(gb_species_data,dest);
        if (!gb_dest) error = GB_get_error();
        if (!error) {
            aww->get_root()->awar(AWAR_SPECIES_NAME)->write_string(dest);
        }
    }
    if (!error) GB_commit_transaction(gb_main);
    else    GB_abort_transaction(gb_main);
    if (error) aw_message(error);
    free(dest);
}

static AW_window *create_species_create_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "CREATE_SPECIES","SPECIES CREATE");
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label");
    aws->create_autosize_button(0,"Please enter the name\nof the new species");

    aws->at("input");
    aws->create_input_field(AWAR_SPECIES_DEST,15);

    aws->at("ok");
    aws->callback(species_create_cb);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}

static GBDATA *expect_species_selected(AW_root *aw_root, char **give_name = 0) {
    GB_transaction  ta(gb_main);
    char           *name       = aw_root->awar(AWAR_SPECIES_NAME)->read_string();
    GBDATA         *gb_species = GBT_find_species(gb_main, name);

    if (!gb_species) {
        if (name && name[0]) aw_message(GBS_global_string("Species '%s' does not exist.", name));
        else aw_message("Please select a species first");
    }

    if (give_name) *give_name = name;
    else free(name);

    return gb_species;
}

static void ad_species_copy_cb(AW_window *aww, AW_CL, AW_CL) {
    AW_root *aw_root    = aww->get_root();
    char    *name;
    GBDATA  *gb_species = expect_species_selected(aw_root, &name);
    if (gb_species) {
        GB_transaction  ta(gb_main);
        GBDATA         *gb_species_data = GB_get_father(gb_species);
        UniqueNameDetector und(gb_species_data);
        char           *copy_name       = AWTC_makeUniqueShortName(GBS_global_string("c%s", name), und);
        GBDATA         *gb_new_species  = GB_create_container(gb_species_data, "species");
        GB_ERROR        error           = 0;

        if (!gb_new_species) {
            error = GB_get_error();
        }
        else {
            error = GB_copy(gb_new_species, gb_species);
            if (!error) {
                GBDATA *gb_name = GB_search(gb_new_species,"name",GB_STRING);
                error           = GB_write_string(gb_name, copy_name);
                
                if (!error) aw_root->awar(AWAR_SPECIES_NAME)->write_string(copy_name); // set focus
            }
        }

        if (error) {
            ta.abort();
            aw_message(error);
        }
        free(copy_name);
    }
}

static void ad_species_rename_cb(AW_window *aww, AW_CL, AW_CL) {
    AW_root *aw_root    = aww->get_root();
    GBDATA  *gb_species = expect_species_selected(aw_root);
    if (gb_species) {
        GB_transaction  ta(gb_main);
        GBDATA         *gb_full_name  = GB_search(gb_species, "full_name", GB_STRING);
        const char     *full_name     = gb_full_name ? GB_read_char_pntr(gb_full_name) : "";
        char           *new_full_name = aw_input("Enter new 'full_name' of species:", 0, full_name);

        if (new_full_name) {
            GB_ERROR error = 0;

            if (strcmp(full_name, new_full_name) != 0) {
                error = GB_write_string(gb_full_name, new_full_name);
            }
            if (!error) {
                if (aw_message("Do you want to re-create the 'name' field?", "Yes,No") == 0) {
                    aw_openstatus("Recreating species name");
                    aw_status("");
                    aw_status(0.0);
                    error = AWTC_recreate_name(gb_species, true);
                    if (!error) {
                        GBDATA *gb_name = GB_find(gb_species, "name", 0, down_level);
                        if (gb_name) aw_root->awar(AWAR_SPECIES_NAME)->write_string(GB_read_char_pntr(gb_name)); // set focus
                    }
                    aw_closestatus();
                }
            }

            if (error) {
                ta.abort();
                aw_message(error);
            }
        }
    }
}

static void ad_species_delete_cb(AW_window *aww, AW_CL, AW_CL) {
    AW_root  *aw_root    = aww->get_root();
    char     *name;
    GBDATA   *gb_species = expect_species_selected(aw_root, &name);
    GB_ERROR  error      = 0;

    if (!gb_species) {
        error = "Please select a species first";
    }
    else if (aw_message(GBS_global_string("Are you sure to delete the species '%s'", name), "OK,CANCEL") == 0) {
        GB_transaction ta(gb_main);
        error = GB_delete(gb_species);
        if (error) ta.abort();
        else aw_root->awar(AWAR_SPECIES_NAME)->write_string("");
    }

    if (error) aw_message(error);
    free(name);
}

static AW_CL    ad_global_scannerid      = 0;
static AW_root *ad_global_scannerroot    = 0;
static AW_root *ad_global_default_awroot = 0;

void AD_set_default_root(AW_root *aw_root) {
    ad_global_default_awroot = aw_root;
}

static void AD_map_species(AW_root *aw_root, AW_CL scannerid, AW_CL mapOrganism)
{
    GB_push_transaction(gb_main);
    char *source = aw_root->awar((bool)mapOrganism ? AWAR_ORGANISM_NAME : AWAR_SPECIES_NAME)->read_string();
    GBDATA *gb_species = GBT_find_species(gb_main,source);
    if (gb_species) {
        awt_map_arbdb_scanner(scannerid,gb_species,0, CHANGE_KEY_PATH);
    }
    GB_pop_transaction(gb_main);
    free(source);
}
void AD_map_viewer(GBDATA *gbd,AD_MAP_VIEWER_TYPE type)
{
    GB_push_transaction(gb_main);

    char *species_name = strdup("");
    {
        GBDATA *gb_species_data        = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
        if (gbd && GB_get_father(gbd) == gb_species_data) {
            // I got a species !!!!!!
            GBDATA *gb_name            = GB_find(gbd,"name",0,down_level);
            if (gb_name) {
                free(species_name);
                species_name = GB_read_string(gb_name);
            }
        }
    }

    // if we have an active info-scanner, then update his awar

    if (ad_global_scannerroot) {
        ad_global_scannerroot->awar(AWAR_SPECIES_NAME)->write_string(species_name);
        // if (type != ADMVT_SELECT) {
        //             awt_map_arbdb_scanner(ad_global_scannerid,gbd,0, CHANGE_KEY_PATH);
        // }
    }
    else {                      // no active scanner -> write global awar
        if (ad_global_default_awroot) {
            ad_global_default_awroot->awar(AWAR_SPECIES_NAME)->write_string(species_name);
        }
    }

    free(species_name);


    while (gbd && type == ADMVT_WWW){
        char *name = strdup("noname");
        GBDATA *gb_name = GB_find(gbd,"name",0,down_level);
        if (!gb_name) gb_name = GB_find(gbd,"group_name",0,down_level); // bad hack, should work
        if (gb_name){
            delete name;
            name = GB_read_string(gb_name);
        }
        GB_ERROR error = awt_openURL_by_gbd(nt.awr,gb_main,gbd, name);
        if (error) aw_message(error);
        break;
    }
    GB_pop_transaction(gb_main);
}

static int count_key_data_elements(GBDATA *gb_key_data) {
    int nitems  = 0;
    for (GBDATA *gb_cnt  = GB_find(gb_key_data,0,0,down_level);
         gb_cnt;
         gb_cnt = GB_find(gb_cnt,0,0,this_level|search_next))
    {
        ++nitems;
    }

    return nitems;
}

static void ad_list_reorder_cb(AW_window *aws, AW_CL cl_cbs1, AW_CL cl_cbs2) {
    GB_begin_transaction(gb_main);
    char     *source  = aws->get_root()->awar(AWAR_FIELD_REORDER_SOURCE)->read_string();
    char     *dest    = aws->get_root()->awar(AWAR_FIELD_REORDER_DEST)->read_string();
    GB_ERROR  warning = 0;

    const adawcbstruct     *cbs1        = (const adawcbstruct*)cl_cbs1;
    const adawcbstruct     *cbs2        = (const adawcbstruct*)cl_cbs2;
    const ad_item_selector *selector    = cbs1->selector;
    GBDATA                 *gb_source   = awt_get_key(gb_main,source, selector->change_key_path);
    GBDATA                 *gb_dest     = awt_get_key(gb_main,dest, selector->change_key_path);

    int left_index  = aws->get_index_of_current_element(cbs1->id, AWAR_FIELD_REORDER_SOURCE);
    int right_index = aws->get_index_of_current_element(cbs2->id, AWAR_FIELD_REORDER_DEST);

    if (!gb_source) {
        aw_message("Please select an item you want to move (left box)");
    }
    else if (!gb_dest) {
        aw_message("Please select a destination where to place your item (right box)");
    }
    else if (gb_dest !=gb_source) {
        nt_assert(left_index != right_index);

        GBDATA  *gb_key_data = GB_search(gb_main, selector->change_key_path, GB_CREATE_CONTAINER);
        int      nitems      = count_key_data_elements(gb_key_data);
        GBDATA **new_order   = new GBDATA *[nitems];

        nitems = 0;

        for (GBDATA *gb_key  = GB_find(gb_key_data,0,0,down_level);
             gb_key;
             gb_key = GB_find(gb_key,0,0,this_level|search_next))
        {
            if (gb_key == gb_source) continue;
            new_order[nitems++] = gb_key;
            if (gb_key == gb_dest) {
                new_order[nitems++] = gb_source;
            }
        }
        warning = GB_resort_data_base(gb_main,new_order,nitems);
        delete [] new_order;

        if (left_index>right_index) { left_index++; right_index++; } // in one case increment indices
    }

    free(source);
    free(dest);

    GB_commit_transaction(gb_main);

    if (warning) {
        aw_message(warning);
    }
    else {
        aws->select_index(cbs1->id, AWAR_FIELD_REORDER_SOURCE, left_index);
        aws->select_index(cbs2->id, AWAR_FIELD_REORDER_DEST, right_index);
    }
}

static void ad_list_reorder_cb2(AW_window *aws, AW_CL cl_cbs2, AW_CL cl_dir) {
    GB_begin_transaction(gb_main);
    int                 dir  = (int)cl_dir;
    const adawcbstruct *cbs2 = (const adawcbstruct*)cl_cbs2;

    GB_ERROR warning = 0;

    char                   *field_name = aws->get_root()->awar(AWAR_FIELD_REORDER_DEST)->read_string();
    const ad_item_selector *selector   = cbs2->selector;
    GBDATA                 *gb_field   = awt_get_key(gb_main, field_name, selector->change_key_path);

    if (!gb_field) {
        warning = "Please select an item to move (right box)";
    }
    else {
        GBDATA  *gb_key_data = GB_search(gb_main, selector->change_key_path, GB_CREATE_CONTAINER);
        int      nitems      = count_key_data_elements(gb_key_data);
        GBDATA **new_order   = new GBDATA *[nitems];

        nitems         = 0;
        int curr_index = -1;

        for (GBDATA *gb_key = GB_find(gb_key_data, 0, 0, down_level);
             gb_key;
             gb_key = GB_find(gb_key, 0, 0, this_level|search_next))
        {
            if (gb_key == gb_field) curr_index = nitems;
            new_order[nitems++] = gb_key;
        }

        nt_assert(curr_index != -1);
        int new_index = curr_index+dir;

        if (new_index<0 || new_index > nitems) {
            warning = GBS_global_string("Illegal target index '%i'", new_index);
        }
        else {
            if (new_index<curr_index) {
                for (int i = curr_index; i>new_index; --i) new_order[i] = new_order[i-1];
                new_order[new_index] = gb_field;
            }
            else if (new_index>curr_index) {
                for (int i = curr_index; i<new_index; ++i) new_order[i] = new_order[i+1];
                new_order[new_index] = gb_field;
            }

            warning = GB_resort_data_base(gb_main,new_order,nitems);
        }

        delete [] new_order;
    }

    GB_commit_transaction(gb_main);
    if (warning) aw_message(warning);
}

AW_window *NT_create_ad_list_reorder(AW_root *root, AW_CL cl_item_selector)
{
    const ad_item_selector *selector = (const ad_item_selector*)cl_item_selector;

    static AW_window_simple *awsa[AWT_QUERY_ITEM_TYPES];
    if (awsa[selector->type]) return (AW_window *)awsa[selector->type];

    AW_window_simple *aws = new AW_window_simple;
    awsa[selector->type]  = aws;

    aws->init( root, "REORDER_FIELDS", "REORDER FIELDS");
    aws->load_xfig("ad_kreo.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    AW_CL cbs1 = awt_create_selection_list_on_scandb(gb_main, (AW_window*)aws, AWAR_FIELD_REORDER_SOURCE, AWT_NDS_FILTER, "source", 0, selector, 20, 10);
    AW_CL cbs2 = awt_create_selection_list_on_scandb(gb_main, (AW_window*)aws, AWAR_FIELD_REORDER_DEST,   AWT_NDS_FILTER, "dest",   0, selector, 20, 10);

    aws->button_length(0);

    aws->at("doit");
    aws->callback(ad_list_reorder_cb, cbs1, cbs2);
    aws->help_text("spaf_reorder.hlp");
    aws->create_button("MOVE_LEFT_BEHIND_RIGHT", "MOVE LEFT\nBEHIND RIGHT","L");

    aws->at("doit2");
    aws->callback(ad_list_reorder_cb2, cbs2, -1);
    aws->help_text("spaf_reorder.hlp");
    aws->create_button("MOVE_UP_RIGHT", "MOVE RIGHT\nUP","U");

    aws->at("doit3");
    aws->callback(ad_list_reorder_cb2, cbs2, 1);
    aws->help_text("spaf_reorder.hlp");
    aws->create_button("MOVE_DOWN_RIGHT", "MOVE RIGHT\nDOWN","U");

    return (AW_window *)aws;
}

static void ad_hide_field(AW_window *aws, AW_CL cl_cbs, AW_CL cl_hide)
{
    AWUSE(aws);
    GB_begin_transaction(gb_main);

    char                   *source    = aws->get_root()->awar(AWAR_FIELD_DELETE)->read_string();
    GB_ERROR                error     = 0;
    const adawcbstruct     *cbs       = (const adawcbstruct*)cl_cbs;
    const ad_item_selector *selector  = cbs->selector;
    GBDATA                 *gb_source = awt_get_key(gb_main,source, selector->change_key_path);

    if (!gb_source) {
        aw_message("Please select the field you want to (un)hide");
    }
    else {
        GBDATA *gb_hidden = GB_search(gb_source, CHANGEKEY_HIDDEN, GB_INT);
        GB_write_int(gb_hidden, (int)cl_hide);
    }

    free(source);

    if (error) {
        GB_abort_transaction(gb_main);
        if (error) aw_message(error);
    }else{
        GB_commit_transaction(gb_main);
        aws->move_selection(cbs->id, AWAR_FIELD_DELETE, 1);
    }
}

static void ad_field_delete(AW_window *aws, AW_CL cl_cbs)
{
    AWUSE(aws);

    GB_begin_transaction(gb_main);

    char                   *source    = aws->get_root()->awar(AWAR_FIELD_DELETE)->read_string();
    GB_ERROR                error     = 0;
    const adawcbstruct     *cbs       = (const adawcbstruct*)cl_cbs;
    const ad_item_selector *selector  = cbs->selector;
    GBDATA                 *gb_source = awt_get_key(gb_main,source, selector->change_key_path);

    int curr_index = aws->get_index_of_current_element(cbs->id, AWAR_FIELD_DELETE);

    if (!gb_source) {
        aw_message("Please select the field you want to delete");
    }else{
        error = GB_delete(gb_source);
    }

    for (GBDATA *gb_item_container = selector->get_first_item_container(gb_main, aws->get_root(), AWT_QUERY_ALL_SPECIES);
         !error && gb_item_container;
         gb_item_container = selector->get_next_item_container(gb_item_container, AWT_QUERY_ALL_SPECIES))
    {
        for (GBDATA *gb_item = selector->get_first_item(gb_item_container);
             !error && gb_item;
             gb_item = selector->get_next_item(gb_item))
        {
            GBDATA *gbd = GB_search(gb_item,source,GB_FIND);
            if (gbd) error = GB_delete(gbd);
        }
    }

    free(source);

    if (error) {
        GB_abort_transaction(gb_main);
        if (error) aw_message(error);
    }else{
        GB_commit_transaction(gb_main);
    }

    aws->select_index(cbs->id, AWAR_FIELD_DELETE, curr_index); // current(deleted) item has disappeared, so this selects the next one
}

AW_window *NT_create_ad_field_delete(AW_root *root, AW_CL cl_item_selector)
{
    const ad_item_selector *selector = (const ad_item_selector*)cl_item_selector;

    static AW_window_simple *awsa[AWT_QUERY_ITEM_TYPES];
    if (awsa[selector->type]) return (AW_window *)awsa[selector->type];

    AW_window_simple *aws = new AW_window_simple;
    awsa[selector->type]  = aws;

    aws = new AW_window_simple;
    aws->init( root, "DELETE_FIELD", "DELETE FIELD");
    aws->load_xfig("ad_delof.fig");
    aws->button_length(6);

    aws->at("close");aws->callback( AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("help");aws->callback( AW_POPUP_HELP,(AW_CL)"spaf_delete.hlp");
    aws->create_button("HELP","HELP","H");

    AW_CL cbs = awt_create_selection_list_on_scandb(gb_main,
                                                    (AW_window*)aws,AWAR_FIELD_DELETE,
                                                    -1,
                                                    "source",0, selector, 20, 10,
                                                    false, false, true);

    aws->button_length(13);
    aws->at("hide");
    aws->callback(ad_hide_field, cbs, (AW_CL)1);
    aws->help_text("rm_field_only.hlp");
    aws->create_button("HIDE_FIELD","Hide field","H");

    aws->at("unhide");
    aws->callback(ad_hide_field, cbs, (AW_CL)0);
    aws->help_text("rm_field_only.hlp");
    aws->create_button("UNHIDE_FIELD","Unhide field","U");

    aws->at("delf");
    aws->callback(ad_field_delete, cbs);
    aws->help_text("rm_field_cmpt.hlp");
    aws->create_button("DELETE_FIELD", "DELETE FIELD\n(DATA DELETED)","C");

    return (AW_window *)aws;
}

static void ad_field_create_cb(AW_window *aws, AW_CL cl_item_selector)
{
    GB_push_transaction(gb_main);
    char *name = aws->get_root()->awar(AWAR_FIELD_CREATE_NAME)->read_string();
    GB_ERROR error = GB_check_key(name);
    GB_ERROR error2 = GB_check_hkey(name);
    if (error && !error2) {
        aw_message("Warning: Your key contain a '/' character,\n"
                   "    that means it is a hierarchical key");
        error = 0;
    }

    int type = (int)aws->get_root()->awar(AWAR_FIELD_CREATE_TYPE)->read_int();

    const ad_item_selector *selector = (const ad_item_selector*)cl_item_selector;
    if(!error) error = awt_add_new_changekey_to_keypath(gb_main, name, type, selector->change_key_path);
    if (error) {
        aw_message(error);
    }else{
        aws->hide();
    }
    free(name);
    GB_pop_transaction(gb_main);
}

AW_window *NT_create_ad_field_create(AW_root *root, AW_CL cl_item_selector)
{
    const ad_item_selector *selector = (const ad_item_selector*)cl_item_selector;

    static AW_window_simple *awsa[AWT_QUERY_ITEM_TYPES];
    if (awsa[selector->type]) return (AW_window *)awsa[selector->type];

    AW_window_simple *aws = new AW_window_simple;
    awsa[selector->type]  = aws;

    aws->init( root, "CREATE_FIELD","CREATE A NEW FIELD");
    aws->load_xfig("ad_fcrea.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("input");
    aws->label("FIELD NAME");
    aws->create_input_field(AWAR_FIELD_CREATE_NAME,15);

    aws->at("type");
    aws->create_toggle_field(AWAR_FIELD_CREATE_TYPE,"FIELD TYPE","F");
    aws->insert_toggle("Ascii Text","S",        (int)GB_STRING);
    aws->insert_toggle("Link","L",          (int)GB_LINK);
    aws->insert_toggle("Rounded Numerical","N", (int)GB_INT);
    aws->insert_toggle("Numerical","R",     (int)GB_FLOAT);
    aws->insert_toggle("MASK = 01 Text","0",    (int)GB_BITS);
    aws->update_toggle_field();

    aws->at("ok");
    aws->callback(ad_field_create_cb, cl_item_selector);
    aws->create_button("CREATE","CREATE","C");

    return (AW_window *)aws;
}

void ad_spec_create_field_items(AW_window *aws) {
    aws->insert_menu_topic("reorder_fields", "Reorder fields ...",     "R", "spaf_reorder.hlp", AD_F_ALL, AW_POPUP, (AW_CL)NT_create_ad_list_reorder, (AW_CL)&AWT_species_selector); 
    aws->insert_menu_topic("delete_field",   "Delete/Hide fields ...", "D", "spaf_delete.hlp",  AD_F_ALL, AW_POPUP, (AW_CL)NT_create_ad_field_delete, (AW_CL)&AWT_species_selector); 
    aws->insert_menu_topic("create_field",   "Create fields ...",      "C", "spaf_create.hlp",  AD_F_ALL, AW_POPUP, (AW_CL)NT_create_ad_field_create, (AW_CL)&AWT_species_selector); 
    aws->insert_separator(); 
    aws->insert_menu_topic("unhide_fields", "Show all hidden fields", "S", "scandb.hlp", AD_F_ALL, (AW_CB)awt_selection_list_unhide_all_cb, (AW_CL)gb_main, AWT_NDS_FILTER); 
    aws->insert_separator(); 
    aws->insert_menu_topic("scan_unknown_fields", "Scan unknown fields",   "u", "scandb.hlp", AD_F_ALL, (AW_CB)awt_selection_list_scan_unknown_cb,  (AW_CL)gb_main, AWT_NDS_FILTER); 
    aws->insert_menu_topic("del_unused_fields",   "Remove unused fields",  "e", "scandb.hlp", AD_F_ALL, (AW_CB)awt_selection_list_delete_unused_cb, (AW_CL)gb_main, AWT_NDS_FILTER); 
    aws->insert_menu_topic("refresh_fields",      "Refresh fields (both)", "f", "scandb.hlp", AD_F_ALL, (AW_CB)awt_selection_list_update_cb,        (AW_CL)gb_main, AWT_NDS_FILTER); 
}

#include <probe_design.hxx>

static void awtc_nn_search_all_listed(AW_window *aww,AW_CL _cbs) {
    struct adaqbsstruct *cbs        = (struct adaqbsstruct *)_cbs;
    nt_assert(cbs->selector->type == AWT_QUERY_ITEM_SPECIES);
    GB_begin_transaction(gb_main);
    int                  pts        = aww->get_root()->awar(AWAR_PROBE_ADMIN_PT_SERVER)->read_int();
    char                *dest_field = aww->get_root()->awar("next_neighbours/dest_field")->read_string();
    char                *ali_name   = aww->get_root()->awar(AWAR_DEFAULT_ALIGNMENT)->read_string();
    GB_ERROR             error      = 0;
    GB_TYPES             dest_type  = awt_get_type_of_changekey(gb_main,dest_field, CHANGE_KEY_PATH);
    if (!dest_type){
        error = GB_export_error("Please select a valid field");
    }
    long max = awt_count_queried_species(cbs);

    if (strcmp(dest_field, "name")==0) {
        int answer = aw_message("CAUTION! This will destroy all name-fields of the listed species.\n",
                                "Continue and destroy all name-fields,Abort");

        if (answer==1) {
            error = GB_export_error("Aborted by user");
        }
    }

    aw_openstatus("Finding next neighbours");
    long count = 0;
    GBDATA *gb_species;
    for (gb_species = GBT_first_species(gb_main);
         !error && gb_species;
         gb_species = GBT_next_species(gb_species)){
        if (!IS_QUERIED(gb_species,cbs)) continue;
        count ++;
        GBDATA *gb_data = GBT_read_sequence(gb_species,ali_name);
        if (!gb_data)   continue;
        if (count %10 == 0){
            GBDATA *gb_name = GB_search(gb_species,"name",GB_STRING);
            aw_status(GBS_global_string("Species '%s' (%li:%li)",
                                        GB_read_char_pntr(gb_name),
                                        count, max));
        }
        if (aw_status(count/(double)max)){
            error = "operation aborted";
            break;
        }
        {
            char *sequence = GB_read_string(gb_data);
            AWTC_FIND_FAMILY ff(gb_main);
            error = ff.go(pts,sequence,GB_TRUE,1);
            if (error) break;
            {
                AWTC_FIND_FAMILY_MEMBER *fm = ff.family_list;
                const char *value;
                if (fm){
                    value = GBS_global_string("%li '%s'",fm->matches,fm->name);
                }else{
                    value = "0";
                }
                GBDATA *gb_dest = GB_search(gb_species,dest_field,dest_type);
                error = GB_write_as_string(gb_dest,value);
            }
            delete sequence;
        }
    }
    aw_closestatus();
    if (error){
        GB_abort_transaction(gb_main);
        aw_message(error);
    }else{
        GB_commit_transaction(gb_main);
    }
    delete dest_field;
    delete ali_name;
}

static void awtc_nn_search(AW_window *aww,AW_CL id) { 
    GB_transaction dummy(gb_main);
    int pts = aww->get_root()->awar(AWAR_PROBE_ADMIN_PT_SERVER)->read_int();
    int max_hits = aww->get_root()->awar("next_neighbours/max_hits")->read_int();
    char *sequence = 0;
    {
        char *sel_species =     aww->get_root()->awar(AWAR_SPECIES_NAME)->read_string();
        GBDATA *gb_species =    GBT_find_species(gb_main,sel_species);

        if (!gb_species){
            delete sel_species;
            aw_message("Select a species first");
            return;
        }
        char *ali_name = aww->get_root()->awar(AWAR_DEFAULT_ALIGNMENT)->read_string();
        GBDATA *gb_data = GBT_read_sequence(gb_species,ali_name);
        if (!gb_data){
            aw_message(GBS_global_string("Species '%s' has no sequence '%s'",sel_species,ali_name));
        }
        free(sel_species);
        free(ali_name);
        if (!gb_data) return;
        sequence = GB_read_string(gb_data);
    }

    AWTC_FIND_FAMILY ff(gb_main);
    GB_ERROR error = ff.go(pts,sequence,GB_TRUE,max_hits);
    free(sequence);
    AW_selection_list* sel = (AW_selection_list *)id;
    aww->clear_selection_list(sel);
    if (error) {
        aw_message(error);
        aww->insert_default_selection(sel,"No hits found","");
    }else{
        AWTC_FIND_FAMILY_MEMBER *fm;
        for (fm = ff.family_list; fm; fm = fm->next){
            const char *dis = GBS_global_string("%-20s Score:%4li",fm->name,fm->matches);
            aww->insert_selection(sel,(char *)dis,fm->name);
        }

        aww->insert_default_selection(sel,"No more hits","");
    }
    aww->update_selection_list(sel);
}

static void awtc_move_hits(AW_window *,AW_CL id, AW_CL cbs){
    awt_copy_selection_list_2_queried_species((struct adaqbsstruct *)cbs,(AW_selection_list *)id);
}

static AW_window *ad_spec_next_neighbours_listed_create(AW_root *aw_root,AW_CL cbs){
    static AW_window_simple *aws = 0;
    if (aws){
        return (AW_window *)aws;
    }
    aw_root->awar_int(AWAR_PROBE_ADMIN_PT_SERVER);
    aw_root->awar_string("next_neighbours/dest_field","tmp");

    aws = new AW_window_simple;
    aws->init( aw_root, "SEARCH_NEXT_RELATIVES_OF_LISTED", "Search Next Neighbours of Listed");
    aws->load_xfig("ad_spec_nnm.fig");

    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"next_neighbours_marked.hlp");
    aws->create_button("HELP","HELP","H");

    aws->at("pt_server");
    awt_create_selection_list_on_pt_servers(aws, AWAR_PROBE_ADMIN_PT_SERVER, AW_TRUE);

    aws->at("field");
    awt_create_selection_list_on_scandb(gb_main,aws,"next_neighbours/dest_field",
                                        (1<<GB_INT) | (1<<GB_STRING), "field",0, &AWT_species_selector, 20, 10);


    aws->at("go");
    aws->callback(awtc_nn_search_all_listed,cbs);
    aws->create_button("SEARCH","SEARCH");

    return (AW_window *)aws;
}

static AW_window *ad_spec_next_neighbours_create(AW_root *aw_root,AW_CL cbs){
    static AW_window_simple *aws = 0;
    if (aws){
        return (AW_window *)aws;
    }
    aw_root->awar_int(AWAR_PROBE_ADMIN_PT_SERVER);
    aw_root->awar_int("next_neighbours/max_hits",20);

    aws = new AW_window_simple;
    aws->init( aw_root, "SEARCH_NEXT_RELATIVE_OF_SELECTED", "Search Next Neighbours");
    aws->load_xfig("ad_spec_nn.fig");

    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"next_neighbours.hlp");
    aws->create_button("HELP","HELP","H");

    aws->at("pt_server");
    awt_create_selection_list_on_pt_servers(aws, AWAR_PROBE_ADMIN_PT_SERVER, AW_TRUE);

    aws->at("max_hit");
    aws->create_input_field("next_neighbours/max_hits",5);

    aws->at("hits");
    AW_selection_list *id = aws->create_selection_list(AWAR_SPECIES_NAME);
    aws->insert_default_selection(id,"No hits found","");
    aws->update_selection_list(id);

    aws->at("go");
    aws->callback(awtc_nn_search,(AW_CL)id);
    aws->create_button("SEARCH","SEARCH");

    aws->at("move");
    aws->callback(awtc_move_hits,(AW_CL)id,cbs);
    aws->create_button("MOVE_TO_HITLIST","MOVE TO HITLIST");

    return (AW_window *)aws;
}

// -----------------------------------------------------------------------------------------------------------------
//      void NT_detach_information_window(AW_window *aww, AW_CL cl_pointer_to_aww, AW_CL cl_Awar_Callback_Info)
// -----------------------------------------------------------------------------------------------------------------

void NT_detach_information_window(AW_window *aww, AW_CL cl_pointer_to_aww, AW_CL cl_AW_detach_information) {
    AW_window **aww_pointer = (AW_window**)cl_pointer_to_aww;

    AW_detach_information *di           = (AW_detach_information*)cl_AW_detach_information;
    Awar_Callback_Info    *cb_info      = di->get_cb_info();
    AW_root               *awr          = cb_info->get_root();
    char                  *curr_species = awr->awar(cb_info->get_org_awar_name())->read_string();

    if (*aww_pointer == aww) {  // first click on detach-button
        // create unique awar :
        static int detach_counter = 0;
        char       new_awar[100];
        sprintf(new_awar, "tmp/DETACHED_INFO_%i", detach_counter++);
        awr->awar_string(new_awar, "", AW_ROOT_DEFAULT);

        cb_info->remap(new_awar); // remap the callback from old awar to new unique awar
        aww->update_label((int*)di->get_detach_button(), "GET");

        *aww_pointer = 0;       // caller window will be recreated on next open after clearing this pointer
        // [Note : the aww_pointer points to the static window pointer]
    }

    awr->awar(cb_info->get_awar_name())->write_string(curr_species);
    aww->set_window_title(GBS_global_string("%s INFORMATION", curr_species));
    free(curr_species);
}

AW_window *create_speciesOrganismWindow(AW_root *aw_root, bool organismWindow)
{
    int windowIdx = (int)organismWindow;

    static AW_window_simple_menu *AWS[2] = { 0, 0 };
    if (AWS[windowIdx]) {
        return (AW_window *)AWS[windowIdx]; // already created (and not detached)
    }

    AW_window_simple_menu *& aws = AWS[windowIdx];

    aws = new AW_window_simple_menu;
    if (organismWindow) aws->init( aw_root, "ORGANISM_INFORMATION", "ORGANISM INFORMATION");
    else                aws->init( aw_root, "SPECIES_INFORMATION", "SPECIES INFORMATION");
    aws->load_xfig("ad_spec.fig");

    aws->button_length(8);

    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("search");
    aws->callback(AW_POPUP, (AW_CL)ad_create_query_window, 0);
    aws->create_button("SEARCH","SEARCH","S");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"sp_info.hlp");
    aws->create_button("HELP","HELP","H");

    AW_CL scannerid = awt_create_arbdb_scanner(gb_main, aws, "box",0,"field","enable",AWT_VIEWER,0,"mark",AWT_NDS_FILTER,
                                               organismWindow ? &AWT_organism_selector : &AWT_species_selector);
    ad_global_scannerid = scannerid;
    ad_global_scannerroot = aws->get_root();

    if (organismWindow) aws->create_menu(0,   "ORGANISM",     "O", "spa_organism.hlp",  AD_F_ALL );
    else                aws->create_menu(0,   "SPECIES",     "S", "spa_species.hlp",  AD_F_ALL );

    aws->insert_menu_topic("species_delete",        "Delete",        "D","spa_delete.hlp",   AD_F_ALL,   ad_species_delete_cb, 0, 0);
    aws->insert_menu_topic("species_rename",        "Rename",        "R","spa_rename.hlp",   AD_F_ALL,   ad_species_rename_cb, 0, 0);
    aws->insert_menu_topic("species_copy",          "Copy",          "y","spa_copy.hlp", AD_F_ALL,   ad_species_copy_cb, 0, 0);
    aws->insert_menu_topic("species_create",        "Create",        "C","spa_create.hlp",   AD_F_ALL,   AW_POPUP, (AW_CL)create_species_create_window, 0);
    aws->insert_menu_topic("species_convert_2_sai", "Convert to SAI","S","sp_sp_2_ext.hlp",AD_F_ALL,    (AW_CB)move_species_to_extended, 0, 0);
    aws->insert_separator();

    aws->create_menu(       0,   "FIELDS",     "F", "spa_fields.hlp",  AD_F_ALL );
    ad_spec_create_field_items(aws);

    {
        const char         *awar_name = (bool)organismWindow ? AWAR_ORGANISM_NAME : AWAR_SPECIES_NAME;
        AW_root            *awr       = aws->get_root();
        Awar_Callback_Info *cb_info   = new Awar_Callback_Info(awr, awar_name, AD_map_species, (AW_CL)scannerid, (AW_CL)organismWindow); // do not delete!
        cb_info->add_callback();

        AW_detach_information *detach_info = new AW_detach_information(cb_info); // do not delete!

        aws->at("detach");
        aws->callback(NT_detach_information_window, (AW_CL)&aws, (AW_CL)detach_info);
        aws->create_button("DETACH", "DETACH", "D");

        detach_info->set_detach_button(aws->get_last_button_widget());
    }

    aws->show();
    AD_map_species(aws->get_root(),scannerid, (AW_CL)organismWindow);
    return (AW_window *)aws;
}

AW_window *NT_create_species_window(AW_root *aw_root) {
    return create_speciesOrganismWindow(aw_root, false);
}
AW_window *NT_create_organism_window(AW_root *aw_root) {
    return create_speciesOrganismWindow(aw_root, true);
}

AW_CL ad_query_global_cbs = 0;

void ad_unquery_all(){
    awt_unquery_all(0,(struct adaqbsstruct *)ad_query_global_cbs);
}

void ad_query_update_list(){
    awt_query_update_list(NULL,(struct adaqbsstruct *)ad_query_global_cbs);
}

AW_window *ad_create_query_window(AW_root *aw_root)
{
    static AW_window_simple_menu *aws = 0;
    if (aws){
        return (AW_window *)aws;
    }
    aws = new AW_window_simple_menu;
    aws->init( aw_root, "SPECIES_QUERY", "SEARCH and QUERY");
    aws->create_menu(0,"More functions","f");
    aws->load_xfig("ad_query.fig");


    awt_query_struct awtqs;

    awtqs.gb_main             = gb_main;
    awtqs.species_name        = AWAR_SPECIES_NAME;
    awtqs.tree_name           = AWAR_TREE;
    awtqs.select_bit          = 1;
    awtqs.use_menu            = 1;
    awtqs.ere_pos_fig         = "ere2";
    awtqs.by_pos_fig          = "by2";
    awtqs.qbox_pos_fig        = "qbox";
    awtqs.rescan_pos_fig      = 0;
    awtqs.key_pos_fig         = 0;
    awtqs.query_pos_fig       = "content";
    awtqs.result_pos_fig      = "result";
    awtqs.count_pos_fig       = "count";
    awtqs.do_query_pos_fig    = "doquery";
    awtqs.config_pos_fig      = "doconfig";
    awtqs.do_mark_pos_fig     = "domark";
    awtqs.do_unmark_pos_fig   = "dounmark";
    awtqs.do_delete_pos_fig   = "dodelete";
    awtqs.do_set_pos_fig      = "doset";
    awtqs.do_refresh_pos_fig  = "dorefresh";
    awtqs.open_parser_pos_fig = "openparser";
    awtqs.create_view_window  = (AW_CL)NT_create_species_window;
    awtqs.selector            = &AWT_species_selector;

    AW_CL cbs           = (AW_CL)awt_create_query_box((AW_window*)aws,&awtqs);
    ad_query_global_cbs = cbs;

    aws->create_menu(       0,   "More search",     "s" );
    aws->insert_menu_topic( "search_equal_fields_within_db","Search For Equal Fields and Mark Duplikates",          "E",0,  -1, (AW_CB)awt_search_equal_entries, cbs, 0 );
    aws->insert_menu_topic( "search_equal_words_within_db", "Search For Equal Words Between Fields and Mark Duplikates",    "W",0,  -1, (AW_CB)awt_search_equal_entries, cbs, 1 );
    aws->insert_menu_topic( "search_next_relativ_of_sel",   "Search Next Relatives of SELECTED Species in PT_Server ...",   "R",0,  -1, (AW_CB)AW_POPUP,    (AW_CL)ad_spec_next_neighbours_create, cbs );
    aws->insert_menu_topic( "search_next_relativ_of_listed","Search Next Relatives of LISTED Species in PT_Server ...",     "L",0,  -1, (AW_CB)AW_POPUP,    (AW_CL)ad_spec_next_neighbours_listed_create, cbs );

    aws->button_length(7);

    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("help");
    aws->callback( AW_POPUP_HELP,(AW_CL)"sp_search.hlp");
    aws->create_button("HELP","HELP","H");

    return (AW_window *)aws;
}
