//  ==================================================================== //
//                                                                       //
//    File      : AWT_input_mask.cxx                                     //
//    Purpose   : General input masks                                    //
//    Time-stamp: <Fri Aug/06/2004 11:32 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in August 2001           //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#include <cctype>
#include <cstdio>

#include <arbdb.h>
#include <awt.hxx>
#include <awt_www.hxx>

#ifndef __MAP__
#include <map>
#endif
#ifndef __VECTOR__
#include <vector>
#endif

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "awt_input_mask.hxx"

using namespace std;

const char *awt_itemtype_names[AWT_IT_TYPES+1] =
{
 "Unknown",
 "Species", "Organism", "Gene", "Experiment",
 "<overflow>"
};

#define SEC_XBORDER    3        // space left/right of NEW_SECTION line
#define SEC_YBORDER    4        // space above/below
#define SEC_LINE_WIDTH 1        // line width

#define MIN_TEXTFIELD_SIZE 1
#define MAX_TEXTFIELD_SIZE 1000

#define AWT_SCOPE_LOCAL  0
#define AWT_SCOPE_GLOBAL 1

awt_input_mask_id_list awt_input_mask_global::global_ids; // stores global ids

// ####################################
// ####################################
// ###                              ###
// ##          global awars          ##
// ###                              ###
// ####################################
// ####################################

#define AWAR_INPUT_MASK_BASE   "tmp/inputMask"
#define AWAR_INPUT_MASK_NAME   AWAR_INPUT_MASK_BASE"/name"
#define AWAR_INPUT_MASK_ITEM   AWAR_INPUT_MASK_BASE"/item"
#define AWAR_INPUT_MASK_SCOPE  AWAR_INPUT_MASK_BASE"/scope"
#define AWAR_INPUT_MASK_HIDDEN AWAR_INPUT_MASK_BASE"/hidden"

#define AWAR_INPUT_MASKS_EDIT_ENABLED AWAR_INPUT_MASK_BASE"/edit_enabled"

static bool global_awars_created = false;

//  --------------------------------------------------------------
//      void AWT_input_mask_create_global_awars(AW_root *awr)
//  --------------------------------------------------------------
static void create_global_awars(AW_root *awr) {
    awt_assert(!global_awars_created);
    awr->awar_string(AWAR_INPUT_MASK_NAME, "new");
    awr->awar_string(AWAR_INPUT_MASK_ITEM, awt_itemtype_names[AWT_IT_SPECIES]);
    awr->awar_int(AWAR_INPUT_MASK_SCOPE, 0);
    awr->awar_int(AWAR_INPUT_MASK_HIDDEN, 0);
    awr->awar_int(AWAR_INPUT_MASKS_EDIT_ENABLED, 0);
    global_awars_created = true;
}

// #########################################################
// #########################################################
// ###                                                   ###
// ##          Callbacks from database and awars          ##
// ###                                                   ###
// #########################################################
// #########################################################

static bool in_item_changed_callback  = false;
static bool in_field_changed_callback = false;
static bool in_awar_changed_callback  = false;

//  --------------------------------------------------------------------------------------------------
//      static void item_changed_cb(GBDATA *gb_item, int *cl_awt_linked_to_item, GB_CB_TYPE type)
//  --------------------------------------------------------------------------------------------------
static void item_changed_cb(GBDATA */*gb_item*/, int *cl_awt_linked_to_item, GB_CB_TYPE type) {
    if (!in_item_changed_callback) { // avoid deadlock
        in_item_changed_callback      = true;
        awt_linked_to_item *item_link = (awt_linked_to_item*)cl_awt_linked_to_item;

        if (type&GB_CB_DELETE) { // handled child was deleted
            item_link->relink();
        }
        else if ((type&(GB_CB_CHANGED|GB_CB_SON_CREATED)) == (GB_CB_CHANGED|GB_CB_SON_CREATED)) {
            // child was created (not only changed)
            item_link->relink();
        }
        else if (type&GB_CB_CHANGED) { // only changed
            item_link->general_item_change();
        }

        in_item_changed_callback = false;
    }
}

//  --------------------------------------------------------------------------------------------------
//      static void field_changed_cb(GBDATA *gb_item, int *cl_awt_input_handler, GB_CB_TYPE type)
//  --------------------------------------------------------------------------------------------------
static void field_changed_cb(GBDATA */*gb_item*/, int *cl_awt_input_handler, GB_CB_TYPE type) {
    if (!in_field_changed_callback) { // avoid deadlock
        in_field_changed_callback  = true;
        awt_input_handler *handler = (awt_input_handler*)cl_awt_input_handler;

        if (type&GB_CB_DELETE) { // field was deleted from db -> relink this item
            handler->relink();
        }
        else if (type&GB_CB_CHANGED) {
            handler->db_changed();  // database entry was changed
        }
        in_field_changed_callback = false;
    }
}
//  -------------------------------------------------------------------------------
//      static void awar_changed_cb(AW_root *awr, AW_CL cl_awt_mask_awar_item)
//  -------------------------------------------------------------------------------
static void awar_changed_cb(AW_root */*awr*/, AW_CL cl_awt_mask_awar_item) {
    if (!in_awar_changed_callback) { // avoid deadlock
        in_awar_changed_callback   = true;
        awt_mask_awar_item *item = (awt_mask_awar_item*)cl_awt_mask_awar_item;
        awt_assert(item);
        if (item) item->awar_changed();
        in_awar_changed_callback   = false;
    }
}

// start of implementation of class awt_input_mask_global:

//  ----------------------------------------------------------------------------------
//      static string awt_input_mask_global::generate_id(const string& mask_name_)
//  ----------------------------------------------------------------------------------
string awt_input_mask_global::generate_id(const string& mask_name_)
{
    string result;
    result.reserve(mask_name_.length());
    for (string::const_iterator p = mask_name_.begin(); p != mask_name_.end(); ++p) {
        if (isalnum(*p)) result.append(1, *p);
        else result.append(1, '_');
    }
    return result;
}

//  ---------------------------------------------------------
//      bool awt_input_mask_global::edit_allowed() const
//  ---------------------------------------------------------
bool awt_input_mask_global::edit_allowed() const {
    return !test_edit_enabled ||
        get_root()->awar(AWAR_INPUT_MASKS_EDIT_ENABLED)->read_int() == 1;
}

//  ------------------------------------------------------------
//      void awt_input_mask_global::no_item_selected() const
//  ------------------------------------------------------------
void awt_input_mask_global::no_item_selected() const {
    aw_message(GBS_global_string("This had no effect, because no %s is selected",
                                 awt_itemtype_names[get_itemtype()]));
}

// -end- of implementation of class awt_input_mask_global.

// start of implementation of class awt_input_mask_id_list:

//  --------------------------------------------------------------------------------------
//      GB_ERROR awt_input_mask_id_list::add(const string& name, awt_mask_item *item)
//  --------------------------------------------------------------------------------------
GB_ERROR awt_input_mask_id_list::add(const string& name, awt_mask_item *item) {
    awt_mask_item *existing = lookup(name);
    if (existing) return GB_export_error("ID '%s' already exists", name.c_str());

    id[name] = item;
    return 0;
}
//  --------------------------------------------------------------------
//      GB_ERROR awt_input_mask_id_list::remove(const string& name)
//  --------------------------------------------------------------------
GB_ERROR awt_input_mask_id_list::remove(const string& name) {
    if (!lookup(name)) return GB_export_error("ID '%s' does not exist", name.c_str());
    id.erase(name);
    return 0;
}

// -end- of implementation of class awt_input_mask_id_list.

// start of implementation of class awt_mask_item:

//  --------------------------------------------------------------------
//      awt_mask_item::awt_mask_item(awt_input_mask_global *global_)
//  --------------------------------------------------------------------
awt_mask_item::awt_mask_item(awt_input_mask_global *global_)
    : global(global_)
{
}

//  ----------------------------------------
//      awt_mask_item::~awt_mask_item()
//  ----------------------------------------
awt_mask_item::~awt_mask_item() {
    awt_assert(!has_name()); // you forgot to call remove_name()
}

//  -----------------------------------------------------------------------------
//      GB_ERROR awt_mask_item::set_name(const string& name_, bool is_global)
//  -----------------------------------------------------------------------------
GB_ERROR awt_mask_item::set_name(const string& name_, bool is_global)
{
    GB_ERROR error = 0;
    if (has_name()) {
        error = GB_export_error("Element already has name (%s)", get_name().c_str());
    }
    else {
        name = new string(name_);
        if (is_global) {
            if (!mask_global()->has_global_id(*name)) { // do not add if variable already defined elsewhere
                error = mask_global()->add_global_id(*name, this);
            }
        }
        else {
            error = mask_global()->add_local_id(*name, this);
        }
    }
    return error;
}

//  ---------------------------------------------
//      GB_ERROR awt_mask_item::remove_name()
//  ---------------------------------------------
GB_ERROR awt_mask_item::remove_name()
{
    GB_ERROR error = 0;
    if (has_name()) {
        error = mask_global()->remove_id(*name);
        name.SetNull();
    }
    return error;
}

// -end- of implementation of class awt_mask_item.

// start of implementation of class awt_mask_awar_item:

//  ----------------------------------------------------------------------------------------------------------------------------------------------------------------
//      awt_mask_awar_item::awt_mask_awar_item(awt_input_mask_global *global_, const string& awar_base, const string& default_value, bool saved_with_properties)
//  ----------------------------------------------------------------------------------------------------------------------------------------------------------------
awt_mask_awar_item::awt_mask_awar_item(awt_input_mask_global *global_, const string& awar_base, const string& default_value, bool saved_with_properties)
    : awt_mask_item(global_)
{
    const char *root_name;

    if (saved_with_properties) root_name = "/input_masks";
    else root_name = "/tmp/input_masks"; // awars starting with /tmp are not saved

    awarName = GBS_global_string("%s/%s", root_name, awar_base.c_str());
#if defined(DEBUG)
    printf("awarName='%s'\n", awarName.c_str());
#endif // DEBUG
    mask_global()->get_root()->awar_string(awarName.c_str(), default_value.c_str()); // create the awar
    add_awar_callbacks();
}

//  ------------------------------------------------------
//      void awt_mask_awar_item::add_awar_callbacks()
//  ------------------------------------------------------
void awt_mask_awar_item::add_awar_callbacks() {
    AW_awar *var = awar();
    awt_assert(var);
    if (var) var->add_callback(awar_changed_cb, AW_CL(this));
}
//  ---------------------------------------------------------
//      void awt_mask_awar_item::remove_awar_callbacks()
//  ---------------------------------------------------------
void awt_mask_awar_item::remove_awar_callbacks() {
    AW_awar *var = awar();
    awt_assert(var);
    if (var) var->remove_callback((AW_RCB)awar_changed_cb, AW_CL(this), AW_CL(0));
}

// -end- of implementation of class awt_mask_awar_item.

// start of implementation of class awt_variable:

//  ---------------------------------------------------------------------------------------------------------------------------------------------------
//      awt_variable::awt_variable(awt_input_mask_global *global_, const string& id, bool is_global_, const string& default_value, GB_ERROR& error)
//  ---------------------------------------------------------------------------------------------------------------------------------------------------
awt_variable::awt_variable(awt_input_mask_global *global_, const string& id, bool is_global_, const string& default_value, GB_ERROR& error)
    : awt_mask_awar_item(global_, generate_baseName(global_, id, is_global_), default_value, true)
    , is_global(is_global_)
{
    error = set_name(id, is_global);
}

//  -------------------------------------
//      awt_variable::~awt_variable()
//  -------------------------------------
awt_variable::~awt_variable()
{
}

// -end- of implementation of class awt_variable.

// start of implementation of class awt_script:

//  --------------------------------------------
//      string awt_script::get_value() const
//  --------------------------------------------
string awt_script::get_value() const
{
    string                        result;
    AW_root                      *root     = mask_global()->get_root();
    const awt_item_type_selector *selector = mask_global()->get_selector();
    GBDATA                       *gbd      = selector->current(root);

    if (gbd) {
        char           *species_name    = root->awar(selector->get_self_awar())->read_string();
        GBDATA         *gb_main = mask_global()->get_gb_main();
        GB_transaction  tscope(gb_main);

        char *val = GB_command_interpreter(gb_main, species_name, script.c_str(), gbd, 0);
        if (!val) {
            aw_message(GB_get_error());
            result = "<error>";
        }
        else {
            result = val;
            free(val);
        }
        free(species_name);
    }
    else {
        result = "<undefined>";
    }


    return result;
}

//  -------------------------------------------------------------------
//      GB_ERROR awt_script::set_value(const string& /*new_value*/)
//  -------------------------------------------------------------------
GB_ERROR awt_script::set_value(const string& /*new_value*/)
{
    return GBS_global_string("You cannot assign a value to script '%s'", has_name() ? get_name().c_str() : "<unnamed>");
}

