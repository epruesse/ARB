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

using std::string;
using std::map;

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
        GBDATA   *gb_key_type = GB_entry(gb_key, CHANGEKEY_TYPE);
        GB_TYPES  key_type    = GB_TYPES(GB_read_int(gb_key_type));

        if (shall_display_type(key_type)) {
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


                if (display) {
                    if (field_filter & SF_SHOW_TYPE) { // prefix type-char
                        display = GBS_global_string("%c: %s", GB_type_2_char(key_type), display);
                    }
                    insert(display, name);
                }
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
        gb_key_data = GB_search(gb_main, selector->change_key_path, GB_CREATE_CONTAINER);
    }
    return new Itemfield_Selection(from_sellist, gb_key_data, type_filter, field_filter, *selector);
}

const int FIELDNAME_VISIBLE_CHARS = 20;
const int NEWFIELD_EXTRA_SPACE    = 13; // max: " (new STRING)"

static struct {
    GB_TYPES    type;
    const char *label;
    const char *mnemonic;
    const char *typeName;
} creatable[] = {
    { GB_INT,    "Rounded numerical", "i", "INT" },
    { GB_FLOAT,  "Floating-point n.", "F", "FLOAT" },
    { GB_STRING, "Ascii text",        "s", "STRING" }
    // keep in sync with ../DB_UI/ui_species.cxx@FIELD_TYPE_DESCRIPTIONS
};

class RegFieldSelection;

typedef map<string, RegFieldSelection> FieldSelectionRegistry;

class RegFieldSelection {
    FieldSelDef              def;
    RefPtr<AW_window_simple> aw_popup;

    static FieldSelectionRegistry registry;
    static MutableItemSelector    NULL_selector;

public:
    bool inAwarChange;

    RegFieldSelection() :
        def("dummy", NULL, NULL_selector, 0),
        aw_popup(NULL)
    {}
    RegFieldSelection(const FieldSelDef& def_) :
        def(def_),
        aw_popup(NULL),
        inAwarChange(false)
    {}

    const FieldSelDef& get_def() const { return def; }

    bool new_fields_allowed() const { return def.new_fields_allowed(); }

    const char *get_field_awarname()  const { return def.get_awarname().c_str(); }
    const char *get_button_awarname() const { return GBS_global_string("tmp/%s/button", get_field_awarname()); }
    const char *get_type_awarname()   const { return GBS_global_string("%s_type",   get_field_awarname()); }

    void popup_window(AW_root *awr);
    void popdown() { if (aw_popup) aw_popup->hide(); }
    void create_window(AW_root *awr);
    void init_awars(AW_root *awr);

    GB_TYPES get_keytype(const char *fieldName) const {
        /*! detect type of (defined) key
         * @param fieldName name of field
         * @return GB_NONE for undefined key or type of defined key
        */
        GB_TYPES        definedType = GB_NONE;
        GBDATA         *gb_main     = def.get_gb_main();
        GB_transaction  ta(gb_main);
        GBDATA         *gb_key      = GBT_get_changekey(gb_main, fieldName, def.get_itemtype().change_key_path);

        if (gb_key) {
            long *elem_type = GBT_read_int(gb_key, CHANGEKEY_TYPE);
            if (elem_type) definedType = GB_TYPES(*elem_type);
        }
        return definedType;
    }

    GB_TYPES get_selected_type(AW_root *awr) {
        return GB_TYPES(awr->awar(get_type_awarname())->read_int());
    }
    GB_TYPES get_unmated_type() {
        // if exactly ONE type is allowed -> return that type
        // return GB_NONE otherwise
        long allowed = def.get_type_filter();

        for (unsigned i = 0; i<ARRAY_ELEMS(creatable); ++i) {
            long typeFlag = 1<<creatable[i].type;
            if (allowed & typeFlag) {
                if (allowed == typeFlag) return creatable[i].type; // exactly one type allowed
                break; // multiple types allowed
            }
        }
        return GB_NONE;
    }

    void update_button_text(AW_root *awr) const;

    // -----------------
    // static interface:
    static RegFieldSelection& registrate(AW_root *awr, const FieldSelDef& def);
    static RegFieldSelection *find(const string& awar_name) {
        FieldSelectionRegistry::iterator found = registry.find(awar_name);
        return found == registry.end() ? NULL : &found->second;
    }
    static void update_buttons();
};

FieldSelectionRegistry RegFieldSelection::registry;
MutableItemSelector    RegFieldSelection::NULL_selector;

const char *get_itemfield_type_awarname(const char *itemfield_awarname) {
    /*! returns the corresponding type-awar for an itemfield_awarname.
     * Only returns an awarname if
     * - itemfield_awarname was used in create_itemfield_selection_button and
     * - new fields are allowed there
     * returns NULL otherwise.
     */

    RegFieldSelection *registered = RegFieldSelection::find(itemfield_awarname);
    if (registered && registered->new_fields_allowed()) {
        return registered->get_type_awarname();
    }
    return NULL;
}

