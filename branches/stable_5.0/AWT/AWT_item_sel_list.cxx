// ==================================================================== //
//                                                                      //
//   File      : AWT_item_sel_list.cxx                                  //
//   Purpose   : selection lists for items (ad_item_selector)           //
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

#include <arbdbt.h>

static AW_window *awt_existing_window(AW_window *, AW_CL cl1, AW_CL) {
    return (AW_window*)cl1;
}

static void populate_selection_list_on_scandb_cb(GBDATA *dummy, struct adawcbstruct *cbs)
{
    GBDATA *gb_key_data;
    gb_key_data = GB_search(cbs->gb_main, cbs->selector->change_key_path, GB_CREATE_CONTAINER);
    AWUSE(dummy);

    cbs->aws->clear_selection_list(cbs->id);

    if (cbs->add_pseudo_fields) {
        cbs->aws->insert_selection(cbs->id, PSEUDO_FIELD_ANY_FIELD, PSEUDO_FIELD_ANY_FIELD);
        cbs->aws->insert_selection(cbs->id, PSEUDO_FIELD_ALL_FIELDS, PSEUDO_FIELD_ALL_FIELDS);
    }

    for (GBDATA *gb_key = GB_entry(gb_key_data,CHANGEKEY); gb_key; gb_key = GB_nextEntry(gb_key)) {
        GBDATA *gb_key_type = GB_entry(gb_key,CHANGEKEY_TYPE);
        if ( !( ((long)cbs->def_filter) & (1<<GB_read_int(gb_key_type)))) continue; // type does not match filter

        GBDATA *gb_key_name = GB_entry(gb_key,CHANGEKEY_NAME);
        if (!gb_key_name) continue;                 // key w/o name -> don't show

        const char *name = GB_read_char_pntr(gb_key_name);
        if (!name) {
            fprintf(stderr, "WARNING: can't read key name (Reason: %s)", GB_await_error());
            name = "<unnamedKey?>";
        }

        long       *hiddenPtr = GBT_read_int(gb_key, CHANGEKEY_HIDDEN);
        const char *display   = 0;

        if (!hiddenPtr) {                           // it's an older db version w/o hidden flag -> add it
            GB_ERROR error = GBT_write_int(gb_key, CHANGEKEY_HIDDEN, 0); // default is "not hidden"
            if (error) {
                GB_warningf("WARNING: can't create " CHANGEKEY_HIDDEN " (Reason: %s)\n", error);
            }

            static long not_hidden = 0;;
            hiddenPtr              = &not_hidden;
        }

        if (*hiddenPtr) { // hidden ?
            if (cbs->include_hidden_fields) {       // show hidden fields ?
                display = GBS_global_string("[hidden] %s", name);
            }
        }
        else display = name;

        if (display) cbs->aws->insert_selection(cbs->id, display, name);
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
                                          awt_selected_fields     field_filter,
                                          const char             *popup_button_id)
{
    /* show fields of a item (e.g. species, SAI, gene)
     * 'varname'                is the awar set by the selection list
     * 'type_filter'            is a bitstring which controls what types are shown in the selection list
     *                          (e.g '1<<GB_INT || 1 <<GB_STRING' enables ints and strings)
     * 'scan_xfig_label'        is the position of the selection box (or selection button)
     * 'rescan_xfig_label'      if not NULL, a 'RESCAN' button is added at that position
     * 'selector'               describes the item type, for which fields are shown
     * 'columns'/'visible_rows' specify the size of the selection list
     * 'field_filter'           controls if pseudo-fields and/or hidden fields are added
     * 'popup_button_id'        if not NULL, a button (with this id) is inserted.
     *                          When clicked a popup window containing the selection list opens.
     */


    AW_selection_list *id              = 0;
    GBDATA            *gb_key_data;
    AW_window         *win_for_sellist = aws;

    GB_push_transaction(gb_main);

    if (scan_xfig_label) aws->at(scan_xfig_label);

    if (popup_button_id) { 
        // create HIDDEN popup window containing the selection list
        {
            AW_window_simple *aw_popup = new AW_window_simple;
            aw_popup->init(aws->get_root(), "SELECT_LIST_ENTRY", "SELECT AN ENTRY");
            //         aw_popup->load_xfig(0, true);

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

        // and bind hidden window popup to button 
        aws->button_length(columns);
        aws->callback((AW_CB2)AW_POPUP,(AW_CL)awt_existing_window, (AW_CL)win_for_sellist);
        aws->create_button(popup_button_id, varname);

    }
    else { // otherwise just insert the selection list at point
        id = aws->create_selection_list(varname,0,"",columns,visible_rows); // 20,10);
    }

    struct adawcbstruct *cbs = new adawcbstruct;
    memset(cbs, 0, sizeof(*cbs));

    cbs->aws                   = win_for_sellist;
    cbs->awr                   = win_for_sellist->get_root();
    cbs->gb_main               = gb_main;
    cbs->id                    = id;
    cbs->def_filter            = (char *)type_filter;
    cbs->selector              = selector;
    cbs->add_pseudo_fields     = field_filter & AWT_SF_PSEUDO;
    cbs->include_hidden_fields = field_filter & AWT_SF_PSEUDO;

    if (rescan_xfig_label) {
        int x, y;
        aws->get_at_position(&x, &y);

        aws->at(rescan_xfig_label);
        aws->callback(selector->selection_list_rescan_cb, (AW_CL)cbs->gb_main,(AW_CL)-1);
        aws->create_button(rescan_xfig_label, "RESCAN","R");

        if (popup_button_id) aws->at(x, y); // restore 'at' position if popup_list_in_window
    }

    populate_selection_list_on_scandb_cb(0,cbs);

    gb_key_data = GB_search(gb_main, cbs->selector->change_key_path, GB_CREATE_CONTAINER);
    GB_add_callback(gb_key_data, GB_CB_CHANGED, (GB_CB)populate_selection_list_on_scandb_cb, (int *)cbs);

    GB_pop_transaction(gb_main);
    return (AW_CL)cbs;
}