// -end- of implementation of class awt_script.

// start of implementation of class awt_linked_to_item:

//  -------------------------------------------------------
//      GB_ERROR awt_linked_to_item::add_db_callbacks()
//  -------------------------------------------------------
GB_ERROR awt_linked_to_item::add_db_callbacks()
{
    GB_ERROR error = 0;
    if (gb_item) error = GB_add_callback(gb_item, (GB_CB_TYPE)(GB_CB_CHANGED|GB_CB_DELETE), item_changed_cb, (int*)this);
    return error;
}

//  ----------------------------------------------------------
//      GB_ERROR awt_linked_to_item::remove_db_callbacks()
//  ----------------------------------------------------------
GB_ERROR awt_linked_to_item::remove_db_callbacks()
{
    GB_ERROR error = 0;
    if (gb_item) error = GB_remove_callback(gb_item, (GB_CB_TYPE)(GB_CB_CHANGED|GB_CB_DELETE), item_changed_cb, (int*)this);
    return error;
}

// -end- of implementation of class awt_linked_to_item.

// start of implementation of class awt_script_viewport:

//  ----------------------------------------------------------------------------------------------------------------------------------------------------
//      awt_script_viewport::awt_script_viewport(awt_input_mask_global *global_, const awt_script *script_, const string& label_, long field_width_)
//  ----------------------------------------------------------------------------------------------------------------------------------------------------
awt_script_viewport::awt_script_viewport(awt_input_mask_global *global_, const awt_script *script_, const string& label_, long field_width_)
    : awt_viewport(global_, generate_baseName(global_), "", false, label_)
    , script(script_)
    , field_width(field_width_)
{
}

//  ---------------------------------------------------
//      awt_script_viewport::~awt_script_viewport()
//  ---------------------------------------------------
awt_script_viewport::~awt_script_viewport()
{
    unlink();
}

//  ------------------------------------------------------------------
//      GB_ERROR awt_script_viewport::link_to(GBDATA *gb_new_item)
//  ------------------------------------------------------------------
GB_ERROR awt_script_viewport::link_to(GBDATA *gb_new_item)
{
    GB_ERROR       error = 0;
    GB_transaction dummy(mask_global()->get_gb_main());

    remove_awar_callbacks();    // unbind awar callbacks temporarily

    if (item()) {
        remove_db_callbacks(); // ignore result (if handled db-entry was deleted, it returns an error)
        set_item(0);
    }

    if (gb_new_item) {
        set_item(gb_new_item);
        db_changed();
        error = add_db_callbacks();
    }

    add_awar_callbacks();       // rebind awar callbacks

    return error;
}

//  --------------------------------------------------------------
//      void awt_script_viewport::build_widget(AW_window *aws)
//  --------------------------------------------------------------
void awt_script_viewport::build_widget(AW_window *aws)
{
    const string& lab = get_label();
    if (lab.length()) aws->label(lab.c_str());

    aws->create_input_field(awar_name().c_str(), field_width);
}

//  ----------------------------------------------
//      void awt_script_viewport::db_changed()
//  ----------------------------------------------
void awt_script_viewport::db_changed()
{
    GB_ERROR error = 0;

//     if (!script) { // search script
//         awt_mask_item *item = mask_global()->get_identified_item(script_id, error);
//         if (!error) {
//             awt_script *found_script = dynamic_cast<awt_script*>(item);
//             if (found_script) script = found_script;
//             else        error        = GBS_global_string("ID '%s' does not represent a script", script_id.c_str());
//         }
//     }

//     if (!error) {
        awt_assert(script);
        string current_value = script->get_value();
        error                = awt_mask_awar_item::set_value(current_value);
//     }

    if (error) aw_message(error);
}

//  ------------------------------------------------
//      void awt_script_viewport::awar_changed()
//  ------------------------------------------------
void awt_script_viewport::awar_changed() {
    aw_message("It makes no sense to change the result of a script");
}

// -end- of implementation of class awt_script_viewport.

// start of implementation of class awt_input_handler:

//  ----------------------------------------------------
//      GB_ERROR awt_input_handler::add_callbacks()
//  ----------------------------------------------------
GB_ERROR awt_input_handler::add_db_callbacks() {
    GB_ERROR error = awt_linked_to_item::add_db_callbacks();
    if (item() && gbd) error = GB_add_callback(gbd, (GB_CB_TYPE)(GB_CB_CHANGED|GB_CB_DELETE), field_changed_cb, (int*)this);
    return error;
}
//  ---------------------------------------------------
//      void awt_input_handler::remove_callbacks()
//  ---------------------------------------------------
GB_ERROR awt_input_handler::remove_db_callbacks() {
    GB_ERROR error = awt_linked_to_item::remove_db_callbacks();
    if (item() && gbd) error = GB_remove_callback(gbd, (GB_CB_TYPE)(GB_CB_CHANGED|GB_CB_DELETE), field_changed_cb, (int*)this);
    return error;
}

//  ---------------------------------------------------------------------------------------------------------------------------------------------
//      awt_input_handler::awt_input_handler(awt_input_mask_global *global_, const string& child_path_, GB_TYPES type_, const string& label_)
//  ---------------------------------------------------------------------------------------------------------------------------------------------
awt_input_handler::awt_input_handler(awt_input_mask_global *global_, const string& child_path_, GB_TYPES type_, const string& label_)
    : awt_viewport(global_, generate_baseName(global_, child_path_), "", false, label_)
    , gbd(0)
    , child_path(child_path_)
    , db_type(type_)
    , in_destructor(false)
{
}

//  ------------------------------------------------
//      awt_input_handler::~awt_input_handler()
//  ------------------------------------------------
awt_input_handler::~awt_input_handler() {
    in_destructor = true;
    unlink();
}

//  -----------------------------------------------------------------
//      GB_ERROR awt_input_handler::link_to(GBDATA *gb_new_item)
//  -----------------------------------------------------------------
GB_ERROR awt_input_handler::link_to(GBDATA *gb_new_item) {
    GB_ERROR       error = 0;
    GB_transaction dummy(mask_global()->get_gb_main());

    remove_awar_callbacks(); // unbind awar callbacks temporarily

    if (item()) {
        remove_db_callbacks();  // ignore result (if handled db-entry was deleted, it returns an error)
        set_item(0);
        gbd = 0;
    }
    else {
        awt_assert(!gbd);
    }

    if (!gb_new_item && !in_destructor) { // crashes if we are in ~awt_input_handler
        db_changed();
    }

    if (!error && gb_new_item) {
        set_item(gb_new_item);
        gbd = GB_search(item(), child_path.c_str(), GB_FIND);

        db_changed();           // synchronize AWAR with DB-entry

        error = add_db_callbacks();
    }

    add_awar_callbacks(); // rebind awar callbacks

    return error;
}

// -end- of implementation of class awt_input_handler.

//  ------------------------------------------------
//      void awt_string_handler::awar_changed()
//  ------------------------------------------------
void awt_string_handler::awar_changed() {
    GBDATA   *gbd       = data();
    GBDATA   *gb_main   = mask_global()->get_gb_main();
    bool      relink_me = false;
    GB_ERROR  error     = 0;

    GB_push_transaction(gb_main);

    if (!mask_global()->edit_allowed()) error = "Editing is disabled. Check the 'Enable edit' switch!";

    if (!error && !gbd) {
        const char *child   = get_child_path().c_str();
        const char *keypath = mask_global()->get_selector()->getKeyPath();

        if (item()) {
            gbd = GB_search(item(), child, GB_FIND);

            if (!gbd) {
                GB_TYPES found_typ = awt_get_type_of_changekey(gb_main, child, keypath);
                if (found_typ != GB_NONE) set_type(found_typ); // fix type if different

                gbd = GB_search(item(), child, type()); // here new items are created
                if (found_typ == GB_NONE) awt_add_new_changekey_to_keypath(gb_main, child, type(), keypath);
                relink_me = true; //  @@@ only if child was created!!
            }
        }
        else {
            mask_global()->no_item_selected();
            aw_message(GBS_global_string("This had no effect, because no %s is selected",
                                         awt_itemtype_names[mask_global()->get_itemtype()]));
        }
    }

    if (!error && gbd) {
        char     *content   = awar()->read_string();
        GB_TYPES  found_typ = GB_read_type(gbd);
        if (found_typ != type()) set_type(found_typ); // fix type if different

        error = GB_write_as_string(gbd, awar2db(content).c_str());

        free(content);
    }

    if (error) {
        aw_message(error);
        GB_abort_transaction(gb_main);
        db_changed(); // write back old value
    }
    else {
        GB_pop_transaction(gb_main);
    }

    if (relink_me) relink();
}
//  ----------------------------------------------
//      void awt_string_handler::db_changed()
//  ----------------------------------------------
void awt_string_handler::db_changed() {
    GBDATA *gbd = data();
    if (gbd) { // gbd may be zero, if field does not exist
        GB_transaction  dummy(mask_global()->get_gb_main());
        char           *content = GB_read_as_string(gbd);
        awar()->write_string(db2awar(content).c_str());
        free(content);
    }
    else {
        awar()->write_string(default_value.c_str());
    }
}

// ###############################
// ###############################
// ###                         ###
// ##          Widgets          ##
// ###                         ###
// ###############################
// ###############################

//  -----------------------------------------------------------
//      void awt_input_field::build_widget(AW_window *aws)
//  -----------------------------------------------------------
void awt_input_field::build_widget(AW_window *aws) {
    const string& lab = get_label();
    if (lab.length()) aws->label(lab.c_str());

    aws->create_input_field(awar_name().c_str(), field_width);
}

// start of implementation of class awt_text_viewport:

//  ------------------------------------------------------------
//      void awt_text_viewport::build_widget(AW_window *aws)
//  ------------------------------------------------------------
void awt_text_viewport::build_widget(AW_window *aws)
{
    const string& lab = get_label();
    if (lab.length()) aws->label(lab.c_str());

    aws->create_input_field(awar_name().c_str(), field_width);
}

// -end- of implementation of class awt_text_viewport.

//  ---------------------------------------------------------
//      void awt_check_box::build_widget(AW_window *aws)
//  ---------------------------------------------------------
void awt_check_box::build_widget(AW_window *aws) {
    const string& lab = get_label();
    if (lab.length()) aws->label(lab.c_str());

    aws->create_toggle(awar_name().c_str());
}
//  ------------------------------------------------------------
//      void awt_radio_button::build_widget(AW_window *aws)
//  ------------------------------------------------------------
void awt_radio_button::build_widget(AW_window *aws) {
    const string& lab = get_label();
    if (lab.length()) aws->label(lab.c_str());

    aws->create_toggle_field(awar_name().c_str(), vertical ? 0 : 1);

    vector<string>::const_iterator b   = buttons.begin();
    vector<string>::const_iterator v   = values.begin();
    int                            pos = 0;

    for (; b != buttons.end() && v != values.end(); ++b, ++v, ++pos) {
        void (AW_window::*ins_togg)(AW_label, const char*, const char*);
        ins_togg = (pos == default_position) ? &AW_window::insert_default_toggle : &AW_window::insert_toggle;

        (aws->*ins_togg)(b->c_str(), mask_global()->hotkey(*b), b->c_str());
    }

    awt_assert(b == buttons.end() && v == values.end());

    aws->update_toggle_field();
}

// ########################################################
// ########################################################
// ###                                                  ###
// ##          Special AWAR <-> DB translations          ##
// ###                                                  ###
// ########################################################
// ########################################################

//  ------------------------------------------------------------------
//      string awt_check_box::awar2db(const string& awar_content)
//  ------------------------------------------------------------------
string awt_check_box::awar2db(const string& awar_content) const {
    GB_TYPES typ = type();

    if (awar_content == "yes") {
        if (typ == GB_STRING) return "yes";
        return "1";
    }
    else {
        if (typ == GB_STRING) return "no";
        return "0";
    }
}
//  ----------------------------------------------------------------
//      string awt_check_box::db2awar(const string& db_content)
//  ----------------------------------------------------------------
string awt_check_box::db2awar(const string& db_content) const {
    if (db_content == "yes" || db_content == "1") return "yes";
    if (db_content == "no" || db_content == "0") return "no";
    return atoi(db_content.c_str()) ? "yes" : "no";
}

