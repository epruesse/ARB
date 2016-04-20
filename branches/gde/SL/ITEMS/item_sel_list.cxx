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
#include <ad_cb.h>

#include <arb_defs.h>
#include <arb_global_defs.h>

#include <aw_root.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <awt_sel_boxes.hxx>

#include <map>

Itemfield_Selection::Itemfield_Selection(AW_selection_list *sellist_,
                                         GBDATA            *gb_key_data,
                                         long               type_filter_,
                                         SelectableFields   field_filter_,
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
        insert(PSEUDO_FIELD_ANY_FIELD,  PSEUDO_FIELD_ANY_FIELD);
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
        insert_default((field_filter & SF_ALLOW_NEW) ? "<no or new field>" : "<no field>", NO_FIELD_SELECTED);
    }
}

Itemfield_Selection *FieldSelDef::build_sel(AW_selection_list *from_sellist) const {
    GBDATA *gb_key_data;
    {
        GB_transaction ta(gb_main);
        gb_key_data = GB_search(gb_main, selector.change_key_path, GB_CREATE_CONTAINER);
    }
    return new Itemfield_Selection(from_sellist, gb_key_data, type_filter, field_filter, selector);
}

const int FIELDNAME_VISIBLE_CHARS = 20;

static struct {
    GB_TYPES    type;
    const char *label;
    const char *mnemonic;
    const char *typeName;
} creatable[] = {
    { GB_INT,    "Rounded numerical", "R", "INT" },
    { GB_FLOAT,  "Numerical",         "N", "FLOAT" },
    { GB_STRING, "Ascii text",        "t", "STRING" }
    // keep in sync with ../DB_UI/ui_species.cxx@FIELD_TYPE_DESCRIPTIONS
};

#define TRIGGER_PREFIX "tmp/trigger/awar/"

static void itemfield_transaction_succeeded(GBDATA *gb_trigger, GB_CB_TYPE cbtype) {
    // this callback is triggered by prepare_and_get_selected_itemfield()
    // when a new field is selected (i.e. field-awar contains sth like "xxx (new INT)").
    // In that case a new changekey is created.
    //
    // If the transaction (of the caller) is aborted, the changekey generation is reverted and this callback will not be called.
    // If the transaction is committed, this callback changes the value of the field-awar to "xxx" (because the field exists now).

    if (cbtype & GB_CB_CHANGED) {
        const char *dbpath     = GB_get_db_path(gb_trigger);
        const char *is_trigger = strstr(dbpath, TRIGGER_PREFIX);
        it_assert(is_trigger);

        const char *awar_name = is_trigger+strlen(TRIGGER_PREFIX); // suffix of gb_trigger-database-path == awar-name
        AW_root::SINGLETON->awar(awar_name)->write_string(GB_read_char_pntr(gb_trigger));
    }
}

const char *prepare_and_get_selected_itemfield(AW_root *awr, const char *awar_name, GBDATA *gb_main, const ItemSelector& itemtype, const char *description, FailIfField failIf) {
    /*! Reads awar used in create_itemfield_selection_button().
     * If the user selected to create a new itemfield, the changekey is created and
     * the content of the awar is corrected (i.e. ' (new TYPE)' gets removed).
     *
     * @param awr             app root
     * @param awar_name       name of awar used for create_itemfield_selection_button()
     * @param gb_main         database
     * @param itemtype        item type
     * @param description     purpose of field (used for messages, defaults to "target")
     * @param failIf          toggles various error conditions
     *
     * @return name of the itemfield (or NULL: if error -> error is exported, otherwise NO_FIELD_SELECTED)
     * If (failIf & FIF_NO_FIELD_SELECTED) an error is exported if NO_FIELD_SELECTED
     */

    it_assert(!GB_have_error());

    AW_awar    *awar  = awr->awar(awar_name);
    const char *value = awar->read_char_pntr();
    const char *isNew = strstr(value, " (new ");
    GB_ERROR    error = NULL;

    if (isNew) {
        char *type  = strdup(isNew+6);
        char *paren = strchr(type, ')');

        if (!paren) error = GBS_global_string("Invalid awar-content ('%s')", value);
        else {
            paren[0]  = 0;
            int found = -1;
            for (unsigned i = 0; i<ARRAY_ELEMS(creatable) && found == -1; ++i) {
                if (strcmp(creatable[i].typeName, type) == 0) found = i;
            }

            if (found == -1) error = GBS_global_string("Unknown type specified for new field ('%s')", type);
            else {
                char *fieldName   = GB_strpartdup(value, isNew-1);
                error             = GB_check_hkey(fieldName);
                if (!error) error = GBT_add_new_changekey_to_keypath(gb_main, fieldName, creatable[found].type, itemtype.change_key_path);
                if (!error) {
                    const char *trigger    = GBS_global_string(TRIGGER_PREFIX "%s", awar_name);
                    GBDATA     *gb_trigger = GB_search(gb_main, trigger, GB_FIND);

                    if (!error) error = GB_write_string(gb_trigger, fieldName); // trigger awar-update-callback (will only execute if the current TA commits!)
                    if (!error) value = GB_read_char_pntr(gb_trigger);          // use fieldname w/o type as result
                }
                free(fieldName);
            }
        }
        free(type);
    }

    if (!error && value) {
        if (!value[0]) {
            value = NO_FIELD_SELECTED; // interpret "" as NO_FIELD_SELECTED (compensates past wrong use which may still be present in properties)
        }
        else if (strcmp(value, "name") == 0) {
            error = GBS_global_string("You may not select 'name' as %s field.", description); // protect user from overwriting the species ID
        }
    }

    if (!error) {
        it_assert(value);
        if (strcmp(value, NO_FIELD_SELECTED) == 0) {
            if (failIf & FIF_NO_FIELD_SELECTED) {
                error = GBS_global_string("Please select a %s field", description);
            }
            else {
                value = NULL;
            }
        }
    }

    if (error) {
        GB_export_error(error);
        value = NULL;
    }
    return value;
}

