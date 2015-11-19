// ==================================================================== //
//                                                                      //
//   File      : item_sel_list.cxx                                      //
//   Purpose   : selection lists for items (ItemSelector)               //
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
#include <arb_global_defs.h>

#if !defined(ARB_GTK)
static AW_window *existing_window_creator(AW_root*, AW_window *aw_existing) {
    return aw_existing;
}
#endif

Itemfield_Selection::Itemfield_Selection(AW_selection_list *sellist_,
                                         GBDATA            *gb_key_data,
                                         long               type_filter_,
                                         SelectedFields     field_filter_,
                                         ItemSelector&      selector_)
    : AW_DB_selection(sellist_, gb_key_data)
    , type_filter(type_filter_)
    , field_filter(field_filter_)
    , selector(selector_)
{
    it_assert(&selector);
}

void Itemfield_Selection::fill() {
    if (field_filter & SF_PSEUDO) {
        insert(PSEUDO_FIELD_ANY_FIELD, PSEUDO_FIELD_ANY_FIELD);
        insert(PSEUDO_FIELD_ALL_FIELDS, PSEUDO_FIELD_ALL_FIELDS);
    }

    GBDATA         *gb_key_data = get_gbd();
    GB_transaction  ta(gb_key_data);

    for (GBDATA *gb_key = GB_entry(gb_key_data, CHANGEKEY); gb_key; gb_key = GB_nextEntry(gb_key)) {
        GBDATA *gb_key_type = GB_entry(gb_key, CHANGEKEY_TYPE);

        if (shall_display_type(GB_read_int(gb_key_type))) {
            GBDATA *gb_key_name = GB_entry(gb_key, CHANGEKEY_NAME);

            if (gb_key_name) {
                const char *name = GB_read_char_pntr(gb_key_name);
                if (!name) {
                    fprintf(stderr, "WARNING: can't read key name (Reason: %s)", GB_await_error());
                    name = "<unnamedKey?>";
                }

                long       *hiddenPtr = GBT_read_int(gb_key, CHANGEKEY_HIDDEN);
                const char *display   = 0;

                if (!hiddenPtr) {               // it's an older db version w/o hidden flag -> add it
                    GB_ERROR error = GBT_write_int(gb_key, CHANGEKEY_HIDDEN, 0); // default is "not hidden"
                    if (error) GB_warningf("WARNING: can't create " CHANGEKEY_HIDDEN " (Reason: %s)\n", error);

                    static long not_hidden = 0;
                    hiddenPtr              = &not_hidden;
                }

                if (*hiddenPtr) {               // hidden ?
                    if (field_filter & SF_HIDDEN) { // show hidden fields ?
                        display = GBS_global_string("[hidden] %s", name);
                    }
                }
                else display = name;

                if (display) insert(display, name);
            }
        }
    }
    insert_default("<no field>", NO_FIELD_SELECTED);
}


Itemfield_Selection *create_selection_list_on_itemfields(GBDATA         *gb_main,
                                                         AW_window      *aws,
                                                         const char     *varname,
                                                         bool            fallback2default,
                                                         long            type_filter,
                                                         const char     *scan_xfig_label,
                                                         const char     *rescan_xfig_label,
                                                         ItemSelector&   selector,
                                                         size_t          columns,
                                                         size_t          visible_rows,
                                                         SelectedFields  field_filter,
                                                         const char     *popup_button_id) // @@@ button id is not needed - only causes xtra macro command
{
    /* show fields of a item (e.g. species, SAI, gene)
     * 'varname'                is the awar set by the selection list
     * 'fallback2default'       whether awar value shall be reset to default (see create_option_menu/create_selection_list)
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

    GBDATA *gb_key_data;
    {
        GB_transaction ta(gb_main);
        gb_key_data = GB_search(gb_main, selector.change_key_path, GB_CREATE_CONTAINER);
    }

    AW_selection_list *sellist = 0;

    if (scan_xfig_label) aws->at(scan_xfig_label);

    if (popup_button_id) {
#ifdef ARB_GTK
        aws->button_length(columns);
        sellist = aws->create_option_menu(varname, fallback2default);
#else
        // create HIDDEN popup window containing the selection list
        AW_window *win_for_sellist = aws;
        {
            AW_window_simple *aw_popup = new AW_window_simple;
            aw_popup->init(aws->get_root(), "SELECT_LIST_ENTRY", "SELECT AN ENTRY");

            aw_popup->auto_space(10, 10);
            aw_popup->at_newline();

            aw_popup->callback((AW_CB0)AW_POPDOWN);
            sellist = aw_popup->create_selection_list(varname, columns, visible_rows, fallback2default);

            aw_popup->at_newline();
            aw_popup->callback((AW_CB0)AW_POPDOWN);
            aw_popup->create_button("CLOSE", "CLOSE", "C");

            aw_popup->window_fit();
            aw_popup->recalc_pos_atShow(AW_REPOS_TO_MOUSE);

            win_for_sellist = aw_popup;
        }

        // and bind hidden window popup to button
        aws->button_length(columns);
        aws->callback(makeCreateWindowCallback(existing_window_creator, win_for_sellist)); 
        aws->create_button(popup_button_id, varname);
#endif

    }
    else { // otherwise just insert the selection list at point
        sellist = aws->create_selection_list(varname, columns, visible_rows, fallback2default);
    }

    if (rescan_xfig_label) {
        int x, y;
        aws->get_at_position(&x, &y);

        aws->at(rescan_xfig_label);
        aws->callback(makeWindowCallback(selector.selection_list_rescan_cb, gb_main, FIELD_UNFILTERED));
        aws->create_button(rescan_xfig_label, "RESCAN", "R");

        if (popup_button_id) aws->at(x, y); // restore 'at' position if popup_list_in_window
    }

    Itemfield_Selection *selection = new Itemfield_Selection(sellist, gb_key_data, type_filter, field_filter, selector);
    selection->refresh();

#ifdef ARB_GTK
    if(popup_button_id) {
        aws->update_option_menu();
    }
#endif
    return selection;
}