//  ---------------------------------------------------------------------
//      string awt_radio_button::awar2db(const string& awar_content)
//  ---------------------------------------------------------------------
string awt_radio_button::awar2db(const string& awar_content) const {
    vector<string>::const_iterator b   = buttons.begin();
    vector<string>::const_iterator v   = values.begin();
    for (; b != buttons.end() && v != values.end(); ++b, ++v) {
        if (*b == awar_content) {
            return *v;
        }
    }

    return string("Unknown awar_content '")+awar_content+"'";
}
//  -------------------------------------------------------------------
//      string awt_radio_button::db2awar(const string& db_content)
//  -------------------------------------------------------------------
string awt_radio_button::db2awar(const string& db_content) const {
    vector<string>::const_iterator b   = buttons.begin();
    vector<string>::const_iterator v   = values.begin();
    for (; b != buttons.end() && v != values.end(); ++b, ++v) {
        if (*v == db_content) {
            return *b;
        }
    }
    return buttons[default_position];
}

//  ----------------------------------------------------------------------------------
//      string awt_numeric_input_field::awar2db(const string& awar_content) const
//  ----------------------------------------------------------------------------------
string awt_numeric_input_field::awar2db(const string& awar_content) const {
    long val = strtol(awar_content.c_str(), 0, 10);

    // correct numeric value :
    if (val<min) val = min;
    if (val>max) val = max;

    return GBS_global_string("%li", val);
}

// ########################################################
// ########################################################
// ###                                                  ###
// ##          Routines to parse user-mask file          ##
// ###                                                  ###
// ########################################################
// ########################################################

//  -------------------------------------------------------------------------
//      static GB_ERROR readLine(FILE *in, string& line, size_t& lineNo)
//  -------------------------------------------------------------------------
static GB_ERROR readLine(FILE *in, string& line, size_t& lineNo) {
    const int  BUFSIZE = 8000;
    char       buffer[BUFSIZE];
    char      *res     = fgets(&buffer[0], BUFSIZE-1, in);
    GB_ERROR   error   = 0;

    if (int err = ferror(in)) {
        error = strerror(err);
    }
    else if (res == 0) {
        error = "Unexpected end of file (@MASK_BEGIN or @MASK_END missing?)";
    }
    else {
        awt_assert(res == buffer);
        res += strlen(buffer);
        if (res>buffer) {
            size_t last = res-buffer;
            if (buffer[last-1] == '\n') {
                buffer[last-1] = 0;
            }
        }
        line = buffer;
        lineNo++; // increment lineNo

        size_t last = line.find_last_not_of(" \t");
        if (last != string::npos && line[last] == '\\') {
            string next;
            error = readLine(in, next, lineNo);
            line  = line.substr(0, last)+' '+next;
        }
    }

    if (error) line = "";

    return error;
}

//  -----------------------------------------------------------------------
//      inline size_t next_non_white(const string& line, size_t start)
//  -----------------------------------------------------------------------
inline size_t next_non_white(const string& line, size_t start) {
    if (start == string::npos) return string::npos;
    return line.find_first_not_of(" \t", start);
}

static bool was_last_parameter = false;

//  --------------------------------------------------------------------------------------------
//      inline size_t eat_para_separator(const string& line, size_t start, GB_ERROR& error)
//  --------------------------------------------------------------------------------------------
inline size_t eat_para_separator(const string& line, size_t start, GB_ERROR& error) {
    size_t para_sep = next_non_white(line, start);

    if (para_sep == string::npos) {
        error = "',' or ')' expected after parameter";
    }
    else {
        switch (line[para_sep]) {
            case ')' :
                was_last_parameter = true;
                break;

            case ',' :
                break;

            default :
                error = "',' or ')' expected after parameter";
                break;
        }
        if (!error) para_sep++;
    }

    return para_sep;
}

//  ---------------------------------------------------------------------------------
//      static void check_last_parameter(GB_ERROR& error, const string& command)
//  ---------------------------------------------------------------------------------
static void check_last_parameter(GB_ERROR& error, const string& command) {
    if (!error && !was_last_parameter) {
        error = GBS_global_string("Superfluous arguments to '%s'", command.c_str());
    }
}

//  ---------------------------------------------------------------------------------------------------------------------
//      static void check_no_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, const string& command)
//  ---------------------------------------------------------------------------------------------------------------------
static void check_no_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, const string& command) {
    size_t start = next_non_white(line, scan_pos);
    scan_pos     = start;

    if (start == string::npos) {
        error = "')' expected";
    }
    else if (line[start] != ')') {
        was_last_parameter = false;
        check_last_parameter(error, command);
    }
    else { // ok
        scan_pos++;
    }
}

//  --------------------------------------------------
//      inline GB_ERROR decode_escapes(string& s)
//  --------------------------------------------------
inline GB_ERROR decode_escapes(string& s) {
    string::iterator f = s.begin();
    string::iterator t = s.begin();

    for (; f != s.end(); ++f, ++t) {
        if (*f == '\\') {
            ++f;
            if (f == s.end()) return GBS_global_string("Trailing \\ in '%s'", s.c_str());
        }
        *t = *f;
    }

    s.erase(t, f);

    return 0;
}

//  ---------------------------------------------------------------------------------------------------
//      static string scan_string_parameter(const string& line, size_t& scan_pos, GB_ERROR& error)
//  ---------------------------------------------------------------------------------------------------
static string scan_string_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, bool allow_escaped = false) {
    string result;
    size_t open_quote = next_non_white(line, scan_pos);
    scan_pos          = open_quote;

    if (open_quote == string::npos || line[open_quote] != '\"') {
        error = "string parameter expected";
    }
    else {
        size_t close_quote = string::npos;

        if (allow_escaped) {
            size_t start = open_quote+1;

            while (1) {
                close_quote = line.find_first_of("\\\"", start);

                if (close_quote == string::npos) break; // error
                if (line[close_quote] == '\"') break; // found closing quote

                if (line[close_quote] == '\\') { // escape next char
                    close_quote++;
                }
                start = close_quote+1;
            }
        }
        else {
            close_quote = line.find('\"', open_quote+1);
        }

        if (close_quote == string::npos) {
            error = "string parameter missing closing '\"'";
        }
        else {
            result = line.substr(open_quote+1, close_quote-open_quote-1);
            if (allow_escaped) {
                awt_assert(!error);
                error = decode_escapes(result);
            }
            if (!error) scan_pos = eat_para_separator(line, close_quote+1, error);
        }
    }

    return result;
}

//  ------------------------------------------------------------
//      string list_keywords(const char **allowed_keywords)
//  ------------------------------------------------------------
string list_keywords(const char **allowed_keywords) {
    string result;
    for (int i = 0; allowed_keywords[i]; ++i) {
        if (i) {
            if (allowed_keywords[i+1]) result += ", ";
            else result                       += " or ";
        }
        result += allowed_keywords[i];
    }
    return result;
}

//  -------------------------------------------------------------------
//      inline int isKeyword(const char *current, const char *keyword)
//  -------------------------------------------------------------------
inline int isKeyword(const char *current, const char *keyword) {
    int pos = 0;
    for (; keyword[pos]; ++pos) {
        if (!current[pos] || std::tolower(current[pos]) != std::tolower(keyword[pos])) {
            return 0;
        }
    }
    return pos;
}
//  --------------------------------------------------------------------------------------------------------------------------------
//      static int scan_keyword_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, const char **allowed_keywords)
//  --------------------------------------------------------------------------------------------------------------------------------
// return index of keyword (or -1)
// allowed_keywords has to be 0-terminated
static int scan_keyword_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, const char **allowed_keywords) {
    int    result = -1;
    size_t start  = next_non_white(line, scan_pos);
    scan_pos      = start;

    awt_assert(!error);

    if (start == string::npos) {
EXPECTED :
        string keywords = list_keywords(allowed_keywords);
        error           = GBS_global_string("%s expected", keywords.c_str());
    }
    else {
        const char *current = line.c_str()+start;

        int i = 0;
        for (; allowed_keywords[i]; ++i) {
            int found_keyword_size = isKeyword(current, allowed_keywords[i]);
            if (found_keyword_size) {
                result    = i;
                scan_pos += found_keyword_size;
                break;
            }
        }
        if (!allowed_keywords[i]) goto EXPECTED;
        awt_assert(!error);
        scan_pos = eat_para_separator(line, scan_pos, error);
    }
    return result;
}

//  -----------------------------------------------------------------------------------------------------------
//      static void scan_string_or_keyword_parameter(const string& line, size_t& scan_pos, GB_ERROR& error,
//  -----------------------------------------------------------------------------------------------------------
static void scan_string_or_keyword_parameter(const string& line, size_t& scan_pos, GB_ERROR& error,
                                             string& string_para, int& keyword_index, // result parameters
                                             const char **allowed_keywords) {
    // if keyword_index != -1 -> keyword found
    //                  == -1 -> string_para contains string-parameter

    awt_assert(!error);

    string_para = scan_string_parameter(line, scan_pos, error);
    if (!error) {
        keyword_index = -1;
    }
    else {                      // no string
        error         = 0;      // ignore error - test for keywords
        keyword_index = scan_keyword_parameter(line, scan_pos, error, allowed_keywords);
        if (keyword_index == -1) { // no keyword
            error = GBS_global_string("string parameter or %s", error);
        }
    }
}

//  -----------------------------------------------------------------------------------------------
//      static long scan_long_parameter(const string& line, size_t& scan_pos, GB_ERROR& error)
//  -----------------------------------------------------------------------------------------------
static long scan_long_parameter(const string& line, size_t& scan_pos, GB_ERROR& error) {
    int    result = 0;
    size_t start  = next_non_white(line, scan_pos);
    bool   neg    = false;

    awt_assert(!error);

    while (start != string::npos) {
        char c = line[start];
        if (c != '+' && c != '-') break;

        start             = next_non_white(line, start+1);
        if (c == '-') neg = !neg;
        continue;
    }

    if (start == string::npos || !isdigit(line[start]) ) {
        scan_pos = start;
        error    = "digits (or+-) expected";
    }
    else {
        size_t end = line.find_first_not_of("0123456789", start);
        scan_pos   = eat_para_separator(line, end, error);
        if (!error) {
            awt_assert(end != string::npos);
            result = atoi(line.substr(start, end-start+1).c_str());
        }
    }

    return neg ? -result : result;
}
//  -------------------------------------------------------------------------------------------------------------------
//      static long scan_long_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, long min, long max)
//  -------------------------------------------------------------------------------------------------------------------
// with range-check
static long scan_long_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, long min, long max) {
    awt_assert(!error);
    size_t old_scan_pos = scan_pos;
    long   result       = scan_long_parameter(line, scan_pos, error);
    if (!error) {
        if (result<min || result>max) {
            scan_pos = old_scan_pos;
            error    = GBS_global_string("value %li is outside allowed range (%li-%li)", result, min, max);
        }
    }
    return result;
}

//  ------------------------------------------------------------------------------------------------------------------
//      static long scan_optional_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, long if_empty)
//  ------------------------------------------------------------------------------------------------------------------
static long scan_optional_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, long if_empty) {
    awt_assert(!error);
    size_t old_scan_pos = scan_pos;
    long   result       = scan_long_parameter(line, scan_pos, error);
    if (error) {
        error    = 0;           // ignore and test for empty parameter
        scan_pos = old_scan_pos;
        scan_pos = eat_para_separator(line, scan_pos, error);

        if (error) {
            error    = "expected number or empty parameter";
            scan_pos = old_scan_pos;
        }
        else {
            result = if_empty;
        }
    }
    return result;
}


//  ---------------------------------------------------------------------------------------------------------------------------
//      static int scan_flag_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, const string& allowed_flags)
//  ---------------------------------------------------------------------------------------------------------------------------
// return 0..n-1 ( = position in 'allowed_flags')
// or error is set
static int scan_flag_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, const string& allowed_flags) {
    awt_assert(!error);
    int    result = 0;
    size_t start  = next_non_white(line, scan_pos);
    scan_pos      = start;

    if (start == string::npos) {
        error    = GBS_global_string("One of '%s' expected", allowed_flags.c_str());
    }
    else {
        char   found       = line[start];
        char   upper_found = std::toupper(found);
        size_t pos         = allowed_flags.find(upper_found);

        if (pos != string::npos) {
            result   = pos;
            scan_pos = eat_para_separator(line, start+1, error);
        }
        else {
            error = GBS_global_string("One of '%s' expected (found '%c')", allowed_flags.c_str(), found);
        }
    }
    return result;
}
//  -----------------------------------------------------------------------------------------------
//      static bool scan_bool_parameter(const string& line, size_t& scan_pos, GB_ERROR& error)
//  -----------------------------------------------------------------------------------------------
static bool scan_bool_parameter(const string& line, size_t& scan_pos, GB_ERROR& error) {
    awt_assert(!error);
    size_t old_scan_pos = scan_pos;
    long   result       = scan_long_parameter(line, scan_pos, error);

    if (!error && (result != 0) && (result != 1)) {
        scan_pos = old_scan_pos;
        error    = "'0' or '1' expected";
    }
    return result != 0;
}