const char *prepare_and_get_selected_itemfield(AW_root *awr, const char *awar_name, GBDATA *gb_main, const ItemSelector& itemtype, FailIfField failIf) {
    /*! Reads awar used in create_itemfield_selection_button().
     *
     * If the user selected to create a new itemfield, the changekey is created.
     *
     * @param awr             app root
     * @param awar_name       name of awar used for create_itemfield_selection_button()
     * @param gb_main         database
     * @param itemtype        item type
     * @param failIf          toggles various error conditions (defaults to FIF_STANDARD)
     *
     * @return name of the itemfield or NULL
     *
     * When NULL is returned
     * - an error IS ALWAYS exported if (failIf&FIF_NO_FIELD_SELECTED), which is included in FIF_STANDARD!
     * - an error MAY BE    exported otherwise (use GB_have_error() in that case)
     */

    it_assert(!GB_have_error());

    RegFieldSelection *selected = RegFieldSelection::find(awar_name);
    GB_ERROR           error    = NULL;
    const char        *value    = NULL;

    if (!selected) {
        error = GBS_global_string("Awar '%s' is not registered as field selection", awar_name);
    }
    else {
        AW_awar    *awar_field  = awr->awar(selected->get_field_awarname());
        const char *field       = awar_field->read_char_pntr();
        const char *kindOfField = selected->get_def().get_described_field().c_str();

        if (!field[0]) field = NO_FIELD_SELECTED;

        if (strcmp(field, NO_FIELD_SELECTED) == 0) {
            if (failIf & FIF_NO_FIELD_SELECTED) {
                error = GBS_global_string("Please select a %s", kindOfField);
            }
        }
        else if (strcmp(field, "name") == 0 && (failIf & FIF_NAME_SELECTED)) {
            error = GBS_global_string("You may not select 'name' as %s.", kindOfField); // protect user from overwriting the species ID
        }
        else { // an allowed fieldname is selected
            GB_TYPES type = selected->get_keytype(field);

            if (type == GB_NONE) { // missing field
                if (selected->new_fields_allowed()) { // allowed to create?
                    error = GB_check_hkey(field);
                    if (!error) {
                        GB_TYPES wantedType = selected->get_selected_type(awr);
                        if (wantedType == GB_NONE) {
                            error = GBS_global_string("Please select the datatype for the new %s '%s'", kindOfField, field);
                        }
                        else {
                            error = GBT_add_new_changekey_to_keypath(gb_main, field, wantedType, itemtype.change_key_path);
                        }
                    }
                }
                else {
                    error = GBS_global_string("Selected %s '%s' is not defined (logical error)", kindOfField, field); // should not occur!
                }
            }
            else { // existing field
                bool typeAllowed = selected->get_def().get_type_filter() & (1<<type);
                if (!typeAllowed) {
                    // There are two known ways this situation can be reached:
                    // 1. select a new key; create key via search tool using a type not allowed here
                    // 2. specify a key with disallowed type as new key name
                    error = GBS_global_string("Selected field '%s' has unwanted type (%i)\nPlease select the %s again.", field, int(type), kindOfField);
                }
            }
            if (!error) value = field;
        }
    }

    if (error) {
        GB_export_error(error);
        value = NULL;
    }
    return value;
}

bool FieldSelDef::matches4reuse(const FieldSelDef& other) const {
    return
        (awar_name == other.awar_name)     && // shall use same awar,
        (&selector == &other.selector)     && // identical itemtype,
        (gb_main == other.gb_main)         && // same database,
        (type_filter == other.type_filter) && // same type-filter and
        (field_filter == other.field_filter); // same field-filter.
}

void RegFieldSelection::update_button_text(AW_root *awr) const {
    it_assert(new_fields_allowed());

    AW_awar    *awar_button = awr->awar(get_button_awarname());
    const char *fieldName   = awr->awar(get_field_awarname())->read_char_pntr();

    if (strcmp(fieldName, NO_FIELD_SELECTED) == 0 || !fieldName[0]) {
        awar_button->write_string("<no field>");
    }
    else {
        GB_TYPES type   = get_keytype(fieldName);
        bool     exists = type != GB_NONE;

        if (!exists) {
            type = GB_TYPES(awr->awar(get_type_awarname())->read_int());
        }

        const char *typeName = NULL;
        for (unsigned i = 0; i<ARRAY_ELEMS(creatable) && !typeName; ++i) {
            if (creatable[i].type == type) typeName = creatable[i].typeName;
        }

        if (typeName) {
            awar_button->write_string(GBS_global_string(exists ? "%s (%s)" : "%s (new %s)", fieldName, typeName));
        }
        else {
            awar_button->write_string(GBS_global_string("<undefined> '%s'", fieldName));
        }
    }
}

