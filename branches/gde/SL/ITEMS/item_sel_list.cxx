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

#if defined(ARB_MOTIF)
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
#if defined(ARB_GTK)
        insert_default(PSEUDO_FIELD_ANY_FIELD, PSEUDO_FIELD_ANY_FIELD);
#else // ARB_MOTIF
        insert(PSEUDO_FIELD_ANY_FIELD, PSEUDO_FIELD_ANY_FIELD);
#endif
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

    if (!get_sellist()->get_default_value()) {
        insert_default("<no field>", NO_FIELD_SELECTED);
    }
}


Itemfield_Selection *create_selection_list_on_itemfields(GBDATA         *gb_main,
                                                         AW_window      *aws,
                                                         const char     *varname,
                                                         ItemSelector&   selector,
                                                         long            type_filter,
                                                         SelectedFields  field_filter,
                                                         const char     *scan_xfig_label,
                                                         const char     *popup_button_id)
{
    /* show fields of a item (e.g. species, SAI, gene)
     * 'varname'                is the awar set by the selection list
     * 'selector'               describes the item type, for which fields are shown
     * 'type_filter'            is a bitstring which controls what types are shown in the selection list
     *                          (e.g '1<<GB_INT || 1 <<GB_STRING' enables ints and strings)
     *                          Several filters are predefined: 'FIELD_FILTER_...', FIELD_UNFILTERED
     * 'field_filter'           controls if pseudo-fields and/or hidden fields are added
     * 'scan_xfig_label'        is the position of the selection box (or selection button)
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

    const bool FALLBACK2DEFAULT = true; // @@@ autodetect later (existing fields only->true, new fields allowed->false)

    const int COLUMNS = 20;
    const int ROWS    = 10;

    if (popup_button_id) {
        int old_button_length = aws->get_button_length();

#ifdef ARB_GTK
        aws->button_length(COLUMNS);
        sellist                    = aws->create_option_menu(varname, FALLBACK2DEFAULT);
#else // ARB_MOTIF
        // create HIDDEN popup window containing the selection list
        AW_window *win_for_sellist = aws;
        {
            AW_window_simple *aw_popup = new AW_window_simple;
            aw_popup->init(aws->get_root(), "SELECT_FIELD", "SELECT A FIELD");
            aw_popup->load_xfig("awt/field_sel.fig");

            aw_popup->at("sel");
            aw_popup->callback(AW_POPDOWN); // @@@ used as SELLIST_CLICK_CB (see #559)
            sellist = aw_popup->create_selection_list(varname, 1, 1, FALLBACK2DEFAULT);

            aw_popup->at("close");
            aw_popup->callback(AW_POPDOWN);
            aw_popup->create_button("CLOSE", "CLOSE", "C");

            aw_popup->recalc_pos_atShow(AW_REPOS_TO_MOUSE); // always popup at current mouse-position (i.e. directly above the button)
            aw_popup->recalc_size_atShow(AW_RESIZE_USER);   // if user changes the size of any field-selection popup, that size will be used for future popups

            win_for_sellist = aw_popup;
        }

        // and bind hidden window popup to button
        aws->button_length(COLUMNS); // @@@ use button length set by caller
        aws->callback(makeCreateWindowCallback(existing_window_creator, win_for_sellist)); 
        aws->create_button(popup_button_id, varname);
#endif
        aws->button_length(old_button_length);
    }
    else { // otherwise just insert the selection list at point
        sellist = aws->create_selection_list(varname, COLUMNS, ROWS, FALLBACK2DEFAULT); // @@@ expect at-to exists -> COLUMNS/ROWS does not matter
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

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

#include <arb_defs.h>

void TEST_lossless_conversion() {
    GB_shell  shell;
    GBDATA   *gb_main = GB_open("no.arb", "c");

    {
        GB_transaction ta(gb_main);

        const long tested_filter[] = {
            FIELD_FILTER_INT_WRITEABLE,
            FIELD_FILTER_BYTE_WRITEABLE,
            FIELD_FILTER_FLOAT_WRITEABLE,
        };

        for (unsigned f = 0; f<ARRAY_ELEMS(tested_filter); ++f) {
            const long filter = tested_filter[f];

            for (GB_TYPES type = GB_BYTE; type<=GB_STRING; type = GB_TYPES(type+1)) {
                if (type == GB_POINTER) continue;

                TEST_ANNOTATE(GBS_global_string("type=%i", int(type)));
                GBDATA *gb_field = GB_create(gb_main, "any", type);
                TEST_REJECT_NULL(gb_field);

                if (filter & (1<<type)) {
                    switch (filter) {
                        case FIELD_FILTER_INT_WRITEABLE: {
                            const int I_min(-2147483648);    // 32bit int min ..
                            const int I_max(2147483647);     // .. and max

                            int tested_int_value[] = { I_min, -1, 0, 1, I_max-1, I_max };
                            for (unsigned i = 0; i<ARRAY_ELEMS(tested_int_value); ++i) {
                                int written = tested_int_value[i];

                                TEST_ANNOTATE(GBS_global_string("type=%i INT written=%i", int(type), written));
                                TEST_EXPECT_NO_ERROR(GB_write_lossless_int(gb_field, written));

                                GB_ERROR error = NULL;
                                int      read  = GB_read_lossless_int(gb_field, error);

                                TEST_EXPECT_NO_ERROR(error);
                                TEST_EXPECT_EQUAL(written, read);
                            }
                            break;
                        }
                        case FIELD_FILTER_BYTE_WRITEABLE: {
                            uint8_t tested_byte_value[] = { 0, 1, 127, 128, 255 };
                            for (unsigned i = 0; i<ARRAY_ELEMS(tested_byte_value); ++i) {
                                uint8_t written = tested_byte_value[i];

                                TEST_ANNOTATE(GBS_global_string("type=%i BYTE written=%u", int(type), written));
                                TEST_EXPECT_NO_ERROR(GB_write_lossless_byte(gb_field, written));

                                GB_ERROR error = NULL;
                                uint8_t  read  = GB_read_lossless_byte(gb_field, error);

                                TEST_EXPECT_NO_ERROR(error);
                                TEST_EXPECT_EQUAL(written, read);
                            }
                            break;
                        }
                        case FIELD_FILTER_FLOAT_WRITEABLE: {
                            float tested_double_value[] = {
                                0.0, 1.0, 0.5,
                                1/3.0,
                                3.123456789,
                                1234567891.,
                                123456789.1,
                                12345678.91,
                                1234567.891,
                                123456.7891,
                                12345.67891,
                                1234.567891,
                                123.4567891,
                                12.34567891,
                                1.234567891,
                                .1234567891,
                                .0123456789,
                                .00123456789,
                                .000123456789,
                                .0000123456789,
                                .00000123456789,
                                .000000123456789,
                                .0000000123456789,
                                .00000000123456789,
                                .000000000123456789,
                                .0000000000123456789,
                                .00000000000000000000123456789,
                                .000000000000000000000000000000123456789,
                                .0000000000000000000000000000000000000000123456789,
                                .00000000000000000000000000000000000000000000000000123456789,
                                .000000000000000000000000000000000000000000000000000000000000123456789,
                                123456789.123456,
                                123456789.123456789,
                                M_PI,
                                123456789.3,
                            };

                            for (unsigned i = 0; i<ARRAY_ELEMS(tested_double_value); ++i) {
                                float written = tested_double_value[i];

                                TEST_ANNOTATE(GBS_global_string("type=%i FLOAT written=%f", int(type), written));
                                TEST_EXPECT_NO_ERROR(GB_write_lossless_float(gb_field, written));

                                double EPSILON = written*0.000001; // choose epsilon value depending on written value
                                if (EPSILON<=0.0) { // avoid zero epsilon value
                                    EPSILON = 0.000000001;
                                }

                                GB_ERROR error = NULL;
                                float    read  = GB_read_lossless_float(gb_field, error);

                                TEST_EXPECT_NO_ERROR(error);
                                TEST_EXPECT_SIMILAR(written, read, EPSILON);
                            }
                            break;
                        }
                        default: it_assert(0); break; // missing test for filter
                    }
                }
                else {
                    // test that GB_write_lossless_... does fail for other types:
                    switch (filter) {
                        case FIELD_FILTER_INT_WRITEABLE:
                            TEST_EXPECT_ERROR_CONTAINS(GB_write_lossless_int(gb_field, 4711), "Cannot use");
                            break;
                        case FIELD_FILTER_BYTE_WRITEABLE:
                            TEST_EXPECT_ERROR_CONTAINS(GB_write_lossless_byte(gb_field, 128), "Cannot use");
                            break;
                        case FIELD_FILTER_FLOAT_WRITEABLE:
                            TEST_EXPECT_ERROR_CONTAINS(GB_write_lossless_float(gb_field, M_PI), "Cannot use");
                            break;
                        default: it_assert(0); break; // missing test for filter
                    }
                }
            }
        }
    }

    GB_close(gb_main);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