//  ---------------------------------------------------------------------------------------------
//      static string scan_identifier(const string& line, size_t& scan_pos, GB_ERROR& error)
//  ---------------------------------------------------------------------------------------------
static string scan_identifier(const string& line, size_t& scan_pos, GB_ERROR& error) {
    string id;
    size_t start = next_non_white(line, scan_pos);
    if (start == string::npos) {
        error = "identifier expected";
    }
    else {
        size_t end = start;
        while (end<line.length()) {
            char c                 = line[end];
            if (!(isalnum(c) || c == '_')) break;
            ++end;
        }
        id         = line.substr(start, end-start);
        scan_pos   = eat_para_separator(line, end, error);
    }
    return id;
}

// //  -----------------------------------------------------------------------------------------------------------------------------------------
// //      static string scan_unused_identifier(const awt_input_mask_global *global, const string& line, size_t& scan_pos, GB_ERROR& error)
// //  -----------------------------------------------------------------------------------------------------------------------------------------
// static string scan_unused_identifier(const awt_input_mask_global *global, const string& line, size_t& scan_pos, GB_ERROR& error) {
//     string id = scan_identifier(line, scan_pos, error);
//     if (!error && global->has_id(id)) error = GB_export_error("ID '%s' declared twice", id.c_str());
//     return id;
// }

// //  ---------------------------------------------------------------------------------------------------------------------------------------
// //      static string scan_used_identifier(const awt_input_mask_global *global, const string& line, size_t& scan_pos, GB_ERROR& error)
// //  ---------------------------------------------------------------------------------------------------------------------------------------
// static string scan_used_identifier(const awt_input_mask_global *global, const string& line, size_t& scan_pos, GB_ERROR& error) {
//     string id                               = scan_identifier(line, scan_pos, error);
//     if (!error && !global->has_id(id)) error = GB_export_error("Undeclared ID '%s' referenced", id.c_str());
//     // @@@ FIXME: if not declared -> store in list and test after parsing mask
//     return id;
// }

// -----------------------------------------------------
//      inline const char *inputMaskDir(bool local)
// -----------------------------------------------------
inline const char *inputMaskDir(bool local) {
    if (local) {
        static char *local_mask_dir;
        if (!local_mask_dir) local_mask_dir = AWT_unfold_path(".arb_prop/inputMasks", "HOME");
        return local_mask_dir;
    }

    static char *global_mask_dir;
    if (!global_mask_dir) global_mask_dir = AWT_unfold_path("lib/inputMasks", "ARBHOME");
    return global_mask_dir;
}

//  -----------------------------------------------------------------------------
//      inline string inputMaskFullname(const string& mask_name, bool local)
//  -----------------------------------------------------------------------------
inline string inputMaskFullname(const string& mask_name, bool local) {
    const char *dir = inputMaskDir(local);
    return string(dir)+'/'+mask_name;
}

#define ARB_INPUT_MASK_ID "ARB-Input-Mask"

//  -----------------------------------------------------------------------------------------------------------------------------
//      static awt_input_mask_descriptor *quick_scan_input_mask(const string& mask_name, const string& filename, bool local)
//  -----------------------------------------------------------------------------------------------------------------------------
static awt_input_mask_descriptor *quick_scan_input_mask(const string& mask_name, const string& filename, bool local) {
    FILE     *in     = fopen(filename.c_str(), "rt");
    size_t    lineNo = 0;
    GB_ERROR  error  = 0;
    int       hidden = 0; // defaults to 'not hidden'

    if (in) {
        string   line;
        error = readLine(in, line, lineNo);

        if (!error && line == ARB_INPUT_MASK_ID) {
            bool   done = false;
            string title;
            string itemtype;

            while (!feof(in)) {
                error = readLine(in, line, lineNo);
                if (error) break;

                if (line[0] == '#') continue; // ignore comments
                if (line == "@MASK_BEGIN") { done = true; break; }

                size_t at = line.find('@');
                size_t eq = line.find('=', at);

                if (at == string::npos || eq == string::npos)  {
                    continue; // ignore all other lines
                }
                else {
                    string parameter = line.substr(at+1, eq-at-1);
                    string value     = line.substr(eq+1);

                    if (parameter == "ITEMTYPE") itemtype = value;
                    else if (parameter == "TITLE") title  = value;
                    else if (parameter == "HIDE") hidden  = atoi(value.c_str());
                }
            }

            if (!error && done) {
                if (itemtype == "") {
                    error = "No itemtype defined";
                }
                else {
                    if (title == "") title = mask_name;
                    return new awt_input_mask_descriptor(title.c_str(), mask_name.c_str(), itemtype.c_str(), local, hidden);
                }
            }
        }
    }

    if (error) {
        aw_message(GBS_global_string("%s (while scanning user-mask '%s')", error, filename.c_str()));
    }

#if defined(DEBUG)
    printf("Skipping '%s' (not a input mask)\n", filename.c_str());
#endif // DEBUG
    return 0;
}

//  ----------------------------------------------------------------------------------------
//      static void AWT_edit_input_mask(AW_window *aww, AW_CL cl_mask_name, bool local)
//  ----------------------------------------------------------------------------------------
static void AWT_edit_input_mask(AW_window *aww, AW_CL cl_mask_name, AW_CL cl_local) {
    const string *mask_name = (const string *)cl_mask_name;
    string        fullmask  = inputMaskFullname(*mask_name, (bool)cl_local);
    awt_edit(aww->get_root(), fullmask.c_str());
}

//  ---------------------------------
//      input mask container :
//  ---------------------------------
typedef SmartPtr<awt_input_mask>        awt_input_mask_ptr;
typedef map<string, awt_input_mask_ptr> InputMaskList; // contains all active masks
static InputMaskList                    input_mask_list;

//  ---------------------------------------------------------------------------------
//      static void awt_input_mask_awar_changed_cb(AW_root *root, AW_CL cl_mask)
//  ---------------------------------------------------------------------------------
static void awt_input_mask_awar_changed_cb(AW_root */*root*/, AW_CL cl_mask) {
    awt_input_mask *mask = (awt_input_mask*)(cl_mask);
    mask->relink();
}
//  -------------------------------------------------------------------
//      static void link_mask_to_database(awt_input_mask_ptr mask)
//  -------------------------------------------------------------------
static void link_mask_to_database(awt_input_mask_ptr mask) {
    awt_input_mask_global         *global = mask->mask_global();
    const awt_item_type_selector  *sel    = global->get_selector();
    AW_root                       *root   = global->get_root();

    sel->add_awar_callbacks(root, awt_input_mask_awar_changed_cb, (AW_CL)(&*mask));
    awt_input_mask_awar_changed_cb(root, (AW_CL)(&*mask));
}
//  -----------------------------------------------------------------------
//      static void unlink_mask_from_database(awt_input_mask_ptr mask)
//  -----------------------------------------------------------------------
static void unlink_mask_from_database(awt_input_mask_ptr mask) {
    awt_input_mask_global        *global = mask->mask_global();
    const awt_item_type_selector *sel    = global->get_selector();
    AW_root                      *root   = global->get_root();

    sel->remove_awar_callbacks(root, awt_input_mask_awar_changed_cb, (AW_CL)(&*mask));
}

//  --------------------------------------------------------
//      inline bool isInternalMaskName(const string& s)
//  --------------------------------------------------------
inline bool isInternalMaskName(const string& s) {
    return s[0] == '0' || s[0] == '1';
}

//  --------------------------------------------------------------------------------------------------------------------------------------------
//      static void awt_open_input_mask(AW_window *aww, AW_CL cl_internal_mask_name, AW_CL cl_mask_to_open, bool reload, bool hide_current)
//  --------------------------------------------------------------------------------------------------------------------------------------------
static void awt_open_input_mask(AW_window *aww, AW_CL cl_internal_mask_name, AW_CL cl_mask_to_open, bool reload, bool hide_current) {
    const string            *internal_mask_name = (const string *)cl_internal_mask_name;
    const string            *mask_to_open       = (const string *)cl_mask_to_open;
    InputMaskList::iterator  mask_iter          = input_mask_list.find(*internal_mask_name);

    awt_assert(internal_mask_name && isInternalMaskName(*internal_mask_name));
    awt_assert(mask_to_open && isInternalMaskName(*mask_to_open));

    if (mask_iter != input_mask_list.end()) {
        awt_input_mask_ptr     mask   = mask_iter->second;
        awt_input_mask_global *global = mask->mask_global();

        printf("aww=%p root=%p ; global=%p root=%p\n", aww, aww->get_root(), global, global->get_root());
        awt_assert(aww->get_root() == global->get_root());

        if (reload) mask->set_reload_on_reinit(true);
        if (hide_current) mask->hide();
        // @@@ hier sollte nicht der Selector der alten Maske verwendet werden, sondern anhand des Typs ein
        // Selector ausgewaehlt werden. Dazu muessen jedoch alle Selectoren registriert werden.
        GB_ERROR error = AWT_initialize_input_mask(global->get_root(), global->get_gb_main(), global->get_selector(), mask_to_open->c_str(), global->is_local_mask());
        // CAUTION: AWT_initialize_input_mask invalidates mask and mask_iter
        if (error && hide_current) {
            mask_iter = input_mask_list.find(*internal_mask_name);
            awt_assert(mask_iter != input_mask_list.end());
            mask_iter->second->show();
        }
    }
#if defined(DEBUG)
    else {
        string mask_name = internal_mask_name->substr(1);
        printf("'%s' (no such mask in input_mask_list)\n", mask_name.c_str());
        awt_assert(0);
    }
#endif // DEBUG
}

static void AWT_reload_input_mask(AW_window *aww, AW_CL cl_internal_mask_name) {
    awt_open_input_mask(aww, cl_internal_mask_name, cl_internal_mask_name, true, true);
}
static void AWT_open_input_mask(AW_window *aww, AW_CL cl_internal_mask_name, AW_CL cl_mask_to_open) {
    awt_open_input_mask(aww, cl_internal_mask_name, cl_mask_to_open, false, false);
}
static void AWT_change_input_mask(AW_window *aww, AW_CL cl_internal_mask_name, AW_CL cl_mask_to_open) {
    awt_open_input_mask(aww, cl_internal_mask_name, cl_mask_to_open, false, true);
}



//  ------------------------------
//      class awt_mask_action
//  ------------------------------
// something that is performed i.e. when user pressed a mask button
// used as callback parameter
class awt_mask_action {
private:
    virtual GB_ERROR action() = 0;
protected:
    awt_input_mask_ptr mask;
public:
    awt_mask_action(awt_input_mask_ptr mask_) : mask(mask_) {}
    virtual ~awt_mask_action() {}

    void perform_action() {
        GB_ERROR error = action();
        if (error) aw_message(error);
    }
};


//  ------------------------------------------------------
//      class awt_assignment : public awt_mask_action
//  ------------------------------------------------------
class awt_assignment : public awt_mask_action {
private:
    string id_dest;
    string id_source;

    virtual GB_ERROR action() {
        GB_ERROR             error       = 0;
        const awt_mask_item *item_source = mask->mask_global()->get_identified_item(id_source, error);
        awt_mask_item       *item_dest   = mask->mask_global()->get_identified_item(id_dest, error);

        if (!error) error = item_dest->set_value(item_source->get_value());

        return error;
    }
public:
    awt_assignment(awt_input_mask_ptr mask_, const string& id_dest_, const string& id_source_)
        : awt_mask_action(mask_)
        , id_dest(id_dest_)
        , id_source(id_source_)
    {}
    virtual ~awt_assignment() {}
};

//  -------------------------------------------------------------------------------------------------------
//      static void AWT_input_mask_perform_action(AW_window */*aww*/, AW_CL cl_awt_mask_action, AW_CL)
//  -------------------------------------------------------------------------------------------------------
static void AWT_input_mask_perform_action(AW_window */*aww*/, AW_CL cl_awt_mask_action, AW_CL) {
    awt_mask_action *action = (awt_mask_action*)cl_awt_mask_action;
    action->perform_action();
}

static void AWT_input_mask_browse_url(AW_window *aww, AW_CL cl_url_srt, AW_CL cl_mask_ptr) {
    AW_root                      *root     = aww->get_root();
    const string                 *url_srt  = (const string *)cl_url_srt;
    const awt_input_mask         *mask     = (const awt_input_mask *)cl_mask_ptr;
    const awt_input_mask_global  *global   = mask->mask_global();
    const awt_item_type_selector *selector = global->get_selector();
    GBDATA                       *gbd      = selector->current(root);

    if (!gbd) {
        aw_message(GBS_global_string("You have to select a %s first", awt_itemtype_names[selector->get_item_type()]));
    }
    else {
        char     *name  = root->awar(selector->get_self_awar())->read_string();
        GB_ERROR  error = awt_open_ACISRT_URL_by_gbd(root, global->get_gb_main(), gbd, name, url_srt->c_str());
        if (error) aw_message(error);
        free(name);
    }
}