static void fieldname_changed_cb(AW_root *awr, RegFieldSelection *fsel) {
    if (!fsel->inAwarChange) {
        LocallyModify<bool> avoidRecursion(fsel->inAwarChange, true);

        AW_awar  *awar_type = awr->awar(fsel->get_type_awarname());
        AW_awar  *awar_name = awr->awar(fsel->get_field_awarname());
        GB_TYPES  defined   = fsel->get_keytype(awar_name->read_char_pntr());

        if (defined) {
            awar_type->write_int(defined);
            fsel->popdown();
        }
        else {
            // if allowed type of field is determined -> set it
            // otherwise set type to 'undefined'
            GB_TYPES determined = fsel->get_unmated_type();
            awar_type->write_int(determined);
#if defined(ARB_MOTIF)
            // wont be useful in gtk (as awar is updated after each character typed)
            if (determined != GB_NONE) fsel->popdown();
#endif
        }

        fsel->update_button_text(awr);
    }
}

static bool fieldtype_change_warning = true;
static void fieldtype_changed_cb(AW_root *awr, RegFieldSelection *fsel) {
    if (!fsel->inAwarChange) {
        LocallyModify<bool> avoidRecursion(fsel->inAwarChange, true);

        AW_awar  *awar_name = awr->awar(fsel->get_field_awarname());
        GB_TYPES  defined   = fsel->get_keytype(awar_name->read_char_pntr());

        if (defined != GB_NONE) {
            AW_awar  *awar_type = awr->awar(fsel->get_type_awarname());
            GB_TYPES  selected  = GB_TYPES(awar_type->read_int());

            if (selected != defined) {
                awar_type->write_int(defined);
                if (fieldtype_change_warning) {
                    aw_message("You cannot change the type of an existing field");
                }
            }
        }

        fsel->update_button_text(awr);
    }
}

void RegFieldSelection::init_awars(AW_root *awr) {
    if (new_fields_allowed()) {
        // extra awars needed for popup-style + callbacks
        awr->awar_string(get_button_awarname(), "?", AW_ROOT_DEFAULT);
        AW_awar *awar_type = awr->awar_int(get_type_awarname(), GB_NONE, AW_ROOT_DEFAULT);
        AW_awar *awar_name = awr->awar(get_field_awarname());

        awar_type->add_callback(makeRootCallback(fieldtype_changed_cb, this));
        awar_name->add_callback(makeRootCallback(fieldname_changed_cb, this));

        LocallyModify<bool> dontWarn(fieldtype_change_warning, false);
        awar_type->touch(); // refresh button-text (do NOT use awar_name to do that!)
    }
}

void RegFieldSelection::update_buttons() {
    for (FieldSelectionRegistry::const_iterator reg = registry.begin(); reg != registry.end(); ++reg) {
        const RegFieldSelection& fsel = reg->second;
        if (fsel.new_fields_allowed()) {
            fsel.update_button_text(AW_root::SINGLETON);
        }
    }
}

RegFieldSelection& RegFieldSelection::registrate(AW_root *awr, const FieldSelDef& def) {
    const string&      field_awarname = def.get_awarname();
    RegFieldSelection *found          = find(field_awarname);

    if (found) {
        bool compatible = !found->get_def().matches4reuse(def);
        if (!compatible) {
            aw_message(GBS_global_string("Incompatible field-selections defined for awar '%s'", field_awarname.c_str()));

            it_assert(0);
            // to use multiple field selections on the same awar, you need to use similar parameters!
            // (otherwise the user might outsmart the defined restrictions by using the other field-selector)
        }
    }
    else {
        registry[field_awarname] = RegFieldSelection(def);

        found = find(field_awarname);
        found->init_awars(awr);

        GBDATA *gb_main = def.get_gb_main();
        GB_transaction ta(gb_main);
        GBDATA *gb_key_data = GB_search(gb_main, def.get_itemtype().change_key_path, GB_FIND);
        if (gb_key_data) {
            aw_message_if(GB_ensure_callback(gb_key_data, GB_CB_CHANGED, makeDatabaseCallback(RegFieldSelection::update_buttons)));
        }
    }
    it_assert(found);
    return *found;
}