inline const char *newNameAwarname(AW_awar *awar_field) { return GBS_global_string("tmp/%s/new/name", awar_field->awar_name); }
inline const char *newTypeAwarname(AW_awar *awar_field) { return GBS_global_string("tmp/%s/new/type", awar_field->awar_name); }

class ItemfieldAwarCbData {
    AW_awar             *awar_field;
    Itemfield_Selection *itemSel;
    AW_window_simple    *aw_popup;

public:
    ItemfieldAwarCbData(AW_awar *awar_field_, Itemfield_Selection *itemSel_, AW_window_simple *aw_popup_)
        : awar_field(awar_field_),
          itemSel(itemSel_),
          aw_popup(aw_popup_),
          inAwarCallback(false)
    {}

    AW_awar *get_field_awar() const { return awar_field; }
    Itemfield_Selection *get_selection() const { return itemSel; }
    void hide_window() const { aw_popup->hide(); }

    bool inAwarCallback;
};


static void newFieldDef_changed_cb(AW_root *awr, ItemfieldAwarCbData *cbdata) {
    if (!cbdata->inAwarCallback) {
        LocallyModify<bool> flag(cbdata->inAwarCallback, true);

        AW_awar *awar_field   = cbdata->get_field_awar();
        char    *newFieldName = awr->awar(newNameAwarname(awar_field))->read_string();
        if (newFieldName[0]) { // non-empty key
            Itemfield_Selection *itemSel = cbdata->get_selection();
            GBDATA              *gb_main = itemSel->get_gb_main();
            GB_transaction       ta(gb_main);

            GB_TYPES type = GBT_get_type_of_changekey(gb_main, newFieldName, itemSel->get_selector().change_key_path);
            if (type == GB_NONE) { // a new key was specified
                GB_TYPES selectedType = (GB_TYPES)awr->awar(newTypeAwarname(awar_field))->read_int();
                for (unsigned i = 0; i<ARRAY_ELEMS(creatable); ++i) {
                    if (creatable[i].type == selectedType) {
                        awar_field->write_string(GBS_global_string_copy("%s (new %s)", newFieldName, creatable[i].typeName));
                        break;
                    }
                }
            }
            else { // an existing key was specified
                long type_filter = itemSel->get_type_filter();
                if (type_filter & (1<<type)) {
                    awar_field->rewrite_string(newFieldName);
                }
                else {
                    awar_field->rewrite_string(NO_FIELD_SELECTED);
                    aw_message(GBS_global_string("Field '%s' already exists, but has incompatible type", newFieldName));
                }
            }
        }
        free(newFieldName);
    }
    it_assert(!GB_have_error());
}