// static void AWT_input_mask_do_assign(AW_window */*aww*/, AW_CL cl_awt_assignment, AW_CL cl_mask_ptr) {
//     const awt_assignment *assign      = (awt_assignment*)cl_awt_assignment;
//     const awt_input_mask *mask        = (const awt_input_mask *)cl_mask_ptr;
//     GB_ERROR              error       = 0;
//     const awt_mask_item  *item_source = mask->mask_global()->get_identified_item(assign->IdSource(), error);
//     awt_mask_item        *item_dest   = mask->mask_global()->get_identified_item(assign->IdDest(), error);

//     if (!error) error = item_dest->set_value(item_source->get_value());

//     if (error) aw_message(error);
// }


// ##########################################
// ##########################################
// ###                                    ###
// ##          User Mask Commands          ##
// ###                                    ###
// ##########################################
// ##########################################

enum MaskCommand  {
    CMD_TEXTFIELD,
    CMD_NUMFIELD,
    CMD_CHECKBOX,
    CMD_RADIO,
    CMD_OPENMASK,
    CMD_CHANGEMASK,
    CMD_TEXT,
    CMD_SELF,
    CMD_WWW,
    CMD_NEW_LINE,
    CMD_NEW_SECTION,
    CMD_ID,
    CMD_GLOBAL,
    CMD_LOCAL,
    CMD_SHOW,
    CMD_ASSIGN,
    CMD_SCRIPT,

    MASK_COMMANDS,
    CMD_UNKNOWN = MASK_COMMANDS
};

struct MaskCommandDefinition {
    const char  *cmd_name;
    MaskCommand  cmd;
};

static struct MaskCommandDefinition mask_command[MASK_COMMANDS+1] =
{
 { "TEXTFIELD", CMD_TEXTFIELD },
 { "NUMFIELD", CMD_NUMFIELD },
 { "CHECKBOX", CMD_CHECKBOX },
 { "RADIO", CMD_RADIO },
 { "OPENMASK", CMD_OPENMASK },
 { "CHANGEMASK", CMD_CHANGEMASK },
 { "TEXT", CMD_TEXT },
 { "SELF", CMD_SELF },
 { "NEW_LINE", CMD_NEW_LINE },
 { "NEW_SECTION", CMD_NEW_SECTION },
 { "WWW", CMD_WWW },
 { "ID", CMD_ID },
 { "GLOBAL", CMD_GLOBAL },
 { "LOCAL", CMD_LOCAL },
 { "SHOW", CMD_SHOW },
 { "ASSIGN", CMD_ASSIGN },
 { "SCRIPT", CMD_SCRIPT },

 { 0, CMD_UNKNOWN}
};

//  ---------------------------------------------------------------
//      inline MaskCommand findCommand(const string& cmd_name)
//  ---------------------------------------------------------------
inline MaskCommand findCommand(const string& cmd_name) {
    int mc = 0;

    for (; mask_command[mc].cmd != CMD_UNKNOWN; ++mc) {
        if (mask_command[mc].cmd_name == cmd_name) {
            return mask_command[mc].cmd;
        }
    }
    return CMD_UNKNOWN;
}

//  -----------------------------------------------------------------------------------------------------------
//      static void parse_CMD_RADIO(string& line, size_t& scan_pos, GB_ERROR& error, const string& command,
//  -----------------------------------------------------------------------------------------------------------
static void parse_CMD_RADIO(string& line, size_t& scan_pos, GB_ERROR& error, const string& command,
                            awt_mask_item_ptr& handler1, awt_mask_item_ptr& handler2, awt_input_mask_global *global) {
    string         label, data_path;
    int            default_position = -1, orientation = -1;
    vector<string> buttons;
    vector<string> values;
    bool           allow_edit       = false;
    int            width            = -1;
    int            edit_position    = -1;

    label                        = scan_string_parameter(line, scan_pos, error);
    if (!error) data_path        = scan_string_parameter(line, scan_pos, error);
    if (!error) default_position = scan_long_parameter(line, scan_pos, error);
    if (!error) {
        orientation = scan_flag_parameter(line, scan_pos, error, "HVXY");
        orientation = orientation&1;
    }
    while (!error && !was_last_parameter) {
        string but = scan_string_parameter(line, scan_pos, error);
        string val = "";
        if (!error) {
            int keyword_index;
            const char *allowed_keywords[] = { "ALLOW_EDIT", 0};
            scan_string_or_keyword_parameter(line, scan_pos, error, val, keyword_index, allowed_keywords);

            if (!error) {
                if (keyword_index != -1) { // found keyword
                    if (allow_edit) error = "ALLOW_EDIT is allowed only once for each RADIO";

                    if (!error) width = scan_long_parameter(line, scan_pos, error, MIN_TEXTFIELD_SIZE, MAX_TEXTFIELD_SIZE);
                    if (!error) val   = scan_string_parameter(line, scan_pos, error);

                    if (!error) {
                        allow_edit    = true;
                        edit_position = buttons.size()+1; // positions are 1..n
                        buttons.push_back(but);
                        values.push_back(val);
                    }
                }
                else { // found string
                    buttons.push_back(but);
                    values.push_back(val);
                }
            }
        }
    }
    check_last_parameter(error, command);

    if (!error && (buttons.size()<2)) error = "You have to define at least 2 buttons.";

    if (!error && allow_edit && edit_position != default_position) {
        error = GBS_global_string("Invalid default %i (must be index of ALLOW_EDIT: %i )", default_position, edit_position);
    }
    if (!error && (default_position<1 || size_t(default_position)>buttons.size())) {
        error = GBS_global_string("Invalid default %i (valid: 1..%i)", default_position, buttons.size());
    }

    if (!error) {
        if (allow_edit) {
            // create radio-button + textfield
            handler1 = new awt_radio_button(global, data_path, label, default_position-1, orientation, buttons, values);
            handler2 = new awt_input_field(global, data_path, "", width, "", GB_STRING);
        }
        else {
            handler1 = new awt_radio_button(global, data_path, label, default_position-1, orientation, buttons, values);
        }
    }
}

//  ----------------------------------------------------------------------------------------
//      static string find_internal_name(const string& mask_name, bool search_in_local)
//  ----------------------------------------------------------------------------------------
static string find_internal_name(const string& mask_name, bool search_in_local) {
    const char *internal_local  = 0;
    const char *internal_global = 0;

    for (int id = 0; ; ++id) {
        const awt_input_mask_descriptor *descriptor = AWT_look_input_mask(id);
        if (!descriptor) break;

        const char *internal_name = descriptor->get_internal_maskname();

        if (strcmp(internal_name+1, mask_name.c_str()) == 0) {
            if (descriptor->is_local_mask()) {
                awt_assert(internal_local == 0);
                internal_local = internal_name;
            }
            else {
                awt_assert(internal_global == 0);
                internal_global = internal_name;
            }
        }
    }

    return (search_in_local && internal_local) ? internal_local : (internal_global ? internal_global : "");
}

//  -----------------------------------------------------------------------------------
//      class awt_marked_checkbox : public awt_viewport, public awt_linked_to_item
//  -----------------------------------------------------------------------------------
class awt_marked_checkbox : public awt_viewport, public awt_linked_to_item {
private:

    string generate_baseName(awt_input_mask_global *global_) {
        return GBS_global_string("%s/marked", global_->get_maskid().c_str());
    }

public:
    awt_marked_checkbox(awt_input_mask_global *global_, const std::string& label_)
        : awt_viewport(global_, generate_baseName(global_), "0", false, label_)
        , awt_linked_to_item()
    {}
    virtual ~awt_marked_checkbox() {}

    virtual GB_ERROR link_to(GBDATA *gb_new_item); // link to a new item
    virtual GB_ERROR relink() { return link_to(mask_global()->get_selector()->current(mask_global()->get_root())); }
    virtual void awar_changed();
    virtual void db_changed();
    virtual void general_item_change() { db_changed(); } // called if item was changed (somehow)
    virtual void build_widget(AW_window *aws); // builds the widget at the current position
};

//  -------------------------------------------------------------------
//      GB_ERROR awt_marked_checkbox::link_to(GBDATA *gb_new_item)
//  -------------------------------------------------------------------
GB_ERROR awt_marked_checkbox::link_to(GBDATA *gb_new_item) { // link to a new item
    GB_ERROR       error = 0;
    GB_transaction dummy(mask_global()->get_gb_main());

    remove_awar_callbacks();    // unbind awar callbacks temporarily

    if (item()) {
        remove_db_callbacks(); // ignore result (if handled db-entry was deleted, it returns an error)
        set_item(0);
    }

    if (gb_new_item) {
        set_item(gb_new_item);
        db_changed();
        error = add_db_callbacks();
    }

    add_awar_callbacks();       // rebind awar callbacks
    return error;
}
//  -------------------------------------------------
//      void awt_marked_checkbox::awar_changed()
//  -------------------------------------------------
void awt_marked_checkbox::awar_changed() { // called when awar changes
    if (item()) {
        string         value  = get_value();
        bool           marked = value == "yes";
        GB_transaction dummy(mask_global()->get_gb_main());
        GB_write_flag(item(), marked);
    }
    else {
        mask_global()->no_item_selected();
    }
}
//  -----------------------------------------------
//      void awt_marked_checkbox::db_changed()
//  -----------------------------------------------
void awt_marked_checkbox::db_changed() {
    bool marked = false;

    if (item()) {
        GB_transaction dummy(mask_global()->get_gb_main());
        if (GB_read_flag(item())) marked = true;
        set_value(marked ? "yes" : "no"); // @@@ TEST: moved into if (was below)
    }
}
//  ---------------------------------------------------------------
//      void awt_marked_checkbox::build_widget(AW_window *aws)
//  ---------------------------------------------------------------
void awt_marked_checkbox::build_widget(AW_window *aws) { // builds the widget at the current position
    const string& lab = get_label();
    if (lab.length()) aws->label(lab.c_str());

    aws->create_toggle(awar_name().c_str());
}