void RegFieldSelection::create_window(AW_root *awr) {
    it_assert(!aw_popup);

    aw_popup = new AW_window_simple;

    const bool allowNewFields = new_fields_allowed();

    {
        const char *format = allowNewFields ? "Select or create a new %s" : "Select the %s";
        const char *title  = GBS_global_string(format, def.get_described_field().c_str());
        aw_popup->init(awr, "SELECT_FIELD", title);
    }
    if (allowNewFields) aw_popup->load_xfig("awt/field_sel_new.fig"); // Do not DRY (ressource checker!)
    else                aw_popup->load_xfig("awt/field_sel.fig");

    aw_popup->at("sel");
    const bool FALLBACK2DEFAULT = !allowNewFields;

    Itemfield_Selection *itemSel;
    {
        AW_selection_list   *sellist = aw_popup->create_selection_list(get_field_awarname(), 1, 1, FALLBACK2DEFAULT);
        itemSel = def.build_sel(sellist);
        itemSel->refresh();
    }

    aw_popup->at("close");
    aw_popup->callback(AW_POPDOWN);
    aw_popup->create_button("CLOSE", "CLOSE", "C");

    AW_awar *awar_field = awr->awar(get_field_awarname()); // user-awar
    if (allowNewFields) {
        aw_popup->at("help");
        aw_popup->callback(makeHelpCallback("field_sel_new.hlp"));
        aw_popup->create_button("HELP", "HELP", "H");

        long allowedTypes  = def.get_type_filter();
        int  possibleTypes = 0;
        int  firstType     = -1;
        for (unsigned i = 0; i<ARRAY_ELEMS(creatable); ++i) {
            if (allowedTypes & (1<<creatable[i].type)) {
                possibleTypes++;
                if (firstType == -1) firstType = creatable[i].type;
            }
        }
        it_assert(possibleTypes>0);

        aw_popup->at("name");
        aw_popup->create_input_field(get_field_awarname(), FIELDNAME_VISIBLE_CHARS);

        // show type selector even if only one type selectable (layout- and informative-purposes)
        aw_popup->at("type");
        aw_popup->create_toggle_field(get_type_awarname(), NULL, 0);
        for (unsigned i = 0; i<ARRAY_ELEMS(creatable); ++i) {
            if (allowedTypes & (1<<creatable[i].type)) {
                aw_popup->insert_toggle(creatable[i].label, creatable[i].mnemonic, int(creatable[i].type));
            }
        }
        if (get_unmated_type() == GB_NONE) { // type may vary -> allowed it to be undefined
            aw_popup->insert_toggle("<undefined>", "", GB_NONE);
        }
        aw_popup->update_toggle_field();
    }
    else {
        awar_field->add_callback(makeRootCallback(awt_auto_popdown_cb, &*aw_popup));
    }
}

void RegFieldSelection::popup_window(AW_root *awr) {
    if (!aw_popup) create_window(awr);

    it_assert(aw_popup);
    aw_popup->recalc_pos_atShow(AW_REPOS_TO_MOUSE); // always popup at current mouse-position (i.e. directly above the button)
    aw_popup->recalc_size_atShow(AW_RESIZE_USER);   // if user changes the size of any field-selection popup, that size will be used for future popups

    aw_popup->activate();
}

static void popup_field_selection(AW_window *aw_parent, RegFieldSelection *fsel) {
    fsel->popup_window(aw_parent->get_root());
}

void create_itemfield_selection_button(AW_window *aws, const FieldSelDef& selDef, const char *at) {
    /*! Create a button that pops up a window allowing to select an item-field.
     * Field may exist or not. Allows to specify type of the new field in the latter case.
     *
     * @param aws         parent window
     * @param selDef      specifies details of field selection
     * @param at          xfig label (ignored if NULL)
     *
     * The length of the button is hardcoded and depends on whether new fields are possible or not.
     * Nevertheless you may override its size by defining an at-label with 'to:'-position in xfig.
     *
     * At start of field-writing code:
     * - use prepare_and_get_selected_itemfield() to create the changekey-info (needed for new fields only)
     *
     * For each item written to:
     * - use GBT_searchOrCreate_itemfield_according_to_changekey() to create the field
     * - use GB_write_lossless_...() to write its value
     */
    if (at) aws->at(at);

    RegFieldSelection& fsel = RegFieldSelection::registrate(aws->get_root(), selDef);

    int old_button_length = aws->get_button_length();
    aws->button_length(FIELDNAME_VISIBLE_CHARS+(selDef.new_fields_allowed() ? NEWFIELD_EXTRA_SPACE : 0));
    aws->callback(makeWindowCallback(popup_field_selection, &fsel));

    char *id = GBS_string_eval(selDef.get_awarname().c_str(), "/=_", NULL);
    aws->create_button(GBS_global_string("select_%s", id), selDef.new_fields_allowed() ? fsel.get_button_awarname() : fsel.get_field_awarname());
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

    const bool FALLBACK2DEFAULT            = true;
    it_assert(selDef.new_fields_allowed() == false); // to allow creation of new fields -> use create_itemfield_selection_button

    RegFieldSelection::registrate(aws->get_root(), selDef); // to avoid that awar is misused for multiple incompatible field-selections

    AW_selection_list   *sellist   = aws->create_selection_list(selDef.get_awarname().c_str(), FALLBACK2DEFAULT);
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
