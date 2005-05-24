// ==================================================================== //
//                                                                      //
//   File      : AWT_item_sel_list.cxx                                  //
//   Purpose   : selection lists for items (ad_item_selector)           //
//   Time-stamp: <Mon May/23/2005 20:04 MET Coder@ReallySoft.de>        //
//                                                                      //
//                                                                      //
// Coded by Ralf Westram (coder@reallysoft.de) in May 2005              //
// Copyright Department of Microbiology (Technical University Munich)   //
//                                                                      //
// Visit our web site at: http://www.arb-home.de/                       //
//                                                                      //
// ==================================================================== //

#include <string.h>

#include <aw_awars.hxx>

#include "awt.hxx"
#include "awtlocal.hxx"
#include "awt_item_sel_list.hxx"

static AW_window *awt_existing_window(AW_window *, AW_CL cl1, AW_CL) {
    return (AW_window*)cl1;
}

static void awt_create_selection_list_on_scandb_cb(GBDATA *dummy, struct adawcbstruct *cbs)
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
    
    for (gb_key = GB_find(gb_key_data,CHANGEKEY,0,down_level);
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

    struct adawcbstruct *cbs = new adawcbstruct;
    memset(cbs, 0, sizeof(*cbs));

    cbs->aws                         = win_for_sellist;
    cbs->awr                         = win_for_sellist->get_root();
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