//  -------------------------------------------------------------------------------------------------------------------------
//      static GB_ERROR writeDefaultMaskfile(const string& fullname, const string& maskname, const string& itemtypename)
//  -------------------------------------------------------------------------------------------------------------------------
static GB_ERROR writeDefaultMaskfile(const string& fullname, const string& maskname, const string& itemtypename, bool hidden) {
    FILE *out = fopen(fullname.c_str(), "wt");
    if (!out) return GBS_global_string("Can't open '%s'", fullname.c_str());

    fprintf(out,
            "%s\n"
            "# New mask '%s'\n\n"
            "# What kind of item to edit:\n"
            "@ITEMTYPE=%s\n\n"
            "# Window title:\n"
            "@TITLE=%s\n\n", ARB_INPUT_MASK_ID, maskname.c_str(), itemtypename.c_str(), maskname.c_str());

    fprintf(out,
            "# Should mask appear in 'User mask' menu\n"
            "@HIDE=%i\n\n", int(hidden));

    fputs("# Spacing in window:\n"
          "@X_SPACING=5\n"
          "@Y_SPACING=3\n\n"
          "# Show edit/reload button?\n"
          "@EDIT=1\n"
          "# Show 'edit enable' toggle?\n"
          "@EDIT_ENABLE=1\n"
          "# Show 'marked' toggle?\n"
          "@SHOW_MARKED=1\n"
          "\n# ---------------------------\n"
          "# The definition of the mask:\n\n"
          "@MASK_BEGIN\n\n"
          "\tTEXT(\"You are editing\") SELF()\n"
          "\tNEW_SECTION()\n"
          "\tTEXTFIELD(\"Full name\", \"full_name\", 30)\n\n"
          "@MASK_END\n\n", out);

    fclose(out);
    return 0;
}
//  --------------------------------------------------------------------------------------------------------------------------
//      static awt_input_mask_ptr awt_create_input_mask(AW_root *root, GBDATA *gb_main, const awt_item_type_selector *sel,
//  --------------------------------------------------------------------------------------------------------------------------
static awt_input_mask_ptr awt_create_input_mask(AW_root *root, GBDATA *gb_main, const awt_item_type_selector *sel,
                                                const string& mask_name, bool local, GB_ERROR& error) {
    size_t             lineNo = 0;
    awt_input_mask_ptr mask;

    error = 0;

    FILE *in = 0;
    {
        string  fullfile = inputMaskFullname(mask_name, local);
        in               = fopen(fullfile.c_str(), "rt");
        if (!in) error   = GBS_global_string("Can't open '%s'", fullfile.c_str());
    }

    if (!error) {
        bool   mask_began = false;
        bool   mask_ended = false;

        // data to be scanned :
        string        title;
        string        itemtypename;
        int           x_spacing   = 5;
        int           y_spacing   = 3;
        bool          edit_reload = false;
        bool          edit_enable = true;
        bool          show_marked = true;
        awt_item_type itemtype;

        string line;
        size_t err_pos = 0;     // 0 = unknown; string::npos = at end of line; else position+1;
        error          = readLine(in, line, lineNo);

        if (!error && line != ARB_INPUT_MASK_ID) {
            error = "'" ARB_INPUT_MASK_ID "' expected";
        }

        while (!error && !mask_began && !feof(in)) {
            error = readLine(in, line, lineNo);
            if (error) break;

            if (line[0] == '#') continue; // ignore comments

            if (line == "@MASK_BEGIN") mask_began = true;
            else  {
                size_t at = line.find('@');
                size_t eq = line.find('=', at);

                if (at == string::npos || eq == string::npos) {
                    continue;
                }
                else {
                    string parameter = line.substr(at+1, eq-at-1);
                    string value     = line.substr(eq+1);

                    if (parameter == "ITEMTYPE")            itemtypename = value;
                    else if (parameter == "TITLE")          title        = value;
                    else if (parameter == "X_SPACING")      x_spacing    = atoi(value.c_str());
                    else if (parameter == "Y_SPACING")      y_spacing    = atoi(value.c_str());
                    else if (parameter == "EDIT")           edit_reload  = atoi(value.c_str()) != 0;
                    else if (parameter == "EDIT_ENABLE")    edit_enable  = atoi(value.c_str()) != 0;
                    else if (parameter == "SHOW_MARKED")    show_marked  = atoi(value.c_str()) != 0;
                    else if (parameter == "HIDE") ; // ignore parameter here
                    else {
                        error = GBS_global_string("Unknown parameter '%s'", parameter.c_str());
                    }
                }
            }
        }

        if (!error && !mask_began) error = "@MASK_BEGIN expected";

        // check data :
        if (!error) {
            if (title == "") title = string("Untitled (")+mask_name+")";
            itemtype = AWT_getItemType(itemtypename);

            if (itemtype == AWT_IT_UNKNOWN)         error = GBS_global_string("Unknown @ITEMTYPE '%s'", itemtypename.c_str());
            if (itemtype != sel->get_item_type())   error = GBS_global_string("Mask is designed for @ITEMTYPE '%s' (current @ITEMTYPE '%s')",
                                                                              itemtypename.c_str(), awt_itemtype_names[sel->get_item_type()]);
        }

        // create mask
        if (!error) mask = new awt_input_mask(root, gb_main, mask_name, itemtype, local, sel, edit_enable);

        // create window
        if (!error) {
            awt_assert(!mask.Null());
            AW_window_simple*& aws = mask->get_window();
            aws                    = new AW_window_simple;
            aws->init(root, "INPUT_MASK", title.c_str());
            aws->load_xfig(0, AW_TRUE);

            aws->recalc_size_at_show = 1; // ignore user size!

            aws->auto_space(x_spacing, y_spacing);
            aws->at_newline();

            aws->callback((AW_CB0)AW_POPDOWN); aws->create_button("CLOSE", "CLOSE", "C");
            aws->callback( AW_POPUP_HELP,(AW_CL)"input_mask.hlp"); aws->create_button("HELP","HELP","H");

            if (edit_reload) {
                aws->callback( AWT_edit_input_mask, (AW_CL)&mask->mask_global()->get_maskname(), (AW_CL)mask->mask_global()->is_local_mask()); aws->create_button("EDIT","EDIT","E");
                aws->callback( AWT_reload_input_mask, (AW_CL)&mask->mask_global()->get_internal_maskname()); aws->create_button("RELOAD","RELOAD","R");
            }

            if (edit_reload && edit_enable && show_marked) aws->at_newline();

            if (edit_enable) {
                aws->label("Enable edit?");
                aws->create_toggle(AWAR_INPUT_MASKS_EDIT_ENABLED);
            }

            if (show_marked) {
                awt_mask_item_ptr handler = new awt_marked_checkbox(mask->mask_global(), "Marked");
                mask->add_handler(handler);
                if (handler->is_viewport()) handler->to_viewport()->build_widget(aws);

            }

            aws->at_newline();

            vector<int> horizontal_lines; // y-positions of horizontal lines
            map<string, size_t> referenced_ids; // all ids that where referenced by the script (size_t contains lineNo of last reference)
            map<string, size_t> declared_ids; // all ids that where declared by the script (size_t contains lineNo of declaration)

            awt_mask_item_ptr lasthandler;

            // read mask itself :
            while (!error && !mask_ended && !feof(in)) {
                error = readLine(in, line, lineNo);
                if (error) break;

                if (line.empty()) continue;
                if (line[0] == '#') continue;

                if (line == "@MASK_END") {
                    mask_ended = true;
                }
                else  {
                PARSE_REST_OF_LINE:
                    was_last_parameter = false;
                    size_t start       = next_non_white(line, 0);
                    if (start != string::npos) { // line contains sth
                        size_t after_command = line.find_first_of(string(" \t("), start);
                        if (after_command == string::npos) {
                            string command = line.substr(start);
                            error          = GBS_global_string("arguments missing after '%s'", command.c_str());
                        }
                        else {
                            string command    = line.substr(start, after_command-start);
                            size_t paren_open = line.find('(', after_command);
                            if (paren_open == string::npos) {
                                error = GBS_global_string("'(' expected after '%s'", command.c_str());
                            }
                            else {
                                size_t            scan_pos = paren_open+1;
                                awt_mask_item_ptr handler;
                                awt_mask_item_ptr radio_edit_handler;
                                MaskCommand       cmd      = findCommand(command);

                                //  --------------------------------------
                                //      code for different commands :
                                //  --------------------------------------

                                if (cmd == CMD_TEXTFIELD) {
                                    string label, data_path;
                                    long   width          = -1;
                                    label                 = scan_string_parameter(line, scan_pos, error);
                                    if (!error) data_path = scan_string_parameter(line, scan_pos, error);
                                    if (!error) width     = scan_long_parameter(line, scan_pos, error, MIN_TEXTFIELD_SIZE, MAX_TEXTFIELD_SIZE);
                                    check_last_parameter(error, command);

                                    if (!error) handler = new awt_input_field(mask->mask_global(), data_path, label, width, "", GB_STRING);
                                }
                                else if (cmd == CMD_NUMFIELD) {
                                    string label, data_path;
                                    long   width = -1;

                                    long min = LONG_MIN;
                                    long max = LONG_MAX;

                                    label                 = scan_string_parameter(line, scan_pos, error);
                                    if (!error) data_path = scan_string_parameter(line, scan_pos, error);
                                    if (!error) width     = scan_long_parameter(line, scan_pos, error, MIN_TEXTFIELD_SIZE, MAX_TEXTFIELD_SIZE);
                                    if (!was_last_parameter) {
                                        if (!error) min = scan_optional_parameter(line, scan_pos, error, LONG_MIN);
                                        if (!error) max = scan_optional_parameter(line, scan_pos, error, LONG_MAX);
                                    }
                                    check_last_parameter(error, command);

                                    if (!error) handler = new awt_numeric_input_field(mask->mask_global(), data_path, label, width, 0, min, max);
                                }
                                else if (cmd == CMD_CHECKBOX) {
                                    string label, data_path;
                                    bool   checked        = false;
                                    label                 = scan_string_parameter(line, scan_pos, error);
                                    if (!error) data_path = scan_string_parameter(line, scan_pos, error);
                                    if (!error) checked   = scan_bool_parameter(line, scan_pos, error);
                                    check_last_parameter(error, command);

                                    if (!error) handler = new awt_check_box(mask->mask_global(), data_path, label, checked);
                                }
                                else if (cmd == CMD_RADIO) {
                                    parse_CMD_RADIO(line, scan_pos, error, command, handler, radio_edit_handler, mask->mask_global());
                                }
                                else if (cmd == CMD_SCRIPT) {
                                    string id, script;
                                    id                 = scan_identifier(line, scan_pos, error);
                                    if (!error) script = scan_string_parameter(line, scan_pos, error, true);
                                    check_last_parameter(error, command);

                                    if (!error) {
                                        handler          = new awt_script(mask->mask_global(), script);
                                        error            = handler->set_name(id, false);
                                        declared_ids[id] = lineNo;
                                    }
                                }
                                else if (cmd == CMD_GLOBAL || cmd == CMD_LOCAL) {
                                    string id, def_value;

                                    id                 = scan_identifier(line, scan_pos, error);
                                    bool global_exists = mask->mask_global()->has_global_id(id);
                                    bool local_exists  = mask->mask_global()->has_local_id(id);

                                    if ((cmd == CMD_GLOBAL && local_exists) || (cmd == CMD_LOCAL && global_exists)) {
                                        error = GB_export_error("ID '%s' already declared as %s ID (rename your local id)",
                                                                id.c_str(), cmd == CMD_LOCAL ? "global" : "local");
                                    }
                                    else if (cmd == CMD_LOCAL && local_exists) {
                                        error = GB_export_error("ID '%s' declared twice", id.c_str());
                                    }

                                    if (!error) def_value = scan_string_parameter(line, scan_pos, error);
                                    if (!error) {
                                        if (cmd == CMD_GLOBAL) {
                                            if (!mask->mask_global()->has_global_id(id)) { // do not create globals more than once
                                                // and never free them -> so we don't need pointer
                                                new awt_variable(mask->mask_global(), id, true, def_value, error);
                                            }
                                            awt_assert(handler.Null());
                                        }
                                        else {
                                            handler = new awt_variable(mask->mask_global(), id, false, def_value, error);
                                        }
                                        declared_ids[id] = lineNo;
                                    }
                                }
                                else if (cmd == CMD_ID) {
                                    string id = scan_identifier(line, scan_pos, error);
                                    check_last_parameter(error, command);

                                    if (!error) {
                                        if (lasthandler.Null()) {
                                            error = "ID() may only be used BEHIND another element";
                                        }
                                        else {
                                            error            = lasthandler->set_name(id, false);
                                            declared_ids[id] = lineNo;
                                        }
                                    }
                                }
                                else if (cmd == CMD_SHOW) {
                                    string         label, id;
                                    long           width = -1;
                                    awt_mask_item *item  = 0;

                                    label             = scan_string_parameter(line, scan_pos, error);
                                    if (!error) id    = scan_identifier(line, scan_pos, error);
                                    if (!error) item  = (awt_mask_item*)mask->mask_global()->get_identified_item(id, error);
                                    if (!error) width = scan_long_parameter(line, scan_pos, error, MIN_TEXTFIELD_SIZE, MAX_TEXTFIELD_SIZE);
                                    check_last_parameter(error, command);

                                    if (!error) {
                                        awt_mask_awar_item *awar_item = dynamic_cast<awt_mask_awar_item*>(item);

                                        if (awar_item) { // item has an awar -> create a viewport using that awar
                                            handler = new awt_text_viewport(awar_item, label, width);
                                        }
                                        else { // item has no awar -> test if it's a script
                                            awt_script *script  = dynamic_cast<awt_script*>(item);
                                            if (script) handler = new awt_script_viewport(mask->mask_global(), script, label, width);
                                            else        error   = "SHOW cannot be used on this ID-type";
                                        }

                                        referenced_ids[id] = lineNo;
                                    }
                                }
                                else if (cmd == CMD_OPENMASK || cmd == CMD_CHANGEMASK) {
                                    string label, mask_to_start;
                                    label                     = scan_string_parameter(line, scan_pos, error);
                                    if (!error) mask_to_start = scan_string_parameter(line, scan_pos, error);
                                    check_last_parameter(error, command);

                                    if (!error) {
                                        char   *key                    = GBS_string_2_key(label.c_str());
                                        AW_CB   cb                     = cmd == CMD_OPENMASK ? AWT_open_input_mask : AWT_change_input_mask;
                                        string  mask_to_start_internal = find_internal_name(mask_to_start, local);

                                        if (mask_to_start_internal.length() == 0) {
                                            error = "Can't detect which mask to load";
                                        }
                                        else {
                                            string *cl_arg1 = new string(mask->mask_global()->get_internal_maskname());
                                            string *cl_arg2 = new string(mask_to_start_internal);

                                            awt_assert(cl_arg1);
                                            awt_assert(cl_arg2);

                                            aws->callback( cb, (AW_CL)cl_arg1, (AW_CL)cl_arg2);
                                            aws->button_length(label.length()+2);
                                            aws->create_button(key, label.c_str());
                                        }

                                        free(key);
                                    }
                                }
                                else if (cmd == CMD_WWW) {
                                    string button_text, url_srt;
                                    button_text         = scan_string_parameter(line, scan_pos, error);
                                    if (!error) url_srt = scan_string_parameter(line, scan_pos, error, true);
                                    check_last_parameter(error, command);

                                    if (!error) {
                                        char *key = GBS_string_2_key(button_text.c_str());

                                        aws->callback(AWT_input_mask_browse_url, (AW_CL)new string(url_srt), (AW_CL)&*mask);
                                        aws->button_length(button_text.length()+2);
                                        aws->create_button(key, button_text.c_str());

                                        free(key);
                                    }
                                }
                                else if (cmd == CMD_ASSIGN) {
                                    string id_dest, id_source, button_text;

                                    id_dest                 = scan_identifier(line, scan_pos, error);
                                    if (!error) id_source   = scan_identifier(line, scan_pos, error);
                                    if (!error) button_text = scan_string_parameter(line, scan_pos, error);

                                    if (!error) {
                                        referenced_ids[id_source] = lineNo;
                                        referenced_ids[id_dest]   = lineNo;

                                        char *key = GBS_string_2_key(button_text.c_str());

                                        aws->callback(AWT_input_mask_perform_action, (AW_CL)new awt_assignment(mask, id_dest, id_source), 0);
                                        aws->button_length(button_text.length()+2);
                                        aws->create_button(key, button_text.c_str());

                                        free(key);
                                    }
                                }
                                else if (cmd == CMD_TEXT) {
                                    string text;
                                    text = scan_string_parameter(line, scan_pos, error);
                                    check_last_parameter(error, command);

                                    if (!error) {
                                        char *key = GBS_string_2_key(text.c_str());
                                        aws->button_length(text.length()+2);
                                        aws->create_button(key, text.c_str());
                                        free(key);
                                    }
                                }
                                else if (cmd == CMD_SELF) {
                                    check_no_parameter(line, scan_pos, error, command);
                                    if (!error) {
                                        const awt_item_type_selector *sel              = mask->mask_global()->get_selector();
                                        string                        button_awar_name = sel->get_self_awar();
                                        char                         *key              = GBS_string_2_key(button_awar_name.c_str());

                                        aws->button_length(sel->get_self_awar_content_length());
                                        aws->create_button(key, button_awar_name.c_str());
                                        free(key);
                                    }
                                }
                                else if (cmd == CMD_NEW_LINE) {
                                    check_no_parameter(line, scan_pos, error, command);
                                    if (!error) {
                                        int width, height;
                                        aws->get_window_size(width, height);
                                        aws->at_shift(0, 2*SEC_YBORDER+SEC_LINE_WIDTH);
                                    }
                                }
                                else if (cmd == CMD_NEW_SECTION) {
                                    check_no_parameter(line, scan_pos, error, command);
                                    if (!error) {
                                        int width, height;
                                        aws->get_window_size(width, height);
                                        horizontal_lines.push_back(height);
                                        aws->at_shift(0, 2*SEC_YBORDER+SEC_LINE_WIDTH);
                                    }
                                }
                                else if (cmd == CMD_UNKNOWN) {
                                    error = GBS_global_string("Unknown command '%s'", command.c_str());
                                }
                                else {
                                    error = GBS_global_string("No handler for '%s'", command.c_str());
                                    awt_assert(0);
                                }

                                //  --------------------------
                                //      insert handler(s)
                                //  --------------------------

                                if (!handler.Null() && !error) {
                                    if (!radio_edit_handler.Null()) { // special radio handler
                                        const awt_radio_button *radio = dynamic_cast<const awt_radio_button*>(&*handler);
                                        awt_assert(radio);

                                        int x_start, y_start;

                                        aws->get_at_position(&x_start, &y_start);

                                        mask->add_handler(handler);
                                        handler->to_viewport()->build_widget(aws);

                                        int x_end, y_end, dummy;
                                        aws->get_at_position(&x_end, &dummy);
                                        aws->at_newline();
                                        aws->get_at_position(&dummy, &y_end);

                                        int height    = y_end-y_start;
                                        int each_line = height/radio->no_of_toggles();
                                        int y_offset  = each_line*(radio->default_toggle());

                                        aws->at(x_end+x_spacing, y_start+y_offset);

                                        mask->add_handler(radio_edit_handler);
                                        radio_edit_handler->to_viewport()->build_widget(aws);

                                        radio_edit_handler.SetNull();
                                    }
                                    else {
                                        mask->add_handler(handler);
                                        if (handler->is_viewport()) handler->to_viewport()->build_widget(aws);
                                    }
                                    lasthandler = handler; // store handler (used by CMD_ID)
                                }

                                // parse rest of line or abort
                                if (!error) {
                                    line = line.substr(scan_pos);
                                    goto PARSE_REST_OF_LINE;
                                }
                                err_pos = scan_pos;
                                if (err_pos != string::npos) err_pos++; // because 0 means unknown
                            }
                        }
                    }
                    else { // reached the end of the current line
                        aws->at_newline();
                    }
                }
            }

            // check declarations and references
            if (!error) {
                for (map<string, size_t>::const_iterator r = referenced_ids.begin(); r != referenced_ids.end(); ++r) {
                    if (declared_ids.find(r->first) == declared_ids.end()) {
                        error = GB_export_error("ID '%s' used in line #%u was not declared", r->first.c_str(), r->second);
                        aw_message(error);
                    }
                }

                for (map<string, size_t>::const_iterator d = declared_ids.begin(); d != declared_ids.end(); ++d) {
                    if (referenced_ids.find(d->first) == referenced_ids.end()) {
                        const char *warning = GBS_global_string("ID '%s' declared in line #%u is never used in %s",
                                                                d->first.c_str(), d->second, mask_name.c_str());
                        aw_message(warning);
                    }
                }

                if (error) error = "Inconsistent IDs";
            }

            if (!error) {
                if (!horizontal_lines.empty()) { // draw all horizontal lines
                    int width, height;
                    aws->get_window_size(width, height);
                    for (vector<int>::const_iterator yi = horizontal_lines.begin(); yi != horizontal_lines.end(); ++yi) {
                        int y = (*yi)+SEC_YBORDER;
                        aws->draw_line(SEC_XBORDER, y, width-SEC_XBORDER, y, SEC_LINE_WIDTH, AW_TRUE);
                    }
                }

                // fit the window
                aws->window_fit();
            }

            // link to database
            if (!error) link_mask_to_database(mask);
        }

        if (error) {
            if (lineNo == 0) {
                ; // do not change error
            }
            else if (err_pos == 0) { // don't knows exact error position
                error = GBS_global_string("%s in line #%u", error, lineNo);
            }
            else if (err_pos == string::npos) {
                error = GBS_global_string("%s at end of line #%u", error, lineNo);
            }
            else {
                int    wanted         = 35;
                size_t end            = line.length();
                string context;
                context.reserve(wanted);
                bool   last_was_space = false;

                for (size_t ex = err_pos-1; ex<end && wanted>0; ++ex) {
                    char ch            = line[ex];
                    bool this_is_space = ch == ' ';

                    if (!this_is_space || !last_was_space) {
                        context.append(1, ch);
                        --wanted;
                    }
                    last_was_space = this_is_space;
                }

                error = GBS_global_string("%s in line #%u at '%s...'", error, lineNo, context.c_str());
            }
        }

        fclose(in);
    }

    if (error) mask.SetNull();

    return mask;
}