static void selField_changed_cb(AW_root *awr, ItemfieldAwarCbData *cbdata) {
    AW_awar    *awar_field  = cbdata->get_field_awar();
    const char *selected    = awar_field->read_char_pntr();
    if (strcmp(selected, NO_FIELD_SELECTED) != 0) {
        if (!cbdata->inAwarCallback) {
            LocallyModify<bool> avoidRecursion(cbdata->inAwarCallback, true);

            Itemfield_Selection *itemSel = cbdata->get_selection();
            GBDATA              *gb_main = itemSel->get_gb_main();
            GB_transaction       ta(gb_main);

            GB_TYPES type = GBT_get_type_of_changekey(gb_main, selected, itemSel->get_selector().change_key_path);
            if (type != GB_NONE) {
                awr->awar(newNameAwarname(awar_field))->write_string("");
                awr->awar(newTypeAwarname(awar_field))->write_int(type);

                cbdata->hide_window();
            }
        }
    }
    it_assert(!GB_have_error());
}

#if defined(ASSERTION_USED)
MutableItemSelector NULL_selector;
FieldSelDef::FieldSelDef() : selector(NULL_selector) {}

bool FieldSelDef::matches4reuse(const FieldSelDef& other) {
    return
        (&selector == &other.selector)     && // shall use identical itemtype,
        (gb_main == other.gb_main)         && // same database,
        (type_filter == other.type_filter) && // same type-filter and
        (field_filter == other.field_filter); // same field-filter.
}
#endif

static AW_window *createFieldSelectionPopup(AW_root *awr, FieldSelDef *selDef) {
    typedef std::map<std::string, AW_window_simple*> AwarToWindowMap;
    static AwarToWindowMap existingPopups; // only one window per awar (otherwise reuse)

#if defined(ASSERTION_USED)
    // check compatibility of multiple selections on same awar (does reuse make sense?)
    typedef std::map<std::string, FieldSelDef> AwarToFieldSelDef;
    static AwarToFieldSelDef existingDefs;
#endif

    AwarToWindowMap::iterator  found    = existingPopups.find(selDef->get_awarname());
    AW_window_simple          *aw_popup = NULL;

    if (found != existingPopups.end()) {
        aw_popup = found->second;

#if defined(ASSERTION_USED)
        AwarToFieldSelDef::iterator prevDef = existingDefs.find(selDef->get_awarname());
        it_assert(prevDef != existingDefs.end());
        it_assert(selDef->matches4reuse(prevDef->second)); // to use multiple selection popups on same awar, you need to use similar parameters
                                                           // (otherwise the user might outsmart your restrictions by using the other field-selector)
#endif
    }
    else {
        aw_popup = new AW_window_simple;

        const bool allowNewFields = selDef->new_fields_allowed();

        aw_popup->init(awr, "SELECT_FIELD", allowNewFields ? "Select or create new field" : "Select a field");
        aw_popup->load_xfig(allowNewFields ? "awt/field_sel_new.fig" : "awt/field_sel.fig");

        aw_popup->at("sel");
        const bool FALLBACK2DEFAULT = !allowNewFields;

        Itemfield_Selection *itemSel;
        {
            AW_selection_list   *sellist = aw_popup->create_selection_list(selDef->get_awarname(), 1, 1, FALLBACK2DEFAULT);
            itemSel = selDef->build_sel(sellist);
            itemSel->refresh();
        }

        aw_popup->at("close");
        aw_popup->callback(AW_POPDOWN);
        aw_popup->create_button("CLOSE", "CLOSE", "C");

        AW_awar *awar_field = awr->awar(selDef->get_awarname()); // user-awar
        if (allowNewFields) {
            aw_popup->at("help");
            aw_popup->callback(makeHelpCallback("field_sel_new.hlp"));
            aw_popup->create_button("HELP", "HELP", "H");

            char    *awarname_newname = strdup(newNameAwarname(awar_field));
            char    *awarname_newtype = strdup(newTypeAwarname(awar_field));
            long     allowedTypes     = selDef->get_type_filter();

            int possibleTypes = 0;
            int firstType     = -1;
            for (unsigned i = 0; i<ARRAY_ELEMS(creatable); ++i) {
                if (allowedTypes & (1<<creatable[i].type)) {
                    possibleTypes++;
                    if (firstType == -1) firstType = creatable[i].type;
                }
            }
            it_assert(possibleTypes>0);

            ItemfieldAwarCbData * const cbdata = new ItemfieldAwarCbData(awar_field, itemSel, aw_popup); // never freed (bound to callbacks)

            awr->awar_string(awarname_newname, "",        AW_ROOT_DEFAULT)->add_callback(makeRootCallback(newFieldDef_changed_cb, cbdata));
            awr->awar_int   (awarname_newtype, firstType, AW_ROOT_DEFAULT)->add_callback(makeRootCallback(newFieldDef_changed_cb, cbdata));

            awar_field->add_callback(makeRootCallback(selField_changed_cb, cbdata));

            aw_popup->at("name");
            aw_popup->create_input_field(awarname_newname, FIELDNAME_VISIBLE_CHARS);

            // show type selector even if only one type selectable (layout- and informative-purposes)
            aw_popup->at("type");
            aw_popup->create_toggle_field(awarname_newtype, NULL, 0);
            for (unsigned i = 0; i<ARRAY_ELEMS(creatable); ++i) {
                if (allowedTypes & (1<<creatable[i].type)) {
                    aw_popup->insert_toggle(creatable[i].label, creatable[i].mnemonic, int(creatable[i].type));
                }
            }
            aw_popup->update_toggle_field();

            // install delayed awar-update callback
            {
                GBDATA         *gb_main    = selDef->get_gb_main();
                GB_transaction  ta(gb_main);
                const char     *trigger    = GBS_global_string(TRIGGER_PREFIX "%s", selDef->get_awarname());
                GBDATA         *gb_trigger = GB_searchOrCreate_string(gb_main, trigger, "");
                GB_ERROR        error;

                if (!gb_trigger) {
                    error = GB_await_error();
                }
                else {
                    error = GB_add_callback(gb_trigger, GB_CB_ALL, makeDatabaseCallback(itemfield_transaction_succeeded));
                }
                if (error) {
                    aw_message(GBS_global_string("Failed to install awar-update-trigger (Reason: %s)", error));
                }
            }

            free(awarname_newtype);
            free(awarname_newname);
        }
        else {
            awar_field->add_callback(makeRootCallback(awt_auto_popdown_cb, aw_popup));
        }

        existingPopups[selDef->get_awarname()] = aw_popup;
#if defined(ASSERTION_USED)
        existingDefs[selDef->get_awarname()] = FieldSelDef(*selDef);
#endif
    }

    aw_popup->recalc_pos_atShow(AW_REPOS_TO_MOUSE); // always popup at current mouse-position (i.e. directly above the button)
    aw_popup->recalc_size_atShow(AW_RESIZE_USER);   // if user changes the size of any field-selection popup, that size will be used for future popups

    delete selDef;

    return aw_popup;
}