//  -------------------------------------------------------------------------------------------------------------------------------------------------
//      GB_ERROR AWT_initialize_input_mask(AW_root *root, GBDATA *gb_main, const awt_item_type_selector *sel, const char* mask_name, bool local)
//  -------------------------------------------------------------------------------------------------------------------------------------------------
GB_ERROR AWT_initialize_input_mask(AW_root *root, GBDATA *gb_main, const awt_item_type_selector *sel, const char* internal_mask_name, bool local) {
    const char              *mask_name  = internal_mask_name+1;
    InputMaskList::iterator  mask_iter  = input_mask_list.find(internal_mask_name);
    GB_ERROR                 error      = 0;
    awt_input_mask_ptr       old_mask;
    bool                     unlink_old = false;

    static list<awt_input_mask_ptr> mask_collector; // here old (aka reloaded) masks are kept
    // (freeing masks is not possible, because of too many nested callbacks)

    // erase mask (so it loads again from scratch)
    if (mask_iter != input_mask_list.end() && mask_iter->second->reload_on_reinit()) // reload wanted ?
    {
        old_mask  = mask_iter->second;
        input_mask_list.erase(mask_iter);
        mask_iter = input_mask_list.end();

        old_mask->hide();
        mask_collector.push_back(old_mask);
        unlink_old = true;
    }

    if (mask_iter == input_mask_list.end()) { // mask not loaded yet
        awt_input_mask_ptr newMask = awt_create_input_mask(root, gb_main, sel, mask_name, local, error);
        if (error) {
            error = GBS_global_string("Error reading %s (%s)", mask_name, error);
            if (!old_mask.Null()) { // are we doing a reload or changemask ?
                input_mask_list[internal_mask_name] = old_mask; // error loading modified mask -> put old one back to mask-list
                unlink_old                          = false;
            }
        }
        else {
            input_mask_list[internal_mask_name] = newMask;
        }
        mask_iter = input_mask_list.find(internal_mask_name);
    }

    if (!error) {
        awt_assert(mask_iter != input_mask_list.end());
        AW_window_simple *aws  = mask_iter->second->get_window();
        aws->show();
    }

    if (unlink_old) {
        old_mask->relink(true); // unlink old mask from database ()
        unlink_mask_from_database(old_mask);
    }

    if (error) aw_message(error);
    return error;
}

// start of implementation of class awt_input_mask:

//  -----------------------------------------
//      awt_input_mask::~awt_input_mask()
//  -----------------------------------------
awt_input_mask::~awt_input_mask()
{
    for (awt_mask_item_list::iterator h = handlers.begin(); h != handlers.end(); ++h) {
        (*h)->remove_name();
    }
}


//  --------------------------------------
//      void awt_input_mask::relink()
//  --------------------------------------
// this functions links/unlinks all registered item handlers to/from the database
void awt_input_mask::relink(bool unlink) {
    const awt_item_type_selector *sel     = global.get_selector();
    GBDATA                       *gb_item = unlink ? 0 : sel->current(global.get_root());

    for (awt_mask_item_list::iterator h = handlers.begin(); h != handlers.end(); ++h) {
         if ((*h)->is_linked_item()) (*h)->to_linked_item()->link_to(gb_item);
//         if ((*h)->is_input_handler()) (*h)->to_input_handler()->link_to(gb_item);
//         else if ((*h)->is_script_viewport()) (*h)->to_script_viewport()->link_to(gb_item);
    }
}

// -end- of implementation of class awt_input_mask.


//  -------------------------------------------------------------------------------------------------------------------------------------------------------------
//      awt_input_mask_descriptor::awt_input_mask_descriptor(const char *title_, const char *maskname_, const char *itemtypename_, bool local, bool hidden_)
//  -------------------------------------------------------------------------------------------------------------------------------------------------------------
awt_input_mask_descriptor::awt_input_mask_descriptor(const char *title_, const char *maskname_, const char *itemtypename_, bool local, bool hidden_) {
    title                = strdup(title_);
    internal_maskname    = (char*)malloc(strlen(maskname_)+2);
    internal_maskname[0] = local ? '0' : '1';
    strcpy(internal_maskname+1, maskname_);
    //     maskname      = strdup(maskname_);
    itemtypename         = strdup(itemtypename_);
    local_mask           = local;
    hidden               = hidden_;
}
//  ----------------------------------------------------------------
//      awt_input_mask_descriptor::~awt_input_mask_descriptor()
//  ----------------------------------------------------------------
awt_input_mask_descriptor::~awt_input_mask_descriptor() {
    free(itemtypename);
    free(internal_maskname);
    free(title);
}
//  -----------------------------------------------------------------------------------------------------
//      awt_input_mask_descriptor::awt_input_mask_descriptor(const awt_input_mask_descriptor& other)
//  -----------------------------------------------------------------------------------------------------
awt_input_mask_descriptor::awt_input_mask_descriptor(const awt_input_mask_descriptor& other) {
    title             = strdup(other.title);
    internal_maskname = strdup(other.internal_maskname);
    itemtypename      = strdup(other.itemtypename);
    local_mask        = other.local_mask;
    hidden            = other.hidden;
}
//  ------------------------------------------------------------------------------------------------------------------
//      awt_input_mask_descriptor& awt_input_mask_descriptor::operator = (const awt_input_mask_descriptor& other)
//  ------------------------------------------------------------------------------------------------------------------
awt_input_mask_descriptor& awt_input_mask_descriptor::operator = (const awt_input_mask_descriptor& other) {
    if (this != &other) {
        free(itemtypename);
        free(internal_maskname);
        free(title);

        title             = strdup(other.title);
        internal_maskname = strdup(other.internal_maskname);
        itemtypename      = strdup(other.itemtypename);
        local_mask        = other.local_mask;
        hidden            = other.hidden;
    }

    return *this;
}

static bool scanned_existing_input_masks = false;
static vector<awt_input_mask_descriptor> existing_masks;

//  -----------------------------------------
//      static void add_new_input_mask()
//  -----------------------------------------
static void add_new_input_mask(const string& maskname, const string& fullname, bool local) {
    awt_input_mask_descriptor *descriptor = quick_scan_input_mask(maskname, fullname, local);
    if (descriptor) {
        existing_masks.push_back(*descriptor);
        delete descriptor;
    }
}
//  ------------------------------------------------
//      static void scan_existing_input_masks()
//  ------------------------------------------------
static void scan_existing_input_masks() {

    awt_assert(!scanned_existing_input_masks);

    for (int scope = 0; scope <= 1; ++scope) {
        const char *dirname = inputMaskDir(scope == AWT_SCOPE_LOCAL);
        DIR        *dirp    = opendir(dirname);

        if (!dirp) {
            fprintf(stderr, "The path '%s' is invalid.\n", dirname);
        }
        else {
            struct dirent *dp;
            for (dp = readdir(dirp); dp; dp = readdir(dirp)) {
                struct stat st;
                string      maskname = dp->d_name;
                string      fullname = inputMaskFullname(maskname, scope == AWT_SCOPE_LOCAL);

                if (stat(fullname.c_str(), &st)) continue;
                if (!S_ISREG(st.st_mode)) continue;
                // now we have a regular file

                size_t ext_pos = maskname.find(".mask");

                if (ext_pos != string::npos && maskname.substr(ext_pos) == ".mask") {
                    awt_input_mask_descriptor *descriptor = quick_scan_input_mask(maskname, fullname, scope == AWT_SCOPE_LOCAL);
                    if (descriptor) { // we found a input mask file
                        existing_masks.push_back(*descriptor);
                        delete descriptor;
                    }
                }
            }
            closedir(dirp);
        }
    }
    scanned_existing_input_masks = true;
}

//  ---------------------------------------------------------------------
//      const awt_input_mask_descriptor *AWT_look_input_mask(int id)
//  ---------------------------------------------------------------------
const awt_input_mask_descriptor *AWT_look_input_mask(int id) {
    if (!scanned_existing_input_masks) scan_existing_input_masks();

    if (id<0 || size_t(id) >= existing_masks.size()) return 0;

    const awt_input_mask_descriptor *descriptor = &existing_masks[id];
    return descriptor;
}

//  -------------------------------------------------------------------
//      awt_item_type AWT_getItemType(const string& itemtype_name)
//  -------------------------------------------------------------------
awt_item_type AWT_getItemType(const string& itemtype_name) {
    awt_item_type type = AWT_IT_UNKNOWN;

    for (int i = AWT_IT_UNKNOWN+1; i<AWT_IT_TYPES; ++i) {
        if (itemtype_name == awt_itemtype_names[i]) {
            type = awt_item_type(i);
            break;
        }
    }

    return type;
}

// ############################################
// ############################################
// ###                                      ###
// ##          Registered Itemtypes          ##
// ###                                      ###
// ############################################
// ############################################

typedef void (*AWT_OpenMaskWindowCallback)(AW_window* aww, AW_CL cl_id, AW_CL);

//  --------------------------------------
//      class AWT_registered_itemtype
//  --------------------------------------
// stores information about so-far-used item types
class AWT_registered_itemtype {
private:
    AW_window_menu_modes       *awm; // the main window responsible for opening windows
    AWT_OpenMaskWindowCallback  open_window_cb; // callback to open the window

public:
    AWT_registered_itemtype() : awm(0), open_window_cb(0) {}
    AWT_registered_itemtype(AW_window_menu_modes *awm_, AWT_OpenMaskWindowCallback open_window_cb_)
        : awm(awm_)
        , open_window_cb(open_window_cb_)
    {}
    virtual ~AWT_registered_itemtype() {}

    AW_window_menu_modes *getWindow() const { return awm; }
    AWT_OpenMaskWindowCallback getOpenCb() const { return open_window_cb; }
};

static map<awt_item_type, AWT_registered_itemtype> registeredTypes;

//  -------------------------------------------------------------------
//      void openMaskWindowByType(int mask_id, awt_item_type type)
//  -------------------------------------------------------------------
static GB_ERROR openMaskWindowByType(int mask_id, awt_item_type type) {
    map<awt_item_type, AWT_registered_itemtype>::const_iterator registered = registeredTypes.find(type);
    GB_ERROR                                                    error      = 0;

    if (registered == registeredTypes.end()) error = GBS_global_string("Type '%s' not registered (yet)", awt_itemtype_names[type]);
    else registered->second.getOpenCb()(registered->second.getWindow(), (AW_CL)mask_id, (AW_CL)0);

    return error;
}

//  ---------------------------------------------------------------------------------------------------------------------------
//      static void registerType(awt_item_type type, AW_window_menu_modes *awm, AWT_OpenMaskWindowCallback open_window_cb)
//  ---------------------------------------------------------------------------------------------------------------------------
static void registerType(awt_item_type type, AW_window_menu_modes *awm, AWT_OpenMaskWindowCallback open_window_cb) {
    map<awt_item_type, AWT_registered_itemtype>::const_iterator alreadyRegistered = registeredTypes.find(type);
    if (alreadyRegistered == registeredTypes.end()) {
        registeredTypes[type] = AWT_registered_itemtype(awm, open_window_cb);
    }
#if defined(DEBUG)
    else {
//         awt_assert(alreadyRegistered->second.getWindow() == awm);
        awt_assert(alreadyRegistered->second.getOpenCb() == open_window_cb);
    }
#endif // DEBUG
}

// #############################################################
// #############################################################
// ###                                                       ###
// ##          Create a new input mask (interactive)          ##
// ###                                                       ###
// #############################################################
// #############################################################

//  ----------------------------------------------------
//      static void create_new_mask_cb(AW_window *)
//  ----------------------------------------------------
static void create_new_mask_cb(AW_window *aww) {
    AW_root *awr = aww->get_root();

    string maskname = awr->awar(AWAR_INPUT_MASK_NAME)->read_string();
    {
        size_t ext = maskname.find(".mask");

        if (ext == string::npos) maskname = maskname+".mask";
        else maskname                     = maskname.substr(0, ext)+".mask";

        awr->awar(AWAR_INPUT_MASK_NAME)->write_string(maskname.c_str());
    }


    string       itemname     = awr->awar(AWAR_INPUT_MASK_ITEM)->read_string();
    int          scope        = awr->awar(AWAR_INPUT_MASK_SCOPE)->read_int();
    int          hidden       = awr->awar(AWAR_INPUT_MASK_HIDDEN)->read_int();
    bool         local        = scope == AWT_SCOPE_LOCAL;
    string       maskfullname = inputMaskFullname(maskname, local);
    bool         openMask     = false;
    bool         closeWindow  = false;
    const char  *error        = 0;
    struct stat  st;

    if (stat(maskfullname.c_str(), &st) == 0) { // file exists
        int answer = aw_message("File does already exist", "Open mask,Cancel");
        switch (answer) {
            case 0:
                openMask   = true;
                break;
            case 1: break;
            default: awt_assert(0); break;
        }
    }
    else {                      // file does not exist
        error = GB_create_directory(inputMaskDir(local));
        if (!error) {
            error = writeDefaultMaskfile(maskfullname, maskname, itemname, hidden);
        }
        if (!error) {
            add_new_input_mask(maskname, maskfullname, local);
            openMask    = true;
            closeWindow = true;
        }
    }

    if (!error && openMask) {
        int           mask_id;
        awt_item_type item_type = AWT_IT_UNKNOWN;

        for (mask_id = 0; ; ++mask_id) {
            const awt_input_mask_descriptor *descriptor = AWT_look_input_mask(mask_id);
            if (!descriptor) {
                error = GBS_global_string("Can't find descriptor for mask '%s'", maskname.c_str());
                break;
            }

            if (strcmp(descriptor->get_maskname(), maskname.c_str()) == 0 &&
                descriptor->is_local_mask() == local)
            {
                item_type = AWT_getItemType(descriptor->get_itemtypename());
                break;          // found wanted mask id
            }
        }

        if (!error) {
            error = openMaskWindowByType(mask_id, item_type);
        }
    }

    if (error) aw_message(error);
}

//  -------------------------------------------------------------------------------------
//      static void create_new_input_mask(AW_window *aww, AW_CL cl_item_type, AW_CL)
//  -------------------------------------------------------------------------------------
static void create_new_input_mask(AW_window *aww, AW_CL cl_item_type, AW_CL) { // create new user mask (interactively)
    static AW_window_simple *aws = 0;

    if (!aws) {
        aws = new AW_window_simple;

        aws->init(aww->get_root(), "CREATE_USER_MASK", "Create new input mask:");

        aws->auto_space(10, 10);

        aws->button_length(10);
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", 0);
        aws->callback(AW_POPUP_HELP,(AW_CL)"input_mask_new.hlp");
        aws->create_button("HELP", "HELP","H");

        aws->at_newline();

        aws->label("Name of new input mask");
        aws->create_input_field(AWAR_INPUT_MASK_NAME, 20);

        aws->at_newline();

        aws->label("Item type");
        aws->create_option_menu(AWAR_INPUT_MASK_ITEM, "", "");
        for (int i = AWT_IT_UNKNOWN+1; i<AWT_IT_TYPES; ++i) {
            aws->insert_option(awt_itemtype_names[i], "", awt_itemtype_names[i]);
        }
        aws->update_option_menu();

        aws->at_newline();

        aws->label("Scope");
        aws->create_toggle_field(AWAR_INPUT_MASK_SCOPE, 1);
        aws->insert_toggle("Local", "L", AWT_SCOPE_LOCAL);
        aws->insert_toggle("Global", "G", AWT_SCOPE_GLOBAL);
        aws->update_toggle_field();

        aws->at_newline();

        aws->label("Visibility");
        aws->create_toggle_field(AWAR_INPUT_MASK_HIDDEN, 1);
        aws->insert_toggle("Normal", "N", 0);
        aws->insert_toggle("Hidden", "H", 1);
        aws->update_toggle_field();

        aws->at_newline();

        aws->callback(create_new_mask_cb);
        aws->create_button("CREATE", "CREATE","C");

        aws->window_fit();
    }

    aws->show();
    aww->get_root()->awar(AWAR_INPUT_MASK_ITEM)->write_string(awt_itemtype_names[int(cl_item_type)]);
}

// ####################################################################
// ####################################################################
// ###                                                              ###
// ##          Create User-Mask-Submenu for any application          ##
// ###                                                              ###
// ####################################################################
// ####################################################################

//  ------------------------------------------------------------------------------------------------------------------------------------------------------------
//      void AWT_create_mask_submenu(AW_window_menu_modes *awm, awt_item_type wanted_item_type, void (*open_window_cb)(AW_window* aww, AW_CL cl_id, AW_CL))
//  ------------------------------------------------------------------------------------------------------------------------------------------------------------
void AWT_create_mask_submenu(AW_window_menu_modes *awm, awt_item_type wanted_item_type, void (*open_window_cb)(AW_window* aww, AW_CL cl_id, AW_CL)) {
    // add a user mask submenu at current position
    AW_root *awr = awm->get_root();

    if (!global_awars_created) create_global_awars(awr);

    awm->insert_sub_menu(0, "User Masks", "k");

    for (int scope = 0; scope <= 1; ++scope) {
        bool entries_made = false;

        for (int id = 0; ;++id) {
            const awt_input_mask_descriptor *descriptor = AWT_look_input_mask(id);

            if (!descriptor) break;
            if (descriptor->is_local_mask() != (scope == AWT_SCOPE_LOCAL)) continue; // wrong scope type

            awt_item_type item_type = AWT_getItemType(descriptor->get_itemtypename());

            if (item_type == wanted_item_type) {
                if (!descriptor->is_hidden()) { // do not show masks with hidden-flag
                    entries_made = true;
                    char *macroname2key = GBS_string_2_key(descriptor->get_internal_maskname());
#if defined(DEBUG) && 0
                    printf("added user-mask '%s' with id=%i\n", descriptor->get_maskname(), id);
#endif // DEBUG
                    awm->insert_menu_topic(macroname2key, descriptor->get_title(), "", "input_mask.hlp", AWM_ALL, open_window_cb, (AW_CL)id, (AW_CL)0);
                    free(macroname2key);
                }
                registerType(item_type, awm, open_window_cb);
            }
            else if (item_type == AWT_IT_UNKNOWN) {
                aw_message(GBS_global_string("Unkown @ITEMTYPE '%s' in '%s'", descriptor->get_itemtypename(), descriptor->get_internal_maskname()));
            }
        }
        if (entries_made) awm->insert_separator();
    }

    awm->insert_menu_topic("new_mask", "New mask ...", "N", "input_mask_new.hlp", AWM_ALL, create_new_input_mask, (AW_CL)wanted_item_type, (AW_CL)0);
    awm->close_sub_menu();
}