void create_itemfield_selection_button(AW_window *aws, const FieldSelDef& selDef, const char *at) {
    /*! Create a button that pops up a window allowing to select an item-field.
     * Field may exist or not. Allows to specify type of the new field in the latter case.
     *
     * @param aws         parent window
     * @param selDef      specifies details of field selection
     * @param at          xfig label (ignored if NULL)
     *
     * If a new, not yet existing field is selected, the awar specified by 'selDef' is set
     * to 'fieldname (new TYPE)'.
     *
     * At start of field-writing code:
     * - use prepare_and_get_selected_itemfield() to create the changekey-info and to correct the fieldname (needed for new fields only).
     *
     * For each item written to:
     * - use GBT_searchOrCreate_itemfield_according_to_changekey() to create the field
     * - use GB_write_lossless_...() to write its value
     */
    if (at) aws->at(at);

    int old_button_length = aws->get_button_length();
    aws->button_length(FIELDNAME_VISIBLE_CHARS);
    aws->callback(makeCreateWindowCallback(createFieldSelectionPopup, new FieldSelDef(selDef)));

    char *id = GBS_string_eval(selDef.get_awarname(), "/=_", NULL);
    aws->create_button(GBS_global_string("select_%s", id), selDef.get_awarname());
    free(id);

    aws->button_length(old_button_length);
}

Itemfield_Selection *create_itemfield_selection_list(AW_window *aws, const FieldSelDef& selDef, const char *at) {
    /*! Create a selection list allowing to select an item-field.
     * Similar to create_itemfield_selection_button().
     *
     * Differences:
     * - returns Itemfield_Selection -> allows to query list of fields (e.g. used in "Reorder fields")
     * - does not allow to select a new, not yet existing field
     */
    if (at) aws->at(at);

    const bool FALLBACK2DEFAULT = true;
    it_assert(selDef.new_fields_allowed() == false); // to allow creation of new fields -> use create_itemfield_selection_button

    AW_selection_list   *sellist   = aws->create_selection_list(selDef.get_awarname(), FALLBACK2DEFAULT);
    Itemfield_Selection *selection = selDef.build_sel(sellist);
    selection->refresh();
    return selection;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

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
